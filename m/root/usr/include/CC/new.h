/*ident	"@(#)ctrans:incl-master/const-headers/new.h	1.4" */
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1991 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#ifndef __NEW_H
#define __NEW_H

#ifndef __STDDEF_H
#include <stddef.h>
#endif

extern void (*set_new_handler (void(*)()))();

#if _MIPS_SIM != _MIPS_SIM_ABI32
inline void *operator new(size_t, void* p) { return p; }
#else
void *operator new(size_t, void*);
#endif

#if __EDG_ABI_COMPATIBILITY_VERSION >= 229
#if _MIPS_SIM != _MIPS_SIM_ABI32
inline void *operator new[](size_t, void* p) { return p; }
#else
void *operator new[](size_t, void*);
#endif
#endif

#endif
