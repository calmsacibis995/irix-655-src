#ifndef __ARRAYSVCS_H__
#define __ARRAYSVCS_H__

/*
 * arraysvcs.h
 *
 *	Interfaces for the array services library
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

#ident "$Revision: 1.43 $"

#if defined(_LANGUAGE_C_PLUS_PLUS)
extern "C" {
#endif

#include <sys/types.h>
#include <netinet/in.h>

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)


/*
 * Basic types
 */
typedef uintptr_t	asserver_t;	/* Server token */

typedef uint64_t	askey_t;	/* Authentication key */
#define AS_NOKEY	0		/* No key specified */

typedef uint16_t	asident_t;	/* Entity identifier */



/*
 * error handling
 *	Most array services store a result code in the global variable
 *	"aserrorcode".  This result code may actually contain several
 *	fields.  A general summary of the error code, similar to the
 *	standard "errno", can be found in "aserrno".
 */

/* Error code declarations */
typedef __uint32_t aserror_t;

#define aserrnoc(E)    ((E) & 255)
#define aserrwhatc(E)  (((E) >> 8) & 255)
#define aserrwhyc(E)   (((E) >> 16) & 255)
#define aserrextrac(E) ((E) >> 24)

extern aserror_t aserrorcode;

#define aserrno    (aserrnoc(aserrorcode))
#define aserrwhat  (aserrwhatc(aserrorcode))
#define aserrwhy   (aserrwhyc(aserrorcode))
#define aserrextra (aserrextrac(aserrorcode))


/* Error summary numbers */
#define ASE_OK		0		/* Operation successful */
#define ASE_SYSERROR	1		/* System operation failed */
#define ASE_BADVALUE	2		/* Invalid value/argument */
#define ASE_REQFAILED	3		/* Request failed */
#define ASE_BADCOMMAND	4		/* Invalid command */
#define ASE_PROTOCOL	5		/* Protocol error */
#define ASE_INTERNAL	6		/* Internal error */
#define ASE_NOARRAYSVCS 7		/* Array services not installed */

/* ASE_OK "why" codes ("what" is undefined) */
#define ASOK_COMPLETED	0		/* Command/request completed */
#define ASOK_INITIATED	1		/* Command started, may not be done */
#define ASOK_CONNECT    2		/* Initiate connection */
#define ASOK_CONNECTED	3		/* Connection completed */

/* ASE_NOARRAYSVCS has no "what" or "why" code */

/* ASE_SYSERROR "what" codes.  ("why" == errno) */
#define ASSE_UNKNOWN	0		/* Origin unknown */
#define ASSE_CMDFORK	1		/* Command process fork */
#define ASSE_REQFORK	2		/* Request process fork */
#define ASSE_WAIT	3		/* Wait for process */
#define ASSE_SELECT	4		/* Wait for event */
#define ASSE_READHDR	5		/* Read message header */
#define ASSE_READBODY	6		/* Read message body */
#define ASSE_WRITEHDR	7		/* Write message header */
#define ASSE_WRITEBODY	8		/* Write message body */
#define ASSE_GETASH	9		/* Extract ASH */
#define ASSE_PIDSINASH	10		/* Extract PIDs in ASH */
#define ASSE_ACCEPT	11		/* Accept client connection */
#define ASSE_SOCKET	12		/* Create socket */
#define ASSE_CONNECT	13		/* Connect to server */
#define ASSE_SETSOCKOPT 14		/* Set socket options */
#define ASSE_BIND	15		/* Bind name to socket */
#define ASSE_LISTEN	16		/* Listen for connections */
#define ASSE_WRITEOUT	17		/* Write output data */
#define ASSE_OPENPROCD	18		/* Open procfs directory */
#define ASSE_OPENOUT    19		/* Open output file */
#define ASSE_STATOUT	20		/* Stat output file */
#define ASSE_READOUT	21		/* Read output file */
#define ASSE_ENUMASHS	22		/* Enumerate ASHs */
#define ASSE_PEERNAME	23		/* Get peer name */
#define ASSE_SOCKNAME	24		/* Get socket name */
#define ASSE_STATUDS	25		/* Stat unix domain socket */
#define ASSE_IOCTLNBIO	26		/* Make socket non-blocking */
#define ASSE_GETSRVOPT  27		/* Get server option */
#define ASSE_KILL	28		/* Send signal to process */
#define ASSE_SETUID	29		/* Set user ID */
#define ASSE_UNAME	30		/* Get OS information */
#define ASSE_GETSPINFO	31		/* Get Service Provider Info */
#define ASSE_ARSOP	32		/* Array Session Operation */
#define ASSE_GETPRIO	33		/* Get scheduling priority */
#define ASSE_GETRLIM	34		/* Get resource limit */
#define ASSE_CHMOD	35		/* Change file permissions */

