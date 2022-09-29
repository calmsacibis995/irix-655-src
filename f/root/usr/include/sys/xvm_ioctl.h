#ifndef __XVM_IOCTL_H__
#define __XVM_IOCTL_H__

/**************************************************************************
 *                                                                        *
 *            Copyright (C) 1998, Silicon Graphics, Inc.                  *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/
#ident "$Revision: 1.1 $"

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structure returned by the DIOCGETXVM ioctl() to describe the
 * subvolume geometry.
 */
#define XVM_DISK_GEOMETRY_VERS	1
typedef struct xvm_disk_geometry_s {
	__uint32_t		version;
	__int32_t		trk_size;	/* in blocks */

	__uint64_t		subvol_size;	/* in blocks */
} xvm_disk_geometry_t;

/*
 * Structure returned by the DIOCGETXLVDEV ioctl to list the
 * subvolume device nodes in a volume.  These are external device
 * numbers.
 */
#define XVM_GETDEV_VERS 1
typedef struct {
        __uint32_t              version;
        dev_t                   data_subvol_dev;

        dev_t                   log_subvol_dev;
        dev_t                   rt_subvol_dev;

	dev_t			sp_subvol_dev;
} xvm_getdev_t;

/*
 * Structure returned by the DIOCGETXVMSTRIPE ioctl() to describe the
 * subvolume stripe units and width.
 */
#define XVM_SUBVOL_GEOMETRY_VERS  1
typedef struct xvm_subvol_stripe_s {
        __uint32_t              version;
        __uint32_t              unit_size;      /* in blocks */
        __uint32_t              width_size;     /* in blocks */
	__uint32_t		pad1;		/* padding */
	dev_t			dev;
} xvm_subvol_stripe_t;


#ifdef __cplusplus
}
#endif

#endif
#endif /* __XVM_IOCTL_H__ */
