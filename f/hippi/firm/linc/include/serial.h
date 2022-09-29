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
 * serial.h
 *
 * $Revision: 1.5 $
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

/* size of serial input/output queues */
#define SERIAL_QSIZE		128

#define SERIAL_BAUD		9600
#define SERIAL_TMR_HZ		(5*SERIAL_BAUD)
#define SERIAL_TMR_TICK		(CPU_HZ/2/SERIAL_TMR_HZ)

#ifdef _LANGUAGE_C

extern void	serialinit(void);
extern void	serialintr(void);

extern void	serial_on(void);
extern void	serial_off(void);

extern int	PollSerial(void);
extern int	GetSerial(void);
extern void	PutSerial(char c);
#ifdef RDBGDEBUG
extern void	PutSerial1(char c);
#endif

extern int	serial_intson;

#endif /* _LANGUAGE_C */
#endif /* _SERIAL_H_ */
