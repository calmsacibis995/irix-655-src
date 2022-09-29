/* =====================================================================\
 * This program generates the software, hardware and the system         /
 * reports. These include both the current configuration report and the \
 * archive configuration report. These reports are printed to HTML.     /
 * =====================================================================\
 */

  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <string.h>
  #include <sys/systeminfo.h>
  #include <klib/klib.h>
  #include <sss/configmon_api.h>
  #include <sys/syssgi.h>
  #include <signal.h>
  #include <time.h>

/* Include all the header files */

  #include <rgPluginAPI.h>
  #include <sscHTMLGen.h>
  #include "Dlist.h"

/* --------------------------------------------------------------------------- */
#ifdef __TIME__
#ifdef __DATE__
#define INCLUDE_TIME_DATE_STRINGS 1
#endif
#endif
/* --------------------------------------------------------------------------- */
#define MYVERSION_MAJOR    1 /* Report Generator PlugIn Major Version number */
#define MYVERSION_MINOR    0 /* Report Generator PlugIn Minor Version number */
#define CMREPORT_DEBUG     0
#define ONEDAY             (24*3600) /* one day in seconds */
/* --------------------------------------------------------------------------- */

typedef struct hwtimestamp {
    struct hwtimestamp *next;
    struct hwtimestamp *before;

    Dlist_t *Dlhead, *Dltail;

    time_t req;		/* request time  */
    time_t lastnow; 	/* last session which looked at this structure */
    time_t timestamp;	/* time stamp. */
    cm_hndl_t config;
    cm_hndl_t first_hwcmp;

} hwtimestamp_t;

typedef struct cm_session {
    unsigned long signature;            /* sizeof(cm_session_t) */
    struct cm_session *next;       /* next pointer */
    int textonly;                       /* text mode only flag */
    
    cm_value_t sys_id;
    cm_hndl_t config;
    cm_hndl_t first_hwcmp;

    int config_start_time,config_end_time; /* for cm changes. */

    time_t req;			/* request time */
    time_t now;			/* the time this session was created. */
    time_t timestamp;		/* time stamp. */

    char *strDate;
    char *strTime;
    char *BoardSerial;
    char *dbname;
    int  tmpInt;		/* integer currently used for swcmp. */
} cm_session_t;

#define BEGINHWROW()       RowBegin("ALIGN=\"TOP\"");
#define ENDHWROW()         RowEnd();

#define BEGINHWROW_TXT()   FormatedBody ( "<pre>  " );
#define ENDHWROW_TXT()     FormatedBody ( "</pre>"  );

#define CFG_BEGIN_RGSTRING "<A HREF=\"/$sss/RG/cmreport~"
#define CFG_END_RGSTRING   "\">"
#define CFG_END_HREF       "</A>"

#define START 0
#define END 1

#define HWDRILLDOWN_PLUS     11
#define HWDRILLDOWN_MINUS    12
#define HWDRILLDOWN_PLUSPLUS 13

/* Include all the report generator header files */

#define MAXLEN 20
/* Flag to print out the SELECT statement */
extern int database_debug = 0;         

/* --------------------------------------------------------------------------- */
static const char myLogo[]            = "Configuration Report Server";
static const char szVersion[]         = "Version";
static const char szTitle[]           = "Title";
static const char szThreadSafe[]      = "ThreadSafe";
static const char szUnloadable[]      = "Unloadable";
static const char szUnloadTime[]      = "UnloadTime";
static const char szBoardSerial[]     = "board_history_serial";
static const char szConfigStartTime[] = "config_start_time";
static const char szConfigEndTime[]   = "config_end_time";
static const char szConfigSysId[]     = "sys_id";
static const char szConfigStartItem[] = "start_Item";
static const char szConfigDbName[]    = "dbname";

static const char szAcceptRawCmdString[] = "AcceptRawCmdString";

static char szServerNameErrorPrefix[] = "Configuration Report Server Error: %s";
static char szVersionStr[64];

static pthread_mutex_t seshFreeListmutex;
static pthread_mutex_t hwtimestamp_mutex;
static const char szUnloadTimeValue[] = "1200"; /* Unload time for this plug in (sec.) */
static const char szThreadSafeValue[] = "0";   /* "Thread Safe" flag value - this plugin is thread safe */
static const char szUnloadableValue[] = "1";   /* "Unloadable" flag value - RG core might unload this plugin from memory */
static const char szAcceptRawCmdStringValue[] = "0"; /* Do not accept raw strings. */

static int volatile mutex_inited      = 0;   /* seshFreeListmutex mutex inited flag */
static cm_session_t *sesFreeList      = 0;   /* Session free list */
static char *dbname = "ssdb";
static hwtimestamp_t *phwtHead=NULL,*phwtTail=NULL;

static void DlistFree(hwtimestamp_t *h);

static long converttimetoseconds(char *timeval)
{
    char *Pchar;
    int h,m,s;
    long seconds;

    Pchar=strtok(timeval,":");
    h=atoi(Pchar);
    Pchar=strtok(NULL,":");
    m=atoi(Pchar);
    Pchar=strtok(NULL,":");
    s=atoi(Pchar);

    seconds=h*3600+m*60+s;
    return seconds;
}

static long maketime(const char *dateval1,int startend)
{
    time_t timevalue;
    struct tm timestruct;
    struct tm *timeptr;
    char dateval[40];
    int i;
    char *p;
    strcpy (dateval,dateval1);
    p = strtok(dateval,"/");
    i = atoi(p);
    i = i - 1;
    timestruct.tm_mon = i;
    p = strtok(NULL,"/");
    timestruct.tm_mday = atoi(p);
    p = strtok(NULL,"/");
    i = atoi(p);
    i = i - 1900;
    timestruct.tm_year = i;
    timestruct.tm_isdst = -1;
    if (!startend)
    {
	timestruct.tm_sec = 00;
	timestruct.tm_min = 00;
	timestruct.tm_hour = 00;
    }
    else
    {
	timestruct.tm_sec = 59;
	timestruct.tm_min = 59;
	timestruct.tm_hour = 23;
    }
    timevalue =  mktime(&timestruct);
    return (timevalue);
}

static hwtimestamp_t *hwtimestamp_Find(time_t t)
{
    hwtimestamp_t *phwtTemp=phwtHead;

    while(phwtTemp)
    {
	if(phwtTemp->timestamp == t)
	    return phwtTemp;
	phwtTemp=phwtTemp->next;
    }
    
    return NULL;
}

static hwtimestamp_t *hwtimestamp_Create(time_t t)
{
    hwtimestamp_t *phwtTemp=kl_alloc_block(sizeof(hwtimestamp_t),CM_TEMP);
    
    memset(phwtTemp,sizeof(hwtimestamp_t),0);
    phwtTemp->timestamp=t;
    
    return phwtTemp;
}

static void hwtimestamp_Remove(hwtimestamp_t *phwtTemp)
{
    
    if(!phwtTemp)
	return;

    if(phwtTail && phwtTemp == phwtTail)
	phwtTail = phwtTemp->before;

    if(phwtHead && phwtTemp == phwtHead)
	phwtHead = phwtTemp->next;

    if(phwtTemp->before)
	(phwtTemp->before)->next=phwtTemp->next;

    if(phwtTemp->next)
	(phwtTemp->next)->before=phwtTemp->before;
    
    kl_free_block(phwtTemp);
    
    return;
}

static void hwtimestamp_Free()
{
    while(phwtHead)
    {
	if(phwtHead->config)
	{
	    cm_free_config(phwtHead->config);
	}
	DlistFree(phwtHead);
	hwtimestamp_Remove(phwtHead);
    }
}

static void hwtimestamp_check_and_Free()
{
    time_t t=time(NULL);
    hwtimestamp_t *phwtTemp=phwtHead;

    while(phwtTemp)
    {
	if(phwtTemp->lastnow <= 0 || (t > (phwtTemp->lastnow+ONEDAY)))
	{
	    if(phwtTemp->config)
	    {
		cm_free_config(phwtTemp->config);
	    }
	    DlistFree(phwtTemp);
	    hwtimestamp_Remove(phwtTemp);
	    phwtTemp=phwtHead;
	}
	else
	    phwtTemp=phwtTemp->next;
    }
}

static hwtimestamp_t *hwtimestamp_Init(time_t t)
{
    hwtimestamp_t *phwtTemp=NULL;

    phwtTemp = hwtimestamp_Create(t);
    
    if(!phwtTemp)
	return NULL;

    phwtHead=phwtTail=phwtTemp;

    return phwtTemp;
}

#if CMREPORT_DEBUG
static void hwtimestamp_Count()
{
    hwtimestamp_t *phwtTemp;
    int i=0;

    phwtTemp=phwtHead;

    while(phwtTemp)
    {
	phwtTemp = phwtTemp->next;
	i++;
    }
    fprintf(stderr,"************************* TIMESTAMPS=%d\n",i);
}
#endif

static void hwtimestamp_AddEnd(hwtimestamp_t *h)
{
    if(!h)
	return;

    if(!phwtHead)
    {
	phwtHead=phwtTail=h;
	h->before=h->next=NULL;
	return;
    }
    
    if(!phwtTail)
	return;			/* should never happen */
    
    if(phwtTail)
    {
	phwtTail->next=h;
	h->before=phwtTail;
	h->next=NULL;
	phwtTail=h;
    }
    return;
}

/* 
 * Dlist means doubly linked list. Functions to traverse, add, delete and
 * free nodes in the doubly linked list below. They all take as first argument 
 * another doubly linked list (code above) which is the hardware timestamp.
 */

/*
 * Add after a node.
 */
static void DlistAddAfter(hwtimestamp_t *h,Dlist_t *tail,Dlist_t *elem)
{
  if(!tail)
    {
      elem->next=elem->before=NULL;

      if(!h->Dltail)
	  h->Dltail = elem;
      if(!h->Dlhead)
	  h->Dlhead = elem;

      return;
    }

  if(h->Dltail == tail)
      h->Dltail = elem;

  elem->next=tail->next;
  tail->next=elem;

  elem->before=tail;
}

/*
 * Remove node from doubly linked list.
 */
static void DlistRemove(hwtimestamp_t *h,Dlist_t *elem)
{
  if(!elem)			/* Silly check */
    return;

  if(h->Dltail && elem == h->Dltail)
    h->Dltail = elem->before;

  if(h->Dlhead && elem == h->Dlhead)
    h->Dlhead = elem->next;

  if(elem->before)
    ((Dlist_t *)elem->before)->next=elem->next;

  if(elem->next)
    ((Dlist_t *)elem->next)->before=elem->before;

  kl_free_block(elem);
}

static void DlistFree(hwtimestamp_t *h)
{
    while(h->Dlhead)
	DlistRemove(h,h->Dlhead);
}

#if CMREPORT_DEBUG
static void DlistCount(hwtimestamp_t *h)
{
    Dlist_t *tmpDl=h->Dlhead;
    int i=0;

    while(tmpDl)
    {
	tmpDl = tmpDl->next;
	i++;
    }
    fprintf(stderr,"************************* C=%d\n",i);
}
#endif

static Dlist_t * DlistCreate_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp)
{
    Dlist_t *tmpDl=NULL;
    
    tmpDl=kl_alloc_block(sizeof(Dlist_t),CM_TEMP);
    tmpDl->cmp=cmp;
    tmpDl->children_present_in_dlist=FALSE;
    tmpDl->grandchildren_present_in_dlist=FALSE;

    return tmpDl;
}

/*
 * Find Dlist by cm_hndl_t
 */
static int DlistFind_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp,Dlist_t **Dl)
{
    Dlist_t *tmpDl=h->Dlhead;

    while(tmpDl)
    {
	if(tmpDl->cmp == cmp)
	{
	    *Dl = tmpDl;
	    return 0;
	}
	tmpDl = tmpDl->next;
    }
    return -1;
}

static int DlistDelete_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return -1;

    if(tmpDl == h->Dlhead)
	return -1;

    DlistRemove(h,tmpDl);

    return 0;
}

