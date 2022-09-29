#ifndef __SYS_ELF_H__
#define __SYS_ELF_H__

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
 * Copyright 1992 Silicon Graphics,  Inc.
 * ALL RIGHTS RESERVED
 * 
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SGI
 * The copyright notice above does not evidence any  actual  or
 * intended  publication of this source code and material is an
 * unpublished work by Silicon  Graphics,  Inc.  This  material
 * contains CONFIDENTIAL INFORMATION that is the property and a
 * trade secret of Silicon Graphics, Inc. Any use,  duplication
 * or  disclosure  not  specifically  authorized  in writing by
 * Silicon Graphics is  strictly  prohibited.  THE  RECEIPT  OR
 * POSSESSION  OF  THIS SOURCE CODE AND/OR INFORMATION DOES NOT
 * CONVEY ANY RIGHTS TO REPRODUCE, DISCLOSE OR  DISTRIBUTE  ITS
 * CONTENTS,  OR  TO MANUFACTURE, USE, OR SELL ANYTHING THAT IT
 * MAY DESCRIBE, IN WHOLE OR IN PART.
 * 
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND
 * Use, duplication or disclosure by the Government is  subject
 * to  restrictions  as  set  forth  in  FAR 52.227.19(c)(2) or
 * subparagraph (c)(1)(ii) of the Rights in Technical Data  and
 * Computer  Software  clause  at  DFARS 252.227-7013 and/or in
 * similar or successor clauses in the FAR, or the DOD or  NASA
 * FAR  Supplement.  Unpublished  --  rights reserved under the
 * Copyright Laws of the United States. Contractor/manufacturer
 * is Silicon Graphics, Inc., 2011 N. Shoreline Blvd., Mountain
 * View, CA 94039-7311
 */

/* WARNING:  This file is included in, and logically a part of,
 * /usr/include/elf.h.  The distribution of material between them
 * is artificial, generally reflecting the distinction between generic
 * and system-dependent, and may change.  Do not include this file
 * directly -- include /usr/include/elf.h instead.
 */

#include <sys/elftypes.h> 

/* 
 * Random constants
 */

#define _TEXT_ALIGN 0x10000
#define _DATA_ALIGN 0x10000
#define ELF_MIPS_MAXPGSZ (64*1024)
#define ELF_MIPS_MINPGSZ (0x1000)

#define MS_ALIAS        0x1	/* ???? */


/* ====================================================================
 *
 * Elf header
 *
 * ====================================================================
 */

/*
 * e_type
 */

#define ET_IR	(ET_LOPROC + 0)

/* ====================================================================
 *
 * Program header
 *
 * ====================================================================
 */

/*
 * e_flags
 */

#define EF_MIPS_NOREORDER	0x00000001
#define EF_MIPS_OPSEX		EF_MIPS_NOREORDER
#define EF_MIPS_PIC		0x00000002
#define EF_MIPS_CPIC		0x00000004
#define EF_MIPS_XGOT		0x00000008
#define EF_MIPS_64BIT_WHIRL	0x00000010
#define EF_MIPS_ABI2		0x00000020 
#define EF_MIPS_OPTIONS_FIRST	0x00000080 
/* obsolete names */
#define EF_MIPS_UGEN_ALLOC	EF_MIPS_XGOT
#define EF_MIPS_UGEN_RESERVED	EF_MIPS_64BIT_WHIRL


/*
 *	The EF_MIPS_ARCH field of e_flags describes the ISA of the object.
 *		size:	4 bits
 *		type:	int
 */
#define EF_MIPS_ARCH		0xf0000000	/* mask: 4 bit field */
#define EF_MIPS_ARCH_1		0x00000000
#define EF_MIPS_ARCH_2          0x10000000
#define EF_MIPS_ARCH_3          0x20000000
#define EF_MIPS_ARCH_4          0x30000000
#define EF_MIPS_ARCH_5          0x40000000
#define EF_MIPS_ARCH_6          0x50000000

/*
 *	The EF_MIPS_ARCH_ASE field of e_flags describes the set of 
 *	Application Specific Extensions used by the object.
 *		size:	4 bits
 *		type:	bit-field
 */
#define EF_MIPS_ARCH_ASE	0x0f000000	/* mask: 4 bit field	*/
#define EF_MIPS_ARCH_ASE_MDMX	0x08000000	/* multi-media extensions*/
#define EF_MIPS_ARCH_ASE_M16	0x04000000	/* MIPS16 isa extensions */

/*
 *	Please reserve these 8 bits of e_flags for future
 *	expansion of the EF_MIPS_ARCH_ASE field;  increasing
 *	the field from 4 bits to 12 bits.
 *	
 *		0x00ff0000	
 *	
 *	If and when we expand it, we'll redefine the EF_MIPS_ARCH_ASE 
 *	macro to be:
 *	
 *		0x0fff0000.
 */



/* 
 * special Program header types
 */

#define PT_MIPS_REGINFO		(PT_LOPROC + 0)
#define PT_MIPS_RTPROC		(PT_LOPROC + 1)	/* runtime procedure table */
#define PT_MIPS_OPTIONS		(PT_LOPROC + 2)


/* 
 * special p_flags
 */

#define PF_MIPS_LOCAL		0x10000000

/* ====================================================================
 *
 * Section Headers
 *
 * ====================================================================
 */

/* 
 * Special mips section indices
 */

#define SHN_MIPS_ACOMMON	(SHN_LOPROC + 0)
#define SHN_MIPS_TEXT		(SHN_LOPROC + 1)
#define SHN_MIPS_DATA		(SHN_LOPROC + 2)
#define SHN_MIPS_SCOMMON	(SHN_LOPROC + 3)
#define SHN_MIPS_SUNDEFINED	(SHN_LOPROC + 4)
#define SHN_MIPS_LCOMMON	(SHN_LOPROC + 5)
#define SHN_MIPS_LUNDEFINED	(SHN_LOPROC + 6)


/*
 * sh_type
 */

#define SHT_MIPS_LIBLIST	(SHT_LOPROC + 0)
#define SHT_MIPS_MSYM		(SHT_LOPROC + 1)
#define SHT_MIPS_CONFLICT	(SHT_LOPROC + 2)
#define SHT_MIPS_GPTAB		(SHT_LOPROC + 3)
#define SHT_MIPS_UCODE		(SHT_LOPROC + 4)
#define SHT_MIPS_DEBUG          (SHT_LOPROC + 5)
#define SHT_MIPS_REGINFO        (SHT_LOPROC + 6)
#ifdef __osf__
#define	SHT_MIPS_PACKAGE	(SHT_LOPROC + 7)
#define	SHT_MIPS_PACKSYM	(SHT_LOPROC + 8)
#endif /* __osf__ */

