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
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/obj_list.h,v 7.5 1993/06/08 01:16:58 bettina Exp $ */

#ifndef __OBJ_LIST_H__
#define __OBJ_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct obj_list {
	unsigned long	data;
	struct obj_list	*next;
	struct obj_list	*prev;			/* back link */
} objList;

#define LIST_BEGINNING	0
#define LIST_END	1
#define LIST_ADD_BEFORE         2
#define LIST_ADD_AFTER          3
#define LIST_DELETE             4
#define LIST_REPLACE            5

#ifdef __cplusplus
}
#endif

#endif	/* __OBJ_LIST_H__ */
