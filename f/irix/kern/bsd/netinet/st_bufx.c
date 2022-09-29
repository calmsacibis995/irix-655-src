/*
 *               Copyright (C) 1997 Silicon Graphics, Inc.             
 *                
 *  These coded instructions, statements, and computer programs  contain
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and
 *  are protected by Federal copyright law.  They  may  not be disclosed
 *  to  third  parties  or copied or duplicated in any form, in whole or
 *  in part, without the prior written consent of Silicon Graphics, Inc.
 *                
 *
 *  Filename: st_bufx.c
 *  Description: ST protocol's "bufx" module.
 *
 *  $Author: jimp $
 *  $Date: 1999/04/30 21:41:58 $
 *  $Revision: 1.9 $
 *
 */

/* Unfortunately, a lot of this code is SHAC-specific -- esp. w.r.t.
 * sprayomatic-ugliness.
*/

#include	"sys/param.h"
#include	"sys/types.h"
#include	"sys/systm.h"
#include 	"sys/cmn_err.h"
#include	"sys/debug.h"
#include	"sys/sema.h"
#include	"net/if.h"
#include	"sys/uio.h"
#include 	"sys/pfdat.h"
#include 	"sys/immu.h"
#include 	"sys/kmem.h"
#include 	"sys/ddi.h"
#include	"sys/alenlist.h"
#include	"sys/mips_addrspace.h"
#include	"ksys/as.h"
#include	"os/as/pmap.h"
#include	"sys/lpage.h"


/* #include	"st_debug.h" */
#include	"st_macros.h"
#include	"sys/st_ifnet.h"
#include	"st_bufx.h"



#define		ST_BUFX_DEBUG

#ifdef		ST_BUFX_DEBUG
int		STBufxDbgLvl = 0;		/* max 40 */
#define dprintf(lvl, x)  if(STBufxDbgLvl >= lvl) printf x
#else		/* !ST_BUFX_DEBUG */
#define dprintf(lvl, x)
#endif		/* ST_BUFX_DEBUG */


/* globals */
st_bufx_ctl_t			*bufx_ctl_head;
uint				num_bufx_entries;
mutex_t				bufx_ctl_mutex;

st_bufx_if_info_t		*interfaces;
uint				num_interfaces;
mutex_t				interfaces_mutex;

uint				last_alloced_bufx, num_last_alloced;
uint				last_released_bufx, num_last_released;

uint				SHAC_bufx_lows[MAX_SPRAY_WIDTHS];
uint				SHAC_bufx_highs[MAX_SPRAY_WIDTHS];


/* private functions */
static st_bufx_ctl_t 	*insert_bufx(st_bufx_ctl_t *, st_bufx_ctl_t *);

static int 	delete_bufx(st_bufx_ctl_t *, st_bufx_ctl_t *);

static 	st_bufx_if_info_t	*add_interface(struct ifnet *, uint *,
							uint *, uint);

static int get_bufx_range(st_bufx_if_info_t *, uint *, uint *, uint, 
							uint, uint);
static int 	update_bufx_range(uint *, uint *, uint);

static int 	record_bufx_alloc(st_bufx_if_info_t *, uint, 
						uint, uint, uint);
static int 	check_refcnt(void);
static int	sanity_check_interfaces(uint);
static	void 	dump_interfaces(void);
static	void 	dump_bufxes(void);


static int
bufx_to_spray_width(uint bufx)
{
	int	spray_width = -1;
	char	high_nibble;

	high_nibble = ((bufx & 0xf0000000) >> 28);
	switch(high_nibble)  {
		default:
			dprintf(0, ("Unknown high nibble; bufx 0x%x\n",
				bufx)); 
			break;
		case 0x0:
			spray_width = 1;
			break;
		case 0x2:
			spray_width = 2;
			break;
		case 0x6:
			spray_width = 4;
			break;
		case 0xa:
			spray_width = 6;
			break;
		case 0xe:
			spray_width = 8;
			break;
	}
	return spray_width;
}



void
register_an_interface(struct ifnet *interface, st_ifnet_t *st_ifp)
{
	int	i, spr_idx, error = 0;

	if(st_ifp)  {
		for(i = 0; i < MAX_SPRAY_WIDTHS - 1; i++)  {
			if(st_ifp->spray[i].bufx_num) {
				spr_idx = i+1;
				SHAC_bufx_lows[spr_idx] = 
					st_ifp->spray[i].bufx_base;
				SHAC_bufx_highs[spr_idx] = 
					SHAC_bufx_lows[spr_idx] + 
					st_ifp->spray[i].bufx_num - 1;
				dprintf(10, 
					("spray %d: lo 0x%x, hi 0x%x, num 0x%x (%d)\n",
					spr_idx, 
					(uint32_t) SHAC_bufx_lows[spr_idx], 
					(uint32_t) SHAC_bufx_highs[spr_idx],
					st_ifp->spray[i].bufx_num,
					st_ifp->spray[i].bufx_num));
			}
		}
	}
	ASSERT_ALWAYS(add_interface(interface, SHAC_bufx_lows, 
			SHAC_bufx_highs, MAX_SPRAY_WIDTHS) != NULL);
}


static void
dump_interfaces()
{
#	ifdef	ST_BUFX_DEBUG
	int	i, j, k;

	ASSERT(mutex_mine(&interfaces_mutex));

	printf("dump_interfaces: %d interfaces\n", num_interfaces);
	for(i = 0; i < num_interfaces; i++)  {
		if(interfaces[i].interface)  {
		   for(k = 0; k < MAX_SPRAY_WIDTHS; k++)  {
			printf("\t %d: if 0x%x (%s), lo %u, hi %u, free %u\n",
				i, interfaces[i].interface, 
				interfaces[i].interface->if_name,
				interfaces[i].bufx_lo[k],
				interfaces[i].bufx_hi[k],
				interfaces[i].num_free_ranges[k]);

			for(j = 0; 
				j < interfaces[i].num_free_ranges[k];
								j++)  {
				printf("\t \t free rng %u: lo %u, hi %u, num %u\n",
			    	j, interfaces[i].free_ranges[k][j].low,
			    	interfaces[i].free_ranges[k][j].high,
			    	interfaces[i].free_ranges[k][j].num_bufx);
			}
			for(; j < MAX_FREE_RANGES; j++)  {
				printf("\t \t UNUSED free rng %u: lo %u, hi %u, num %u\n",
			    	j, 
				interfaces[i].free_ranges[k][j].low,
			    	interfaces[i].free_ranges[k][j].high,
			    	interfaces[i].free_ranges[k][j].num_bufx);
			}
		    }
		}
	}
	printf("\n");
#	endif	/* ST_BUFX_DEBUG */
}


void
idbg_dump_interfaces(void (*printf)(char *, ...))
{
#	ifdef	ST_BUFX_DEBUG
	int	i, j, k;

	/* ASSERT(mutex_mine(&interfaces_mutex)); */

	printf("dump_interfaces: %d interfaces\n", num_interfaces);
	for(i = 0; i < num_interfaces; i++)  {
		if(interfaces[i].interface)  {
		    for(k = 0; k < MAX_SPRAY_WIDTHS; k++)  {
			printf("\t %d: if 0x%x (%s), lo %u, hi %u, free %u\n",
				i, interfaces[i].interface, 
				interfaces[i].interface->if_name,
				interfaces[i].bufx_lo[k],
				interfaces[i].bufx_hi[k],
				interfaces[i].num_free_ranges[k]);

			for(j = 0; 
				j < interfaces[i].num_free_ranges[k]; 
								j++)  {
				printf("\t \t free rng %u: lo %u, hi %u, num %u\n",
				    j, 
				interfaces[i].free_ranges[k][j].low,
				interfaces[i].free_ranges[k][j].high,
				interfaces[i].free_ranges[k][j].num_bufx);
			}
			for(; j < MAX_FREE_RANGES; j++)  {
				printf("\t \t UNUSED free rng %u: lo %u, hi %u, num %u\n",
				    j, 
				interfaces[i].free_ranges[k][j].low,
			    	interfaces[i].free_ranges[k][j].high,
			    	interfaces[i].free_ranges[k][j].num_bufx);
			}
		    }
		}
	}
	printf("\n");
#	endif	/* ST_BUFX_DEBUG */
}


static int
sanity_check_interfaces(uint spray_w)
{
	int		i, j, error = 0;

	for(i = 0; i < num_interfaces; i++)  {
		for(j = 0; j < interfaces[i].num_free_ranges[spray_w]; 
								j++)  {
			if(interfaces[i].free_ranges[spray_w][j].num_bufx == 0) {
				printf("Intf %u, spray %u (0x%x) gone insane at %d\n",
					i, spray_w,
					interfaces[i].interface, j);
				error = 1;
			}
		}
		for(; j < MAX_FREE_RANGES; j++)  {
			if(interfaces[i].free_ranges[spray_w][j].num_bufx != 0)  {
				printf("Intf %u, spray %u (0x%x) gone insane at %d\n",
					i, spray_w,
					interfaces[i].interface, j);
				error = 1;
			}
		}
	}

	
	if(error)  {
		dump_interfaces();
	}
	return error;
}