#define SHT_MIPS_RELD		(SHT_LOPROC + 9)
#define SHT_MIPS_DONTUSE	(SHT_LOPROC + 10)
/* Don't  use 10 until after the ragnarok beta */
#define SHT_MIPS_IFACE		(SHT_LOPROC + 11)
#define SHT_MIPS_CONTENT	(SHT_LOPROC + 12)
#define SHT_MIPS_OPTIONS	(SHT_LOPROC + 13)

#define SHT_MIPS_SHDR		(SHT_LOPROC + 16)
#define SHT_MIPS_FDESC		(SHT_LOPROC + 17)
#define SHT_MIPS_EXTSYM		(SHT_LOPROC + 18)
#define SHT_MIPS_DENSE		(SHT_LOPROC + 19)
#define SHT_MIPS_PDESC		(SHT_LOPROC + 20)
#define SHT_MIPS_LOCSYM		(SHT_LOPROC + 21)
#define SHT_MIPS_AUXSYM		(SHT_LOPROC + 22)
#define SHT_MIPS_OPTSYM		(SHT_LOPROC + 23)
#define SHT_MIPS_LOCSTR		(SHT_LOPROC + 24)
#define SHT_MIPS_LINE		(SHT_LOPROC + 25)
#define SHT_MIPS_RFDESC		(SHT_LOPROC + 26)

#define SHT_MIPS_DELTASYM	(SHT_LOPROC + 27)
#define SHT_MIPS_DELTAINST	(SHT_LOPROC + 28)
#define SHT_MIPS_DELTACLASS	(SHT_LOPROC + 29)

#define SHT_MIPS_DWARF		(SHT_LOPROC + 30)
#define SHT_MIPS_DELTADECL	(SHT_LOPROC + 31)
#define SHT_MIPS_SYMBOL_LIB	(SHT_LOPROC + 32)
#define SHT_MIPS_EVENTS        	(SHT_LOPROC + 33)
#define SHT_MIPS_TRANSLATE     	(SHT_LOPROC + 34)
#define SHT_MIPS_PIXIE     	(SHT_LOPROC + 35)
#define SHT_MIPS_XLATE		(SHT_LOPROC + 36)
#define SHT_MIPS_XLATE_DEBUG	(SHT_LOPROC + 37)
#define SHT_MIPS_WHIRL		(SHT_LOPROC + 38)
#define SHT_MIPS_EH_REGION	(SHT_LOPROC + 39)
#define SHT_MIPS_XLATE_OLD	(SHT_LOPROC + 40)
#define SHT_MIPS_PDR_EXCEPTION	(SHT_LOPROC + 41)

#define SHT_MIPS_NUM		42 /* Number of highest type */
                                   /* SHT_MIPS_NUM is one more
                                   ** than the highest
                                   ** offset to SHT_LOPROC.
				   ** and is thus the number of 
				   ** extensions
                                   ** It is not a section type.
                                   */



/*
 * sh_flags
 */

#define SHF_MIPS_GPREL		0x10000000
#define SHF_MIPS_MERGE		0x20000000	
#define SHF_MIPS_ADDR		0x40000000
#define SHF_MIPS_STRINGS	0x80000000
#define SHF_MIPS_NOSTRIP 	0x08000000
#define SHF_MIPS_LOCAL		0x04000000
#define SHF_MIPS_NAMES		0x02000000
#define SHF_MIPS_NODUPE		0x01000000

/*
 * special section names
 */

#define MIPS_SDATA		".sdata"
#define MIPS_REL_SDATA		".rel.sdata"
#define MIPS_SRDATA		".srdata"
#define MIPS_RDATA		".rdata"
#define MIPS_SBSS		".sbss"
#define MIPS_LIT4		".lit4"
#define MIPS_LIT8		".lit8"
#define MIPS_LIT16		".lit16"
#define MIPS_REGINFO		".reginfo"
#define MIPS_LIBLIST		".liblist"
#define MIPS_MSYM		".msym"
#define MIPS_RHEADER		".rheader"
#define MIPS_CONFLICT		".conflict"
#define MIPS_GPTAB_SDATA	".gptab.sdata"
#define MIPS_GPTAB_DATA		".gptab.data"
#define MIPS_GPTAB_BSS		".gptab.bss"
#define MIPS_GPTAB_SBSS		".gptab.sbss"
#define MIPS_LBSS		".lbss"
#define MIPS_UCODE		".ucode"
#define MIPS_MDEBUG		".mdebug"
#define MIPS_COMPACT_RELOC	".compact_rel"
#ifdef __osf__
#define MIPS_PACKAGE		".package"
#define MIPS_PACKSYM		".packsym"
#endif /* __osf__ */
#define MIPS_CONTENT		".MIPS.content"
#define MIPS_EVENTS		".MIPS.events"
#define MIPS_INTERFACES		".MIPS.interfaces"
#define MIPS_OPTIONS		".MIPS.options"
#define MIPS_DELTACLASS		".MIPS.dclass"
#define MIPS_DELTASYM		".MIPS.dsym"
#define MIPS_DELTAINST		".MIPS.dinst"
#define MIPS_DELTADECL		".MIPS.ddecl"
#define MIPS_REL_DELTA		".rel.delta"
#define MIPS_SYMBOL_LIB		".MIPS.symlib"
#define MIPS_DEBUG_INFO		".debug_info"
#define MIPS_DEBUG_LINE		".debug_line"
#define MIPS_DEBUG_ABBREV	".debug_abbrev"
#define MIPS_DEBUG_FRAME	".debug_frame"
#define MIPS_DEBUG_ARANGES	".debug_aranges"
#define MIPS_DEBUG_PUBNAMES	".debug_pubnames"
#define MIPS_DEBUG_STR		".debug_str"
#define MIPS_DEBUG_FUNCNAMES	".debug_funcnames"
#define MIPS_DEBUG_TYPENAMES	".debug_typenames"
#define MIPS_DEBUG_VARNAMES	".debug_varnames"
#define MIPS_DEBUG_WEAKNAMES	".debug_weaknames"
#define MIPS_XLATE		".MIPS.Xlate"
#define MIPS_XLATE_DEBUG	".MIPS.Xlate_debug"
#define MIPS_XLATE_OLD		".MIPS.Xlate_old"
#define MIPS_WHIRL		".WHIRL"
#define MIPS_PERF_FUNCTIONS    	".MIPS.Perf_function"
#define MIPS_PERF_WEAKNAMES	".MIPS.Perf_weak_names"
#define MIPS_PERF_CALLGRAPH	".MIPS.Perf_call_graph"
#define MIPS_PERF_BBOFFSETS	".MIPS.Perf_bb_offsets"
#define MIPS_PERF_TABLE		".MIPS.Perf_table"
#define MIPS_PERF_ARGTRACE  	".MIPS.Perf_argtrace"
#define MIPS_EH_REGION		"_MIPS_eh_region"
#define MIPS_EH_REGION_SUPP	"_MIPS_eh_region_supp"
#define MIPS_PDR_EXCEPTION	".MIPS.pdr_exception"


