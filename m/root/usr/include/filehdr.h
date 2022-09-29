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
#ident "$Revision: 7.13 $"
#ifndef __FILEHDR_H__
#define __FILEHDR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifdef __mips 
/*
 * The field f_symptr now is a file pointer to the symbolic header which was
 * the file pointer to symtab.  The field f_nsyms is now the size of the
 * symbolic header which was number of symtab entries.
 */
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#if (_MIPS_SZLONG == 32)
struct filehdr {
	unsigned short	f_magic;	/* magic number */
	unsigned short	f_nscns;	/* number of sections */
	long		f_timdat;	/* time & date stamp */
	long		f_symptr;	/* file pointer to symbolic header */
	long		f_nsyms;	/* sizeof(symbolic hdr) */
	unsigned short	f_opthdr;	/* sizeof(optional hdr) */
	unsigned short	f_flags;	/* flags */
	};
#endif
#if (_MIPS_SZLONG == 64)
#include <sgidefs.h>
struct filehdr {
	unsigned short	f_magic;	/* magic number */
	unsigned short	f_nscns;	/* number of sections */
	__int32_t	f_timdat;	/* time & date stamp */
	__int32_t	f_symptr;	/* file pointer to symbolic header */
	__int32_t	f_nsyms;	/* sizeof(symbolic hdr) */
	unsigned short	f_opthdr;	/* sizeof(optional hdr) */
	unsigned short	f_flags;	/* flags */
	};
#endif
#endif /* _LANGUAGE_C */

#ifdef _LANGUAGE_PASCAL
type
  filehdr = packed record
      f_magic : ushort; 		/* magic number 		     */
      f_nscns : ushort; 		/* number of sections		     */
      f_timdat : long;			/* time & date stamp		     */
      f_symptr : long;			/* file pointer to symbolic header   */
      f_nsyms : long;			/* sizeof(symbolic hdr) 	     */
      f_opthdr : ushort;		/* sizeof(optional hdr) 	     */
      f_flags : ushort; 		/* flags			     */
    end {record};
#endif /* _LANGUAGE_PASCAL */

/*
 *   Bits for f_flags:
 *
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable  (i.e. no unresolved
 *				externel references)
 *	F_LNNO		line nunbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_MINMAL	this is a minimal object file (".m") output of fextract
 *	F_UPDATE	this is a fully bound update file, output of ogen
 *	F_SWABD		this file has had its bytes swabbed (in names)
 *	F_AR16WR	this file has the byte ordering of an AR16WR (e.g. 11/70) machine
 *				(it was created there, or was produced by conv)
 *	F_AR32WR	this file has the byte ordering of an AR32WR machine(e.g. vax)
 *	F_AR32W		this file has the byte ordering of an AR32W machine (e.g. 3b,maxi,MC68000)
 *	F_PATCH		file contains "patch" list in optional header
 *	F_NODF		(minimal file only) no decision functions for
 *				replaced functions
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define  F_RELFLG	0000001
#define  F_EXEC		0000002
#define  F_LNNO		0000004
#define  F_LSYMS	0000010
#define  F_MINMAL	0000020
#define  F_UPDATE	0000040
#define  F_SWABD	0000100
#define  F_AR16WR	0000200
#define  F_AR32WR	0000400
#define  F_AR32W	0001000
#define  F_PATCH	0002000
#define  F_NODF		0002000
#define  F_64INT        0004000      /* basic int size is 64 bits */
#endif /* _LANGUAGE_C */

#ifdef _LANGUAGE_PASCAL
#define  F_RELFLG	16#0001
#define  F_EXEC		16#0002
#define  F_LNNO		16#0004
#define  F_LSYMS	16#0008
#define  F_MINMAL	16#0010
#define  F_UPDATE	16#0020
#define  F_SWABD	16#0040
#define  F_AR16WR	16#0080
#define  F_AR32WR	16#0100
#define  F_AR32W	16#0200
#define  F_PATCH	16#0400
#define  F_NODF		16#0400
#define  F_64INT        16#0800      /* basic int size is 64 bits */
#endif /* _LANGUAGE_PASCAL */

/*
 *	BELLMAC-32	Identification field
 *	F_BM32B		file contains BM32B code (as opposed to strictly BM32A)
 *	F_BM32MAU	file requires MAU (math arith unit) to execute
 */

#define	F_BM32ID	0160000
#define	F_BM32MAU	0040000
#define F_BM32B         0020000

/*	F_BM32RST	file has RESTORE work-around	*/
#define F_BM32RST	0010000

/*
 *	Flags for the INTEL chips.  If the magic number of the object file
 *	is IAPX16 or IAPX16TV or IAPX20 or IAPX20TV then if F_80186
 *	is set, there are some 80186 instructions in the code, and hence
 *	and 80186 or 80286 chip must be used to run the code.
 *	If F_80286 is set, then the code has to be run on an 80286 chip.
 *	And if neither are set, then the code can run on an 8086, 80186, or
 *	80286 chip.
 *	
 */