static void
dump_bufxes()
{
#	ifdef	ST_BUFX_DEBUG
	int 	i;
	
	ASSERT(mutex_mine(&bufx_ctl_mutex));

	printf("dump_bufxes: %u bufxes\n", num_bufx_entries);
	printf("last alloced %d bufxes starting %d\n", 
		num_last_alloced, last_alloced_bufx);
	printf("last released %d bufxes starting %d\n", 
		num_last_released, last_released_bufx);
	for(i = 0; i < num_bufx_entries; i++)  {
		if(bufx_ctl_head[i].bufsize != BUFSZ_FREE)  {
			printf("\t Bufx %u, gid %lu pid %lu, bufsz %u\n",
				i, bufx_ctl_head[i].gid,
				bufx_ctl_head[i].pid, 
				bufx_ctl_head[i].bufsize);
			printf("\t \t tot_sz %u, num bufx %u, base %u, paddr 0x%x\n",
				bufx_ctl_head[i].tot_size,
				bufx_ctl_head[i].num_bufxes,
				bufx_ctl_head[i].base_bufx,
				bufx_ctl_head[i].buf_paddrs);
		}
		else {
			printf("\t Bufx %u is free\n", i);
		}
	}
#	endif	/* ST_BUFX_DEBUG */
}

void
idbg_dump_bufxes(void (*printf)(char *, ...))
{
#	ifdef	ST_BUFX_DEBUG
	int 	i;
	
	printf("dump_bufxes: %u bufxes\n", num_bufx_entries);
	printf("last alloced %d bufxes starting %d\n", 
		num_last_alloced, last_alloced_bufx);
	printf("last released %d bufxes starting %d\n", 
		num_last_released, last_released_bufx);
	for(i = 0; i < num_bufx_entries; i++)  {
		if(bufx_ctl_head[i].bufsize != BUFSZ_FREE)  {
			printf("\t Bufx %u, gid %lu pid %lu, bufsz %u\n",
				i, bufx_ctl_head[i].gid,
				bufx_ctl_head[i].pid, 
				bufx_ctl_head[i].bufsize);
			printf("\t \t tot_sz %u, num bufx %u, base %u, paddr 0x%x\n",
				bufx_ctl_head[i].tot_size,
				bufx_ctl_head[i].num_bufxes,
				bufx_ctl_head[i].base_bufx,
				bufx_ctl_head[i].buf_paddrs);
		}
		else {
			printf("\t Bufx %u is free\n", i);
		}
	}
#	endif	/* ST_BUFX_DEBUG */
}


static void
dump_bufx(st_bufx_ctl_t *bufx)
{
	int		i;

	dprintf(0, ("bufx; gid %u, pid %u, bufsz %u, tot_size %u\n",
		bufx->gid, bufx->pid, bufx->bufsize, bufx->tot_size));
	dprintf(0, ("num_bufxes %u, base_bufx %u, num_frags %u\n",
		bufx->num_bufxes, bufx->base_bufx, bufx->num_frags));
	for(i = 0; i < bufx->num_bufxes; i++)  {
		dprintf(0, ("\t num %u: uaddr 0x%x, ulen %u\n",
			i, bufx->buf_uaddrs[i], bufx->buf_ulengths[i]));
	}
}

void
init_st_bufx_tab()
{
	bufx_ctl_head = kmem_zalloc(MAX_BUFX_ENTRIES * 
				sizeof(st_bufx_ctl_t), KM_SLEEP);
	ASSERT_ALWAYS(NULL != bufx_ctl_head);
	num_bufx_entries = 0;
	init_mutex(&bufx_ctl_mutex, MUTEX_DEFAULT, "st_bufx_ctl", 0);

	interfaces = kmem_zalloc(MAX_INTERFACES *
				sizeof(st_bufx_if_info_t), KM_SLEEP);
	num_interfaces = 0;
	init_mutex(&interfaces_mutex, MUTEX_DEFAULT, "st_interfaces", 0);
	
	return;
}


static st_bufx_if_info_t *
find_interface(struct ifnet *interface)
{
	int		i;
	
	for(i = 0; i < MAX_INTERFACES; i++)  {
		if(interfaces[i].interface == interface) {
			return &(interfaces[i]);
		}
	}

	return NULL;
}


static st_bufx_if_info_t *
add_interface(struct ifnet *interface, uint *buf_lo, uint *buf_hi, 
						uint num_spray)
{
	st_bufx_if_info_t	*intf = NULL;
	int			i;

	LOCK(interfaces_mutex);

	if(num_interfaces == MAX_INTERFACES)  {
		dprintf(0, ("No more space to add ST-bufx interfaces\n"));
		goto quit_add_interface;
	}

#	ifdef	ST_BUFX_DEBUG
	intf = find_interface(interface);
	if(intf)  {
		goto quit_add_interface;
	}
#	endif	/* ST_BUFX_DEBUG */

	intf = &(interfaces[num_interfaces]);
	ASSERT(intf->interface == NULL);

	for(i = 0; i < num_spray; i++, buf_hi++, buf_lo++)  {
		if(*buf_hi < *buf_lo)  {
			dprintf(0, 
				("spray %d Bad parms: low bufx (0x%x)"
				" is > high (0x%x)\n", 
				i, *buf_lo, *buf_hi));
			goto quit_add_interface;
		}

		intf->bufx_lo[i] = *buf_lo;
		intf->bufx_hi[i] = *buf_hi;
		intf->free_ranges[i][0].low = *buf_lo;
		intf->free_ranges[i][0].high = *buf_hi;
		intf->free_ranges[i][0].num_bufx = 
						*buf_hi - *buf_lo + 1;
		intf->num_free_ranges[i] = 1;
		dprintf(30, ("interface %s; low %d, high %d\n",
			intf->interface->if_name, intf->bufx_lo[i], 
			intf->bufx_hi[i]));
	}

	intf->interface = interface;
	num_interfaces++;

quit_add_interface:
	UNLOCK(interfaces_mutex);
	return intf;
}


static st_bufx_if_info_t *
if_was_added(struct ifnet *intf)
{
	int		i;

	ASSERT(mutex_mine(&interfaces_mutex));

	for(i = 0; i < num_interfaces; i++)  {
		if(interfaces[i].interface == intf)  {
			return &(interfaces[i]);
		}
	}

	ASSERT(i == num_interfaces);
	return NULL;
}



static int
find_free_bufx_entry(st_bufx_ctl_t *list_head)
{
	static	int	last_found = 0;
	int		num_searched;
	st_bufx_ctl_t	*cur_buf;

	ASSERT(list_head);
	ASSERT(mutex_mine(&bufx_ctl_mutex));
	ASSERT(mutex_mine(&interfaces_mutex));
	if(MAX_BUFX_ENTRIES < num_bufx_entries + 1) {
		return -1;
	}

	for(num_searched = 0; num_searched < MAX_BUFX_ENTRIES;
						num_searched++) {
		cur_buf = &(list_head[last_found]);
		if(cur_buf->bufsize == BUFSZ_FREE)  {
			bzero(cur_buf, sizeof(st_bufx_ctl_t));
			return last_found;
		}
		last_found = (last_found + 1) % MAX_BUFX_ENTRIES;
	}

	ASSERT(num_searched >= MAX_BUFX_ENTRIES);
	return	-1;
}


static int
find_free_range(st_bufx_range_t *range, uint max_num)
{
	int		i;
	st_bufx_range_t	*free;
	
	ASSERT(mutex_mine(&interfaces_mutex));

	for(i = 0; i < max_num; i++)  {
		free = &(range[i]);
		if(! free->num_bufx)  {
			return i;
		}
	}

	return -1;
}


static st_bufx_ctl_t *
find_bufx_entry(st_bufx_ctl_t *list_head, ulong_t gid, int base_bufx)
{
	int	cur_idx;

	ASSERT(list_head);
	ASSERT(mutex_mine(&bufx_ctl_mutex));
	for(cur_idx = 0; cur_idx < MAX_BUFX_ENTRIES; cur_idx++)  {
		if((list_head[cur_idx].gid == gid 
				|| gid == 0)
		    && list_head[cur_idx].base_bufx == base_bufx) {
			return &(list_head[cur_idx]);
		}
	}
	ASSERT(cur_idx >= MAX_BUFX_ENTRIES);
	return NULL;
}


static st_bufx_ctl_t *
insert_bufx(st_bufx_ctl_t *list_head, st_bufx_ctl_t *bufx)
{
	st_bufx_ctl_t	*free_bufx;
	int		free_idx;

	ASSERT(list_head);
	ASSERT(mutex_mine(&bufx_ctl_mutex));

#	ifdef	ST_BUFX_DEBUG
	if((free_bufx = find_bufx_entry(list_head, 
				bufx->pid, bufx->base_bufx)) != NULL) {
		dprintf(10, ("insert_into_list(): duplicate entry.\n"));
		return NULL;
	}
#	endif	/* ST_BUFX_DEBUG */

	if((free_idx = find_free_bufx_entry(list_head)) == -1) {
		return	NULL;
	}
	free_bufx = &(list_head[free_idx]);

	bcopy(bufx, free_bufx, sizeof(st_bufx_ctl_t));
	
	return free_bufx;
}


