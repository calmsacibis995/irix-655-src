#include <sys/types.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#undef _XOPEN4UX	/* Problem in stdlib */
#include <stdlib.h>

#include "load.h"

/*
 * load.c
 *
 *   Calculates the load average for the running system.
 *
 *   Clock ticks are distributed into the CPU_XXX fields, so the
 *   one-second deltas sum to roughly CLK_TCK (it may occasionally
 *   be little off since data is fetched asynchronously).
 *
 *   If gfx info is included, we'll have five bars showing
 *   "User Sys Intr IOwait GFXwait," where:
 *
 *	SUM     = (cpu_idle + cpu_user + cpu_kernel +
 *	  	   cpu_wait + cpu_sxbrk + cpu_intr)
 *
 *	User    = cpu_user / SUM
 *	Sys     = (cpu_kernel + cpu_sxbrk) / SUM)
 *	Intr    = cpu_intr / SUM
 *	IOwait  = (wait_io + wait_swap + wait_pio) / SUM
 *	GFXwait = (wait_gfxf + wait_gfxc) / SUM
 *
 *	These fractions sum to approximately 1.0.
 *
 *   If gfx info is not to be included, we'll have four bars showing
 *   "User Sys Intr IOwait," where gfx is lumped in with io as follows:
 *
 *	IOwait  = (wait_io + wait_swap + wait_pio +
 *		   wait_gfxf + wait_gfxc) / SUM
 */

#define MIN(x, y)	((x) < (y) ? (x) : (y))
#define SCALE(j, sum)	MIN((255 * (j) + (sum) / 2) / (sum), 255)

typedef struct si_subset_s {
    /* This structure matches the beginning of sysinfo_t */
    time_t		cpu[6];
    time_t		wait[5];
} si_subset_t;

static	si_subset_t    *si_prv[LOAD_CPU_MAX];
static	si_subset_t    *si_cur[LOAD_CPU_MAX];
static	si_subset_t    *si_avg[LOAD_CPU_MAX];

static	int		sinfosz;
static	int		ncpus_sysmp;
static	int		ncpus;
static	int		inited;

static	u_char	       *load_data;

/*
 * load_init
 */

static void load_init(void)
{
    int			i;

    if (! inited) {
	if (ncpus_sysmp == 0)
	    ncpus_sysmp = sysmp(MP_NPROCS);

	if (ncpus == 0)
	    ncpus = ncpus_sysmp;

	sinfosz = sizeof (si_subset_t);

	/*
	 * Hack: we make sure the 16 bytes preceding the load_data
	 * buffer are available for tacking on protocol info.
	 */

	load_data = (u_char *) malloc(LOAD_DATA_SIZE(ncpus) + 16) + 16;

	for (i = 0; i < ncpus; i++) {
	    si_prv[i] = (si_subset_t *) calloc(1, sinfosz);
	    si_cur[i] = (si_subset_t *) calloc(1, sinfosz);
	    si_avg[i] = (si_subset_t *) calloc(1, sinfosz);

	    sysmp(MP_SAGET1, MPSA_SINFO,
		  (char *) si_prv[i], sinfosz, i % ncpus_sysmp);
	}

	inited = 1;
    }
}

static void load_term(void)
{
    if (inited) {
	free(load_data);
	inited = 0;
    }
}

/*
 * load_cpu_get
 *
 *   Returns the number of CPUs present in the system.
 */

int load_cpu_get(void)
{
    load_init();

    return ncpus;
}

/*
 * load_cpu_set
 *
 *   Overrides the number of CPUs present in the system
 *   (primarily for debugging).
 */

void load_cpu_set(int ncpus_override)
{
    load_term();
    ncpus = ncpus_override;
    load_init();
}

/*
 * load_graph
 *
 *   Generates the data for a load graph and returns a pointer to the
 *   static buffer containing the data.
 *
 *   The data buffer receives 5 bytes of load information per CPU if
 *   include_gfx is true, for a total of (ncpus * 5) bytes.  The data
 *   buffer receives only 4 bytes of load information per CPU if
 *   include_gfx is false.
 *
 *   Each group of bytes represents the load for user, sys, intr, io
 *   (and possibly) gfx, respectively.  Each byte is scaled from 0 to
 *   255, and it is guaranteed that the sum of the bytes will not
 *   exceed 255.
 *
 *   Calculates the decay (smoothing) factor in terms of
 *   (1) The amount of decay per second (e.g. 0.25)
 *   (2) The graph (display) update period (e.g. 0.25)
 */

u_char *load_graph(int include_gfx,
		   double decay_per_second,
		   double update_period)
{
    int			i, j;
    u_char	       *p;
    double		decay_factor;

    load_init();

    decay_factor = pow(decay_per_second, update_period);

    p = load_data;

    for (i = 0; i < ncpus; i++) {
	int		sum, avg, new;
	int		t_user, t_sys, t_intr, t_io, t_gfx;	/* Ticks */

	sysmp(MP_SAGET1, MPSA_SINFO,
	      (char *) si_cur[i], sinfosz, i % ncpus_sysmp);

	/*
	 * The sum of all CPU values is usually CLK_TCK, except
	 * since values are obtained asynchronously, it may be
	 * off by a little.
	 */

	sum = 0;

	for (j = 0; j < 6; j++) {
	    avg = si_avg[i]->cpu[j];
	    new = si_cur[i]->cpu[j] - si_prv[i]->cpu[j];

	    avg = avg * decay_factor + new * (1.0 - decay_factor);

	    si_avg[i]->cpu[j] = avg;
	    si_prv[i]->cpu[j] = si_cur[i]->cpu[j];

	    sum += avg;
	}

	for (j = 0; j < 5; j++) {
	    avg = si_avg[i]->wait[j];
	    new = si_cur[i]->wait[j] - si_prv[i]->wait[j];

	    avg = avg * decay_factor + new * (1.0 - decay_factor);

	    si_avg[i]->wait[j] = avg;
	    si_prv[i]->wait[j] = si_cur[i]->wait[j];
	}

	t_user = (si_avg[i]->cpu[CPU_USER]);
	t_sys  = (si_avg[i]->cpu[CPU_KERNEL] +
		  si_avg[i]->cpu[CPU_SXBRK]);
	t_intr = (si_avg[i]->cpu[CPU_INTR]);
	t_io   = (si_avg[i]->wait[W_IO] +
		  si_avg[i]->wait[W_SWAP] +
		  si_avg[i]->wait[W_PIO]);
	t_gfx  = (si_avg[i]->wait[W_GFXF] +
		  si_avg[i]->wait[W_GFXC]);

	if (! include_gfx)
	    t_io += t_gfx;

	if (sum == 0)
	    sum = 1;

	j = t_user;
	*p++ = SCALE(j, sum);
	j += t_sys;
	*p++ = SCALE(j, sum);
	j += t_intr;
	*p++ = SCALE(j, sum);
	j += t_io;
	*p++ = SCALE(j, sum);

	if (include_gfx) {
	    j += t_gfx;
	    *p++ = SCALE(j, sum);
	}
    }

    return load_data;
}