#define F_80186		    010000
#define F_80286		    020000

/*
 * doesn't hurt to overload these bits since they are processor dependent
 */
#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define F_MIPS_NO_SHARED    010000
#define F_MIPS_SHARABLE     020000
#define F_MIPS_CALL_SHARED  030000
#define F_MIPS_NO_REORG     040000        /* may remove nops */
#define F_MIPS_XGOT	    0100000
#define F_MIPS_UGEN_ALLOC   F_MIPS_XGOT	/* obsolete flags */
#endif
#ifdef _LANGUAGE_PASCAL
#define F_MIPS_NO_SHARED    16#01000
#define F_MIPS_SHARABLE     16#02000
#define F_MIPS_CALL_SHARED  16#03000
#define F_MIPS_NO_REORG     16#04000
#define F_MIPS_XGOT	    16#08000
#define F_MIPS_UGEN_ALLOC   F_MIPS_XGOT
#endif

/*
 *   Magic Numbers
 */

	/* iAPX - the stack frame and return registers differ from
	 * 	  Basic-16 and x86 C compilers, hence new magic numbers
	 *	  are required.  These are cross compilers.
	 */

	/* Intel */
#define  IAPX16		0504
#define  IAPX16TV	0505
#define  IAPX20		0506
#define  IAPX20TV	0507
/* 0514, 0516 and 0517 reserved for Intel */

	/* __mips */
#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define	 MAGIC_ARCH_MASK		0x00FF
#define	 MAGIC_ARCH_SHFT		0
#define	 MAGIC_S_ARCH_MASK		0xFF00
#define	 MAGIC_S_ARCH_SHFT		8
#define  MIPSEBMAGIC	0x0160
#define  MIPSELMAGIC	0x0162
#define  SMIPSEBMAGIC	0x6001
#define  SMIPSELMAGIC	0x6201
#define  MIPSEBUMAGIC	0x0180
#define  MIPSELUMAGIC	0x0182
#endif /* _LANGUAGE_C */
#ifdef _LANGUAGE_PASCAL
#define	 MAGIC_ARCH_MASK		16#00FF
#define	 MAGIC_ARCH_SHFT		0
#define	 MAGIC_S_ARCH_MASK		16#FF00
#define	 MAGIC_S_ARCH_SHFT		8
#define  MIPSEBMAGIC	16#0160
#define  MIPSELMAGIC	16#0162
#define  SMIPSEBMAGIC	16#6001
#define  SMIPSELMAGIC	16#6201
#define  MIPSEBUMAGIC	16#0180
#define  MIPSELUMAGIC	16#0182
#endif
#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define  MIPSEBMAGIC_2	0x0163
#define  MIPSELMAGIC_2	0x0166
#define  SMIPSEBMAGIC_2	0x6301
#define  SMIPSELMAGIC_2	0x6601
#endif /* _LANGUAGE_C */
#ifdef _LANGUAGE_PASCAL
#define  MIPSEBMAGIC_2	16#0163
#define  MIPSELMAGIC_2	16#0166
#define  SMIPSEBMAGIC_2	16#6301
#define  SMIPSELMAGIC_2	16#6601
#endif
#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define  MIPSEBMAGIC_3	0x0140
#define  MIPSELMAGIC_3	0x0142
#define  SMIPSEBMAGIC_3	0x4001
#define  SMIPSELMAGIC_3	0x4201
#define  MIPSEBMAGIC_4  0x0144
#define  MIPSELMAGIC_4  0x0146
#define  SMIPSEBMAGIC_4 0x4401
#define  SMIPSELMAGIC_4 0x4601
#define	 MAGIC_MIPS1	0x0062
#define	 MAGIC_MIPS2	0x0066
#define  MAGIC_MIPS3    0x0042
#define  MAGIC_MIPS4	0x0046
#endif /* _LANGUAGE_C */
#ifdef _LANGUAGE_PASCAL
#define  MIPSEBMAGIC_3	16#0140
#define  MIPSELMAGIC_3	16#0142
#define  SMIPSEBMAGIC_3	16#4001
#define  SMIPSELMAGIC_3	16#4201
#define  MIPSEBMAGIC_4  16#0144
#define  MIPSELMAGIC_4  16#0146
#define  SMIPSEBMAGIC_4 16#4401
#define  SMIPSELMAGIC_4 16#4601
#define	 MAGIC_MIPS1	16#0062
#define	 MAGIC_MIPS2	16#0066
#define  MAGIC_MIPS3    16#0042
#define  MAGIC_MIPS4	16#0044
#endif

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define IS_MIPSEBMAGIC(x) ((x) == MIPSEBMAGIC || (x) == MIPSEBMAGIC_2 || (x) == MIPSEBMAGIC_3)
#define IS_SMIPSEBMAGIC(x) ((x) == SMIPSEBMAGIC || (x) == SMIPSEBMAGIC_2 || (x) == SMIPSEBMAGIC_3)
#define IS_MIPSELMAGIC(x) ((x) == MIPSELMAGIC || (x) == MIPSELMAGIC_2 || (x) == MIPSELMAGIC_3)
#define IS_SMIPSELMAGIC(x) ((x) == SMIPSELMAGIC || (x) == SMIPSELMAGIC_2 || (x) == SMIPSELMAGIC_3)
#endif /* _LANGUAGE_C */

	/* Basic-16 */