static int
delete_bufx(st_bufx_ctl_t *list_head, st_bufx_ctl_t *bufx)
{
	st_bufx_ctl_t	*free_bufx;

	ASSERT(list_head);
	ASSERT(mutex_mine(&bufx_ctl_mutex));

	if((free_bufx = find_bufx_entry(list_head,
			bufx->pid, bufx->base_bufx)) == NULL) {
		return BUFX_BAD_BUFX;
	}

	ASSERT(free_bufx->pid == bufx->pid);
	ASSERT(free_bufx->base_bufx == bufx->base_bufx);

	bzero(free_bufx, sizeof(st_bufx_ctl_t));
	return BUFX_SUCCESS;
}


static int
get_bufx_range(st_bufx_if_info_t *intf, uint *lo, uint *hi, 
				uint num_bufx, uint bufsz, uint spray_w)
{
	int	i, found = 0;

	ASSERT(mutex_mine(&bufx_ctl_mutex));
	ASSERT(mutex_mine(&interfaces_mutex));
	ASSERT(*hi - *lo >= num_bufx);

	dprintf(20, ("get_bufx_range: lo %u, hi %u, num_bufx %u, bufsz %u\n",
		*lo, *hi, num_bufx, bufsz));


	if(! if_was_added(intf->interface))  {
		dprintf(20, ("Interface 0x%x wasn't added\n",
			intf->interface));
		return BUFX_BAD_IF;
	}

	dprintf(20, ("Intf 0x%x: free ranges %u\n",
		intf, intf->num_free_ranges));

	for(i = 0; i < intf->num_free_ranges[spray_w]; i++) {
		if(intf->free_ranges[spray_w][i].num_bufx >= num_bufx) {
			if(intf->free_ranges[spray_w][i].low >= *lo)  {
				*lo = intf->free_ranges[spray_w][i].low;
				*hi = min(*hi, 
					intf->free_ranges[spray_w][i].high);
				dprintf(20, ("Found lo %u, hi %u\n",
					*lo, *hi));
				ASSERT(*hi - *lo >= num_bufx);
				found = 1;
			}
			else if(intf->free_ranges[spray_w][i].high 
							<= *hi)  {
				*hi = 
				    intf->free_ranges[spray_w][i].high;
				*lo = min(*lo, 
				    intf->free_ranges[spray_w][i].low);
				dprintf(20, ("Found hi %u, lo %u\n",
					*hi, *lo));
				ASSERT(*hi - *lo >= num_bufx);
				found = 1;
			}
			else {
				dprintf(20, ("Passing %u: low %u, high %u \n",
					intf->free_ranges[spray_w][i].low,
					intf->free_ranges[spray_w][i].high));
			}

			if(found)  {
				dprintf(20, ("Returning lo %u, hi %u\n",
					*lo, *hi));
				return BUFX_SUCCESS;
			}
		}
		else {
			dprintf(20, ("free range %u: too few bufxes (need %u, has %u)\n",
				i, num_bufx,
				intf->free_ranges[spray_w][i].num_bufx));
		}
	}

	ASSERT(i == intf->num_free_ranges[spray_w]);
	dprintf(20, ("get_bufx_range: no suitable range\n"));
	return BUFX_NO_SPACE;
}


static int
move_buf_ctl(st_bufx_range_t *list, uint *list_size,
					uint idx, uint num_indices)
{
	st_bufx_range_t	*tmp;

	if(*list_size + num_indices > MAX_FREE_RANGES)  {
		return BUFX_NO_SPACE;
	}
	tmp = kmem_alloc((*list_size - idx) * sizeof(st_bufx_range_t),
					KM_SLEEP);
	ASSERT_ALWAYS(NULL != tmp);
	bcopy(&(list[idx]), tmp,
			(*list_size - idx) * sizeof(st_bufx_range_t)); 
	bcopy(tmp, &(list[idx + num_indices]),
			(*list_size - idx) * sizeof(st_bufx_range_t)); 
	kmem_free(tmp, (*list_size - idx) * sizeof(st_bufx_range_t));
	*list_size += num_indices;
	dprintf(0, ("num_free_ranges inc by %d in move_buf_ctl\n",
		num_indices));
	return 0;
}


static void
delete_buf_range(st_bufx_range_t *list, uint *list_size,
					uint idx, uint num_indices)
{
	st_bufx_range_t	*tmp;
	int		first_copy_entry;
	int		num_moving_entries;

	dprintf(10, ("Before deleting %d entries, starting idx %d\n",
		num_indices, idx));
	/* dump_interfaces(); */

	first_copy_entry = idx + num_indices;
	ASSERT(*list_size >= first_copy_entry);
	num_moving_entries = (*list_size - first_copy_entry);
	if(num_moving_entries) {
		tmp = kmem_alloc(num_moving_entries
				* sizeof(st_bufx_range_t), KM_SLEEP);
		ASSERT_ALWAYS(NULL != tmp);
		bcopy(&(list[first_copy_entry]), tmp,
	     		num_moving_entries * sizeof(st_bufx_range_t));
		bcopy(tmp, &(list[idx]), num_moving_entries
	     				* sizeof(st_bufx_range_t));
		kmem_free(tmp, num_moving_entries
					* sizeof(st_bufx_range_t));
	}
	else {
		/* this might be subsumed by the bzero outside the 
		** else: to be checked later */
		/* dump_interfaces(); */
		bzero(&(list[idx]), num_indices
					* sizeof(st_bufx_range_t));
	}

	dprintf(10, ("After deleting %d entries\n",
		num_indices));
	/* dump_interfaces(); */

	/* bzero the last entries: they were just clobbered */
	bzero(&(list[*list_size - num_indices]), num_indices
					* sizeof(st_bufx_range_t));
	*list_size -= num_indices;
	dprintf(10, ("num_free_ranges dec. by %d in delete_buf_range\n",
		num_indices));
	/* dump_interfaces(); */
}




static int
record_bufx_alloc(st_bufx_if_info_t *intf, uint lo, uint hi, 
					uint num_bufx, uint spray_w)
{
	int		i, range_found;

	ASSERT(mutex_mine(&bufx_ctl_mutex));
	ASSERT(mutex_mine(&interfaces_mutex));
	ASSERT(hi - lo == num_bufx - 1);


	dprintf(20, ("record_bufx: free_rngs %u, lo %u, hi %u, num %u\n",
		intf->num_free_ranges, lo, hi, num_bufx));

	for(i = 0, range_found = 0; 
			i < intf->num_free_ranges[spray_w]; i++) {
		if(intf->free_ranges[spray_w][i].num_bufx >= num_bufx
				&& intf->free_ranges[spray_w][i].low <= lo
				&& intf->free_ranges[spray_w][i].high >= hi)  {
			dprintf(20, ("record_bufx %d: free_rngs %u, "
				"low %u, high %u\n",
				i, intf->num_free_ranges[spray_w], 
				intf->free_ranges[spray_w][i].low, 
				intf->free_ranges[spray_w][i].high));
			range_found = 1;
			if(lo == intf->free_ranges[spray_w][i].low
			    && hi == intf->free_ranges[spray_w][i].high) {
				ASSERT(num_bufx == 
					intf->free_ranges[spray_w][i].num_bufx);
				delete_buf_range(intf->free_ranges[spray_w], 
						&intf->num_free_ranges[spray_w],
						i, 1);
				dprintf(20, ("Deleted range %u to %u, num_free %u\n",
					lo, hi, intf->num_free_ranges[spray_w]));
			}
			else if(lo == intf->free_ranges[spray_w][i].low
			    && hi < intf->free_ranges[spray_w][i].high) {
				dprintf(20, ("Updating low %u to %u (low collapse)\n",
					intf->free_ranges[spray_w][i].low, hi+1));
				intf->free_ranges[spray_w][i].low = hi + 1;
				intf->free_ranges[spray_w][i].num_bufx -=
							num_bufx;
				dprintf(20, ("New num_bufx %u\n",
					intf->free_ranges[spray_w][i].num_bufx));
			}
			else if(lo > intf->free_ranges[spray_w][i].low
			    && hi == intf->free_ranges[spray_w][i].high) {
				dprintf(20, ("Updating high %u to %u (hi collapse)\n",
					intf->free_ranges[spray_w][i].high, lo - 1));
				intf->free_ranges[spray_w][i].high = lo - 1;
				intf->free_ranges[spray_w][i].num_bufx -=
							num_bufx;
				dprintf(20, ("New num_bufx %u\n",
					intf->free_ranges[spray_w][i].num_bufx));
			}
			else if(lo > intf->free_ranges[spray_w][i].low
			    && hi < intf->free_ranges[spray_w][i].high) {
				if(move_buf_ctl(intf->free_ranges[spray_w],
					    &intf->num_free_ranges[spray_w], 
								i, 1)) {
					return BUFX_NO_SPACE;
				}
				intf->free_ranges[spray_w][i].high = lo - 1;
				intf->free_ranges[spray_w][i].num_bufx = 
					intf->free_ranges[spray_w][i].high -
					   intf->free_ranges[spray_w][i].low + 1;
				intf->free_ranges[spray_w][i+1].low = hi + 1;
				intf->free_ranges[spray_w][i+1].num_bufx = 
					intf->free_ranges[spray_w][i+1].high -
					   intf->free_ranges[spray_w][i+1].low + 1;
			}
			else {
				cmn_err(CE_PANIC, 
				    "Unknown path in record_bufx_alloc "
				    "(lo %u, hi %u, low %u, high %u\n",
				    	lo, hi, intf->free_ranges[spray_w][i].low,
					intf->free_ranges[spray_w][i].high);
			}
		}
	}

	ASSERT(range_found);
	return BUFX_SUCCESS;
}


