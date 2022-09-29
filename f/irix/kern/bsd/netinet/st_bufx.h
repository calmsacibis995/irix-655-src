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
 *  Filename: st_bufx.h
 *  Description: ST protocol's "bufx" module header.
 *
 *  $Author: kaushik $
 *  $Date: 1999/04/30 21:15:17 $
 *  $Revision: 1.1 $
 *  $Source: /proj/irix6.5f/isms/irix/kern/bsd/netinet/RCS/st_bufx.h,v $
 *
 */


/* let's start with a linear list of bufx first */
#define	MAX_BUFX_ENTRIES	2048
#define	MAX_INTERFACES		11
#define	MAX_FREE_RANGES		MAX_BUFX_ENTRIES/2
#define	MAX_SPRAY_WIDTHS	(8+1)

#define	ST_BUFX_ALLOW_SEND	(1<<1)
#define	ST_BUFX_ALLOW_RECV	(1<<2)


typedef	struct st_bufx_ctl {
	ulong_t		gid, pid;
	uint		bufsize;
#	define		BUFSZ_FREE	0
	uint		tot_size;
	uint		num_bufxes;
	void		**buf_uaddrs;
	size_t		*buf_ulengths;
	opaque_t	*dma_cookies;
	uint		base_bufx;
	paddr_t		*buf_paddrs;
	size_t		*buf_lengths;
	uint		num_frags;
	size_t		min_frag_size;
} st_bufx_ctl_t;


typedef	struct st_bufx_range   {
	uint		low;		
	uint		high;
	uint		num_bufx;
} st_bufx_range_t;


typedef	struct st_bufx_if_info {
	struct	ifnet	*interface;
	uint		bufx_lo[MAX_SPRAY_WIDTHS];
	uint		bufx_hi[MAX_SPRAY_WIDTHS];
	st_bufx_range_t	free_ranges[MAX_SPRAY_WIDTHS][MAX_FREE_RANGES];	
	uint		num_free_ranges[MAX_SPRAY_WIDTHS];
} st_bufx_if_info_t;


#define	BUFX_SUCCESS		0
#define	BUFX_NO_SPACE		1
#define	BUFX_BAD_BUFNUM		2
#define	BUFX_BAD_BUFSZ		3
#define	BUFX_BAD_IF		4
#define	BUFX_BAD_SPRAY		5
#define	BUFX_BAD_BUFX		6
#define	BUFX_NOT_UNMAPPED	7
#define	BUFX_NOT_FOUND		8
#define	BUFX_RANGE_BAD_IF	9
#define	BUFX_EFAULT		10
#define	BUFX_OVER_NBPP		11
#define	BUFX_INVALID_PARAM	12		/* catch all! */

/* valid IRIX pages mask */
#define	PAGE_16K	NBPP
#define	PAGE_64K	(PAGE_16K << 2)
#define	PAGE_256K	(PAGE_64K << 2)
#define	PAGE_1M		(PAGE_256K << 2)
#define	PAGE_4M		(PAGE_1M << 2)
#define	PAGE_16M	(PAGE_4M << 2)
#define	ST_MAX_PAGE_SIZE	PAGE_16M
#define	ALL_PAGE_SZ_MASK	(PAGE_16K | PAGE_64K | PAGE_256K \
				| PAGE_1M | PAGE_4M  | PAGE_16M)



void		init_st_bufx_tab(void);
int	st_bufx_alloc(uint *, uint, uint, struct ifnet **, uint, 
							uint, uint *);
int	st_bufx_free(uint, uint, struct ifnet **, uint, uint);
int	st_bufx_map(uint, iovec_t *, uint, uint, int, 
				opaque_t *, uint, alenlist_t *);
int	st_bufx_unmap(uint, uint, uint, int );
int	st_bufx_to_nfrags(u_int32_t, uint);
int	st_bufx_to_phys(paddr_t *, u_int32_t, uint);
int	st_bufx_to_frag_size(u_int32_t, uint);
void	print_btop(u_int32_t, uint);
void	print_uio_frags(alenlist_t*);
int	st_len_to_bufxnum(u_char, size_t, u_int32_t, uint);
int	uio_to_frag_size(iovec_t *, uint, 
					alenlist_t *, int, opaque_t *);
void	idbg_dump_interfaces(void (*printf)(char *, ...));
void	idbg_dump_bufxes(void (*printf)(char *, ...));


/* test routine, to be deleted */
void	register_an_interface(struct ifnet *, st_ifnet_t *);



