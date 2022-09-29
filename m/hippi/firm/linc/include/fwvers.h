/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/


#define SRC_HC_OFFSET       0x21100 /* where in src SDRAM to find hc_cmd
				       area */
#define DST_HC_OFFSET       0x21100 /* where in dst SDRAM to find hc_cmd
				       area */

#define HIP_VERSION_MAJOR	3
#define HIP_VERSION_MINOR	22

typedef union {
    struct {
	char		is_src;		/* 1 for src, 0 for dst */
	uchar_t		major;      /* major part of rev */
	ushort_t	minor;      /* minor part of rev */
    } parts;
    uint_t	whole;
} hippi_linc_fwvers_t;

#ifndef _KERNEL
hippi_linc_fwvers_t hippi_srcvers = {1, HIP_VERSION_MAJOR, HIP_VERSION_MINOR};
hippi_linc_fwvers_t hippi_dstvers = {0, HIP_VERSION_MAJOR, HIP_VERSION_MINOR};
hippi_linc_fwvers_t hippi_lincpromvers = {0, HIP_VERSION_MAJOR, HIP_VERSION_MINOR};
#endif