static int DlistNext_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp,cm_hndl_t *pCmp)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return -1;
    
    tmpDl = tmpDl->next;

    if(tmpDl)
	*pCmp=tmpDl->cmp;
    else
	*pCmp=NULL;

    return 0;
}

static int DlistInsert_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t before_cmp,
				  cm_hndl_t elem_cmp)
{
    Dlist_t *tmpDl0=NULL, *tmpDl1 = NULL;

    /* 
     * If this is not there then this is a problem.
     */
    if(DlistFind_by_cm_hndl(h,before_cmp,&tmpDl0))
	        return -1;

    /* 
     * If this is already in there this is a problem.
     */
    if(!DlistFind_by_cm_hndl(h,elem_cmp,&tmpDl1))
	        return -1;
    
    tmpDl1 = DlistCreate_by_cm_hndl(h,elem_cmp);
    
    if(!tmpDl1)
	return -1;

    DlistAddAfter(h,tmpDl0,tmpDl1);
    
    return 0;
}

static int DlistHasChildren_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return 0;

    return tmpDl->children_present_in_dlist==TRUE;
}

static int DlistHasGrandChildren_by_cm_hndl(hwtimestamp_t *h,cm_hndl_t cmp)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return 0;

    return tmpDl->grandchildren_present_in_dlist==TRUE;
}

static int DlistSetChildren(hwtimestamp_t *h,cm_hndl_t cmp,int trueorfalse)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return -1;

    tmpDl->children_present_in_dlist=trueorfalse;
    
    return 0;
}

static int DlistSetGrandChildren(hwtimestamp_t *h,cm_hndl_t cmp,
				 int trueorfalse)
{
    Dlist_t *tmpDl=NULL;
    
    if(DlistFind_by_cm_hndl(h,cmp,&tmpDl))
	return -1;

    tmpDl->grandchildren_present_in_dlist=trueorfalse;
    
    return 0;
}


static int DlistInit(hwtimestamp_t *h,cm_hndl_t cmp)
{
     Dlist_t *tmpDl=NULL;
     
     tmpDl = DlistCreate_by_cm_hndl(h,cmp);

     if(!tmpDl)
	 return -1;
     
     h->Dlhead=h->Dltail=tmpDl;

     return 0;
}


static void generate_begin_background(cm_session_t *sess, char *title,int help)
{
  if ( sess->textonly == 0 )
  {
    Body("<HTML> <HEAD> <TITLE>SGI Embedded Support Partner - Ver. 1.0</TITLE></HEAD> <BODY BGCOLOR=\"#ffffcc\" vlink=\"#333300\" link=\"#333300\">\n");
    Body("<form method=POST>\n");
    TableBegin("border=0 cellpadding=0 cellspacing=0 width=100%");
    RowBegin("");
      CellBegin("bgcolor=\"#cccc99\" width=\"15\"");
	 Body("&nbsp;&nbsp;&nbsp;&nbsp;\n");
      CellEnd();
      CellBegin("bgcolor=\"#cccc99\" ");
	   Body("<font face=\"Arial,Helvetica\">\n");
	   FormatedBody(title);
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("COLSPAN=2");
	   Body("&nbsp;\n");
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("COLSPAN=2");
	   Body("&nbsp;\n");
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("");
	   Body("&nbsp;\n");
      CellEnd();
      CellBegin("");
  } 
  else 
  {
    /* Lynx */
    Body("<HTML> <HEAD> <TITLE>Embedded Support Partner - Ver. 1.0</TITLE></HEAD> <BODY BGCOLOR=\"#ffffcc\">\n");
    FormatedBody("<pre>   %s</pre>",title);
    Body("<hr width=100%>\n");
  }    
  
  return;
}

static void generate_end_background(cm_session_t *sess)
{
  if ( sess->textonly == 0 )
  {
      CellEnd();
    RowEnd();
   TableEnd();
   Body("</form>\n");
  } 
  else
  {
   Body("</body></html>\n");
  } 
}

static void generate_error_page(cm_session_t *sess, char *title,char *msg)
{
  if ( sess->textonly == 0 )
  {
   Body("<HTML> <HEAD> <TITLE>SGI Embedded Support Partner - Ver. 1.0</TITLE> </HEAD> <BODY BGCOLOR=\"#ffffcc\">\n");
   TableBegin("border=0 cellpadding=0 cellspacing=0 width=100%");
    RowBegin("");
      CellBegin("bgcolor=\"#cccc99\" width=\"15\"");
	 Body("&nbsp;&nbsp;&nbsp;&nbsp;\n");
      CellEnd();
      CellBegin("bgcolor=\"#cccc99\" ");
	   Body("<font face=\"Arial,Helvetica\">\n");
	   FormatedBody(title);
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("COLSPAN=2");
	   Body("&nbsp;<P>&nbsp;\n");
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("");
	   Body("&nbsp;\n");
      CellEnd();
      CellBegin("");
       	   FormatedBody(msg);
   } 
   else
   {
    Body("<HTML> <HEAD> <TITLE>Embedded Support Partner - Ver. 1.0</TITLE></HEAD> <BODY BGCOLOR=\"#ffffcc\">\n");
    FormatedBody("<pre>   %s</pre>",title);
    Body("<hr width=100%>\n");
    FormatedBody("<p>%s",msg);
   }    	   
   generate_end_background ( sess );
}

/*-------------*/
/*System Report*/
/*-------------*/

static int sys_report(sscErrorHandle hError,cm_session_t *session,int flag)
{
    cm_hndl_t dbcmd;
    cm_hndl_t icmp;
    cm_hndl_t item;
    char systype[6];
    char *ip_address, *hostname, *sys_serial, *tbuf;
    char buf[MAXSYSIDSIZE];
    cm_value_t sys_id;
    cm_hndl_t config;
    
    if(!session)
    {
	return -1;
    }
    
    if (!(config=session->config)) {
	return(-1);
    }
    sys_id= session->sys_id;
    
    /* Allocate the structure for the SYSTEM table */
    dbcmd = cm_alloc_dbcmd(config,SYSINFO_TYPE);
    
    /* Make the SELECT statement */
    cm_add_condition(dbcmd,SYS_SYS_ID,OP_EQUAL,(cm_value_t)sys_id);
    
    switch (flag) 
    {

    case 1:
        Body("<P>\n");
        Body("<P>\n");
        if (!(icmp = cm_select_list(dbcmd,ACTIVE_FLG))) 
	{
	    cm_free_dbcmd(dbcmd);
	    return(1);
	}
	break;
 
    case 2:
        FormatedBody("%s","ARCHIVE CONFIGURATION");
        Body("<P>\n");
        Body("<P>\n");
        if (!(icmp = cm_select_list(dbcmd,ARCHIVE_FLG))) 
	{
	    cm_free_dbcmd(dbcmd);
	    return(1);
	}
	break;
    case 3:
        FormatedBody("%s","ENTIRE CONFIGURATION");
        Body("<P>\n");
        Body("<P>\n");
        if (!(icmp = cm_select_list(dbcmd,ALL_FLG))) 
	{
	    cm_free_dbcmd(dbcmd);
	    return(1);
	}
	break;
   default:
        break;
    } 

    /* Select the list */
  if ( session->textonly == 0 )
  {
    TableBegin("BORDER=0 CELLSPACING=0 CELLPADDING=0 NOSAVE");
    item = cm_item(icmp,CM_FIRST);
    if (item) 
    {
	sprintf(systype, "IP%d",
                CM_SIGNED(cm_field(item, SYSINFO_TYPE, SYS_SYS_TYPE)));
	ip_address = CM_STRING(cm_field(item, SYSINFO_TYPE, SYS_IP_ADDRESS));
	hostname = CM_STRING(cm_field(item, SYSINFO_TYPE, SYS_HOSTNAME));
	sys_serial = CM_STRING(cm_field(item, SYSINFO_TYPE, 
					SYS_SERIAL_NUMBER));
	
        RowBegin("valign=top");
	CellBegin("");
	         Body("System name\n");
        CellEnd();
	CellBegin("");
	         Body("&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;\n");
        CellEnd();
        CellBegin("");
                 FormatedBody("%15s",hostname);
        CellEnd();
	RowEnd();

	RowBegin("valign=top");
	CellBegin("");
	         Body("System ID\n");
        CellEnd();
	CellBegin("");
	         Body("&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;\n");
        CellEnd();
	CellBegin("");
	         FormatedBody("%12llX",sys_id);
	CellEnd();
	RowEnd();
	     
        RowBegin("valign=top");
	CellBegin("");
	         Body("System serial number\n");
	CellEnd();
	CellBegin("");
	         Body("&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;\n");
        CellEnd();
	CellBegin("");
	         FormatedBody("%13s",sys_serial);
	CellEnd();
	RowEnd();

        RowBegin("valign=top");
	CellBegin("");
	         Body("IP type\n");
	CellEnd();
	CellBegin("");
	         Body("&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;\n");
        CellEnd();
	CellBegin("");
                 FormatedBody("%8s",systype);
	CellEnd();
	RowEnd();

        RowBegin("valign=top");
	CellBegin("");
	         Body("System IP address\n");
	CellEnd();
	CellBegin("");
	         Body("&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;\n");
        CellEnd();
        CellBegin("");
                 FormatedBody("%-15s",ip_address);
        CellEnd();
        RowEnd();
    }
    TableEnd();
  } 
  else   
  {
     /* Lynx stuff */  
    TableBegin("BORDER=0 CELLSPACING=0 CELLPADDING=0 NOSAVE");
    item = cm_item(icmp,CM_FIRST);
    if (item) 
    {
	sprintf(systype, "IP%d", 
	             CM_SIGNED(cm_field(item, SYSINFO_TYPE, SYS_SYS_TYPE)));
	ip_address = CM_STRING(cm_field(item, SYSINFO_TYPE, SYS_IP_ADDRESS));
	hostname   = CM_STRING(cm_field(item, SYSINFO_TYPE, SYS_HOSTNAME));
	sys_serial = CM_STRING(cm_field(item, SYSINFO_TYPE, SYS_SERIAL_NUMBER));

        FormatedBody("<pre>   System name          : %s</pre>",hostname);
        FormatedBody("<pre>   System ID            : %-12llX</pre>",sys_id);
        FormatedBody("<pre>   System serial number : %-13s</pre>",sys_serial);
	FormatedBody("<pre>   IP type              : %-8s</pre>",systype);
	FormatedBody("<pre>   System IP address    : %-15s</pre>",ip_address);
    }
  } /* End of lynx */
   
/* Print the stuff from the table into HTML */
    cm_free_list(config,icmp);
    cm_free_dbcmd(dbcmd);

    return(0);
}

/*---------------*/
/*Software Report*/
/*---------------*/

static int generate_sw_report_html(cm_session_t *sess,char *str,int start,int flag)
{
   if ( sess->textonly == 0 )
   {
    FormatedBody("%s%d~%d?%s=%s&%s=%s&%s=%d&%s=%llx %s%s%s",CFG_BEGIN_RGSTRING,2,flag,"config_date",sess->strDate,"config_time",sess->strTime,szConfigStartItem,start,szConfigSysId,sess->sys_id,CFG_END_RGSTRING,str,CFG_END_HREF);
   } 
   else 
   {
     /* Lynx */
    FormatedBody ( "%s%d~%d?%s=%s&%s=%s&%s=%d&%s=%llx %s%s%s",
                   CFG_BEGIN_RGSTRING,
                   2,
                   flag,
                   "config_date",
                   sess->strDate,
                   "config_time",
                   sess->strTime,
                   szConfigStartItem,
                   start,
                   szConfigSysId,
                   sess->sys_id,
                   CFG_END_RGSTRING,
                   str,
                   CFG_END_HREF );
   }
    
    return 0;
}

