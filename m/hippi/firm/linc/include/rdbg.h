/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/*
 * rdbg.h
 *
 * Utilities for remote debugging the firmware.
 *
 * $Revision: 1.8 $
 */

#ifndef _RDBG_H_
#define _RDBG_H_

#include "eframe.h"

/* XXX: don't want to include signal.h */
#define	SIGINT	2
#define SIGQUIT	3
#define	SIGILL	4
#define	SIGTRAP	5
#define	SIGFPE	8
#define	SIGBUS	10

#ifdef _LANGUAGE_C

#ifdef DEBUG
#define ASSERT(EX)	((EX)?(void)0:assfail(__FILE__, __LINE__))
#else
#define ASSERT(EX)	((void)0)
#endif

#define assert(EX)	((EX)?(void)0:assfail(__FILE__, __LINE__))


/* stuff in rdbg.c, rdbg_gdb.c, and rdbg_kdbx.c */
extern u_int	badaddr, baddr_cause;
extern int	debugger_mode;
extern void	init_rdbg(eframe_t *);
extern int	read_mem_word( u_int *addr, u_int *val );
extern int	write_mem_word( u_int *addr, u_int val );
extern int	read_mem_byte( u_char *addr, u_char *val );
extern int	write_mem_byte( u_char *addr, u_char val );
extern void	printf( char *fmt, ... );
extern void	debug_exc( int signal, eframe_t *eframe );

extern void	assfail(char *, int);

#ifdef RDBGDEBUG
extern void	printf1( char *fmt, ... );
#endif

/* stuff in rdbg_brkpt.c */
extern void	initbp( void );
extern int	addbrkpt( inst_t *addr, int type );
extern void	fixup_brkpts( void );
extern int	step1( eframe_t *ep );
extern int	cont( eframe_t *ep );
extern int	brkpt_cont;

/* things in locore.s: */
extern void	breakpoint(void);

#endif /* _LANGUAGE_C */
#endif /* _RDBG_H_ */
