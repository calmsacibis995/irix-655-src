
/************************************************************************** 
 *                                                                        *
 *               Copyright (C) 1995, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/*
 * $Revision: 1.2 $
 */

#ifndef _NIC_FORMAT_H_
#define _NIC_FORMAT_H_

/* Dallas Semiconductor defined NIC serial number format. */

typedef union nic_sernum {
    struct {
	/* Note: Not in order extracted from DS2505 NIC. */
        char serial[6];  /* serial[5] = MSB, serial[0] = LSB */
        char family;
        char crc_passed;  /* Boolean, not actual CRC. */
    } fmt;
    struct {
        int data[2];
    } raw;
} nic_sernum_t;


#define NIC_PAGE_SIZE      32 /* NIC page size in bytes. */
#define NIC_PAGE_A          0 /* Potentially "redirected" page # for page A info. */
#define NIC_PAGE_B          1 /* Potentially "redirected" page # for page B info. */

/* SGI-defined format for page A of DS2505 NIC. */

#define SGI_FORMAT_INVALID  0xff
#define SGI_FORMAT_1        0x01

typedef union nic_page_a {
    struct {
        char format;		/* SGI NIC format revision code */
        char sgi_ser_num[10];	/* SGI ASCII serial number */
        char sgi_part_num[19];	/* SGI ASCII part number */
        short crc;		/* 16-bit CRC: X^16 + X^15 + 1 */
    } sgi_fmt_1;
    struct {
        int data[8];
    } raw;
} nic_page_a_t;

/* SGI-defined format for page B of DS2505 NIC. */

typedef union nic_page_b {
    struct {
        char dash_lvl[6];	/* Dash-level */
        char rev_code[4];		/* Revision code */
        char grp_code;		/* Group code */
        char capability[4];	/* Capability code */
        char variety;		/* Variety code */
        char name[14];		/* ASCII name field */
        short crc;   		/* 16-bit CRC: X^16 + X^15 + 1 */
    } sgi_fmt_1;
    struct {
        int data[8];
    } raw;
} nic_page_b_t;

#define NIC_BAD_SERNUM       0x1
#define NIC_BAD_PAGE_A       0x2
#define NIC_BAD_PAGE_B       0x4

/* Package of extracted DS2505 information. */

typedef struct nic_info {
    int flags;                  /* Set to NIC_BAD_x if section x mangled. */
    nic_sernum_t sernum;
    nic_page_a_t page_a;
    nic_page_b_t page_b;
} nic_info_t;

#endif /* _NIC_FORMAT_H_ */
