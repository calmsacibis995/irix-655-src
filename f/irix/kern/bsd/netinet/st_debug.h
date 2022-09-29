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
 *  Filename: st_debug.h
 *  Description: header file for debugging routines for ST protocol.
 *
 *  $Author: kaushik $
 *  $Date: 1999/04/30 21:18:39 $
 *  $Revision: 1.2 $
 *  $Source: /proj/irix6.5f/isms/irix/kern/bsd/netinet/RCS/st_debug.h,v $
 *
 */


#ifndef __ST_DEBUG_H__
#define __ST_DEBUG_H__

#ifdef DEBUG
#define DPRINTF(x,y) ((x & iSTDebugFlags) == x) ? printf y : (void) NULL
#define DPANIC printf
#else
#define DPRINTF(x,y)
#define DPANIC panic
#endif

extern int iSTDebugFlags;

#define ST_DEBUG_NONE   	0x0
#define ST_DEBUG_TIMERS 	0x1
#define ST_DEBUG_ENTRY  	0x10
#define ST_DEBUG_SEND   	0x100
#define ST_DEBUG_RECV   	0x1000
#define ST_DEBUG_SETUP  	0x10000

#define	ST_DEBUG
#undef	ST_DUMP

#ifdef	ST_DEBUG
extern int STDebugLevel;
#define	dprintf(lvl, x)  if(STDebugLevel >= lvl) printf x
#else	/* ! ST_DEBUG */
#define dprintf(lvl, x)
#endif	/* ST_DEBUG */


#ifdef	ST_DUMP
#define	ST_HDR_DUMP
#define	ST_DUMP_HDR(x)	st_dump_hdr(x)
#define	ST_DUMP_PCB(x)	/* st_dump_pcb(x) */
#define	ST_DUMP_PAYLOAD(x, y) /* st_dump_payload(x, y) */
#else
#define	ST_DUMP_HDR(x)
#define	ST_DUMP_PCB(x)
#define	ST_DUMP_PAYLOAD(x, y)
#endif	/* DUMP */

#define	ST_DUMP_PAYLOAD_PREFIX(x)	/* st_dump_payload_prefix(x) */

char	*st_decode_opcode(unsigned short opcode);
char	*st_decode_state(unsigned short state);
char	*st_decode_flags(unsigned short flags);
void	st_dump_hdr(sthdr_t *);
void	st_dump_pcb(struct stpcb *);
void	st_dump_tx(st_tx_t *);
void	st_dump_rx(st_rx_t *);
void	st_dump_kx(st_kx_t *);
void	print_uio(uio_t *);
void	st_dump_payload(char *, uint);
void	st_dump_payload_prefix(uio_t *);
void	st_dump_bnum_tab(st_rx_t *);
void	st_dump_CTS_tab(st_rx_t *);
void	st_dump_data_CTS_tab(st_tx_t *);

#endif
