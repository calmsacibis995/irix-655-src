#ifndef __ELF_H__
#define __ELF_H__
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <sys/elftypes.h> 

#define ELF32_FSZ_ADDR	4
#define ELF32_FSZ_HALF	2
#define ELF32_FSZ_OFF	4
#define ELF32_FSZ_SWORD	4
#define ELF32_FSZ_WORD	4


/*	"Enumerations" below use ...NUM as the number of
 *	values in the list.  It should be 1 greater than the
 *	highest "real" value.
 */


/*	ELF header
 */

#define EI_NIDENT	16

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* ident bytes */
	Elf32_Half	e_type;			/* file type */
	Elf32_Half	e_machine;		/* target machine */
	Elf32_Word	e_version;		/* file version */
	Elf32_Addr	e_entry;		/* start address */
	Elf32_Off	e_phoff;		/* phdr file offset */
	Elf32_Off	e_shoff;		/* shdr file offset */
	Elf32_Word	e_flags;		/* file flags */
	Elf32_Half	e_ehsize;		/* sizeof ehdr */
	Elf32_Half	e_phentsize;		/* sizeof phdr */
	Elf32_Half	e_phnum;		/* number phdrs */
	Elf32_Half	e_shentsize;		/* sizeof shdr */
	Elf32_Half	e_shnum;		/* number shdrs */
	Elf32_Half	e_shstrndx;		/* shdr string index */
} Elf32_Ehdr;
#endif

#define EI_MAG0		0		/* e_ident[] indexes */
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

#define ELFMAG0		0x7f		/* EI_MAG */
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define IS_ELF(ehdr)	((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
			(ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
			(ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
			(ehdr).e_ident[EI_MAG3] == ELFMAG3)
#endif


#define ELFCLASSNONE	0		/* EI_CLASS */
#define ELFCLASS32	1
#define ELFCLASS64	2
#define ELFCLASSNUM	3  /* Number of ELFCLASS*, not a real EI_CLASS. */
			   /* ELFCLASSNUM is one greater than the highest
			   ** EI_CLASS value, not an EI_CLASS entry.
			   */

#define ELFDATANONE	0		/* EI_DATA */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2
#define ELFDATANUM	3  /* Number of ELFDATA*, not a real EI_DATA. */
			   /* ELFDATANUM is one greater than the highest
			   ** EI_DATA value, not an EI_DATA entry.
			   */

#define ET_NONE		0		/* e_type */
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_NUM		5  /* Number of e_types, not an e_type. */
			   /* ET_NUM must be one greater than the 
			   ** highest e_type.
			   */

#define	ET_LOPROC	0xff00		/* processor specific range */
#define	ET_HIPROC	0xffff

#define EM_NONE		0		/* e_machine */
#define EM_M32		1		/* AT&T WE 32100 */
#define EM_SPARC	2		/* Sun SPARC */
#define EM_386		3		/* Intel 80386 */
#define EM_68K		4		/* Motorola 68000 */
#define EM_88K		5		/* Motorola 88000 */
#define EM_486		6		/* Intel 80486 */
#define EM_860		7		/* Intel i860 */
#define	EM_MIPS		8		/* Mips R2000 */
#define	EM_S370		9		/* Amdhal	*/
#define EM_NUM		10 /* Number of e_machine entries, not */
			   /* an actual e_machine entry. */
			   /* EM_NUM must be one greater than the 
			   ** highest e_machine value.
			   */

#define EV_NONE		0		/* e_version, EI_VERSION */
#define EV_CURRENT	1
#define EV_NUM		2  /* Number of version entries, not */
			   /* an actual version. */
			   /* EV_NUM must be one greater than the 
			   ** highest e_version value.
			   */


/*	Program header
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Word	p_type;		/* entry type */
	Elf32_Off	p_offset;	/* file offset */
	Elf32_Addr	p_vaddr;	/* virtual address */
	Elf32_Addr	p_paddr;	/* physical address */
	Elf32_Word	p_filesz;	/* file size */
	Elf32_Word	p_memsz;	/* memory size */
	Elf32_Word	p_flags;	/* entry flags */
	Elf32_Word	p_align;	/* memory/file alignment */
} Elf32_Phdr;
#endif

#define PT_NULL		0		/* p_type */
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_NUM		7 	/* Number of p_type s, not an actual */
				/* p_type. */
			   	/* PT_NUM must be one greater than the 
			   	** highest p_type value.
			   	*/

#define PT_LOPROC	0x70000000	/* processor specific range */
#define PT_HIPROC	0x7fffffff

#define PF_R		0x4		/* p_flags */
#define PF_W		0x2
#define PF_X		0x1

#define PF_MASKPROC	0xf0000000	/* processor specific values */


/*	Section header
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Word	sh_name;	/* section name */
	Elf32_Word	sh_type;	/* SHT_... */
	Elf32_Word	sh_flags;	/* SHF_... */
	Elf32_Addr	sh_addr;	/* virtual address */
	Elf32_Off	sh_offset;	/* file offset */
	Elf32_Word	sh_size;	/* section size */
	Elf32_Word	sh_link;	/* misc info */
	Elf32_Word	sh_info;	/* misc info */
	Elf32_Word	sh_addralign;	/* memory alignment */
	Elf32_Word	sh_entsize;	/* entry size if table */
} Elf32_Shdr;
#endif

#define SHT_NULL	0		/* sh_type */
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_NUM		12 	/* Number of sh_types, not an actual */
				/* sh_type. */
			   	/* SHT_NUM must be one greater than the 
			   	** highest sh_type value.
			   	*/
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

#define	SHT_LOPROC	0x70000000	/* processor specific range */
#define	SHT_HIPROC	0x7fffffff

#define SHF_WRITE	0x1		/* sh_flags */
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4

#define SHF_MASKPROC	0xf0000000	/* processor specific values */

#define SHN_UNDEF	0		/* special section numbers */
#define SHN_LORESERVE	0xff00
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

#define SHN_LOPROC	0xff00		/* processor specific range */
#define SHN_HIPROC	0xff1f

/*
 * special section names
 */

#define ELF_BSS		".bss"
#define ELF_COMMENT	".comment"
#define ELF_DATA	".data"
#define ELF_DEBUG	".debug"
#define ELF_DYNAMIC	".dynamic"
#define ELF_DYNSTR	".dynstr"
#define ELF_DYNSYM	".dynsym"
#define ELF_MSYM	".msym"
#define ELF_FINI	".fini"
#define ELF_GOT		".got"
#define ELF_HASH	".hash"
#define ELF_INIT	".init"
#define ELF_REL_DATA	".rel.data"
#define ELF_REL_FINI	".rel.fini"
#define ELF_REL_INIT	".rel.init"
#define ELF_REL_DYN	".rel.dyn"
#define ELF_REL_RODATA	".rel.rodata"
#define ELF_REL_TEXT	".rel.text"
#define ELF_RODATA	".rodata"
#define ELF_SHSTRTAB	".shstrtab"
#define ELF_STRTAB	".strtab"
#define ELF_SYMTAB	".symtab"
#define ELF_TEXT	".text"

#ifdef __osf__
/*
 * OSF package additions
 */
#define       ELF_PACKAGE     ".package"
#define       ELF_PACKSYM     ".packsym"
#endif /* __osf__ */


/*	Symbol table
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;	/* bind, type: ELF_32_ST_... */
	unsigned char	st_other;
	Elf32_Half	st_shndx;	/* SHN_... */
} Elf32_Sym;
#endif