static int
update_bufx_range(uint *lo, uint *hi, uint num_bufx)
{
	ASSERT(*hi - *lo >= num_bufx);
	ASSERT(*lo != (uint) -1);
	*hi = *lo + num_bufx - 1;
	return BUFX_SUCCESS;
}


static int
register_bufx_alloc(ulong_t gid, ulong_t pid, uint bufsize, 
			uint num_bufx, uint base_bufx, uint *cookie)
{
	st_bufx_ctl_t	*free_bufx;
	int		free_idx;

	ASSERT(mutex_mine(&bufx_ctl_mutex));
	ASSERT(mutex_mine(&interfaces_mutex));

	if((free_idx = find_free_bufx_entry(bufx_ctl_head)) == -1)  {
		return BUFX_NO_SPACE;
	}

	free_bufx = &(bufx_ctl_head[free_idx]);
	free_bufx->gid = gid;
	free_bufx->pid = pid;
	free_bufx->bufsize = bufsize;
	free_bufx->tot_size = bufsize * num_bufx;
	free_bufx->num_bufxes = num_bufx;
	free_bufx->base_bufx = base_bufx;
	ASSERT(free_bufx->buf_paddrs == NULL);
	*cookie = free_idx;
	num_bufx_entries++;
	dprintf(30, ("Returning bufx %u from register_bufx_alloc\n",
		free_idx));

	return 0;
}

void
print_uio_frags(alenlist_t *in_alen)
{
	alenaddr_t	addr;
	size_t		length;
	uint		frag_num = 0;

	printf("printing uio fragments\n");
	while(ALENLIST_SUCCESS == (alenlist_get(*in_alen, NULL, 0, 
						&addr, &length, 0))) {
		printf("print_uio_frags: frag num %d, size %d, "
				"paddr 0x%x\n",
			frag_num++, length, addr); 
	}
}


int
get_lgpgsize(void *vaddr)
{
	int		lgpgsz = -1;
	preg_t		*prp;
	vasid_t		vasid;
	pas_t		*pas;
	ppas_t 		*ppas;
	pde_t		*ppte;
	pgno_t		sz, size;
	char		*pvaddr;

	dprintf(30, ("get_lgpgsize, vaddr 0x%x, USEG %d\n", 
		vaddr, IS_KUSEG(vaddr)));
	as_lookup_current(&vasid);
	pas = VASID_TO_PAS(vasid);
	ppas = (ppas_t *) vasid.vas_pasid;

	mraccess(&pas->pas_aspacelock);

	prp = findpreg(pas, ppas, vaddr);
	if(! prp)  {
		dprintf(0, ("NULL prp after findpreg\n")); 
		goto as_rele_quit_get_lgpgsize;
	}

	size = prp->p_pglen;
	pvaddr = prp->p_regva;

	reglock(prp->p_reg);
	ppte = pmap_probe(prp->p_pmap, &pvaddr, &size, &sz);
	if(! ppte)  {
		dprintf(0, ("NULL ppte after pmap_probe\n")); 
		goto reg_rele_quit_lgpgsize;
	}
	
	dprintf(10, ("pte: pte_shotdn 0x%x, pte_pgmaskshift 0x%x\n",
		ppte->pte.pte_shotdn, ppte->pte.pte_pgmaskshift));
	lgpgsz = PAGE_MASK_INDEX_TO_SIZE(ppte->pte.pte_pgmaskshift);
	dprintf(10, ("lgpgsz seen as %d on vaddr 0x%x\n", 
		lgpgsz, vaddr));
	ASSERT_ALWAYS(ALL_PAGE_SZ_MASK & lgpgsz);

reg_rele_quit_lgpgsize:
	regrele(prp->p_reg);
as_rele_quit_get_lgpgsize:
	mrunlock(&pas->pas_aspacelock);
	return lgpgsz;
}

int
uio_to_frag_size(iovec_t *iov, uint iovcnt, alenlist_t *in_alen,
				int xfer_dir, opaque_t *dma_cookies)
{
	int		i;
 	alenlist_t	sub_alen = NULL;
 	alenlist_t	tmp_alen = NULL;
 	alenlist_t	fragsize_alen = NULL;
	int		error = BUFX_SUCCESS;
	void		*buf, *fragsize_buf;
	char		*curr_addr;
	uint		num_userdma_set = 0;
	size_t		size;
	alenaddr_t	addr, tmp_alen_addr;
	size_t		length, tmp_alen_length, min_frag_size = -1;
	uint		frag_num = 0;
	__psint_t	offset;
	int		is_uvaddr;


	ASSERT(in_alen);
	for(i = 0; i < iovcnt; i++)  {
		buf = iov[i].iov_base;
		offset = ((__psint_t) (buf))
				- ((__psint_t) (buf) & ~(NBPP - 1));
		fragsize_buf = (void *) 
			((__psint_t) (buf) & ~(NBPP - 1));
		dprintf(10, ("buf @ 0x%x; round to 16K 0x%x, offset 0x%x\n",
			iov[i].iov_base, fragsize_buf, offset));
		size = iov[i].iov_len;
		dprintf(10, ("uio_to_frag_size: adjusted buf at 0x%x, len %d, off %d (0x%x)\n",
			buf, size, offset, offset));
		if(! IS_KUSEG(buf))  {
			is_uvaddr = 0;
			sub_alen = kvaddr_to_alenlist(sub_alen, buf, 
					size, AL_LEAVE_CURSOR);
			if(offset)  {
				fragsize_alen = kvaddr_to_alenlist(
					fragsize_alen, fragsize_buf, 
					size, AL_LEAVE_CURSOR);
			}
		} 
		else {
			is_uvaddr = 1;
			dprintf(20, ("uvaddr 0x%x size %u for bufx\n", 
				buf, size));
			ASSERT(buf);
			ASSERT(size);
			if(dma_cookies)  {
				/* read/write flag to be either 
				*  RD or WR;
				*  we take advantage of pinning an
				*  offset into a page, for pinning 
				*  the whole page */
				dprintf(30, 
				("pinning %d bytes, at uvaddr 0x%x\n",
					size, buf));
				if(fast_userdma(buf, size,
					xfer_dir, &(dma_cookies[i]))) {
					dprintf(0, ("fast_userdma err: "
						"uvaddr 0x%x, size %u\n",
						buf, size));
					error = BUFX_EFAULT;
					goto quit_uio_to_fragsize;
				}
				num_userdma_set++;
			}
			dprintf(20, ("uvaddr 0x%x size %u before uvaddr_to_alenlist\n", 
				buf, size));
			sub_alen = uvaddr_to_alenlist(sub_alen, buf,
						size, AL_LEAVE_CURSOR);
			if(offset)  {
				fragsize_alen = uvaddr_to_alenlist(
					fragsize_alen, fragsize_buf, 
						size, AL_LEAVE_CURSOR);
			}
			/**********
			al_size = alenlist_size(sub_alen);
			dprintf(0, ("alenlist_size %d, xfer sz %d\n", 
				al_size, size));
			**********/
		}
		alenlist_concat(sub_alen, *in_alen);
		alenlist_destroy(sub_alen);
		sub_alen = NULL;
	}

	if(sub_alen)  {
		alenlist_destroy(sub_alen);
	}

	if(! offset)  {
		ASSERT_ALWAYS(NULL == fragsize_alen);
		fragsize_alen = *in_alen;
	}

	curr_addr = (char *) buf;
	while(ALENLIST_SUCCESS == (alenlist_get(fragsize_alen, NULL, 0, 
					&addr, &tmp_alen_length, 0))) {
		/* hack for f^$@ing brain-dead SHAC & s/w "spec",
		   which mandates that all frags must be of same size */
		dprintf(10, ("alenlist: frag num %d, size %d, "
				"paddr 0x%x\n",
			frag_num++, tmp_alen_length, addr)); 

		if(is_uvaddr)  {
			/* get lgpgsz */
			/* hack for god-damned prod-con tests, which
			** don't have pagemask set for lpages */
			length = max(tmp_alen_length,
				get_lgpgsize((void *) curr_addr));
		}
		else {
			length = tmp_alen_length;
		}

		if(length < min_frag_size || min_frag_size == -1) {
			min_frag_size = length;
		}

		curr_addr += tmp_alen_length;
	}

	if(min_frag_size % NBPP)  {
		min_frag_size = NBPP;
		dprintf(20, ("alenlist: min frag updated to %d\n", 
			min_frag_size)); 
	}
	else while(min_frag_size && 
				!(ALL_PAGE_SZ_MASK & min_frag_size)) {
		dprintf(2, ("%u pgsz invalid; dec to %u\n",
			min_frag_size, min_frag_size >> 1));
		min_frag_size >>= 1; 
	}

	dprintf(20, ("alenlist: min frag size %d\n", min_frag_size)); 
	ASSERT_ALWAYS(min_frag_size > 0);


quit_uio_to_fragsize:
	if(sub_alen)  {
		alenlist_destroy(sub_alen);
	}
	if(fragsize_alen && fragsize_alen != *in_alen) {
		alenlist_destroy(fragsize_alen);
	}
	if(error)  {
		dprintf(0, ("error is %u in uio_to_fragsize\n", error));
		for(i = 0; i < num_userdma_set; i++)  {
			dprintf(30, 
				("unpinning %d bytes, at uvaddr 0x%x\n",
				iov[i].iov_len,
				iov[i].iov_base));
			fast_undma(iov[i].iov_base, iov[i].iov_len,
				xfer_dir, &(dma_cookies[i]));
		}
		bzero(dma_cookies, num_userdma_set * sizeof(opaque_t));
		min_frag_size = -1;
	}

	dprintf(30, ("uio_to_fragsz: returning %d frag size \n",
		min_frag_size));

	return min_frag_size;
}


