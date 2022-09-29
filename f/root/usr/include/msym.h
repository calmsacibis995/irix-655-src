#ifndef __CMPLRS_MSYM_H__
#define __CMPLRS_MSYM_H__

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <sgidefs.h>
#include <elf.h>
/*
 * ".msym" structure
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
typedef struct
{
	Elf32_Word	ms_hash_value;
	Elf32_Word	ms_info;
} Elf32_Msym;
#endif


/*
 * ms_info
 */

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))
#define ELF32_MS_REL_INDEX(i)	((i) >> 8)
#define ELF32_MS_FLAGS(i)	((i) & 0xff)
#define ELF32_MS_INFO(r,f)	(((r) << 8) + ((f) & 0xff))

#define ELF64_MS_REL_INDEX      ELF32_MS_REL_INDEX
#define ELF64_MS_FLAGS          ELF32_MS_FLAGS
#define ELF64_MS_INFO           ELF32_MS_INFO

#endif

#endif /* __CMPLRS_MSYM_H__ */
