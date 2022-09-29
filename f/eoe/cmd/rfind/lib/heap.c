#include <sys/param.h>
#include <sys/immu.h>
#include <sys/syssgi.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include "fsdump.h"

/*
 *  PtoD:
 *	converts Pointer to heap base (e.g. &mhnew.hp_inum) TO that heaps Descriptor (des_t *)
 *	Cheats: assumes that des_t is right after pun_t.  See struct memory_heap layout.
 */

#define PtoD(hpp) ((des_t *)((pun_t *)(hpp) + 1))

/*
 * Heap management routines:
 *	roundup	-- rounds up the amount to allocate to a heap by 1 to 2 pages.
 *	init	-- calloc space for estimated size of heap and setup heap struct
 *	reinit	-- realloc space for a larger size heap
 *	finit	-- initialize memory heap from mmap'd file
 *	alloc	-- get space from within heap, adjust heap struct
 *	realloc	-- grow item on heap, perhaps allocating new space, copying over
 *	shrink	-- just moves top down to specified value - within current space.
 *	bsize	-- return size in bytes of portion of heap alloc'd so far.
 *	btop	-- return total number of bytes (alloc'd or not) on heap.
 *	nsize	-- return size in number of allocated elements of heap.
 *	ntop	-- return total number of elements (alloc'd or not) on heap.
 */
#ifdef SGI_CONST

static struct pageconst pageconst;

/*
 * These defines have been added based on the use of
 * the following pagesize-dependent macros:
 *
 *	ctob
 *	dtop
 */
# define	_PAGESZ		pageconst.p_pagesz
# define	PNUMSHFT	pageconst.p_pnumshft

#endif /* SGI_CONST */

static index heaproundup (index nel, size_t sz) {
	size_t nbytes = nel * sz;
	size_t npgs;

#ifdef SGI_CONST
	if (!pageconst.p_pagesz) {
		if (syssgi(SGI_CONST, SGICONST_PAGESZ, &pageconst,
				sizeof(pageconst), 0) == -1) {
			perror("syssgi: pagesz");
			exit(1);
		}	
	}
#endif

	npgs = btoc(nbytes);

	npgs += 1;
	nbytes = ctob (npgs);
	return nbytes / sz;
}

void heapinit (void **hpp, index top, size_t sz) {
	des_t *dp = PtoD (hpp);

	top = heaproundup (top, sz);
	if ((*hpp = calloc (top, sz)) == NULL) {
		printf ("heapinit: no memory : % of size %d\n", top, sz);
		error ("no memory");
	}
	dp->hd_elsz = sz;
	dp->hd_next = 0;
	dp->hd_top  = top;
	dp->hd_mflg = MALC;
}

static void heapreinit (void **hpp, index oldtop, index newtop) {
	des_t *dp = PtoD (hpp);
	size_t sz = dp->hd_elsz;
	size_t nbytes;
	void *oldhp;

	newtop = heaproundup (newtop, sz);
	nbytes = newtop * sz;
	if (dp->hd_mflg == MALC) {
		oldhp = *hpp;
		if ((*hpp = realloc (*hpp, nbytes)) == NULL) {
			printf ("heapreinit: realloc: no memory:  size %d\n", nbytes);
			error ("no memory");
		}
	} else {
		oldhp = *hpp;
		if ((*hpp = calloc (newtop, sz)) == NULL) {
			printf ("heapreinit: calloc:  no memory: %d %d size %d\n", oldtop, newtop, sz);
			error ("no memory");
		}

		bcopy (oldhp, *hpp, oldtop*sz);
		dp->hd_mflg = MALC;
	}
	if (newtop > dp->hd_top)
		bzero ((char *)(*hpp) + sz * dp->hd_top, nbytes - sz * dp->hd_top);
	dp->hd_top = newtop;
}

void heapfinit (
	void **hpp,		/* address of heap base */
	char *mapaddr,		/* base of mmap'd buffer == base of fsdump file */
	off_t boff,		/* base offset in buffer of this heap */
	off_t toff,		/* top offset (end) in buffer of this heap */
	size_t elsz,		/* size of each element in this heap */
	size_t bsize		/* number of bytes of alloc'd elements */
) {
	des_t *dp = PtoD (hpp);

	dp->hd_elsz = elsz;
	dp->hd_next = bsize/elsz;
	dp->hd_top = (toff - boff)/elsz;
	dp->hd_mflg = MMAP;
	*hpp = mapaddr + boff;
}

