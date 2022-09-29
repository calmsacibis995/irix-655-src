/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1998, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/* $Revision: 1.2 $    $Date: 1999/04/30 21:41:58 $ */

/*
 * st_ifnet.h
 *
 * This file contains structures and defines for the Scheduled Transfer 
 * (ST) protocol interface to device drivers.
 * 
 */

/* Current as of 11/4/97 struct spec. */

#ifndef ___ST_IFNET_H
   #define ___ST_IFNET_H

#include <netinet/st.h> /* needed for the st_hdr_t struct */
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
/*
 * ULP Bufx Flags
 */
#define ST_BUFX_ALTDIDN (1<<0)
#define ST_BUFX_SRC	(1<<1)
#define ST_BUFX_DST	(1<<2)

/*
 * ULP Desc flags
 */
#define IF_ST_TX_RAW         (1<<0)
#define IF_ST_TX_CTL_MSG     (1<<1)
#define IF_ST_TX_ADDR_PADDR  (1<<2)
#define IF_ST_TX_OPT_PAYLOAD (1<<3)
#define IF_ST_TX_CONN_MSG    (1<<4)
#define IF_ST_TX_ACK         (1<<5)
#define IF_ST_USER_FIFO      (1<<6)
#define IF_ST_TX_MAC_ADDR    (1<<7)

/*
 * ULP common structs
 */

typedef struct st_machdr_s {
        uint64_t        dst_mac;    /* MAC opaque cookie */
        uint64_t        src_mac;    /* MAC opaque cookie */
} st_macaddr_t;


typedef struct st_io_s {
        struct inaddrpair iap;
        st_macaddr_t      stmac;
        sthdr_t           sth;
} st_io_t;


typedef struct st_mx_s {
    uint32_t    base_spray      :3,
                                :3,
                bufsize         :6,
                bufx_base       :20;
    uint32_t    key;
    uint32_t    bufx_range      :16,
                                :16;
    uint32_t    flags           :4,
                poison          :1,
                port            :11,
                stu_num         :16;
} st_mx_t;

typedef struct st_port_s {
 	/*-- TX Parameters --*/
	uint32_t bufx_base; 
	uint32_t bufx_range    	:16,
				:10,
		src_bufsize   	:6;  /* as a pow of 2 */
#define MAX_VC 4
	uchar_t	 vc_fifo_credit[MAX_VC];
	/*-- RX Parameters --*/
	uint32_t key;
	uint32_t fw_assist_ctl :8, /* 8 bits matching the hardware port entry */
				:3,
		 xp            	:1,
		 didn          	:4,
				:16;
	uint32_t ddq_size;  /* in bytes */ 
	uint64_t ddq_addr;  /* destination descriptor Q */
} st_port_t; 

typedef struct if_st_tx_desc {
	uint32_t	vc : 2,
			flags : 30;
	int32_t		dst_bufsize;
	int32_t		max_STU;
	int32_t		len;
	uint32_t	token;
	int32_t		bufx;
	uint32_t	offset;
	caddr_t     	addr;      
	uint64_t	dst_MAC;   /* cookie for dst MAC addr - opaque to ST */
} if_st_tx_desc_t;


/* Transmit descriptor: st_fw_tx_desc_t */
/* Numbered names are bit position numbers within quad word. */
typedef union st_fw_tx_desc_u {

    /*
     * TX descriptor format for packets of type OP != DATA.
     * This differs from the GSN version because 800 needs a valid bit. The
     * opt_payload field gets obsoleted by adding extracted_upkts so it is
     * now the valid bit.
     */
    struct {
	uint32_t	raw		: 1,
	        	control_op	: 1,
	        	int_when_done	: 1,
	        	unused_124_120	: 5,
			extracted_upkts : 2, /* only for 800 */
			unused_117_112	: 6,
	        	valid		: 1, /* replaces opt_payload */
	        	unused_110_96	: 15;
	uint32_t	unused_95_32[2];
	uint32_t	tx_done;
#define GSN_ST_ULA_SIZE 6
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint16_t	unused_79_64;
	st_hdr_t	st_hdr;
	uint32_t	payload[8];
	uint32_t	unused[7];
	uint32_t	d_ifield;
    } ctl_desc;

    /* 
     * TX descriptor format for packets of type OP == DATA.
     * This differs from GSN version because 800 needs a valid bit.
     * extracted_upkts is now part of all descs and unused_111 becomes valid.
     */
    struct {
	uint32_t	raw		: 1,
			control_op	: 1,
			int_when_done	: 1,
			max_stu_size	: 5,
			extracted_upkts	: 2, /* only for 800 */
			dest_bufsize	: 6,
			valid		: 1, /* replaces unused_111 */
			src_offset_hi	: 15;
	uint32_t	src_offset_lo	: 7,
#define GET_SRC_OFFSET(desc) (desc.src_offset_hi << 7 | desc.src_offset_lo)
#define SET_SRC_OFFSET(desc,val) {desc.src_offset_hi = (val & 0x3fffff) >> 7; \
				  desc.src_offset_lo = val & 0x7f;}

			length		: 25;
	uint32_t	src_bufx;
	uint32_t	tx_done;
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint16_t	unused_79_64;
	st_hdr_t	st_hdr;
	uint32_t	append_payload[15];
	uint32_t	d_ifield;
    } data_desc;

    /* TX descriptor format for the data and ctl common fields. */
    struct {
	uint32_t	raw		: 1,
	        	control_op	: 1,
	        	int_when_done	: 1,
			unused_124_120	: 5,
			extracted_upkts	: 2,
			unused_117_112	: 6,
			valid		: 1,
			unused_110_96	: 15;
	uint32_t	unused_95_32[2];
	uint32_t	tx_done;
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint16_t	unused_79_64;
	st_hdr_t	st_hdr;
	uint32_t	unused[15];
	uint32_t	d_ifield;
    } common_desc;

    uint32_t	w[32];
    uint64_t	dw[16];
} st_fw_tx_desc_t;