/* ASE_BADVALUE "what" codes */
#define ASBV_UNKNOWN	0		/* Unknown value */
#define ASBV_PORTNUM	1		/* Port number */
#define ASBV_ARRAY	2		/* Array name */
#define ASBV_HOST	3		/* Host name */
#define ASBV_SRVTOKEN	4		/* Server token */
#define ASBV_OPTNAME	5		/* Option name */
#define ASBV_OPTVAL	6		/* Option value */
#define ASBV_PID	7		/* Process ID */
#define ASBV_DEST       8		/* Destination */
#define ASBV_CONNECT	9		/* Connection info */
#define ASBV_ADDRFAMILY 10		/* Address family */
#define ASBV_OPTINFO	11		/* Options information */
#define ASBV_ARGS	12		/* Command line arguments */
#define ASBV_TARGET	13		/* Target of signal */

/* ASE_BADVALUE "why" codes */
#define ASBVY_UNKNOWN	0		/* Do not know why */
#define ASBVY_RANGE	1		/* Value out of range */
#define ASBVY_PARSE	2		/* Cannot parse value */
#define ASBVY_NOTFOUND	3		/* Value not found */
#define ASBVY_SIZE	4		/* Size of value is invalid */
#define ASBVY_INVALID	5		/* Value generally invalid */
#define ASBVY_NODEFAULT 6		/* No default for unspecified value */

/* ASE_REQFAILED "why" codes  ("what" is undefined) */
#define ASRFY_UNKNOWN	0		/* Do not know why */
#define ASRFY_VERSION	1		/* Wrong version of arrayd */
#define ASRFY_TIMEOUT	2		/* Timed out waiting for command */
#define ASRFY_BADOS	3		/* Wrong version of IRIX */
#define ASRFY_NOCONNECT	4		/* Unable to connect interactively */
#define ASRFY_BADCLIENT 5		/* Client connection invalid */
#define ASRFY_CONNECTTO 6		/* Interactive connection timed out */
#define ASRFY_BADAUTH	7		/* Authentication error */

/* ASE_BADCOMMAND "why" codes ("what" is undefined) */
#define ASBCY_UNKNOWN	0		/* Do not know why */
#define ASBCY_NOTFOUND	1		/* Command not found */
#define ASBCY_EMPTY	2		/* Nothing to INVOKE */
#define ASBCY_BADUSER	3		/* Invalid USER */
#define ASBCY_BADGROUP	4		/* Invalid GROUP */
#define ASBCY_BADPROJ	5		/* Invalid PROJECT */
#define ASBCY_ILLMERGE  6		/* MERGE illegal for request */
#define ASBCY_NOINFO	7		/* No command info provided */

/* Error functions */
aserror_t asmakeerror(int, int, int, int);
void asperror(const char *, ...);
const char *asstrerror(aserror_t);


/* Destination flags */
#define ASDST_NONE   0			/* Destination not specified */
#define ASDST_LOCAL  1			/* Local machine only */
#define ASDST_SERVER 2			/* Specified server only */
#define ASDST_ARRAY  3			/* Specified array */


/*
 * Server functions
 *	Most array commands take an optional "asserver_t" argument to
 *	specify an array server other than the default. These functions
 *	create/destroy these tokens and modify options associated with them.
 */
asserver_t asopenserver(const char *, int);
void       ascloseserver(asserver_t);

int asdfltserveropt(int, void *, int *);
int asgetserveropt(asserver_t, int, void *, int *);
int assetserveropt(asserver_t, int, const void *, int);

