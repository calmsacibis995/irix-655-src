/*
 * gio2pci.h
 *
 * Header file describing Indigo2 GIO2PCI backplane
 *
 * Copyright 1995 Silicon Graphics, Inc.  All rights reserved.
 *
 */

#ifndef __GIO2PCI_H__
#define __GIO2PCI_H__

#ident "$Revision: 1.0 $"

#define NUM_PCI_SLOTS	2		/* Total number of PCI slots */

#define PCI_BYTECNT     0xbf400000 /* Byte count register */
#define PCI_CONFIG0     0xbf480000 /* PCI Slot 0 Configuration Space */
#define PCI_CONFIG1     0xbf4c0000 /* PCI Slot 1 Configuration Space */
#define PCI_MEM         0xbf500000 /* PCI Memory Space */
#define PCI_CONFIG_SIZE 256             /* Size of config space */

#ifdef _KERNEL
#if !defined(poke)
#define poke(a,v)       (volatile) *(volatile WORD *)(a) = (v)
#define peek(a)         (*(volatile WORD *)(a))
#endif
#endif

#define PEEK_PCI_CONFIG(slot,ofs) ((slot) ? peek(PCI_CONFIG1+(ofs)) : peek(PCI_CONFIG0+(ofs)))
#if 1
#define POKE_PCI_CONFIG(slot,ofs,x) if (slot) {\
					poke(PCI_CONFIG1+(ofs),(x));\
				    } else {\
					poke(PCI_CONFIG0+(ofs),(x));\
				    }
#else
#define POKE_PCI_CONFIG(slot,ofs,x) (slot) ? poke(PCI_CONFIG1+(ofs),(x)) : poke(PCI_CONFIG0+(ofs),(x))
#endif

#define PEEK_PCI_BYTECNT() peek(PCI_BYTECNT)
#define POKE_PCI_BYTECNT(x) poke(PCI_BYTECNT,(x))

#define PEEK_PCI(ofs) peek(PCI_MEM+(ofs))
#define POKE_PCI(ofs,x) poke(PCI_MEM+(ofs),(x))

#endif
