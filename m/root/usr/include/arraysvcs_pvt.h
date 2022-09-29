#ifndef __ARRAYSVCS_PVT_H__
#define __ARRAYSVCS_PVT_H__

/*
 * arraysvcs_pvt.h
 *
 *	Common data, declarations and definitions for libarray that are
 *	private to IRIX tools and utilities. Public interfaces may be
 *	found in the header <arraysvcs.h>
 *
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */

#ident "$Revision: 1.59 $"

#if defined(_LANGUAGE_C_PLUS_PLUS)
extern "C" {
#endif

#include <sys/types.h>
#include <errno.h>
#include <invent.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/un.h>

#include "arraysvcs.h"
#include "nodeinfo.h"

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

/*--------------------------------------------------------------------------*/
/*									    */
/*			        MESSAGE INFO				    */
/*									    */
/* These are data types used for describing various parts of basic messages */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Serial number
 *	Used to correlate a request and the following response
 */
typedef __uint64_t assernum_t;

/*
 * Version number
 *	Used to specify which version of the array services daemon protocol
 *	is being used.
 */
typedef __uint64_t asversion_t;

#define ARRAYD_VER_0_1		0x00000001	/* version 0.1 */
#define ARRAYD_VER_1_0		0x00000100	/* version 1.0 */
#define ARRAYD_VER_1_1		0x00000101	/* version 1.1 */
#define ARRAYD_VER_1_2		0x00000102	/* version 1.2 */
#define ARRAYD_VER_1_3		0x00000103	/* version 1.3 */
#define ARRAYD_VER_1_4		0x00000104	/* version 1.4 */
#define ARRAYD_VER_2_0		0x00000200	/* version 2.0 */
#define ARRAYD_VER_2_1		0x00000201	/* version 2.1 */
#define ARRAYD_VER_2_2		0x00000202	/* version 2.2 */
#define ARRAYD_VER_2_3		0x00000203	/* version 2.3 */
#define ARRAYD_VER_2_4		0x00000204	/* version 2.4 */
#define ARRAYD_VER_CURR		ARRAYD_VER_2_4	/* current version number */


/*
 * Message types
 *	Identify the specific type of message
 */
typedef __uint64_t asmsgtype_t;

#define ARRAYD_MT_REQUEST	0x00000001	/* Request for service */
#define ARRAYD_MT_RESPONSE	0x00000002	/* Response to a request */


/*
 * Message info
 *	Details about a message
 */
typedef struct asmsginfo {
	asversion_t	Version;	/* Version of following message */
	assernum_t	SerNum;		/* Serial number */
	asmsgtype_t	Type;		/* Message type */
} asmsginfo_t;



/*--------------------------------------------------------------------------*/
/*									    */
/*			        BASIC TYPES				    */
/*									    */
/* These are data types used for constructing the various array services    */
/* messages that are sent across the network.				    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Credentials
 *	The various items of information that identify the user that
 *	is making the request
 */
typedef struct ascreds {
	const char  *Origin;		/* Hostname of invoking machine */
	const char  *User;		/* Name of invoking user (effective) */
	const char  *RealUser;		/* Name of invoking user (real) */
	const char  *Group;		/* Grp of invoking user (effective) */
	const char  *RealGroup;		/* Grp of invoking user (real) */
	const char  *Project;		/* Project name of invoking user */
	ash_t	    ASH;		/* ASH of invoking user */
} ascreds_t;


/*
 * Encoded data
 *	Arbitrary data in undecoded network format. Typically used by
 *	functions such as SUBMIT, which are simply forwarding data (in
 *	this case, an argument list) to another daemon and don't need
 *	to waste time decoding the data only to immediately re-encode it.
 */
typedef struct asencoded {
	int Length;	/* # of bytes */
	char *Data;	/* Encoded data */
} asencoded_t;


/*
 * Execution results
 *	The results of executing a single command
 */
typedef struct asexecrslt {
	int	   Status;	/* Process completion status */
	const char *OutFile;	/* Path to file containing stdout/stderr */
	ash_t	   ASH;		/* Array session handle of command */
} asexecrslt_t;


/*
 * OS Info
 *	Information about a machine's OS level, etc.
 */
typedef struct asosinfo {
	int	RelMajor;	/* Major number of release */
	int	RelMinor;	/* Minor number of release */
	int	RelMaint;	/* Maintenance level of release */
	int	PtrSize;	/* Size of pointers in bits */
} asosinfo_t;


/*
 * List
 *	argv-style list of pointers. The array is terminated with a NULL
 *	pointer, so it actually contains NumPtrs+1 entries. This means it
 *	can be passed directly to execv, etc.
 */
typedef struct aslist {
	int  NumPtrs;		/* # of pointers */
	const void **Ptrs;	/* Array of pointers */
} aslist_t;


/*
 * Resource limit
 *	Information about an individual resource limit
 */
typedef struct asrlimit {
	int		Resource;	/* Resource code */
	struct rlimit64 Limits;		/* Actual limits */
} asrlimit_t;


/*
 * Signal Information
 *	Information about a signal that is to be delivered to an array
 *	session, process, etc.
 */
typedef struct assiginfo {
	uint32_t Flags;		/* Control flags */
	int	 Signal;	/* Signal to be delivered */
	union {
		ash_t	ASH;	/* Array session to signal */
		pid_t	PID;	/* Process to signal */
	}	 Target;	/* Recipient of signal */
} assiginfo_t;

#define ASSIG_PROCESS	0x00000001	/* Recipient is a process */
#define ASSIG_ARSESS	0x00000002	/* Recipient is an array session */


/*
 * Socket address
 *	May be from either the INET or UNIX domain. It is assumed that
 *	sockaddr_in.sin_family and sockaddr_un.sun_family are of the same
 *	size and at the same offset.
 */
typedef union assockaddr {
	struct sockaddr_in Inet;
	struct sockaddr_un Unix;
} assockaddr_t;



/*--------------------------------------------------------------------------*/
/*									    */
/*				   ASH INFO				    */
/*									    */
/*--------------------------------------------------------------------------*/

typedef struct asashinforeq {
	uint32_t flags;			/* Option flags */
	char	 *origin;		/* Assumed origin name */
	char	 *array;		/* Assumed array name */
	/* Future expansion possible */
} asashinforeq_t;

#define ASAI_GUESSFIRST	0x80000000	/* Guess first array with origin */
#define ASAI_GUESSONLY  0x40000000	/* Guess only array with origin */


typedef struct asashinfo {
	uint32_t flags;			/* Flag bits */
	char	 *origin;		/* Name of origin machine */
	char	 *array;		/* Name of associated array */
} asashinfo_t;

#define ASAI_LOCAL	0x80000000	/* Array session is local */
#define ASAI_OUNKNOWN	0x40000000	/* Cannot determine origin machine */
#define ASAI_OASSUMED	0x20000000	/* Used assumed origin name */
#define ASAI_AUNKNOWN	0x08000000	/* Cannot determine array name */
#define ASAI_AASSUMED	0x04000000	/* Used assumed array name */
#define ASAI_ADEFAULT	0x02000000	/* ASH specified default array */
#define ASAI_AGUESSED	0x01000000	/* Array name guessed heuristically */


asashinfo_t *asgetashinfo(asserver_t, ash_t, const asashinforeq_t *);
void asfreeashinfo(asashinfo_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			        HARDWARE INFO				    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Hardware configuration information
 */
typedef struct ashwinfo {
	short       versmaj;		/* Version - major number */
	short	    versmin;		/* Version - minor number */

	const char  *machname;		/* Machine name */
	int	    nprocs;		/* # of CPUs according to kernel */

	int	    numinvents;		/* # of inventory entries */
	inventory_t **invents;		/* List of ptrs to inventory entries */
} ashwinfo_t;

#define ASHWINFO_VERSION_MAJOR 1	/* Current version number */
#define ASHWINFO_VERSION_MINOR 1


typedef struct ashwinfolist {
	int	   numentries;		/* Number of entries */
	ashwinfo_t **entries;		/* Array of ptrs to ashwinfo_t's */
} ashwinfolist_t;


/* 
 * Functions for gathering hardware configuration information
 */
ashwinfolist_t	 *asgethwinfo_array(asserver_t, const char *);
ashwinfo_t	 *asgethwinfo_server(asserver_t);

/*
 * Functions for releasing hardware configuration information
 */
void asfreehwinfo(ashwinfo_t *);
void asfreehwinfolist(ashwinfolist_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			        NETWORK INFO				    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Network configuration information
 */
typedef struct asnetinv {
	uint32_t       flags;		/* Status flags */
	char	       *device;		/* Device name */
	struct in_addr network;		/* Network address of device */
	struct in_addr address;		/* IP address of device */
} asnetinv_t;

#define ASNETF_UP	0x80000000	/* Interface is up */


typedef struct asnetinfo {
	short	     versmaj;		/* Version - major number */
	short	     versmin;		/* Version - minor number */

	const char   *machname;		/* Machine name */
	const char   *hostname;		/* Network host name */
	int	     hostid;		/* Host ID */

	int	     numinvents;	/* # of inventory entries */
	asnetinv_t   **invents;		/* Array of ptrs to netinv entries */
} asnetinfo_t;

#define ASNETINFO_VERSION_MAJOR 1	/* Current version number */
#define ASNETINFO_VERSION_MINOR 1


typedef struct asnetinfolist {
	int	    numentries;		/* Number of entries */
	asnetinfo_t **entries;		/* Array of ptrs to asnetinfo_t's */
} asnetinfolist_t;


/* 
 * Functions for gathering network configuration information
 */
asnetinfolist_t	 *asgetnetinfo_array(asserver_t, const char *);
asnetinfo_t	 *asgetnetinfo_server(asserver_t);

/*
 * Functions for releasing network configuration information
 */
void asfreenetinv(asnetinv_t *);
void asfreenetinfo(asnetinfo_t *);
void asfreenetinfolist(asnetinfolist_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			         NODEINFO				    */
/*									    */
/*--------------------------------------------------------------------------*/

typedef struct asnodeinfolist {
	int	 numnodes;		/* Number of nodes */
	nodeinfo **nodes;		/* Array of ptrs to nodeinfo's */
} asnodeinfolist_t;

asnodeinfolist_t *asgetnodeinfo_array(asserver_t, const char *);
nodeinfo	 *asgetnodeinfo_server(asserver_t);

void asfreenodeinfo(nodeinfo *);
void asfreenodeinfolist(asnodeinfolist_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			       REMOTE EXECUTION				    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Remote execution
 *	Remote execution info maintained internally by array services
 */
typedef struct asremex {
	uint32_t	Flags;		/* User flags (from asremexecinfo) */
	uint32_t	CFlags;		/* Other control flags */
	const char	*Login;		/* User name on remote machine */
	aslist_t	Cmd;		/* Command to execute */
	aslist_t	Env;		/* Additional environment vars */
	const char	*StartDir;	/* Starting directory */
	assiginfo_t	*ExitSig;	/* Exit signal info */
	asosinfo_t	*OSInfo;	/* OS info */
	int		NumRLimits;	/* Number of entries in RLimits */
	asrlimit_t	*RLimits;	/* Resource limits array */
	int		SPILen;		/* Length of service provider info */
	char		*SPI;		/* Service Provider Info */
	int		Priority;	/* Process priority */
} asremex_t;

/* CFlags */
#define ASRX_PRIO_VALID	0x80000000	/* Priority field is valid */

/*
 * User remote execution info
 *	This struct is an input argument for the libarray function asremexec()
 */
typedef struct asremexecinfo {
	uint32_t   flags;	/* Flags */
	uint32_t   ioflags;	/* I/O flags (see ascommand) */
	char	   **args;	/* NULL-terminated array of command args */
	char	   **envvars;	/* NULL-terminated array of env. vars */
	const char *login;	/* Login name on remote machine */
	const char *startdir;	/* Starting directory path name */
	int	   exitsig;	/* Signal to be sent when remote cmd exits */

	char	   rsrvd[76];	/* reserved for expansion: init to 0's */
} asremexecinfo_t;

/* Flags for asremex_t and asremexecinfo_t */
#define ASRE_EXEC	0x80000000	/* Shell should "exec" command */
#define ASRE_NEWSESS	0x40000000	/* Start new array session */
#define ASRE_GETASH	0x20000000	/* Return ASH if possible */
#define ASRE_NOFALLBACK 0x10000000	/* Don't fallback for older servers */
#define ASRE_SPI	0x08000000	/* Propagate Service Provider Info */
#define ASRE_RLIMITS	0x04000000	/* Propagate resource limits */
#define ASRE_PRIORITY	0x02000000	/* Propagate priority */

#define ASRE_RELAXED	0x00000001	/* Request relaxed login rules */


/*
 * User remote execution results
 *	This is the struct returned by the libarray function asremexec()
 */
typedef struct asremexecrslt {
	uint32_t flags;		/* Flags */
	uint32_t ioflags;	/* I/O flags (see ascommand) */
	int	 stdinfd;	/* File descriptor for command's stdin */
	int	 stdoutfd;	/* File descriptor for command's stdout */
	int	 stderrfd;	/* File descriptor for command's stderr */
	int	 signalfd;	/* File descriptor for sending signals */
	ash_t	 ash;		/* Array session handle of remote command */
} asremexecrslt_t;	

/* Flags for asremexecrslt_t */
#define ASRR_GOTASH	0x80000000	/* ASH field is valid */


/* Function prototypes */
asremexecrslt_t *asremexec(asserver_t, const asremexecinfo_t *);
void asfreeremexecrslt(asremexecrslt_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*				SERVER INFO				    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Server configuration information
 */
typedef struct assrvrinfo {
	short	   versmaj;		/* Version - major number */
	short	   versmin;		/* Version - minor number */

	const char *dfltarray;		/* Name of default array */
	const char *hostname;		/* Network hostname of server */
	u_short	   portnum;		/* Port number of array svcs daemon */
	asident_t  ident;		/* Server ID (LOCAL IDENT) */
	uint32_t   flags;		/* Flags  (see ASSI_* below) */

	/* Added in version 1.2 */
	asversion_t  srvrprotocol;	/* Array protocol vers. of arrayd */
	asversion_t  libprotocol;	/* Array protocol vers. of libarray */
	asident_t    machid;		/* Kernel machine ID */
} assrvrinfo_t;

#define ASSRVRINFO_VERSION_MAJOR 1	/* Current version number */
#define ASSRVRINFO_VERSION_MINOR 2

#define ASSI_KGLOBALASH	0x80000000	/* Global ASHs generated by kernel */
#define ASSI_NOMACHID	0x40000000	/* Kernel does not support machid's */
#define ASSI_GENIDENT	0x20000000	/* Server ident generated by arrayd */


typedef struct assrvrinfolist {
	int	    numentries;		/* Number of entries */
	assrvrinfo_t **entries;		/* Array of ptrs to asnetinfo_t's */
} assrvrinfolist_t;


/*
 * Functions for obtaining server configuration information
 */
assrvrinfolist_t *asgetsrvrinfo_array(asserver_t, const char *);
assrvrinfo_t     *asgetsrvrinfo_server(asserver_t);
asversion_t	 asprotocolversion(void);

/*
 * Functions for releasing server configuration information
 */
void asfreesrvrinfo(assrvrinfo_t *, uint32_t);
void asfreesrvrinfolist(assrvrinfolist_t *, uint32_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*				  REQUESTS				    */
/*									    */
/* These are the requests that go between array daemons or between an array */
/* daemon and a client after they have been decoded, decrypted, adjusted    */
/* for version differences, etc. The actual data that is transmitted across */
/* the network is somewhat different and is defined in "protocol.h".	    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Request codes
 *	It is safe to add to this list, but never remove or change
 *	existing items in this list unless the version of arrayd that
 *	uses them is no longer supported.
 */
#define ARRAYD_NOP	 1	/* Do nothing */
#define ARRAYD_COMMAND	 2	/* Array command from daemon */
#define ARRAYD_ALLOCASH  3	/* Allocate global array session handle */
#define ARRAYD_GETARRAYS 4	/* Enumerate known arrays */
#define ARRAYD_GETMACHS  5	/* Enumerate machines in array */
#define ARRAYD_GETASHS	 6	/* Enumerate known ASH's */
#define ARRAYD_HWINFO    7	/* Get machine information */
#define ARRAYD_NETINFO   8	/* Get network information */
#define ARRAYD_DFLTARRAY 9	/* Get default array information */
#define ARRAYD_REMEX20	 10	/* Array 2.0 remote execution request */
#define ARRAYD_SRVRINFO  11	/* Get server info */

/* These are only valid if -DDEBUG has been specified */
#define ARRAYD_EXEC	 12	/* Execute arbitrary command on daemon */
#define ARRAYD_PRINT	 13	/* Print arbitrary message */
#define ARRAYD_QUIT	 14	/* Terminate server */
#define ARRAYD_TEST	 15	/* Experimental CONNECT interface */

/* More normal request codes */
#define ARRAYD_REMEX21	 16	/* Array 2.1 remote execution request */
#define ARRAYD_SIGNAL	 17	/* Send a signal to a process/array session */
#define ARRAYD_REMEX30	 18	/* Array 3.0-BETA1 remote execution request */
#define ARRAYD_REMEXT	 19	/* Tokenized remote execution request */
#define ARRAYD_GETPIDS	 20	/* Enumerate PIDs in an ASH */


/*
 * Request args
 *	A union of the various types of arguments that can be sent in
 *	a request
 */
typedef union asreqargs {
	asencoded_t  Encoded;	/* Undecoded data */
	ash_t	     ASH;	/* Array session handle */
	aslist_t     StrLst;	/* List of strings */
	asremex_t    *RemEx;	/* Remote execution info */
	assiginfo_t  *SigInfo;	/* Signal info */
	const char   *String;	/* Single string */
	uint32_t     Flags;	/* Miscellaneous flags */
} asreqargs_t;


/*
 * Request struct
 *	Internal representation of a request, generated by decoding a
 *	request message. It is safe to change this from release to
 *	release, provided the decoding routines initialize any new
 *	fields to default values when earlier versions of requests are
 *	received.
 */
typedef struct asrequest {
	int		ReqCode;	/* Request code */
	uint32_t	Flags;		/* Request flags */
	uint32_t	ConnectFlags;	/* Connect flags (see asconnect_t) */
	int		ConnectTimeout;	/* Timeout for connects (in secs) */
	int		Timeout;	/* Timeout for response (in secs) */
	const char	*Array;		/* Target array name */
	assockaddr_t	Server;		/* Address of requested server */
	asreqargs_t	Args;		/* Request arguments */
	ascreds_t	Creds;		/* Credentials of invoker */

	/* These fields are only valid on the receiver */
	asversion_t	Version;	/* Protocol version of request */
	assernum_t	SerNum;		/* Serial number */
	askey_t		OriginKey;	/* Encryption key for origin machine */
	const void	*Buffer;	/* Original network buffer */
} asrequest_t;

/* Request/Response flags */
#define ASREQ_BROADCAST	0x80000000	/* Forward request to other machines */
#define ASREQ_NEWSESS   0x40000000	/* Start new global array session */
#define ASREQ_QUIET	0x20000000	/* Do not collect command output */
#define ASREQ_WAIT	0x10000000	/* Wait for command completion */
#define ASREQ_FORWARD	0x08000000	/* Forward request to other server */

#define ASREQ_EMBEDDED  0x00800000	/* Msg embedded in larger msg */
#define ASREQ_MULTRESP  0x00400000	/* Multiple responses present */
#define ASREQ_ENCODED	0x00200000	/* "Payload" is still encoded */
#define ASREQ_GOTASH	0x00100000	/* ASH has been assigned to request */
#define ASREQ_NODECRESP 0x00080000	/* Do not decode response */
#define ASREQ_NOUNLINK  0x00040000	/* Do not unlink temp files */
#define ASREQ_STATIC	0x00020000	/* Response is statically allocated */
#define ASREQ_WILLMERGE 0x00010000	/* Output will be merged */
#define ASREQ_MERGED    0x00008000	/* Response OutFile is merged */
#define ASREQ_GOTFILE   0x00004000	/* Response OutFile is valid */
#define ASREQ_GOTCREDS  0x00002000	/* Credentials are valid */
#define ASREQ_ERROR	0x00001000	/* Only Result.Errno is valid */
#define ASREQ_LOCAL	0x00000800	/* Originated on local machine */
#define ASREQ_SKIPRESP	0x00000400	/* Don't bother sending response */

/* Version 1.X flags */
#define ASREQ_FORWARDED ASREQ_GOTASH	/* (same function, different names) */


/*
 * Function prototypes
 */
void	     ASFreeRequest(asrequest_t *);
asrequest_t  *ASGetRequest(int, askey_t, askey_t);
assernum_t   ASSendRequest(int, asrequest_t *, askey_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			    INTERACTIVE REQUESTS			    */
/*									    */
/* These are request variants that gather input and/or output from the	    */
/* local machine, rather like rsh.					    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Connection info
 *	Port information for establishing interactive connections
 */
typedef struct asconnect {
	assockaddr_t	MainAddr;	/* Address for stdin/stdout */
	assockaddr_t	CtrlAddr;	/* Address for signals/stderr */
	uint32_t	Flags;		/* Flags (see ASCMDIO_*) */
} asconnect_t;

/* Additional ASCMDIO flags. COORDINATE THESE WITH USER FLAGS IN arraysvcs.h */
#define ASCMDIO_SHRERR	(ASCMDIO_STDERR | ASCMDIO_OUTERRSHR)

#define ASCMDIO_USESHRERR(F) (((F) & ASCMDIO_STDERR) &&  \
			      ((F) & ASCMDIO_OUTERRSHR))
#define ASCMDIO_USESEPERR(F) (((F) & ASCMDIO_STDERR) &&  \
			      !((F) & ASCMDIO_OUTERRSHR))

#define ASCMDIO_USEMAIN(F)  (((F) & (ASCMDIO_STDIN | ASCMDIO_STDOUT)) || \
			     ASCMDIO_USESHRERR(F))
#define ASCMDIO_USECTRL(F)  (((F) & ASCMDIO_SIGNAL)  ||  ASCMDIO_USESEPERR(F))

#define ASCMDIO_USEIO(F)   ((F) & ASCMDIO_FULLIO)
#define ASCMDIO_GOTIO(F,G) (((F) & ASCMDIO_FULLIO) == ((G) & ASCMDIO_FULLIO))


/*
 * Interactive connection results
 *	Results from a successful interactive connection request.
 */
typedef struct asconnrslt {
	int	     ResponseSocket;	/* Socket for response message */
	int	     MainSocket;	/* Socket for stdin/stdout */
	int	     CtrlSocket;	/* Socket for signals/stderr */
	uint32_t     Flags;		/* Flags (see asconnect_t) */
	asexecrslt_t *ExecRslt;		/* Execution results */
	assernum_t   SerNum;		/* Serial number of original request */
} asconnrslt_t;

/* Additional ASCMDIO flags. COORDINATE THESE WITH USER FLAGS IN arraysvcs.h */
#define ASCMDIO_WAITMAIN 0x00000001	/* Main socket not yet connected */
#define ASCMDIO_WAITCTRL 0x00000002	/* Ctrl socket not yet connected */
#define ASCMDIO_WAITING  (ASCMDIO_WAITMAIN | ASCMDIO_WAITCTRL)


/*
 * Function prototypes
 */
struct asresponse;
int ASAcceptInteractiveConnection(int, const asrequest_t *, asconnrslt_t *,
				  struct asresponse *);
struct asresponse *ASSubmitInteractiveRequest(asserver_t, asrequest_t *);




/*--------------------------------------------------------------------------*/
/*									    */
/*				 RESPONSES				    */
/*									    */
/* These are the responses that are sent by an after servicing a request    */
/* but before they have been converted into an actual network message. Like */
/* requests, the actual data that is transmitted across the network has a   */
/* somewhat different format and can be found in protocol.h.		    */
/*									    */
/*--------------------------------------------------------------------------*/


/*
 * Response results
 *	A union of the various types of data that can be returned as a result
 */
typedef union asresprslt {
	int		ErrNo;		/* Error number */
	asarray_t	*Array;		/* Array info */
	asconnect_t	*Connect;	/* Interactive connection info */
	asconnrslt_t	*ConnRslt;	/* Interactive connection results */
	asencoded_t	Encoded;	/* Undecoded response */
	asexecrslt_t	*Exec;		/* Execution results */
	ash_t		ASH;		/* Array Session Handle */
	ashwinfo_t	*HWInfo;	/* Hardware info */
	aslist_t	List;		/* List of results */
	asnetinfo_t	*NetInfo;	/* Network info */
	assrvrinfo_t	*SrvrInfo;	/* Server info */
} asresprslt_t;


/*
 * Response struct
 *
 * Internal representation of a response, later to be encoded into an actual
 * response message. It is safe to change this from release to release,
 * provided the en/decoding routines initialize any new fields to default
 * values when earlier versions of requests are received.
 */
typedef struct asresponse {
	int	 ReqCode;	/* Original request code */
	uint32_t Flags;		/* Flags - see asrequest_t */
	int	 RespCode;	/* Response code */
	int	 CompCode;	/* Completion code */
	const char   *Name;	/* Name of responding machine */
	asresprslt_t Result;	/* Result data */

	/* These fields valid only on receiver */
	const void   *Buffer;	/* Original network buffer */
} asresponse_t;


/* Response codes: these indicate whether or not the array daemon was   */
/* able to *start* processing the command, not if the requested command */
/* itself completed successfully.					*/
#define ASRESP_NONE	  0	/* Unspecified response! */
#define ASRESP_ACCEPT	  1	/* Request accepted */
#define ASRESP_BADVERSION 2	/* arrayd version not supported */
#define ASRESP_BADREQUEST 3	/* Request code not recognized */
#define ASRESP_BADFORKREQ 4	/* Couldn't fork request subprocess */
#define ASRESP_NODFLTDEST 5	/* No default destination */
#define ASRESP_BADDSTTYPE 6	/* Destination type not supported */
#define ASRESP_BADDEST    7	/* Destination not defined in configuration */
#define ASRESP_BADBRDCAST 8	/* Request broadcasting failed */
#define ASRESP_DECODEFAIL 9	/* Request decoding failed */
#define ASRESP_NOCONNECT  10	/* Unable to start interactive connection */
#define ASRESP_NOCTRLPROC 11	/* Unable to start CONNECT control process */
#define ASRESP_ILLCONNECT 12	/* CONNECT illegal for this request */
#define ASRESP_ILLBCAST	  13	/* BROADCAST illegal for this request */
#define ASRESP_BADCREDS	  14	/* Invalid credentials */
#define ASRESP_BADHOST	  15	/* Invalid destination host address */
#define ASRESP_BADFWDREQ  16	/* Request forwarding failed */
#define ASRESP_CONNECT	  17	/* Connection information available */
#define ASRESP_ILLMERGE	  18	/* MERGE command illegal for this request */

/* Completion codes: these indicate the final disposition of an accepted */
/* array command.							 */
#define ASCOMP_NONE	  0	/* No result! */
#define ASCOMP_OK	  1	/* "Successful completion" */
#define ASCOMP_INITIATED  2	/* Command has been initiated */
#define ASCOMP_BADCOMMAND 3	/* Command not recognized */
#define ASCOMP_BADFORKCMD 4	/* Couldn't fork exec process */
#define ASCOMP_BADWAITCMD 5	/* Wait for exec process failed */
#define ASCOMP_CMDEMPTY	  6	/* Command has nothing to INVOKE */
#define ASCOMP_BADARGS    7	/* Error evaluating arguments */
#define ASCOMP_BADUSER	  8	/* Invalid USER */
#define ASCOMP_BADGROUP   9	/* Invalid GROUP */
#define ASCOMP_BADPROJECT 10	/* Invalid PROJECT */
#define ASCOMP_BADASHS	  11	/* Internal error on EnumASHs */
#define ASCOMP_BADOUTFILE 12	/* Unable to collect command output */
#define ASCOMP_SYSERROR	  13	/* Error on system call */


/*
 * Function prototypes
 */
void	      ASFreeResponse(asresponse_t *);
asresponse_t  *ASGetResponse(int, assernum_t, askey_t);
int           ASSendResponse(int, asresponse_t *, assernum_t, asversion_t,
			     askey_t);



/*--------------------------------------------------------------------------*/
/*									    */
/*			   SOCKET COMMUNICATIONS			    */
/*									    */
/*--------------------------------------------------------------------------*/

/*
 * Credentials gleaned from a client socket connection
 */
typedef struct assockcreds {
	uid_t	UID;		/* User ID */
	gid_t	GID;		/* Group ID */
} assockcreds_t;

/*
 * Function prototypes
 */
int ASAcceptLocalConnection(int, assockcreds_t *);
int ASAcceptRemoteConnection(int, const struct in_addr *, u_short);
int ASCloseClientSocket(int);
int ASConnectToServer(const assockaddr_t *, int);
int ASConnectToServerByToken(asserver_t);
const char *ASGetHostName(const char *);
int ASGetPortNum(const char *);
void ASHideLocalSocket(int);
int ASInitSockAddr(const char *, short, u_short, assockaddr_t *);
const struct in_addr *ASLocalhostAddr(void);
int ASReadSocket(int, void *, int);
int ASSetupLocalServerSocket(void);
int ASSetupRemoteServerSocket(u_short);
int ASWriteNBSocket(int, const void *, int);
int ASWriteSocket(int, const void *, int);



/*--------------------------------------------------------------------------*/
/*									    */
/*			     DEBUGGING/LOGGING				    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Debugging flags */
extern uint32_t ASDebug;
#define ASDEBUG_SOCKET	 0x00000001	/* Socket communications */
#define ASDEBUG_PROTOCOL 0x00000002	/* Protocol decode/encode */
#define ASDEBUG_PROCESS	 0x00000004	/* Process handling */
#define ASDEBUG_REQUEST  0x00000008	/* Request handling */
#define ASDEBUG_RESPONSE 0x00000010	/* Response handling */
#define ASDEBUG_RESULT	 0x00000020	/* Results */
#define ASDEBUG_INTERNAL 0x00000040	/* Internal and system errors */
#define ASDEBUG_ALL	 0x000000ff	/* All array services debugging */

/* Logging flags */
extern uint32_t ASLogging;
#define ASLOG_ERRTOSTDERR 0x00000001	/* Log error messages to stderr */
#define ASLOG_DBGTOSTDERR 0x00000002	/* Log debug messages to stderr */
#define ASLOG_ERRTOSYSLOG 0x00000004	/* Log error messages to syslog */
#define ASLOG_DBGTOSYSLOG 0x00000008	/* Log debug messages to syslog */

/* ASE_PROTOCOL "what" codes ("why" is undefined) */
#define ASPR_UNKNOWN	0	/* Do now know what */
#define ASPR_INTERRUPT	1	/* Interrupted by system call */
#define ASPR_SPURIOUS	2	/* Received spurious/ignored data */
#define ASPR_DISCONNECT	3	/* Remote side disconnected without sending */
#define ASPR_MAGIC	4	/* Invalid magic number in message */
#define ASPR_SHORT	5	/* Message body came out short */
#define ASPR_BADTYPE	6	/* Invalid message type */
#define ASPR_SERNUM	7	/* Serial number mismatch in response */
#define ASPR_BADCLADDR	8	/* Unauthorized client address */
#define ASPR_BADCLPORT  9	/* Unauthorized client port */
#define ASPR_AUTHFAIL	10	/* Message failed authentication */
#define ASPR_AUTHSUSP	11	/* Disconnect, authentication suspected */

/* ASE_INTERNAL "why" codes - for many of these, additional detail can */
/* be found by looking at the error log (usually SYSLOG) on the machine */
/* running the array daemon.						*/
#define ASINY_UNKNOWN	0	/* Do not know why */
#define ASINY_NOMEM	1	/* Out of memory */
#define ASINY_RANGE	2	/* Value out of range (see "what") */
#define ASINY_NULL	3	/* Null pointer (see "what") */
#define ASINY_ENUMASH	4	/* Unable to enumerate ASHs */
#define ASINY_TEMPFILE	5	/* Unable to create temporary file */
#define ASINY_NOTMULT	6	/* ASREQ_MULTRESP was expected */
#define ASINY_STRANGEFD 7	/* Unexpected FD was SELECTed */
#define ASINY_INVENT	8	/* Unexpected inventory data */
#define ASINY_MISMATCH	9	/* Two objects were mismatched somehow */
#define ASINY_ODDCOMBO	10	/* Unrecognized response: "what"=RespCode, */
				/*			  "extra"=CompCode */
#define ASINY_INSUFFIO	12	/* Insufficient I/O connections for CONNECT */
#define ASINY_HIDEISOCK 13	/* Attempted to hide non-UNIX socket */
#define ASINY_CHREQFAIL 14	/* Request to parent arrayd failed */

/* ASE_INTERNAL "what" codes for ASINY_RANGE */
#define ASIN_UNKNOWN	0	/* Do not know what */
#define ASIN_REQCODE	1	/* Request code (see "extra") */
#define ASIN_DSTTYPE	2	/* Destination type */
#define ASIN_REQVERSION 3	/* Request version */
#define ASIN_RSPVERSION 4	/* Response version */
#define ASIN_ITEMTYPE   5	/* EvalItem item type */
#define ASIN_COMPCODE	6	/* Completion code */
#define ASIN_ADDRFAMILY 7	/* Address family */

/* ASE_INTERNAL "extra" codes for ASIN_REQCODE/ASINY_RANGE */
#define ASINX_UNKNOWN	0	/* Do not know anything extra */
#define ASINX_DECV1REQ  1	/* DecodeV1Request */
#define ASINX_ENCV1REQ  2	/* EncodeV1Request */
#define ASINX_DECV1RSP	3	/* DecodeV1Response */
#define ASINX_ENCV1RSP	4	/* EncodeV1Response */
#define ASINX_DECV2REQ	5	/* DecodeV2Request */
#define ASINX_ENCV2REQ	6	/* EncodeV2Request */
#define ASINX_REMOTE	7	/* Remote server */

/* ASE_INTERNAL "what" codes for ASINY_NULL */
#define ASIN_NETINV	1	/* asnetinv_t */
#define ASIN_MEMINFO	2	/* nodeinfo_mem_info_t */
#define ASIN_BOARDINFO	3	/* nodeinfo_pe_board_t */
#define ASIN_CPUINFO	4	/* nodeinfo_pe_cpu_t */
#define ASIN_FPUINFO	5	/* nodeinfo_pe_fpu_t */
#define ASIN_MSGINFO	6	/* asmsginfo_t */
#define ASIN_SOCKADDR	7	/* assockaddr_t */
#define ASIN_REMEX	8	/* asremex_t */
#define ASIN_SRVINT	9	/* asserver_internal_t */
#define ASIN_DFLTHOST	10	/* ASGetHostName() */
#define ASIN_REMEXINFO	11	/* asremexecinfo_t */

/* ASE_INTERNAL "what" codes for ASINY_MISMATCH */
#define ASIN_CONNRESP	1	/* Couldn't match response to connect info */
#define ASIN_BADCONNECT	2	/* Got response but failed connect */
#define ASIN_NORESPONSE 3	/* Got connect but no response */

/* Error location */
extern const char *aserrorfile;
extern int aserrorline;

/* Function declarations */
void ASDebugErr(const char *, ...);
void ASDebugErrA(const char *, ...);
void ASDebugErrS(const char *, ...);
void ASDebugMsg(const char *, ...);
void ASErrMsg(const char *, ...);
void ASErrMsgA(const char *, ...);
void ASErrMsgS(const char *, ...);
void ASSetLogInfo(int, int);
aserror_t ASTranslateResponse(const asresponse_t *);



/*--------------------------------------------------------------------------*/
/*									    */
/*				MISCELLANEOUS				    */
/*									    */
/*--------------------------------------------------------------------------*/


/* Environment variable names */
#define ASCONNECTTO_VAR "ARRAYD_CONNECTTO" /* Connect timeout */
#define ASFORWARD_VAR	"ARRAYD_FORWARD"   /* Forwarding on/off */
#define ASHOSTNAME_VAR	"ARRAYD"	   /* Server's host name */
#define ASLOCALKEY_VAR  "ARRAYD_LOCALKEY"  /* Local authentication key */
#define ASPORTNUM_VAR	"ARRAYD_PORT"	   /* Server's port number */
#define ASREMOTEKEY_VAR "ARRAYD_REMOTEKEY" /* Remote authentication key */
#define ASTIMEOUT_VAR	"ARRAYD_TIMEOUT"   /* Normal timeout */

/* Default values */
#define ASDFLT_CONNECTTO  5		/* secs before timing out a connect */
#define ASDFLT_HOST	  "localhost"	/* arrayd server host */
#define ASDFLT_PORT	  5434		/* arrayd port */
#define ASDFLT_SERVICE	  "sgi-arrayd"	/* service name */
#define ASDFLT_TIMEOUT	  15		/* secs before timing out a response */

/* Authentication info */
#define RESERVEDKEY	0	/* Do not encrypt/decrypt */

/* Path names */
#ifdef CLIENT53
#define ASPATH_TMPDIR	"/tmp"
#else
#define ASPATH_TMPDIR	"/tmp/.arraysvcs"
#endif   /* CLIENT53 */

#define ASPATH_LCLSRVR	ASPATH_TMPDIR "/lclsrvr"
#define ASPATH_LCLCLNT	ASPATH_TMPDIR "/clientXXXXXX"
#define ASPATH_LCLFILE	ASPATH_TMPDIR "/lcloutXXXXXX"
#define ASPATH_REMFILE  ASPATH_TMPDIR "/remoutXXXXXX"
#define ASPATH_MRGFILE	ASPATH_TMPDIR "/mrgoutXXXXXX"

/* Miscellaneous function declarations */

/* Names & Addresses */
const char *ASAddr2Name(struct in_addr);
const char *ASBaseName(const char *);
const char *ASLocalName(int);
const char *ASRemoteName(int);

/* Strings & storage */
#define ASFree(P) if ((P) != NULL) free(P)
int  ASLine2List(const char *, aslist_t *);
char *ASList2Line(const aslist_t *);
void *ASMalloc(size_t, const char *);
char *ASStrDup(const char *);
char *ASUnquoteChar(char *);

/* Request processing */
asresponse_t *ASSubmitPublicRequest(asserver_t, asrequest_t *);
asresponse_t *ASWaitForResponse(int, int, assernum_t, askey_t);

/* Miscellaneous miscellaneous */
int ASCompareASHs(const void *, const void *);
int ASGetOSInfo(asosinfo_t *);

#endif /* C || C++ */

#if defined(_LANGUAGE_C_PLUS_PLUS)
}
#endif

#endif /* !__ARRAYSVCS_PVT_H_ */
