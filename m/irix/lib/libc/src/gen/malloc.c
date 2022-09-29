/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/malloc.c	1.31"

/**********************************************************************
	Memory management: malloc(), realloc(), free().

	The following #-parameters may be redefined:
	SEGMENTED: if defined, memory requests are assumed to be
		non-contiguous across calls of GETCORE's.
	GETCORE: a function to get more core memory. If not SEGMENTED,
		GETCORE(0) is assumed to return the next available
		address. Default is 'sbrk'.
	ERRCORE: the error code as returned by GETCORE.
		Default is ((char *)(-1)).

	This algorithm is based on a  best fit strategy with lists of
	free elts maintained in a self-adjusting binary tree. Each list
	contains all elts of the same size. The tree is ordered by size.
	For results on self-adjusting trees, see the paper:
		Self-Adjusting Binary Trees,
		DD Sleator & RE Tarjan, JACM 1985.

	The header of a block contains the size of the data part in bytes.
	Since the size of a block is 0%4, the low two bits of the header
	are free and used as follows:

		BIT0:	1 for busy (block is in use), 0 for free.
		BIT1:	if the block is busy, this bit is 1 if the
			preceding block in contiguous memory is free.
			Otherwise, it is always 0.
**********************************************************************/

#ifdef __STDC__
	#pragma weak malloc = _malloc
	#pragma weak realloc = _realloc
	#pragma weak free = _free
#endif
#include "synonyms.h"
#include "mallint.h"
#include "malloc.h"
#include <values.h>
#include <string.h>	/* to prototype memove() */
#include <unistd.h>	/* to prototype sbrk(), getpagesize() */

#if defined(_LIBC_NOMP)
#undef __LOCK_MALLOC
#undef __UNLOCK_MALLOC
#define __LOCK_MALLOC()		(void)1
#define __UNLOCK_MALLOC()	(void)0
#endif

static TREE	*_morecore(size_t);	/* function to get more core */
static void	t_delete(reg TREE *);
static void	t_splay(reg TREE *);
static void	realfree(void *);
static void	cleanfree(void *);
static void	*__malloc(size_t);
static void	*_smalloc(size_t);
void __free(void *);

/* size for preserving free blocks until next malloc */
#define FREESIZE	(1<<5)
#define FREEMASK	FREESIZE-1

/* threshold used to double size of GETCORE calls */
#define	CORETHRESH	8

/*
 * WARNING!!!  Don't initialize any of the global variables in this file.
 * Programs that use unexec() can be come confused when reloaded and dump
 * core.  All global malloc symbols should be in the COMMON section.
 */

#ifdef _LIBC_NONSHARED
static TREE	*Root,		/* root of the free tree */
		*Bottom;	/* the last free chunk in the arena */

static char	*Baddr;		/* current high address of the arena */
static char	*Lfree;		/* last freed block with data intact */

/* list of blocks to be freed on next malloc */
static VOID	*flist[FREESIZE];

/* index of free blocks in flist % FREESIZE */
static int	freeidx;

/* default value to grow core & number of calls */
static int	coresizemask, corecalls;

/* lists of small blocks */
static TREE	*List[MINSIZE/WORDSIZE-1];
#else
static TREE	*Root = NULL,	/* root of the free tree */
		*Bottom = NULL;	/* the last free chunk in the arena */

static char	*Baddr = NULL;	/* current high address of the arena */
static char	*Lfree = NULL;	/* last freed block with data intact */

/* list of blocks to be freed on next malloc */
static VOID	*flist[FREESIZE] = { 0 };

/* index of free blocks in flist % FREESIZE */
static int	freeidx = 0;

/* default value to grow core & number of calls */
static int	coresizemask = 0, corecalls = 0;

/* lists of small blocks */
static TREE	*List[MINSIZE/WORDSIZE-1] = { 0 };
#endif


/*
**	Allocation of small blocks
*/

