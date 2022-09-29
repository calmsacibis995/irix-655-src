/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991 MIPS Computer Systems, Inc.            |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 52.227-7013.   |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Drive                                |
 * |         Sunnyvale, CA 94086                               |
 * |-----------------------------------------------------------|
 */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/reloc.h,v 7.17 1994/06/01 04:14:35 dlai Exp $ */
#ifndef __RELOC_H__
#define __RELOC_H__

#ifdef __cplusplus
extern "C" {
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifdef __mips

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#if _MIPS_SZLONG == 32
struct reloc {
    long	r_vaddr;	/* (virtual) address of reference */
    unsigned	r_symndx:24,	/* index into symbol table */
		r_reserved:2,
    		r_type:5,	/* relocation type */
		r_extern:1;	/* if 1 symndx is an index into the external
				   symbol table, else symndx is a section # */
    };
#endif	/* _MIPS_SZLONG === 32 */
#if _MIPS_SZLONG == 64
#include <sgidefs.h>
struct reloc {
    __int32_t	r_vaddr;	/* (virtual) address of reference */
    unsigned	r_symndx:24,	/* index into symbol table */
		r_reserved:2,
    		r_type:5,	/* relocation type */
		r_extern:1;	/* if 1 symndx is an index into the external
				   symbol table, else symndx is a section # */
    };
#endif	/* _MIPS_SZLONG == 64 */
#endif /*_LANGUAGE_C */

#ifdef _LANGUAGE_PASCAL
type
  reloc = packed record
      r_vaddr : long;			/* (virtual) address of reference    */
      r_symndx : 0..lshift(1, 24)-1;	/* index into symbol table	     */
      r_reserved : 0..3;
      r_type : 0..31;			/* relocation type		     */
      r_extern : 0..1;			/* if 1, symndx is an index into the */
					/* external symbol table, else	     */
					/* symndx is a section		   # */
      end {record};
#endif /* _LANGUAGE_PASCAL */

/*
 * Section numbers for symndex for local relocation entries (r_extern == 0).
 * For these entries the starting address for the section referenced by the
 * section number is used in place of an external symbol table entry's value.
 */
#define	R_SN_NULL	0
#define	R_SN_TEXT	1
#define	R_SN_RDATA	2
#define	R_SN_DATA	3
#define	R_SN_SDATA	4
#define	R_SN_SBSS	5
#define	R_SN_BSS	6
#define	R_SN_INIT	7
#define	R_SN_LIT8	8
#define	R_SN_LIT4	9
#define	R_SN_XDATA	10
#define	R_SN_PDATA	11
#define R_SN_FINI       12
#define MAX_R_SN        12

#else /* !defined(__mips) */

struct reloc {
	long	r_vaddr;	/* (virtual) address of reference */
	long	r_symndx;	/* index into symbol table */
	unsigned short	r_type;	/* relocation type */
	};
#endif /* __mips */

/*
 *   relocation types for all products and generics
 */

/*
 * All generics
 *	reloc. already performed to symbol in the same section
 */
#define  R_ABS		0

/*
 * Mips machines
 *
 *	16-bit reference
 *	32-bit reference
 *	26-bit jump reference
 *	reference to high 16-bits
 *	reference to low 16-bits
 *	reference to global pointer reletive data item
 *	reference to global pointer reletive literal pool item
 */
#define	R_REFHALF	1
#define	R_REFWORD	2
#define	R_JMPADDR	3
#define	R_REFHI		4
#define	R_REFLO		5
#define	R_GPREL		6
#define	R_LITERAL	7
#define R_REL32         8
#define R_REFGOT	R_REL32	    /* alias for compatibility */
#define R_REFHI_64      9
#define R_REFLO_64      10
#define R_REFWORD_64    11
#define R_PC16	        12
#ifdef __osf__
#define R_RELHI         13
#define R_RELLO         14
#endif /* __osf__ */
#define R_REFSHFT       15
#define R_REFHI_ADDEND  16	    /* lo value is in immed of inst */
#define R_SHFTCNT       17
#define R_MULTRELHI     18
#define R_MULTRELLO     19
#define R_DATA16        20

/* pseudo reloc type corresponding to R_MIPS_CALL16 in Elf.  MIPS does not 
   output this r_type in COFF objects. But ld converts Elf objects to COFF  
   internally during processing, and thus need an entry here to hold the  
   value of R_MIPS_CALL16. */
#define R_LGOTCALLHI	25
#define R_LGOTCALLLO	26
#define R_LGOTHI	27
#define R_LGOTLO	28
#define R_GPWORD        29      
#define R_REFGOTCALL    30

#define MAX_R_TYPE      31

/*
 * X86 generic
 *	8-bit offset reference in 8-bits
 *	8-bit offset reference in 16-bits 
 *	12-bit segment reference
 *	auxiliary relocation entry
 */
#define	R_OFF8		07
#define R_OFF16		010
#define	R_SEG12		011
#define	R_AUX		013

/*
 * B16 and X86 generics
 *	16-bit direct reference
 *	16-bit "relative" reference
 *	16-bit "indirect" (TV) reference
 */
#define  R_DIR16	01
#define  R_REL16	02
#define  R_IND16	03

/*
 * 3B generic
 *	24-bit direct reference
 *	24-bit "relative" reference
 *	16-bit optimized "indirect" TV reference
 *	24-bit "indirect" TV reference
 *	32-bit "indirect" TV reference
 */
#define  R_DIR24	04
#define  R_REL24	05
#define  R_OPT16	014
#define  R_IND24	015
#define  R_IND32	016

/*
 * 3B and M32 || u3b15 || u3b5 || u3b2 generics
 *	32-bit direct reference
 */
#define  R_DIR32	06

/*
 * M32 || u3b15 || u3b5 || u3b2 generic
 *	32-bit direct reference with bytes swapped
 */
#define  R_DIR32S	012

/*
 * DEC Processors  VAX 11/780 and VAX 11/750
 *
 */

#define R_RELBYTE	017
#define R_RELWORD	020
#define R_RELLONG	021
#define R_PCRBYTE	022
#define R_PCRWORD	023
#define R_PCRLONG	024

/*
 * Motorola 68000
 *
 * ... uses R_RELBYTE, R_RELWORD, R_RELLONG, R_PCRBYTE and R_PCRWORD as for
 * DEC machines above.
 */

#define	RELOC	struct reloc
#define	RELSZ	sizeof(RELOC)

	/* Definition of a "TV" relocation type */

#if _N3B
#define ISTVRELOC(x)	((x==R_OPT16)||(x==R_IND24)||(x==R_IND32))
#endif
#if _B16 || _X86
#define ISTVRELOC(x)	(x==R_IND16)
#endif
#if _M32 || __u3b15 || __u3b5 || __u3b2
#define ISTVRELOC(x)	(x!=x)	/* never the case */
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __RELOC_H__ */
