#ident "$Revision: 1.2 $"
/*--------------------------------------------------------------------*/
/*                                                                    */
/* Copyright 1992-1998 Silicon Graphics, Inc.                         */
/* All Rights Reserved.                                               */
/*                                                                    */
/* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics    */
/* Inc.; the contents of this file may not be disclosed to third      */
/* parties, copied or duplicated in any form, in whole or in part,    */
/* without the prior written permission of Silicon Graphics, Inc.     */
/*                                                                    */
/* RESTRICTED RIGHTS LEGEND:                                          */
/* Use, duplication or disclosure by the Government is subject to     */
/* restrictions as set forth in subdivision (c)(1)(ii) of the Rights  */
/* in Technical Data and Computer Software clause at                  */
/* DFARS 252.227-7013, and/or in similar or successor clauses in      */
/* the FAR, DOD or NASA FAR Supplement.  Unpublished - rights         */
/* reserved under the Copyright Laws of the United States.            */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* amconvert.c                                                        */
/*   amconvert is the program which migrates old availmon configurati */
/*   on to new sss configuration. It is normally run at install time  */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ssdbapi.h>
#include <amssdb.h>
#include <amdefs.h>
#include <oamdefs.h>
#include <amnotify.h>

#define  CHECKFLAG              1
#define  CHECKFLAGNVALUE        2
#define  CHECKNONE              0

#define  OPT_STR		"hC:"

char	cfile[MAX_STR_LEN];
char	*scfile;
char	*tfn = NULL;
char	syscmd[MAX_LINE_LEN];
int	sendtype[SEND_TYPE_NO];
char	addresses[SEND_TYPE_NO][MAX_LINE_LEN];
char	addresses1[SEND_TYPE_NO][MAX_LINE_LEN];
int     adds = 0;
int     deletes = 0;

typedef struct {
    char	*addrp[200];
} addrp_t;

addrp_t	addr[SEND_TYPE_NO], addr1[SEND_TYPE_NO];

ssdb_Client_Handle     Client = 0;
ssdb_Connection_Handle Connection = 0;
ssdb_Request_Handle    Request = 0;
ssdb_Error_Handle      Error = 0;
int  ssdbInitComplete = 0;

/*
 * Function Prototypes
 */

int convertFile(char *, char *);
int initdb(void);
void PrintDBError(char *, ssdb_Error_Handle);
int convertConfig(int);
void sendReport(int);
int updateConfigFlags(char *, char *, int);
int chkconfig(amconf_t, int);
int getword(char *, int, char *);
void getemailaddresses(char *);
int addresscompare(const void *, const void *);
void sortaddresses(addrp_t[]);
void compare(void);
int main(int, char **);


int convertFile(char *logfile, char *oldfile)
{
    return(0);
}

/*
 * initdb
 *   initializes Database Connection & sets up connection
 *
 */

int initdb(void)
{
    int ErrCode = 0;
    
    /*
     * Initialize DB API
     */

    if ( !ssdbInit()) {
	PrintDBError("Init", Error);
	return(0);
    }

    if ( (Error = ssdbCreateErrorHandle()) == NULL ) {
	PrintDBError("CreateErrorHandle", Error);
	ErrCode++;
	goto newend;
    }

    if ( (Client = ssdbNewClient(Error, SSDBUSER, SSDBPASSWD, 0)) == NULL ) {
	PrintDBError("NewClient", Error);
	ErrCode++;
	goto newend;
    }

    if ( !(Connection = ssdbOpenConnection(Error, Client, SSDBHOST,
					   SSDB,
					   (SSDB_CONFLG_RECONNECT|SSDB_CONFLG_REPTCONNECT))) ) {
	PrintDBError("OpenConnection", Error);
	ErrCode++;
	goto newend;
    }

newend:
    if (Error) ssdbDeleteErrorHandle(Error);
    return ( (ErrCode > 0) ? -1 : 0 );

}

/*
 * PrintDBError
 *  Prints last DB Error encountered
 */

void PrintDBError(char *location, ssdb_Error_Handle error)
{
    fprintf(errorfp, "Error in ssdb%s: %s\n", location, 
			    ssdbGetLastErrorString(error));
}

/*
 * convertConfig
 *   Reads old availmon configuration which already exists on a system
 *   and inserts it into the database
 */