/* ====================================================================
 *
 * Symbol table
 *
 * ====================================================================
 */

/*
 * Special mips st_other
 */
#define STO_DEFAULT		0x0
#define STO_INTERNAL		0x1
#define STO_HIDDEN		0x2
#define STO_PROTECTED		0x3
#define STO_OPTIONAL		0x4
#define STO_SC_ALIGN_UNUSED	0xff

/*
 * Special mips st_info
 */
#define STB_SPLIT_COMMON	(STB_LOPROC+0)

/* ====================================================================
 *
 * .gptab Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef union
{
	struct
	{
		Elf32_Word	gt_current_g_value;
		Elf32_Word	gt_unused;
	} gt_header;
	struct
	{
		Elf32_Word	gt_g_value;
		Elf32_Word	gt_bytes;
	} gt_entry;
} Elf32_Gptab;
#endif


/* ====================================================================
 *
 * .reginfo Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf32_Word	ri_gprmask;
	Elf32_Word	ri_cprmask[4];
	Elf32_Sword	ri_gp_value;
} Elf32_RegInfo;
#endif


/* ====================================================================
 *
 * .MIPS.options Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf64_Byte	kind;		/* determines interpretation of the */
					/* variable part of descriptor	    */
	Elf64_Byte	size;		/* size of descriptor, incl. header */
	Elf64_Section	section;	/* section header index of section  */
					/* affected, 0 for global options   */
	Elf64_Word	info;		/* Kind-specific information	    */
} Elf_Options;
#endif


/*
 * Options descriptor kinds
 */

#define ODK_NULL	0	/* Undefined */
#define ODK_REGINFO	1	/* Register usage information */
#define ODK_EXCEPTIONS	2	/* Exception processing options  */
#define ODK_PAD		3	/* Section padding options */
#define ODK_HWPATCH     4       /* Hardware workarounds performed */
#define ODK_FILL	5	/* record the fill value used by the linker */
#define ODK_TAGS	6	/* reserve space for desktop tools to write */
#define	ODK_HWAND	7	/* HW workarounds.  'AND' bits when merging */ 
#define	ODK_HWOR	8	/* HW workarounds.  'OR' bits when merging */ 
#define	ODK_GP_GROUP	9	/* GP group to use for text/data sections */ 
#define	ODK_IDENT	10	/* ID information */ 


/* Masks for the info word of an ODK_EXCEPTIONS descriptor: */
#define OEX_FPU_MIN     0x1f    /* FPE's which MUST be enabled */
#define OEX_FPU_MAX     0x1f00  /* FPE's which MAY be enabled */
#define OEX_PAGE0       0x10000 /* page zero must be mapped */
#define OEX_SMM		0x20000 /* Force sequential memory mode? */
#define OEX_FPDBUG      0x40000 /* Force floating point debug mode? */
#define OEX_PRECISEFP   OEX_FPDBUG
#define OEX_DISMISS	0x80000 /* Dismiss invalid address faults? */

#define OEX_FPU_INVAL	0x10
#define OEX_FPU_DIV0	0x08
#define OEX_FPU_OFLO	0x04
#define OEX_FPU_UFLO	0x02
#define OEX_FPU_INEX	0x01

/* Masks for the info word of an ODK_HWPATCH descriptor: */
#define OHW_R4KEOP      0x1	/* R4000 end-of-page patch */
#define OHW_R8KPFETCH   0x2     /* may need R8000 prefetch patch */
#define OHW_R5KEOP      0x4	/* R5000 end-of-page patch */
#define	OHW_R5KCVTL	0x8	/* R5000 cvt.[ds].l bug.  clean=1 */
#define	OHW_R10KLDL	0x10	/* R10000 requires LDL patch	*/

#define OPAD_PREFIX	0x1
#define OPAD_POSTFIX	0x2
#define OPAD_SYMBOL	0x4

/* Masks for the info word of an ODK_IDENT/ODK_GP_GROUP descriptor: */
#define OGP_GROUP	0x0000ffff	/* GP group number */
#define OGP_SELF	0x00010000	/* Self-contained GP groups */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf32_Word	hwp_flags1;	/* extra flags */
	Elf32_Word	hwp_flags2;	/* extra flags */
} Elf_Options_Hw;
#endif
/* ====================================================================
 *
 * .MIPS.options Section	-- HW_AND / HW_OR
 *
 * ====================================================================
 *
 *	These two option descriptors are used by the kernel and 
 *	various tools to track HW specific bugs/issues/features.
 *
 *	The HW_AND option is used for those bits that should be
 *	AND'd by the linker when merged and the HW_OR option is
 *	used for those bits that should be OR'd by the linker.
 *
 *	EXAMPLE:
 *      --------
 *	Suppose the kernel wanted to know if a program used CMOV 
 *	instructions.  We might define the following :
 *		#define	OHWA0_CMOV_CHECKED	0x00000001
 *		#define OHWO0_CMOV_PRESENT	0x00000001
 *
 *	Assume, for this example, that  'eo_hw_and'   and   'eo_hw_or'
 *	are pointers to the two different options.  If the compiler 
 *	checked an object and found it generated no CMOV instructions 
 *	it would generate the following:
 *		eo_hw_and->info |= OHWA0_CMOV_CHECKED;
 *		eo_hw_or->info  |= OHWO0_CMOV_PRESENT;
 *	
 *	The linker would merge these two options by AND'ing all HW_AND
 *	options and OR'ing all HW_OR options.  After all copies are merged
 *	by the linker the meaning of the fields would be:
 *
 *	    CHECKED  PRESENT	MEANING
 *		1	1	All objects checked, CMOVS in binary.
 *		1	0	All objects checked, no CMOVS in binary.
 *		0	1	Some objects checked, CMOVS in binary.
 *		0	0	Some objects checked, CMOVS may be in binary.
 *
 *	NOTES:
 *	------
 *	(1) Dummy defines are included below to indicate naming conventions
 *	    for these fields.
 *	(2) Often it will be appropriate to have both a CHECKED bit and
 *	    a PRESENT bit, but it is not required.  It is up to the 
 *	    implementors to make the choice.
 *	(3) This is a SGI specific feature.  These option words should
 *	    be ignored by any other vendor.
 *	(4) {A|O}0 refers to the info field;
 *	    {A|O}1 refers to the hwp_flags1 field;
 *	    {A|O}2 refers to the hwp_flags2 field.
 *      (5) Any name can be used for bit define's but here is a suggested
 *          naming convention:
 *              OHW{A|O}{0|1|2}_<topic>_{CHECKED|PRESENT}
 *
 * ====================================================================
 */