static int sw_report(sscErrorHandle hError,cm_session_t *session,int flag)
{
    cm_hndl_t swcmp;
    cm_hndl_t item;
    time_t req,t;
    int rec_key = 0, version = 0,install_time = 0;
    char *name, *description, *tbuf, *tbuf1;
    cm_value_t sys_id;
    cm_hndl_t config;
    char *title="SYSTEM INFORMATION &gt; Software";
    int total_sw_items=0;
    int sw_item;
    int i;
    
    if(!session)
    {
	return -1;
    }

    if (!(config=session->config)) {
	sscError(hError,szServerNameErrorPrefix,
		 "Config not allocated in sw_report");
	return(-1);

    }

    if(session->req > session->now)
	req=0;
    else
	req=session->req;

    if(cm_get_sysconfig(config,session->sys_id,req))
    {
	generate_error_page( session, title,
			    "This information is not available currently.");
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
	return 0;
    }
    swcmp = cm_first_swcmp(config);
    
    generate_begin_background(session, title,0);
    sys_report(hError,session,1);
    
    if ( session->textonly == 0 )
    {
      Body("<P><HR><P>\n");
    } 
    else
    { /* Lynx */
      Body("<HR width=100%>\n");
    }

    item = swcmp;
    i=sw_item=total_sw_items=0;
    while (item) 
    {
	item = cm_get_swcmp(item,CM_NEXT);
	total_sw_items++;
    }

    /* 
     * This should not happen. But just in case...
     */
    if(session->tmpInt >total_sw_items)
	session->tmpInt=0;

    item = swcmp;
    while(item && sw_item < session->tmpInt)
    {
	item = cm_get_swcmp(item,CM_NEXT);
	sw_item++;
    }

    if(!item)
    {
	generate_error_page(session, title,
			    "Information not available currently.");
	return 0;
    }

  if ( session->textonly == 0 )
  {
    TableBegin("border=0 cellpadding=0 cellspacing=0");
    RowBegin("");
      CellBegin("align=right ");
	 FormatedBody("Page %d of %d",(session->tmpInt/10+1),((total_sw_items+(10-1))/10));
      CellEnd();
    RowEnd();
    RowBegin("");
      CellBegin("");
        TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=7 WIDTH=100% NOSAVE");
           RowBegin("ALIGN=\"CENTER\"");
             CellBegin("");
                     Body("<B>\n");
                     Body("Name\n");
             CellEnd();
             CellBegin("");
                     Body("<B>\n");
                     Body("Version\n");
             CellEnd();
             CellBegin("");
                     Body("<B>\n");
                     Body("Install Date\n");
             CellEnd();
             CellBegin("");
                     Body("<B>\n");
                     Body("Description\n");
              CellEnd();
             RowEnd();
  } 
  else
  {
    FormatedBody("<p>Page %d of %d<br>",(session->tmpInt/10+1),((total_sw_items+(10-1))/10));

    FormatedBody("<pre>");
    FormatedBody("  |----------------------------------------------------------------------|\n");
    FormatedBody("  | Name                                          Version        Install |\n");
    FormatedBody("  | Description                                                    Date  |\n");
    FormatedBody("  |----------------------------------------------------------------------|\n");
  }
          
        while (item && i<10) 
        {
	    tbuf = (char *)kl_alloc_block(100, CM_TEMP);
	    t = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, SW_INSTALL_TIME));
	    cftime(tbuf, "%m/%d/%Y", &t);
	    tbuf1 = (char *)kl_alloc_block(100, CM_TEMP);
	    t = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, 
				     SW_DEINSTALL_TIME));
	    cftime(tbuf1, "%m/%d/%Y", &t);
	    rec_key = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, SW_REC_KEY));
	    name = CM_STRING(cm_field(item, SWCOMPONENT_TYPE, SW_NAME));
	    version = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, SW_VERSION));
	    description = CM_STRING(cm_field(item, SWCOMPONENT_TYPE,
					     SW_DESCRIPTION));
        	
           if ( session->textonly == 0 )
           {
	    RowBegin("ALIGN=\"TOP\"");
		 CellBegin("");
		 FormatedBody("%-16s",name);
		 CellEnd();
		 CellBegin("");
		 FormatedBody("%u",version);
		 CellEnd();
		 CellBegin("");
		 FormatedBody("%10s",tbuf);
		 CellEnd();
		 CellBegin("");
		 FormatedBody("%30s",description);
		 CellEnd();
            RowEnd();
           } 
           else 
           { /* Lynx */
                 FormatedBody("  | %-16.16s                              %-10u   %-10.10s|\n",name,version,tbuf);
                 FormatedBody("  | %-69.69s|\n",description);
                 FormatedBody("  |----------------------------------------------------------------------|\n");
           } 
        
       	    kl_free_block(tbuf);
            kl_free_block(tbuf1);
            item = cm_get_swcmp(item,CM_NEXT);
	    i++;
        }
        
   if ( session->textonly == 0 )
   {
        TableEnd();
      CellEnd();
    RowEnd();
    RowBegin("");
       CellBegin("align=center");
	   i=session->tmpInt/10+1;
           Body("<BR>\n");
   }
   else 
   {
     i=session->tmpInt/10+1;
     Body("</pre>   \n");
   }        
           
           
           if(i>10)
	   {
	       /* 
		* put beginning arrows.
		* start event of previous page = (tmpInt-100)/100*100
		*/
		
	       if ( session->textonly == 0 )
	       {
	       generate_sw_report_html(session,
				       "<img src=\"/images/double_arrow_left.gif\" border=0 alt=\"First page\">",
				       0,flag);
	       Body("&nbsp;&nbsp;\n");
	       generate_sw_report_html(session,
				       "<img src=\"/images/arrow_left.gif\" "
				       "border=0 alt=\"Previous 10 pages\">",
				       (session->tmpInt)/100*100-10,flag);
	       Body("&nbsp;&nbsp;\n");
	       } 
	       else 
	       { /* Lynx */
	       generate_sw_report_html(session, "&lt;", (session->tmpInt)/100*100-10, flag);
	       Body("&nbsp;&nbsp;\n");
	       generate_sw_report_html(session, "&lt;&lt;",  0,flag);
	       Body("&nbsp;&nbsp;\n");
	       }
	   }
	   
	   for(i=0;i<10;i++)
	   {
	       char buf[1024];
	       /* 
		* put the number = (tmpInt)/100*10+i+1
		*/
	       if((session->tmpInt/10+1)%10 == (i+1)%10)
	       {
		   FormatedBody("<b><font color=\"#cc6633\">%d</b>",(session->tmpInt)/100*10+i+1);
		   Body("&nbsp;&nbsp;			\n");
	       }
	       else
	       {
		   sprintf(buf,"<b>%d</b>",
			   (session->tmpInt)/100*10+i+1);
		   generate_sw_report_html(session,buf,
				    (i*10+(session->tmpInt/100*100)),flag);
		   Body("&nbsp;&nbsp;\n");
	       }
	       if((session->tmpInt/100*10+i+1)*10>(total_sw_items-1))
	       {
		   break;
	       }
	   }
	   if(((session->tmpInt+100)/100*100)<(total_sw_items))
	   {
	       /*
		* put ending arrows.
		*/
	       if ( session->textonly == 0 )
	       {
	       generate_sw_report_html(session,
				       "<img src=\"/images/arrow_right.gif\" "
				       "border=0 alt=\"Next 10 pages\">",
				(session->tmpInt+100)/100*100,flag);
	       Body("&nbsp;&nbsp;\n");
	       generate_sw_report_html(session,
				       "<img src=\"/images/double_arrow_right.gif\" border=0 alt=\"Last page\">",
				       (total_sw_items-1)/10*10,flag);
	       Body("&nbsp;&nbsp;\n");
	       }		
	       else
	       { /* Lynx */
	       generate_sw_report_html(session, "&gt;&gt;", (total_sw_items-1)/10*10,flag);
	       Body("&nbsp;&nbsp;\n");
	       generate_sw_report_html(session,	"&gt;",  (session->tmpInt+100)/100*100,flag);
	       Body("&nbsp;&nbsp;\n");
	       }
	   }
	   
   if ( session->textonly == 0 )
   {
       CellEnd();
    RowEnd();
    TableEnd();


   } 
   else 
   {
    Body("<HR width = 100%>\n");
    Body("<p><a href = \"/index_sem.txt.html\">Return to Main page</a>\n");
   }   
   
   generate_end_background(session);

   cm_free_config(config);
   session->config=NULL;
   return(0);
}            

/*-----------------*/
/*H/W stuff to html*/
/*-----------------*/

static int display_hw_url(cm_session_t *session, cm_hndl_t cmp)
{
    hwtimestamp_t *h;

    if(!session)
	return -1;

    if(!(h=hwtimestamp_Find(session->timestamp)))
    {				
	return -1;
    }
    
    if(DlistHasChildren_by_cm_hndl(h,cmp))
    {				/* Put the minus */
      if ( session->textonly == 0 )
      {
	CellBegin("width=20% ALIGN =CENTER");
	     TableBegin("BORDER=0 CELLSPACING=0 CELLPADDING=0");
	     RowBegin("");
	     if(DlistHasGrandChildren_by_cm_hndl(h,cmp) == FALSE)
	     {
	        CellBegin("");
	            FormatedBody("%s%d~0x%x~%d?%s=%llx&%s=%s %s<img src=\"/images/double_arrow_right.gif\" border=0 alt=\"Expand all levels below\">%s",CFG_BEGIN_RGSTRING,HWDRILLDOWN_PLUSPLUS,cmp,session->timestamp,szConfigSysId,session->sys_id,szConfigDbName,session->dbname,CFG_END_RGSTRING,CFG_END_HREF);
	        CellEnd();
	        CellBegin("");
	            Body("&nbsp;&nbsp;\n");
		CellEnd();
	     }
	        CellBegin("");
	            FormatedBody("%s%d~0x%x~%d?%s=%llx&%s=%s %s<img src=\"/images/arrow_down.gif\" border=0 alt=\"Collapse all levels below\">%s",CFG_BEGIN_RGSTRING,HWDRILLDOWN_MINUS,cmp,session->timestamp,szConfigSysId,session->sys_id,szConfigDbName,session->dbname,CFG_END_RGSTRING,CFG_END_HREF);
	        CellEnd();
	     RowEnd();
	     TableEnd();
        CellEnd();
      } 
      else 
      { 
       /* Lynx */
       /* We are not going to do it in Lynx */
       /*
       if(DlistHasGrandChildren_by_cm_hndl(h,cmp) == FALSE)
       {
         FormatedBody ( "%s%d~0x%x~%d?%s=%llx %s--%s",
                        CFG_BEGIN_RGSTRING,
                        HWDRILLDOWN_PLUSPLUS, 
                        cmp,
                        session->timestamp,
                        szConfigSysId,
                        session->sys_id,
                        CFG_END_RGSTRING,
                        CFG_END_HREF );
       }
       */
       FormatedBody ( "|%s%d~0x%x~%d?%s=%llx %s[-]%s|", 
                       CFG_BEGIN_RGSTRING, 
                       HWDRILLDOWN_MINUS, 
                       cmp, 
                       session->timestamp, 
                       szConfigSysId, 
                       session->sys_id, 
                       CFG_END_RGSTRING, 
                       CFG_END_HREF );
      } /* End of  Lynx  */  
    }
    else if (cm_get_hwcmp(cmp,CM_LEVEL_DOWN))
    {				/* Put the plus */
      if ( session->textonly == 0 )
      {
	CellBegin("width=20% ALIGN=CENTER");
	     TableBegin("BORDER=0 CELLSPACING=0 CELLPADDING=0 ");
	     RowBegin("");
	        CellBegin("");
	            FormatedBody("%s%d~0x%x~%d?%s=%llx&%s=%s %s<img src=\"/images/double_arrow_right.gif\" border=0 alt=\"Expand all levels below\">%s",CFG_BEGIN_RGSTRING,HWDRILLDOWN_PLUSPLUS,cmp,session->timestamp,szConfigSysId,session->sys_id,szConfigDbName,session->dbname,CFG_END_RGSTRING,CFG_END_HREF);
	        CellEnd();
	        CellBegin("");
	            Body("&nbsp;&nbsp;\n");
		CellEnd();
		CellBegin("");
		    FormatedBody("%s%d~0x%x~%d?%s=%llx&%s=%s %s<img src=\"/images/arrow_right.gif\" border=0 alt=\"Expand one level below\">%s",CFG_BEGIN_RGSTRING,HWDRILLDOWN_PLUS,cmp,session->timestamp,szConfigSysId,session->sys_id,szConfigDbName,session->dbname,CFG_END_RGSTRING,CFG_END_HREF);
		CellEnd();
             RowEnd();
             TableEnd();
	CellEnd();
      }	
      else
      {
        /* We are not going to do this for Lynx 
	FormatedBody ( "%s%d~0x%x~%d?%s=%llx %s++%s",
	               CFG_BEGIN_RGSTRING,
	               HWDRILLDOWN_PLUSPLUS,
	               cmp,
	               session->timestamp,
	               szConfigSysId,
	               session->sys_id,
	               CFG_END_RGSTRING,
	               CFG_END_HREF );
	*/
	               
	FormatedBody ( "|%s%d~0x%x~%d?%s=%llx %s[+]%s|",
	               CFG_BEGIN_RGSTRING,
	               HWDRILLDOWN_PLUS,
	               cmp,
	               session->timestamp,
	               szConfigSysId,
	               session->sys_id,
	               CFG_END_RGSTRING,
	               CFG_END_HREF );
      }
    }
    else
    {
      if ( session->textonly == 0 )
      {
	CellBegin("width=20%");
	     Body("&nbsp;\n");
        CellEnd();
      } else {
        FormatedBody("|   |");
      }  
    }

    return 0;
}

