#ifndef __SYS_XFS_CLNT_H__
#define __SYS_XFS_CLNT_H__

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1993, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ident "$Revision: 1.15 $"

#include <sys/types.h>
 
/*
 * XFS arguments to the mount system call.
 */
struct xfs_args {

        /*
	 * These items common to all versions.
	 */
	int	version;	/* version of this */
				/* 1, see xfs_args_ver_1 */
				/* 2, see xfs_args_ver_2 */
				/* 3, see xfs_args_ver_3 */
				/* 4, see xfs_args_ver_4 */
	int	flags;		/* flags, see XFSMNT_... below */
	int	logbufs;	/* Number of log buffers, -1 to default */
	int	logbufsize;	/* Size of log buffers, -1 to default */
	char	*fsname;	/* filesystem name (mount point) */

	/*
	 * The following items added in version 2.  They are for stripe
         * aligment.  Set 0 for no alignment handling (see XFSMNT_NOALIGN 
	 * flag).
	 */
	int	sunit;		/* stripe unit (bbs) */
	int	swidth;		/* stripe width (bbs), multiple of sunit */

        /*
         * The following items added in version 3.
	 */
	uchar_t	iosizelog;	/* log2 of the preferred I/O size */
	uchar_t	reserved_0;	/* reserved fields */
	short	reserved_1;
	int	reserved_2;
	int	reserved_3;

        /*
	 * The following items added in version 4.  This stuff is for
         * cxfs support.
	 */
        char    **servlist;     /* Table of hosts which may be servers */
        int     *servlistlen;   /* Table of hostname lengths. */
        int     slcount;        /* Count of hosts which may be servers. */
        int     stimeout;       /* Server timeout in milliseconds */
        int     ctimeout;       /* Client timeout in milliseconds */
        char    *server;        /* Designated server hostname (for remount). */
        int     servlen;        /* Length of server hostname (for remount). */
        int     servcell;       /* Server cell (internal testing only) */
};

#ifdef _KERNEL

#include <fs/xfs/xfs_types.h>
 
struct xfs_args32_ver_1 {
	__int32_t	version;
	__int32_t	flags;
	__int32_t	logbufs;
	__int32_t	logbufsize;
	app32_ptr_t	fsname;
};

struct xfs_args32_ver_2 {
	__int32_t	version;
	__int32_t	flags;
	__int32_t	logbufs;
	__int32_t	logbufsize;
	app32_ptr_t	fsname;
	__int32_t	sunit;
	__int32_t	swidth;
};

struct xfs_args32_ver_3 {
	__int32_t	version;
	__int32_t	flags;
	__int32_t	logbufs;
	__int32_t	logbufsize;
	app32_ptr_t	fsname;
	__int32_t	sunit;
	__int32_t	swidth;
	uint8_t		iosizelog;
	uint8_t		reserved_3_0;
	__int16_t	reserved_3_1;
	__int32_t	reserved_3_2;
	__int32_t	reserved_3_3;
};

struct xfs_args32_ver_4 {
	__int32_t	version;
	__int32_t	flags;
	__int32_t	logbufs;
	__int32_t	logbufsize;
	app32_ptr_t	fsname;
	__int32_t	sunit;
	__int32_t	swidth;
	uint8_t		iosizelog;
	uint8_t		reserved_3_0;
	__int16_t	reserved_3_1;
	__int32_t	reserved_3_2;
	__int32_t	reserved_3_3;
        app32_ptr_t     servlist;
        app32_ptr_t     servlistlen;
  	__int32_t	slcount;
        __int32_t       stimeout;
        __int32_t       ctimeout;
        app32_ptr_t     server;
  	__int32_t	servlen;
  	__int32_t	servcell;
};

struct xfs_args_ver_1 {
	int	version;
	int	flags;
	int	logbufs;
	int	logbufsize;
	char	*fsname;
};

struct xfs_args_ver_2 {
	int	version;
	int	flags;
	int	logbufs;
	int	logbufsize;
	char	*fsname;
	int	sunit;
	int	swidth;
};

struct xfs_args_ver_3 {
	int	version;
	int	flags;
	int	logbufs;
	int	logbufsize;
	char	*fsname;
	int	sunit;
	int	swidth;
	uchar_t	iosizelog;
	uchar_t	reserved_0;
	short	reserved_1;
	int	reserved_2;
	int	reserved_3;
};

#define XFSARGS_FOR_CXFSARR(ap)		\
	((ap)->servlist || (ap)->slcount >= 0 || \
	 (ap)->stimeout >= 0 || (ap)->ctimeout >= 0 || \
	 (ap)->flags & (XFSMNT_CLNTONLY | XFSMNT_UNSHARED))

#endif /* _KERNEL */

/*
 * XFS mount option flags
 */
#define	XFSMNT_CHKLOG		0x00000001	/* check log */
#define	XFSMNT_WSYNC		0x00000002	/* safe mode nfs mount
						 * compatible */
#define	XFSMNT_INO64		0x00000004	/* move inode numbers up
						 * past 2^32 */
#define XFSMNT_UQUOTA		0x00000008	/* user quota accounting */
#define XFSMNT_PQUOTA		0x00000010	/* project quota accounting */
#define XFSMNT_UQUOTAENF	0x00000020	/* user quota limit
						 * enforcement */
#define XFSMNT_PQUOTAENF	0x00000040	/* project quota limit
						 * enforcement */
#define XFSMNT_QUOTAMAYBE	0x00000080	/* don't turn off if SB
						 * has quotas on */
#define XFSMNT_NOATIME		0x00000100	/* don't modify access
						 * times on reads */
#define XFSMNT_NOALIGN		0x00000200	/* don't allocate at
						 * stripe boundaries*/
#define XFSMNT_RETERR		0x00000400	/* return error to user */
#define XFSMNT_NORECOVERY	0x00000800	/* no recovery, implies
						 * read-only mount */
#define XFSMNT_SHARED		0x00001000	/* shared XFS mount */
#define XFSMNT_IOSIZE		0x00002000	/* optimize for I/O size */
#define XFSMNT_OSYNCISDSYNC	0x00004000	/* treat o_sync like o_dsync */
#define XFSMNT_CLNTONLY         0x00008000	/* cxfs mount as client only */
#define XFSMNT_UNSHARED         0x00010000	/* cxfs filesystem mounted
                                                 * unshared */
#define XFSMNT_CHGCLNTONLY      0x00020000      /* changing client only flag */
                                                /* (for remount only) */
#define XFSMNT_SERVCELL         0x00040000      /* setting server cell */
                                                /* (allowed on remount) */
#define XFSMNT_MAKESERVER       0x00080000      /* become the server (remount */
                                                /* only) */
#define XFSMNT_NOTSERVER        0x00100000      /* give up being the server */
                                                /* (remount only) */

#endif /* !__SYS_XFS_CLNT_H__ */
