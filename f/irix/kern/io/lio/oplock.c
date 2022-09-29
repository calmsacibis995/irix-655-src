/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992-1998 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*
 * oplock - opportunistic locks for SMB servers
 *
 * $Revision: 1.6 $
 *
 * Overview
 *
 * This driver provides kernel-level opportunistic locks for user-level
 * SMB servers like the Samba daemon.  The server will open a file on
 * behalf of an SMB client and then attempt to register an oplock for
 * that file.  If no other references are held on the file, an exclusive
 * oplock is given to the server which grants it to the SMB client.
 *
 * Once an oplock is granted, any operation on the file by a thread
 * other than one in the registered process will cause the oplock to
 * be revoked or at least broken down to a Level II (read-only) oplock.
 * Note that any access to mutable state contained in or assocociated
 * with the file (data, attributes, etc) will be mediated through such
 * an operation.  Vnode references which occur without doing any vnode
 * operations are not included in the above and will not cause an oplock
 * to be revoked.  When an oplock is revoked, the SMB server is informed
 * so that it can inform the SMB client.  When the SMB client is done
 * flushing its cache, the SMB server acknowledges the kernel's oplock
 * revoke message.  All external operations on the file hang until the
 * SMB server ACK comes in or until we systunably timeout.
 *
 * User Interface
 *
 * There are three fcntl commands added to support oplocks.  We also
 * write to a user-supplied pipe to provide a signalling mechanism
 * for detection by select().
 *
 * fcntl(fd, F_OPLKREG, p[1])
 * The oplock registration fcntl identifies the file to oplock and the
 * write side of the pipe to use as the signalling mechanism.  The same
 * write side pipe can be used for any number of oplocked files.
 *
 * fcntl(p[1], F_OPLKSTAT, &os)
 * The oplock state change fcntl is used to get state change information
 * on recently externally referenced files.  The returned oplock_stat_t
 * structure contains the current state and the dev/ino information to
 * identify the file.  This is only done on backchannels (eg p) that
 * select() indicates there is a byte of data to read.
 *
 * fcntl(fd, F_OPLKACK, state)
 * The oplock acknowledgement fcntl is primarily used to respond to
 * oplock state changes due to external references.  The state given
 * should match the state reported in the os.os_state field from the
 * F_OPLKSTAT command.
 * The F_OPLKACK command is also used to check the current state of
 * the oplock (if the arg state = -1) or to voluntarily release the
 * oplock (if the arg state = OP_REVOKE).
 *
 * FD_SET(p[0], &readfds);
 * select(nfds, &readfds, 0, 0, 0)
 * Select is used to indicate when an oplock state transition has
 * occurred and on which backchannel to search for queued state
 * transition messages.  The SMB server will need to know that
 * a byte available for reading on p[0] indicates an F_OPLKSTAT
 * should be run on p[1].  The server will also need to know how to
 * map the os.os_dev and os.os_ino to the oplocked file descriptor.
 *
 * Deadlock may occur on the oplocked files of different SMB servers.
 * It is up to the SMB server applications to avoid deadlock.  It is
 * my understanding that Samba's application level oplocks already
 * do this.
 *
 * Theory of Operation
 *
 * The service works by hooking into a vnode's behavior chain so that a
 * custom VOP is called for a select set of vnode operations.  We are
 * particularly concerned with new opens (are they read or write?), but
 * NFS gives out file handles which cause references via vn_get() that
 * only get VOPed later so we also need setattr, getattr, read, write,
 * etc.  There are also a number of places in the kernel that lookup a
 * pathname and VOP on the returned vnode.
 *
 * Each of these hooked VOP routines calls check_oplock() to determine
 * if the operation will cause an oplock to be revoked or (though not
 * yet implemented) broken down.  If so, the state is changed (eg, to
 * OP_REVOKE from OP_EXCLUSIVE) and a zero byte is VOP_WRITEn to the
 * backchannel (determined during oplock registration to be the write
 * end of a pipe) specified in the oplock.  The thread that causes the
 * state change is put to sleep until either the SMB server responds to
 * the state change or a systunable timeout expires.
 *
 * The VOP_WRITE of the zero byte to the write end of the pipe backchannel
 * is intended to act as a signal mechanism to select or poll in the
 * server application.  The application reads the byte from the read
 * end of the pipe and then sends the write end of the pipe in the
 * F_OPLKSTAT fcntl command.  The returned oplock_stat structure
 * identifes the file by its dev/inode, and it is up to the server
 * application to translate that into the right file descriptor.
 *
 * When the server has gotten the SMB client to flush its cache, it
 * tells the kernel to wake up any threads sleeping on the oplock by
 * using with F_OPLKACK fcntl command.
 */
#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <ksys/vfile.h>
#include <ksys/fdt.h>
#include <sys/kthread.h>
#include <ksys/vproc.h>
#include <sys/fsid.h>
#include <sys/kmem.h>
#include <sys/cred.h>
#include <sys/poll.h>
#include <sys/sema.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pvnode.h>
#include <ksys/oplock.h>
#include <string.h>
#include <bstring.h>
#include <stddef.h>

#if DEBUG
#undef STATIC
#define	STATIC
#endif

STATIC kqueuehead_t op_list;	/* list of OP_REVOKE oplocks */

STATIC oplock_data_t *op_alloc(void);
STATIC void op_free(oplock_data_t *);