/*
 * AND info field:
 */
 	    /*
	     * Marks an executable as r4k end of page bug safe.
	     * Both bits must be AND'd for the executable to be
	     * certified safe.
	     *
	     * CHECKED  PRESENT	MEANING
	     *	    1	    1	    certified clean of r4k eop bug
	     * All other variations will render the executable
	     * suspect.
	     */
#define	OHWA0_R4KEOP_CHECKED	0x00000001
#define OHWA0_R4KEOP_CLEAN	0x00000002

/*
 * AND hwp_flags1 field:	-- none yet defined
 */
/* #define OHWA1_... */

/*
 * AND hwp_flags2 field:	-- none yet defined
 */
/* #define OHWA2_... */

/*
 * OR info field:
 */
#define OHWO0_FIXADE	0x00000001	/* Object requires FIXADE call */

/*
 * OR hwp_flags1 field:		-- none yet defined
 */
/* #define OHWO1_... */

/*
 * OR hwp_flags2 field:		-- none yet defined
 */
/* #define OHWO2_... */


/* ====================================================================
 *
 * .rel, .rela Section
 *
 * ====================================================================
 */

/*
 * relocation types
 */

#define R_MIPS_NONE		0
#define R_MIPS_16		1
#define R_MIPS_32		2
#define R_MIPS_ADD		R_MIPS_32
#define R_MIPS_REL		3
#define R_MIPS_REL32		R_MIPS_REL
#define R_MIPS_26		4
#define R_MIPS_HI16		5
#define R_MIPS_LO16		6
#define R_MIPS_GPREL		7
#define R_MIPS_GPREL16		R_MIPS_GPREL
#define R_MIPS_LITERAL		8
#define R_MIPS_GOT		9
#define R_MIPS_GOT16		R_MIPS_GOT
#define R_MIPS_PC16		10
#define R_MIPS_CALL		11
#define R_MIPS_CALL16		R_MIPS_CALL
#define R_MIPS_GPREL32		12

#define R_MIPS_SHIFT5		16
#define R_MIPS_SHIFT6		17
#define R_MIPS_64		18
#define R_MIPS_GOT_DISP		19
#define R_MIPS_GOT_PAGE		20
#define R_MIPS_GOT_OFST		21
#define R_MIPS_GOT_HI16		22
#define R_MIPS_GOT_LO16		23
#define R_MIPS_SUB		24
#define R_MIPS_INSERT_A		25
#define R_MIPS_INSERT_B		26
#define R_MIPS_DELETE		27
#define R_MIPS_HIGHER		28
#define R_MIPS_HIGHEST		29
#define R_MIPS_CALL_HI16	30
#define R_MIPS_CALL_LO16	31
#define R_MIPS_SCN_DISP		32
#define	R_MIPS_REL16		33
#define R_MIPS_ADD_IMMEDIATE    34
#define R_MIPS_PJUMP    	35
#define R_MIPS_RELGOT		36
#define R_MIPS_JALR	    	37

#define _R_MIPS_COUNT_		38	/* Number of relocations */
	/* _R_MIPS_COUNT_ is not a relocation type, it is
	** a count of relocation types. 
        ** Must be one greater than the highest relocation
        ** type.
	*/

#define R_MIPS_LOVEND0R		100 /* Vendor specific relocations */
#define R_MIPS_HIVENDOR		127


/* ====================================================================
 *
 * .content Section
 *
 * sh_type:	SHT_MIPS_CONTENT
 * sh_link:	section header index of section classified
 * sh_info:	0
 * attributes:	SHF_ALLOC, SHF_MIPS_NOSTRIP
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

/* Content kind -- valid for ELF-32 and ELF-64: */
typedef enum {
    CK_NULL	= 0,	    /* Invalid, same as EK_NULL */
    CK_DEFAULT	= 0x30,	    /* Default type of data for section */
    CK_ALIGN	= 0x31,	    /* Alignment for described range */
    CK_INSTR	= 0x32,	    /* Instructions */
    CK_DATA	= 0x33,	    /* Non-address data */
    CK_SADDR_32	= 0x34,	    /* Simple 32-bit addresses */
    CK_GADDR_32	= 0x35,	    /* GP-relative 32-bit addresses */
    CK_CADDR_32	= 0x36,	    /* Complex 32-bit addresses */
    CK_SADDR_64	= 0x37,	    /* Simple 64-bit addresses */
    CK_GADDR_64	= 0x38,	    /* GP-relative 64-bit addresses */
    CK_CADDR_64	= 0x39,	    /* Complex 64-bit addresses */
    CK_NO_XFORM	= 0x3a,	    /* No transformations allowed in this range */
    CK_NO_REORDER = 0x3b,   /* No reordering allowed in this range */
    CK_GP_GROUP = 0x3c,	    /* Text/data in range with length given by
			       second argument references GP group given
			       by first. */
    CK_STUBS	= 0x3d	    /* Text in range is stub code. ULEB */
} Elf_Content_Kind;


#endif