/* Server option names */
#define AS_SO_TIMEOUT	1	/* Response timeout value */
#define AS_SO_CTIMEOUT	2	/* Connection timeout value */
#define AS_SO_FORWARD	3	/* Forwarding on/off */
#define AS_SO_LOCALKEY  4	/* Local authentication key */
#define AS_SO_REMOTEKEY 5	/* Remote authentication key */
#define AS_SO_PORTNUM	6	/* Port number (dflt only) */
#define AS_SO_HOSTNAME	7	/* Host name (dflt only) */


/*
 * ASH functions
 */
ash_t asallocash(asserver_t, const char *);
int   asashisglobal(ash_t);
ash_t asashofpid(pid_t);

typedef struct aspidlist {
	int	numpids;	/* Number of PIDs in list */
	pid_t   *pids;		/* Array of PIDs */
} aspidlist_t;

typedef struct asmachinepidlist {
	int	   numpids;	/* Number of PIDs in list */
	pid_t	   *pids;	/* Array of PIDs */
	const char *machname;	/* Name of machine */
	char	   rsrvd[44];	/* reserved for expansion */
} asmachinepidlist_t;

typedef struct asarraypidlist {
	int		nummachines;	/* Number of machines in list */
	asmachinepidlist_t **machines;	/* List of asmachpidlist_t's */
	const char	*arrayname;	/* Name of array */
	char		rsrvd[44];	/* reserved for expansion */
} asarraypidlist_t;

aspidlist_t *aspidsinash(ash_t);
aspidlist_t *aspidsinash_local(ash_t);
asmachinepidlist_t *aspidsinash_server(asserver_t, ash_t);
asarraypidlist_t *aspidsinash_array(asserver_t, const char *, ash_t);


/*
 * Array command functions
 */
typedef struct ascmdreq {
	char	 *array;	/* Name of target array */
	uint32_t flags;		/* Option flags */
	int	 numargs;	/* Number of arguments */
	char	 **args;	/* Cmd arguments (ala argv) */
	uint32_t ioflags;	/* I/O flags for interactive commands */

	char     rsrvd[100];	/* reserved for expansion: init to 0's */
} ascmdreq_t;

#define ASCMDREQ_LOCAL		0x80000000	/* Do not broadcast to array */
#define ASCMDREQ_NEWSESS	0x40000000	/* Start new array session */
#define ASCMDREQ_OUTPUT		0x20000000	/* Collect output from cmd */
#define ASCMDREQ_NOWAIT		0x10000000	/* Do not wait on command */
#define ASCMDREQ_INTERACTIVE	0x08000000	/* Run interactively */ 

#define ASCMDIO_STDIN		0x80000000	/* Provide stdin to command */
#define ASCMDIO_STDOUT		0x40000000	/* Provide stdout to command */
#define ASCMDIO_STDERR		0x20000000	/* Provide stderr to command */
#define ASCMDIO_SIGNAL		0x10000000	/* Send signals to command */
#define ASCMDIO_OUTERRSHR	0x08000000	/* Provide stderr via stdout */

#define ASCMDIO_FULLIO		(ASCMDIO_STDIN  | \
				 ASCMDIO_STDOUT | \
				 ASCMDIO_STDERR | \
				 ASCMDIO_SIGNAL)	/* Provide full I/O */


typedef struct ascmdrslt {
	char	  *machine;	/* Name of responding machine */
	ash_t	  ash;		/* ASH of running command */
	uint32_t  flags;	/* Result flags */
	aserror_t error;	/* Error code for this command */
	int	  status;	/* Exit status */
	char	  *outfile;	/* Name of output file */

	uint32_t  ioflags;	/* I/O connections (see ascmdreq_t) */
	int	  stdinfd;	/* File descriptor for command's stdin */
	int	  stdoutfd;	/* File descriptor for command's stdout */
	int	  stderrfd;	/* File descriptor for command's stderr */
	int	  signalfd;	/* File descriptor for sending signals */

	/* stay tuned for future expansion */
} ascmdrslt_t;

#define ASCMDRSLT_OUTPUT	0x80000000	/* Output available */
#define ASCMDRSLT_MERGED	0x40000000	/* Output is merged */
#define ASCMDRSLT_ASH		0x20000000	/* ASH is available */
#define ASCMDRSLT_INTERACTIVE	0x10000000	/* I/O connections available */