#define VN_TO_BHVU(vp) vn_bhv_lookup_unlocked(VN_BHV_HEAD(vp), &oplock_vnodeops)
#define VN_TO_BHVL(vp) vn_bhv_lookup(VN_BHV_HEAD(vp), &oplock_vnodeops)
#define op_initlist(void)	kqueue_init(&op_list)
#define op_insert(op)		kqueue_enter(&op_list, (kqueue_t *)op)
#define op_unlink(op)		{ \
					kqueue_remove((kqueue_t *)op); \
					kqueue_null((kqueue_t *)op); \
				}
#define op_first()		(oplock_data_t *)kqueue_first(&op_list)
#define op_next(op)		(oplock_data_t *)kqueue_next((kqueue_t *)op)

/* Only support kernel oplocks for XFS */
extern int xfs_fstype;
#define BADFS(vfsp)	((vfsp)->vfs_fstype != xfs_fstype)

#ifdef CELL_CAPABLE
#define OPLK_INIT(lock)		mutex_init(&(lock),MUTEX_DEFAULT,"oplock lock")
#define OPLK_DESTROY(lock)	mutex_destroy(&(lock))
#define OPLK_LOCK(lock, s)						\
{									\
	mutex_lock(&(lock), PZERO);					\
	(s) = 0;							\
}
#define OPLK_UNLOCK(lock, s)	mutex_unlock(&(lock))
#else	/* CELL_CAPABLE */
#define OPLK_INIT(lock)		spinlock_init(&(lock), "oplock lock")
#define OPLK_DESTROY(lock)	spinlock_destroy(&(lock))
#define OPLK_LOCK(lock, s)	(s) = mutex_spinlock(&(lock))
#define OPLK_UNLOCK(lock, s)	mutex_spinunlock(&(lock), s)
#endif	/* CELL_CAPABLE */


/* our special vnode operations jump table, filled in dynamically */
vnodeops_t oplock_vnodeops;
STATIC void oplock_init_vnodeops(void);

STATIC mutex_t	oplist_lock;	/* oplock list lock */
extern mutex_t	imonlsema;	/* interposer insertion lock */

#ifdef CELL_CAPABLE
/* CXFS required */
extern int cfs_oplock(vnode_t *, int, void *);
#define IS_CXFS(vp)		(cfs_oplock(vp, CXFS_CHECK, NULL))
#endif	/* CELL_CAPABLE */

/*----------------------------oplock data management-----------------------*/

STATIC zone_t *oplock_zone;

void
oplockinit(void)
{
	static int	inited = 0;

	if (inited)
		return;
	inited = 1;
	oplock_init_vnodeops();
	op_initlist();
	mutex_init(&oplist_lock, MUTEX_DEFAULT, "oplist");
	oplock_zone = kmem_zone_init(sizeof(oplock_data_t), "oplockdata");
}

STATIC oplock_data_t *
op_alloc(void)
{
	oplock_data_t *op;

	op = kmem_zone_zalloc(oplock_zone, KM_SLEEP);
	OPLK_INIT(op->o_lock);
	sv_init(&op->o_sv, SV_DEFAULT, "oplock wait");
	return op;
}

STATIC void
op_free(oplock_data_t *op)
{
	OPLK_DESTROY(op->o_lock);
	sv_destroy(&op->o_sv);
	kmem_zone_free(oplock_zone, op);
}

/*----------------------------oplock internals---------------------------*/

/*
 * Oplock states:
 *	none
 *	exclusive
 *	(intermediate) breakdown (not implemented)
 *	levelII (read-only, not implemented)
 *	(intermediate) revoking
 *	none
 */

#define I_OWN_OPLOCK(op)	(KT_CUR_ISUTHREAD() && \
					op->o_pid == current_pid())
#define BHV_TO_OPLOCK(bdp)	((oplock_data_t *)(BHV_PDATA(bdp)))

/*
 * Send a signal byte up the o_backchan pipe so the daemon select() can
 * detect state transition.
 */
STATIC void
signal_statechange(oplock_data_t *op)
{
	struct uio uio;
	struct iovec aiov;
	int zero = 0;
	int error;
	/* REFERENCED */
	int s;
	vnode_t *backchan;

	/*
	 * Avoid a race condition with oplock_close().
	 * Get the list lock so we can be sure we insert an oplock
	 * that has the right state on the list.
	 */
	mutex_lock(&oplist_lock, PINOD);
	OPLK_LOCK(op->o_lock, s);
	if (op->o_state != OP_REVOKE) {
		/*
		 * State changed, so give up early.
		 */
		OPLK_UNLOCK(op->o_lock, s);
		mutex_unlock(&oplist_lock);
		return;
	}
	op_insert(op);
	backchan = op->o_backchan;
	VN_HOLD(backchan);
	OPLK_UNLOCK(op->o_lock, s);
	mutex_unlock(&oplist_lock);

	aiov.iov_base = &zero;
	aiov.iov_len = uio.uio_resid = 1;
	uio.uio_iov = &aiov;
	uio.uio_fmode = FWRITE|FNONBLOCK;
	uio.uio_iovcnt = 1;
	uio.uio_pmp = NULL;
	uio.uio_pio = 0;
	uio.uio_readiolog = 0;
	uio.uio_writeiolog = 0;
	uio.uio_pbuf = 0;
	uio.uio_sigpipe = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_limit = RLIM_INFINITY;
	uio.uio_fp = NULL;	/* only used by xfs, not fifofs */
	uio.uio_offset = 0;

	VOP_WRITE(backchan, &uio, FNONBLOCK, get_current_cred(), NULL, error);

	VN_RELE(backchan);
	switch (error) {
	case 0:
		break;
	case EAGAIN:
		cmn_err(CE_WARN, "oplock 0x%x: backchannel full", op);
		break;
	case EPIPE:
		cmn_err(CE_WARN, "oplock 0x%x: no backchannel reader", op);
		break;
	default:
		cmn_err(CE_WARN, "oplock 0x%x: error %d on backchannel",
			op, error);
		break;
	}
}

