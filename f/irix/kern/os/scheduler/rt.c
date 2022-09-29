/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992-1995 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#include <sys/kthread.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include "frame/frame_state.h"
#include "space.h"
#include "q.h"
#include "utility.h"
#include <sys/rt.h>
#include <sys/schedctl.h>

kthread_t *rt_gq;
static lock_t		gqlock;

kthread_t        	**bindings;
static lock_t           bindinglock;
int			rt_dither;
#define BINDING_TO_CPUID(b)      ((cpuid_t) ((b) - &bindings[0]))

void
init_rt(void)
{
	int i;

	bindings = kmem_zalloc(maxcpus * sizeof(kthread_t *), KM_NOSLEEP);
	spinlock_init(&bindinglock, "bindinglock");
	for (i = 0; i < maxcpus; i++)
		bindings[i] = NULL;
	spinlock_init(&gqlock, "rtgqlck");
}

#ifdef MP
kthread_t *
rt_thread_select()
{
	ASSERT(issplhi(getsr()));
	do {
		if (!rt_gq)
			return 0;
	} while (!nested_spintrylock(&gqlock));

	if (rt_gq) {
		kthread_t	*kt;
		kt = rt_gq;
		do {
			kthread_t *pkt;
			kt_nested_lock(kt);
			if (runcond_ok(kt) && 
				cpuset_runok(kt->k_cpuset, 
						cpu_to_cpuset[cpuid()])) {
				kt->k_rflink->k_rblink = kt->k_rblink;
				kt->k_rblink->k_rflink = kt->k_rflink;
				if (rt_gq == kt)
					rt_gq =
					  kt->k_rflink == kt ? 0 : kt->k_rflink;
				kt->k_rblink = kt->k_rflink = kt;
				nested_spinunlock(&gqlock);
				return kt; /* return kt locked */
			}
			pkt = kt;
			kt = kt->k_rflink;
			kt_nested_unlock(pkt);
		} while (kt != rt_gq);
	}
	nested_spinunlock(&gqlock);
	return 0;
}
#endif

int
rt_remove_q(kthread_t *kt)
{
	if (!nested_spintrylock(&gqlock))
		return 0;
	kt->k_rflink->k_rblink = kt->k_rblink;
	kt->k_rblink->k_rflink = kt->k_rflink;
	if (rt_gq == kt)
		rt_gq = kt->k_rflink == kt ? 0 : kt->k_rflink;
	kt->k_rblink = kt->k_rflink = kt;
	kt->k_onrq = CPU_NONE;
	nested_spinunlock(&gqlock);
	return 1;
}

INLINE static kpri_t
rt_peekqueue(kthread_t *kt)
{
	if (!kt || KT_ISNPRMPT(kt))
		return PIDLE;
	else
		return(kt->k_pri);
}

static kpri_t
rt_queued_pri(void)
{
	int p1, p2;
	p1 = rt_peekqueue(rt_gq);
	p2 = rt_peekqueue(private.p_cpu.c_threads);
	return(p1 > p2 ? p1 : p2);
}

void
reset_pri(kthread_t *kt)
{
	int qp = rt_queued_pri();

	private.p_curpri = kt->k_pri;
	if (qp > private.p_curpri)
		private.p_runrun = 1; /* FIX: REVISIT */
}

#define RTPRI(kt)	(kt ? kt->k_pri : -1)

static kpri_t
get_lowest_bound(int cpuset)
{
	int i;
	kpri_t lowpri;

	for (i = 0, lowpri = 256; i < maxcpus; i++) {
		/* Check for holes in processor space */
		if (!cpu_running(i) || 
			pdaindr[i].pda->p_cpu.c_restricted == 1 ||
		    !cpuset_runok(cpuset, cpu_to_cpuset[i]))
			continue;
		if (!bindings[i]) 
			return -1;
		if (bindings[i]->k_pri < lowpri)
			lowpri = bindings[i]->k_pri;
	}
	return lowpri;
}

void
start_rt(kthread_t *kt, kpri_t pri)
{
	int i;
	kthread_t **lowbinding, *victim;
	kpri_t lowpri;
	int cpuset = kt->k_cpuset == -1 ? CPUSET_GLOBAL : kt->k_cpuset;
	ASSERT(kt_islocked(kt) && issplhi(getsr()));
	ASSERT((kt->k_flags & KT_BIND) && kt->k_binding == CPU_NONE);
retry:
	nested_spinlock(&bindinglock);
	ASSERT(cpuset >= 1);
	lowbinding = NULL;
	if (KT_ISMR(kt)) {
		if (RTPRI(bindings[kt->k_mustrun]) < pri)
			lowbinding = &bindings[kt->k_mustrun];
		if (!lowbinding) {
			nested_spinunlock(&bindinglock);
			return;
		}
	} else {
		if (pri <= cpuset_lbpri(cpuset)) {
			nested_spinunlock(&bindinglock);
			return;
		}
		for (i = 0, lowpri = pri; i < maxcpus; i++) {
			/* Check for holes in processor space */
			if (!cpu_running(i))
				continue;
			if (!cpuset_runok(cpuset, cpu_to_cpuset[i]))
				continue;
			if (pdaindr[i].pda->p_cpu.c_restricted == 1)
				continue;
			if (!bindings[i]) {
				lowbinding = &bindings[i];
				break;
			} else if (RTPRI(bindings[i]) < lowpri) {
				lowbinding = &bindings[i];
				lowpri = bindings[i]->k_pri;
			}
		}
	}
	if (!lowbinding) { 
		nested_spinunlock(&bindinglock);
		return;
	}
	if (victim = *lowbinding) {
		if (kt_nested_trylock(victim)) {
			victim->k_binding = CPU_NONE;
			kt_nested_unlock(victim);
		} else {
			nested_spinunlock(&bindinglock);
			/* victim might also be trying to get the
			 * bindinglock, so give it a chance to get it */
			DELAY(1);
			goto retry;
		}
	}
	*lowbinding = kt;
	kt->k_binding = BINDING_TO_CPUID(lowbinding);
	cpuset_lbpri(cpuset) = get_lowest_bound(cpuset);
	nested_spinunlock(&bindinglock);
}