int
st_bufx_alloc(uint *base_bufx, uint num_bufx, uint bufsz, 
				struct ifnet **ifs, uint num_ifs,
				uint spray_width, uint *cookie)
{
	st_bufx_if_info_t	*intf;
	uint			cur_lo, cur_hi;
	uint			lo, hi;
	int			interval_changed;
	int			status, error = BUFX_SUCCESS;
	int			i;
	ulong_t			gid = 0, pid = 0;
	st_bufx_ctl_t		*bufx = NULL;
	

	ASSERT(ifs != NULL);
	ASSERT_ALWAYS(num_ifs == 1);		/* for now */
	if(1 < spray_width) {
		dprintf(20, ("st_bufx_alloc: spraying over %d bufs\n",
			spray_width));
	}

	switch(spray_width) {
		default:
			cmn_err(CE_WARN, "Unknown spray width\n");
			return BUFX_BAD_SPRAY;
			/* NOTREACHED */
			break;
		case 1:
			cur_lo = SHAC_bufx_lows[1];
			cur_hi = SHAC_bufx_highs[1];
			break;
		case 2:
			cur_lo = SHAC_bufx_lows[2];
			cur_hi = SHAC_bufx_highs[2];
			break;
		case 4:
			cur_lo = SHAC_bufx_lows[4];
			cur_hi = SHAC_bufx_highs[4];
			break;
		case 6:
			cur_lo = SHAC_bufx_lows[6];
			cur_hi = SHAC_bufx_highs[6];
			break;
		case 8:
			cur_lo = SHAC_bufx_lows[8];
			cur_hi = SHAC_bufx_highs[8];
			break;
	}
	
	if(drv_getparm(PPGRP, &gid) == -1)  {
		dprintf(5, ("Couldn't get group-id at st_bufx_alloc\n"));
	}
	if(drv_getparm(PPID, &pid) == -1)  {
		dprintf(0, ("Couldn't get pid at st_bufx_alloc \n"));
	}


	LOCK(interfaces_mutex);
	LOCK(bufx_ctl_mutex);

	if(sanity_check_interfaces(spray_width))  {
		cmn_err(CE_PANIC, "Intf. error seen before st_bufx_alloc "
			"num_bufx %u, bufsz %u, num_ifs %u\n",
			num_bufx, bufsz, num_ifs);
	}

	lo = cur_lo;
	hi = cur_hi;
	do {
		for(i = 0, interval_changed = 0; i < num_ifs; i++)  {
			intf = if_was_added(ifs[i]);
			if(intf == NULL) {
				dump_interfaces();
				cmn_err(CE_PANIC, "i %d; intf 0x%x; tot xfaces %d\n",
					i, ifs[i], num_interfaces);
			}
			status = get_bufx_range(intf, &lo, &hi, 
					  num_bufx, bufsz, spray_width);
			if(! status)  {
				dprintf(20, ("get_bufx_range: low %u, hi %u\n",
					lo, hi));
				if(lo > cur_lo) {
					cur_lo = lo;
					interval_changed = 1;
				}
				if(hi < cur_hi) {
					cur_hi = hi;
					interval_changed = 1;
				}
				if(interval_changed)  {
					break;	/* try again all ifs */
				}
			}
			else {
				cmn_err(CE_WARN, "Bad status %d from get_bufx_range\n",
					status);
				error = status;

#if	0
				dump_interfaces();
				dump_bufxes();

				/* freq. used for debug of bufx code */
				STBufxDbgLvl = 30;
				get_bufx_range(intf, &lo, &hi,
						      num_bufx, bufsz);
				ASSERT(0);
#endif
				
				goto quit_st_bufx_alloc;
			}
		}
	} while (interval_changed);

	status = update_bufx_range(&cur_lo, &cur_hi, num_bufx);
	ASSERT_ALWAYS(BUFX_SUCCESS == status);

	dprintf(30, ("update_bufx_range: cur_lo %u, cur_hi %u\n",
		cur_lo, cur_hi));

	for(i = 0; i < num_ifs; i++)  {
		intf = if_was_added(ifs[i]);
		error = record_bufx_alloc(intf, cur_lo, cur_hi, 
						num_bufx, spray_width);
		if(error)  {
			goto quit_st_bufx_alloc;
		}
	}

	if(error = register_bufx_alloc(gid, pid, bufsz, num_bufx, 
						cur_lo, cookie))  {
		goto quit_st_bufx_alloc;
	}

	*base_bufx = cur_lo;

	dprintf(10, ("bufx_alloc: allocated %u bufxes starting at %u\n",
		num_bufx, *base_bufx));

	bufx = &(bufx_ctl_head[*cookie]);

	ASSERT(bufx->num_bufxes);
	ASSERT(NULL == bufx->buf_uaddrs);
	bufx->buf_uaddrs = kmem_zalloc(
			bufx->num_bufxes * sizeof(void *), KM_SLEEP);
	ASSERT(NULL != bufx->buf_uaddrs);

	ASSERT(NULL == bufx->buf_ulengths);
	bufx->buf_ulengths = kmem_zalloc(
			bufx->num_bufxes * sizeof(size_t), KM_SLEEP);
	ASSERT(NULL != bufx->buf_ulengths);

	ASSERT(NULL == bufx->dma_cookies);
	bufx->dma_cookies = kmem_zalloc(
			bufx->num_bufxes * sizeof(opaque_t), KM_SLEEP);
	ASSERT(NULL != bufx->dma_cookies);

	if(sanity_check_interfaces(spray_width))  {
		cmn_err(CE_PANIC, "Intf. error seen in st_bufx_alloc "
			"num_bufx %u, bufsz %u, num_ifs %u\n",
			num_bufx, bufsz, num_ifs);
	}

quit_st_bufx_alloc:
	if(error && bufx)  {
		if(bufx->buf_uaddrs)  {
			kmem_free(bufx->buf_uaddrs, 
				bufx->num_bufxes * sizeof(void *));
		bufx->buf_uaddrs = NULL;
		}
		if(bufx->buf_ulengths)  {
			kmem_free(bufx->buf_ulengths, 
				bufx->num_bufxes * sizeof(size_t));
			bufx->buf_ulengths = NULL;
		}
		if(bufx->dma_cookies)  {
			kmem_free(bufx->dma_cookies, 
				bufx->num_bufxes * sizeof(opaque_t));
			bufx->dma_cookies = NULL;
		}
	}
	UNLOCK(bufx_ctl_mutex);
	UNLOCK(interfaces_mutex);

	dprintf(20, ("st_bufx_alloc: returning %u bufxes starting 0x%x\n",
		num_bufx, *base_bufx));

	if(! error)  {
		last_alloced_bufx = *base_bufx;
		num_last_alloced = num_bufx;
	}

	return error;
}


static int
check_refcnt()
{
	dprintf(5, ("check_refcnt() not implemented yet\n"));
	return 0;
}



static int
find_prev_next(st_bufx_range_t *list,  uint low, uint high,
			int *this, int *prev, int *next, uint list_size)
{
#	define	FIND_RANGE_ALL_FOUND	0x0
#	define	FIND_RANGE_NO_THIS	0x01
#	define	FIND_RANGE_NO_PREV	0x02
#	define	FIND_RANGE_NO_NEXT	0x04


	st_bufx_range_t		*ret = NULL, *curr;
	uint			lower_diff, upper_diff;
	int			i, j;
	int			error = FIND_RANGE_ALL_FOUND;


	dprintf(30, ("find_prev_next: lo %u, hi %u, list-size %u\n",
		low, high, list_size));

	ASSERT(list);
	ASSERT(high >= low);
	ASSERT(this);
	ASSERT(prev);
	ASSERT(next);

	for(i = 0; i < list_size; i++)  {
		j = i;
		ret = &(list[j]);
		dprintf(40, ("Going over list el [%u, %u]\n",
			ret->low, ret->high));
		if(ret->low == low && ret->high == high) {
			break;
		}
	}

	if(ret->low == low && ret->high == high)  {
		*this = j;
	}
	else {
		*this = -1;
		error |= FIND_RANGE_NO_THIS;
	}

	lower_diff = upper_diff = (uint) -1;
	error |= (FIND_RANGE_NO_PREV | FIND_RANGE_NO_NEXT);
	*prev = *next = -1;

	for(i = 0; i < list_size; i++)  {
		curr = &(list[i]);
		dprintf(40, ("list entr %d, num_bufx %u, lo %u, hi %u\n", 
			i, curr->num_bufx, curr->low, curr->high));
		if(curr->num_bufx)  {
			if(low > curr->high
			    && (low - curr->high < lower_diff))  {
				lower_diff = low - curr->high;
				*prev = i;
				error &= ~(FIND_RANGE_NO_PREV);
				dprintf(30, ("Found new lo range %u\n",
					curr->high));
			}
			if(high < curr->low
			    && (curr->low - high < upper_diff))  {
				upper_diff = curr->low - high;
				*next = i;
				error &= ~(FIND_RANGE_NO_NEXT);
				dprintf(30, ("Found new hi range %u\n",
					curr->low));
			}
		}
	}

	dprintf(20, ("f_p_n: returning t %d, p %d, n %d\n",
		*this, *prev, *next));
	return error;
}