/*
 * Timeout expiration routine.
 *
 * Check o_state==OP_REVOKE for race with responder/closer.
 */
STATIC void
oplockexpire(oplock_data_t *op)
{
	/* REFERENCED */
	int s;
	vnode_t *release_backchan = 0;

	OPLK_LOCK(op->o_lock, s);
	if (op->o_state == OP_REVOKE) {
#ifdef CELL_CAPABLE
		{
		int is_cxfs = 0;

		if (is_cxfs = IS_CXFS(BHV_TO_VNODE(&op->o_bhv)) && 
		    (is_cxfs == CXFS_CLIENT))
			/* 
			 * Notify the server of the state change.  We
			 * can borrow the F_OPLKACK cmd for this since it
			 * does exactly what we want in the release case.
			 */
			(void) cfs_oplock(BHV_TO_VNODE(&op->o_bhv), F_OPLKACK, 
				&is_cxfs);
		}
#endif	/* CELL_CAPABLE */
		release_backchan = op->o_backchan;
		op->o_backchan = 0;
		op->o_state = OP_NONE;
		op->o_flags = 0;
		VN_FLAGCLR(BHV_TO_VNODE(&op->o_bhv), VOPLOCK);
		sv_broadcast(&op->o_sv);
#ifdef CELL_CAPABLE
		op->o_sleepers = 0;
#endif	/* CELL_CAPABLE */

	} /* else responder/closer won the race */
	op->o_timeoutid = 0;
	OPLK_UNLOCK(op->o_lock, s);
	if (release_backchan)
		VN_RELE(release_backchan);
	cmn_err(CE_WARN, "oplock 0x%x: timed out", op);
}

/*
 * Schedule on oplock timeout when entering OP_REVOKE state.
 * Unschedule it and wake up sleepers when exiting OP_REVOKE state.
 */
STATIC void
sched_oplock_timeout(oplock_data_t *op)
{
	ASSERT(!op->o_timeoutid);
	op->o_timeoutid = timeout(oplockexpire, op, oplock_timeout * HZ);
}

STATIC void
unsched_oplock_timeout(oplock_data_t *op, int *sp)
{
	ASSERT(op->o_timeoutid);
	ASSERT_ALWAYS(op->o_state == OP_REVOKE);
	if (untimeout(op->o_timeoutid)) {
		op->o_timeoutid = 0;
		sv_broadcast(&op->o_sv);
#ifdef CELL_CAPABLE
		op->o_sleepers = 0;
#endif	/* CELL_CAPABLE */
	} else {
		/*
		 * We missed the timeout but are still in OP_REVOKE state.
		 * There MUST be a thread spinning in oplockexpire().  To
		 * avoid letting it race with oplock_reclaim(), wait here
		 * for it.
		 */
#ifdef CELL_CAPABLE
		op->o_sleepers++;
#endif  /* CELL_CAPABLE */
		sv_wait(&op->o_sv, PZERO, &op->o_lock, *sp);
		OPLK_LOCK(op->o_lock, *sp);
	}
}

/*
 * Put an external reference thread to sleep.
 * This is currently only valid in the OP_REVOKE state.
 */
STATIC int
oplocksleep(oplock_data_t *op, int s)
{
	int interrupted = 0;

#ifdef CELL_CAPABLE
	op->o_sleepers++;
#endif	/* CELL_CAPABLE */
	interrupted = sv_wait_sig(&op->o_sv, PZERO + 1, &op->o_lock, s);
	if (interrupted)
		return(EINTR);
	return 0;
}

/*
 * Check if this VOP reference causes an oplock to be revoked or broken down.
 * For now, assume writeflag==1 to only support EXCLUSIVE oplocks.
 */
STATIC int
check_oplock(bhv_desc_t *bdp, int writeflag)
{
	oplock_data_t *op = BHV_TO_OPLOCK(bdp);
	int s;

	OPLK_LOCK(op->o_lock, s);

	if (!writeflag)
		writeflag = 1;
	switch(op->o_state) {
	case OP_EXCLUSIVE:
#ifdef CELL_CAPABLE
		if ((op->o_flags & (OPLK_CXFS_SERVER|OPLK_CXFS_CLIENT_HELD)) ==
			(OPLK_CXFS_SERVER | OPLK_CXFS_CLIENT_HELD)) {
#pragma mips_frequency_hint NEVER
			/* 
			 * CXFS oplock path.
			 * 
			 * We are the cxfs server and a client holds the 
			 * oplock.  We must revoke this oplock by making
			 * an rpc to the client.  We will block in this
			 * rpc until the client has finished the revoke.
			 * We don't set our own timeout here but instead
			 * rely on the client setting a timeout. 
			 */
			op->o_state = OP_REVOKE;
			OPLK_UNLOCK(op->o_lock, s);
			(void) cfs_oplock(BHV_TO_VNODE(bdp), 
					  CXFS_REVOKE, 
					  NULL);
			return(0);
		} else
#endif	/* CELL_CAPABLE */
			/* standard oplock path */
			if (I_OWN_OPLOCK(op))
				break;

		/*
		 * A non-owning thread is about to operate on the oplocked
		 * file.  Put the thread to sleep while revoking the oplock.
		 * Timeout and state transition may occur any time after the
		 * sched_oplock_timeout().
		 */
		op->o_state = OP_REVOKE;
		sched_oplock_timeout(op);
		OPLK_UNLOCK(op->o_lock, s);
		signal_statechange(op);
		OPLK_LOCK(op->o_lock, s);
		if (op->o_state == OP_REVOKE) {
			return (oplocksleep(op, s)); /* and unlock */
		}
		break;

	case OP_REVOKE:
		if (I_OWN_OPLOCK(op))
			break;

		/*
		 * Introduce a secondary sleeper on a file whose oplock
		 * is already being revoked.
		 */
		return (oplocksleep(op, s)); /* and unlock */

	case OP_NONE:
		break;

	case OP_BREAKDOWN:	/* not yet implemented */
	case OP_LEVELII:	/* not yet implemented */
	default:
		cmn_err(CE_PANIC, "oplock 0x%x held by %d: bad state %d",
			op, op->o_pid, op->o_state);
		break;
	}

	OPLK_UNLOCK(op->o_lock, s);
	return 0;
}