#define STN_UNDEF	0

/*	The macros compose and decompose values for S.st_info
 *
 *	bind = ELF32_ST_BIND(S.st_info)
 *	type = ELF32_ST_TYPE(S.st_info)
 *	S.st_info = ELF32_ST_INFO(bind, type)
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF32_ST_BIND(info)		((info) >> 4)
#define ELF32_ST_TYPE(info)		((info) & 0xf)
#define ELF32_ST_INFO(bind,type)	(((bind)<<4)+((type)&0xf))
#endif

#define STB_LOCAL	0		/* BIND */
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_NUM		3 /* Number of STB_ entries, not a binding. */
			  /* STB_NUM must be one greater than the 
			  ** highest  actual STB_* binding.
			  */

    /* WARNING: new defines go into <sys/elf.h> */

#define STB_LOPROC	13		/* processor specific range */
#define STB_HIPROC	15

#define STT_NOTYPE	0		/* TYPE */
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_NUM		5 /* Number of STT entries, not a type. */
			  /* STT_NUM must be one greater than the 
			  ** highest  actual STT_* type.
			  */

#define STT_LOPROC	13		/* processor specific range */
#define STT_HIPROC	15


/*	Relocation
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;		/* sym, type: ELF32_R_... */
} Elf32_Rel;
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;		/* sym, type: ELF32_R_... */
	Elf32_Sword	r_addend;
} Elf32_Rela;
#endif

/*	The macros compose and decompose values for Rel.r_info, Rela.f_info
 *
 *	sym = ELF32_R_SYM(R.r_info)
 *	type = ELF32_R_TYPE(R.r_info)
 *	R.r_info = ELF32_R_INFO(sym, type)
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF32_R_SYM(info)	((info)>>8)
#define ELF32_R_TYPE(info)	((unsigned char)(info))
#define ELF32_R_INFO(sym,type)	(((sym)<<8)+(unsigned char)(type))
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct {
    Elf32_Sword         d_tag;
    union {
	Elf32_Word      d_val;
	Elf32_Addr      d_ptr;
    } d_un;
} Elf32_Dyn;
#endif
/*  GOT is Global Offset Table
    PLT is Procedure Linkage Table.
    The Procedure Linkage Table (used with, for example,
	the ATT 3B2 computer) entries are in an area of
	the program text called the GOT stubs area (.MIPS.stubs)
	in a MIPS binary.  The MIPS ABI does not have a .plt 
	(Procedure Linkage Table) section, though the GOT stubs 
	perform  a function very like that of the 
	ATT 3B2 Procedure Linkage Table.
*/
#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff


/* Archive macros */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF_AR_SYMTAB_NAME	"/"
#define ELF_AR_SYMTAB_NAME_LEN	1
#define ELF_AR_STRTAB_NAME	"//"
#define ELF_AR_STRTAB_NAME_LEN	2
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define IS_ELF_AR_SYMTAB(s) \
    ((s[0] == '/') && ((s[1] == ' ') || (s[1] == '\0')))
#define IS_ELF_AR_STRTAB(s) \
    (((s[0] == '/') && (s[1] == '/')) && ((s[2] == ' ') || (s[2] == '\0')))
#endif

#include <sys/elf.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ELF_H__ */
