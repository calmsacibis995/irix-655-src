#ifndef __SYS_ELFTYPES_H__
#define __SYS_ELFTYPES_H__

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <sgidefs.h>

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

/* 32 bit data types */
#if (_MIPS_SZLONG == 32)
typedef unsigned long	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef long		Elf32_Sword;
typedef unsigned long	Elf32_Word;
#else /* ! _MIPS_SZLONG == 32 */
typedef __uint32_t	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef __uint32_t	Elf32_Off;
typedef __int32_t	Elf32_Sword;
typedef __uint32_t	Elf32_Word;
#endif	/* ! _MIPS_SZLONG == 32 */

typedef unsigned char	Elf32_Byte;	
typedef unsigned short	Elf32_Section;	

/* 64 bit data types */
typedef __uint64_t	Elf64_Addr;
typedef unsigned short	Elf64_Half;
typedef __uint64_t	Elf64_Off;
typedef __int32_t	Elf64_Sword;
typedef __int64_t	Elf64_Sxword;
typedef __uint32_t	Elf64_Word;
typedef __uint64_t	Elf64_Xword;
typedef unsigned char	Elf64_Byte;	/* unsigned tiny integer */
typedef unsigned short	Elf64_Section;	/* section index (unsigned) */

#endif

#endif /* __SYS_ELFTYPES_H__ */
