#ifndef __NLIST_H__
#define __NLIST_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Copyright (C) 1989 Silicon Graphics, Inc. All rights reserved.  */
/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991, 1990 MIPS Computer Systems, Inc.      |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 252.227-7013.  |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Avenue                               |
 * |         Sunnyvale, California 94088-3650, USA             |
 * |-----------------------------------------------------------|
 */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/nlist.h,v 7.6 1994/06/03 00:22:52 bhaskar Exp $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <sgidefs.h>

#ifdef __mips
struct nlist {
    char *n_name;
    unsigned long n_value;
    short n_type;		/* 0 if not there, 1 if found */
    short reserved;
};

/* A clone of struct nlist with the n_value widened to 64-bits. */
struct nlist64 {
    char *n_name;
    __uint64_t n_value;
    short n_type;
    short reserved;
};
#endif	/* __mips */

extern int	nlist(const char *, struct nlist *);
extern int	nlist64(const char *, struct nlist64 *);

#ifdef __cplusplus
}
#endif

#endif	/* _NLIST_H */
