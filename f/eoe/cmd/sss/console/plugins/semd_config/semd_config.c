/*
 * C Interface to reconfigure the semd/sgmd daemon.
 */
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <common.h>
#include <seh_archive.h>
#include <sgmdefs.h>
#include <strings.h>

#define SEMD_CONFIG_PTHREADS 1

/*
 * Actually get a message. Do the IPC.
 * returns
 * >0 on success
 * <= 0 on fail
 */
static int semd_getmsg(int mq, char *mb,int len)
{
  int j;
  fd_set readfds;
  struct timeval timeout;
  struct sockaddr_un *from=NULL;
  int fromlen=sizeof(struct sockaddr_un);
  int newsock;

  timeout.tv_sec=10;
  timeout.tv_usec=0;
  bzero(&readfds,sizeof(fd_set));
  FD_SET(mq,&readfds);
  
  if(select((mq+1),&readfds,0,0,&timeout) > 0)
  {
      if((newsock= accept(mq,(struct sockaddr *)from,&fromlen)) < 0)
      {
	  return -1;
      }
  }
  else
  {
      return -1;
  }

  timeout.tv_sec=10;
  timeout.tv_usec=0;
  bzero(&readfds,sizeof(fd_set));
  FD_SET(newsock,&readfds);

  if((select((newsock+1),&readfds,0,0,&timeout) > 0) && 
     (j=read(newsock,mb,len)))
    {
      close(newsock);
      return j;
    }
  close(newsock);

  return -1;
}

/*
 * Actually send a message. Do the IPC.
 * returns
 * >0 on success
 * <= 0 on fail
 */
static int semd_sendmsg(int mq, char *mb,int len,char *Pchar_to)
{
  int i,j;
  int tolen;
  struct sockaddr_un saddr;
  
  tolen=strlen(Pchar_to)+sizeof(saddr.sun_family);
  
  strcpy(saddr.sun_path,Pchar_to);
  saddr.sun_family=AF_UNIX;
  
  if(mq < 0) 
  {
      return -1;
  }
  
  if(connect(mq,(struct sockaddr *)&saddr,tolen) < 0)
  {
      return -1;
  }

  if(write(mq,mb,len) == len)
  {
      return len;
  }
  return -1;
}

/*
 * Create message queue.
 * 0 on success. -1 on fail
 */
static int createq(int *Pmq,char *path)
{
  int saddrlen;
  struct sockaddr_un saddr;


  if((*Pmq=socket(AF_UNIX,SOCK_STREAM,0)) < 0)
    return -1;
  
  if(path)
  {
      unlink(path);
      saddrlen=strlen(path)+sizeof(saddr.sun_family);
      strcpy(saddr.sun_path,path);
      saddr.sun_family=AF_UNIX;
      if(bind(*Pmq,(struct sockaddr*)&saddr,saddrlen) < 0)
      {
	  return -1;
      }
      listen(*Pmq,50);
  }
  
  fcntl(*Pmq,F_SETFD,FD_CLOEXEC);	/* close on exec... just to be sure. */
  return 0;
}

/*
 * Delete message queue.
 * 0 on success.
 */
static int delq(int mq,char *recv_path)
{
    unlink(recv_path);
    if(close(mq))
    {
	return -1;
    }
    return 0;
}


