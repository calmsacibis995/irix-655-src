/**************************************************************************
 *									  *
 *	      Copyright (C) 1998, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs	 contain  *
 *  unpublished	 proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may	not be disclosed  *
 *  to	third  parties	or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ident "$Revision: 1.1 $"

#include <sys/ddi.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/uuid.h>
#include <sys/major.h>
#include <sys/hwgraph.h>

int volume_get_devts (dev_t, dev_t *, dev_t *, dev_t *, dev_t *, int *);
extern int xlv_get_subvolumes(  dev_t device,
				dev_t *ddev,
				dev_t *logdev,
				dev_t *rtdev );
extern int xvm_get_subvolumes(  dev_t xvm_dev,
				dev_t *data,
				dev_t *log,
				dev_t *realtime,
				dev_t *secondary_partition);
extern int xvm_dev(dev_t device);

int
volume_get_devts (
	dev_t device, dev_t *ddev, dev_t *logdev, dev_t *rtdev,
	dev_t *secondary, int *status)
{

	*status = 0;
	*ddev = *logdev = *rtdev = *secondary = 0;

	if (emajor(device) == XLV_MAJOR) {
		*status = 1;
		if (xlv_get_subvolumes(device, ddev, logdev, rtdev) != 0) {
			return (ENODEV);
		}
		return(0);
	}

	if (*status = xvm_dev(device)) {
		if (xvm_get_subvolumes(device, ddev, logdev,
					rtdev, secondary) != 0) {
			return (ENODEV);
		}
		if (!*logdev)
			*logdev = *ddev;
		return(0);
	}

	return(0);
}