/* ====================================================================
 *
 * .events Section
 *
 * sh_type:	SHT_MIPS_EVENTS
 * sh_link:	section header index of section whose events tracked
 * sh_info:	section header index of associated interface section
 * attributes:	SHF_ALLOC, SHF_MIPS_NOSTRIP
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
/* Event kind -- valid for ELF-32 and ELF-64: */
typedef enum {
    EK_NULL = 0x00,	    /* No valid information */
    EK_ADDR_RESET = 0x01,   /* Reset offset into associated text section */
    EK_INCR_LOC_EXT = 0x02, /* Increment offset into associated text section */
    EK_ENTRY = 0x03,	    /* Subprogram entrypoint */
    EK_IF_ENTRY = 0x04,	    /* Subprogram entrypoint with associated interface offset */
    EK_EXIT = 0x05,	    /* Subprogram exit */
    EK_PEND = 0x06,	    /* Subprogram end (last instruction) */

    EK_SWITCH_32 = 0x7,	    /* jr for switch stmt, table entries are 32bit */
    EK_SWITCH_64 = 0x8,	    /* jr for switch stmt, table entries are 64bit */
    EK_DUMMY = 0x09,	    /* empty slot */

    EK_BB_START = 0x0a,	    /* Basic block beginning */
    EK_INCR_LOC_UNALIGNED = 0x0b,    /* Increment unaligned byte offset */
    EK_GP_PROLOG_HI = 0x0c, /* Establish high 16bits of GP */
    EK_GP_PROLOG_LO = 0x0d, /* Establish low 16bits of GP */
    EK_GOT_PAGE = 0x0e,	    /* Compact relocation: GOT page pointer */
    EK_GOT_OFST = 0x0f,     /* Compact relocation: GOT page offset */
    EK_HI = 0x10,	    /* Compact relocation: high 16bits of abs. addr */
    EK_LO = 0x11,	    /* Compact relocation: low 16bits of abs. addr */
    EK_64_HIGHEST = 0x12,   /* Compact relocation: most significant 16 bits
			       of a 64bit absolute address */
    EK_64_HIGHER = 0x13,    /* Compact relocation: second most significant
			       16 bits of a 64bit absolute address */
    EK_64_HIGH = 0x14,	    /* Compact relocation: third most significant
			       16 bits of a 64bit absolute address */
    EK_64_LOW = 0x15,       /* Compact relocation: least significant 16 bits
			       of a 64bit absolute address */
    EK_GPREL = 0x16,        /* Compact relocation: GP relative reference */

    EK_DEF = 0x17,	    /* Define new event kind format */

    EK_FCALL_LOCAL = 0x18,	/* point-of-call (jalr) to a local procedure */
    EK_FCALL_EXTERN = 0x19,	/* jalr to extern procedure (small got case) */
    EK_FCALL_EXTERN_BIG = 0x1a,	/* jalr to extern procedure (large got case) */
    EK_FCALL_MULT = 0x1b,	/* jalr to more than one procedure */
    EK_FCALL_MULT_PARTIAL = 0x1c, /* jalr to multiple + unknown procedures */

    EK_LTR_FCALL = 0x1d,	/* jalr to rld lazy-text res.  index of
				   symbol associated. */
    EK_PCREL_GOT0 = 0x1e, 	/* immediate is hi 16 bits of 32-bit
				   constant.  Argument is offset to lo,
				   in instructions, not bytes*/

    /* The following events are reserved for supporting Purify-type tools: */
    EK_MEM_COPY_LOAD = 0x1f,    /* load only for copying data */
    EK_MEM_COPY_STORE = 0x20,   /* store only for copying data --
                                   LEB128 operand is word offset to
                                   paired load */
    EK_MEM_PARTIAL_LOAD = 0x21, /* load for reference to a subset of bytes --
                                   BYTE operand's 8 bits indicate which
                                   bytes are actually used */
    EK_MEM_EAGER_LOAD = 0x22,   /* load is speculative */
    EK_MEM_VALID_LOAD = 0x23,   /* load of data known to be valid */

				  
		/*
		 * Yet to be defined kinds with no fields (like EK_EXIT)
		 */
    EK_CK_UNUSED_NONE_0 = 0x50, /*  */
    EK_CK_UNUSED_NONE_1 = 0x51, /*  */
    EK_CK_UNUSED_NONE_2 = 0x52, /*  */
    EK_CK_UNUSED_NONE_3 = 0x53, /*  */
    EK_CK_UNUSED_NONE_4 = 0x54, /*  */

		/*
		 * Yet to be defined kinds with 1 16 bit field
		 */
    EK_CK_UNUSED_16BIT_0 = 0x55,
    EK_CK_UNUSED_16BIT_1 = 0x56,
    EK_CK_UNUSED_16BIT_2 = 0x57, /*  */
    EK_CK_UNUSED_16BIT_3 = 0x58, /*  */
    EK_CK_UNUSED_16BIT_4 = 0x59, /*  */

		/*
		 * Yet to be defined kinds with 1 32 bit field
		 */
    EK_CK_UNUSED_32BIT_0 = 0x5a, /*  */
    EK_CK_UNUSED_32BIT_1 = 0x5b, /*  */
    EK_CK_UNUSED_32BIT_2 = 0x5c, /*  */

		/*
		 * Yet to be defined kinds with 1 64 bit field
		 */

    EK_CK_UNUSED_64BIT_0 = 0x5d,
    EK_CK_UNUSED_64BIT_1 = 0x5e,
    EK_CK_UNUSED_64BIT_2 = 0x5f, /*  */
    EK_CK_UNUSED_64BIT_3 = 0x60, /*  */
    EK_CK_UNUSED_64BIT_4 = 0x61, /*  */

		/*
		 * Yet to be defined kinds with 1 uleb128 field
		 */
    EK_CK_UNUSED_ULEB128_0 = 0x62, /* */
    EK_CK_UNUSED_ULEB128_1 = 0x63, /*  */
    EK_CK_UNUSED_ULEB128_2 = 0x64, /*  */
    EK_CK_UNUSED_ULEB128_3 = 0x65, /*  */
    EK_CK_UNUSED_ULEB128_4 = 0x66, /*  */
    EK_CK_UNUSED_ULEB128_5 = 0x67, /*  */
    EK_CK_UNUSED_ULEB128_6 = 0x68, /*  */
    EK_CK_UNUSED_ULEB128_7 = 0x69, /*  */
    EK_CK_UNUSED_ULEB128_8 = 0x6a, /*  */
    EK_CK_UNUSED_ULEB128_9 = 0x6b, /*  */


    EK_INCR_LOC = 0x80	    /* Increment offset into associated text section */

} Elf_Event_Kind;

/* The following defines list the various types of operands that are 
 * supported with the EK_DEF event kind.
 */
#define EK_DEF_UCHAR	(1)	    /* unsigned char (8 bits) */
#define EK_DEF_USHORT	(2)	    /* unsigned short (16 bits) */
#define EK_DEF_UINT	(3)	    /* unsigned int (32 bits) */
#define EK_DEF_ULONG	(4)	    /* unsigned long (64 bits) */
#define EK_DEF_ULEB128	(5)	    /* unsigned LEB128 encoded number */
#define EK_DEF_CHAR	(6)	    /* signed char (8 bits) */
#define EK_DEF_SHORT	(7)	    /* signed short (16 bits) */
#define EK_DEF_INT	(8)	    /* signed int (32 bits) */
#define EK_DEF_LONG	(9)	    /* signed long (64 bits) */
#define EK_DEF_LEB128	(10)	    /* signed LEB128 encoded number */
#define EK_DEF_STRING	(11)	    /* null terminated string */
#define EK_DEF_VAR	(12)	    /* variable length field: the first 2
				       bytes is an unsigned short
				       specifying the total number of bytes
				       of this field including the first 2
				       bytes */
#define CK_DEF EK_DEF    

#endif


/* ====================================================================
 *
 * .interfaces Section
 *
 * sh_type:	SHT_MIPS_IFACE
 * sh_link:	section header index of associated symbol table
 * sh_info:	0
 * attributes:	SHF_ALLOC, SHF_MIPS_NOSTRIP
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

typedef struct {
    Elf64_Word symbol;		/* symbol table index of subprogram, or 0 */
    Elf64_Half attrs;		/* Attributes: See list below */
    Elf64_Byte pcnt;		/* Parameter count */
    Elf64_Byte fpmask;		/* bit on indicates an FP parameter register */
} Elf_Ifd;

typedef Elf_Ifd Elf_Interface_Descriptor;  /* for compatibility */

