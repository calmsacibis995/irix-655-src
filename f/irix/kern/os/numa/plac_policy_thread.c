/*
 * os/numa/plac_policy_thread.c
 *
 *
 * Copyright 1995, 1996 Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/pfdat.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/proc.h>
#include <sys/pda.h>
#include <sys/sysmacros.h>
#include <sys/cpumask.h>
#include <sys/pmo.h>
#include <sys/numa.h>
#include <sys/nodemask.h>
#include "pmo_base.h"
#include "pmo_error.h"
#include "pmo_list.h"
#include "pmo_ns.h"
#include "mld.h"
#include "mldset.h"
#include "pm.h"
#include "aspm.h"
#include "memsched.h"
#include "numa_init.h"
#include "pm_policy_common.h"

/*
 * Private PlacementThread definitions
 */
typedef struct plac_policy_thread_data {
        ushort aff_turn;
} plac_policy_thread_data_t;

#define  THREADS_PER_MLD 2

static int plac_policy_thread_create(pm_t* pm, void* args, pmo_ns_t* pmo_ns);
static char* plac_policy_thread_name(void);
static void plac_policy_thread_destroy(pm_t* pm);
static pfd_t* plac_policy_thread_pagealloc(struct pm* pm,
                                           uint ckey,
                                           int flags,
                                           size_t* ppsz,
                                           caddr_t vaddr);
static void plac_policy_thread_afflink(struct pm* pm,
                                       aff_caller_t aff_caller,
                                       kthread_t* ckt,
                                       kthread_t* pkt);
static int plac_policy_thread_plac_srvc(struct pm* pm,
					pm_srvc_t srvc,
					void* args, ...);


/*
 * The dynamic memory allocation zone for plac_policy_default child data
 */
static zone_t* plac_policy_thread_data_zone = 0;


/*
 * The PlacementDefault initialization procedure.
 * To be called very early during the system
 * initialization procedure.
 */
int
plac_policy_thread_init()
{
        ASSERT(plac_policy_thread_data_zone == 0);
        plac_policy_thread_data_zone =
                kmem_zone_init(sizeof(plac_policy_thread_data_t),
                               "plac_policy_thread_data");
        ASSERT(plac_policy_thread_data_zone);
        pmfactory_export(plac_policy_thread_create, plac_policy_thread_name());

        return (0);
}

/*
 * The constructor for PlacementDefault takes one argument:
 * -- number of threads that will be using this addr space
 */

/*ARGSUSED*/
static int
plac_policy_thread_create(pm_t* pm, void* args, pmo_ns_t* pmo_ns)
{
	mldset_t* mldset;
	pmo_handle_t mldset_handle = (pmo_handle_t)(__uint64_t)args;
	pmolist_t* mldlist;
	mld_t*	mld;

        ASSERT(plac_policy_thread_data_zone);
        ASSERT(pm);
        ASSERT(pmo_ns);

	if (pm->pmo) {
		/*
		 * CPR restart case
		 */
		if (pmo_gettype(pm->pmo) != __PMO_MLDSET)
			return (PMOERR_EINVAL);

		mldset = pm->pmo;
		pmo_incref(mldset, pmo_ref_find);

	} else {

		mldset = (mldset_t*)pmo_ns_find(pmo_ns, mldset_handle, __PMO_MLDSET);
		if (mldset == NULL)
			return (PMOERR_ENOENT);
	}
	mldlist = mldset_getmldlist(mldset);

	if (mldlist == NULL) {
		pmo_decref(mldset, pmo_ref_find);
		return PMOERR_EINVAL;
	}

	mld = (mld_t*)pmolist_getelem(mldlist, 0);
	ASSERT(mld);

	/*
	 * Check if an mld in the set is placed.
 	 */

	if (!mld_isplaced(mld)) {
		pmo_decref(mldset, pmo_ref_find);
		return PMOERR_EINVAL;
	}

	pm->pmo = (void*)mldset;

        /*
         * Incr ref for reference from pm, release ref acquired
         * in find. Done this way to keep debugging trace sane.
         */
        pmo_incref(mldset, pm);
        pmo_decref(mldset, pmo_ref_find);        
        
        /*
         * Placement Policy constructors have to set
         * the following methods:
         * -- pagealloc
         * -- afflink
         * -- plac_srvc
         * The placement policy private object data can be attached
         * to plac_data.
         */

        pm->pagealloc = plac_policy_thread_pagealloc;
        pm->afflink = plac_policy_thread_afflink;
        pm->plac_srvc = plac_policy_thread_plac_srvc;

        /*
         * Allocate and initialize memory for the private PlacPol data
         */
        pm->plac_data = kmem_zone_alloc(plac_policy_thread_data_zone, KM_SLEEP);
        ASSERT(pm->plac_data);
        ((plac_policy_thread_data_t*)(pm->plac_data))->aff_turn = 0;

        return (0);
}

/*ARGSUSED*/
static char*
plac_policy_thread_name(void)
{
        return ("PlacementThreadLocal");
}

/*ARGSUSED*/
static void
plac_policy_thread_destroy(pm_t* pm)
{
        ASSERT(pm);

	if (pm->pmo) {
		pmo_decref(pm->pmo, pm);
	}

        /*
         * Free the plac_data memory
         */
	if (pm->plac_data) {
		kmem_zone_free(plac_policy_thread_data_zone, pm->plac_data);
	}
}