static void
free_list_compaction(st_bufx_if_info_t *intf, uint spray_w)
{
	int	i, num_occupied, free, last_free = 0;

	for(i = intf->num_free_ranges[spray_w]; 
				i < MAX_FREE_RANGES; i++)  {
		if(intf->free_ranges[spray_w][i].num_bufx != 0)  {
			for(free = last_free; 
				free < MAX_FREE_RANGES; free++)  {
				if(! intf->free_ranges[spray_w][free].num_bufx) {
					last_free = free;
					break;
				}
			}
			if(free == MAX_FREE_RANGES)  {
				cmn_err(CE_PANIC, "MAX_FREE_RANGES too small\n");
			}
			bcopy(&(intf->free_ranges[spray_w][i]),
				&(intf->free_ranges[spray_w][free]),
				sizeof(st_bufx_range_t));
			bzero(&(intf->free_ranges[spray_w][i]), 
				sizeof(st_bufx_range_t));
		}
	}

	for(i = num_occupied = 0; i < MAX_FREE_RANGES; i++)  {
		if(intf->free_ranges[spray_w][i].num_bufx != 0)  {
			num_occupied++;
		}
	}

	ASSERT_ALWAYS(num_occupied == intf->num_free_ranges[spray_w]);
}


static int
add_free_to_intf(st_bufx_if_info_t *intf, uint lo, uint hi, 
							uint spray_w)
{

	st_bufx_range_t		*prev = NULL, *next = NULL;
	st_bufx_range_t		*curr = NULL;
	st_bufx_range_t		*free;
	int			prev_idx, next_idx, curr_idx;
	int			free_idx, status;
	uint			num_freed_bufs = hi - lo + 1;


	if((status = find_prev_next(intf->free_ranges[spray_w], lo, hi, 
			&curr_idx, &prev_idx, &next_idx, 
						MAX_FREE_RANGES)))  {
		dprintf(20, ("f_p_n: 0x%x; cur %d, prev %d, next %d\n",
			status, curr_idx, prev_idx, next_idx));
	}

	ASSERT(next_idx != prev_idx || prev_idx == -1);
	
	if(prev_idx != -1)  {
		prev = &(intf->free_ranges[spray_w][prev_idx]);
		dprintf(30, ("Prev is 0x%x\n", prev));
	}
	if(curr_idx != -1) {
		curr = &(intf->free_ranges[spray_w][curr_idx]);
		dprintf(30, ("Curr is 0x%x\n", curr));
	}
	if(next_idx != -1)  {
		next = &(intf->free_ranges[spray_w][next_idx]);
		dprintf(30, ("Next is 0x%x\n", next));
	}

	if(prev && (prev->high + 1) == lo)  {
		/* the lady vanishes! */
		dprintf(10, ("hi of prev %u, num %u\n", 
			prev->high, prev->num_bufx));
		prev->high = hi;
		prev->num_bufx += num_freed_bufs;
		dprintf(10, ("prev hi made %u, num %u\n",
			prev->high, prev->num_bufx));
		num_freed_bufs = 0;
	}

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error after prev in add_free "
			"intf 0x%x, lo %u, hi %u\n",
			intf, lo, hi);
	}

	if(next && (next->low - 1) == hi)  {
		if(num_freed_bufs)  {
			dprintf(10, ("lo of next %u, num %u\n",
				next->low, next->num_bufx));
			next->low = lo;
			next->num_bufx += num_freed_bufs;
			dprintf(10, ("next lo made %u, num %u\n",
				next->low, next->num_bufx));
			num_freed_bufs = 0;
		}
		else {
			ASSERT(prev);
			dprintf(10, ("num_free_ranges will be inc; prev <%u, %u, %u> \n",
				prev->low, prev->high, prev->num_bufx));
			if(next)  {
				dprintf(10, ("next: <%u, %u, %u>\n",
					next->low, next->high,
					next->num_bufx));
			}
			else {
				dprintf(10, ("No next\n"));
			}
			
			/* whichever is lower in the list should 
			vanish, aiding list compaction */
			if(prev_idx < next_idx) {
				prev->high = next->high;
				prev->num_bufx += (next->num_bufx);
				bzero(next, sizeof(st_bufx_range_t));
				(intf->num_free_ranges[spray_w])--;
				free_list_compaction(intf, spray_w);
			}
			else {
				next->low = prev->low;
				next->num_bufx += (prev->num_bufx);
				bzero(prev, sizeof(st_bufx_range_t));
				(intf->num_free_ranges[spray_w])--;
				free_list_compaction(intf, spray_w);
			}

			dprintf(10, 
			  ("num_free_ranges has been dec by 1 (to %d) "
					"in add_free, lo %u, hi %u\n",
				intf->num_free_ranges[spray_w], lo, hi));
			/* dump_interfaces(); */

			/* ASSERT(intf->num_free_ranges <= 
						MAX_FREE_RANGES); */
			if(intf->num_free_ranges[spray_w] > MAX_FREE_RANGES)  {
				dump_interfaces();
				dump_bufxes();
				cmn_err(CE_PANIC, "%u free ranges; lo %u, hi %u)\n",
					intf->num_free_ranges[spray_w], lo, hi);
			}
			dprintf(10, 
			  	("prev hi made next hi (%u), num %u\n",
				prev->high, prev->num_bufx));
		}
	}

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error after next (%u, %u, %u) "
			"in add_free intf 0x%x, lo %u, hi %u\n",
			next->low, next->high, next->num_bufx,
			intf, lo, hi);
	}


	if(num_freed_bufs)  {
		dprintf(10, ("Adding new frange <%u, %u>; tot: %u\n",
			lo, hi, intf->num_free_ranges));
		/* dump_interfaces(); */
		if((free_idx = find_free_range(
				intf->free_ranges[spray_w], 
					MAX_FREE_RANGES)) < 0)  {
			dprintf(0, ("find_free_range: RANGE FULL\n"));
			return BUFX_NO_SPACE;
		}
		
		free = &(intf->free_ranges[spray_w][free_idx]);
		free->low = lo;
		free->high = hi;
		free->num_bufx = hi - lo + 1;
		(intf->num_free_ranges[spray_w])++;
		dprintf(10, ("Added new frange <%u, %u: %u>; lo %u, hi %u, tot %u\n",
			free->low, free->high, free->num_bufx,
			lo, hi, intf->num_free_ranges[spray_w]));
		/* dump_interfaces(); */
	}


	return 0;
}


static int
remove_bufx(st_bufx_if_info_t *intf, uint bufx_base, uint num_bufs,
					uint cookie, uint spray_w)
{
	ulong_t		gid = 0, pid = 0;
	st_bufx_ctl_t	*entry_to_remove;
	uint		lo = bufx_base, hi = bufx_base + num_bufs - 1;
	int		status;

	if(drv_getparm(PPGRP, &gid) == -1)  {
		dprintf(5, ("Couldn't get group-id at removing bufx\n"));
	}
	if(drv_getparm(PPID, &pid) == -1)  {
		dprintf(5, ("Couldn't get pid at removing bufx\n"));
	}

	ASSERT(bufx_ctl_head != NULL);
	ASSERT(interfaces != NULL);

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error before in remove_bufx "
			"intf 0x%x, base %u, num %u\n",
			intf, bufx_base, num_bufs);
	}
	
	entry_to_remove = &(bufx_ctl_head[cookie]);
	if(entry_to_remove->base_bufx != bufx_base)  {
		dprintf(0, ("Mismatch in base_bufx in freeing bufxes\n"));
		return BUFX_BAD_BUFX;
	}

	if(entry_to_remove->num_bufxes != num_bufs)  {
		dprintf(0, ("Trying to remove partial (%d bufs out of %d)\n",
			num_bufs, entry_to_remove->num_bufxes));
		return BUFX_BAD_BUFX;
	}

	if(entry_to_remove->buf_paddrs)  {
		dprintf(0, ("Trying to free bufx before unmapping\n"));
		return BUFX_NOT_UNMAPPED;
	}
	
	if(pid && entry_to_remove->pid != pid)  {
		dprintf(0, ("bufx: freeing process (%d) different from allocating (%d)\n",
			pid, entry_to_remove->pid));
	}

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error before add_free_to_intf "
			"intf 0x%x, lo %u, hi %u\n",
			intf, lo, hi);
	}

	if(status = add_free_to_intf(intf, lo, hi, spray_w))   {
		dprintf(0, ("Problem %d in adding free entry\n",
			status));
		dump_interfaces();
		return BUFX_BAD_BUFX;		/* generic error! */
	}

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error after add_free_to_intf "
			"intf 0x%x, lo %u, hi %u\n",
			intf, lo, hi);
	}

	ASSERT(entry_to_remove->num_bufxes);
	ASSERT(entry_to_remove->buf_uaddrs);
	kmem_free(entry_to_remove->buf_uaddrs, 
		entry_to_remove->num_bufxes * sizeof(void *));
	entry_to_remove->buf_uaddrs = NULL;

	ASSERT(entry_to_remove->buf_ulengths);
	kmem_free(entry_to_remove->buf_ulengths, 
		entry_to_remove->num_bufxes * sizeof(size_t));
	entry_to_remove->buf_ulengths = NULL;

	ASSERT(entry_to_remove->dma_cookies);
	kmem_free(entry_to_remove->dma_cookies, 
		entry_to_remove->num_bufxes * sizeof(opaque_t));
	entry_to_remove->dma_cookies = NULL;

	bzero(entry_to_remove, sizeof(st_bufx_ctl_t));
	num_bufx_entries--;

	if(sanity_check_interfaces(spray_w))  {
		cmn_err(CE_PANIC, "Intf. error seen in remove_bufx "
			"intf 0x%x, base %u, num %u\n",
			intf, bufx_base, num_bufs);
	}

	return 0;
}