static VOID *	_smalloc(size)
size_t		size;
	{
	reg TREE	*tp;
	reg size_t	i;
	reg int		hb;

	/**/ ASSERT(size%WORDSIZE == 0);
	/* want to return a unique pointer on malloc(0) */
	if(size == 0)
		size = WORDSIZE;

	/* list to use */
	i = size/WORDSIZE - 1;

	/* get history bits & check free list */
	hb = (int)((long)List[i] & BITS01);
	if((List[i] == NULL) || (hb != 0))
		{
		reg TREE *np;
		reg size_t n, sz;

		/* number of blocks to get at one time */
#define NPS (WORDSIZE)
		/**/ ASSERT((size+WORDSIZE)*NPS >= MINSIZE);

		/* set # elements to create on this pass */
		n = NPS << hb;
		sz = (size+WORDSIZE)*n;

		/* get 'n' of these block types */
		if((List[i] = (TREE *) __malloc(sz)) == NULL)
			return NULL;

		/* make them into a link list */
		for(np = List[i]; n > 0; --n)
			{
			tp = np;
			SIZE(tp) = size;
			np = NEXT(tp);
			AFTER(tp) = np;
			}

		/* advance history bits and save away on end of list */
		if (++hb > BITS01)
			hb = BITS01;
		AFTER(tp) = (TREE *)(long)hb;
		}

	/* allocate from the head of the queue */
	tp = List[i];
	List[i] = AFTER(tp);
	SETBIT0(SIZE(tp));
	return DATA(tp);
	}

VOID *	_malloc(size)
size_t		size;
	{
	VOID *rv;
	__LOCK_MALLOC();
	rv = __malloc(size);
	__UNLOCK_MALLOC();
	return(rv);
	}


static VOID *	__malloc(size)
size_t		size;
	{
	reg ssize_t	n;
	reg TREE	*tp, *sp;
	reg size_t	o_bit1;

	/**/ COUNT(nmalloc);
	/**/ ASSERT(WORDSIZE == ALIGN);

	/* make sure that size is 0 mod ALIGN */
	ROUND(size);

	/* see if the last free block can be used */
	if (Lfree)
		{
		sp = BLOCK(Lfree);
		n = (ssize_t)SIZE(sp);
		CLRBITS01(n);
		if (n == size)
			{	/* exact match, use it as is */
			freeidx = (freeidx + FREESIZE - 1) & FREEMASK; /* one back */
			flist[freeidx] = Lfree = NULL;
			return DATA(sp);
			}
		else if (size >= MINSIZE && n > size)
			{ 
			/* got a big enough piece */
			freeidx = (freeidx + FREESIZE - 1) & FREEMASK; /* one back */
			flist[freeidx] = Lfree = NULL;
			o_bit1 = SIZE(sp) & BIT1;
			SIZE(sp) = n;
			goto leftover;
			}
		}
		o_bit1 = 0;

	/* perform free's of space since last malloc */
	cleanfree(NULL);

	/* small blocks */
	if(size < MINSIZE)
		return _smalloc(size);

	/* search for an elt of the right size */
	sp = NULL;
	n  = 0;
	if(Root)
		{
		tp = Root;
		for (;;)
			{
			/* branch left */
			if(SIZE(tp) >= size)
				{
				if(n == 0 || n >= SIZE(tp))
					{
					sp = tp;
					n = (ssize_t)SIZE(tp);
					}
				if(LEFT(tp))
					tp = LEFT(tp);
				else	break;
				}
			else	{ /* branch right */
				if(RIGHT(tp))
					tp = RIGHT(tp);
				else	break;
				}
			}

		if(sp)	{
			t_delete(sp);
			}
		else if(tp != Root)
			{
			/* make the searched-to element the root */
			t_splay(tp);
			Root = tp;
			}
		}

	/* if found none fitted in the tree */
	if(!sp)	{
		if(Bottom && size <= SIZE(Bottom))
			sp = Bottom;
		else if((sp = _morecore(size)) == NULL) { /* no more memory */
			return NULL;
			}
		}

	/* tell the forward neighbor that we're busy */
	CLRBIT1(SIZE(NEXT(sp)));

	/**/ ASSERT(ISBIT0(SIZE(NEXT(sp))) || SIZE(NEXT(sp)) < MINSIZE);


leftover:
	/* if the leftover is enough for a new free piece */
	if((n = (ssize_t)(SIZE(sp) - size)) >= MINSIZE + WORDSIZE)
		{
		n -= WORDSIZE;
		SIZE(sp) = size;
		tp = NEXT(sp);
		SIZE(tp) = n|BIT0;
		realfree(DATA(tp));
		}
	else if(BOTTOM(sp))
		Bottom = NULL;

	/* return the allocated space */
	SIZE(sp) |= BIT0 | o_bit1;
	return DATA(sp);
	}