static int display_hw_html ( cm_session_t *session, cm_hndl_t item )
{
    char *hw_name, *part_number, *serial_number, *location, *revision;
    int sequence = 0, level = 0;

    hw_name       = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_NAME));
    location      = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_LOCATION));
    part_number   = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_PART_NUMBER));
    serial_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_SERIAL_NUMBER));
    revision      = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_REVISION));
    sequence      = CM_SIGNED(cm_field(item,HWCOMPONENT_TYPE, HW_SEQ));
    level         = CM_SIGNED(cm_field(item,HWCOMPONENT_TYPE, HW_LEVEL));
    
    if (sequence == 0) 
    {
	return(-1);
    }

   if ( session->textonly == 0 )
   {
#if 0
    CellBegin("width=10%");
	 Body("<B>\n");
	 FormatedBody("%5d",sequence);
    CellEnd();
    CellBegin("width=10%");
         Body("<B>\n");
         FormatedBody("%5d",level);
    CellEnd();
#endif
    CellBegin("COLSPAN=13");
         FormatedBody("%14s",hw_name?hw_name:"NA");
    CellEnd();
    CellBegin("");
         FormatedBody("%8s",location?location:"NA");
    CellEnd();
    CellBegin("");
         FormatedBody("%12s",part_number?part_number:"NA");
    CellEnd();
    CellBegin("");
         FormatedBody("%10s",serial_number?serial_number:"NA");
    CellEnd();
    CellBegin("");
         FormatedBody("%8s",revision?revision:"NA");
    CellEnd();
    } 
    else 
    { /* Lynx */
    
      /*     
      FormatedBody ( "<pre>  -----------------------------------------------------------------------|</pre>" );
      FormatedBody ( "<pre>  |   |Name              | Location | Part Number |Serial Number|Revision|</pre>" );
      FormatedBody ( "<pre>  -----------------------------------------------------------------------|</pre>" );
      */
      FormatedBody ( "%-18.18s|",  hw_name ? hw_name : "NA"         ); 
      FormatedBody ( "%-10.10s|",  location? location: "NA"         );
      FormatedBody ( "%-13.13s|",  part_number?part_number:"NA"     );
      FormatedBody ( "%-13.13s|",  serial_number?serial_number:"NA" );
      FormatedBody ( "%-8.8s|"  ,  revision?revision:"NA"           );
    }
    
    return(1);
 }
 
/*---------------*/
/*Hardware Report*/
/*---------------*/

static void delete_children(hwtimestamp_t *h,cm_hndl_t cmp)
{
    cm_hndl_t cm_child;
    Dlist_t *tmpCmp=NULL;

    if(!cmp)
	return;

    if(DlistFind_by_cm_hndl(h,cmp,&tmpCmp))
	return;

    CHILDREN_INSERTED(h,cmp,FALSE);
    GRANDCHILDREN_INSERTED(h,cmp,FALSE);

    cm_child=cm_get_hwcmp(cmp,CM_LEVEL_DOWN);

    while(cm_child)
    {
	delete_children(h,cm_child);
	DELETE_DLIST(h,cm_child);
	cm_child=cm_get_hwcmp(cm_child,CM_NEXT_PEER);
    }
    return;
}

static void insert_children(hwtimestamp_t *h,cm_hndl_t cmp)
{
    cm_hndl_t cm_next;
    Dlist_t *tmpCmp=NULL;
    int down=0;
    cm_hndl_t firstcmp=cmp;

    if(!cmp)
	return;

    if(DlistFind_by_cm_hndl(h,cmp,&tmpCmp))
	return;
    
    cm_next = cm_get_hwcmp(cmp,CM_LEVEL_DOWN);

    if(cm_next)
    {
	CHILDREN_INSERTED(h,cmp,TRUE);
    }

    while(cm_next)
    {
	INSERT_DLIST(h,cmp,cm_next);
	cmp=cm_next;
	if(cm_get_hwcmp(cmp,CM_LEVEL_DOWN))
	    down++;
	cm_next = cm_get_hwcmp(cmp,CM_NEXT_PEER);
    }

    if(!down)
    {
	GRANDCHILDREN_INSERTED(h,firstcmp,TRUE);
    }

    return;
}

static void insert_peers(hwtimestamp_t *h,cm_hndl_t cmp)
{
    cm_hndl_t cm_next;
    Dlist_t *tmpCmp=NULL;
    cm_hndl_t firstcmp=cmp;

    if(!cmp)
	return;

    if(DlistFind_by_cm_hndl(h,cmp,&tmpCmp))
	return;
    
    cm_next = cm_get_hwcmp(cmp,CM_NEXT_PEER);

    while(cm_next)
    {
	INSERT_DLIST(h,cmp,cm_next);
	cmp=cm_next;
	cm_next = cm_get_hwcmp(cmp,CM_NEXT_PEER);
    }

    return;
}



static void insert_all_children(hwtimestamp_t *h,cm_hndl_t cmp)
{
    cm_hndl_t cm_next;
    Dlist_t *tmpCmp=NULL;
    cm_hndl_t cmp_first=cmp;

    if(!cmp)
	return;

    if(DlistFind_by_cm_hndl(h,cmp,&tmpCmp))
	return;
    

    cm_next = cm_get_hwcmp(cmp_first,CM_LEVEL_DOWN);

    if(cm_next)
    {
	CHILDREN_INSERTED(h,cmp,TRUE);
	GRANDCHILDREN_INSERTED(h,cmp,TRUE);
    }

    while(cm_next)
    {
	INSERT_DLIST(h,cmp,cm_next);
	cmp=cm_next;
	cm_next = cm_get_hwcmp(cmp,CM_NEXT_PEER);
    }

    /* 
     * Now start inserting any nodes further below.
     */
    cm_next = cm_get_hwcmp(cmp_first,CM_LEVEL_DOWN);
    while(cm_next)
    {
	insert_all_children(h,cm_next);
	cm_next = cm_get_hwcmp(cm_next,CM_NEXT_PEER);
    }
    return;
}

static void display_hwcmp(cm_session_t *s,cm_hndl_t cmp)
{
    if ( s->textonly == 0 )
    {
      BEGINHWROW();
      display_hw_url (s,cmp);
      display_hw_html(s,cmp);
      ENDHWROW();
    } 
    else 
    {
      BEGINHWROW_TXT();
      display_hw_url (s,cmp);
      display_hw_html(s,cmp);
      ENDHWROW_TXT();
    }  
}

static void display_hw(sscErrorHandle hError,cm_session_t *session)
{
    hwtimestamp_t *h;
    cm_hndl_t cmp;
    char *title="SYSTEM INFORMATION &gt; Hardware";
    
    if(!session)
	return;

    if(!(h=hwtimestamp_Find(session->timestamp)))
    {		
	generate_error_page(session, title,
			    "This information is not available "
			    "currently.");
	
	return;
    }

    if(!(cmp=session->first_hwcmp))
    {
	generate_error_page(session, title,
			    "This information is not available "
			    "currently.");
	return;
    }

    generate_begin_background(session, title,0);
    sys_report(hError,session,1);
    
   if ( session->textonly == 0 )    
   {
    Body("<P><HR><P>\n");

    TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=20 WIDTH=60% NOSAVE");
       RowBegin("\"ALIGN=\"CENTER\"");
#if 0
         CellBegin("width=10%");
                 Body("<B>\n");
                 FormatedBody("SEQUENCE");
         CellEnd();
         CellBegin("width=10%");
                 Body("<B>\n");
                 FormatedBody("LEVEL");
         CellEnd();
#endif
         CellBegin("");
                 Body("&nbsp;\n");
	 CellEnd();
         CellBegin("COLSPAN=13");
                 Body("<B>\n");
                 FormatedBody("Name");
         CellEnd();
         CellBegin("");
                 Body("<B>\n");
                 FormatedBody("Location");
         CellEnd();
         CellBegin("");
                 Body("<B>\n");
                 FormatedBody("Part Number");
         CellEnd();
         CellBegin("");
                 Body("<B>\n");
                 FormatedBody("Serial Number");
         CellEnd();
         CellBegin("");
                 Body("<B>\n");
                 FormatedBody("Revision");
	 CellEnd();
      RowEnd();
    } 
    else
    {   /* Lynx */
      FormatedBody ( "<pre>  ------------------------------------------------------------------------</pre>" );
      FormatedBody ( "<pre>  |   |Name              | Location | Part Number |Serial Number|Revision|</pre>" );
      FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
    }  

    while(cmp)
    {
	display_hwcmp(session,cmp);
	if(NEXT_DLIST(h,cmp,&cmp))
	    break;
    }

   if ( session->textonly == 0 )    
   {
    TableEnd();
   } 
   else 
   { /* Lynx */
     FormatedBody ( "<pre>  ------------------------------------------------------------------------</pre>" );
     FormatedBody ( "<a href = \"/index_sem.txt.html\">Return to Main page</a>" );
   }
    
   generate_end_background(session);
	
   return;
}

static void select_all(sscErrorHandle hError,cm_session_t *session,
                       cm_hndl_t cmp)
{
    hwtimestamp_t *h;

    if(!session || !cmp)
        return;

    if(!(h=hwtimestamp_Find(session->timestamp)))
    {				
	return;
    }

    insert_all_children(h,cmp);

    return;
}

static void select_plus(sscErrorHandle hError,cm_session_t *session,
                        cm_hndl_t cmp)
{
    hwtimestamp_t *h;

    if(!session || !cmp)
	return;

    if(!(h=hwtimestamp_Find(session->timestamp)))
    {			
	return;
    }
    insert_children(h,cmp);

    return;
}

static void select_minus(sscErrorHandle hError,cm_session_t *session,
                         cm_hndl_t cmp)
{
    hwtimestamp_t *h;

    if(!session || !cmp)
	return;
    
    if(!(h=hwtimestamp_Find(session->timestamp)))
    {				
	return;
    }
    delete_children(h,cmp);
    
    return;
}	

