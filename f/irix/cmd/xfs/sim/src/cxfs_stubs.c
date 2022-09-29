#define _KERNEL 1
#include <sys/types.h>
#include <sys/buf.h>
#undef _KERNEL
#include <sys/cred.h>
#include <sys/sema.h>
#include <sys/vnode.h>
#include <sys/uuid.h>
#include <sys/fs/xfs_types.h>
#include <sys/fs/xfs_inum.h>
#include <sys/fs/xfs_log.h>
#include <sys/fs/xfs_trans.h>
#include <sys/fs/xfs_sb.h>
#include <sys/fs/xfs_dir.h>
#include <sys/fs/xfs_mount.h>

#include "sim.h"


/* ARGSUSED */
void
cxfs_arrinit() {}
 
void
cxfs_unmount() {}

/* ARGSUSED */
void
cxfs_export(
	struct vfs *vfsp,
	struct xfs_mount *mp) 
{
        return;
}

/* ARGSUSED */
void
cxfs_expinfo_free(
	struct xfs_mount *mp) 
{
        return;
}