/*
**	realloc().
**	If the block size is increasing, we try forward merging first.
**	This is not best-fit but it avoids some data recopying.
*/
VOID *	_realloc(old,size)
VOID		*old;
size_t 		size;
	{
	reg TREE	*tp, *np;
	reg size_t	ts;
	reg char	*new;

	/**/ COUNT(nrealloc);

	/* pointer to the block */
	if(old == NULL)
		return malloc(size);
	__LOCK_MALLOC();

	/* perform free's of space since last malloc */
	cleanfree(old);

	/* make sure that size is 0 mod ALIGN */
	ROUND(size);

	tp = BLOCK(old);
	ts = SIZE(tp);

	/* if the block was freed, data has been destroyed. */
	if(!ISBIT0(ts)) {
		__UNLOCK_MALLOC();
		return NULL;
	}

	/* nothing to do */
	CLRBITS01(SIZE(tp));
	if(size == SIZE(tp))
		{
		SIZE(tp) = ts;
		__UNLOCK_MALLOC();
		return old;
		}

	/* special cases involving small blocks */
	if(size < MINSIZE || SIZE(tp) < MINSIZE)
		goto call_malloc;

	/* block is increasing in size, try merging the next block */
	if(size > SIZE(tp))
		{
		np = NEXT(tp);
		if(!ISBIT0(SIZE(np)) && SIZE(np) >= MINSIZE)
			{
			/**/ ASSERT(!ISBIT1(SIZE(np)));
			SIZE(tp) += SIZE(np)+WORDSIZE;
			if(np != Bottom)
				t_delete(np);
			else	Bottom = NULL;
			CLRBIT1(SIZE(NEXT(np)));
			}

#ifndef SEGMENTED
		/* not enough & at TRUE end of memory, try extending core */
		if(size > SIZE(tp) && BOTTOM(tp) && (char *)GETCORE(0) == Baddr)
			{
			Bottom = tp;
			if((tp = _morecore(size)) == NULL)
				tp = Bottom;
				Bottom = 0;
			}
#endif /*!SEGMENTED*/
		}

	/* got enough space to use */
	if(size <= SIZE(tp))
		{
		reg ssize_t n;

chop_big:;
		if((n = (ssize_t)(SIZE(tp) - size)) >= MINSIZE + WORDSIZE)
			{
			n -= WORDSIZE;
			SIZE(tp) = size;
			np = NEXT(tp);
			SIZE(np) = n|BIT0;
			realfree(DATA(np));
			}
		else if(BOTTOM(tp))
			Bottom = NULL;

		/* the previous block may be free */
		SETOLD01(SIZE(tp), ts);
		__UNLOCK_MALLOC();
		return old;
		}

	/* call malloc to get a new block */
call_malloc:;
		SETOLD01(SIZE(tp), ts);
		if((new = __malloc(size)) != NULL)
			{
			CLRBITS01(ts);
			if(ts > size)
				ts = size;
			MEMCOPY(new, old, ts);
			__free(old);
			__UNLOCK_MALLOC();
			return new;
			}

		/*
		** Attempt special case recovery allocations since malloc() failed:
		**
		** 1. size <= SIZE(tp) < MINSIZE
		**	Simply return the existing block
		** 2. SIZE(tp) < size < MINSIZE
		**	malloc() may have failed to allocate the chunk of
		**	small blocks. Try asking for MINSIZE bytes.
		** 3. size < MINSIZE <= SIZE(tp)
		**	malloc() may have failed as with 2.  Change to
		**	MINSIZE allocation which is taken from the beginning
		**	of the current block.
		** 4. MINSIZE <= SIZE(tp) < size
		**	If the previous block is free and the combination of
		**	these two blocks has at least size bytes, then merge
		**	the two blocks copying the existing contents backwards.
		*/

		CLRBITS01(SIZE(tp));
		if(SIZE(tp) < MINSIZE)
			{
			if(size < SIZE(tp))		/* case 1. */
				{
				SETOLD01(SIZE(tp), ts);
				__UNLOCK_MALLOC();
				return old;
				}
			else if(size < MINSIZE)		/* case 2. */
				{
				size = MINSIZE;
				goto call_malloc;
				}
			}
		else if(size < MINSIZE)			/* case 3. */
			{
			size = MINSIZE;
			goto chop_big;
			}
		else if(ISBIT1(ts) && (SIZE(LAST(tp))+SIZE(tp)+WORDSIZE) >= size && SIZE(np=LAST(tp)) >= MINSIZE)
			{
			/**/ ASSERT(!ISBIT0(SIZE(np)));
			t_delete(np);
			SIZE(np) += SIZE(tp) + WORDSIZE;
			/*
			** Since the copy may overlap, use memmove() if available.
			** Otherwise, copy by hand.
			*/
#ifdef __STDC__
			(void)memmove(DATA(np), old, SIZE(tp));
#else
			{
			reg WORD *src = (WORD *)old;
			reg WORD *dst = (WORD *)DATA(np);
			reg size_t  n = SIZE(tp) / WORDSIZE;

			do	{
				*dst++ = *src++;
				} while (--n > 0);
			}
#endif
			old = DATA(np);
			tp = np;
			CLRBIT1(ts);
			goto chop_big;
			}
		SETOLD01(SIZE(tp), ts);
		__UNLOCK_MALLOC();
		return NULL;
	}



