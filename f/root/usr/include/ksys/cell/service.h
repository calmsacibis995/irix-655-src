/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef	_KSYS_SERVICE_H_
#define	_KSYS_SERVICE_H_	1
#ident "$Id: service.h,v 1.11 1999/05/14 20:13:13 lord Exp $"

#ifndef CELL_CAPABLE
#error included by non-CELL configuration
#endif

/*
 * Definitions relating to distributed services
 */

/*
 * Service numbers. 
 */
#define	SVC_VNODE		0
#define SVC_KORE		1
#define SVC_WPD			2
#define SVC_CREDID		3
#define	SVC_CMSID		4
#define	SVC_EXIM		5
#define	SVC_VFS			6
#define	SVC_CELL_TEST		7
#define	SVC_CORPSE		8
#define SVC_XVM			9
#define	SVC_MESG		10
#define SVC_MAX			10

#define NUMSVCS			(SVC_MAX+1)

#define	SVC_NONE		(-1)

extern char *svc_id_string[NUMSVCS];
#define SVC_ID_STRINGS {	\
	"SVC_VNODE",		\
	"SVC_KORE",		\
	"SVC_WPD",		\
	"SVC_CREDID",		\
	"SVC_CMSID",		\
	"SVC_EXIM",		\
	"SVC_VFS",		\
	"SVC_CELL_TEST",	\
	"SVC_CORPSE",		\
	"SVC_XVM",		\
	"SVC_MESG"		\
}

/*
 * We make this a struct so that it is harder to pass a cell number off
 * as a service_t.
 */
typedef struct {
	union {
		struct {
			short	cell;
			short	svcnum;
		} struct_value;
		int	int_value;
	} un;
} service_t;

#define	s_cell		un.struct_value.cell
#define	s_svcnum	un.struct_value.svcnum
#define	s_intval	un.int_value

#define SERVICE_MAKE(svc, cell, svcnum)				\
        { 							\
	        (svc).s_cell = cell; 				\
		(svc).s_svcnum = svcnum; 			\
	}

#define SERVICE_MAKE_NULL(svc)		(svc).s_cell = -1
#define SERVICE_IS_NULL(svc)		((svc).s_cell == -1)

#define	SERVICE_TO_CELL(svc)		(svc).s_cell
#define SERVICE_TO_SVCNUM(svc)		(svc).s_svcnum
#define SERVICE_EQUAL(svc1, svc2)	((svc1).s_intval == (svc2).s_intval)

#define	SERVICE_TO_WP_VALUE(svc)	((svc).s_intval)
#define SERVICE_FROMWP_VALUE(svc, val)	((svc).s_intval = (val))

#endif	/* _KSYS_SERVICE_H_ */