/* Receive descriptor: st_fw_rx_desc_t */
/* Numbered names are bit position numbers within quad word. */
typedef union st_fw_rx_desc_u {
    /* RX descriptor format for packets of type OP != DATA. */
    struct {
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint8_t		s_ula[GSN_ST_ULA_SIZE];
	uint32_t	m_len;
	uint8_t		seq		: 4,
	        	vc		: 4;
	uint8_t    	ssap;
	uint8_t 	ctl;
	uint8_t        	org[3];
	uint16_t       	ether_type;
	st_hdr_t	st_hdr;
	uint32_t	i_bufx;
	uint32_t	i_offset;
	uint32_t	opt_payload[7];
    } ctl_desc;

    /* RX descriptor format for packets of type OP == DATA. */
    struct {
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint8_t		s_ula[GSN_ST_ULA_SIZE];
	uint32_t	m_len;
	uint8_t		seq		: 4,
	        	vc		: 4;
	uint8_t    	ssap;
	uint8_t 	ctl;
	uint8_t        	org[3];
	uint16_t       	ether_type;
	st_hdr_t	st_hdr;
	uint32_t	opaque;
	uint32_t	append_payload[16];
    } data_desc;

    struct {
	uint8_t		d_ula[GSN_ST_ULA_SIZE];
	uint8_t		s_ula[GSN_ST_ULA_SIZE];
	uint32_t	m_len;
	uint8_t		seq		: 4,
	        	vc		: 4;
	uint8_t    	ssap;
	uint8_t 	ctl;
	uint8_t        	org[3];
	uint16_t       	ether_type;
	st_hdr_t	st_hdr;
	uint32_t	unused[17];
    } common_desc;

    /* XXX admin is a 6400-PH type so not needed here right? */
    
    uint32_t	w[32];
    uint64_t	dw[16];
} st_fw_rx_desc_t;

typedef struct st_spray {
    uint32_t bufx_base;
    int32_t  bufx_num;
} st_spray_t;


typedef struct st_ifnet_s {
	uint32_t		flags;
#define	ST_IF_CKSUM		1 << 0
#define	ST_IF_OPT_PAYLOAD	1 << 1
#define	ST_IF_STU		1 << 2
#define	ST_IF_2_XTALK		1 << 3
#define MAX_SPRAY 16
	st_spray_t	spray[MAX_SPRAY];
	int32_t	port_base_standard;
	int32_t	port_num_standard;
	int32_t	port_base_opt_payload;
	int32_t	port_num_opt_payload;
	int32_t	mx_base;
	int32_t	mx_num;
#define MAX_SLOT_VC 4
	int32_t	slot_vc[MAX_SLOT_VC];
	uint32_t	tx_bufsize_vec;
	uint32_t	rx_bufsize_vec;
	int32_t	tx_max_stu;
	int32_t	rx_max_stu;
	int32_t	xmit_desc_ifhdr_off;
	int32_t	xmit_desc_mac_desc_off;
	int32_t	xmit_desc_sthdr_off;
	int32_t	xmit_desc_opt_payload_off;
	int32_t	xmit_desc_opt_payload_len;
	int32_t	xmit_desc_raw_off;
	int32_t	bufx_offset_align;

	int32_t	(*if_st_get_port)(struct ifnet *, int32_t,st_port_t *);
	int32_t	(*if_st_set_port)(struct ifnet *, int32_t,st_port_t *);
	int32_t	(*if_st_clear_port)(struct ifnet *, int);
	int32_t	(*if_st_get_mx)(struct ifnet *, int32_t, st_mx_t *);
	int32_t	(*if_st_set_mx)(struct ifnet *, int32_t, st_mx_t *);
	int32_t	(*if_st_clear_mx)(struct ifnet *, int32_t);
	int32_t	(*if_st_get_bufx)(struct ifnet *, uint32_t, 
				paddr_t *, int32_t, uint32_t);
	int32_t	(*if_st_set_bufx)(struct ifnet *, uint32_t, 
				paddr_t *, int32_t, int32_t, uint32_t, int32_t);
	int32_t	(*if_st_clear_bufx)(struct ifnet *, 
				uint32_t, int32_t, uint32_t);

	int32_t	(*if_st_mac_cmp)(caddr_t);
        int32_t	(*if_st_get_nxt_desc)();		/* TO DO: */
} st_ifnet_t;

#endif /* ___ST_IFNET_H */