/*
**	realfree().
**	Coalescing of adjacent free blocks is done first.
**	Then, the new free block is leaf-inserted into the free tree
**	without splaying. This strategy does not guarantee the amortized
**	O(nlogn) behaviour for the insert/delete/find set of operations
**	on the tree. In practice, however, free is much more infrequent
**	than malloc/realloc and the tree searches performed by these
**	functions adequately keep the tree in balance.
*/
static void	realfree(old)
VOID		*old;
	{
	reg TREE	*tp, *sp, *np;
	reg size_t	ts, size;

	/**/ COUNT(nfree);

	/* pointer to the block */
	tp = BLOCK(old);
	ts = SIZE(tp);
	if(!ISBIT0(ts))
		return;
	CLRBITS01(SIZE(tp));

	/* small block, put it in the right linked list */
	if(SIZE(tp) < MINSIZE)
		{
		/**/ ASSERT(SIZE(tp)/WORDSIZE >= 1);
		ts = SIZE(tp)/WORDSIZE - 1;
		AFTER(tp) = List[ts];
		List[ts] = tp;
		return;
		}

	/* see if coalescing with next block is warranted */
	np = NEXT(tp);
	if(!ISBIT0(SIZE(np)) && SIZE(np) >= MINSIZE)
		{
		if(np != Bottom)
			t_delete(np);
		SIZE(tp) += SIZE(np)+WORDSIZE;
		}

	/* the same with the preceding block */
	if(ISBIT1(ts))
		{
		np = LAST(tp);
		if (SIZE(np) >= MINSIZE)
			{
			/**/ ASSERT(!ISBIT0(SIZE(np)));
			/**/ ASSERT(np != Bottom);
			t_delete(np);
			SIZE(np) += SIZE(tp)+WORDSIZE;
			tp = np;
			}
		}

	/* initialize tree info */
	PARENT(tp) = LEFT(tp) = RIGHT(tp) = LINKFOR(tp) = NULL;

	/* the last word of the block contains self's address */
	*(SELFP(tp)) = tp;

	/* set bottom block, or insert in the free tree */
	if(BOTTOM(tp))
		Bottom = tp;
	else	{
		/* search for the place to insert */
		if(Root)
			{
			size = SIZE(tp);
			np = Root;
			for(;;)
				{
				if(SIZE(np) > size)
					{
					if(LEFT(np))
						np = LEFT(np);
					else	{
						LEFT(np) = tp;
						PARENT(tp) = np;
						break;
						}
					}
				else if(SIZE(np) < size)
					{
					if(RIGHT(np))
						np = RIGHT(np);
					else	{
						RIGHT(np) = tp;
						PARENT(tp) = np;
						break;
						}
					}
				else	{
					if((sp = PARENT(np)) != NULL)
						{
						if(np == LEFT(sp))
							LEFT(sp) = tp;
						else	RIGHT(sp) = tp;
						PARENT(tp) = sp;
						}
					else	Root = tp;

					/* insert to head of list */
					if((sp = LEFT(np)) != NULL)
						PARENT(sp) = tp;
					LEFT(tp) = sp;

					if((sp = RIGHT(np)) != NULL)
						PARENT(sp) = tp;
					RIGHT(tp) = sp;

					/* doubly link list */
					LINKFOR(tp) = np;
					LINKBAK(np) = tp;
					SETNOTREE(np);

					break;
					}
				}
			}
		else	Root = tp;
		}

	/* tell next block that this one is free */
	SETBIT1(SIZE(NEXT(tp)));

	/**/ ASSERT(ISBIT0(SIZE(NEXT(tp))) || SIZE(NEXT(tp)) < MINSIZE);

	return;
	}