static int hw_report(sscErrorHandle hError,cm_session_t *session,
		     int plusorminus,
		     cm_hndl_t cmp,time_t argT)
{
    Dlist_t *tmpCmp=NULL;
    char *title="SYSTEM INFORMATION &gt; Hardware";
    hwtimestamp_t *h;
    cm_hndl_t config;
    time_t req;
    
    if(!session)
	return -1;

    if (!session->config) {
	sscError(hError,szServerNameErrorPrefix,
		 "Config not allocated in hw_report");
	return(-1);
    }
	
    pthread_mutex_lock(hwtimestamp_mutex);
    if(!(h=hwtimestamp_Find(argT)))
    {				
	/* 
	 * Reset stuff if this click is not from the same session.
	 */
	cmp=0;
	plusorminus=0;
    }
    else
    {
	if(config=session->config)
	{
	    cm_free_config(config);
	    session->config=NULL;
	}

	session->timestamp=argT;
	session->req=h->req;
	session->config=h->config;
	session->first_hwcmp=h->first_hwcmp;

	h->lastnow=session->now;
    }
    pthread_mutex_unlock(hwtimestamp_mutex);

    if(!plusorminus)
    {

	h=hwtimestamp_Create(session->now);
	config=session->config;
	session->timestamp=session->now;

	if(session->req > session->now)
	    req=0;
	else
	    req=session->req;
	if(cm_get_sysconfig(config,session->sys_id,req))
	{
	    generate_error_page(session, title,
				"This information is not available "
				"currently.");
	    return 0;
	}

	session->first_hwcmp = cm_first_hwcmp(config);
	if(session->first_hwcmp)
	{
	    session->first_hwcmp = cm_get_hwcmp(session->first_hwcmp,CM_NEXT);
	}
	h->first_hwcmp=session->first_hwcmp;
	h->config=config;
	h->req=session->req;
	h->lastnow=session->now;

	DlistInit(h,session->first_hwcmp);
	insert_peers(h,session->first_hwcmp);
	pthread_mutex_lock(hwtimestamp_mutex);
	hwtimestamp_AddEnd(h);
	pthread_mutex_unlock(hwtimestamp_mutex);
    } else if (cmp)
    {		
	if(plusorminus == HWDRILLDOWN_PLUS)
	{
	    select_plus(hError,session,cmp);
	}
	else if (plusorminus == HWDRILLDOWN_PLUSPLUS)
	{
	    select_all(hError,session,cmp);
	}
	else
	{
	    select_minus(hError,session,cmp);
	}
    }
	    

    display_hw(hError,session);

#if CMREPORT_DEBUG
    DlistCount(h);
#endif

    pthread_mutex_lock(hwtimestamp_mutex);
    hwtimestamp_check_and_Free();
    pthread_mutex_unlock(hwtimestamp_mutex);    
    return 0;
}


