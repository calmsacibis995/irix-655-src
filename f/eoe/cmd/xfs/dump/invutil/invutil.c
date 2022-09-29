#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <sys/uuid.h>
#include <unistd.h>
#include <sys/mman.h>

#include "types.h"
#include "mlog.h"
#include "inv_priv.h"

#define STR_LEN			INV_STRLEN+32
#define GEN_STRLEN              32

char *g_programName;

char *GetFstabFullPath(void);
char *GetNameOfInvIndex (uuid_t);
void PruneByMountPoint(int, char **);
void CheckAndPruneFstab(bool_t , char *, time_t );
int CheckAndPruneInvIndexFile( bool_t , char *, time_t );
void usage (void);

void main (int argc, char *argv[])
{
    char *tempstr = "test";
    time_t temptime = 0;

    g_programName = argv[0];

    if (( argc == 4 ) && ( strcasecmp( argv[1], "-M" ) == 0 ) )
    {
	PruneByMountPoint(argc, argv);
    }
    else if (( argc == 2 ) && ( strcasecmp( argv[1], "-C" ) == 0 ))
    {
	CheckAndPruneFstab(BOOL_TRUE,tempstr,temptime); 
    }
    else
	usage();
}

void PruneByMountPoint(int argc, char **argv)
{
    char *dummy, monS[3], dayS[3], yearS[5];
    time_t timeSecs; /* command line time in secs since Unix Epoch */
    struct tm timeStr;

    if ( strlen( argv[argc -1] ) != 10 ) 
    {
	fprintf( stderr, "%s: Improper format for time\n", 
	    g_programName );
	usage();
    }

    /* Better to use getdate here? */
    dummy = (char *) malloc( 11 );
    memset( monS, 0, 3 );
    memset( dayS, 0, 3 );
    memset( yearS, 0, 5 );
    strcpy( dummy, argv[3] );
    strtok( dummy, "/" );
    strcpy( monS, dummy );
    monS[2]='\0';
    strcpy( dummy, argv[3]+strlen(monS)+1 );
    strtok( dummy, "/" );
    strcpy( dayS, dummy );
    dayS[2]='\0';
    strcpy( dummy, argv[3]+strlen(monS)+strlen(dayS)+2 );
    strcpy( yearS, dummy );
    timeStr.tm_year = atoi(yearS) - 1900;
    timeStr.tm_mon = atoi(monS) - 1;
    timeStr.tm_mday = atoi(dayS);
    timeStr.tm_hour = 0;
    timeStr.tm_min = 0;
    timeStr.tm_sec = 1;
    timeStr.tm_isdst = -1;
    timeSecs = mktime( &timeStr );
    /* printf("the date entered is %s/%s/%s\n", monS, dayS, yearS); */
    /* printf("the date entered in secs is %d\n", timeSecs); */

    CheckAndPruneFstab(BOOL_FALSE , argv[2] , timeSecs);

}

char *GetNameOfInvIndex (uuid_t uuid)
{
    char *str,*name;
    uint_t stat;

    uuid_to_string( &uuid, &str, &stat );
    if (stat != uuid_s_ok)
    {
	fprintf( stderr, "%s: Bad uuid\n", g_programName);
	fprintf( stderr, "%s: abnormal termination\n", g_programName );
	exit(1);
    }

    name = (char *) malloc( strlen( INV_DIRPATH ) + 1  + strlen( str ) 
			     + strlen( INV_INVINDEX_PREFIX ) + 1);
    strcpy( name, INV_DIRPATH );
    strcat( name, "/" );
    strcat( name, str );
    strcat( name, INV_INVINDEX_PREFIX );

    return(name);
}

char *GetFstabFullPath(void)
{
    char *fstabname;

    fstabname = (char *) malloc( strlen(INV_DIRPATH) + 1 /* one for the "/" */
				   + strlen("fstab") + 1 );
    strcpy( fstabname, INV_DIRPATH );
    strcat( fstabname, "/" );
    strcat( fstabname, "fstab" );
    return(fstabname);
}