void
rt_rebind(kthread_t *kt)
{
	ASSERT(kt_islocked(kt) && (kt->k_flags & KT_BIND) &&
		(kt->k_binding != CPU_NONE || KT_ISMR(kt)));
	if (kt->k_binding != CPU_NONE) {
		/* 
		 * undo binding and try to re-establish
		 */
		nested_spinlock(&bindinglock);
		bindings[kt->k_binding] = NULL;
		ASSERT(kt->k_cpuset >= 1);
		if (pdaindr[kt->k_binding].pda->p_cpu.c_restricted != 1)
			cpuset_lbpri(kt->k_cpuset) = -1;
		kt->k_binding = CPU_NONE;
		nested_spinunlock(&bindinglock);
	}
	start_rt(kt, kt->k_pri);
}

void
end_rt(kthread_t *kt)
{
	ASSERT(kt_islocked(kt) && issplhi(getsr()));
	nested_spinlock(&bindinglock);
	if (kt->k_binding != CPU_NONE) {
		bindings[kt->k_binding] = NULL;
		ASSERT(kt->k_cpuset >= 1);
		if (pdaindr[kt->k_binding].pda->p_cpu.c_restricted != 1)
			cpuset_lbpri(kt->k_cpuset) = -1;
		kt->k_binding = CPU_NONE;
	}
	nested_spinunlock(&bindinglock);
}

/* ARGSUSED */
void
redo_rt(kthread_t *kt, kpri_t newpri)
{
	/* if kthread bound, may need to redo binding */
	/* needs the schedlock */
}

#ifdef MP
void
rt_add_gq(kthread_t *kt)
{
	kthread_t	*tkt;
	short		pri;

	ASSERT(kt_islocked(kt) && KT_ISPS(kt));
	ASSERT(kt == kt->k_rflink && kt == kt->k_rblink);

	pri = kt->k_preempt ? kt->k_pri + 1 : kt->k_pri;
	nested_spinlock(&gqlock);

	if (rt_gq) {
		tkt = rt_gq;
		do {
			if (tkt->k_pri < pri)
				break;
			tkt = tkt->k_rflink;
		} while (tkt != rt_gq);
		kt->k_rflink = tkt;
		kt->k_rblink = tkt->k_rblink;
		kt->k_rblink->k_rflink = kt;
		tkt->k_rblink = kt;
		if (rt_gq->k_pri < pri)
			rt_gq = kt;
	} else
		rt_gq = kt;
	kt->k_onrq = maxcpus;
	nested_spinunlock(&gqlock);
}
#endif

void
rt_restrict(cpuid_t cpu)
{
	kthread_t *kt;

	if ((kt = bindings[cpu]) == NULL) {
		return;
	}
		

	ASSERT(issplhi(getsr()));
	kt_nested_lock(kt);
	if ((kt->k_flags & KT_BIND) && kt->k_binding == cpu &&
		kt->k_mustrun != cpu)
	{
		rt_rebind(kt);
	}
	kt_nested_unlock(kt);
}

void
rt_unrestrict(cpuid_t cpu)
{
	ASSERT(issplhi(getsr()));
	nested_spinlock(&bindinglock);
	ASSERT(cpu >= 0);
	if (bindings[cpu])
		cpuset_lbpri(cpu_to_cpuset[cpu]) = get_lowest_bound(cpu_to_cpuset[cpu]);
	else
		cpuset_lbpri(cpu_to_cpuset[cpu]) = -1;
	nested_spinunlock(&bindinglock);
}

#ifdef MP
void *
rt_pin_thread(void)
{
	__psint_t pspin;
	kthread_t *kt;
	int s;

	if ((kt = curthreadp) == NULL)
		return 0;

	s = kt_lock(kt);
	pspin = kt->k_flags & KT_PSPIN;
	kt->k_flags |= KT_PSPIN;
	kt_unlock(kt, s);
	return (void *)pspin;
}

void
rt_unpin_thread(void *arg)
{
	kthread_t *kt;
	int s;
	int resched = 0;

	if ((kt = curthreadp) == NULL)
		return;

	s = kt_lock(kt);
	if (!arg) {
		kt->k_flags &= ~KT_PSPIN;
		if (private.p_cpu.c_restricted == 1 && cpuid() != kt->k_mustrun)
			resched = 1;
	}
	kt_unlock(kt, s);
	if (resched)
		qswtch(MUSTRUNCPU);
}
#endif