/*-------------*/
/*Change Report*/
/*-------------*/
static int sys_change_report(sscErrorHandle hError,cm_session_t *session)
{
    cm_hndl_t cm_history,cm_event;
    cm_hndl_t change_item,icmp,item;
    time_t change_time;
    int type = 0,item_type = 0,count = 0;
    char *tbuf, *tbuf1;
    char *ip_address, *hostname, *sys_serial;
    char systype[6];
    int rec_key = 0, version = 0,install_time = 0;
    char *name, *description;
    char *hw_name, *part_number, *serial_number, *location, *revision;
    cm_value_t sys_id;
    cm_hndl_t config;
    int sys=0,sw=0,hw=0;
    char *title="SYSTEM INFORMATION  &gt; System Changes";
    char *initbuf=(char *)kl_alloc_block(100, CM_TEMP);
    time_t inittime=0;

    if(!session)
    {
	generate_error_page(session, title,
			    "No system changes are currently available.");
	return -1;
    }
    
    if (!(config=session->config)) {
	generate_error_page(session, title,
		 "Config not allocated in sys_change_report");
	return(-1);
    }
    sys_id= session->sys_id;
    
    if (!(cm_history = cm_event_history(config,sys_id,0,0,ALL_CM_EVENTS))) {
        generate_error_page(session, title,"No historical information available.");
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
        return(1);
    }
    
    if (!(cm_event = cm_item(cm_history,CM_FIRST))) 
    {
	generate_error_page(session, title,"No change report available.");
	cm_free_list(config,cm_history);
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
	return(1);
    }

    do 
    {  
	change_time = CM_UNSIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TIME));

	/* ConfigMon init time */
	/* get the time value. This could either be change time or */
	/* time when ConfigMon was initialized */
	if (CM_SIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TYPE)) == 
	    CONFIGMON_INIT) 
	{
	    inittime = change_time;

	    continue;
	}

	if((session->config_end_time > 0 && 
	    change_time > session->config_end_time) || 
	   (session->config_start_time > 0 && 
	    change_time < session->config_start_time))
	{
	    cm_event = cm_item(cm_history,CM_NEXT);
	    continue;
	}

	if(session->config_start_time > inittime)
	    inittime=session->config_start_time;

	icmp = cm_change_items(config,change_time);
	if (!icmp) 
	{
	    generate_error_page(session, title,
		    "Information not available currently.");
	    if(config)	
	    {
		cm_free_config(config);
		session->config=NULL;
	    }
	    kl_free_block(initbuf);
	    return(1);
	}
	
	if (!(change_item = cm_item(icmp,CM_FIRST))) 
	{
	    cm_free_list(config,icmp);
	    continue;
	}

	/* Get the first changed item */
   
	do 
	{
	    type = 
		CM_SIGNED(cm_field(change_item,CHANGE_ITEM_TYPE,CI_TYPE));
	    
	    /* Get the type of the changed item */
	    
	    if (type == SYSINFO_CURRENT || type == SYSINFO_OLD) 
	    {
		if(!(sys++))
		{
		    generate_begin_background(session, title,0);
		    sys_report(hError,session,1);
		    
		    if ( session->textonly == 0 )
		    {
		    cftime(initbuf, "%m/%d/%Y", &inittime);
		    Body("<P>\n");
		    FormatedBody("Archive name: %s<BR>",session->dbname);
		    FormatedBody("<B>All Changes since %s</B>",initbuf);
		    Body("<HR><P>\n");
	            Body("<P>\n");

		    FormatedBody("%s","SYSTEM CHANGES");
		    Body("<P>\n");

		    TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=6 WIDTH=60% NOSAVE");
		    RowBegin("ALIGN=\"CENTER\"");
		    CellBegin("");
			   Body("<B>\n");
			   Body("&nbsp;\n");
		    CellEnd();
		    CellBegin("");
			   Body("<B>\n");
			   Body("System Id\n");
		    CellEnd();
		    CellBegin("");
			   Body("<B>\n");
			   Body("System type\n");
		    CellEnd();
		    CellBegin("");
			   Body("<B>\n");
			   Body("System serial number\n");
		    CellEnd();
		    CellBegin("");
			   Body("<B>\n");
			   Body("Hostname\n");
		    CellEnd();
		    CellBegin("");
			   Body("<B>\n");
			   Body("IP address\n");
		    CellEnd();
		    RowEnd();
		    } 
		    else
		    { /* Lynx */
		    cftime(initbuf, "%m/%d/%Y", &inittime);
		    FormatedBody ( "All Changes since %s", initbuf );
		    Body("<HR width=100%>\n");
                    FormatedBody ( "<p><b>SYSTEM CHANGES</b>" );
		    }
                 }         
	    }
	    else
	    {
		change_item = cm_item(icmp,CM_NEXT);
		continue;
	    }
	    item = cm_get_change_item(config,change_item);
	    item_type = cm_change_item_type(change_item);

	    /* 
	     * Component that is installed / de-installed depending upon the 
	     * type 
	     */
             if(item_type == SYSINFO_TYPE || !item_type) 
	     {
		 sprintf(systype, "IP%d",CM_SIGNED(cm_field(item, 
							    SYSINFO_TYPE, 
							    SYS_SYS_TYPE)));
		 ip_address = CM_STRING(cm_field(item, 
						 SYSINFO_TYPE, 
						 SYS_IP_ADDRESS));
		 hostname   = CM_STRING(cm_field(item, 
				  	         SYSINFO_TYPE, 
					         SYS_HOSTNAME));
		 sys_serial = CM_STRING(cm_field(item, 
						 SYSINFO_TYPE, 
						 SYS_SERIAL_NUMBER));
		 
                 if ( session->textonly == 0 )
		 {
	         RowBegin("ALIGN=\"TOP\"");
		 CellBegin("");
		 if(type == SYSINFO_CURRENT)
		 {
		     FormatedBody("%s","Current System");
	         }
		 else
		 {
		     FormatedBody("%s","Previous System");
		 }
		 CellEnd();
		 CellBegin("");
		    FormatedBody("%12llu",sys_id);
		 CellEnd();
		 CellBegin("");
		    FormatedBody("%8s",systype);
		 CellEnd();
		 CellBegin("");
		    FormatedBody("%13s",sys_serial);
		 CellEnd();
		 CellBegin("");
		    FormatedBody("%15s",hostname);
		 CellEnd();
		 CellBegin("");
		    FormatedBody("%-15s",ip_address);
		 CellEnd();
		 RowEnd();
		} 
		else  
		{ /* Lynx */
                 FormatedBody("<pre>  ------------------------------------------------------------------------ </pre>");
		 if(type == SYSINFO_CURRENT)
		 {
		     FormatedBody ( "<pre>   <b>Current System</b></pre>" );
	         }
		 else
		 {
		     FormatedBody ( "<pre>   <b>Previous System</b></pre>");
		 }
   	         FormatedBody ( "<pre>   System Id            %-12llu</pre>" , sys_id     );
   	         FormatedBody ( "<pre>   System type          %-8s   </pre>" , systype    );
		 FormatedBody ( "<pre>   System serial number %-13s  </pre>" , sys_serial );
   	         FormatedBody ( "<pre>   Hostname             %-15s  </pre>" , hostname   );
                 FormatedBody ( "<pre>   IP address           %-15s  </pre>" , ip_address );
		}
	     }
	     cm_free_item(config, item, item_type);
	} while(change_item = cm_item(icmp,CM_NEXT));
	cm_free_list(config,icmp);

     } while(cm_event = cm_item(cm_history,CM_NEXT));

    if(sys)
    {
      if ( session->textonly == 0 )
      {
	TableEnd();
	Body("<P><HR><P>\n");
      } 
      else 
      {
        FormatedBody("<pre>  ------------------------------------------------------------------------ </pre>");
      }
    }


    /* 
     * Software Changes Now.
     */
    if (!(cm_event = cm_item(cm_history,CM_FIRST))) {
	generate_error_page(session, title,"No change event.");
	cm_free_list(config,cm_history);
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
	kl_free_block(initbuf);

	return(1);
    }
    
    do {  
	
	change_time = CM_UNSIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TIME));
	/* ConfigMon init time */
	/* get the time value. This could either be change time or */
	/* time when ConfigMon was initialized */
	
        if((session->config_end_time > 0   && 
	    change_time > session->config_end_time) ||
	   (session->config_start_time > 0 && 
	    change_time < session->config_start_time))
	{
	    cm_event = cm_item(cm_history,CM_NEXT);
	    continue;
	}


	if (CM_SIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TYPE)) == 
	    CONFIGMON_INIT) 
	{
             continue;
	}

	icmp = cm_change_items(config,change_time);
	if (!icmp) 
	{
	    generate_error_page(session, title,
				"sw_change_report: icmp struct not allocated.");
	    if(config)	
	    {
		cm_free_config(config);
		session->config=NULL;
	    }
	    kl_free_block(initbuf);

	    return(1);
	}
	
	/* Get the first changed item */
	if (!(change_item = cm_item(icmp,CM_FIRST))) 
	{
	    cm_free_list(config,icmp);
	    continue;
	}

	do 
	{
	    /* Get the type of the changed item */
	    type = CM_SIGNED(cm_field(change_item,CHANGE_ITEM_TYPE,CI_TYPE));

	    if(type != SW_INSTALLED && type != SW_DEINSTALLED)
	    {
		change_item = cm_item(icmp,CM_NEXT);
		continue;
	    }
	    item = cm_get_change_item(config,change_item);
	    item_type = cm_change_item_type(change_item);

	    /* 
	     * Component that is installed / de-installed depending upon the 
	     * type 
	     */
             if(item_type == SWCOMPONENT_TYPE) 
	     {
		 tbuf = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, 
					     SW_INSTALL_TIME));
		 cftime(tbuf, "%m/%d/%Y", &change_time);
		 tbuf1 = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE,
					     SW_DEINSTALL_TIME));
		 cftime(tbuf1, "%m/%d/%Y", &change_time);
		 rec_key = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE,
						SW_REC_KEY));
		 name = CM_STRING(cm_field(item, SWCOMPONENT_TYPE,
					   SW_NAME));
		 version = CM_UNSIGNED(cm_field(item, SWCOMPONENT_TYPE, 
						SW_VERSION));
		 description = CM_STRING(cm_field(item, SWCOMPONENT_TYPE,
						  SW_DESCRIPTION));

	         if(!name)
	         {
		     change_item = cm_item(icmp,CM_NEXT);
		     cm_free_item(config, item, item_type);		     
		     continue;
		 }

		 if(!(sw++))
		 {
		     if(!sys)
		     {
			 generate_begin_background(session, title,0);
			 sys_report(hError,session,1);
			 
                         if ( session->textonly == 0 )
		         {
			 Body("<P>\n");
			 cftime(initbuf, "%m/%d/%Y", &inittime);
			 FormatedBody("Archive name: %s<BR>",session->dbname);
			 FormatedBody("<B>All Changes since %s</B>",initbuf);
			 Body("<HR><P>\n");

			 Body("<P>\n");
			 } 
			 else 
			 { /* Lynx */
		         cftime(initbuf, "%m/%d/%Y", &inittime);
		         FormatedBody ( "All Changes since %s", initbuf );
		         Body("<HR width=100%>\n");
			 }
		     }

                     if ( session->textonly == 0 )
                     {
		     FormatedBody("%s","SOFTWARE CHANGES");
		     Body("<P>\n");

		     TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=6 WIDTH=80% NOSAVE");
			  RowBegin("ALIGN=\"CENTER\"");
		          CellBegin("");
			      Body("<B>\n");
			      Body("Name\n");
			  CellEnd();
			  CellBegin("");
			      Body("<B>\n");
 			      Body("Version\n");
			  CellEnd();
			  CellBegin("");
			      Body("<B>\n");
			      Body("Install Date\n");
			  CellEnd();
			  CellBegin("");
			      Body("<B>\n");
			      Body("Deinstall Date\n");
			  CellEnd();
			  CellBegin("");
			      Body("<B>\n");
			      Body("Description\n");
			  CellEnd();
			  RowEnd();
		      }	
		      else
		      { /* Headers */
		      FormatedBody("<p><b>SOFTWARE CHANGES</b><br>");
                      FormatedBody("<pre>");
                      FormatedBody("  |----------------------------------------------------------------------|\n");
                      FormatedBody("  | Name                     Version      Install Date     Deinstall Date|\n");
                      FormatedBody("  | Description                                                          |\n");
		      }  
		  }
		  
		 if (session->textonly == 0 )
		 {
		 RowBegin("ALIGN=\"TOP\"");
		      CellBegin("");
			   FormatedBody("%-16s",name);
		      CellEnd();
		      CellBegin("");
			   FormatedBody("%u",version);
		      CellEnd();
		      CellBegin("");
			   FormatedBody("%10s",tbuf);
		      CellEnd();
		      if (type == SW_INSTALLED) 
		      {
			  CellBegin("");
			       FormatedBody("%s","0");
			  CellEnd();
		      }
		      else 
		      {
			  CellBegin("");
			       FormatedBody("%10s",tbuf1);
			  CellEnd();
		      }
		      CellBegin("");
			   FormatedBody("%30s",description);
		      CellEnd();
		  RowEnd();
		  }
		  else 
		  {
                      FormatedBody ( "  |----------------------------------------------------------------------|\n");
  		      FormatedBody ( "  | %-25.25s", name    );
  		      FormatedBody ( "%-13u"       , version );
  		      FormatedBody ( "%-17.17s"    , tbuf    );
		      if (type == SW_INSTALLED)                 /* Deinstall Date */
			FormatedBody ( "%-14.14s|\n", "Installed");
		      else
			FormatedBody ( "%-14.14s|\n", tbuf1 );
			
   		      FormatedBody   ( "  | %-69.69s|\n",  description );
   		  }    
		  
		  kl_free_block(tbuf);
		  kl_free_block(tbuf1);
	     }
	     cm_free_item(config, item, item_type);
	} while(change_item = cm_item(icmp,CM_NEXT));
	cm_free_list(config,icmp);

     } while(cm_event = cm_item(cm_history,CM_NEXT));


    if(sw)
    {
      if ( session->textonly == 0 )
      {
	TableEnd();
	Body("<P><HR><P>\n");
      }
      else
      {
        FormatedBody("  |----------------------------------------------------------------------|</pre>");
      }	
    }


    /* 
     * Now do the hardware.
     */
    if (!(cm_event = cm_item(cm_history,CM_FIRST))) {
	generate_error_page(session, title,"No change event.");
	cm_free_list(config,cm_history);
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
	kl_free_block(initbuf);
	
	return(1);
    }
    
    do {  
	
	change_time = CM_UNSIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TIME));
	
	/* ConfigMon init time */
	/* get the time value. This could either be change time or */
	/* time when ConfigMon was initialized */
	
        if((session->config_end_time > 0   && 
	    change_time > session->config_end_time) ||
	   (session->config_start_time > 0 && 
	    change_time < session->config_start_time))
	{
	    cm_event = cm_item(cm_history,CM_NEXT);
	    continue;
	}

	if (CM_SIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TYPE)) == 
	    CONFIGMON_INIT) 
	{
             continue;
	}

	icmp = cm_change_items(config,change_time);
	if (!icmp) 
	{
	    generate_error_page(session, title,
		    "hw_change_report(): icmp struct not allocated.");
	    if(config)	
	    {
		cm_free_config(config);
		session->config=NULL;
	    }
	    kl_free_block(initbuf);
	    return(1);
	}
	
	if (!(change_item = cm_item(icmp,CM_FIRST))) 
	{
	    cm_free_list(config,icmp);
	    continue;
	}

	/* Get the first changed item */
   
	do 
	{
	    type = CM_SIGNED(cm_field(change_item,CHANGE_ITEM_TYPE,CI_TYPE));
	    
		/* Get the type of the changed item */

	    if((type != HW_INSTALLED  && type != HW_DEINSTALLED) ||
	       !(item = cm_get_change_item(config,change_item)))
	    {
		change_item = cm_item(icmp,CM_NEXT);
		continue;
	    }

	    item_type = cm_change_item_type(change_item);

	    /* 
	     * Component that is installed / de-installed depending upon the 
	     * type 
	     */

             if (item_type ==  HWCOMPONENT_TYPE)
	     {
		 hw_name     = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_NAME));
		 location    = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, 
					       HW_LOCATION));
		 part_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE,
						  HW_PART_NUMBER));
		 serial_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE,
						    HW_SERIAL_NUMBER));
		 revision = CM_STRING(cm_field(item,HWCOMPONENT_TYPE,
					       HW_REVISION));
		 tbuf = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE,
					     HW_INSTALL_TIME));
		 cftime(tbuf, "%m/%d/%Y", &change_time);
		 tbuf1 = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE,
					     HW_DEINSTALL_TIME));
		 cftime(tbuf1, "%m/%d/%Y", &change_time);

	         if(!serial_number)
	         {
		     change_item = cm_item(icmp,CM_NEXT);
		     cm_free_item(config, item, item_type);		     
		     continue;
		 }

        	 if(!(hw++))
        	 {
		    if(!sys && !sw)
		    {
			generate_begin_background(session, title,0);
			sys_report(hError,session,1);
                        if ( session->textonly == 0 )
                        {		
			    Body("<P>\n");
			    cftime(initbuf, "%m/%d/%Y", &inittime);
			    FormatedBody("Archive name: %s<BR>",session->dbname);
			    FormatedBody("<B>All Changes since %s</B>",initbuf);
			    Body("<HR><P>\n");
			    Body("<P>\n");
	                } 
	                else
	                {
			    cftime(initbuf, "%m/%d/%Y", &inittime);
			    FormatedBody("<p>All Changes since %s",initbuf);
			    Body("<HR width=100%>\n");
	                }
		    }
        	
                    if ( session->textonly == 0 )
                    {			
		    FormatedBody("%s","HARDWARE CHANGES");
		    Body("<P>\n");

        	    TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=6 WIDTH=60% NOSAVE");
        	    RowBegin("ALIGN=\"CENTER\"");
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Name\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Location\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Part Number\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Serial Number\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Revision\n");
        	    CellEnd();
        	    CellBegin("");
        	    	 Body("<B>\n");
        	    	 Body("Install Date\n");
        	    CellEnd();
        	    CellBegin("");
        	    	 Body("<B>\n");
        	    	 Body("Deinstall Date\n");
        	    CellEnd();
        	    RowEnd();
        	    }
        	    else
        	    { /* Lynx */
	            FormatedBody("<p><b>HARDWARE CHANGES</b><br>");
                    FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                    FormatedBody ( "<pre>  | Name                 | Location | Part Number |Serial Number|Revision|</pre>" );
                    FormatedBody ( "<pre>  | Installed:           | Deinstaled:            |                      |</pre>" );       
        	    }
        	 }
        	 
                 if ( session->textonly == 0 )
                 {			
        	 RowBegin("ALIGN=\"TOP\"");
        	      CellBegin("");
		           FormatedBody("%14s",hw_name?hw_name:"NA");
        	      CellEnd();
        	      CellBegin("");
		           FormatedBody("%8s",location?location:"NA");
        	      CellEnd();
        	      CellBegin("");
		           FormatedBody("%12s",part_number?part_number:"NA");
        	      CellEnd();
        	      CellBegin("");
		           FormatedBody("%10s",serial_number?serial_number:"NA");
        	      CellEnd();
        	      CellBegin("");
		           FormatedBody("%8s",revision?revision:"NA");
        	      CellEnd();
        	      CellBegin("");
        		   FormatedBody("%10s",tbuf);
        	      CellEnd();
        	      if (type == HW_INSTALLED) 
        	      {
        		  CellBegin("");
        		       FormatedBody("%s","0");
        		  CellEnd();
        	      }
        	      else 
        	      {
        		  CellBegin("");
        		       FormatedBody("%10s",tbuf1);
        		  CellEnd();
        	      }
        	  RowEnd();
        	 } 
        	 else  
        	 { /* Lynx */
                   FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                   FormatedBody ( "<pre>  | %-21.21s|",  hw_name ? hw_name : "NA"); 
                   FormatedBody ( "%-10.10s|",  location? location: "NA"         );
                   FormatedBody ( "%-13.13s|",  part_number?part_number:"NA"     );
                   FormatedBody ( "%-13.13s|",  serial_number?serial_number:"NA" );
                   FormatedBody ( "%-8.8s|</pre>"  ,  revision?revision:"NA"     );
       		   FormatedBody ( "<pre>  | Installed: %-10.10s| Deinstaled: %-11.11s|                      |</pre>", 
       		                   tbuf, (type == HW_INSTALLED) ? "0" : tbuf1 );
                 }
        	  
        	 kl_free_block(tbuf);
        	 kl_free_block(tbuf1);
	     }
	     cm_free_item(config, item, item_type);
	} while(change_item = cm_item(icmp,CM_NEXT));
	cm_free_list(config,icmp);

     } while(cm_event = cm_item(cm_history,CM_NEXT));


    if(hw)
    {
      if ( session->textonly == 0 )
      {			
	TableEnd();
      } 
      else
      {	
        FormatedBody ( "<pre>  ------------------------------------------------------------------------</pre>" );
      } 
    }


    if(sys || sw || hw)
	generate_end_background(session);
    else
    {
	generate_error_page(session, title,
			    "No system changes are currently available.");
    }
    cm_free_list(config,cm_history);
    cm_free_config(config);
    session->config=NULL;
    kl_free_block(initbuf);

    return(0);
}

/* 
 * hw_change_report_by_serial
 */
