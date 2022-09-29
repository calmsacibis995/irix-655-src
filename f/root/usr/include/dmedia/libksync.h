
/*****************************************************************************
*
*  Copyright 1996, Silicon Graphics, Inc.
*  All Rights Reserved.
*
*  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
*  the contents of this file may not be disclosed to third parties, copied or
*  duplicated in any form, in whole or in part, without the prior written
*  permission of Silicon Graphics, Inc.
*
*  RESTRICTED RIGHTS LEGEND:
*  Use, duplication or disclosure by the Government is subject to restrictions
*  as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
*  and Computer Software clause at DFARS 252.227-7013, and/or in similar or
*  successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
*  rights reserved under the Copyright Laws of the United States.
* 
*****************************************************************************/

#ifndef __LIBKSYNC_H__
#define __LIBKSYNC_H__


#include <sys/ksync.h> /* For kstat_t */

typedef struct ksync_user_s {
    kstat_t status;
    int     module_id;
} ksync_user_t; 


#ifdef __cplusplus
extern "C" {
#endif

int     ksyncstat( 
	    ksync_user_t    *buf, 
	    int             bufSz );        /* in bytes */


int     ksyncset( char *kName, int module_id );

#ifdef __cplusplus
}
#endif


#endif /* __LIBKSYNC_H__ */