int convertConfig(int ConfigOption)
{ 
    amconf_t  i;
    int       j, k;
    int       registration = 0;
    char      word[10];
    char      **tmpaddr;
    struct stat statbuf;

    if ( stat("/var/adm/avail/config", &statbuf) < 0 ) 
        return(0);

    /*
     * lets first check autoemail flag value.  There is a possibility that
     * someone has changed it manually during the time machine was up
     * last time.  We check whether the value in config directory is
     * the same as the value in .save directory.  If they both are different,
     * then we need to either register or de-register accordingly.
     */

    strcpy(cfile, amcs[amc_autoemaillist].filename);

    j = chkconfig(amc_autoemail, 0);
    k = chkconfig(amc_autoemail, 1);

    for ( i = amc_autoemail ; i <= amc_statusinterval; i++ ) {
	snprintf(word, 10, "%d", chkconfig(i, 0));
	if ( updateConfigFlags(amcs[i].flagname, word, CHECKFLAG) != 0 )
	    return(-1); 
	else
	    unlink(amcs[i].filename);
    }

    /*
     * Update tickfile and tickduration values
     */

    updateConfigFlags ("tickfile", "/var/adm/avail/.save/lasttick",CHECKFLAG);
    updateConfigFlags ("tickduration", "300", CHECKFLAG);

    getemailaddresses(cfile);
    sortaddresses(addr);
    
    if ( ConfigOption ) {
	k = chkconfig(amc_autoemail, 1);
	if ( j != k ) registration = 1;
	else {
	    if ( stat(samcs[amc_autoemaillist].filename, &statbuf) == 0 ) {
		scfile = samcs[amc_autoemaillist].filename;
		getemailaddresses(scfile);
		sortaddresses(addr1);
		compare();
		if (adds) sendReport(1);
		if (deletes) sendReport(0);
		unlink(scfile);
	    }
	}

	unlink(samcs[amc_autoemail].filename);
    }

    /*
     * Insert addresses found in config/autoemail.list directory
     * into SSDB.  At this point, addr will contain config/autoemail.list
     * entries.  We will go ahead and insert these in SSDB
     */

    for ( k = 0; k < SEND_TYPE_NO; k++ ) {
	tmpaddr = addr[k].addrp;
	while ( *tmpaddr ) {
	    updateConfigFlags(repTypestr[k], *tmpaddr, CHECKFLAGNVALUE);
	    tmpaddr++;
	}
    }

    if ( registration ) {
	if ( j ) {
	    system("/var/adm/avail/amformat -a -r");
	} else {
	    system("/var/adm/avail/amformat -a -d");
	}
    }

    unlink(cfile);

    return(0);
}

/*
 * sendReport
 *    Sends REGISTRATION/DE-REGISTRATION Reports to necessary reciepents
 */

void sendReport(int regisflag)
{
    char  commandargs[MAXINPUTLINE];
    int   nobytes = 0;
    char  tmpaddrbuffer[MAX_LINE_LEN];
    int   i = 0, j = 0;
    char  *lasts = NULL;
    
    nobytes = snprintf(commandargs, MAXINPUTLINE, 
		       "/var/adm/avail/amformat -%c ",
		       ((regisflag == 1) ? 'r' : 'd'));

    for ( i = 0; i < SEND_TYPE_NO; i++ ) {

	if ( regisflag ) {
	    strcpy(tmpaddrbuffer, addresses[i]);
	    lasts = addresses[i];
	} else {
	    strcpy(tmpaddrbuffer,addresses1[i]);
	    lasts = addresses1[i];
	}

	if ( strtok(tmpaddrbuffer, " ,") != NULL ) {
	    nobytes += snprintf(&commandargs[nobytes], MAXINPUTLINE,
				"-E \"%s:%s\" ",
				repTypestr[i], 
				((*lasts == ',') ? lasts+1 : lasts));
				
	    j++;
	}

    }

    if (j) system(commandargs);
    
}

/*
 * updateConfigFlags
 *    Updates Configuration Flag in the Database.  Inserts if not exists.
 */

