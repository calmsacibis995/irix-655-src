/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/


#ifndef __MISER_DEBUG_HEADER__
#define __MISER_DEBUG_HEADER__

#include "miser_private.h"
#include "iostream.h"
#include "Debug.h"
id_type_t read_id(istream& is);

istream& operator>> (istream& is, id_type_t& id);
ostream& operator<< (ostream& os, space& queue);
istream& operator>> (istream& is, miser_seg_t& segment);
ostream& operator<< (ostream& os, miser_seg_t& segment);
istream& operator>> (istream& os, miser_qseg_t& resource);
ostream& operator<< (ostream& os, miser_qseg_t& resource);
#endif