#ifdef CELL_CAPABLE
void
oplock_cxfs_check(bhv_desc_t *bdp)
{
	check_oplock(bdp, NULL);
	return;
}

int
oplock_cxfs_req( 
	vnode_t *vp,  
	int cmd, 
	int client_called, 
	int *req_results)
{
	bhv_desc_t *bdp;
	oplock_data_t *op;
	int s;

	*req_results = NULL;
	
	switch (cmd) {
	case F_OPLKREG: /* register an oplock */
		/* 
		 * If we were called by a CXFS client we need to interpose
		 * the oplock behavior on the server on behalf of the client.
		 * Otherwise if we were called by the CXFS server that would
		 * mean that the Samba server is running on the CXFS server
		 * itself.  We still need a oplock behavior in this case as
		 * well.
		 */
		if (client_called) {
			BHV_TRYPROMOTE(VN_BHV_HEAD(vp));
			if (!BHV_AM_WRITE_OWNER(VN_BHV_HEAD(vp))) {
				*req_results = NULL;
				return (EAGAIN);
			}
		} else {
			VN_BHV_WRITE_LOCK(VN_BHV_HEAD(vp));
		}
		bdp = VN_TO_BHVL(vp);
		if (bdp) {
			/*
			 * Oplock behavior is already there.
			 */
			op = BHV_TO_OPLOCK(bdp);
			/*
			 * XXX We should probably do some sanity checks here.
			 * XXX If this vnode has a oplock behavior, but it was
			 * XXX interposed before this file was exported.  
			 * XXX That would indicate that we have samba servers
			 * XXX running on two machines trying to export the
			 * XXX same file.  We don't support that yet!
			 *
			 */
		} else {
			/* 
			 * We should not be interposing any new oplock bhv
			 * if we were not called by a client:  oplock_fcntl()
			 * should have already done this for us.
			 */
			ASSERT(client_called);
			op = op_alloc();
			op->o_state = OP_NONE;
			bdp = &op->o_bhv;
			/* 
			 * We don't go for the imonlsema here because we
			 * know that were are dealing with cxfs oplocks.
			 * Therefore we know that behavior locking is turned
			 * on!
			 */
			bhv_desc_init(bdp, op, vp, &oplock_vnodeops);
			vn_bhv_insert(VN_BHV_HEAD(vp), bdp);
		}
		if (client_called) {
			BHV_WRITE_TO_READ(VN_BHV_HEAD(vp));
		} else {
			VN_BHV_WRITE_UNLOCK(VN_BHV_HEAD(vp));
		}

		/*
		 * We now have an oplock on the vnode behavior list.  Any
		 * thread that VOPs will go through our oplock_* hooks.
		 * Now make the official v_count check and set state.
		 * Make sure any unconsumed state changes are off the list.
		 */
		if (client_called) {
			mutex_lock(&oplist_lock, PINOD);
			OPLK_LOCK(op->o_lock, s);
		}
		if (vp->v_count == 1 && op->o_state == OP_NONE) {
			/* grant oplock */
			ASSERT(op->o_backchan == 0);
			/* oplock request came from the client */
			op->o_flags = OPLK_CXFS_SERVER;
			*req_results = OP_EXCLUSIVE;
			if (client_called) {
				VN_FLAGSET(vp, VOPLOCK);
				op->o_state = OP_EXCLUSIVE;
				op->o_pid = 0;
				op->o_backchan = 0;
				op->o_dev = 0;
				op->o_ino = 0;
				op->o_sleepers = 0;
				op->o_opencnt = 1;
				op->o_flags |= OPLK_CXFS_CLIENT_HELD;
				OPLK_UNLOCK(op->o_lock, s);
				mutex_unlock(&oplist_lock);
			}
		} else {
			/* reject oplock request */
			if (client_called) {
				OPLK_UNLOCK(op->o_lock, s);
				mutex_unlock(&oplist_lock);
			}
			*req_results = NULL;
			return (EAGAIN);
		}
		break;

	case CXFS_REVOKE:
		if (!client_called) {
			VN_BHV_READ_LOCK(VN_BHV_HEAD(vp));
		}
		bdp = vn_bhv_lookup(VN_BHV_HEAD(vp), &oplock_vnodeops);
		if (!client_called) {
			VN_BHV_READ_UNLOCK(VN_BHV_HEAD(vp));
		}
		if (bdp) {
			op = BHV_TO_OPLOCK(bdp);
			OPLK_LOCK(op->o_lock, s);
			
			switch(op->o_state) {
			case OP_EXCLUSIVE:
				op->o_state= OP_REVOKE;
				sched_oplock_timeout(op);
				OPLK_UNLOCK(op->o_lock, s);
				signal_statechange(op);
				OPLK_LOCK(op->o_lock, s);
				if (op->o_state == OP_REVOKE) {
				    return (oplocksleep(op,s)); /* and unlock */
				}
				break;

			case OP_REVOKE:
				/*
				 * Introduce a secondary sleeper on a file 
				 * whose oplock is already being revoked.
				 */
				return (oplocksleep(op,s)); /* and unlock */

			case OP_NONE:
				/* 
				 * We might have received the revoke before
				 * we finished the grant of the oplock.  
				 */
				OPLK_UNLOCK(op->o_lock, s);
				return (EAGAIN);

			case OP_BREAKDOWN:      /* not yet implemented */
			case OP_LEVELII:        /* not yet implemented */
			default:
                		break;
        		}
	
			OPLK_UNLOCK(op->o_lock, s);
			return (0);

			
		} else {
#pragma mips_frequency_hint NEVER
			/* XXX possible to not have oplock bhv?? what then?? */
			ASSERT(0);
		}
#pragma mips_frequency_hint NEVER
		break;

	case CXFS_CLIENT_RECOVERY:
		/* 
		 * Called during client recovery.
		 *
		 */
		bdp = vn_bhv_lookup(VN_BHV_HEAD(vp), &oplock_vnodeops);
		if (bdp) {
			/* 
			 * Tear down oplock of dead client.
			 */
			op = BHV_TO_OPLOCK(bdp);
			OPLK_LOCK(op->o_lock, s);
			op->o_state = OP_NONE;
			op->o_flags = 0;
			op->o_opencnt = 0;
			if (op->o_sleepers) {
				sv_broadcast(&op->o_sv);
				op->o_sleepers = 0;
			}
			OPLK_UNLOCK(op->o_lock, s);
		}
		break;

	case F_OPLKACK:
		/* 
		 * We will always be the cxfs server called by a client rpc. 
		 * We therefore should already have the bhv read locked.
		 */
		ASSERT(client_called);
		ASSERT(VN_BHV_IS_READ_LOCKED(VN_BHV_HEAD(vp)));
		ASSERT(VN_BHV_LOCK_MINE(VN_BHV_HEAD(vp)));

		bdp = vn_bhv_lookup(VN_BHV_HEAD(vp), &oplock_vnodeops);
		if (bdp) {
			op = BHV_TO_OPLOCK(bdp);
			OPLK_LOCK(op->o_lock, s);
			op->o_state = OP_NONE;
			op->o_flags = 0;
			VN_FLAGCLR(BHV_TO_VNODE(&op->o_bhv), VOPLOCK);
			op->o_opencnt = 0;
			if (op->o_sleepers) {
				sv_broadcast(&op->o_sv);
				op->o_sleepers = 0;
			}
			OPLK_UNLOCK(op->o_lock, s);
		} else {
#pragma mips_frequency_hint NEVER
			/* XXXX possible to not have oplock bhv?? what then?? */
			ASSERT(0);
		}
		break;

	default:
		cmn_err(CE_PANIC, "cxfs_oplock_req: bad cmd %d", cmd);
		break;
	}
	return 0;
}
#endif	/* CELL_CAPABLE */