/*
**	Get more core. Gaps in memory are noted as busy blocks.
*/
static TREE *	_morecore(size)
size_t		size;
	{
	reg TREE	*tp;
	reg size_t	n, offset;
	reg char	*addr;

	/* compute new amount of memory to get */
	tp = Bottom;
	n = size + 2*WORDSIZE;
	addr = GETCORE(0);

	/* need to pad size out so that addr is aligned */
	if((((size_t)addr) % ALIGN) != 0)
		offset = ALIGN - (size_t)addr%ALIGN;
	else
		offset = 0;

#ifndef SEGMENTED
	/* if not segmented memory, what we need may be smaller */
	if(addr == Baddr)
		{
		n -= WORDSIZE;
		if(tp != NULL)
			n -= SIZE(tp);
		}
#endif /*!SEGMENTED*/

	/* setup core size and double allocation if called enough times */
	if (coresizemask == 0)
		coresizemask = getpagesize() - 1;
	if (++corecalls == CORETHRESH)
		coresizemask = ((coresizemask + 1) * 2) - 1;

	/* get a multiple of coresizemask (must be a power of 2) */
	n = ((n - 1) + coresizemask) & ~coresizemask;

#ifdef sgi
	/* since _end, the start of the malloc arena, can be unaligned, we
	   would end up calling sbrk with a non-page aligned value (which the
	   kernel rounds up to the nearest page) and then pretend that the
	   remaining bytes on that last page don't belong to us.  so, always
	   allocate all the way up to the end of the page to get every byte. */
	if (Baddr == NULL) {
		/* only need to do this when we init the arena! */
		if ((((size_t)addr + offset) & coresizemask) != 0) {
			n += (coresizemask + 1) - 
				(((size_t)addr + offset) & coresizemask);
		}
	}
#endif /* sgi */

	/* call GETCORE routine to allocate memory */
	/*
	 * First, ensure that we are asking for a positive increment
	 * of sbrk(). Malloc'ed memory never shrinks, so, anything
	 * greater than MAXINT (which is negative for signed int's) is wrong.
	 */
	if ((n + offset) > (size_t) MAXLONG)
		return NULL;
	if((addr = GETCORE((ssize_t)(n + offset))) == ERRCORE)
		return NULL;

	/* contiguous memory */
	if(addr == Baddr)
		{
		/**/ ASSERT(offset ==0);
		if(tp)
			{
			addr = ((char *)tp);
			n += SIZE(tp) + 2*WORDSIZE;
			}
		else	{
			addr = Baddr-WORDSIZE;
			n += WORDSIZE;
			}
		}
	else	addr += offset;

	/* new bottom address */
	Baddr = addr + n;

	/* new bottom block */
	tp = ((TREE *) addr);
	SIZE(tp) = n - 2*WORDSIZE;
	/**/ ASSERT((SIZE(tp)%ALIGN) == 0);

	/* reserved the last word to head any noncontiguous memory */
	SETBIT0(SIZE(NEXT(tp)));

	/* non-contiguous memory, free old bottom block */
	if(Bottom && Bottom != tp)
		{
		SETBIT0(SIZE(Bottom));
		realfree(DATA(Bottom));
		}

	return tp;
	}



/*
**	Tree rotation functions (BU: bottom-up, TD: top-down)
*/