static int semd_config(int flag,char *pStr)
{
    char send_path[1024];
    char recv_path[1024];
    char *sendmsg=NULL;
    int length,c;
    char buffer[1024];
    int sendq;
    int recvq;
    Archive_t *Parchive;
    int i=0;


    bzero(buffer,1024);

    switch(flag)
    {
    case(RULE_CONFIG) :
	{
	    if(!pStr)
		return -1;
	    strcpy(buffer,RULE_CONFIG_STRING);
	    strcat(buffer," ");
	    strcat(buffer,pStr);
	    break;
	}
    case(EVENT_CONFIG) :
	{
	    if(!pStr)
		return -1;
	    strcpy(buffer,EVENT_CONFIG_STRING);
	    strcat(buffer," ");
	    strcat(buffer,pStr);
	    break;
	}
    case(GLOBAL_CONFIG) :
	{
	    if(!pStr)
		return -1;
	    strcpy(buffer,GLOBAL_CONFIG_STRING);
	    strcat(buffer," ");
	    strcat(buffer,pStr);
	    break;
	}
#if SGM
    case(SGM_OPS) :
	{
	    if(!pStr)
		return -1;
	    strcpy(buffer,SGM_CONFIG_STRING);
	    strcat(buffer," ");
	    strcat(buffer,pStr);
	    break;
	}
#endif
    case(ARCHIVE_START_CONFIG) :
	{
	    strcpy(buffer,ARCHIVE_START);
	    break;
	}
    case(ARCHIVE_END_CONFIG) :
	{
	    strcpy(buffer,ARCHIVE_END);
	    break;
	}
    default:
	{
	    free(sendmsg);
	    return -1;
	}
    }

    length=strlen(buffer);

    sendmsg=calloc(length+MAX_MESSAGE_LENGTH,1);
    if(!sendmsg)
	return -1;
    memset(sendmsg,length+MAX_MESSAGE_LENGTH,0);

    /* 
     * Create the send and receive queues.
     */
    snprintf(recv_path,100,SSS_WORKING_DIRECTORY "/%s.%d",SEH_TO_ARCHIVE_MSGQ,
	     getpid());
    for(i=0;i<ARCHIVE_IDS;i++)
#if SEMD_CONFIG_PTHREADS
	snprintf(recv_path,100,"%s.%lld",recv_path,(__uint64_t)pthread_self());
#else
        snprintf(recv_path,100,"%s.%lld",recv_path,(__uint64_t)0);
#endif  
  
    snprintf(send_path,100,SSS_WORKING_DIRECTORY "/%s",ARCHIVE_TO_SEH_MSGQ);

    if(createq(&sendq,NULL) <0)
    {
	free(sendmsg);
	return -1;
    }
    
    if(createq(&recvq,recv_path) < 0)
    {
	free(sendmsg);
	close(sendq);
	return -1;
    }

    /* 
     * Setup the send packet.
     */
    Parchive=(Archive_t *)sendmsg;
    Parchive->LmsgKey=getpid();
    for(i=0;i<ARCHIVE_IDS;i++)
#if SEMD_CONFIG_PTHREADS
	Parchive->aullArchive_id[i]=(__uint64_t)pthread_self();
#else
        Parchive->aullArchive_id[i]=0;
#endif
    Parchive->LMagicNumber=ARCHIVE_MAGIC_NUMBER;
    Parchive->iVersion    =ARCHIVE_VERSION_1_0;
    Parchive->iDataLength =length;
    memcpy((char *)&(Parchive->ptrData),buffer,length);

    if(semd_sendmsg(sendq,sendmsg,MAX_MESSAGE_LENGTH+length,send_path) <= 0)
    {
	goto LOCAL_ERROR;
    }

    if((c=semd_getmsg(recvq,sendmsg,(sizeof(Archive_t)-sizeof(void*)))) <= 0)
    {
	goto LOCAL_ERROR;
    }
    
    if(Parchive->LMagicNumber!=ARCHIVE_MAGIC_NUMBER)
	goto LOCAL_ERROR;
    
    if(Parchive->iVersion != ARCHIVE_VERSION_1_0)
	goto LOCAL_ERROR;

    free(sendmsg);
    close(sendq);
    delq(recvq,recv_path);

    return 0;

LOCAL_ERROR:
    free(sendmsg);
    close(sendq);
    delq(recvq,recv_path);
    
    return -1;
}

int configure_event(char *pStr)
{
    return semd_config(EVENT_CONFIG,pStr);
}

int configure_rule(char *pStr)
{
    return semd_config(RULE_CONFIG,pStr);
}

int configure_global(char *pStr)
{
    return semd_config(GLOBAL_CONFIG,pStr);
}

#if SGM
int configure_sgm(char *pStr)
{
    return semd_config(SGM_OPS,pStr);
}
#endif

int archive_start()
{
    return semd_config(ARCHIVE_START_CONFIG,NULL);
}

int archive_end()
{
    return semd_config(ARCHIVE_END_CONFIG,NULL);
}

#define TEST 0

#if TEST
int main()
{
    /* 
     * Class = 1 Type =2 sehthrottle=3 sehfrequency = 4 throttled flag = 1
     */
    configure_event("UPDATE localhost 1 2 3 4 1");
    configure_event("DELETE localhost 1 2");
    configure_event("NEW localhost 1 2 3 4 1");

    /* 
     * Rule configuration, aciton_id = 1
     */
    configure_rule("UPDATE 1");
    configure_rule("DELETE 1");
    configure_rule("NEW 1");

    /* 
     * Glbal configuration, throttle = 1, logging = 1, action = 1
     */
    configure_global("1 1 1");

#if SGM
    /* 
     * Sgm configuration, Mode change...
     */
    configure_sgm("MODE 1");
    /* 
     * Sgm configuration subscribe & unsubscribe 12345.
     */
    configure_sgm("SUBSCRIBE 12345");
    configure_sgm("UNSUBSCRIBE 12345");
#endif
    return 0;
}
#endif
