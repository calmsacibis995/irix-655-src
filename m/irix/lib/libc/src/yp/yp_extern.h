/**************************************************************************
 *									  *
 * Copyright (C) 1986-1993 Silicon Graphics, Inc.			  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef __YP_EXTERN_H__
#define __YP_EXTERN_H__

/* yp_bind.c */
extern struct timeval _ypserv_timeout;
extern unsigned int _ypsleeptime;

/* ypxdr.c */
extern bool_t xdr_yp_binding(XDR *, struct ypbind_binding *);

#endif