typedef struct ascmdrsltlist {
	int	numresults;	/* Number of ascmdrslt_t's */
	ascmdrslt_t **results;	/* Array of ascmdrslt_t pointers */
} ascmdrsltlist_t;


ascmdrsltlist_t *ascommand(asserver_t, const ascmdreq_t *);


/*
 * Remote execution functions
 */
int asrcmd(asserver_t, char *, char *, int *);
int asrcmdv(asserver_t, char *, char **, int *);



/*
 * aslist<> functions
 *	These functions return a malloc'ed list of items of the specified
 *	type containing configuration or status information. The corresponding
 *	"asfree<>" functions may be used to release the storage allocated by
 *	the "aslist<>" functions.
 */
typedef struct asarray {
	const char *name;		/* Name of array */
	int	   numattrs;		/* Number of attribute strings */
	const char **attrs;		/* List of attribute strings */
	u_short	   ident;		/* Array ID */
} asarray_t;

typedef struct asarraylist {
	int	  numarrays;		/* Number of arrays in list */
	asarray_t **arrays;		/* Array of asarray_t pointers */
} asarraylist_t;

asarray_t *asgetdfltarray(asserver_t);
asarraylist_t *aslistarrays(asserver_t);


typedef struct asashlist {
	int	numashs;		/* Number of ASHs in list */
	ash_t   *ashs;			/* Array of ASHs */
} asashlist_t;

asashlist_t *aslistashs(asserver_t, const char *, int Dest, uint32_t Flags);
asashlist_t *aslistashs_array(asserver_t, const char *);
asashlist_t *aslistashs_local(void);
asashlist_t *aslistashs_server(asserver_t);

/* Control flags for aslistashs */
#define ASLAF_NOLOCAL	0x00000001	/* Global ASHs only */
#define ASLAF_NODUPS	0x00000002	/* Remove duplicate ASHs */
#define ASLAF_NODUPES	0x00000002	/* (for compatibility) */


typedef struct asmachine {
	const char     *name;		/* Familiar name of machine */
	const char     *hostname;	/* Network hostname of machine */
	int	       numattrs;	/* Number of attribute strings */
	const char     **attrs;		/* List of attribute strings */
	struct in_addr inaddr;		/* IP address of machine */
	u_short	       portnum;		/* Port # of array daemon */
	asident_t      ident;		/* ID of array daemon */
} asmachine_t;

typedef struct asmachinelist {
	int	nummachines;		/* Number of machines in list */
	asmachine_t **machines;		/* Array of asmachine_t pointers */
} asmachinelist_t;

asmachinelist_t *aslistmachines(asserver_t, const char *ArrayName);


/*
 * asfree<> functions
 *	These functions are used to release the storage allocated by other
 *	libarray functions (e.g. the aslist<> functions)
 */
void asfreearray(asarray_t *, uint32_t);
void asfreearraylist(asarraylist_t *, uint32_t);
void asfreearraypidlist(asarraypidlist_t *, uint32_t);
void asfreeashlist(asashlist_t *, uint32_t);
void asfreecmdrslt(ascmdrslt_t *, uint32_t);
void asfreecmdrsltlist(ascmdrsltlist_t *, uint32_t);
void asfreemachine(asmachine_t *, uint32_t);
void asfreemachinelist(asmachinelist_t *, uint32_t);
void asfreemachinepidlist(asmachinepidlist_t *, uint32_t);
void asfreepidlist(aspidlist_t *, uint32_t);

#define ASFLF_FREEDATA	0x80000000	/* Free individual list items */
#define ASFLF_UNLINK    0x40000000	/* Unlink temporary files */
#define ASFLF_CLOSEIO	0x20000000	/* Close I/O connections */
#define ASFLF_CLOSESRV	0x10000000	/* Close server token */


/*
 * Option parsing functions
 *	Used by array services clients to provide a standard set of
 *	command line options.
 */