/* Flags that can be set in the 'attrs' field of Elf_Interface_Descriptor */
#define SA_PROTOTYPED	0x8000	/* Does def or ref have prototype ? */
#define SA_VARARGS	0x4000	/* Is this a varargs subprogram ? */
#define SA_PIC		0x2000	/* Are memory references PIC? */
#define SA_DSO_ENTRY	0x1000	/* Is subprogram valid DSO entry? */
#define SA_ADDRESSED	0x0800	/* Is subprogram address taken? */
#define SA_FUNCTION	0x0400	/* Does subprogram return a result? */
#define SA_NESTED	0x0200	/* Is subprogram nested? */
#define SA_IGNORE_ERROR	0x0100	/* Ignore consistency errors? */
#define SA_DEFINITION	0x0080	/* Is this a definition (no just call)? */
#define SA_AT_FREE	0x0040	/* Is the at register free at all branches? */
#define SA_FREE_REGS	0x0020	/* Free register mask precedes parm profile */
#define SA_PARAMETERS	0x0010	/* Parameter profile follows descriptor? */
#define SA_ALTINTERFACE 0x0008	/* Alternate descriptor follows? */

/* Parameter descriptor masks */
#define PDM_TYPE	0x00ff	/* Fundamental type of parameter */
#define PDM_REFERENCE	0x4000	/* Reference parameter ? */
#define PDM_SIZE	0x2000	/* Followed by explicit 32-bit byte count? */
#define PDM_Qualifiers	0x0f00	/* Count of type qualifiers << 8 */

/* Parameter descriptor mask flags */
#define PDMF_REFERENCE  0x40
#define PDMF_SIZE       0x20
#define PDMF_Qualifiers 0x0f

/* Fundamental Parameter Types */
#define FT_unknown         0x0000
#define FT_signed_char     0x0001
#define FT_unsigned_char   0x0002
#define FT_signed_short    0x0003
#define FT_unsigned_short  0x0004
#define FT_signed_int32    0x0005
#define FT_unsigned_int32  0x0006
#define FT_signed_int64    0x0007
#define FT_unsigned_int64  0x0008
#define FT_pointer32       0x0009
#define FT_pointer64       0x000a
#define FT_float32         0x000b
#define FT_float64         0x000c
#define FT_float128        0x000d
#define FT_complex64       0x000e
#define FT_complex128      0x000f
#define FT_complex256      0x0010
#define FT_void            0x0011
#define FT_bool32          0x0012
#define FT_bool64          0x0013
#define FT_label32         0x0014
#define FT_label64         0x0015
#define FT_struct          0x0020
#define FT_union           0x0021
#define FT_enum            0x0022
#define FT_typedef         0x0023
#define FT_set             0x0024
#define FT_range           0x0025
#define FT_member_ptr      0x0026
#define FT_virtual_ptr     0x0027
#define FT_class           0x0028

/* Parameter Qualifiers (aka Modifiers)  */
#define MOD_pointer_to     0x01
#define MOD_reference_to   0x02
#define MOD_const          0x03
#define MOD_volatile       0x04
#define MOD_function       0x80
#define MOD_array_of       0x81

#endif

/* ====================================================================
 *
 * .liblist Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf32_Word	l_name;
	Elf32_Word	l_time_stamp;
	Elf32_Word	l_checksum;
	Elf32_Word	l_version;
	Elf32_Word	l_flags;
} Elf32_Lib;
#endif


/*
 * l_flags
 */

#define LL_NONE			0
#define LL_EXACT_MATCH		0x1
#define LL_IGNORE_INT_VER	0x2
#define LL_REQUIRE_MINOR	0x4
#define LL_EXPORTS		0x8
#define LL_DELAY_LOAD		0x10

#define LL_DELTA                0x20

/* ====================================================================
 *
 * .conflict Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef Elf32_Addr Elf32_Conflict;
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
extern Elf32_Conflict	_ConflictList [];
#endif

#define RLD_VERSION            1


/* ====================================================================
 *
 * .got Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf32_Addr	g_index;
} Elf32_Got;


typedef struct
{
	Elf64_Addr	g_index;
} Elf64_Got;

extern Elf32_Got	_GlobalOffsetTable [];

extern Elf64_Got	_GlobalOffsetTable64 [];

#endif


/* ====================================================================
 *
 * .package Section
 *
 * Multiple package entries for the same package are allowed
 * in order to express out of order symbols in a package.
 *
 * ====================================================================
 */

#ifdef __osf__

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Word	pkg_name;	/* index into String Space of name */
	Elf32_Word	pkg_version;	/* index into String Space of version string */
	Elf32_Half	pkg_flags;	/* package flags */
} Elf32_Package;
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
extern Elf32_Package	_PackageList [];
#endif


/*
 * pkg_name --
 * index of a string that identifies the name of this package
 * implementation, which cannot be the null string; the offset is in
 * bytes of a zero terminated string from the start of the .dynstr section
 * pkg_version --
 * index of a string that identifies the version of this package
 * implementation, which may be the null string; the offset is in
 * bytes of a zero terminated string from the start of the .dynstr section
 * pkg_flags --
 * export flag means package is exported, import flag means package is imported,
 * both flags must be set if a package is exported and is also used by other
 * packages within the shared library.  continuance flag means that this
 * package entry defines additional symbols for a previously defined
 * package.  continuance entries must exactly match the original entry in each
 * field, except for the pkg_start, pkg_count, and continuance flag in the pkg_flags.
 * The conflict flag is a possibility for future support for symbol preemption.
 */

/*
 * pkg_flags
 */

#define PKGF_EXPORT	0x1
#define PKGF_IMPORT	0x2
/* #define PKGF_CONFLICT	0x8 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef Elf32_Word Elf32_Package_Symbol;
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define	PACKSYM_NULL_INDEX	((Elf32_Word) 0)
#endif

#endif /* __osf__ */


/* ====================================================================
 *
 * .dynamic Section
 *
 * ====================================================================
 */