/*----------------------------oplock fcntl---------------------------*/

int
oplock_fcntl(vfile_t *fp, int cmd, sysarg_t arg, rval_t *rvp)
{
	struct vattr va;
	struct bhv_desc *bdp;
	vfile_t *bfp;
	vnode_t *bvp;
	vnode_t *vp;
	int error;
	int s;
	oplock_data_t *op;
	cred_t *cr = get_current_cred();
	oplock_stat_t os;
	vnode_t *release_backchan = 0;
	int found;
	int oplock_granted = 0;
	int is_cxfs = 0;
	int is_cxfs_server = 0; 


	if (!oplocks_enabled)
		return EINVAL;

	if (!cap_able_cred(cr, CAP_NETWORK_MGT))
		return EPERM;

	switch (cmd) {
	case F_OPLKREG: /* register an oplock */
		/* fcntl(fd, F_OPLKREG, backchan[1]) */
		if (!VF_IS_VNODE(fp))
			return EBADF;
		if (error = getf((int)arg, &bfp))
			return error;
		if (!VF_IS_VNODE(bfp))
			return EBADF;
		bvp = VF_TO_VNODE(bfp);
		if (bvp->v_type != VFIFO)
			return EBADF;
		if (bfp->vf_flag & FWRITE == 0)
			return EBADF;

		vp = VF_TO_VNODE(fp);

#ifdef CELL_CAPABLE
		/*
		 * Figure out if this vnode is an exported cxfs vnode.
		 */
		if (is_cxfs = IS_CXFS(vp)) {
			if (is_cxfs == CXFS_SERVER) {
				is_cxfs_server = CXFS_SERVER;
			}
		} else
#endif	/* CELL_CAPABLE */
		if (BADFS(vp->v_vfsp))
			return EINVAL;
		if (vp->v_type != VREG)
			return EBADF;
		/* informal check to avoid overhead below */
		if (!is_cxfs_server  && vp->v_count > 1)
			return EAGAIN;
		va.va_mask = AT_FSID | AT_NODEID;
		VOP_GETATTR(vp, &va, 0, get_current_cred(), error);
		if (error)
			return EIO;

		VN_BHV_WRITE_LOCK(VN_BHV_HEAD(vp));
		bdp = VN_TO_BHVL(vp);
		if (bdp) {
			/*
			 * Oplock behavior is already there.
			 */
			op = BHV_TO_OPLOCK(bdp);
		} else {
			op = op_alloc();
			op->o_state = OP_NONE;
			bdp = &op->o_bhv;
			bhv_desc_init(bdp, op, vp, &oplock_vnodeops);
			mutex_lock(&imonlsema, PINOD);
			error = vn_bhv_insert(VN_BHV_HEAD(vp), bdp);
			if (error) {
				/* rare... another thread inserted a bhv */
				op_free(op);
				bdp = VN_TO_BHVL(vp);
				op = BHV_TO_OPLOCK(bdp);
			}
			mutex_unlock(&imonlsema);
		}
		VN_BHV_WRITE_UNLOCK(VN_BHV_HEAD(vp));

		/*
		 * We now have an oplock on the vnode behavior list.  Any
		 * thread that VOPs will go through our oplock_* hooks.
		 * Now make the official v_count check and set state.
		 * Make sure any unconsumed state changes are off the list.
		 */
		mutex_lock(&oplist_lock, PINOD);
		OPLK_LOCK(op->o_lock, s);

		if (!is_cxfs) {
			/* standard oplock path */
			if (vp->v_count == 1 && op->o_state == OP_NONE) {
				/* grant oplock */

				oplock_granted = 1;

			} else {
				oplock_granted = 0;
			}
		}
#ifdef CELL_CAPABLE
		else {
			/*
			 * CXFS oplock path.
			 * We must do a bunch of work in cxfs to determine
			 * if we can grant the oplock.
			 */
			if (((!is_cxfs_server) && (vp->v_count > 1)) || 
			    (op->o_state != OP_NONE)) {
				/* reject oplock request */
				oplock_granted = 0;
			} else {
				/* 
				 * We must ask the server if we can grant
				 * the oplock request.
				 */
				error = cfs_oplock(vp, cmd, &is_cxfs_server);
				if (error) {
					/* reject oplock request */
					oplock_granted = 0;
				} else {
					/* grant oplock */
					oplock_granted = 1;
				}
			}
		}
#endif	/* CELL_CAPABLE */

		if (oplock_granted) {
			ASSERT(op->o_backchan == 0);
			VN_FLAGSET(vp, VOPLOCK);
			rvp->r_val1 = op->o_state = OP_EXCLUSIVE;
			if (!is_cxfs_server) 
				op->o_flags = 0;
			op->o_pid = current_pid();
			op->o_opencnt = 1;
			op->o_backchan = bvp;
			VN_HOLD(op->o_backchan);
			op->o_dev = va.va_fsid;
			op->o_ino = va.va_nodeid;
#ifdef CELL_CAPABLE
			op->o_sleepers = 0;
#endif	/* CELL_CAPABLE */
			if (op->o_next)
				op_unlink(op);
			OPLK_UNLOCK(op->o_lock, s);
			mutex_unlock(&oplist_lock);
		} else {
			/* reject oplock request */
			OPLK_UNLOCK(op->o_lock, s);
			mutex_unlock(&oplist_lock);
			return EAGAIN;
		}
		break;

	case F_OPLKACK: /* Acknowledge an oplock state change. */
		/* fcntl(fd, F_OPLKACK, state) */
		/*
		 * While typically used to acknowledge state transitions
		 * brought about by external references, this command is
		 * also used to voluntarily release an oplock or check
		 * the current state of a file's oplock.
		 *
		 * State:
		 * Support OP_REVOKE now to voluntarily release an oplock
		 * or acknowledge oplock revocation.  Support -1 to return
		 * the current state value.  Support OP_BREAKDOWN later.
		 */
		if (!VF_IS_VNODE(fp))
			return EBADF;
		vp = VF_TO_VNODE(fp);
		if (!(bdp = VN_TO_BHVU(vp))) {
			rvp->r_val1 = OP_NONE;
			return 0;
		}
		if (arg != OP_REVOKE && arg != -1)
			return EINVAL;

		op = BHV_TO_OPLOCK(bdp);
		OPLK_LOCK(op->o_lock, s);
		if (arg == -1) {
			rvp->r_val1 = op->o_state;
		} else {


			release_backchan = op->o_backchan;
			op->o_backchan = 0;
			op->o_flags = 0;
			if (op->o_state == OP_REVOKE) {
				unsched_oplock_timeout(op, &s);
			}
			rvp->r_val1 = op->o_state = OP_NONE;
			VN_FLAGCLR(BHV_TO_VNODE(&op->o_bhv), VOPLOCK);
#ifdef CELL_CAPABLE
			/*
		 	 * Figure out if this vnode is an cxfs vnode.
		  	 */
			if (is_cxfs=IS_CXFS(vp)) {
				/* Notify server of state change */
				(void) cfs_oplock(vp, cmd, &is_cxfs);
			}
#endif	/* CELL_CAPABLE */
		}
		OPLK_UNLOCK(op->o_lock, s);
		if (release_backchan)
			VN_RELE(release_backchan);
		break;

	case F_OPLKSTAT: /* get the next state change on this backchannel */
		/* fcntl(backchan[1], F_OPLKSTAT, &os) */
		vp = VF_TO_VNODE(fp);
		mutex_lock(&oplist_lock, PINOD);
		/*
		 * TODO: hash this lookup.
		 */
		found = 0;
		for (op = op_first();
		    !found && op != (oplock_data_t *)&op_list;
		    op = op_next(op)) {
			OPLK_LOCK(op->o_lock, s);
			if (op->o_backchan == vp) {
				/*
				 * TODO:
				 * Figure out a good way to move this decision
				 * to after the successful copyout.
				 * Technically, it should not be unlinked if
				 * copyout fails, but a copyout failure
				 * usually means disaster for the app anyway.
				 */
				op_unlink(op);
				os.os_ino = op->o_ino;
				os.os_dev = op->o_dev;
				os.os_state = op->o_state;
				found++;
			}
			OPLK_UNLOCK(op->o_lock, s);
		}
		mutex_unlock(&oplist_lock);
		if (found) {
			if (copyout(&os, (void *)arg, sizeof(os))) {
				error = EFAULT;
			} else {
				error = 0;
			}
			return error;
		}
		return EAGAIN;

	default:
		cmn_err(CE_PANIC, "oplock_fcntl: bad cmd %d", cmd);
		break;
	}
	return 0;
}