typedef struct asoptinfo {
	int	argc;		/* Number of unparsed arguments */
	char	**argv;		/* Array of ptrs to unparsed arguments */

	int	valid;		/* Bitmap of valid fields */
	int	invalid;	/* Bitmap of fields with invalid arguments */
	int	options;	/* Option flags */

	asserver_t token;	/* Token for specified server */

	char	*server;	/* ASCII name of target server */
	char	*array;		/* Name of array to operate on */
	askey_t	localkey;	/* Local authentication key */
	askey_t remotekey;	/* Remote authentication key */
	ash_t	ash;		/* ASH to operate on */
	pid_t	pid;		/* PID to operate on */
	int	portnum;	/* Port number of arrayd */
	int	timeout;	/* Response timeout in seconds */
	int	connectto;	/* Connect timeout in seconds */
	int	verbose;	/* Verbose level */
} asoptinfo_t;

/* Select/Valid flags */
#define ASOIV_TOKEN	0x00000001	/* Server token */
#define ASOIV_SERVER	0x00000002	/* Server name */
#define ASOIV_ARRAY	0x00000004	/* Array name */
#define ASOIV_LCLKEY	0x00000008	/* Local authentication key */
#define ASOIV_REMKEY	0x00000010	/* Remote authentication key */
#define ASOIV_ASH	0x00000020	/* Array session handle */
#define ASOIV_PID	0x00000040	/* Process ID */
#define ASOIV_PORTNUM	0x00000080	/* Port number */
#define ASOIV_TIMEOUT	0x00000100	/* Timeout */
#define ASOIV_CONNECTTO 0x00000200	/* Connect timeout */
#define ASOIV_VERBOSE	0x00000400	/* Verbose level */
#define ASOIV_FORWARD	0x00000800	/* Forwarding mode */
#define ASOIV_LOCAL	0x00001000	/* Local mode */

#define ASOIV_STD	(ASOIV_SERVER | ASOIV_ARRAY | ASOIV_LCLKEY | \
			 ASOIV_REMKEY | ASOIV_PORTNUM | ASOIV_TIMEOUT | \
			 ASOIV_CONNECTTO | ASOIV_FORWARD)

/* Option flags */
#define ASOIO_LOCAL	0x00000001	/* Local machine only */
#define ASOIO_FORWARD	0x00000002	/* Forward request via local server */

/* Control flags */
#define ASOIC_LOGERRS	0x00000001	/* Print error messages */
#define ASOIC_NODUPS	0x00000002	/* Do not allow redundant args */
#define ASOIC_OPTSONLY	0x00000004	/* Stop at first non-option arg */
#define ASOIC_SELONLY	0x00000008	/* Stop at first unselected arg */

void asfreeoptinfo(asoptinfo_t *, uint32_t);
asserver_t  asopenserver_from_optinfo(const asoptinfo_t *);
asoptinfo_t *asparseopts(int argc, char **argv, int select, int control);


/* Signal functions */
int askillpid_server(asserver_t, pid_t, int signum);

int askillash_array(asserver_t, const char *array, ash_t, int signum);
int askillash_local(ash_t, int signum);
int askillash_server(asserver_t, ash_t, int signum);


/*
 * Constants pertaining to ASH's and PRID's
 */
#ifndef lint
#define ASBADASH	-1LL	/* An invalid Array Session Handle */
#define ASMINASH	0LL	/* Smallest valid Array Session Handle */
#define ASNOASH		0LL	/* "No ASH specified" */
#define ASASHMASK	0x7FFFFFFFFFFFFFFFLL	/* Valid ASH bits */

#define ASBADPRID	-1LL	/* An invalid project ID */
#define ASMINPRID	0LL	/* Smallest valid project ID */
#define ASNOPRID	0LL	/* "No project ID specified" */

#else
/* Bogus values that don't use lint-unfriendly "LL" */
#define ASBADASH	-1
#define ASMINASH	0
#define ASNOASH		0
#define ASASHMASK	0x7FFFFFFF
#define ASBADPRID	-1
#define ASMINPRID	0
#define ASNOPRID	0
#endif


/*
 * Miscellaneous functions
 */
const char *asgetattr(const char *, const char **, int);

#endif /* C || C++ */

#if defined(_LANGUAGE_C_PLUS_PLUS)
}
#endif

#endif  /* !__ARRAYSVCS_H_ */