int updateConfigFlags(char *configFlag,
		      char *configValue, int flag)
{
    char   sqlstmt[1024];
    int    ErrCode = 0;

    if ( configFlag == NULL ) return(-1);

    switch(flag) {
    case CHECKFLAG:
	sprintf(sqlstmt, "DELETE from %s where %s = '%s' and %s = '%s'",
		TOOLTABLE, TOOLNAME, TOOLCLASS, TOOLOPTION, configFlag);
	break;
    case CHECKFLAGNVALUE:
	sprintf(sqlstmt, "DELETE from %s where %s = '%s' and %s = '%s' and %s = '%s'",
		TOOLTABLE, TOOLNAME, TOOLCLASS, TOOLOPTION, configFlag, 
		OPTIONDEFAULT, configValue);
	break;
    default:
	break;
    }

    if ( flag ) {
	if ( !(Request = ssdbSendRequest(Error, Connection, 
					 SSDB_REQTYPE_DELETE, sqlstmt))) {
	    PrintDBError("SendRequest", Error);
	} else {
	    ssdbFreeRequest(Error, Request);
	}
    }

    sprintf(sqlstmt, "INSERT into %s values('%s', '%s', '%s')",
	    TOOLTABLE, TOOLCLASS, configFlag, configValue);

    if ( !(Request = ssdbSendRequest(Error, Connection, SSDB_REQTYPE_INSERT,
				     sqlstmt))){
	PrintDBError("SendRequest", Error);
	ErrCode++;
    } else 
	ssdbFreeRequest(Error, Request);

    return(ErrCode);
}

/*
 * chkconfig
 *    A simple utility to check if a flag is on or off
 */

int
chkconfig(amconf_t flag, int sflag)
{
    amc_t       *amc;
    char        line[MAX_LINE_LEN];
    FILE        *fp;
    int         confflag = 0;

    if (sflag)
	amc = samcs;
    else
	amc = amcs;

    if ((fp = fopen(amc[flag].filename, "r")) == NULL) {
	if ( flag == amc_statusinterval ) 
	    return 60;
	return 0; 
    }

    /*
     * check "on" or "off".
     */

    if ( fgets(line, MAX_LINE_LEN, fp) == NULL ) {
	confflag = 0;
    } else if ( flag == amc_statusinterval ) {
	confflag = atoi(line);
    } else if ( strncasecmp(line, "on", 2) != 0 ) {
	confflag = 0;
    } else 
	confflag = 1;

    fclose(fp);
    return(confflag);
}

/*
 * getword
 *    Takes a line and returns the wno word delimited by '|'
 * words are numbered 0,1,2 and are in format 0|1|2\n (vertical bar is
 * the field separator)
 */

#define        PIPE                '|'

int
getword(char *line, int wno, char *word)
{
    int i = 0;
    char *p, *p1;

    if (wno) {
	i = wno;
	for (p = line; *p; p++)
	    if (*p == '|')
		if (--i == 0)
		    break;
    }
    if (i)
	return(-1);	/* not enough fields */
    p1 = ++p;
    for (; *p; p++)
	if (*p == '|') {
	    *p = NULL;
	    strcpy(word, p1);
	    *p = PIPE;
	    return(0);
	}
    if (*(--p) == '\n') {
	    *p = NULL;
	    strcpy(word, p1);
	    *p = '\n';
    } else
	strcpy(word, p1);
    return(0);
}

/*
 * getemailaddresses
 *   Reads e-mail addresses from old autoemail.list
 */

void
getemailaddresses(char *cfile)
{
    FILE	*fp;
    char	line[ MAX_LINE_LEN], *ap, *dp;
    int		type, c, len[SEND_TYPE_NO], newaddr;

    if ((fp = fopen(cfile, "r")) == NULL) {
	fprintf(stderr, "Error: %s: cannot open configuration file %s\n",
		"amconvert", cfile);
	exit(1);
    }

    for (type = 0; type < SEND_TYPE_NO; type++) {
	addresses[type][0] = '\0';
	sendtype[type] = 0;
	len[type] = strlen(sendtypestr[type]);
    }

    while (fgets(line, MAX_LINE_LEN, fp)) {
	if (line[0] == '#')
	    continue;

	for (type = 0; type < SEND_TYPE_NO; type++)
	    if (strncmp(line, sendtypestr[type], len[type]) == 0)
		break;

	if (type == SEND_TYPE_NO)
	    continue;

	ap = line + len[type];
	dp = &addresses[type][sendtype[type]];
	newaddr = 1;
	while ((c = *ap++) != '\n') {
	    if ((c == '\0') || (c == '#'))
		break;
	    if ((c == '\t') || (c == ' ') || (c == ',')) {
		newaddr = 1;
		continue;
	    }
	    if (newaddr) {
		newaddr = 0;
		*dp++ = ' ';
		sendtype[type]++;
	    }
	    *dp++ = c;
	    sendtype[type]++;
	}
	*dp++ = ' ';
	*dp = '\0';
    }
    fclose(fp);
}