/*----------------------------vnode operations---------------------------*/

STATIC int
oplock_open(bhv_desc_t *bdp, vnode_t **vpp, mode_t flag, struct cred *cred)
{
	int		error;
	bhv_desc_t	*nbdp;
	int		writeflag;
	oplock_data_t	*op;
	/* REFERENCED */
	int s;

	writeflag = flag & (FWRITE|FCREAT|FTRUNC);
	check_oplock(bdp, writeflag);

	op = BHV_TO_OPLOCK(bdp);
	OPLK_LOCK(op->o_lock, s);
	if (op->o_state != OP_NONE && I_OWN_OPLOCK(op))
		op->o_opencnt++;
	OPLK_UNLOCK(op->o_lock, s);

	PV_NEXT(bdp, nbdp, vop_open);
	PVOP_OPEN(nbdp, vpp, flag, cred, error);
	return error;
}

STATIC int
oplock_close(bhv_desc_t *bdp, int flag, lastclose_t lastclose, struct cred *cr)
{
	oplock_data_t *op;
	int error;
	bhv_desc_t *nbdp;
	int s;
	vnode_t *release_backchan = 0;

	/*
	 * If we own the oplock and this is our last lastclose on the file,
	 * release the oplock and tell any sleepers to wake up.
	 */
	if (lastclose == L_TRUE) {
		op = BHV_TO_OPLOCK(bdp);
		mutex_lock(&oplist_lock, PINOD);
		OPLK_LOCK(op->o_lock, s);
		if (op->o_state != OP_NONE && I_OWN_OPLOCK(op)) {
			ASSERT(op->o_opencnt > 0);
			if (--op->o_opencnt == 0) {
				release_backchan = op->o_backchan;
				op->o_backchan = 0;
				op->o_flags = 0;
				if (op->o_state == OP_REVOKE)
					unsched_oplock_timeout(op, &s);
				op->o_state = OP_NONE;
				VN_FLAGCLR(BHV_TO_VNODE(&op->o_bhv), VOPLOCK);
				if (op->o_next)
					op_unlink(op);
#ifdef CELL_CAPABLE
				{
				int is_cxfs;
				vnode_t *vp = BHV_TO_VNODE(bdp);

				if ((is_cxfs=IS_CXFS(vp)) &&
					(is_cxfs == CXFS_CLIENT))
					/* 
					 * Notify the server of the state
					 * change.  We can borrow the F_OPLKACK
					 * cmd for this since it does exactly
					 * what we want in the release case.
					 */
					(void) cfs_oplock(vp, F_OPLKACK, 
							 &is_cxfs);
				}
#endif	/* CELL_CAPABLE */
			}
		}
		OPLK_UNLOCK(op->o_lock, s);
		mutex_unlock(&oplist_lock);
		if (release_backchan)
			VN_RELE(release_backchan);
	}

	PV_NEXT(bdp, nbdp, vop_close);

	PVOP_CLOSE(nbdp, flag, lastclose, cr, error);
	return error;
}