#define LEFT1(x,y)	if((RIGHT(x) = LEFT(y))) PARENT(RIGHT(x)) = x;\
			if((PARENT(y) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(y)) = y;\
				else RIGHT(PARENT(y)) = y;\
			LEFT(y) = x; PARENT(x) = y

#define RIGHT1(x,y)	if((LEFT(x) = RIGHT(y))) PARENT(LEFT(x)) = x;\
			if((PARENT(y) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(y)) = y;\
				else RIGHT(PARENT(y)) = y;\
			RIGHT(y) = x; PARENT(x) = y

#define BULEFT2(x,y,z)	if((RIGHT(x) = LEFT(y))) PARENT(RIGHT(x)) = x;\
			if((RIGHT(y) = LEFT(z))) PARENT(RIGHT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			LEFT(z) = y; PARENT(y) = z; LEFT(y) = x; PARENT(x) = y

#define BURIGHT2(x,y,z)	if((LEFT(x) = RIGHT(y))) PARENT(LEFT(x)) = x;\
			if((LEFT(y) = RIGHT(z))) PARENT(LEFT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			RIGHT(z) = y; PARENT(y) = z; RIGHT(y) = x; PARENT(x) = y

#define TDLEFT2(x,y,z)	if((RIGHT(y) = LEFT(z))) PARENT(RIGHT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			PARENT(x) = z; LEFT(z) = x;

#define TDRIGHT2(x,y,z)	if((LEFT(y) = RIGHT(z))) PARENT(LEFT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			PARENT(x) = z; RIGHT(z) = x;



/*
**	Delete a tree element
*/
static void	t_delete(op)
reg TREE	*op;
	{
	reg TREE	*tp, *sp, *gp;

	/* if this is a non-tree node */
	if(ISNOTREE(op))
		{
		tp = LINKBAK(op);
		if((sp = LINKFOR(op)) != NULL)
			LINKBAK(sp) = tp;
		LINKFOR(tp) = sp;
		return;
		}

	/* make op the root of the tree */
	if(PARENT(op))
		t_splay(op);

	/* if this is the start of a list */
	if((tp = LINKFOR(op)) != NULL)
		{
		PARENT(tp) = NULL;
		if((sp = LEFT(op)) != NULL)
			PARENT(sp) = tp;
		LEFT(tp) = sp;

		if((sp = RIGHT(op)) != NULL)
			PARENT(sp) = tp;
		RIGHT(tp) = sp;

		Root = tp;
		return;
		}

	/* if op has a non-null left subtree */
	if((tp = LEFT(op)) != NULL )
		{
		PARENT(tp) = NULL;

		if(RIGHT(op))
			{
			/* make the right-end of the left subtree its root */
			while((sp = RIGHT(tp)) != NULL)
				{
				if((gp = RIGHT(sp)) != NULL)
					{
					TDLEFT2(tp,sp,gp);
					tp = gp;
					}
				else	{
					LEFT1(tp,sp);
					tp = sp;
					}
				}

			/* hook the right subtree of op to the above elt */
			RIGHT(tp) = RIGHT(op);
			PARENT(RIGHT(tp)) = tp;
			}
		}

	/* no left subtree */
	else if((tp = RIGHT(op)) != NULL)
		PARENT(tp) = NULL;

	Root = tp;
	}



/*
**	Bottom up splaying (simple version).
**	The basic idea is to roughly cut in half the
**	path from Root to tp and make tp the new root.
*/
static void	t_splay(tp)
reg TREE	*tp;
	{
	reg TREE	*pp, *gp;

	/* iterate until tp is the root */
	while((pp = PARENT(tp)) != NULL)
		{
		/* grandparent of tp */
		gp = PARENT(pp);

		/* x is a left child */
		if(LEFT(pp) == tp)	
			{
			if(gp && LEFT(gp) == pp)
				{
				BURIGHT2(gp,pp,tp);
				}
			else	{
				RIGHT1(pp,tp);
				}
			}
		else	{
			/**/ ASSERT(RIGHT(pp) == tp);
			if(gp && RIGHT(gp) == pp)
				{
				BULEFT2(gp,pp,tp);
				}
			else	{
				LEFT1(pp,tp);
				}
			}
		}
	} 



/*
**	free().
**	Performs a delayed free of the block pointed to
**	by old. The pointer to old is saved on a list, flist,
**	until the next malloc or realloc. At that time, all the
**	blocks pointed to in flist are actually freed via
**	realfree(). This allows the contents of free blocks to
**	remain undisturbed until the next malloc or realloc.
*/
void	_free(old)
VOID	*old;
	{
	__LOCK_MALLOC();
	__free(old);
	__UNLOCK_MALLOC();
	}

void	__free(old)
VOID	*old;
	{
	if(old == NULL)
		return;

	if(flist[freeidx] != NULL)
		realfree(flist[freeidx]);
	flist[freeidx] = Lfree = old;
	freeidx = (freeidx + 1) & FREEMASK; /* one forward */
	}



/*
**	cleanfree() frees all the blocks pointed to be flist.
**	
**	realloc() should work if it is called with a pointer
**	to a block that was freed since the last call to malloc() or
**	realloc(). If cleanfree() is called from realloc(), ptr
**	is set to the old block and that block should not be
**	freed since it is actually being reallocated.
*/
static void	cleanfree(VOID *ptr)
{
	reg char	**flp;

	flp = (char **)&(flist[freeidx]);
	for (;;)
		{
		if (flp == (char **)&(flist[0]))
			flp = (char **)&(flist[FREESIZE]);
		if (*--flp == NULL)
			break;
		if ((VOID *)*flp != ptr)
			realfree(*flp);
		*flp = NULL;
		}
	freeidx = 0;
	Lfree = NULL;
}