#define DT_MIPS_RLD_VERSION     0x70000001
#define DT_MIPS_TIME_STAMP      0x70000002
#define DT_MIPS_ICHECKSUM       0x70000003
#define DT_MIPS_IVERSION        0x70000004
#define DT_MIPS_FLAGS           0x70000005
#define DT_MIPS_BASE_ADDRESS    0x70000006
#define DT_MIPS_MSYM            0x70000007
#define DT_MIPS_CONFLICT        0x70000008
#define DT_MIPS_LIBLIST         0x70000009
#define DT_MIPS_LOCAL_GOTNO     0x7000000A
#define DT_MIPS_CONFLICTNO      0x7000000B
#define DT_MIPS_LIBLISTNO       0x70000010
#define DT_MIPS_SYMTABNO        0x70000011
#define DT_MIPS_UNREFEXTNO      0x70000012
#define DT_MIPS_GOTSYM          0x70000013
#ifndef __osf__
#define DT_MIPS_HIPAGENO        0x70000014
/* 0x70000015 is skipped */
#define DT_MIPS_RLD_MAP         0x70000016
#define DT_MIPS_DELTA_CLASS 	0x70000017	/* contains Delta C++ class definition */
#define DT_MIPS_DELTA_CLASS_NO 	0x70000018	/* the number of entries in DT_MIPS_DELTA_CLASS */
#define DT_MIPS_DELTA_INSTANCE	0x70000019	/* contains Delta C++ class instances */
#define DT_MIPS_DELTA_INSTANCE_NO	0x7000001A	/* the number of entries in DT_MIPS_DELTA_INSTANCE */
#define DT_MIPS_DELTA_RELOC	0x7000001B	/* contains Delta relocations */
#define DT_MIPS_DELTA_RELOC_NO	0x7000001C	/* the number of entries in DT_M
IPS_DELTA_RELOC */
#define DT_MIPS_DELTA_SYM	0x7000001D	/* contains Delta symbols that Delta relocations refer to */
#define DT_MIPS_DELTA_SYM_NO	0x7000001E	/* the number of entries in DT_M
IPS_DELTA_SYM */
#define DT_MIPS_DELTA_CLASSSYM	0x70000020	/* contains Delta symbols that hold the class declaration */
#define DT_MIPS_DELTA_CLASSSYM_NO	0x70000021	/* the number of entries in DT_MIPS_DELTA_CLASSSYM */
#define	DT_MIPS_CXX_FLAGS	0x70000022	/* Flags indicating information about C++ flavor */
#define	DT_MIPS_PIXIE_INIT	0x70000023	/* Flags indicating information about C++ flavor */
#define	DT_MIPS_SYMBOL_LIB	0x70000024
#define DT_MIPS_LOCALPAGE_GOTIDX	0x70000025
#define DT_MIPS_LOCAL_GOTIDX	0x70000026
#define DT_MIPS_HIDDEN_GOTIDX	0x70000027
#define DT_MIPS_PROTECTED_GOTIDX	0x70000028
#define DT_MIPS_OPTIONS		0x70000029	/* Address of .MIPS.options */
#define DT_MIPS_INTERFACE	0x7000002a	/* Address of .interface */
#define DT_MIPS_DYNSTR_ALIGN	0x7000002b
#define DT_MIPS_INTERFACE_SIZE	0x7000002c	/* size of the .interface sec. */
#define	DT_MIPS_RLD_TEXT_RESOLVE_ADDR 0x7000002d    /* Address of rld_text_rsolve function stored in the got */
#define	DT_MIPS_PERF_SUFFIX	0x7000002e	/* Default suffix of dso to be added by */
						/* rld on dlopen() calls. */
#define DT_MIPS_COMPACT_SIZE	0x7000002f	/* (O32)Size of compact rel scn */
#define DT_MIPS_GP_VALUE	0x70000030	/* gp value for aux gots */
#define DT_MIPS_AUX_DYNAMIC	0x70000031      /* Address of aux .dynamic */
#else  /* __osf__ */
#define DT_MIPS_PACKAGE        	0x70000014
#define DT_MIPS_PACKAGENO       0x70000015
#define DT_MIPS_PACKSYM		0x70000016
#define DT_MIPS_PACKSYMNO	0x70000017
#define	DT_MIPS_IMPACKNO	0x70000018
#define	DT_MIPS_EXPACKNO	0x70000019
#define	DT_MIPS_IMPSYMNO	0x7000001A
#define	DT_MIPS_EXPSYMNO	0x7000001B
#define DT_MIPS_HIPAGENO        0x7000001C
#endif /* __osf__ */

#define RHF_NONE                    0x00000000
#define RHF_QUICKSTART              0x00000001
			/* RHF_QUICKSTART is turned on by ld if
			   ld determines that at link time the
			   object is quickstartable.
			*/
#define RHF_NOTPOT                  0x00000002 	
			/* RHF_NOTPOT bit non-zero if elf hash 
			   table element count
			   is NOT a Power Of Two 
			   If 0, rld uses mask, else rld uses %
			   (mod) operator to turn hash into index.
			*/
#define RHF_NO_LIBRARY_REPLACEMENT  0x00000004
#define RHF_NO_MOVE                 0x00000008
#define RHF_SGI_ONLY                0x00000010
#define RHF_GUARANTEE_INIT	    0x00000020
#define RHF_DELTA_C_PLUS_PLUS	    0x00000040
#define RHF_GUARANTEE_START_INIT    0x00000080
#define RHF_PIXIE    		    0x00000100
#define	RHF_DEFAULT_DELAY_LOAD	    0x00000200
#define	RHF_REQUICKSTART	    0x00000400
			/* If RHF_REQUICKSTART is on, rqs will
			   process the object. If off, 
			   rqs will not process the object unless
			   -force_requickstart  option used.
			   Set by ld or rqs.
			   ld sets it if ld believes the object should
			   allow rqs-ing.  -abi objects do not have
			   this turned on by ld.
			*/
#define	RHF_REQUICKSTARTED          0x00000800
			/* RHF_REQUICKSTARTED  bit non-zero if
			   rqs has processed the object
			   Set by rqs.
			*/
#define RHF_CORD                    0x00001000
			/* RHF_CORD bit is non-zero if
			   the object  has been cord(1)ed
			   set by cord.
			*/
#define RHF_NO_UNRES_UNDEF	    0x00002000
			/* RHF_NO_UNRES_UNDEF is non-zero if
			   every external symbol is resolved
			   (ie, no externals are undefined)
			   Set by ld or rqs.
			*/
#define RHF_RLD_ORDER_SAFE	    0x00004000
			/* If on, RHF_RLD_ORDER_SAFE tells rld it
			   can stop searching for UNDEFineds to
			   resolve when the
			   first non-ABS, non-UNDEF is found
			   (saving time in rld).
			   Set by ld.
			*/



/* ====================================================================
 * ====================================================================
 *
 * 64-bit declarations
 *
 * ====================================================================
 * ====================================================================
 */

#define ELF64_FSZ_ADDR		8
#define ELF64_FSZ_HALF		2
#define ELF64_FSZ_OFF		8
#define ELF64_FSZ_SWORD		4
#define ELF64_FSZ_WORD		4
#define ELF64_FSZ_SXWORD	8
#define ELF64_FSZ_XWORD		8