static int hw_change_report_by_serial(sscErrorHandle hError,
				      cm_session_t *session)
{
    cm_hndl_t cm_history,cm_event;
    cm_hndl_t change_item,icmp,item;
    time_t change_time;
    int type = 0,item_type = 0,count = 0;
    char *tbuf, *tbuf1;
    char *ip_address, *hostname, *sys_serial;
    char systype[6];
    time_t init_time;
    int rec_key = 0, version = 0,install_time = 0;
    char *name, *description;
    char *hw_name, *part_number, *serial_number, *location, *revision;
    cm_value_t sys_id;
    cm_hndl_t config;
    int i=0;
    char *title="SYSTEM INFORMATION &gt; Part Changes";
    
    if(!session)
    {
	return -1;
    }
    
    if (!(config=session->config)) {
	generate_error_page(session, title,
		 "Config not allocated in hw_change_report_by_serial");
	return(-1);
    }
    sys_id= session->sys_id;
    
    if (!session->BoardSerial || 
	!(cm_history = 
	  cm_event_history(config,sys_id,0,0,ALL_CM_EVENTS))) {
        generate_error_page(session, title,"No historical information available currently.");
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
        return(1);
    }
    
    if (!(cm_event = cm_item(cm_history,CM_FIRST))) {
	generate_error_page(session, title,
			    "No historical information available currently.");
	cm_free_list(config,cm_history);
	if(config)	
	{
	    cm_free_config(config);
	    session->config=NULL;
	}
	return(1);
    }
    
    do {  
	
	change_time = CM_UNSIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TIME));
	
	/* ConfigMon init time */
	/* get the time value. This could either be change time or */
	/* time when ConfigMon was initialized */
	
	if (CM_SIGNED(cm_field(cm_event,CM_EVENT_TYPE,CE_TYPE)) == 
	    CONFIGMON_INIT) 
	{
	    init_time=change_time;
	    continue;
	}

	icmp = cm_change_items(config,change_time);
	if (!icmp) 
	{
	    generate_error_page(session, title,
				"hw_change_report_by_serial() --> "
				"icmp struct not allocated");
	    if(config)	
	    {
		cm_free_config(config);
		session->config=NULL;
	    }
	    return(1);
	}
	
	if (!(change_item = cm_item(icmp,CM_FIRST))) 
	{
	    cm_free_list(config,icmp);
	    continue;
	}

	/* Get the first changed item */
   
	do 
	{
	    type = CM_SIGNED(cm_field(change_item,
				      CHANGE_ITEM_TYPE,CI_TYPE));
	    
		/* Get the type of the changed item */

	    if((type != HW_INSTALLED  && type != HW_DEINSTALLED) ||
	       !(item = cm_get_change_item(config,change_item)))
	    {
		change_item = cm_item(icmp,CM_NEXT);
		continue;
	    }
	    
	    item_type = cm_change_item_type(change_item);

	    /* 
	     * Component that is installed / de-installed depending upon the 
	     * type 
	     */

             if (item_type == HWCOMPONENT_TYPE)
	     {
		 hw_name = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_NAME));
		 location = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, 
					       HW_LOCATION));
		 part_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, 
						  HW_PART_NUMBER));
		 serial_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, 
						    HW_SERIAL_NUMBER));
		 revision = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, 
					       HW_REVISION));
		 tbuf = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE, 
						    HW_INSTALL_TIME));
		 cftime(tbuf, "%m/%d/%Y", &change_time);
		 tbuf1 = (char *)kl_alloc_block(100, CM_TEMP);
		 change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE, 
						    HW_DEINSTALL_TIME));
		 cftime(tbuf1, "%m/%d/%Y", &change_time);

		 if(!hw_name)
		 {
		     change_item = cm_item(icmp,CM_NEXT);
		     cm_free_item(config, item, item_type);		     
		     continue;
		 }

		 if(serial_number && 
		    !strcasecmp(session->BoardSerial,serial_number))
		 {
        		 if(!(i++))
        		 {
             		   generate_begin_background(session, title,0);
 		           sys_report(hError,session,1);
        		   if ( session->textonly == 0 )
        		   {
			    Body("<P><HR><P>\n");

        		    TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=6 WIDTH=60% NOSAVE");
        		    RowBegin("ALIGN=\"CENTER\"");
        		    CellBegin("");
        			   Body("<B>\n");
        			   Body("Name\n");
        		    CellEnd();
        		    CellBegin("");
        			   Body("<B>\n");
        			   Body("Location\n");
        		    CellEnd();
        		    CellBegin("");
        			   Body("<B>\n");
        			   Body("Part Number\n");
        		    CellEnd();
        		    CellBegin("");
        			   Body("<B>\n");
        			   Body("Serial Number\n");
        		    CellEnd();
        		    CellBegin("");
        			   Body("<B>\n");
        			   Body("Revision\n");
        		    CellEnd();
        		    CellBegin("");
        		    	 Body("<B>\n");
        		    	 Body("Install Date\n");
        		    CellEnd();
        		    CellBegin("");
        		    	 Body("<B>\n");
        		    	 Body("Deinstall Date\n");
        		    CellEnd();
        		    RowEnd();
        		   } 
        		   else
        		   {
                            FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                            FormatedBody ( "<pre>  | Name                 | Location | Part Number |Serial Number|Revision|</pre>" );
                            FormatedBody ( "<pre>  | Installed:           | Deinstaled:            |                      |</pre>" );       
        		   } 
        		 }
        		 
        		 if ( session->textonly == 0 )
        		 {
        		 RowBegin("ALIGN=\"TOP\"");
        		      CellBegin("");
			           FormatedBody("%14s",hw_name?hw_name:"NA");
        		      CellEnd();
        		      CellBegin("");
			           FormatedBody("%8s",location?location:"NA");
        		      CellEnd();
        		      CellBegin("");
			           FormatedBody("%12s",part_number?part_number:"NA");
        		      CellEnd();
        		      CellBegin("");
			           FormatedBody("%10s",serial_number?serial_number:"NA");
        		      CellEnd();
        		      CellBegin("");
			           FormatedBody("%8s",revision?revision:"NA");
        		      CellEnd();
        		      CellBegin("");
        			   FormatedBody("%10s",tbuf);
        		      CellEnd();
        		      if (type == HW_INSTALLED) 
        		      {
        			  CellBegin("");
        			       FormatedBody("%s","0");
        			  CellEnd();
        		      }
        		      else 
        		      {
        			  CellBegin("");
        			       FormatedBody("%10s",tbuf1);
        			  CellEnd();
        		      }
        		  RowEnd();
        		  }
        		  else 
        		  {
                           FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                           FormatedBody ( "<pre>  | %-21.21s|",  hw_name ? hw_name : "NA"); 
                           FormatedBody ( "%-10.10s|",  location? location: "NA"         );
                           FormatedBody ( "%-13.13s|",  part_number?part_number:"NA"     );
                           FormatedBody ( "%-13.13s|",  serial_number?serial_number:"NA" );
                           FormatedBody ( "%-8.8s|</pre>"  ,  revision?revision:"NA"     );
       		           FormatedBody ( "<pre>  | Installed: %-10.10s| Deinstaled: %-11.11s|                      |</pre>", 
       		                          tbuf, (type == HW_INSTALLED) ? "0" : tbuf1 );
        		  }
        		  
        		  kl_free_block(tbuf);
        		  kl_free_block(tbuf1); 
		 }
		 cm_free_item(config, item, item_type);
	     }
	} while(change_item = cm_item(icmp,CM_NEXT));
	cm_free_list(config,icmp);

     } while(cm_event = cm_item(cm_history,CM_NEXT));
    cm_free_list(config,cm_history);


    if(config && !cm_get_sysconfig(config,session->sys_id,init_time) 
       && (item=cm_first_hwcmp(config)))
    {
	do
	{
	    sys_serial = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_SERIAL_NUMBER));
	    hw_name = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_NAME));
	    location = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_LOCATION));
	    part_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_PART_NUMBER));
	    serial_number = CM_STRING(cm_field(item,HWCOMPONENT_TYPE, HW_SERIAL_NUMBER));
	    revision = CM_STRING(cm_field(item,HWCOMPONENT_TYPE,HW_REVISION));
	    tbuf = (char *)kl_alloc_block(100, CM_TEMP);
	    change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE, HW_INSTALL_TIME));
	    cftime(tbuf, "%m/%d/%Y", &change_time);
	    tbuf1 = (char *)kl_alloc_block(100, CM_TEMP);
	    change_time = CM_UNSIGNED(cm_field(item, HWCOMPONENT_TYPE, HW_DEINSTALL_TIME));
	    cftime(tbuf1, "%m/%d/%Y", &change_time);

	    if(sys_serial &&
	       !strcasecmp(session->BoardSerial,sys_serial))
	    {
		if(!(i++))
		{
		    generate_begin_background(session, title,0);
		    sys_report(hError,session,1);
	            if ( session->textonly == 0 )
	            {	    
		    Body("<P><HR><P>\n");
			   
        	    TableBegin("BORDER=4 CELLSPACING=1 CELLPADDING=6 COLS=6 WIDTH=60% NOSAVE");
        	    RowBegin("ALIGN=\"CENTER\"");
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Name\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Location\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Part Number\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Serial Number\n");
        	    CellEnd();
        	    CellBegin("");
        		   Body("<B>\n");
        		   Body("Revision\n");
        	    CellEnd();
        	    CellBegin("");
        	    	 Body("<B>\n");
        	    	 Body("Install Date\n");
        	    CellEnd();
        	    CellBegin("");
        	    	 Body("<B>\n");
        	    	 Body("Deinstall Date\n");
        	    CellEnd();
        	    RowEnd();
        	    } 
        	    else 
        	    { 
                      FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                      FormatedBody ( "<pre>  | Name                 | Location | Part Number |Serial Number|Revision|</pre>" );
                      FormatedBody ( "<pre>  | Installed:           | Deinstaled:            |                      |</pre>" );       
        	    }
        	}

                if ( session->textonly == 0 )
	        {	    
        	RowBegin("ALIGN=\"TOP\"");
        	     CellBegin("");
		          FormatedBody("%14s",hw_name?hw_name:"NA");
        	     CellEnd();
        	     CellBegin("");
		          FormatedBody("%8s",location?location:"NA");
        	     CellEnd();
        	     CellBegin("");
		          FormatedBody("%12s",part_number?part_number:"NA");
        	     CellEnd();
        	     CellBegin("");
		          FormatedBody("%10s",serial_number?serial_number:"NA");
        	     CellEnd();
        	     CellBegin("");
		          FormatedBody("%8s",revision?revision:"NA");
        	     CellEnd();
        	     CellBegin("");
        		   FormatedBody("%10s",tbuf);
        	     CellEnd();
        	     if (!change_time) 
        	     {
        		  CellBegin("");
        		       FormatedBody("%s","0");
        		  CellEnd();
        	     }
        	     else 
        	     {
        		  CellBegin("");
        		       FormatedBody("%10s",tbuf1);
        		  CellEnd();
        	     }
        	 RowEnd();
        	 }
        	 else
        	 {
                   FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
                   FormatedBody ( "<pre>  | %-21.21s|",  hw_name ? hw_name : "NA"); 
                   FormatedBody ( "%-10.10s|",  location? location: "NA"         );
                   FormatedBody ( "%-13.13s|",  part_number?part_number:"NA"     );
                   FormatedBody ( "%-13.13s|",  serial_number?serial_number:"NA" );
                   FormatedBody ( "%-8.8s|</pre>"  ,  revision?revision:"NA"     );
       		   FormatedBody ( "<pre>  | Installed: %-10.10s| Deinstaled: %-11.11s|                      |</pre>", 
       		                   tbuf, (!change_time) ? "0" : tbuf1 );
        	 }
        	 
        	 kl_free_block(tbuf);
        	 kl_free_block(tbuf1);
	    }
	} while(item=cm_get_hwcmp(item,CM_NEXT));
    }

    if(i)
    {
       if ( session->textonly == 0 )
       {
	TableEnd();
       }
       else
       {
         FormatedBody ( "<pre>  |----------------------------------------------------------------------|</pre>" );
       }
        generate_end_background(session);
    }
    else
    {
	generate_error_page(session, title,
			    "No information available currently.");
    }

    cm_free_config(config);
    session->config=NULL;

    return(0);
}

