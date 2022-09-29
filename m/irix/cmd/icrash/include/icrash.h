#ident "$Header: /proj/irix6.5m/isms/irix/cmd/icrash/include/RCS/icrash.h,v 1.77 1999/05/25 19:21:38 tjm Exp $"
#ifndef ICRASH_H
#define ICRASH_H

/* Pick up the icrash error codes 
 */
#include "error.h"

#define ICRASH_PROGRAM  1
#define FRU_PROGRAM     2

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Definitions for banner printouts.
 */
#define BANNER 1                     /* header printout (columns)            */
#define SMAJOR 2                     /* major separator (using '=' sign)     */
#define SMINOR 4                     /* minor separator (using '-' sign)     */

/* Maximum icrash block size (for alloc_block(), etc.)
 */
#define I_NBPB 4096

struct command {
	FILE *fp;            /* Global outfile -- temporary */
	FILE *ofp;
	char *args[128];
	char com[256];
	int nargs;
	int flags;
	char pipe_cmd[256];
};

#define command_t struct command

struct procent {
	long isym;
	long iline;
	long regmask;
	short regoffset;
	short frameoffset;
};

/*
 * Macro definitions for help and usage.
 */
extern char *helpformat(char *);

#define CMD_HELP(cmd, u, s) \
	char *t = helpformat(s); \
	fprintf(cmd.ofp, "COMMAND: %s %s\n\n%s\n", cmd.com, u, t); \
	free (t);

#define CMD_USAGE(cmd, s) \
	fprintf(cmd.ofp, "USAGE: %s %s\n", cmd.com, s);

/*
 * flags that can be globally set. Note that the first three flags must 
 * match the coresponding flags in klib.h 
 */
#define C_B_TEMP   K_TEMP      /* Allocate a B_TEMP block (K_TEMP)    */
#define C_B_PERM   K_PERM      /* Allocate a B_PERM block (K_PERM)    */
#define C_ALL      K_ALL       /* -a option (K_ALL)                   */
#define C_INDENT   K_INDENT    /* indentation, used with -n           */
#define C_FULL     K_FULL      /* -f option                           */
#define C_SUPRSERR K_SUPRSERR  /* Suppress error msgs (during report) */
#define C_DECIMAL  K_DECIMAL   /* -d option                           */
#define C_OCTAL    K_OCTAL     /* -o option                           */
#define C_HEX      K_HEX       /* -x option                           */

/* 
 * icrash specific flags
 */
#define C_NEXT     0x00000200  /* -n option                           */
#define C_PIPE     0x00000400  /* setting of '|' to output            */
#define C_WRITE    0x00000800  /* -w option (takes argument)          */
#define C_PROC     0x00001000  /* -p option (takes argument)          */
#define C_VERTEX   0x00002000  /* treat parameter as vertex           */
#define C_NOVARS   0x00004000  /* option specific to 'whatis'         */
#define C_WHATIS   0x00008000  /* option specific to 'whatis'         */
#define C_TRUE     0x00010000  /* true (requires command argument)    */
#define C_FALSE    0x00020000  /* false (does not require argument)   */
#define C_MAYBE    0x00040000  /* maybe (might require an argument)   */
#define C_DWORD    0x00080000  /* double word (-D)                    */
#define C_WORD     0x00100000  /* word (-W)                           */
#define C_HWORD    0x00200000  /* half word (-H)                      */
#define C_BYTE     0x00400000  /* byte (-B)                           */
#define C_MASK     0x00800000  /* mask for search (takes argument)    */
#define C_STRING   0x01000000  /* string for search (takes argument)  */
#define C_LIST     0x02000000  /* -l option                           */
#define C_SELF     0x04000000  /* self-parsing flag                   */
#define C_SIZEOF   0x08000000  /* used by sizeof func in print        */
#define C_STRUCT   0x10000000  /* print output as C-type struct       */
#define C_COMMAND  0x20000000  /* Treat eval var as a command line    */
#define C_KTHREAD  0x40000000  /* Display thread as a kthread 		  */
#define C_SIBLING  0x80000000  /* Display all siblings (e.g. uthreads)*/

/*
 * Reusing C_FLAGS now..
 */
#define C_UTHREAD  C_PROC

#define FPRINTF_KADDR(f, s, a) \
	if (PTRSZ64) { \
		fprintf(f, "%16llx", (a)); \
	} \
	else { \
		fprintf(f, "%8llx", (a)); \
		fprintf(f, "        "); \
	} \
	if (strlen(s)) { \
		fprintf(f, "%s", (s)); \
	}

#define indent_it(x, y) if (x & C_INDENT) { fprintf(y, "  "); }

/* Maximum number of bytes to allow for pattern matching in search command 
 */
#define MAX_SEARCH_SIZE 256

/* Definitions for print_line(), for carriage returns.
 */
#define PL_NONE  0x0000             /* No carriage returns        */
#define PL_ZERO  0x0000             /* No carriage returns        */
#define PL_BEGIN 0x0001             /* Carriage return at BOL     */
#define PL_END   0x0002             /* Carriage return at EOL     */
#define PL_ALL   0x0004             /* Carriage return, both ends */
#define PL_BOTH  0x0004             /* Carriage return, both ends */

/* Maximum history buffer allowed.
 */
#define MAX_HISTORY 50

/* Timer definition.
 */
#define timerdiff(tv1p, tv2p, tv3p) \
	(tv3p)->tv_sec = (tv1p)->tv_sec - (tv2p)->tv_sec; \
	(tv3p)->tv_usec = (tv1p)->tv_usec - (tv2p)->tv_usec; \
	if ((tv2p)->tv_usec > (tv1p)->tv_usec) { \
		(tv3p)->tv_usec += 1000000; \
		(tv3p)->tv_sec--; \
	}

/* Plural flag definitions.
 */
#define PLURAL(str, cnt, ofp) \
	fprintf(ofp, "%d %s%s found\n", cnt, str, (cnt != 1) ? "s" : "");
#endif

/* Application level debug classes
 */
#define DC_GLOBAL           0x0000000000000000
#define DC_FUNCTRACE        0x0000000000000010
#define DC_ALLOC            0x0000000000000020
#define DC_BUF              0x0000000000000040
#define DC_DWARF            0x0000000000000080
#define DC_ELF              0x0000000000000100
#define DC_EVAL             0x0000000000000200
#define DC_FILE             0x0000000000000400
#define DC_KTHREAD          0x0000000000000800
#define DC_MEM           	0x0000000000001000
#define DC_PAGE             0x0000000000002000
#define DC_PDA              0x0000000000004000
#define DC_PROC             0x0000000000008000
#define DC_REGION           0x0000000000010000
#define DC_SEMA             0x0000000000020000
#define DC_SNODE            0x0000000000040000
#define DC_SOCKET           0x0000000000080000
#define DC_STREAM           0x0000000000100000
#define DC_STRUCT           0x0000000000200000
#define DC_SYM              0x0000000000400000
#define DC_TRACE            0x0000000000800000
#define DC_USER             0x0000000001000000
#define DC_VNODE            0x0000000002000000
#define DC_ZONE             0x0000000004000000

/*
 * Platform specific defines.
 */
#define PLATFORM_EVEREST (K_IP == 21 || K_IP == 25 || K_IP == 19)
#define PLATFORM_SNXX    (K_IP == 27 || K_IP == 29)

/* Include all other icrash header files so that only icrash.h needs
 * to be included in any icrash source file.
 */
#include "eval.h"
#include "variable.h"
#include "extern.h"
#include "cmds.h"
#include "stream.h"

