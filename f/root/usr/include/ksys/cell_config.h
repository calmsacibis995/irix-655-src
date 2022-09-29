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
#ifndef	_KSYS_CELL_CONFIG_H_
#define	_KSYS_CELL_CONFIG_H_	1
#ident "$Id: cell_config.h,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Definitions relating to cell configurability..i.e. whether it's present
 * or not
 */
#if CELL_CAPABLE

static __inline void cell_never(void) {}
#pragma mips_frequency_hint NEVER cell_never

static __inline void cell_always(void) {}
#pragma mips_frequency_hint FREQUENT cell_always


#define CELL_ONLY(x)	{ if (cell_enabled) { cell_never(); x; } }
#define	CELL_NOT(x)	{ if (!cell_enabled) { cell_always(); x; } }
#define CELL_IF(a, b)   (cell_enabled ? (cell_never(), (a)) : (b))
#define CELL_IF_STMT(a, b) \
	if (cell_enabled) {\
		cell_never(); a;\
	} else {\
		b;\
	}
#define CELL_MUST(a)    (ASSERT(cell_enabled), a)
#define	CELL_ASSERT(x)	(ASSERT(!cell_enabled || (x)))

extern int cell_enabled;	/* running cellular kernel */

#else

#define CELL_ONLY(x)
#define CELL_NOT(x)	(x)
#define CELL_IF(a, b)   (b)
#define CELL_IF_STMT(a, b)  b;
#define CELL_MUST(a)    ASSERT(0)
#define	CELL_ASSERT(x)

#endif	/* CELL_CAPABLE */
#endif	/* _KSYS_CELL_CONFIG_H_ */