int
st_bufx_free(uint bufx_base, uint num_bufx, 
			struct ifnet **ifs, uint num_ifs, uint cookie)
{
	int			error = BUFX_SUCCESS;
	int			i;
	st_bufx_if_info_t	*intf;
	uint			spray_width;

	dprintf(30, ("st_bufx_free: base %u, num %u, num_ifs %u\n",
		bufx_base, num_bufx, num_ifs));

	spray_width = bufx_to_spray_width(bufx_base);
	ASSERT(spray_width != -1);

	LOCK(interfaces_mutex);
	LOCK(bufx_ctl_mutex);
	if(error = check_refcnt()) {
		goto quit_st_bufx_free;
	}

	if(sanity_check_interfaces(spray_width))  {
		cmn_err(CE_PANIC, "Intf. error seen before st_bufx_free "
			"base %u, num %u, num_ifs %u\n",
			bufx_base, num_bufx, num_ifs);
	}

	for(i = 0; i < num_ifs; i++)  {
		if((intf = if_was_added(ifs[i])) == NULL)  {
			error = BUFX_BAD_IF;
			goto quit_st_bufx_free;
		}
		if(error = remove_bufx(intf, bufx_base, num_bufx, 
						cookie, spray_width)) {
			dprintf(0, ("Got error of %d in remove_bufx\n",
				error));
			goto quit_st_bufx_free;
		}
	}

	if(sanity_check_interfaces(spray_width))  {
		cmn_err(CE_PANIC, "Intf. error seen in st_bufx_free "
			"base %u, num %u, num_ifs %u\n",
			bufx_base, num_bufx, num_ifs);
	}

quit_st_bufx_free:
	UNLOCK(bufx_ctl_mutex);
	UNLOCK(interfaces_mutex);

	dprintf(20, ("st_bufx_free: freed %u bufxes starting at %u\n",
		num_bufx, bufx_base));

	if(! error)  {
		last_released_bufx = bufx_base;
		num_last_released = num_bufx;
	}

	return error;
}


int
st_bufx_map(uint bufx_base, iovec_t *iov, uint iovcnt,
		uint cookie, int xfer_dir, opaque_t *dma_cookies,
				uint min_frag_size, alenlist_t *alen)
{
	ulong_t		gid = 0;
	st_bufx_ctl_t	*bufx = NULL;
	alenaddr_t	addr;
	size_t		length, tot_length;
	int		error = BUFX_SUCCESS;
	uint		num_frags, num_sub_frags;
	int		i;
	paddr_t		frag_mask;
	unsigned long	tmp_kvaddr;
	

	if(! iov || ! iovcnt)  {
		dprintf(0, ("Bad iovec passed into st_bufx_map\n"));
		error = BUFX_INVALID_PARAM;
		return error;
	}
	if(drv_getparm(PPGRP, &gid) == -1)  {
		dprintf(5, ("Couldn't get group-id at st_bufx_map\n"));
	}
	if(cookie > MAX_BUFX_ENTRIES)  {
		error = BUFX_INVALID_PARAM;
		return error;
	}
	if(xfer_dir != B_READ && xfer_dir != B_WRITE)  {
		error = BUFX_INVALID_PARAM;
		return error;
	}
	if(! dma_cookies)  {
		error = BUFX_INVALID_PARAM;
		return error;
	}
	if(!(ALL_PAGE_SZ_MASK & min_frag_size)) { 
		dprintf(0, ("bufx_map: bad frag size %d\n",
			min_frag_size));
		error = BUFX_INVALID_PARAM;
		return error;
	}
	if(! alen)  {
		dprintf(0, ("bufx_map: bad alen list\n"));
		error = BUFX_INVALID_PARAM;
		return error;
	}

	dprintf(30, ("iov 0x%x, base_addr 0x%x, len %u, count %u\n",
		iov, iov->iov_base, iov->iov_len, iovcnt));

	LOCK(bufx_ctl_mutex);
	bufx = &(bufx_ctl_head[cookie]);
	
#if 0
	dump_bufx(bufx);
#endif /* 0 */

	if(bufx && bufx->gid != gid || bufx->base_bufx != bufx_base)  {
		error = BUFX_INVALID_PARAM;
		goto quit_st_bufx_map;
	}

	for(i = 0; i < iovcnt; i++)  {
		bufx->buf_uaddrs[i] = iov[i].iov_base;
		bufx->buf_ulengths[i] = iov[i].iov_len;
		bufx->dma_cookies[i] = dma_cookies[i];
	}

	ASSERT_ALWAYS(ALENLIST_SUCCESS == alenlist_cursor_init(
						*alen, 0, NULL));
	num_frags = 0;
	while(ALENLIST_SUCCESS == (alenlist_get(*alen, NULL, 0, 
						&addr, &length, 0))) {
		/* hack for f^$@ing brain-dead SHAC & s/w "spec",
		   which mandates that all frags must be of same size */
		if(length % min_frag_size 
				|| ((min_frag_size-1) & addr))  {
			num_frags += length/min_frag_size + 1;
		}
		else  {
			num_frags += length/min_frag_size;
		}
		tmp_kvaddr = PHYS_TO_K0(addr);
		dprintf(20, 
			("paddr 0x%x, length %u, nasid %u, nfrags %u\n",
			addr, length, kvatonasid(tmp_kvaddr), 
			num_frags));
	}

	bufx->num_frags = num_frags;
	ASSERT(bufx->num_bufxes >= bufx->num_frags);

	bufx->min_frag_size = min_frag_size;
	/* ASSERT(NBPP == bufx->min_frag_size); */
	if(NBPP < bufx->min_frag_size)  {
		dprintf(30, ("Note: min-frag > 16K\n"));
	}

	if(bufx->num_bufxes < bufx->num_frags) {
		cmn_err(CE_WARN, "st_bufx_map: Reserved %u bufxes, using %u in map\n",
			bufx->num_bufxes, bufx->num_frags); 
#if 0
		error = BUFX_BAD_BUFNUM;
		goto quit_st_bufx_map;
#endif /* 0 */
	}
	ASSERT(NULL == bufx->buf_paddrs);
	ASSERT(NULL == bufx->buf_lengths);
	bufx->buf_paddrs = kmem_zalloc(
			bufx->num_frags * sizeof(paddr_t), KM_SLEEP);
	ASSERT(bufx->buf_paddrs != NULL);
	bufx->buf_lengths = kmem_zalloc(
			bufx->num_frags * sizeof(size_t), KM_SLEEP);
	ASSERT(bufx->buf_lengths != NULL);

	num_frags = tot_length = 0;
	ASSERT_ALWAYS(ALENLIST_SUCCESS == 
			alenlist_cursor_init(*alen, 0, NULL));
			
	frag_mask = min_frag_size - 1;
	frag_mask = ~(frag_mask);
	while(ALENLIST_SUCCESS == (alenlist_get(*alen, NULL, 0, 
						&addr, &length, 0))) {
		/* hack for f^$@ing brain-dead SHAC & s/w "spec" */
		tot_length += length;
		if(length % min_frag_size
				|| ((min_frag_size-1) & addr))  {
			num_sub_frags = length/min_frag_size + 1;
		}
		else {
			num_sub_frags = length/min_frag_size;
		}
		dprintf(15, ("%d subfrags for addr 0x%x, len %u\n",
			num_sub_frags, addr, length));
		for(i = 0; i < num_sub_frags; i++) {
			bufx->buf_paddrs[num_frags] = frag_mask & addr;
			bufx->buf_lengths[num_frags] = 
					MIN(length, min_frag_size);
			dprintf(10, ("bufx %d frag %d: "
					"len %u, paddr 0x%x "
					"(of 0x%x), mask 0x%x\n",
				bufx_base, num_frags, 
				bufx->buf_lengths[num_frags],
				bufx->buf_paddrs[num_frags], addr,
				frag_mask));
			num_frags++;
			addr += min_frag_size;
		}
	}

	/* ASSERT(num_frags == bufx->num_frags); */
	if(num_frags != bufx->num_frags)  {
		cmn_err(CE_PANIC, "num frags %d, bufx->num_frags %d\n",
			num_frags, bufx->num_frags);
	}

quit_st_bufx_map:
	if(error)  {
		dprintf(0, ("error is %d in st_bufx_map\n", error));
		if(bufx->buf_paddrs)  {
			kmem_free(bufx->buf_paddrs, 
				bufx->num_frags * sizeof(paddr_t *));
			bufx->buf_paddrs = NULL;
		}
		if(bufx->buf_lengths)  {
			kmem_free(bufx->buf_lengths,
				bufx->num_frags * sizeof(size_t));
			bufx->buf_lengths = NULL;
		}
		for(i = 0; i < iovcnt; i++)  {
			dprintf(0, 
				("unmapping %d bytes, at uvaddr 0x%x\n",
				bufx->buf_ulengths[i],
				bufx->buf_uaddrs[i]));
			fast_undma(bufx->buf_uaddrs[i], 
				bufx->buf_ulengths[i],
				xfer_dir, &bufx->dma_cookies[i]);
			dprintf(0, ("Done unmapping bad bufx\n"));
		}
		bzero(&bufx->buf_uaddrs, 
				bufx->num_bufxes * sizeof(void *));
		bzero(&bufx->buf_ulengths, 
				bufx->num_bufxes * sizeof(size_t));
		bzero(&bufx->dma_cookies, 
				bufx->num_bufxes * sizeof(opaque_t));
	}
	UNLOCK(bufx_ctl_mutex);
	return error;
}