void CheckAndPruneFstab(bool_t checkonly, char *mountPt, time_t prunetime)
{
    char *fstabname, *mapaddr, *invname,*uuidStr , response[GEN_STRLEN];
    int fstabEntries,nEntries;
    int  fd,i,j;
    uint_t uuidstatus;
    bool_t removeflag;
    invt_fstab_t *fstabentry;
    invt_counter_t *counter,cnt;

    fstabname = GetFstabFullPath();
    fd = open( fstabname, O_RDWR );
    if (fd == -1)
    {
	fprintf( stderr, "%s: unable to open file %s\n", g_programName, 
	    fstabname );
	fprintf( stderr, "%s: abnormal termination\n", g_programName );
	exit(1);
    }
    
    read(fd,&cnt,sizeof(invt_counter_t) );
    nEntries = cnt.ic_curnum;

    lseek( fd, 0, SEEK_SET );
    mapaddr = (char *) mmap( 0, (nEntries*sizeof(invt_fstab_t))
		     + sizeof(invt_counter_t),
        (PROT_READ|PROT_WRITE), MAP_SHARED, fd, 0 );

    if (mapaddr == (char *)-1)
    {
	fprintf( stderr, 
	    "%s: error in mmap at %d with errno %d for file %s\n", 
	    g_programName, __LINE__, errno, fstabname );
	fprintf( stderr, "%s: abnormal termination\n", g_programName );
	perror( "mmap" );
	exit(1);
    }

    counter = (invt_counter_t *)mapaddr;
    fstabentry = (invt_fstab_t *)(mapaddr + sizeof(invt_counter_t));

    printf("Processing file %s\n",fstabname);

    /* check each entry in fstab for mount pt match */
    for (i = 0; i < counter->ic_curnum; )
    {
	removeflag = BOOL_FALSE;

	printf("   Found entry for %s\n" , fstabentry[i].ft_mountpt);

	for (j = i +1 ; j < counter->ic_curnum ; j++ )
	    if (uuid_equal(&(fstabentry[i].ft_uuid), &(fstabentry[j].ft_uuid),
		&uuidstatus) == B_TRUE)
	    {
		printf("     duplicate fstab entry\n");
		removeflag = BOOL_TRUE;
		break;
	    }
	
	if (!removeflag)
	{
	    bool_t IdxCheckOnly = BOOL_TRUE, GotResponse = BOOL_FALSE;

	    invname = GetNameOfInvIndex(fstabentry[i].ft_uuid);

	    if (( checkonly == BOOL_FALSE ) && 
		 (strcmp( fstabentry[i].ft_mountpt, mountPt ) == 0))
	    {
		uuid_to_string( &(fstabentry[i].ft_uuid), &uuidStr, 
							  &uuidstatus );
		printf("-------------------------------------------------\n");
		printf("\nAn entry matching the mount point is :\n");
		printf( "UUID\t\t:\t%s\nMOUNT POINT\t:\t%s\nDEV PATH\t:\t%s\n",
		    uuidStr, fstabentry[i].ft_mountpt, 
			     fstabentry[i].ft_devpath );
		while ( GotResponse == BOOL_FALSE )
		{
		    printf("\nDo you want to prune this entry: [y/n]");
		    gets( response );
		    if (strcasecmp( response, "Y" ) == 0)
		    {
			IdxCheckOnly = BOOL_FALSE;
			GotResponse = BOOL_TRUE;
		    }
		    else if (strcasecmp( response, "N" ) == 0)
		    {
			GotResponse = BOOL_TRUE;
		    }
		}
		printf("-------------------------------------------------\n\n");
	    }

	    if ( CheckAndPruneInvIndexFile( IdxCheckOnly, invname , prunetime )
				   == -1 )
		 removeflag = BOOL_TRUE;

	    free( invname );

	}

	if (removeflag == BOOL_TRUE)
	{
	    printf("     removing fstab entry %s\n", fstabentry[i].ft_mountpt);
	    if ( counter->ic_curnum > 1 )
	        bcopy((void *)&fstabentry[i + 1], (void *)&fstabentry[i],
		    (sizeof(invt_fstab_t) * (counter->ic_curnum - i - 1)));
	    counter->ic_curnum--;
	}
	else 
	    i++; /* next entry if this entry not removed */
    }

    fstabEntries = counter->ic_curnum;

    munmap( mapaddr, (nEntries*sizeof(invt_fstab_t))
			 + sizeof(invt_counter_t) );

    if ((fstabEntries != 0)  && (fstabEntries != nEntries))
          ftruncate(fd, sizeof(invt_counter_t) + 
		(sizeof(invt_fstab_t) * fstabEntries));

    close(fd);

    if (fstabEntries == 0)
    {
       unlink( fstabname );
    }

    free( fstabname );
}

