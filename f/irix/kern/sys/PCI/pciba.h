/**************************************************************************
 *									  *
 * 		 Copyright (C) 1997, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef __PCI_PCIBA_H__
#define __PCI_PCIBA_H__

#include <sys/ioccom.h>

#ident "$Revision: 1.6 $"

/* /hw/.../pci/[slot]/config accepts ioctls to read
 * and write specific registers as follows:
 *
 * "t" is the native type (char, short, uint32, uint64)
 * to read from CFG space; results will be arranged in
 * byte significance (ie. first byte from PCI is lowest
 * or last byte in result).
 *
 * "r" is the byte offset in PCI CFG space of the first
 * byte of the register (it's least significant byte,
 * in the little-endian PCI numbering). This can actually
 * be as much as 16 bits wide, and is intended to match
 * the layout of a "Type 1 Configuration Space" address:
 * the register number in the low eight bits, then three
 * bits for the function number and five bits for the
 * slot number.
 */
#define	PCIIOCCFGRD(t,r)	_IOR(0,(r),t)
#define	PCIIOCCFGWR(t,r)	_IOW(0,(r),t)

/* Some common config register access commands.
 * Use these as examples of how to construct
 * values for other registers you want to access.
 */

/* PCIIOCGETID: arg is ptr to 32-bit int,
 * returns the 32-bit ID value with VENDOR
 * in the bottom 16 bits and DEVICE in the top.
 */
#define	PCIIOCGETID		PCIIOCCFGRD(uint32_t,PCI_CFG_VENDOR_ID)

/* PCIIOCSETCMD: arg is ptr to a 16-bit short,
 * which will be written to the CMD register.
 */
#define	PCIIOCSETCMD		PCIIOCCFGWR(uint16_t,PCI_CFG_COMMAND)

/* PCIIOCGETREV: arg is ptr to an 8-bit char,
 * which will get the 8-bit revision number.
 */
#define	PCIIOCGETREV		PCIIOCCFGRD(uint8_t,PCI_CFG_REV_ID)

/* PCIIOCGETHTYPE: arg is ptr to an 8-bit char,
 * which will get the 8-bit header type.
 */
#define	PCIIOCGETHTYPE		PCIIOCCFGRD(uint8_t,PCI_CFG_HEADER_TYPE)

/* PCIIOCGETBASE(n): arg is ptr to a 32-bit int,
 * which will get the value of the BASE<n> register.
 */
#define	PCIIOCGETBASE(n)	PCIIOCCFGRD(uint32_t,PCI_CFG_BASE_ADDR(n))

/* /hw/.../pci/[slot]/intr accepts an ioctl to
 * set up user level interrupt handling as follows:
 *
 * "n" is a bitmap of which of the four PCI interrupt
 * lines are of interest, using PCIIO_INTR_LINE_[ABCD].
 */
#define	PCIIOCSETULI(n)		_IOWR(1,n,struct uliargs)
#if _KERNEL
#define PCIIOCSETULI32(n)	_IOWR(1,n,struct uliargs32)
#endif

/* /hw/.../pci/[slot]/dma accepts ioctls to allocate
 * and free physical memory for use in user-triggered
 * DMA operations.
 */
#define	PCIIOCDMAALLOC		_IOWR(0,1,uint64_t)
#define	PCIIOCDMAFREE		_IOW(0,1,uint64_t)

/* The parameter for PCIIOCDMAALLOC needs to contain
 * both the size of the request and the flag values
 * to be used in setting up the DMA.
 *
 * Any flags normally useful in pciio_dmamap
 * or pciio_dmatrans function calls can6 be used here.
 */
#define	PCIIOCDMAALLOC_REQUEST_PACK(flags,size)		\
	((((uint64_t)(flags))<<32)|			\
	 (((uint64_t)(size))&0xFFFFFFFF))

#endif	/* __PCI_PCIBA_H__ */