STATIC int
oplock_read(
	bhv_desc_t 	*bdp,
	struct uio 	*uiop,
	int 		ioflag,
	struct cred 	*cr,
	struct flid 	*fl)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, 0);

	PV_NEXT(bdp, nbdp, vop_read);

	PVOP_READ(nbdp, uiop, ioflag, cr, fl, error);
	return error;

}

STATIC int
oplock_write(
	bhv_desc_t *bdp,
	struct uio *uiop,
	int ioflag,
	struct cred *cr,
	struct flid *fl)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, FWRITE);

	PV_NEXT(bdp, nbdp, vop_write);

	PVOP_WRITE(nbdp, uiop, ioflag, cr, fl, error);
	return error;
}

STATIC int
oplock_getattr(
	bhv_desc_t 	*bdp,
	struct vattr 	*vap,
	int 		flags,
	struct cred 	*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, 0);

	PV_NEXT(bdp, nbdp, vop_getattr);

	PVOP_GETATTR(nbdp, vap, flags, cr, error);
	return error;
}

STATIC int
oplock_setattr(
	bhv_desc_t 	*bdp,
	struct vattr 	*vap,
	int 		flags,
	struct cred 	*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, FWRITE);

	PV_NEXT(bdp, nbdp, vop_setattr);

	PVOP_SETATTR(nbdp, vap, flags, cr, error);
	return error;
}