/*
 * addresscompare
 *    a function used by qsort to compare two e-mail addresses
 */

int
addresscompare(const void *address1, const void *address2)
{
    return(strcmp(*(char **)address1, *(char **)address2));
}

/* 
 * sortaddresses
 *    Takes a addr array and sorts addresses in it.
 */

void
sortaddresses(addrp_t addr[])
{
    int		type, n, len, i;
    char	*ap, *dp, **addrps;

    for (type = 0; type < SEND_TYPE_NO; type++) {
	n = 0;
	addrps = addr[type].addrp;
	if (sendtype[type]) {
	    ap = &addresses[type][1];
	    while (dp = strchr(ap, ' ')) {
		*dp = '\0';
		len = strlen(ap);
		addrps[n] = malloc(len + 1);
		strcpy(addrps[n], ap);
		n++;
		ap = dp + 1;
	    }
	}
	addrps[n] = NULL;
	if (n)
	    qsort(addrps, n, sizeof(char *), addresscompare);
    }
}

/*
 * compare
 *   Compares two addr structures;
 */

void
compare()
{
    int		type, i;
    char	*addrp, *addrp1, **ap, **ap1;
    FILE	*fp;

    adds = deletes = 0;
    for (type = 0; type < SEND_TYPE_NO; type++) {
	addrp = addresses[type];
	addrp1 = addresses1[type];
	*addrp = '\0';
	*addrp1 = '\0';
	ap = addr[type].addrp;
	ap1 = addr1[type].addrp;
	while (*ap && *ap1) {
	    i = strcmp(*ap, *ap1);
	    if (i == 0) {
		ap++;
		ap1++;
	    } else if (i < 0) {
		strcat(addrp, ",");
		strcat(addrp, *ap);
		adds++;
		ap++;
	    } else {
		strcat(addrp1, ",");
		strcat(addrp1, *ap1);
		deletes++;
		ap1++;
	    }
	}
	while (*ap) {
	    strcat(addrp, ",");
	    strcat(addrp, *ap);
	    adds++;
	    ap++;
	}
	while (*ap1) {
	    strcat(addrp1, ",");
	    strcat(addrp1, *ap1);
	    deletes++;
	    ap1++;
	}
    }
}

/*
 * main function
 *  takes the following arguments
 *   -C           Convert old configuration
 *   -L <file>    Insert <file> data into SSDB
 *   -O <ofile>   Insert <ofile> data into SSDB
 */

int main(int argc, char **argv)
{
    int configflag = 0;
    int lflag = 0;
    char *lfilename = NULL;
    char *ofilename = NULL;
    FILE *fp = NULL;
    int  configoptions = 0;
    int oflag = 0;
    int c = 0;
    int errorflag = 0;

    while ( (c = getopt(argc, argv, OPT_STR)) != -1){
	switch(c) {
	case 'h':
	    /*Usage();*/
	    exit(0);
	case 'C':
	    configflag = 1;
	    configoptions = atoi(optarg);
	    break;
	case 'L':
	    lflag = 1;
	    lfilename = (char *) strdup(optarg);
	    break;
	case 'O':
	    oflag = 1;
	    ofilename = (char *) strdup(optarg);
	default:
	    errorflag++;
	    break;
	}
    }

    /*
     * If error, no more processing
     */

    if (errorflag) goto end;

    /*
     * Initialize SSDB Connection.  If we can't initialize
     * then we can't do anything.
     */

    if ( initdb() < 0 ) {
	errorflag++;
	goto end;
    }
	
    /*
     * First convert configuration if specified
     */

    if ( configflag && (convertConfig(configoptions) != 0) ) {
	perror("Error converting configuration");
	exit(-1);
    }

    /*
     * Availmon rotates its /var/adm/avail/availlog file once
     * it reaches a certain size.  If we find availlog and
     * oavaillog, then we first insert oavaillog data and then
     * insert availlog data for continuity sake.  This should give 
     * us a pretty decent history on all systems except large
     * origin systems.  It is a constant problem on large o2k
     * systems that whenever a hinv change is identified, the whole
     * hinv diff takes up all the space in the log file.
     */

    if ( (lflag || oflag) && !convertFile(lfilename,ofilename) ) {
	perror("Error converting old availmon files");
	exit(-1);
    }

end:
    if ( errorflag ) {
	/*Usage();*/
	exit(-1);
    } 

    exit(0);

}