int CheckAndPruneInvIndexFile( bool_t checkonly, char *idxFileName , 
						 time_t prunetime ) 
{
    char *temp;
    int fd;
    int i, validEntries,nEntries;
    bool_t removeflag;

    invt_entry_t *invIndexEntry;
    invt_counter_t *counter,header;

    printf("      processing index file \n"
	   "       %s\n",idxFileName);
    errno=0;
    fd = open( idxFileName, O_RDWR );
    if (fd == -1)
    {
	fprintf( stderr, "      %s: open of %s failed with errno %d\n",
	    g_programName, idxFileName, errno );
	perror( "open" );
	return(-1);
    }

    read( fd, &header, sizeof(invt_counter_t) );
    nEntries = header.ic_curnum;

    lseek( fd, 0, SEEK_SET );
    errno = 0;
    temp = (char *) mmap( NULL, (nEntries*sizeof(invt_entry_t))
				 + sizeof(invt_counter_t),
	(PROT_READ|PROT_WRITE), MAP_SHARED, fd,
	0 );

    if (temp == (char *)-1)
    {
	fprintf( stderr, 
	    "%s: error in mmap at %d with errno %d for file %s\n", 
	    g_programName, __LINE__, errno, idxFileName );
	fprintf( stderr, "%s: abnormal termination\n", g_programName );
	perror( "mmap" );
	exit(1);
    }

    counter = (invt_counter_t *)temp;
    invIndexEntry = (invt_entry_t *)( temp + sizeof(invt_counter_t));

    for (i=0; i < counter->ic_curnum; )
    {
	removeflag = BOOL_FALSE;
	errno = 0;
	printf("         Checking access for\n"
	       "          %s\n", invIndexEntry[i].ie_filename);

	if (( access( invIndexEntry[i].ie_filename, R_OK | W_OK ) == -1)  &&
	   (errno == ENOENT))
	{
	    printf("         Unable to access %s referred in %s\n",
		invIndexEntry[i].ie_filename, idxFileName);
	    printf("         removing index entry \n");
	    removeflag = BOOL_TRUE;
	}    

	if (( !removeflag ) && (checkonly == BOOL_FALSE) && 
		( invIndexEntry[i].ie_timeperiod.tp_end < prunetime))
	{
	    printf("           Pruning matching index entry corresponding to\n"
		   "           %s\n", invIndexEntry[i].ie_filename );
	    unlink( invIndexEntry[i].ie_filename );
	    removeflag = BOOL_TRUE;
	}

	if (removeflag == BOOL_TRUE)
	{
	    if ( counter->ic_curnum > 1 )
	        bcopy((void *)&invIndexEntry[i + 1], (void *)&invIndexEntry[i],
		    (sizeof(invt_entry_t) * (counter->ic_curnum - i - 1)));
	    counter->ic_curnum--;
	}
	else 
	    i++; /* next entry if this entry not removed */
    }

    validEntries = counter->ic_curnum;

    munmap( temp, (nEntries*sizeof(invt_entry_t)) + sizeof(invt_counter_t) );

    if ((validEntries != 0)  && (validEntries != nEntries))
    	ftruncate( fd, sizeof(invt_counter_t) +
	    (validEntries * sizeof(invt_entry_t)) );

    close( fd );

    if (validEntries == 0)
    {
       unlink( idxFileName );
       return(-1);
    }

    return(0);
}

void usage (void)
{
    fprintf( stderr, "%s: version 1.0\n", g_programName );
    fprintf( stderr, "check or prune the xfsdump inventory\n" );
    fprintf( stderr, "usage: \n" );
    fprintf( stderr, "xfsinvutil [ -M mountPt mm/dd/yyyy ] ( prune all entries\n"
		     "                  older than specified date\n"
		     "                  for the specified mount point )\n" );
    fprintf( stderr, "           [ -C ]  ( check and fix xfsdump inventory\n"
		     "                  database inconsistencies ) \n" );
    exit(1);
}

