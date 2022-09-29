#ifndef TYPES_H
#define TYPES_H

#include <sys/types.h>

#ident "$Header: /proj/irix6.5m/isms/eoe/cmd/rfind/include/RCS/types.h,v 1.3 1995/08/29 18:26:31 doucette Exp $"

/* fundamental page size - probably should not be hardwired, but
 * for now we will
 */
#define PGSZLOG2	12
#define PGSZ		( 1 << PGSZLOG2 )
#define PGMASK		( PGSZ - 1 )

/* integers
 */
typedef unsigned char u_char_t;
typedef u_int32_t u_intgen_t;
typedef unsigned long long int u_int64_t;
typedef int32_t intgen_t;
typedef long long int int64_t;
typedef unsigned long u_long_t;
typedef size_t ix_t;
typedef u_int32_t size32_t;
typedef u_int64_t size64_t;

/* limits
 */
#define UINT16MAX	0xffff
#define INT32MAX	0x7fffffff
#define UINT32MAX	0xffffffff
#define INTGENMAX	INT32MAX
#define UINTGENMAX	UINT32MAX
#define INT64MAX	0x7fffffffffffffffll
#define UINT64MAX	0xffffffffffffffffll
#define OFFMAX		INT32MAX
#define SIZEMAX		UINT32MAX
#define INOMAX		UINT32MAX
#define INO64MAX	UINT64MAX
#define OFF64MAX	INT64MAX
#define TIMEMAX		INT32MAX

/* boolean
 */
typedef intgen_t bool_t;
#define BOOL_TRUE	1
#define BOOL_FALSE	0
#define BOOL_UNKNOWN	( -1 )
#define BOOL_ERROR	( -2 )

/* useful return code scheme
 */
typedef enum { RV_OK,		/* mission accomplished */
	       RV_NOMORE,	/* no more work to do */
	       RV_EOD,		/* ran out of data */
	       RV_ERROR,	/* operator error or resource exhaustion */
	       RV_DONE,		/* return early because someone else did work */
	       RV_INTR,		/* cldmgr_stop_requested( ) */
	       RV_CORRUPT,	/* stopped because corrupt data encountered */
	       RV_QUIT,		/* stop using resource */
	       RV_DRIVE,	/* drive quit working */
	       RV_CORE		/* really bad - dump core! */
} rv_t;

/* typedefs I'd like to see ...
 */
typedef struct stat stat_t;
typedef struct stat64 stat64_t;
typedef struct getbmap getbmap_t;

#endif /* TYPES_H */
