#ifndef __SYS_VOLUME_H__
#define __SYS_VOLUME_H__

/**************************************************************************
 *                                                                        *
 *            Copyright (C) 1999, Silicon Graphics, Inc.                  *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/
#ident "$Revision: 1.1 $"

/*
 * Subvolume Types for all volume managers.
 * 
 * There is a maximum of 255 subvolumes. 0 is reserved.
 *	Note:  SVTYPE_LOG, SVTYPE_DATA, SVTYPE_RT values matches XLV.
 *	       Do not change - Colin Ngam
 */
typedef enum sv_type_e {
	SVTYPE_ALL		=0,	 /* special: denotes all sv's */
	SVTYPE_LOG	 	=1,	 /* XVM Log subvol type */
	SVTYPE_DATA,			 /* XVM Data subvol type */
	SVTYPE_RT,			 /* XVM Real Time subvol type */
	SVTYPE_SWAP,			 /* swap area */
	SVTYPE_RSVD5,			 /* reserved 5 */
	SVTYPE_RSVD6,			 /* reserved 6 */
	SVTYPE_RSVD7,			 /* reserved 7 */
	SVTYPE_RSVD8,			 /* reserved 8 */
	SVTYPE_RSVD9,			 /* reserved 9 */
	SVTYPE_RSVD10,			 /* reserved 10 */
	SVTYPE_RSVD11,			 /* reserved 11 */
	SVTYPE_RSVD12,			 /* reserved 12 */
	SVTYPE_RSVD13,			 /* reserved 13 */
	SVTYPE_RSVD14,			 /* reserved 14 */
	SVTYPE_RSVD15,			 /* reserved 15 */
	SVTYPE_USER1,			 /* First User Defined Subvol Type */
	SVTYPE_LAST		=255
} sv_type_t;

#endif /* __SYS_VOLUME_H__ */