STATIC int
oplock_access(
	bhv_desc_t 	*bdp,
	int 		mode,
	struct cred 	*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, mode&FWRITE);

	PV_NEXT(bdp, nbdp, vop_access);

	PVOP_ACCESS(nbdp, mode, cr, error);
	return error;
}

/*
 * VOP_CREATE() is done on the directory, not the directory entry.
 * Work around this shortcoming in XFS with a special hook.
 */
#define OPLOCK_CREATE_WAR
#ifdef OPLOCK_CREATE_WAR

void
oplock_fs_create(vnode_t *vp)
{
	bhv_desc_t 	*bdp;

	if ((bdp = VN_TO_BHVU(vp)) == NULL)
#pragma mips_frequency_hint NEVER
		return;

	check_oplock(bdp, FWRITE);
}

#else /* OPLOCK_CREATE_WAR */

STATIC int
oplock_create(
	bhv_desc_t 	*bdp,
	char 		*name,
	struct vattr 	*vap,
	int 		flags,
	int 		mode,
	struct vnode 	**vpp,
	struct cred 	*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, FWRITE);

	PV_NEXT(bdp, nbdp, vop_create);

	PVOP_CREATE(nbdp, name, vap, flags, mode, vpp, cr, error);
	return error;
}

STATIC int
oplock_remove(
	bhv_desc_t 	*bdp,
	char 		*nm,
	struct cred 	*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, FWRITE);

	PV_NEXT(bdp, nbdp, vop_remove);

	PVOP_REMOVE(nbdp, nm, cr, error);
	return error;
}

#endif /* OPLOCK_CREATE_WAR */

STATIC int
oplock_frlock(
	bhv_desc_t 	*bdp,
	int 		cmd,
	struct flock 	*lfp,
	int 		flag,
	off_t 		offset,
	vrwlock_t	vrwlock,
	cred_t 		*cr)
{
	int		error;
	bhv_desc_t	*nbdp;

	check_oplock(bdp, FWRITE);

	PV_NEXT(bdp, nbdp, vop_frlock);

	PVOP_FRLOCK(nbdp, cmd, lfp, flag, offset, vrwlock, cr, error);
	return error;
}

STATIC int
oplock_inactive(bhv_desc_t *bdp, cred_t *cr)
{
	int		error;
	oplock_data_t	*op;
	bhv_desc_t	*nbdp;
	vnode_t		*vp;

	/* vp is protected by VINACT, so oplock state is stable */
	op = BHV_TO_OPLOCK(bdp);
	vp = BHV_TO_VNODE(bdp);
	ASSERT(vp->v_flag & VINACT);
	ASSERT(op->o_flags == 0);
	ASSERT(op->o_state == OP_NONE);
	ASSERT(op->o_backchan == 0);

	mutex_lock(&oplist_lock, PINOD);
	if (op->o_next)
		op_unlink(op);
	mutex_unlock(&oplist_lock);

	PV_NEXT(bdp, nbdp, vop_inactive);
	PVOP_INACTIVE(nbdp, cr, error);
	/*
	 * If the underlying behaviors all go away now,
	 * then we have to go as well.
	 */
	if (error == VN_INACTIVE_NOCACHE) {
		vn_bhv_remove(VN_BHV_HEAD(vp), &op->o_bhv);
		op_free(op);
	}
	return error;
}

STATIC int
oplock_reclaim(bhv_desc_t *bdp, int flags)
{
	int		error = 0;
	oplock_data_t	*op;
	bhv_desc_t	*nbdp;
	vnode_t		*vp;

	/* vp is protected by VRECLM, so oplock state is stable */
	op = BHV_TO_OPLOCK(bdp);
	vp = BHV_TO_VNODE(bdp);
	ASSERT(vp->v_flag & VRECLM);

	if (bdp->bd_next) {
		PV_NEXT(bdp, nbdp, vop_reclaim);
		PVOP_RECLAIM(nbdp, flags, error);
		/*
		 * If the behavior below us refuses to be reclaimed,
		 * then stick around until it decides to go away.
		 */
		if (error) {
			return error;
		}
	} else {
		nbdp = NULL;
	}

	ASSERT(op->o_flags == 0);
	ASSERT(op->o_state == OP_NONE);
	ASSERT(op->o_backchan == 0);
	ASSERT(op->o_next == 0);

	vn_bhv_remove(VN_BHV_HEAD(vp), &op->o_bhv);
	op_free(op);

	return error;
}


STATIC void
oplock_init_vnodeops(void)
{
	oplock_vnodeops = *vn_passthrup;
	oplock_vnodeops.vn_position.bi_position = VNODE_POSITION_OPLOCK;
	oplock_vnodeops.vop_open = oplock_open;
	oplock_vnodeops.vop_close = oplock_close;
	oplock_vnodeops.vop_read = oplock_read;
	oplock_vnodeops.vop_write = oplock_write;
	oplock_vnodeops.vop_getattr = oplock_getattr;
	oplock_vnodeops.vop_setattr = oplock_setattr;
	oplock_vnodeops.vop_access = oplock_access;
#ifndef OPLOCK_CREATE_WAR
	oplock_vnodeops.vop_create = oplock_create;
	oplock_vnodeops.vop_remove = oplock_remove;
#endif
	oplock_vnodeops.vop_inactive = oplock_inactive;
	oplock_vnodeops.vop_frlock = oplock_frlock;
	oplock_vnodeops.vop_reclaim = oplock_reclaim;
}