#define  B16MAGIC	0502
#define  BTVMAGIC	0503

	/* x86 */
#define  X86MAGIC	0510
#define  XTVMAGIC	0511

	/* Intel 286 */
#define I286SMAGIC	0512
#define I286LMAGIC	0522	/* used by mc68000 (UNIX PC) and iAPX 286 */

	/* n3b */
/*
 *   NOTE:   For New 3B, the old values of magic numbers
 *		will be in the optional header in the structure
 *		"aouthdr" (identical to old 3B aouthdr).
 */
#define  N3BMAGIC	0550	/* 3B20 executable, no TV */
#define  NTVMAGIC	0551	/* 3B20 executable with TV */

	/*  XL  */
#define	 XLMAGIC	0540

	/*  MAC-32, 3B15, 3B5  */
#define  WE32MAGIC	0560	/* WE 32000, no TV */
#define  FBOMAGIC	0560	/* WE 32000, no TV */
#define  RBOMAGIC	0562	/* reserved for WE 32000 */
#define  MTVMAGIC	0561	/* WE 32000 with TV */

	/* VAX 11/780 and VAX 11/750 */

			/* writeable text segments */
#define VAXWRMAGIC	0570
			/* readonly sharable text segments */
#define VAXROMAGIC	0575

	/* pdp11 */
/*			0401	UNIX-rt ldp */
/*			0405	pdp11 overlay */
/*			0407	pdp11/pre System V vax executable */
/*			0410	pdp11/pre System V vax pure executable */
/*			0411	pdp11 seperate I&D */
/*			0437	pdp11 kernel overlay */


	/* Motorola 68000/68008/68010/68020 */
#define	MC68MAGIC	0520
#define MC68KWRMAGIC	0520	/* writeable text segments */
#define	MC68TVMAGIC	0521
#define MC68KROMAGIC	0521	/* readonly shareable text segments */
#define MC68KPGMAGIC	0522	/* demand paged text segments */
#define	M68MAGIC	0210
#define	M68TVMAGIC	0211


	/* IBM 370 */
#define	U370WRMAGIC	0530	/* writeble text segments	*/
#define	U370ROMAGIC	0535	/* readonly sharable text segments	*/
/* 0532 and 0533 reserved for u370 */

	/* Amdahl 470/580 */
#define AMDWRMAGIC	0531	/* writable text segments */
#define AMDROMAGIC	0534	/* readonly sharable text segments */

	/* NSC */
/* 0524 and 0525 reserved for NSC */

	/* Zilog */
/* 0544 and 0545 reserved for Zilog */

#define	FILHDR	struct filehdr
#define	FILHSZ	sizeof(FILHDR)

#define ISCOFF(x) \
		(((x)==B16MAGIC) || ((x)==BTVMAGIC) || ((x)==X86MAGIC) \
		|| ((x)==XTVMAGIC) || ((x)==N3BMAGIC) || ((x)==NTVMAGIC) \
		|| ((x)==FBOMAGIC) || ((x)==VAXROMAGIC) || ((x)==VAXWRMAGIC) \
		|| ((x)==RBOMAGIC) || ((x)==MC68TVMAGIC) \
		|| ((x)==MC68MAGIC) || ((x)==M68MAGIC) || ((x)==M68TVMAGIC) \
		|| ((x)==IAPX16) || ((x)==IAPX16TV) \
		|| ((x)==IAPX20) || ((x)==IAPX20TV) \
		|| ((x)==MIPSEBMAGIC) || ((x)==MIPSELMAGIC) \
		|| ((x)==SMIPSEBMAGIC) || ((x)==SMIPSELMAGIC) \
		|| ((x)==MIPSEBUMAGIC) || ((x)==MIPSELUMAGIC) \
		|| ((x)==MIPSEBMAGIC_2) || ((x)==MIPSELMAGIC_2) \
		|| ((x)==SMIPSEBMAGIC_2) || ((x)==SMIPSELMAGIC_2) \
		|| ((x)==MIPSEBMAGIC_3) || ((x)==MIPSELMAGIC_3) \
		|| ((x)==SMIPSEBMAGIC_3) || ((x)==SMIPSELMAGIC_3) \
		|| ((x)==U370WRMAGIC) || ((x)==U370ROMAGIC) || ((x)==MTVMAGIC) \
		|| ((x)==I286SMAGIC) || ((x)==I286LMAGIC) \
		|| ((x)==MC68KWRMAGIC) || ((x)==MC68KROMAGIC) \
		|| ((x)==MC68KPGMAGIC))

#ifdef __cplusplus
}
#endif

#endif  /* __FILEHDR_H__ */
