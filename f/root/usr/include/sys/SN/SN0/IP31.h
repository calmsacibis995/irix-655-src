/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1992-1997, Silicon Graphics, Inc.          *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

#ifndef __SYS_SN_SN0_IP31_H__
#define __SYS_SN_SN0_IP31_H__

/*
 * PIMM_PSC codes for values read from the MD_SLOTID_USTAT register
 * N.B.: When adding a new config, the ip31_psc_to_flash_config table in
 * sys/SN/SN0/ip27config.h needs to be updated
 */

#define	PIMM_NOT_PRESENT		0xf
#define	PIMM_2__225__R10__3_45__1__113	0xe
#define	PIMM_2__225__R10__2_60__4__225	0xd
#define	PIMM_2__300__R12__2_60__4__200	0xc
#define	PIMM_2__195__R10__3_45__4__130	0xb
#define	PIMM_2__250__R10__2_60__4__250	0xa

#define	PIMM_PSC(nasid)	\
	((REMOTE_HUB_L(nasid, MD_SLOTID_USTAT) & MSU_PIMM_PSC_MASK) >> \
	MSU_PIMM_PSC_SHFT)

#define	PIMM_PRESENT(nasid)	(PIMM_PSC(nasid) != PIMM_NOT_PRESENT)

#endif /* __SYS_SN_SN0_IP31_H__ */
