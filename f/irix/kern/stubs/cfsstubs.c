#include <sys/types.h>
#include <sys/systm.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <ksys/cell.h>
#include <sys/ktrace.h>

vfsops_t cfs_vfsops;
vnodeops_t dsvn_ops;
int	cfs_fstype = -2;	/* Bad vfs type which will not match anything */

int dvn_trace_mask = 0;
ktrace_t *dvn_trace_buf = NULL;
void *dcvn_msg_list[];
void *dsvn_msg_list[];
void *dcvfs_msg_list[];
void *dsvfs_msg_list[];