/* ====================================================================
 *
 * ELF header
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* ident bytes */
	Elf64_Half	e_type;			/* file type */
	Elf64_Half	e_machine;		/* target machine */
	Elf64_Word	e_version;		/* file version */
	Elf64_Addr	e_entry;		/* start address */
	Elf64_Off	e_phoff;		/* phdr file offset */
	Elf64_Off	e_shoff;		/* shdr file offset */
	Elf64_Word	e_flags;		/* file flags */
	Elf64_Half	e_ehsize;		/* sizeof ehdr */
	Elf64_Half	e_phentsize;		/* sizeof phdr */
	Elf64_Half	e_phnum;		/* number phdrs */
	Elf64_Half	e_shentsize;		/* sizeof shdr */
	Elf64_Half	e_shnum;		/* number shdrs */
	Elf64_Half	e_shstrndx;		/* shdr string index */
} Elf64_Ehdr;
#endif


/* ====================================================================
 *
 * Program header
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf64_Word	p_type;		/* entry type */
	Elf64_Word	p_flags;	/* entry flags */
	Elf64_Off	p_offset;	/* file offset */
	Elf64_Addr	p_vaddr;	/* virtual address */
	Elf64_Addr	p_paddr;	/* physical address */
	Elf64_Xword	p_filesz;	/* file size */
	Elf64_Xword	p_memsz;	/* memory size */
	Elf64_Xword	p_align;	/* memory/file alignment */
} Elf64_Phdr;
#endif


/* ====================================================================
 *
 * Section Headers
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf64_Word	sh_name;	/* section name */
	Elf64_Word	sh_type;	/* SHT_... */
	Elf64_Xword	sh_flags;	/* SHF_... */
	Elf64_Addr	sh_addr;	/* virtual address */
	Elf64_Off	sh_offset;	/* file offset */
	Elf64_Xword	sh_size;	/* section size */
	Elf64_Word	sh_link;	/* misc info */
	Elf64_Word	sh_info;	/* misc info */
	Elf64_Xword	sh_addralign;	/* memory alignment */
	Elf64_Xword	sh_entsize;	/* entry size if table */
} Elf64_Shdr;
#endif


/* ====================================================================
 *
 * .symtab Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf64_Word	st_name;
	unsigned char	st_info;	/* bind, type: ELF_64_ST_... */
	unsigned char	st_other;
	Elf64_Half	st_shndx;	/* SHN_... */
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;
#endif


/*	The macros compose and decompose values for S.st_info
 *
 *	bind = ELF64_ST_BIND(S.st_info)
 *	type = ELF64_ST_TYPE(S.st_info)
 *	S.st_info = ELF64_ST_INFO(bind, type)
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF64_ST_BIND(info)		((info) >> 4)
#define ELF64_ST_TYPE(info)		((info) & 0xf)
#define ELF64_ST_INFO(bind,type)	(((bind)<<4)+((type)&0xf))
#endif


/* ====================================================================
 *
 * .rel, .rela Section
 *
 * WARNING:  The Elf64_Rel and Elf64_Rela structures must be identical
 * except for the r_addend field.
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Word	r_sym;		/* Symbol index */
	Elf64_Byte	r_ssym;		/* Special symbol */
	Elf64_Byte	r_type3;	/* 3rd relocation op type */
	Elf64_Byte	r_type2;	/* 2nd relocation op type */
	Elf64_Byte	r_type;		/* 1st relocation op type */
} Elf64_Rel;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Word	r_sym;		/* Symbol index */
	Elf64_Byte	r_ssym;		/* Special symbol */
	Elf64_Byte	r_type3;	/* 3rd relocation op type */
	Elf64_Byte	r_type2;	/* 2nd relocation op type */
	Elf64_Byte	r_type;		/* 1st relocation op type */
	Elf64_Sxword	r_addend;
} Elf64_Rela;

/* Values for the r_ssym field: */
typedef enum {
    RSS_UNDEF	= 0,	/* Undefined */
    RSS_GP	= 1,	/* Context pointer (gp) value */
    RSS_GP0	= 2,	/* gp value used to create object being relocated */
    RSS_LOC	= 3	/* Address of location being relocated */
} Elf64_Rel_Ssym;

#endif

/*	The macros compose and decompose values for Rel.r_info, Rela.f_info
 *
 *	sym = ELF64_R_SYM(R.r_info)
 *	type = ELF64_R_TYPE(R.r_info)
 *	R.r_info = ELF64_R_INFO(sym, type)
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF64_R_SYM(info)	((info)>>8)
#define ELF64_R_TYPE(info)	((unsigned char)(info))
#define ELF64_R_INFO(sym,type)	(((sym)<<8)+(unsigned char)(type))
#endif


/* ====================================================================
 *
 * .conflict Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef Elf64_Addr Elf64_Conflict;

extern Elf64_Conflict   _ConflictList64 [];
#endif


/* ====================================================================
 *
 * .reginfo Section
 *
 * now the tail part of an .MIPS.options ODK_REGINFO structure 
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
        Elf64_Word      ri_gprmask;	/* mask of general regs used */
        Elf64_Word      ri_pad;		/* for alignment */
        Elf64_Word      ri_cprmask[4];	/* mask of cop regs used */
        Elf64_Addr      ri_gp_value;	/* initial value of gp */
} Elf64_RegInfo;
#endif


/* ====================================================================
 *
 * .content Section
 *
 * sh_type:	SHT_MIPS_CONTENT
 * sh_link:	section header index of section classified
 * sh_info:	0
 * attributes:	SHF_ALLOC, SHF_MIPS_NOSTRIP
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef union {
    struct {				/* Normal descriptor */
	Elf64_Word	con_info;
	Elf64_Word	con_start;
    } con_y;
    Elf64_Xword	con_xval;		/* Extension descriptor */
} Elf64_Content;

/* con_info masks: */
#define __CON64_EMASK	0x80000000
#define __CON64_ESHIFT	31
#define __CON64_KMASK	0x7f000000
#define __CON64_KSHIFT	24
#define __CON64_LMASK	0x00ffffff
#define __CON64_VMASK	0x7fffffffffffffffll

/* Access macros: */
#define ELF64_CON_EXTN(c) \
	(((c).con_y.con_info & __CON64_EMASK)>>__CON64_ESHIFT)
#define ELF64_CON_KIND(c) \
	(((c).con_y.con_info & __CON64_KMASK)>>__CON64_KSHIFT)
#define ELF64_CON_LENGTH(c)	((c).con_y.con_info & __CON64_LMASK)
#define ELF64_CON_XVAL(c)	((c).con_xval & __CON64_VMASK)

#endif


/* ====================================================================
 *
 * .dynamic Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf64_Xword	d_tag;
	union {
		Elf64_Xword	d_val;
		Elf64_Addr	d_ptr;
	} d_un;
} Elf64_Dyn;
#endif


/* ====================================================================
 *
 * .liblist Section
 *
 * ====================================================================
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
        Elf64_Word      l_name;
        Elf64_Word      l_time_stamp;
        Elf64_Word      l_checksum;
        Elf64_Word      l_version;
        Elf64_Word      l_flags;
} Elf64_Lib;
#endif

#endif /* __SYS_ELF_H__ */