int
st_bufx_unmap(uint bufx_base, uint num_bufxes, uint cookie, int xfer_dir)
{
	ulong_t		gid = 0;
	st_bufx_ctl_t	*bufx;
	int		error = BUFX_SUCCESS;
	int		i;


	if(drv_getparm(PPGRP, &gid) == -1)  {
		dprintf(5, ("Couldn't get group-id at st_bufx_map\n"));
	}
	if(cookie > MAX_BUFX_ENTRIES)  {
		cmn_err(CE_WARN, "Bad cookie %u in st_bufx_unmap\n");
		return BUFX_INVALID_PARAM;
	}
	if(xfer_dir != B_READ && xfer_dir != B_WRITE)  {
		error = BUFX_INVALID_PARAM;
		return error;
	}

	dprintf(30, ("unmap: gid %lu, bufx_base %u, num_bufxes %u, cookie %u\n",
		gid, bufx_base, num_bufxes, cookie));


	LOCK(bufx_ctl_mutex);

	bufx = &(bufx_ctl_head[cookie]);
	if((gid && bufx->gid != gid) || bufx->base_bufx != bufx_base 
		|| bufx->num_bufxes != num_bufxes)  {
		cmn_err(CE_WARN, "Bad parameters in st_bufx_unmap\n");
		dprintf(0, ("tab parms: gid %u, base_bufx %u, num_bufxes %u\n",
			bufx->gid, bufx->base_bufx, bufx->num_bufxes));
		dprintf(0, ("Calling parms: gid %u, base_bufx %u, num_bufxes %u, cookie %u\n",
			gid, bufx_base, num_bufxes, cookie));

		error = BUFX_INVALID_PARAM;
		goto quit_st_bufx_unmap;
	}

	if(bufx->num_bufxes && IS_KUSEG(bufx->buf_uaddrs[0]))  {
		for(i = 0; i < bufx->num_bufxes; i++) 	{
			dprintf(30, 
				("unmapping len %d, @ uvaddr 0x%x\n",
				bufx->buf_ulengths[i],
				bufx->buf_uaddrs[i]));
			fast_undma(bufx->buf_uaddrs[i], 
					bufx->buf_ulengths[i], 
					xfer_dir,
					&bufx->dma_cookies[i]);
		}
	}

	kmem_free(bufx->buf_paddrs, 
				bufx->num_frags * sizeof(paddr_t *));
	bufx->buf_paddrs = NULL;

	kmem_free(bufx->buf_lengths, 
		                bufx->num_frags * sizeof(size_t));
	bufx->buf_lengths = NULL;

	bzero(bufx->buf_uaddrs, bufx->num_bufxes * sizeof(void *));
	bzero(bufx->buf_ulengths, bufx->num_bufxes * sizeof(size_t *));
	bzero(bufx->dma_cookies, bufx->num_bufxes * sizeof(opaque_t));

	dprintf(5, ("st_bufx_unmap refcnt needs thought!\n"));

quit_st_bufx_unmap:
	UNLOCK(bufx_ctl_mutex);
	return error;
}


int
st_bufx_to_nfrags(u_int32_t bufx, uint cookie)
{
	int		retval = 0;
	st_bufx_ctl_t	*bufx_ptr;

	if(cookie > MAX_BUFX_ENTRIES)  {
		return retval;
	}

	LOCK(bufx_ctl_mutex);
	bufx_ptr = &(bufx_ctl_head[cookie]);

	if(bufx_ptr->base_bufx == bufx) {
		retval = bufx_ptr->num_frags;
	}
	else {
		dprintf(0, ("st_buf_to_phys: cookie-bufx mismatch "
			" cookie: %u, bufx %u and %u\n",
			cookie, bufx, bufx_ptr->base_bufx));
	}

	UNLOCK(bufx_ctl_mutex);
	return retval;
}


int
st_bufx_to_phys(paddr_t	*addrs, u_int32_t bufx, uint cookie)
{
	st_bufx_ctl_t	*bufx_ptr;
	int		error = 0;

	if(cookie > MAX_BUFX_ENTRIES)  {
		return BUFX_INVALID_PARAM;
	}

	LOCK(bufx_ctl_mutex);
	bufx_ptr = &(bufx_ctl_head[cookie]);

	if(bufx_ptr->base_bufx == bufx) {
		if(bufx_ptr->num_frags)  {
			bcopy(bufx_ptr->buf_paddrs, addrs, 
					bufx_ptr->num_frags 
						* sizeof(paddr_t *));
		}
		else {
			dprintf(0, ("st_buf_to_phys: num frags %u\n",
				bufx_ptr->num_frags));
			error = BUFX_NO_SPACE;
		}
	}
	else {
		dprintf(0, ("st_buf_to_phys: cookie-bufx mismatch "
			" cookie: %u, bufx %u and %u\n",
			cookie, bufx, bufx_ptr->base_bufx));
		error = BUFX_BAD_BUFX;
	}

	UNLOCK(bufx_ctl_mutex);
	return error;
}


void
print_btop(u_int32_t bufx, uint cookie)
{
	st_bufx_ctl_t	*bufx_ptr;
	int		i;

	LOCK(bufx_ctl_mutex);
	bufx_ptr = &(bufx_ctl_head[cookie]);

	if(bufx_ptr->base_bufx == bufx) {
		if(bufx_ptr->num_frags)  {
			for(i = 0; i < bufx_ptr->num_frags; i++)  {
				printf("frag %d: paddr 0x%x, len %u\n",
					i, bufx_ptr->buf_paddrs[i],
					bufx_ptr->buf_lengths[i]);
			}
		}
		else {
			printf("st_buf_to_phys: no frags %u\n",
				bufx_ptr->num_frags);
		}
	}
	else {
		printf("Bad cookie: %u (bufx %u and %u)\n",
			cookie, bufx, bufx_ptr->base_bufx);
	}

	UNLOCK(bufx_ctl_mutex);
}


int
st_len_to_bufxnum(u_char spray, size_t len, u_int32_t bufx, uint cookie)
{
	size_t		cum_len = 0;
	st_bufx_ctl_t	*bufx_ptr;
	int		i, retval = -1;

	if(cookie > MAX_BUFX_ENTRIES)  {
		return retval;
	}

	LOCK(bufx_ctl_mutex);
	bufx_ptr = &(bufx_ctl_head[cookie]);

	if(bufx_ptr->base_bufx != bufx) {
		dprintf(0, ("Bad cookie: %u (bufx %u and %u)\n",
			cookie, bufx, bufx_ptr->base_bufx));
		goto quit_len_to_bufxnum;
	}

	for(i = 0; i < bufx_ptr->num_frags && cum_len <= len; i++) {
		cum_len += spray * bufx_ptr->buf_lengths[i];
	}
	ASSERT(cum_len >= len);
	if(cum_len > len)  {
		retval = bufx_ptr->base_bufx + i - 1;
	}
	else if(cum_len == len)  {
		ASSERT_ALWAYS(0 == cum_len 
					&& 0 == bufx_ptr->num_frags);
		retval = bufx_ptr->base_bufx;
	}

	dprintf(10, ("Returning 0x%x from len_to_bufxnum (all OK)\n",
		retval));

quit_len_to_bufxnum:
	UNLOCK(bufx_ctl_mutex);
	return	retval;
}


int
st_bufx_to_frag_size(u_int32_t bufx, uint cookie)
{
	st_bufx_ctl_t	*bufx_ptr;
	int		retval = -1;

	if(cookie > MAX_BUFX_ENTRIES)  {
		return retval;
	}

	LOCK(bufx_ctl_mutex);
	bufx_ptr = &(bufx_ctl_head[cookie]);

	if(bufx_ptr->base_bufx != bufx) {
		dprintf(0, ("Bad cookie: %u (bufx %u and %u)\n",
			cookie, bufx, bufx_ptr->base_bufx));
		goto quit_bufx_to_frag_size;
	}

	ASSERT(bufx_ptr->min_frag_size > 0);
	retval = bufx_ptr->min_frag_size;

quit_bufx_to_frag_size:
	UNLOCK(bufx_ctl_mutex);
	return	retval;
}