index heapalloc (void **hpp, index nel) {
	des_t *dp = PtoD (hpp);
	index newndx;

	newndx = dp->hd_next;
	if (dp->hd_next + nel > dp->hd_top)
		heapreinit (hpp, dp->hd_next, dp->hd_next + nel);
	dp->hd_next += nel;
	return newndx;
}

index heaprealloc (void **hpp, index oldndx, index oldnel, index incrnel) {
	des_t *dp = PtoD (hpp);
	index newndx;

	if (oldndx + oldnel == dp->hd_next) {
		if (dp->hd_next + incrnel > dp->hd_top)
			heapreinit (hpp, dp->hd_next, dp->hd_next + incrnel);
		newndx = oldndx;
		dp->hd_next += incrnel;
	} else {
		size_t sz = dp->hd_elsz;
		void *oldbp;
		void *newbp;

		if (dp->hd_next + oldnel + incrnel > dp->hd_top)
			heapreinit (hpp, dp->hd_next, dp->hd_next + oldnel + incrnel);
		newndx = dp->hd_next;
		dp->hd_next += oldnel + incrnel;

		oldbp = (void *)(*(char **)hpp + sz * oldndx);
		newbp = (void *)(*(char **)hpp + sz * newndx);
		if (oldnel == 1 && sz == sizeof(off_t)) {
			*(off_t *)newbp = *(off_t *)oldbp;
			*(off_t *)oldbp = NULL;
		} else if (oldnel != 0) {
			bcopy (oldbp, newbp, sz*oldnel);
			bzero (oldbp, sz*oldnel);
		}
	}
	return newndx;
}

void heapshrink (void **hpp, index nel) {
	des_t *dp = PtoD (hpp);
	dp->hd_next = dp->hd_top = nel;
	return;
}

size_t heapbsize (void **hpp) {			/* allocated heap byte size */
	des_t *dp = PtoD (hpp);
	return dp->hd_elsz * dp->hd_next;
}

static size_t heapbtop (void **hpp) {			/* total heap byte size */
	des_t *dp = PtoD (hpp);
	return dp->hd_elsz * dp->hd_top;
}

index heapnsize (void **hpp) {			/* number allocated elements in heap */
	des_t *dp = PtoD (hpp);
	return dp->hd_next;
}

index heapntop (void **hpp) {			/* total number elements available */
	des_t *dp = PtoD (hpp);
	return dp->hd_top;
}


static void xheapreinit (void **hpp, index oldtop, index newtop) {
        des_t *dp = PtoD (hpp);
        size_t sz = dp->hd_elsz;
        size_t nbytes;
        void *oldhp;
	int fd;

        newtop = heaproundup (newtop, sz);
        nbytes = newtop * sz;
	if (dp->hd_mflg != MMPA) {
		oldhp = *hpp;
		if ((fd = open ("/dev/zero", O_RDWR)) < 0) {  
        		error ("cannot open temp file");
        	}
		*hpp = mmap((void *)0,300000000,(PROT_READ|PROT_WRITE), MAP_PRIVATE|MAP_AUTOGROW|MAP_AUTORESRV, fd, 0);
		if ((int)*hpp == -1) {
			error("error in mmap autogrow");
		}
		bcopy (oldhp, *hpp, oldtop*sz);
		dp->hd_mflg = MMPA;
	}
        dp->hd_top = newtop;
}

index xheaprealloc (void **hpp, index oldndx, index oldnel, index incrnel) {
        des_t *dp = PtoD (hpp);
        index newndx;

        if (oldndx + oldnel == dp->hd_next) {
                if (dp->hd_next + incrnel > dp->hd_top)
                        xheapreinit (hpp, dp->hd_next, dp->hd_next + incrnel);
                newndx = oldndx;
                dp->hd_next += incrnel;
        } else {
                size_t sz = dp->hd_elsz;
                void *oldbp;
                void *newbp;

                if (dp->hd_next + oldnel + incrnel > dp->hd_top) {
                        xheapreinit (hpp, dp->hd_next, dp->hd_next + oldnel + incrnel);
			newbp = (void *)(*(char **)hpp + sz * (dp->hd_top - 1));
		}
                newndx = dp->hd_next;
                dp->hd_next += oldnel + incrnel;

                oldbp = (void *)(*(char **)hpp + sz * oldndx);
                newbp = (void *)(*(char **)hpp + sz * newndx);
                if (oldnel == 1 && sz == sizeof(off_t)) {
                        *(off_t *)newbp = *(off_t *)oldbp;
                        *(off_t *)oldbp = NULL;
                } else if (oldnel != 0) {
                        bcopy (oldbp, newbp, sz*oldnel);
                        bzero (oldbp, sz*oldnel);
                }
        }
        return newndx;
}
