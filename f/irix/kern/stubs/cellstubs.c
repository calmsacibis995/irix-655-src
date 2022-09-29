#include <sys/types.h>
#include <sys/systm.h>
#include <sys/vfs.h>
#include <sys/errno.h>
#include <ksys/cell.h>
#include <sys/ktrace.h>

int cell_enabled = 0;
cell_t my_cellid = CELL_NONE;

cell_t cellid(){ return 0;}
pid_t kthread_get_pid() { return(0); }

#ifdef CELL_CAPABLE
void
cell_array_preinit(void)
{
	return;
}

void
tsv_init(void)
{
	return;
}

void
bla_init()
{
	return;
}

void
cms_config_init()
{
	return;
}

void
bhv_global_dp_init()
{
	return;
}

/* ARGSUSED */
void
kthread_cell_init(kthread_t *kt)
{
	return;
}

/* ARGSUSED */
void
kthread_cell_destroy(kthread_t *kt)
{
	return;
}

/* ARGSUSED */
int
thread_remote_interrupt(kthread_t *kt, int *s)
{
	return(0);
}

int
sysget_array_cell()
{
	return(0);
}

void
credid_lookup_remove()
{
	return;
}


/* ARGSUSED */
void
bhv_queue_ucallout(
        bhv_head_t      *bhp,
	int		flags,
        bhv_ucallout_t  *func,
        void            *arg1,
        void            *arg2,
        caddr_t         argv,
        size_t          argvsz)
{
	return;
}

/* ARGSUSED */
int
bhv_try_deferred_ucalloutp(bhv_head_lock_t* bhl)
{
	noreach();
	/* NOTREACHED */
}


/* ARGSUSED */
void
bla_prevent_starvation(int backup_pc)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
int
bla_test_defer_barrier(mrlock_t *mrp)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
#ifdef BLALOG
_bla_push(mrlock_t *mrp, int rwi, void *ra)
#else
_bla_push(mrlock_t *mrp, int rwi)
#endif
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
_bla_pop(mrlock_t *mrp)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
_bla_isleep(void)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
_bla_iunsleep(void)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
uint_t
_bla_wait_for_mrlock(mrlock_t *mrp)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
_bla_got_mrlock(uint_t arg)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
void
bhv_print_ucallout(bhv_head_t *bhp)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
pfd_t *
vnode_page_cell_migrate(
	pfd_t *opfd,
	pfd_t *npfd,
	int why)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
int
do_ucopy_copyin(
        caddr_t src,
        caddr_t dst,
        size_t  len)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
int
do_ucopy_copyout(
        caddr_t src,
        caddr_t dst,
        size_t  len)
{
	noreach();
	/* NOTREACHED */
}

/* ARGSUSED */
int
do_ucopy_zero(
        caddr_t dst,
        size_t  len)
{
	noreach();
	/* NOTREACHED */
}

#endif
