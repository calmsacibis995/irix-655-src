/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef	_KSYS_CELL_H_
#define	_KSYS_CELL_H_	1
#ident "$Id: cell.h,v 1.43 1999/05/14 20:13:13 lord Exp $"

/*
 * Definitions relating to cells and their identities.
 *
 * Lots of stuff is for UNIKERNEL - running simulated cells
 * within a single kernel. A UNIKERNEL system can partition memory
 * enough to provide administrative shutdown.
 * In UNIKERNEL mode, threads 'shuttle' between cells - they need
 *	not actually context switch, they simply need to change
 *	their cellid.
 *
 * Dynamic memory all defaults to allocating virtual space from a
 * particular cell and physical space from the same cell/NUMA-node.
 */

#include <ksys/kern_heap.h>
#include <ksys/partition.h>
#include <stdarg.h>

/*
 * This is the maximum number of cells this kernel can connect to -
 * There are various things that contribute to this maximum - partitioning
 * of 32 bit id's (pids, shm, etc), token data structures, etc.
 */
#define MAX_CELLS	64

/*
 * This is the maximum cell number - not to be confused with the number of
 * cells. The max cell number might be substantially larger the number of
 * cells.
 */
#define MAX_CELL_NUMBER	(MAX_CELLS-1)

/*
 * Externs.
 */
extern cell_t	my_cellid;		/* returned by cellid() */

extern void	cell_backoff(int);	

#if CELL_CAPABLE
#include "ksys/cell/cell_set.h"
#define CELLBIT(cell)		(((__uint64_t)1)<<(cell))
#define CELLID()		my_cellid
#if EVEREST
#define PARTID_TO_CELLID(p)	(p)
#define CELLID_TO_PARTID(c)	(c)
#else
#define PARTID_TO_CELLID(p)	(partid_to_cellid(p))
#define CELLID_TO_PARTID(c)	(cellid_to_partid(c))
#endif

/*
 * All subcell partition ids start from 32 so that they don't clash with
 * real partition ids. Part ids cannot be > 64
 */
#define	PART_SUBCELL_OFFSET	32

#define	GET_CELL_TRANSPORT(cell)	cell_transport[(cell)]

extern	partid_t	base_part; /* Base partitionid for subcell system */
extern	int		max_subcells; /* Max number of sub cells */
extern	partid_t	part_subcell_offset;
extern	int		cell_transport[];
extern void	cell_array_preinit(void);
extern void	cell_array_init(void);
extern int      cell_array_init_done(void);
extern void	cellid_init(void);
extern cell_t	cell_configured_cellid(void);
extern void	cell_failure(cell_t, int);
extern void	cell_up(cell_t, int);
extern void	cell_disconnect(cell_t);
extern void	cell_recovery(cell_t);
extern void	cell_get_connectivity_set(cell_set_t *, cell_set_t *);
extern void	cell_test_connectivity(cell_t);
extern void	part_config_add(partid_t, cell_t);

extern cell_t	partid_to_cellid(partid_t);
extern partid_t	cellid_to_partid(cell_t);
extern clusterid_t	clusterid(void);
#endif /* CELL_CAPABLE */

/*
 * Well-known cells.
 */
#define CELL_LOCAL	my_cellid	/* this cell */
#define CELL_NONE	-1		/* no cell */
#define CELL_ANY	-2		/* any one cell */
#define CELL_ALL	-3		/* every cell */



#endif	/* _KSYS_CELL_H_ */
