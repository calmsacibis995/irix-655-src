#include "sgidefs.h"
#include <sys/errno.h>
#include <sys/types.h>
#include <fs/xfs/xfs_clnt.h>

void *dsxvn_msg_list[];
void *cxfs_array_msg_list[];

/*
 * cxfs stubs
 */
void	cxfs_init(){}
void	cxfs_arrinit() {}

void	*cxfs_dcxvn_make() { return((void *)0); }
void    cxfs_dcxvn_obtain() {}

/*
 * cxfs_dcxvn_recall should really never be called. But, let's be nice
 * and return the tkset passed in. But, we can't include tkm.h because
 * non-CELL can't include tkm.h so let's copy the typedefs here.
 */

typedef __uint64_t tk_set_t;
typedef __uint64_t tk_disp_t;

/* ARGSUSED */
tk_set_t cxfs_dcxvn_recall(void *dp, tk_set_t tk, tk_disp_t td) { return(tk); }

void    cxfs_dcxvn_return() {}
void 	cxfs_dcxvn_destroy() {}

void	*cxfs_dsxvn_make() { return(0); }
int	cxfs_dsxvn_obtain() { return(0); }
int	cxfs_allocate_extents() { return ENOTSUP; }
int	cxfs_sync_inode() { return ENOTSUP; }
int	cxfs_clear_setuid() { return ENOTSUP; }
void	cxfs_dsxvn_obtain_done() {}
void	cxfs_dsxvn_return() {}
void	cxfs_dsxvn_destroy() {}
void	cxfs_inode_quiesce() {}
int     cxfs_inode_qset() { return 0; }

int 	cxfs_remount_server() { return EINVAL; }
int	cxfs_force_shutdown() { return EINVAL; }
void	*get_cxfs_mountp() { return 0; }
void	cxfs_unmount() {}
void	cxfs_sethostname() {}
void	cxfs_strat_complete_buf() {};

int	cxfs_dsxvn_oplock_req() { return 0; }

/* ARGSUSED */
int
cxfs_mount(
        void *mp,
        struct xfs_args *ap,
        dev_t dev,
        int *clp)
{
        *clp = 0;
        if (ap && XFSARGS_FOR_CXFSARR(ap))
                return (EINVAL);
        else
                return (0);
}