int RGPGAPI rgpgGenerateReport(sscErrorHandle hError, 
			       rgpgSessionID session, 
			       int argc, 
			       char* argv[], 	
			       char *rawCommandString,
			       streamHandle result,
			       char *userAccessMask)
{  
    int ret = 0, table_id = 0, flag = 0;
    unsigned long id = 0;
    cm_session_t *pSession=session;
#if CMREPORT_DEBUG
    static int i=1;
#endif
   
/* Declaration and Initialization of the variables */

   if (argc <= 2) {
     sscError(hError,szServerNameErrorPrefix,
	      "Error : No argument specified\n\n");
     return RGPERR_SUCCESS;
    }

   if(!pSession->dbname)
       pSession->dbname=dbname;

   if(pSession->config)
   {
       cm_free_config(pSession->config);
       pSession->config=NULL;
   }
   pSession->config=cm_alloc_config(pSession->dbname,1);

   table_id = atoi(argv[1]);
   flag = atoi(argv[2]);
   
    
   /* Create the HTML generator */

   ret = createMyHTMLGenerator(result);
   if (ret != 0) {
       sscError(hError,szServerNameErrorPrefix,
		"Could not create the HTML generator\n\n");
       return RGPERR_SUCCESS;
   }
   

   /* Allocate structure for the database */  

   switch (table_id) {
       
   case 1: 
       sys_report(hError,pSession,flag);
       if(pSession->config)	
       {
	   cm_free_config(pSession->config);
       }
       break;
   case 2:
       sw_report(hError,pSession,flag);
       break;
   case 3:
       hw_report(hError,pSession,0,0,0);
       break;
   case 5: 
       hw_change_report_by_serial(hError,pSession);
       break;
   case 6: 
       sys_change_report(hError,pSession);
       break;
   case HWDRILLDOWN_PLUS:
   case HWDRILLDOWN_MINUS:
   case HWDRILLDOWN_PLUSPLUS:
       hw_report(hError,pSession,table_id,
		 (cm_hndl_t)strtol(argv[2],NULL,16),
		 (time_t)strtol(argv[3],NULL,10));
#if CMREPORT_DEBUG
       if(!(i++%10))
       {
	   fprintf(stderr," ***************************** Free Memory.\n");
	   hwtimestamp_Free();
	   alloc_debug=1;
       }
       else
	   alloc_debug=0;
#endif
       break;
   default:
       break;
   }
   
   ret = deleteMyHTMLGenerator();
   if (ret != 0) 
   {
       sscError(hError,szServerNameErrorPrefix,
		"Could not delete the HTML generator\n\n");
       return RGPERR_SUCCESS;
   }
   return RGPERR_SUCCESS;
}

/*---------------------*/
/*Report Generator Init*/
/*---------------------*/

/* --------------------------- rgpgInit ---------------------------------- */
int RGPGAPI rgpgInit(sscErrorHandle hError)
{
    time_t tmpT;
    
    /* Try initialize "free list of cm_session_t" mutex */
    if(pthread_mutex_init(&seshFreeListmutex,0))
    { 
	sscError(hError,szServerNameErrorPrefix,"Can't initialize mutex");
	return 0; /* error */
    }
    if(pthread_mutex_init(&hwtimestamp_mutex,0))
    { 
	sscError(hError,szServerNameErrorPrefix,"Can't initialize mutex");
	return 0; /* error */
    }

    mutex_inited++;
#ifdef INCLUDE_TIME_DATE_STRINGS
    sprintf(szVersionStr,"%d.%d %s %s",
	    MYVERSION_MAJOR,MYVERSION_MINOR,__DATE__,__TIME__);
#else
    sprintf(szVersionStr,"%d.%d",MYVERSION_MAJOR,MYVERSION_MINOR);
#endif
  
  
    cm_init();
    return 1; /* success */
}

/* ----------------------------- rgpgDone ------------------------------------ */
int RGPGAPI rgpgDone(sscErrorHandle hError)
{ 
    cm_session_t *s;

    if(mutex_inited)
    { 
	pthread_mutex_destroy(&seshFreeListmutex);
	pthread_mutex_destroy(&hwtimestamp_mutex);
	mutex_inited = 0;
	while((s = sesFreeList) != 0)
	{ 
	    sesFreeList = s->next;
	    free(s);
	}
    }

    pthread_mutex_lock(hwtimestamp_mutex);
    hwtimestamp_Free();
    pthread_mutex_unlock(hwtimestamp_mutex);

#if CMREPORT_DEBUG
    alloc_debug=1;
    free_temp_blocks();
#endif
    
    cm_free();

    return 1;
}

/*---------------------*/
/*Create Session Module*/
/*---------------------*/

rgpgSessionID RGPGAPI rgpgCreateSesion(sscErrorHandle hError)
{
    cm_session_t *session=NULL;
    char buf[MAXSYSIDSIZE];
    module_info_t mod_info;

    pthread_mutex_lock(&seshFreeListmutex);
    if((session = sesFreeList) != 0) sesFreeList = session->next;
    pthread_mutex_unlock(&seshFreeListmutex);

    if(!session) session = 
		     (cm_session_t *)kl_alloc_block(sizeof(cm_session_t),
						    CM_TEMP);
    if(!session)
    { 
	sscError(hError,szServerNameErrorPrefix,
		 "No memory to create session in rgpgCreateSession()");
	return 0;
    }
    memset(session,0,sizeof(cm_session_t));
    session->signature = sizeof(cm_session_t);
    session->now = time(NULL);
 
    if(get_module_info(0,&mod_info,sizeof(module_info_t)) >= 0)
    {
	session->sys_id = mod_info.serial_num;
    }
    else if (sysinfo(SI_HW_SERIAL,buf,MAXSYSIDSIZE) >= 0) 
    {
	session->sys_id = strtoull(buf,NULL,10);
	session->sys_id &= 0xFfFfFfFfFfFfFfFfULL;
    }
    else
    {
	sscError(hError,szServerNameErrorPrefix,"Could not read sys id in "
		 "rgpgCreateSesion()");
	return 0;
    }

    return session;
}

/*---------------------*/
/*Delete Session Module*/
/*---------------------*/
void RGPGAPI rgpgDeleteSesion(sscErrorHandle hError, 
			      rgpgSessionID session)
{
    cm_session_t *pSession=session;
    extern int alloc_debug;

    if(!pSession)
	return;

    if(pSession->strDate)
	free(pSession->strDate);
    if(pSession->strTime)
	free(pSession->strTime);
    if(pSession->BoardSerial)
	free(pSession->BoardSerial);
    if(pSession->dbname && pSession->dbname != dbname)
	free(pSession->dbname);

    kl_free_block(pSession);
#if CMREPORT_DEBUG
    if(alloc_debug)
	free_temp_blocks();
#endif

}

/*--------------------*/
/*Get Attribute Module*/
/*--------------------*/

/* 
 * ------------------------ rgpgGetAttribute ---------------------------------
 */
char *RGPGAPI rgpgGetAttribute(sscErrorHandle hError,rgpgSessionID session, const char *attributeID, const char *extraAttrSpec,int *typeattr)
{  
    char *s=NULL;
    cm_session_t *sess=session;

    if(typeattr) *typeattr = RGATTRTYPE_STATIC;
    if(!strcasecmp(attributeID,szVersion))         
	s = (char*)szVersionStr;
    else if(!strcasecmp(attributeID,szTitle))      
	s = (char*)myLogo;
    else if(!strcasecmp(attributeID,szUnloadTime)) 
	s = (char*)szUnloadTimeValue;
    else if(!strcasecmp(attributeID,szThreadSafe)) 
	s = (char*)szThreadSafeValue;
    else if(!strcasecmp(attributeID,szUnloadable)) 
	s = (char*)szUnloadableValue;
    else if(!strcasecmp(attributeID,szAcceptRawCmdString)) 
	s = (char*)szAcceptRawCmdStringValue;
    else if(!strcasecmp(attributeID,"config_date"))
    {
	if(typeattr) *typeattr = RGATTRTYPE_MALLOCED;	
	s = sess->strDate;
    }
    else if(!strcasecmp(attributeID,"config_time"))
    {
	if(typeattr) *typeattr = RGATTRTYPE_MALLOCED;	
	s = sess->strTime;
    }
    else if(!strcasecmp(attributeID,"config_time"))
    {
	if(typeattr) *typeattr = RGATTRTYPE_MALLOCED;
	s = sess->BoardSerial;
    }
    
    else
    { 
	sscError(hError,szServerNameErrorPrefix,
		 "%s No such attribute '%s' in rgpgGetAttribute()",
		 szServerNameErrorPrefix,attributeID);
	return 0;
    }

   if(!s) 
       sscError(hError,szServerNameErrorPrefix,
		szServerNameErrorPrefix,"No memory in rgpgGetAttribute()");
   return s;
}

/* ------------------- rgpgFreeAttributeString ---------------------------- */
void RGPGAPI rgpgFreeAttributeString(sscErrorHandle hError,
				     rgpgSessionID session,
				     const char *attributeID,
				     const char *extraAttrSpec,
				     char *attrString,int attrtype)
{  
    cm_session_t *sess = (cm_session_t *)session;
    
    if(!sess || sess->signature != sizeof(cm_session_t))
    { 
	sscError(hError,szServerNameErrorPrefix,
		 "Incorrect session id in rgpgFreeAttributeString()");
	return;
    }
    if ((attrtype == RGATTRTYPE_SPECIALALLOC || 
	 attrtype == RGATTRTYPE_MALLOCED) && attrString) free(attrString);
}


/* ------------------ rgpgSetAttribute------------------------------------- */

void  RGPGAPI rgpgSetAttribute(sscErrorHandle hError, rgpgSessionID session, 
			       const char *attributeID, 
			       const char *extraAttrSpec, const char *value)
{  
    char buff[128]; 
    cm_session_t *sess = session;

   if(!sess || sess->signature != sizeof(cm_session_t))
   { 
       sscError(hError,szServerNameErrorPrefix,
		"Incorrect session id in rgpgGetAttribute()");
       return;
   }
   
   if(!strcasecmp(attributeID,szVersion)    || 
      !strcasecmp(attributeID,szTitle)      ||
      !strcasecmp(attributeID,szThreadSafe) || 
      !strcasecmp(attributeID,szUnloadable) ||
      !strcasecmp(attributeID,szUnloadTime) ||
      !strcasecmp(attributeID,szAcceptRawCmdString))
   { 
       sscError(hError,szServerNameErrorPrefix,
		"Attempt to set read only attribute in rgpgSetAttribute()");
       return;
   }

   if(!strcasecmp(attributeID,"User-Agent") && value)
   { 
       sess->textonly = (strcasecmp(value,"Lynx") == 0) ? 1 : 0;
       return;
   }
   
   if(!strcasecmp(attributeID,"config_date") && value)
   {
       if(sess->strDate)
	   free(sess->strDate);
       sess->strDate=NULL;

       sess->req     = maketime(value,START);
       sess->strDate = strdup(value);
   }

   if(!strcasecmp(attributeID,"config_time") && value)
   {
       if(sess->strTime)
	   free(sess->strTime);
       sess->strTime=NULL;

       sess->req    += converttimetoseconds((char *)value);
       sess->strTime = strdup(value);
   }

   if(!strcasecmp(attributeID,szBoardSerial))
   {
       if(sess->BoardSerial)
	   free(sess->BoardSerial);
       sess->BoardSerial = NULL;

       if(value)
	   sess->BoardSerial = strdup(value);
   }

   if(!strcasecmp(attributeID,szConfigStartTime))	
   {
       if(value)
	   sess->config_start_time=maketime(value,START);
       else
	   sess->config_start_time=0;
   }

   if(!strcasecmp(attributeID,szConfigEndTime))	
   {
       if(value)
       {
	   sess->config_end_time=maketime(value,END);
       }
       else
	   sess->config_end_time=0;
   }

   if(!strcasecmp(attributeID,szConfigSysId))	
   {
       if(value)
       {
	   sess->sys_id  = strtoull(value,NULL,16);
	   sess->sys_id &= 0xFfFfFfFfFfFfFfFfULL;
       }
   }

   if(!strcasecmp(attributeID,szConfigStartItem))	
   {
       if(value)
       {
	   sess->tmpInt = atoi(value);
       }
   }

   if(!strcasecmp(attributeID,szConfigDbName))	
   {
       if(value)
       {
	   sess->dbname = strdup(value);
       }
   }
}


/*-----------------*/
/*Done in a big way*/
/*-----------------*/
    

    
       