/*ARGSUSED*/
static pfd_t*
plac_policy_thread_pagealloc(struct pm* pm,
                             uint ckey,
                             int flags,
                             size_t* ppsz,
                             caddr_t vaddr)
{
        cnodeid_t node;
        pfd_t* pfd;
        mld_t* mld;
        
        ASSERT(pm);
        ASSERT(pm->pmo);
        ASSERT(ppsz);


        /*
         * If the number of threads is THREADS_PER_MLD or less,
         * we only have an mld.
         * Otherwise we have to deal with an mldset.
         */

        mld = UT_TO_KT(private.p_curuthread)->k_mldlink;
        
	ASSERT(mld);

        node = mld_getnodeid(mld);
        ASSERT(node >= 0);

	if (pm->policy_flags & POLICY_CACHE_COLOR_FIRST)
		flags |= VM_PACOLOR;
	
	if (*ppsz == PM_PAGESIZE) {
        	pfd = pagealloc_size_node(node, ckey, flags, pm->page_size);
		*ppsz = pm->page_size;
	} else {
		ASSERT(valid_page_size(*ppsz));
        	pfd = pagealloc_size_node(node, ckey, flags, *ppsz);
	}


        if (pfd == NULL) {
                pfd = (*pm->fallback)(pm, ckey, flags, mld, ppsz);
		if ((flags & VM_PACOLOR) && (pfd == NULL)) {
                        flags &= ~VM_PACOLOR;
                        pfd = (*pm->fallback)(pm, ckey, flags, mld, ppsz);
                }
        }


        return (pfd);
}

/*
 * This method sets the affinity fields in the proc structure for the
 * child process `cp'. We can also dynamically change the placement policy
 * defined by this pm according to the calls to this method, which is called
 * every time a process forks (sprocs) or execs.
 */
static void
plac_policy_thread_afflink(struct pm* pm,
                           aff_caller_t aff_caller,
                           kthread_t* ckt,
                           kthread_t* pkt)
{
        mld_t* mld;
        mldset_t* mldset;
        int aff_turn;

        ASSERT(pm);
        ASSERT(pm->pmo);
        ASSERT(ckt);

        pm_mrupdate(pm);
        ASSERT(pmo_gettype(pm->pmo) == __PMO_MLDSET);
        mldset = (mldset_t*)pm->pmo;
        ASSERT(mldset_isplaced(mldset));
        
        switch (aff_caller) {
        case AFFLINK_NEWPROC_SHARED:     /* sproc with shared address space */
        case AFFLINK_NEWPROC_PRIVATE:    /* fork, or privated sproc */
        {
                /*
                 * We link the new process to the next mld in the mldset
                 * indexed by aff_turn.
                 */
                ASSERT(pkt);
                aff_turn = ((plac_policy_thread_data_t*)(pm->plac_data))->aff_turn;
                ((plac_policy_thread_data_t*)(pm->plac_data))->aff_turn = (aff_turn + 1) % mldset_getclen(mldset);
                mld = mldset_getmld(mldset, aff_turn);
                pm_mrunlock(pm);
                aspm_proc_affset(ckt, mld, mld_to_bv(mld));
                break;
        }
                
                
        case AFFLINK_EXEC:               /* exec */
        {
                /*
                 * We need to select a preferred mld from the newly created
                 * mldset.
                 */
                if (pmo_gettype(pm->pmo) == __PMO_MLD) {
                        mld = (mld_t*)pm->pmo;
                } else {
                        mldset = (mldset_t*)pm->pmo;
                        ASSERT(mldset);
                        mld = mldset_getmld(mldset, 0);
                }
                ASSERT(mld);
                pm_mrunlock(pm);
                aspm_proc_affset(ckt, mld, mld_to_bv(mld));
                break;
        }

        default:
                cmn_err(CE_PANIC,"[plac_policy_thread_afflink]: Invalid caller type");
        }
}

/*
 * This method provides generic services for this module:
 * -- destructor,
 * -- getname,
 * -- getaff (not valid for this placement module).
 */
/*ARGSUSED*/
static int
plac_policy_thread_plac_srvc(struct pm* pm, pm_srvc_t srvc, void* args, ...)
{
        ASSERT(pm);
        
        switch (srvc) {
        case PM_SRVC_DESTROY:
                plac_policy_thread_destroy(pm);
                return (0);

        case PM_SRVC_GETNAME:
		*(char **)args = plac_policy_thread_name();
                return (0);

        case PM_SRVC_GETAFF:
                return (0);

        case PM_SRVC_SYNC_POLICY:
		return (plac_policy_sync(pm, (pm_setas_t *)args));

	case PM_SRVC_GETARG:
        {
                va_list ap; 
                pmo_ns_t* ns;

                va_start(ap, args);
                ns = va_arg(ap, pmo_ns_t*);
                va_end(ap);
		*((int *)args) = pmo_ns_pmo_lookup(ns, __PMO_MLDSET, pm->pmo);
		return (0);
        }

        case PM_SRVC_ATTACH:
                return (0);

        default:
                cmn_err(CE_PANIC, "[plac_policy_thread_srvc]: Invalid service type");
        }

        return (0);
}
