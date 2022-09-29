/* FILE: data.c */
#include "sys/qlfc.h"


/* REFERENCED */
static	int		qlfc_mikey = 0;

int	qlfc_devflag = D_MP | D_ASYNC_ATTACH;


static	int		qlfc_ctlr_number;

static	int		qlfc_use_full_duplex = 0;
static	int		qlfc_use_fabric_infrastructure = 1;

/*
** Externs defined in master.d/qlfc
*/
extern	int		qlfc_use_connection_mode;
extern	int     	qlfc_debug;
extern	int		qlfc_watchdog_time;
extern	int		qlfc_probe_wait_loop_up;
extern	int		qlfc_trace_buffer_entries;

/* REFERENCED */
static	int		qlfc_controller_sz = sizeof(QLFC_CONTROLLER);
/* REFERENCED */
static	int		qlfc_target_map_sz = sizeof(target_map_t);
/* REFERENCED */
static	int		qlfc_target_map_array_sz = sizeof(target_map_t)*QL_MAX_LOOP_IDS;
/* REFERENCED */
static	int		qlfc_local_lun_info_sz = sizeof(qlfc_local_lun_info_t);
/* REFERENCED */
static	int		qlfc_local_targ_info_sz = sizeof(qlfc_local_targ_info_t);

/*
** qLogic Fibre Channel Adapters supported by this driver.
**	Null terminated.
*/

       uint16_t qLogicDeviceIds[] = {
#ifdef QL2100
			QLOGIC_2100,
#endif
			QLOGIC_2200,
			0xffff
			};

       QLFC_CONTROLLER *qlfc_controllers;		/* list of controllers */
static QLFC_CONTROLLER *qlfc_last_controller;

static mutex_t qlfc_attach_semaphore;

#define QLFC_CDB_STRING_LEN	(9)
static char *qlfc_cdb_string =
	"test-rdy"	"\0"
	"rezero  "	"\0"
	"02      "	"\0"
	"req-sens"	"\0"
	"format  "	"\0"
	"05      "	"\0"
	"06      "	"\0"
	"reass-bk"	"\0"
	"read    "	"\0"
	"09      "	"\0"
	"write   "	"\0"
	"seek    "	"\0"
	"0c      "	"\0"
	"0d      "	"\0"
	"0e      "	"\0"
	"0f      "	"\0"
	"10      "	"\0"
	"11      "	"\0"
	"inquiry "	"\0"
	"13      "	"\0"
	"14      "	"\0"
	"modesel "	"\0"
	"reserve "	"\0"
	"release "	"\0"
	"18      "	"\0"
	"19      "	"\0"
	"modesens"	"\0"
	"strt/stp"	"\0"
	"rcv-diag"	"\0"
	"snd-diag"	"\0"
	"1e      "	"\0"
	"1f      "	"\0"
	"20      "	"\0"
	"21      "	"\0"
	"22      "	"\0"
	"23      "	"\0"
	"24      "	"\0"
	"readcap "	"\0"
	"26      "	"\0"
	"27      "	"\0"
	"read    "	"\0"
	"29      "	"\0"
	"write   "	"\0"
	"seek    "	"\0"
	"2c      "	"\0"
	"2d      "	"\0"
	"wrt-vrfy"	"\0"
	"verify  "	"\0"
	"30      "	"\0"
	"31      "	"\0"
	"32      "	"\0"
	"33      "	"\0"
	"prefetch"	"\0"
	"synccach"	"\0"
	"36      "	"\0"
	"rd-defct"	"\0"
	"38      "	"\0"
	"39      "	"\0"
	"3a      "	"\0"
	"wrt-buf "	"\0"
	"readbuf "	"\0"
	"3d      "	"\0"
	"readlong"	"\0"
	"wrt-long"	"\0"
	"40      "	"\0"
	"wrt-same"	"\0"
	"42      "	"\0"
	"43      "	"\0"
	"44      "	"\0"
	"45      "	"\0"
	"46      "	"\0"
	"47      "	"\0"
	"48      "	"\0"
	"49      "	"\0"
	"4a      "	"\0"
	"4b      "	"\0"
	"logsel  "	"\0"
	"logsense"	"\0"
	"4e      "	"\0"
	"4f      "	"\0"
	"XDWrite "	"\0"
	"XPWrite "	"\0"
	"XDRead  "	"\0"
	"53      "	"\0"
	"54      "	"\0"
	"modesel "	"\0"
	"reserve "	"\0"
	"release "	"\0"
	"58      "	"\0"
	"59      "	"\0"
	"modesens"	"\0"
	"5b      "	"\0"
	"5c      "	"\0"
	"5d      "	"\0"
	"5e      "	"\0"
	"5f      "	"\0"
	"60      "	"\0"
	"61      "	"\0"
	"62      "	"\0"
	"63      "	"\0"
	"64      "	"\0"
	"65      "	"\0"
	"66      "	"\0"
	"67      "	"\0"
	"68      "	"\0"
	"69      "	"\0"
	"6a      "	"\0"
	"6b      "	"\0"
	"6c      "	"\0"
	"6d      "	"\0"
	"6e      "	"\0"
	"6f      "	"\0"
	"70      "	"\0"
	"71      "	"\0"
	"72      "	"\0"
	"73      "	"\0"
	"74      "	"\0"
	"75      "	"\0"
	"76      "	"\0"
	"77      "	"\0"
	"78      "	"\0"
	"79      "	"\0"
	"7a      "	"\0"
	"7b      "	"\0"
	"7c      "	"\0"
	"7d      "	"\0"
	"7e      "	"\0"
	"7f      "	"\0"
	"XDWrite "	"\0"
	"rebuild "	"\0"
	"regen   "	"\0"
	"83      "	"\0"
	"84      "	"\0"
	"85      "	"\0"
	"86      "	"\0"
	"87      "	"\0"
	"88      "	"\0"
	"89      "	"\0"
	"8a      "	"\0"
	"8b      "	"\0"
	"8c      "	"\0"
	"8d      "	"\0"
	"8e      "	"\0"
	"8f      "	"\0"
	"90      "	"\0"
	"91      "	"\0"
	"92      "	"\0"
	"93      "	"\0"
	"94      "	"\0"
	"95      "	"\0"
	"96      "	"\0"
	"97      "	"\0"
	"98      "	"\0"
	"99      "	"\0"
	"9a      "	"\0"
	"9b      "	"\0"
	"9c      "	"\0"
	"9d      "	"\0"
	"9e      "	"\0"
	"9f      "	"\0"
	"rpt-luns"	"\0"
	"a1      "	"\0"
	"a2      "	"\0"
	"a3      "	"\0"
	"a4      "	"\0"
	"a5      "	"\0"
	"a6      "	"\0"
	"a7      "	"\0"
	"a8      "	"\0"
	"a9      "	"\0"
	"aa      "	"\0"
	"ab      "	"\0"
	"ac      "	"\0"
	"ad      "	"\0"
	"ae      "	"\0"
	"af      "	"\0"
	"b0      "	"\0"
	"b1      "	"\0"
	"b2      "	"\0"
	"b3      "	"\0"
	"b4      "	"\0"
	"b5      "	"\0"
	"b6      "	"\0"
	"rd-defct"	"\0"
	"b8      "	"\0"
	"b9      "	"\0"
	"ba      "	"\0"
	"bb      "	"\0"
	"bc      "	"\0"
	"bd      "	"\0"
	"be      "	"\0"
	"bf      "	"\0"
	"c0      "	"\0"
	"c1      "	"\0"
	"c2      "	"\0"
	"c3      "	"\0"
	"c4      "	"\0"
	"c5      "	"\0"
	"c6      "	"\0"
	"c7      "	"\0"
	"c8      "	"\0"
	"c9      "	"\0"
	"ca      "	"\0"
	"cb      "	"\0"
	"cc      "	"\0"
	"cd      "	"\0"
	"ce      "	"\0"
	"cf      "	"\0"
	"d0      "	"\0"
	"d1      "	"\0"
	"d2      "	"\0"
	"d3      "	"\0"
	"d4      "	"\0"
	"d5      "	"\0"
	"d6      "	"\0"
	"d7      "	"\0"
	"d8      "	"\0"
	"d9      "	"\0"
	"da      "	"\0"
	"db      "	"\0"
	"dc      "	"\0"
	"dd      "	"\0"
	"de      "	"\0"
	"df      "	"\0"
	"e0      "	"\0"
	"e1      "	"\0"
	"e2      "	"\0"
	"e3      "	"\0"
	"e4      "	"\0"
	"e5      "	"\0"
	"e6      "	"\0"
	"e7      "	"\0"
	"e8      "	"\0"
	"e9      "	"\0"
	"ea      "	"\0"
	"eb      "	"\0"
	"ec      "	"\0"
	"ed      "	"\0"
	"ee      "	"\0"
	"ef      "	"\0"
	"f0      "	"\0"
	"f1      "	"\0"
	"f2      "	"\0"
	"f3      "	"\0"
	"f4      "	"\0"
	"f5      "	"\0"
	"f6      "	"\0"
	"f7      "	"\0"
	"f8      "	"\0"
	"f9      "	"\0"
	"fa      "	"\0"
	"fb      "	"\0"
	"fc      "	"\0"
	"fd      "	"\0"
	"fe      "	"\0"
	"ff      "	"\0"
	;



static char *qlfc_sr_status_string[] = {
	"SC_GOOD",
	"SC_TIMEOUT",
	"SC_HARDERR",
	"SC_PARITY",
	"SC_MEMERR",
	"SC_CMDTIME",
	"SC_ALIGN",
	"SC_ATTN",
	"SC_REQUEST"
	};

/*
** Which firmware to load for a specific controller.
**	Must be in the same order as qLogicDeviceIds!
*/

/*	[ctlr_type_index]		*/

#ifdef QL2100
extern unsigned short ql2100_extended_lun;
extern unsigned short ql2100_fc_tape;
extern unsigned short ql2100_risc_code_length01;
extern unsigned short ql2100_risc_code_version;
extern unsigned char  ql2100_firmware_version[];
extern unsigned short ql2100_risc_code_addr01;
extern unsigned short ql2100_risc_code01[];
#endif

extern unsigned short ql2200_extended_lun;
extern unsigned short ql2200_fc_tape;
extern unsigned short ql2200_risc_code_length01;
extern unsigned short ql2200_risc_code_version;
extern unsigned char  ql2200_firmware_version[];
extern unsigned short ql2200_risc_code_addr01;
extern unsigned short ql2200_risc_code01[];

static qLogicFirmware_t ql_firmware[] = {
#ifdef QL2100
	{	/* 2100 */
		(uint16_t *) &ql2100_extended_lun,
		(uint16_t *) &ql2100_fc_tape,
		(uint16_t *) &ql2100_risc_code_version,
		(uint8_t  *) &ql2100_firmware_version[0],
		(uint16_t *) &ql2100_risc_code_addr01,
		(uint16_t *) &ql2100_risc_code01[0],
		(uint16_t *) &ql2100_risc_code_length01,
	},
#endif

	{	/* 2200 */
		(uint16_t *) &ql2200_extended_lun,
		(uint16_t *) &ql2200_fc_tape,
		(uint16_t *) &ql2200_risc_code_version,
		(uint8_t  *) &ql2200_firmware_version[0],
		(uint16_t *) &ql2200_risc_code_addr01,
		(uint16_t *) &ql2200_risc_code01[0],
		(uint16_t *) &ql2200_risc_code_length01,
	}
};

/* FILE: dump.c */
/*
 * qlfc_dump - called prior to doing a panic dump
 *
 * We do have to reset the locks however, in case the panic
 * occured while a qlogic device was active in this driver.
 *
 * The actual dumping will be done by calls to qlfc_command() later.
 *      called at spl7()
 */
/*ARGSUSED*/
int
qlfc_dump(vertex_hdl_t 	qlfc_ctlr_vhdl)
{
#define DUMP_SUCCESS 1
#define DUMP_FAIL    0

#if 0
    	int		rv;
    	QLFC_CONTROLLER *ctlr;
	qlfc_local_lun_info_t *qli;
	unsigned 	id, lun;
	vertex_hdl_t	ql_lun_vhdl;

	/*
	 * Interrupts are disabled, so set DUMPING flag for all
	 * adapters. This becomes important when qlfc_halt is called with
	 * interrupts disabled.
	 */
	for (ctlr = qlfc_controllers; ctlr; ctlr = ctlr->next) {
		atomicSetInt(&ctlr->flags, CFLG_DUMPING);
	}

	ctlr = qlfc_ctlr_info_from_ctlr_get(qlfc_ctlr_vhdl);
    	if (ctlr == NULL) {
		qlfc_cpmsg(qlfc_ctlr_vhdl, "Dump failed; no host adapter structure\n");
		return(DUMP_FAIL);
    	}

	INITLOCK (&ctlr->req_lock, "ql", qlfc_ctlr_vhdl);
	INITLOCK (&ctlr->res_lock, "ql", qlfc_ctlr_vhdl);
	INITLOCK (&ctlr->mbox_lock, "ql_mbox_lock", qlfc_ctlr_vhdl);
	INITLOCK (&ctlr->waitQ_lock, "ql_waitQ_lock", qlfc_ctlr_vhdl);
	INITLOCK (&ctlr->probe_lock, "ql_probe_lock", qlfc_ctlr_vhdl);

        QL_MUTEX_ENTER(ctlr);
        qlfc_reset_interface(ctlr,1);
        QL_MUTEX_EXIT(ctlr);

	/* Init the raw request queue to empty.  */
	ctlr->req_forw = NULL;
	ctlr->req_back = NULL;

	for (id = 0; id < QL_MAX_LOOP_IDS; id++) {
		/*
		 * Cleanup the target structures
		 */
		INITLOCK (&(ctlr->timeout_info[id].per_target_tlock), "ql_timeout", qlfc_ctlr_vhdl);
		IOLOCK (ctlr->timeout_info[id].per_target_tlock);
		ctlr->req_active[id] = NULL;
		ctlr->req_last[id] = NULL;
		IOUNLOCK (ctlr->timeout_info[id].per_target_tlock);

		/*
		 * Cleanup the LUN structures
		 */
		for (lun = 0; lun < QL_MAXLUNS; ++lun) {
							/* PROTOTYPE CODE */
								if (fabric_device && qlfc_use_fabric_infrastructure) {
									lun_vhdl = scsi_fabric_lun_vhdl_get(ctlr->ctlr_vhdl,
										node_name, 
										fc_short_portname(node_name,port_name), 
										lun);
								}
								else {
									lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, loop_id, lun);
								}

			if ((ql_lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, id, lun)) == GRAPH_VERTEX_NONE) XXX won't compile now -jh
				continue;
			qli = SLI_INFO(scsi_lun_info_get(ql_lun_vhdl));
			INITLOCK (&qli->qli_open_mutex, "ql_open", ql_lun_vhdl);
			INITLOCK (&qli->qli_lun_mutex, "ql_lun", ql_lun_vhdl);
			IOLOCK(qli->qli_lun_mutex);
			qli->qli_dev_flags = 0; /* was DFLG_BEHAVE_QERR1 */
			qli->qli_awaitf = qli->qli_awaitb = NULL;
			qli->qli_iwaitf = qli->qli_iwaitb = NULL;
			qli->qli_cmd_rcnt = qli->qli_cmd_awcnt = qli->qli_cmd_iwcnt = 0;
			IOUNLOCK(qli->qli_lun_mutex);
		} /* for (lun) */
	} /* for (id) */

	atomicClearInt(&(ctlr->ql_ncmd), 0xffffffff);
	rv = qlfc_do_reset(qlfc_ctlr_vhdl);
	if (rv) {
		QL_MUTEX_ENTER(ctlr);
		if (rv = qlfc_reset_interface(ctlr,1))
			atomicSetInt(&ctlr->flags, CFLG_SHUTDOWN);
		QL_MUTEX_EXIT(ctlr);
	}
	if (rv == 0) {
		for (id = 0; id < QL_MAX_LOOP_IDS; id++) {
			if (id == ctlr->ql_defaults.fw_params.hardID)
				continue;
			if ((ql_lun_vhdl = scsi_lun_vhdl_get(qlfc_ctlr_vhdl,id,0))
				!= GRAPH_VERTEX_NONE) {
				qlfc_info(ql_lun_vhdl);
			}
		}
	}
        return((rv == 0) ? DUMP_SUCCESS : DUMP_FAIL);
#else
	return DUMP_FAIL;
#endif
}

/* FILE: mbox_data.c */
#define	QLFC_MBOX_CMD_STRING_LEN	(14)
static char *mbox_cmd_string =
	"nop          "	"\0"
	"load ram     "	"\0"
	"exec firmware"	"\0"
	"dump ram     "	"\0"
	"write ram    "	"\0"
	"read ram     "	"\0"
	"mbox reg test"	"\0"
	"verify chksum"	"\0"
	"abort fw     "	"\0"
	"ld risc ram  "	"\0"
	"dump risc ram"	"\0"
	"0x0b         "	"\0"
	"0x0c         "	"\0"
	"0x0d         "	"\0"
	"chksum fw    "	"\0"
	"0x0f         "	"\0"
	"init req que "	"\0"
	"init rsp que "	"\0"
	"0x12         "	"\0"
	"wakeup       "	"\0"
	"stop firmware"	"\0"
	"abort        "	"\0"
	"abort device "	"\0"
	"abort target "	"\0"
	"rst loop trgs"	"\0"
	"stop queue   "	"\0"
	"start queue  "	"\0"
	"step queue   "	"\0"
	"abort queue  "	"\0"
	"queue status "	"\0"
	"0x1e         "	"\0"
	"firmware stat"	"\0"
	"get loop id  "	"\0"
	"0x21         "	"\0"
	"get retry cnt"	"\0"
	"0x23         "	"\0"
	"0x24         "	"\0"
	"0x25         "	"\0"
	"0x26         "	"\0"
	"0x27         "	"\0"
	"0x28         "	"\0"
	"get prt q prm"	"\0"
	"0x2a         "	"\0"
	"0x2b         "	"\0"
	"0x2c         "	"\0"
	"0x2d         "	"\0"
	"0x2e         "	"\0"
	"0x2f         "	"\0"
	"0x30         "	"\0"
	"0x31         "	"\0"
	"set retry cnt"	"\0"
	"0x33         "	"\0"
	"0x34         "	"\0"
	"0x35         "	"\0"
	"0x36         "	"\0"
	"0x37         "	"\0"
	"0x38         "	"\0"
	"get prt q prm"	"\0"
	"0x3a         "	"\0"
	"0x3b         "	"\0"
	"0x3c         "	"\0"
	"0x3d         "	"\0"
	"0x3e         "	"\0"
	"0x3f         "	"\0"
	"0x40         "	"\0"
	"0x41         "	"\0"
	"0x42         "	"\0"
	"0x43         "	"\0"
	"0x44         "	"\0"
	"0x45         "	"\0"
	"0x46         "	"\0"
	"0x47         "	"\0"
	"0x48         "	"\0"
	"0x49         "	"\0"
	"0x4a         "	"\0"
	"0x4b         "	"\0"
	"0x4c         "	"\0"
	"0x4d         "	"\0"
	"0x4e         "	"\0"
	"0x4f         "	"\0"
	"load ram 64  "	"\0"
	"dump ram 64  "	"\0"
	"0x52         "	"\0"
	"0x53         "	"\0"
	"exec iocb 64 "	"\0"
	"0x55         "	"\0"
	"0x56         "	"\0"
	"0x57         "	"\0"
	"0x58         "	"\0"
	"0x59         "	"\0"
	"0x5a         "	"\0"
	"5b           "	"\0"
	"5c           "	"\0"
	"5d           "	"\0"
	"5e           "	"\0"
	"5f           "	"\0"
	"init firmware"	"\0"
	"get init cb  "	"\0"
	"init lip     "	"\0"
	"get pos map  "	"\0"
	"get port db  "	"\0"
	"clear aca    "	"\0"
	"send tgt rst "	"\0"
	"clear tsk set"	"\0"
	"abt task set "	"\0"
	"get fw state "	"\0"
	"get port name"	"\0"
	"get link stat"	"\0"
	"init lip rst "	"\0"
	"0x6d         "	"\0"
	"send sns     "	"\0"
	"login fb port"	"\0"
	"send chng req"	"\0"
	"logout fb prt"	"\0"
	"lip flw login"	"\0"
	"73           "	"\0"
	"login loop pt"	"\0"
	"node name lst"	"\0"
	"76           "	"\0"
	"77           "	"\0"
	"78           "	"\0"
	"79           "	"\0"
	"7a           "	"\0"
	"7b           "	"\0"
	"7c           "	"\0"
	"linit        "	"\0"
	"7e           "	"\0"
	"7f           "	"\0"
	"80           "	"\0"
	"81           "	"\0"
	"82           "	"\0"
	"83           "	"\0"
	"84           "	"\0"
	"85           "	"\0"
	"86           "	"\0"
	"87           "	"\0"
	"88           "	"\0"
	"89           "	"\0"
	"8a           "	"\0"
	"8b           "	"\0"
	"8c           "	"\0"
	"8d           "	"\0"
	"8e           "	"\0"
	"8f           "	"\0"
	"90           "	"\0"
	"91           "	"\0"
	"92           "	"\0"
	"93           "	"\0"
	"94           "	"\0"
	"95           "	"\0"
	"96           "	"\0"
	"97           "	"\0"
	"98           "	"\0"
	"99           "	"\0"
	"9a           "	"\0"
	"9b           "	"\0"
	"9c           "	"\0"
	"9d           "	"\0"
	"9e           "	"\0"
	"9f           "	"\0"
	"a0           "	"\0"
	"a1           "	"\0"
	"a2           "	"\0"
	"a3           "	"\0"
	"a4           "	"\0"
	"a5           "	"\0"
	"a6           "	"\0"
	"a7           "	"\0"
	"a8           "	"\0"
	"a9           "	"\0"
	"aa           "	"\0"
	"ab           "	"\0"
	"ac           "	"\0"
	"ad           "	"\0"
	"ae           "	"\0"
	"af           "	"\0"
	"b0           "	"\0"
	"b1           "	"\0"
	"b2           "	"\0"
	"b3           "	"\0"
	"b4           "	"\0"
	"b5           "	"\0"
	"b6           "	"\0"
	"b7           "	"\0"
	"b8           "	"\0"
	"b9           "	"\0"
	"ba           "	"\0"
	"bb           "	"\0"
	"bc           "	"\0"
	"bd           "	"\0"
	"be           "	"\0"
	"bf           "	"\0"
	"c0           "	"\0"
	"c1           "	"\0"
	"c2           "	"\0"
	"c3           "	"\0"
	"c4           "	"\0"
	"c5           "	"\0"
	"c6           "	"\0"
	"c7           "	"\0"
	"c8           "	"\0"
	"c9           "	"\0"
	"ca           "	"\0"
	"cb           "	"\0"
	"cc           "	"\0"
	"cd           "	"\0"
	"ce           "	"\0"
	"cf           "	"\0"
	"d0           "	"\0"
	"d1           "	"\0"
	"d2           "	"\0"
	"d3           "	"\0"
	"d4           "	"\0"
	"d5           "	"\0"
	"d6           "	"\0"
	"d7           "	"\0"
	"d8           "	"\0"
	"d9           "	"\0"
	"da           "	"\0"
	"db           "	"\0"
	"dc           "	"\0"
	"dd           "	"\0"
	"de           "	"\0"
	"df           "	"\0"
	"e0           "	"\0"
	"e1           "	"\0"
	"e2           "	"\0"
	"e3           "	"\0"
	"e4           "	"\0"
	"e5           "	"\0"
	"e6           "	"\0"
	"e7           "	"\0"
	"e8           "	"\0"
	"e9           "	"\0"
	"ea           "	"\0"
	"eb           "	"\0"
	"ec           "	"\0"
	"ed           "	"\0"
	"ee           "	"\0"
	"ef           "	"\0"
	"f0           "	"\0"
	"f1           "	"\0"
	"f2           "	"\0"
	"f3           "	"\0"
	"f4           "	"\0"
	"f5           "	"\0"
	"f6           "	"\0"
	"f7           "	"\0"
	"f8           "	"\0"
	"f9           "	"\0"
	"fa           "	"\0"
	"fb           "	"\0"
	"fc           "	"\0"
	"fd           "	"\0"
	"fe           "	"\0"
	"ff           "	"\0"
	;

/* FILE: piotrace.c */
#ifdef PIO_TRACE
ushort
get_qlfc_reg(QLFC_CONTROLLER * ha, volatile u_short *rp) {
	register uint *logp, log_cnt;
	u_short	sval;
	sval = *rp;
	if (ha && (logp = ha->ql_log_vals) && ((((__psint_t)rp & 0x7fff) != 0x0008) || (sval != 0))) {
	    if ((log_cnt = *logp) == 0)
		log_cnt = 1;
	    logp[log_cnt++] = (((__psint_t)rp & 0x7fff) << 16) | sval;
	    if (log_cnt >= QL_LOG_CNT)
		log_cnt = 1;
	    *logp = log_cnt;
	}
	return sval;
}

void
set_qlfc_reg(QLFC_CONTROLLER * ha, volatile u_short *rp, u_short sval) {
	register uint *logp, log_cnt;
	if (ha && (logp = ha->ql_log_vals)) {
	    if ((log_cnt = *logp) == 0)
		log_cnt = 1;
	    logp[log_cnt++] = (((__psint_t)rp & 0x7fff) << 16) | (1 << 31) | sval;
	    if (log_cnt >= QL_LOG_CNT)
		log_cnt = 1;
	    *logp = log_cnt;
	}
	*rp = sval;
}

#define	QL_PCI_INH(x)		get_qlfc_reg(ha, x)
#define	QL_PCI_OUTH(x,y)	set_qlfc_reg(ha, x, y)

#else

#define	QL_PCI_INH(x)		PCI_INH(x)
#define	QL_PCI_OUTH(x,y)	PCI_OUTH(x,y)

#endif /* PIO_TRACE */

/* FILE: trace.c */
#if TRACE_ARGS != 6
	compile error - trace.c needs fix up;
#endif

void trace (QLFC_CONTROLLER *, char *, __psint_t, __psint_t, __psint_t, __psint_t, __psint_t, __psint_t);

void
trace_on (QLFC_CONTROLLER *ctlr)
{
	ctlr->trace_enabled = 1;
}

void
trace_off (QLFC_CONTROLLER *ctlr)
{
	int prev = ctlr->trace_enabled;
	trace (ctlr,"TRACING DISABLED, previous enable state: %d.\n",prev,0,0,0,0,0);
	ctlr->trace_enabled = 0;
}

void
trace_init (QLFC_CONTROLLER *ctlr)
{
	mutex_lock (&ctlr->trace_lock, PRIBIO);

	if (qlfc_trace_buffer_entries <= 0) return;	/* no tracing */

	if (!ctlr->first_trace_entry) {

		/*
		** Allocate memory for the trace buffer
		*/

		ctlr->first_trace_entry = (trace_entry_t *)
					kmem_zalloc (sizeof(trace_entry_t)*qlfc_trace_buffer_entries,
						VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

		/*
		** Initialize data structures.
		*/

		if (ctlr->first_trace_entry) {
			ctlr->trace_index = 0;
			ctlr->last_trace_entry = ctlr->first_trace_entry + (qlfc_trace_buffer_entries-1);
			trace_on (ctlr);
		}
	}

	mutex_unlock (&ctlr->trace_lock);

	return;
}

void
trace (QLFC_CONTROLLER *ctlr, char *fmt,
	__psint_t arg0,
	__psint_t arg1,
	__psint_t arg2,
	__psint_t arg3,
	__psint_t arg4,
	__psint_t arg5)
{

	trace_entry_t *entry;

	if (ctlr->trace_enabled && ctlr->first_trace_entry) {

		atomicSetInt (&ctlr->trace_code,-1);
		entry = &ctlr->first_trace_entry[atomicIncWithWrap (&ctlr->trace_index,qlfc_trace_buffer_entries)];

		nanotime (&entry->timestamp);
		entry->format = fmt;
		entry->arg0 = arg0;
		entry->arg1 = arg1;
		entry->arg2 = arg2;
		entry->arg3 = arg3;
		entry->arg4 = arg4;
		entry->arg5 = arg5;
	}
}

void
traceif (QLFC_CONTROLLER *ctlr, int code, char *fmt,
	__psint_t arg0,
	__psint_t arg1,
	__psint_t arg2,
	__psint_t arg3,
	__psint_t arg4,
	__psint_t arg5)
{

	trace_entry_t *entry;

	if (ctlr->trace_enabled && ctlr->first_trace_entry) {

		if (compare_and_swap_int(&ctlr->trace_code, -1, code)) {

			entry = &ctlr->first_trace_entry[atomicIncWithWrap (&ctlr->trace_index,qlfc_trace_buffer_entries)];

			nanotime (&entry->timestamp);
			entry->format = fmt;
			entry->arg0 = arg0;
			entry->arg1 = arg1;
			entry->arg2 = arg2;
			entry->arg3 = arg3;
			entry->arg4 = arg4;
			entry->arg5 = arg5;

		}
	}
}
/* FILE: notify.c */
/********************************************************************************
*										*
*	qlfc_notify								*
*										*
*	Common routine for calling a scsi requests notify function.		*
*										*
*	Entry:									*
*		Various.							*
*										*
*	Input:									*
*		ctlr - pointer to controller information (may be null)		*
*		sr - pointer to the scsi request				*
*		target - target map index					*
*		lun - self evident!						*
*										*
*	Output:									*
*		None.								*
*										*
*	Notes:									*
*		Call the notify routine, making any trace entries and adjusting	*
*		the request as appropriate.					*
*										*
********************************************************************************/

void
qlfc_notify (QLFC_CONTROLLER *ctlr, scsi_request_t *sr, int target, int lun)
{
	if (ctlr) {
		if (sr->sr_status >= sizeof(qlfc_sr_status_string)/sizeof(char *)) {
			TRACE (ctlr, "NOTIFY  : %d %d: sr 0x%x, status %d\n",
				target,lun,
				(__psint_t) sr,
				sr->sr_status,
				0,0);
		}
		else {
			TRACE (ctlr, "NOTIFY  : %d %d: sr 0x%x, status %d (%s)\n",
				target,lun,
				(__psint_t) sr,
				sr->sr_status,
				(__psint_t) qlfc_sr_status_string[sr->sr_status],
				0);
		}
	}

	(sr->sr_notify)(sr);
}
/* FILE: awol.c */

/*
** Note: awol_count is used to enable/disable processing within
** the timeout routine.
*/

static void
qlfc_target_clear_awol (QLFC_CONTROLLER *ctlr, int loop_id)
{
	qlfc_local_targ_info_t	*qti;

	/*
	** target has returned, stop awol processing
	**	enter with target_map mutex locked
	*/

	if (!mutex_mine (&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err (CE_PANIC, "qlfc: Mutex not owned by caller.");
	}
	qti = ctlr->target_map[loop_id].tm_qti;
	if (qti) {
		if (ctlr->target_map[loop_id].tm_flags & (TM_AWOL_PENDING|TM_AWOL)) {
			IOLOCK (qti->target_mutex);

			if (ctlr->target_map[loop_id].tm_flags & TM_AWOL_PENDING) {
				atomicAddInt (&ctlr->ql_awol_count,-1);
			}

			atomicClearInt (&ctlr->target_map[loop_id].tm_flags,(TM_AWOL|TM_AWOL_PENDING));

			qti->awol_timeout = 0;

			TRACE (ctlr,"clr awol: target %d: setting awol_timeout to %d, awol_count %d\n",
				qti->target,qti->awol_timeout,ctlr->ql_awol_count,0,0,0);

			qti->awol_retries = 0;
			qti->awol_step = 0;
			IOUNLOCK (qti->target_mutex);
			if (qlfc_debug > 3) qlfc_tpmsg (ctlr->ctlr_vhdl, loop_id, "target has returned.\n");
		}
	}
}

static void
qlfc_target_set_awol_pending (QLFC_CONTROLLER *ctlr, int loop_id)
{
	STATUS			status;
	qlfc_local_targ_info_t	*qti;
	uint32_t		port_id;

	/*
	** set up target for awol processing
	**	enter with target_map mutex locked
	**	returns with mutex unlocked!!!
	*/

	if (!mutex_mine (&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err (CE_PANIC, "qlfc: Mutex not owned by caller.");
	}
	if ((ctlr->target_map[loop_id].tm_flags & (TM_LOGGED_IN|TM_AWOL_PENDING|TM_AWOL)) == TM_LOGGED_IN) {
		TRACE (ctlr, "SET_AWOL: loop id %d has gone AWOL\n",loop_id,0,0,0,0,0);
		qlfc_stop_target_queue (ctlr,loop_id);
		QL_MUTEX_ENTER (ctlr);
		qlfc_flush_active_requests (ctlr, SC_CMDTIME, loop_id);	/* retry goes to iwaitf */
		QL_MUTEX_EXIT (ctlr);
		qti = ctlr->target_map[loop_id].tm_qti;
		IOLOCK (qti->target_mutex);
		qti->awol_timeout = AWOL_TIMEOUT;
		qti->awol_retries = 2;	/* used if fabric device */
		qti->awol_giveup_retries = 6;	/* 6 retries if stuck in awol recovery due to inhibit flag */
		atomicSetInt (&ctlr->target_map[loop_id].tm_flags,TM_AWOL_PENDING);
		atomicAddInt (&ctlr->ql_awol_count,1);

		TRACE (ctlr,"SET_AWOL: target %d: setting awol_timeout to %d, awol_count %d\n",
			qti->target,qti->awol_timeout,ctlr->ql_awol_count,0,0,0);

		IOUNLOCK (qti->target_mutex);
		mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		if (ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE) {
			qlfc_logout_fabric_port (ctlr,loop_id);
			/*
			** Try and log it back in, just in case.
			*/
			port_id = ctlr->target_map[loop_id].tm_port_id;
			status = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);	/* nameserver alive? */
			if (status == OK) status = qlfc_login_fabric_port (ctlr, loop_id, port_id, NULL);
			if (status == OK) {
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qlfc_start_target_queue (ctlr, loop_id);	/* clears AWOL */
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
		}
		else {
			atomicClearInt (&ctlr->target_map[loop_id].tm_flags,TM_LOGGED_IN);
			status = qlfc_login_loop_port (ctlr, loop_id, 1);
			if (status == OK) {
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qlfc_start_target_queue (ctlr, loop_id);	/* clears AWOL */
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
		}
		if (status != OK) qlfc_tpmsg (ctlr->ctlr_vhdl, loop_id, "target has disappeared.\n");
	}
	else {
		mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		if (ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE) {
			qlfc_logout_fabric_port (ctlr,loop_id);
			/*
			** Try and log it back in, just in case.
			*/
			port_id = ctlr->target_map[loop_id].tm_port_id;
			status = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);	/* nameserver alive? */
			if (status == OK) status = qlfc_login_fabric_port (ctlr, loop_id, port_id, NULL);
			if (status == OK) {
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qlfc_start_target_queue (ctlr, loop_id);	/* clears AWOL */
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
		}
	}
}

static void
qlfc_target_set_awol (QLFC_CONTROLLER *ctlr, int loop_id)
{
	qlfc_local_targ_info_t	*qti;

	if (!mutex_mine (&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err (CE_PANIC, "qlfc: Mutex not owned by caller.");
	}

	qti = ctlr->target_map[loop_id].tm_qti;

	/* flush the queues for this target */

	TRACE (ctlr, "SET_AWOL: target %d (0x%x) has not returned.  "
		     "Flushing queue.\n",
		loop_id,ctlr->target_map[loop_id].tm_port_id,0,0,0,0);

	atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_AWOL);
	atomicClearInt (&ctlr->target_map[loop_id].tm_flags, TM_AWOL_PENDING);
	atomicAddInt (&ctlr->ql_awol_count,-1);

	qlfc_tpmsg (ctlr->ctlr_vhdl,qti->target,"target has not returned.  Clearing requests.\n");

	QL_MUTEX_ENTER(ctlr);
	qlfc_flush_queue (ctlr, SC_TIMEOUT, loop_id);
	QL_MUTEX_EXIT(ctlr);

	qti->awol_timeout = 0;

	TRACE (ctlr, "awol rec: bail: target %d, setting awol_timeout to %d, awol_count %d\n",
		qti->target,qti->awol_timeout,ctlr->ql_awol_count,0,0,0);

}

/* FILE: fabric.c */
static void
qlfc_stop_target_queue (QLFC_CONTROLLER *ctlr, int loop_id)
{
	qlfc_local_lun_info_t	*qli, *next;
	qlfc_local_targ_info_t	*qti;

	/*
	** Set the DFLG_INIT_IN_PROGRESS flag for each
	**	lun connected to a loop_id.
	*/

	if (ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN) {
		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti) {
			qli = qti->local_lun_info;
			while (qli) {
				IOLOCK (qli->qli_lun_mutex);
				TRACE (ctlr, "STOP_QUE: loop_id: %d, lun %d\n",loop_id,qli->qli_lun,0,0,0,0);
				atomicSetInt (&qli->qli_dev_flags, DFLG_INIT_IN_PROGRESS);
				atomicClearInt (&qli->qli_dev_flags, DFLG_INITIALIZED);
				next = qli->next;
				IOUNLOCK (qli->qli_lun_mutex);
				qli = next;
			}
		}
		else {
			TRACE (ctlr, "STOPTGTQ: null QTI pointer.  Loop id %d.\n",loop_id,0,0,0,0,0);
		}
	}
}

static void
qlfc_start_lun_queue (QLFC_CONTROLLER *ctlr, qlfc_local_lun_info_t *qli)
{

	scsi_request_t		*lun_forw, *lun_back;

	IOLOCK (qli->qli_lun_mutex);
	TRACE (ctlr, "STRT_QUE: lun %d, enter, have lun_mutex, %d commands queued.\n",
		qli->qli_lun,
		qli->qli_cmd_iwcnt,
		0,0,0,0);
	atomicClearInt (&qli->qli_dev_flags, DFLG_INIT_IN_PROGRESS);
	atomicSetInt(&qli->qli_dev_flags, DFLG_INITIALIZED);

	/*
	** Move commands waiting in LUN init. queue into the wait Q
	*/

	lun_forw = qli->qli_iwaitf;
	lun_back = qli->qli_iwaitb;
	qli->qli_iwaitf = qli->qli_iwaitb = NULL;
	qli->qli_cmd_iwcnt = 0;
	IOUNLOCK (qli->qli_lun_mutex);

	if (lun_forw) {
		IOLOCK(ctlr->waitQ_lock);
		if (ctlr->waitf) {
			ctlr->waitb->sr_ha = lun_forw;
			ctlr->waitb = lun_back;
		}
		else {
			ctlr->waitf = lun_forw;
			ctlr->waitb = lun_back;
		}
		ctlr->waitb->sr_ha = NULL;
		IOUNLOCK(ctlr->waitQ_lock);
	}
	TRACE (ctlr, "STRT_QUE: lun %d, exit.\n",qli->qli_lun,0,0,0,0,0);
}

static void
qlfc_start_target_queue (QLFC_CONTROLLER *ctlr, int loop_id)
{
	qlfc_local_lun_info_t	*qli;
	qlfc_local_targ_info_t	*qti;

	if (ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN) {
		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti) {
			qlfc_target_clear_awol (ctlr,loop_id);
			qli = qti->local_lun_info;
			while (qli) {
				qlfc_start_lun_queue (ctlr,qli);
				qli = qli->next;
			}
		}
	}
}

static STATUS
qlfc_logout_fabric_port (QLFC_CONTROLLER *ctlr, int loop_id)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];

	if (mutex_mine(&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err(CE_PANIC, "qlfc: target mutex owned by caller!\n");
	}

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 2, 1, MBOX_CMD_LOGOUT_FABRIC_PORT, (loop_id << 8),
				0,
				0,
				0,
				0,0,0);

	QL_MUTEX_EXIT(ctlr);

	atomicClearInt (&ctlr->target_map[loop_id].tm_flags,TM_LOGGED_IN);

	if (rc == OK) {
		if (mbox_sts[0] != MBOX_STS_COMMAND_COMPLETE) rc = ERROR;
	}

	TRACE (ctlr, "LOGOUT  : loop_id %d, rc %d,0x%x\n",loop_id,rc,mbox_sts[0],0,0,0);
	return rc;
}

static STATUS
qlfc_set_scn (QLFC_CONTROLLER *ctlr, int scn_level)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 2, 1, MBOX_CMD_SEND_CHANGE_REQUEST, scn_level,
				0,
				0,
				0,
				0,0,0);

	QL_MUTEX_EXIT(ctlr);

	if (scn_level == 0xff)
		atomicClearInt (&ctlr->flags, CFLG_SCN_ENABLED);
	else
		atomicSetInt (&ctlr->flags, CFLG_SCN_ENABLED);

	if (rc == OK) {
		if (mbox_sts[0] != MBOX_STS_COMMAND_COMPLETE) rc = ERROR;
	}

	TRACE (ctlr, "SET SCN : scn level 0x%x, rc %d,0x%x\n",scn_level,rc,mbox_sts[0],0,0,0);
	return rc;
}

static STATUS
qlfc_login_fabric_port (QLFC_CONTROLLER *ctlr, int loop_id, uint32_t port_id, uint16_t *mbox)
{
	STATUS		rc;
	int		i;
	uint16_t     	mbox_sts[MBOX_REGS];

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 4, 3, MBOX_CMD_LOGIN_FABRIC_PORT, (loop_id << 8),
				(port_id >> 16) & 0xff,
				port_id & 0xffff,
				0,
				0,0,0);

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {
		if (mbox_sts[0] != MBOX_STS_COMMAND_COMPLETE) rc = ERROR;
		else atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_LOGGED_IN);

	}

	if (mbox) {
		for (i=0; i<MBOX_REGS; i++) *mbox++ = mbox_sts[i];
	}

	TRACE (ctlr, "LOGIN   : loop_id %d, port_id 0x%x, rc %d, 0x%x, 0x%x, 0x%x\n",
		loop_id,port_id,rc,mbox_sts[0],mbox_sts[1],mbox_sts[2]);

	return rc;
}

static void
qlfc_login_fabric_ports (QLFC_CONTROLLER *ctlr)
{
	int	i;
	STATUS	rc;

	TRACE (ctlr,"LOGIN   : logging in all fabric ports.\n",0,0,0,0,0,0);

	for (i=QL_FABRIC_ID_HIGH; i >= 0 && !(ctlr->flags&CFLG_ISP_PANIC); i--) {

		if ((ctlr->target_map[i].tm_flags & (TM_FABRIC_DEVICE|TM_LOGGED_IN)) == (TM_FABRIC_DEVICE)) {
			rc = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);	/* nameserver alive? */
			if (rc == OK) rc = qlfc_login_fabric_port (ctlr,i,ctlr->target_map[i].tm_port_id,NULL);
			mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
			if (rc == OK) {
				qlfc_start_target_queue (ctlr,i);	/* clears awol */
				mutex_unlock (&ctlr->target_map[i].tm_mutex);
			}
			else if (!(ctlr->target_map[i].tm_flags & (TM_AWOL_PENDING|TM_AWOL))) {
				qlfc_target_set_awol_pending (ctlr,i);	/* unlocks tm_mutex */
			}
			else
				mutex_unlock (&ctlr->target_map[i].tm_mutex);
		}
	}
}

static void
qlfc_logout_fabric_ports (QLFC_CONTROLLER *ctlr)
{
	int			i;
	qlfc_local_targ_info_t	*qti;

	for (i=QL_FABRIC_ID_HIGH; i >= 0 && !(ctlr->flags&CFLG_ISP_PANIC); i--) {

		if ((ctlr->target_map[i].tm_flags & (TM_FABRIC_DEVICE|TM_LOGGED_IN)) == (TM_FABRIC_DEVICE|TM_LOGGED_IN)) {
			mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
			TRACE (ctlr, "LGO_FABS: loop id %d has gone AWOL\n",i,0,0,0,0,0);
			qlfc_stop_target_queue (ctlr,i);
			QL_MUTEX_ENTER (ctlr);
			qlfc_flush_active_requests (ctlr, SC_CMDTIME, i);	/* retry goes to iwaitf */
			QL_MUTEX_EXIT (ctlr);
			qti = ctlr->target_map[i].tm_qti;
			IOLOCK (qti->target_mutex);
			qti->awol_timeout = AWOL_TIMEOUT;
			qti->awol_retries = 2;
			qti->awol_giveup_retries = 6;	/* 6 retries if stuck in awol recovery due to inhibit flag */
			atomicSetInt (&ctlr->target_map[i].tm_flags,TM_AWOL_PENDING);
			atomicAddInt (&ctlr->ql_awol_count,1);
			IOUNLOCK (qti->target_mutex);
			mutex_unlock (&ctlr->target_map[i].tm_mutex);
			qlfc_logout_fabric_port (ctlr,i);
		}
	}
}

static int
qlfc_find_available_loop_id (QLFC_CONTROLLER *ctlr, uint32_t port_id)
{
	int i;
	int available = -1;
	int alpa;
	target_map_t	*tm;

	/*
	**	Find an available loop id.
	**	  Take low byte of port id, use as alpa, convert to loop id + 128, try that.
	**	  If not available, try to find ANY available loop id.
	*/

	alpa = port_id & 0xff;
	i = ALPA_2_TID (alpa) + 128;
	if (!(ctlr->target_map[i].tm_flags & TM_RESERVED) && !ctlr->target_map[i].tm_port_name) {
		mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
		tm = &ctlr->target_map[i];
		tm->tm_port_id = 0;
		tm->tm_flags = 0;
		tm->tm_node_name = 0;
		available = i;
	}
	else {
		for (i=QL_FABRIC_ID_HIGH; i >= QL_FABRIC_ID_LOW; i--) {
			if (!(ctlr->target_map[i].tm_flags&TM_RESERVED)) { 
				mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
				if ( !ctlr->target_map[i].tm_port_name) {
					tm = &ctlr->target_map[i];
					tm->tm_port_id = 0;
					tm->tm_flags = 0;
					tm->tm_node_name = 0;
					available = i;
					break;
				}
				mutex_unlock (&ctlr->target_map[i].tm_mutex);
			}
		}
	}
	return available;
}

static int
qlfc_find_loop_id (QLFC_CONTROLLER *ctlr, uint64_t port_name)
{
	int i;
	int loop_id=-1;

	/*
	**	Check to see if port_name already assigned a loop id.
	**	If so, take the mutex for the target map!
	**
	**	Note: returns with MUTEX LOCKED!
	*/

	for (i=QL_FABRIC_ID_HIGH; i >= QL_FABRIC_ID_LOW; i--) {

		if (ctlr->target_map[i].tm_flags & TM_RESERVED) continue;
		mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
		if (ctlr->target_map[i].tm_port_name == port_name) {	/* it's here */
			loop_id = i;
			break;
		}
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}

	return loop_id;
}


static int
qlfc_assign_loop_id (QLFC_CONTROLLER *ctlr, uint32_t port_id, uint64_t node_name, uint64_t port_name)
{
	/*
	** This routine returns with the target map mutex locked IF
	** the a loop_id could be assigned.  Locked by qlfc_find_*_loop_id.
	*/

	int loop_id=-1;

	loop_id = qlfc_find_loop_id (ctlr, port_name);
	if (loop_id == -1) loop_id = qlfc_find_available_loop_id (ctlr, port_id);

	if (loop_id >= QL_FABRIC_ID_LOW) {
		ctlr->target_map[loop_id].tm_node_name = node_name;
		ctlr->target_map[loop_id].tm_port_name = port_name;
		ctlr->target_map[loop_id].tm_port_id = port_id;
	}

	TRACE (ctlr, "AssignLI: %d: port id 0x%x, node 0x%x, port 0x%x\n",loop_id,port_id,node_name,port_name,0,0);
	if ((loop_id != -1) && !mutex_mine(&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err(CE_PANIC, "qlfc: target mutex not owned by caller!\n");
	}
	return loop_id;
}

static void
qlfc_free_loop_id (QLFC_CONTROLLER *ctlr, int loop_id)
{
	if (!(ctlr->target_map[loop_id].tm_flags & (TM_RESERVED|TM_LOGGED_IN))) {
		ctlr->target_map[loop_id].tm_node_name = 0;
		ctlr->target_map[loop_id].tm_port_name = 0;
		ctlr->target_map[loop_id].tm_port_id = 0;
	}
}

static void
qlfc_reserve_loop_id (QLFC_CONTROLLER *ctlr, int loop_id)
{
	atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_RESERVED);
}

static void
qlfc_stop_queues (QLFC_CONTROLLER *ctlr)
{
	int i;

	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
		qlfc_stop_target_queue (ctlr,i);
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}
}

/* FILE: loop.c */
static void
qlfc_start_loop_queues (QLFC_CONTROLLER *ctlr)
{
	int i;

	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		mutex_lock (&ctlr->target_map[i].tm_mutex,PRIBIO);
		if (!(ctlr->target_map[i].tm_flags & (TM_AWOL_PENDING|TM_AWOL|TM_RESERVED|TM_FABRIC_DEVICE))) {
			qlfc_start_target_queue (ctlr,i);
		}
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}
}

static STATUS
qlfc_login_loop_port (QLFC_CONTROLLER *ctlr, int loop_id, int login_options)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 2, 1, MBOX_CMD_LOGIN_LOOP_PORT, (loop_id << 8),
				login_options,
				0, 0, 0, 0, 0);

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {
		if (mbox_sts[0] != MBOX_STS_COMMAND_COMPLETE) rc = ERROR;
		else atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_LOGGED_IN);

	}

	TRACE (ctlr, "LOGIN   : loop_id %d, options 0x%x, rc %d, 0x%x\n",
		loop_id,login_options,rc,mbox_sts[0],0,0);

	return rc;
}

static void
qlfc_login_loop_ports (QLFC_CONTROLLER *ctlr)
{
	int	i;
	STATUS	rc;

	TRACE (ctlr,"LOGIN   : logging in all awol loop ports.\n",0,0,0,0,0,0);

	for (i=QL_FABRIC_ID_HIGH; i >= 0 && !(ctlr->flags&CFLG_ISP_PANIC); i--) {

		if ((ctlr->target_map[i].tm_flags & (TM_AWOL|TM_FABRIC_DEVICE|TM_LOGGED_IN)) == TM_AWOL) {
			rc = qlfc_login_loop_port (ctlr,i,ctlr->target_map[i].tm_port_id);
			if (rc == OK) {
				mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
				qlfc_start_target_queue (ctlr,i);	/* clears awol */
				mutex_unlock (&ctlr->target_map[i].tm_mutex);
			}
		}
	}
}

/* FILE: abort.c */
int
qlfc_abort(scsi_request_t *scsi_request)
{
	scsi_lun_info_t		*lun_info;
	scsi_targ_info_t	*targ_info;
	uint16_t		mbox_sts[MBOX_REGS];
	QLFC_CONTROLLER		*ctlr;
	qlfc_local_lun_info_t	*qli;
	int			retval = 0; /* assume failure */
	uint16_t		mailbox1, mailbox2;

	lun_info = scsi_lun_info_get (scsi_request->sr_lun_vhdl);
	ctlr = SLI_CTLR(lun_info);
	qli = SLI_INFO (lun_info);
	targ_info = SLI_TARG_INFO(lun_info);

	/*
	 * Only issue the ABORT mailbox command if there are
	 * outstanding * I/Os, otherwise punt.
	 */

	IOLOCK(qli->qli_lun_mutex);

	if (((qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) == 0) &&
	    (qli->qli_cmd_rcnt > 0)) {
		atomicSetInt(&qli->qli_dev_flags, DFLG_ABORT_IN_PROGRESS);

		IOUNLOCK (qli->qli_lun_mutex);

		QL_MUTEX_ENTER(ctlr);
		if (ctlr->flags & CFLG_EXTENDED_LUN) {
			mailbox1 = (uint16_t) ((STI_TARG(targ_info)) << 8);
			mailbox2 = (uint16_t) (SLI_LUN(lun_info));
		}
		else {
			mailbox1 = (uint16_t) ((STI_TARG(targ_info)) << 8) 
				 | (uint16_t) (SLI_LUN(lun_info));
			mailbox2 = 0;
		}
		if (qlfc_mbox_cmd(ctlr, mbox_sts, 3, 2,
				MBOX_CMD_ABORT_DEVICE,
				mailbox1,
				mailbox2,
				0, 0, 0, 0, 0))
		{
			qlfc_lpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), SLI_LUN(lun_info),
				 "MBOX_CMD_ABORT_DEVICE Command failed\n");
		}
		else {
			retval = 1; /* success */
		}
		QL_MUTEX_EXIT(ctlr);
	}
	else {
		IOUNLOCK (qli->qli_lun_mutex);
	}

	return(retval);
}

/* FILE: alloc.c */
void
qlfc_free(vertex_hdl_t 	ql_lun_vhdl,
	    void 		(*cb)())

{
        scsi_lun_info_t	*lun_info = scsi_lun_info_get(ql_lun_vhdl);
	qlfc_local_lun_info_t *qli = SLI_INFO(lun_info);
	QLFC_CONTROLLER *	 ha = qlfc_ctlr_from_lun_vhdl_get(ql_lun_vhdl);

	if (ha == NULL) return;

	mutex_lock(&qli->qli_open_mutex, PRIBIO);

	if (qli->qli_dev_flags & DFLG_EXCLUSIVE)
		atomicClearInt(&qli->qli_dev_flags, DFLG_EXCLUSIVE);
        if (qli->qli_dev_flags & DFLG_CONTINGENT_ALLEGIANCE)
		atomicClearInt(&qli->qli_dev_flags, DFLG_CONTINGENT_ALLEGIANCE);

	if (qli->qli_ref_count != 0)
		atomicAddInt(&qli->qli_ref_count, -1);

	if (cb && !qli->qli_ref_count) {
		if (qli->qli_sense_callback == cb)
	    		qli->qli_sense_callback = NULL;
		else
			qlfc_lpmsg(ha->ctlr_vhdl, SLI_TARG(lun_info), SLI_LUN(lun_info),
				 "qlfcfree: callback 0x%x not active\n", cb);
	}

	mutex_unlock(&qli->qli_open_mutex);
}

int
qlfc_alloc(vertex_hdl_t 	ql_lun_vhdl,
	int 		opt,
	void 		(*cb)())
{
	int	         retval = SCSIALLOCOK;
	int		set_cb = 0;
	scsi_lun_info_t	*lun_info;
	qlfc_local_lun_info_t *qli;
	QLFC_CONTROLLER	*ctlr=NULL;

	ctlr = qlfc_ctlr_from_lun_vhdl_get(ql_lun_vhdl);

	if (ctlr) TRACE (ctlr, "ALLOC   : lun vhdl %d\n",ql_lun_vhdl,0,0,0,0,0);

	lun_info = scsi_lun_info_get(ql_lun_vhdl);
	qli = SLI_INFO(lun_info);
	ASSERT (qli);

	mutex_lock(&qli->qli_open_mutex, PRIBIO);

	atomicAddInt(&qli->qli_ref_count, 1);

	if (qli->qli_dev_flags & DFLG_EXCLUSIVE) {
		if (ctlr) TRACE (ctlr, "ALLOC   : lun vhdl %d, busy 1\n",ql_lun_vhdl,0,0,0,0,0);
		retval = EBUSY;
		goto _allocout;
	}

	if (cb) {
		if (qli->qli_sense_callback) {
			if (qli->qli_sense_callback != cb) {
				if (ctlr) TRACE (ctlr, "ALLOC   : lun vhdl %d, einval 1\n",ql_lun_vhdl,0,0,0,0,0);
				retval = EINVAL;
				goto _allocout;
			}
		}
		else {
			qli->qli_sense_callback = cb;
			set_cb = 1;
		}
	}

	if (opt & SCSIALLOC_EXCLUSIVE) {
		if (qli->qli_ref_count != 1) {
			if (ctlr) TRACE (ctlr, "ALLOC   : lun vhdl %d, busy 2\n",ql_lun_vhdl,0,0,0,0,0);
	    		retval = EBUSY;
	    		goto _allocout;
		} else
	    		atomicSetInt(&qli->qli_dev_flags, DFLG_EXCLUSIVE);
	}
_allocout:
	if (retval != SCSIALLOCOK) {
		atomicAddInt(&qli->qli_ref_count, -1);	/* remove reference count */
		if (set_cb) qli->qli_sense_callback = NULL;
	}
	mutex_unlock(&qli->qli_open_mutex);
    	return retval;

}

/* FILE: command.c */
/* Entry-Point
** Function:    qlfc_command
**
** Description: Execute a SCSI request.
**
** Calls:	scsi_lun_info_get()
**		qlfc_ctlr_from_lun_vhdl_get()
**		qlfc_start_scsi()
**		qlfc_intr()
*/

void
qlfc_command (scsi_request_t *request)
{
	scsi_lun_info_t	*lun_info;

	qlfc_local_targ_info_t	*qti;
	qlfc_local_lun_info_t	*qli;
	QLFC_CONTROLLER		*ctlr;

	/* Before processing the request, make sure we will return good status.  */

	if (request == NULL) return;

	lun_info = scsi_lun_info_get(request->sr_lun_vhdl);
	ctlr = qlfc_ctlr_from_lun_vhdl_get(request->sr_lun_vhdl);

	request->sr_status = 0;
	request->sr_scsi_status = 0;
	request->sr_ha_flags = 0;
	request->sr_sensegotten = 0;
	request->sr_spare = NULL;

        /* Error if the adapter specified is invalid.  */
    	if (ctlr == NULL || lun_info == NULL || (ctlr->flags & CFLG_SHUTDOWN)) {
		request->sr_status = SC_REQUEST;
		qlfc_notify (ctlr, request, -1, -1);
		return;
    	}

	qti = STI_INFO(SLI_TARG_INFO(lun_info));

	/*
	 * Ensure buffer alignment constraints are enforced. Only need
	 * to check for the case of KV or UV addresses; PAGEIO is
	 * guaranteed to be page aligned.
	 */
	if (request->sr_flags & (SRF_MAPUSER | SRF_MAP) &&
	    request->sr_buflen != 0 &&
	    ((uintptr_t) request->sr_buffer & 3))
	{
		request->sr_status = SC_ALIGN;
		qlfc_notify (ctlr, request, qti->target, SLI_LUN(lun_info));
		return;
	}

	qli = SLI_INFO(lun_info);

	/*
	 * If CONTINGENT ALLEGIANCE in effect and requestor hasn't set
	 * the AEN_ACK flag, then bail out.
	 */
        if (qli->qli_dev_flags & DFLG_CONTINGENT_ALLEGIANCE) {
	    if (request->sr_flags & SRF_AEN_ACK) {
		TRACE (ctlr, "COMMAND : %d %d: clearing contingent allegiance.\n",qti->target, SLI_LUN(lun_info),0,0,0,0);
		atomicClearInt(&qli->qli_dev_flags, DFLG_CONTINGENT_ALLEGIANCE);
            }
            else {
		TRACE (ctlr, "COMMAND : %d %d: contingent allegiance active.\n",qti->target, SLI_LUN(lun_info),0,0,0,0);
		request->sr_status = SC_ATTN;
		qlfc_notify (ctlr, request, qti->target, SLI_LUN(lun_info));
                return;
            }
	}

	/*
	 * If the target has gone awol, return the request SC_TIMEOUT.
	 *
	 */

	if (ctlr->target_map[qti->target].tm_flags&TM_AWOL) {
		TRACE (ctlr, "COMMAND : %d %d: target AWOL.\n",qti->target, SLI_LUN(lun_info),0,0,0,0);
		request->sr_status = SC_TIMEOUT;
		qlfc_notify (ctlr, request, qti->target, SLI_LUN(lun_info));
		return;
	}

	/*
	 * If the LUN is being aborted, stick the command into the LUN
	 * abort wait Q.
	 */
	if (qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) {
		IOLOCK(qli->qli_lun_mutex);

	        /*
		 * After grabbing the lock, check again whether the
		 * LUN is still being aborted. There could be a race
		 * here which could cause commands to be "forgotten"
		 * if they're stuck in the LUN wait Q after the LUN
		 * abort is complete.
		 */
	        if ((qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) == 0) {
			IOUNLOCK(qli->qli_lun_mutex);
		}
		else {
			if (qli->qli_awaitf)
				qli->qli_awaitb->sr_ha = request;
			else
				qli->qli_awaitf = request;
			qli->qli_awaitb = request;
			request->sr_ha = NULL;
			++qli->qli_cmd_awcnt;
			TRACE (ctlr, "COMMAND : queued sr 0x%x on await, count %d, qli_cmd_rcnt %d\n",
				(__psint_t)request,qli->qli_cmd_awcnt,qli->qli_cmd_rcnt,0,0,0);
			IOUNLOCK(qli->qli_lun_mutex);
			return;
		}
	}

	/*
	** If LUN is being initialized, stick the command into the LUN
	** init. Q.
	*/

	if (qli->qli_dev_flags & DFLG_INIT_IN_PROGRESS) {
		IOLOCK(qli->qli_lun_mutex);

	        /*
		 * After grabbing the lock, check again whether the
		 * LUN is still being inited. There could be a race
		 * here which could cause commands to be "forgotten"
		 * if they're stuck in the LUN wait Q after the LUN
		 * init. is complete.
		 */
	        if ((qli->qli_dev_flags & DFLG_INIT_IN_PROGRESS) == 0) {
			IOUNLOCK(qli->qli_lun_mutex);
		}
		else {
			if (qli->qli_iwaitf) {
				qli->qli_iwaitb->sr_ha = request;
			}
			else {
				qli->qli_iwaitf = request;
			}

			qli->qli_iwaitb = request;
			request->sr_ha = NULL;
			++qli->qli_cmd_iwcnt;
			TRACE (ctlr, "COMMAND : queued sr 0x%x on iwait, count %d\n",
				(__psint_t)request,qli->qli_cmd_iwcnt,0,0,0,0);
			IOUNLOCK(qli->qli_lun_mutex);
			return;
		}
	}

	/*
	** Check that the device has been initialized
	*/

	if (!(qli->qli_dev_flags & DFLG_INITIALIZED) ) {
		qlfc_start_lun_queue (ctlr, qli);
    	}


	/*
	** Stick the command into the LUN Wait Q
	*/

	TRACE (ctlr, "COMMAND : %d %d: queuing 0x%x on wait queue.\n",
		qti->target,qli->qli_lun,(__psint_t)request,0,0,0);

	IOLOCK(ctlr->waitQ_lock);
	if (ctlr->waitf)
		ctlr->waitb->sr_ha = request;
	else
		ctlr->waitf = request;
	ctlr->waitb      = request;
	request->sr_ha = NULL;
	IOUNLOCK(ctlr->waitQ_lock);

        /* Start the command (if possible).  */

        if (!(ctlr->flags & CFLG_DRAIN_IO)) qlfc_start_scsi(ctlr);

    	if (ctlr->flags & CFLG_DUMPING) {
		CONTROLLER_REGS *   isp = (CONTROLLER_REGS *)ctlr->base_address;
		while (!(QL_PCI_INH(&isp->isr) & ISR_RISC_INTERRUPT)) {
			DELAY(25);
		}
		qlfc_intr(ctlr);
    	}

    	return;
}
/* FILE: done.c */
static void
qlfc_done(scsi_request_t *request)
{
    vsema(request->sr_dev);
}

/* FILE: error.c */
int
qlfc_error(struct pci_record *pp, edt_t *e, __int32_t reason)
{
    printf("ql_error: does nothing - pci_record %x edt %x reason %x\n",
	   pp, e, reason);
    return(0);
}

/* FILE: fastPost.c */
/*********************************************************************
 * qlfc_cmd_complete_fast_post.
 *
 * This routine is aclled from Service_mailbox_interrupt routine.
 * When IO completes Good, adapter will return the handle in mailbox1
 * and mailbox2. Post scsi status, completion status to good and
 * residual count to zero and complete the request. This is
 * performance mode for ispfc_ device.
 *
 *********************************************************************/
static void
qlfc_cmd_complete_fast_post(QLFC_CONTROLLER *ctlr, scsi_request_t *request)
{

	qlfc_local_lun_info_t	*qli;
	qlfc_local_targ_info_t	*qti;
	scsi_lun_info_t		*lun_info;

	atomicAddInt(&(ctlr->ql_ncmd),-1);
	if ((ctlr->flags & CFLG_DRAIN_IO) && (ctlr->ql_ncmd == 0)) {
		atomicClearInt (&ctlr->flags, CFLG_DRAIN_IO);
	}

	ctlr->ha_last_intr = lbolt/HZ;

	/*
	 * Update LUN status - if all aborted commands have been
	 * returned, transfer all commands queued during the abort
	 * phase into the wait Q.
	 */
	lun_info = scsi_lun_info_get (request->sr_lun_vhdl);

	qli = SLI_INFO(lun_info);
	qti = STI_INFO(SLI_TARG_INFO(lun_info));

	atomicClearInt (&qti->recovery_step, 0xffffffff);
	atomicClearInt (&ctlr->recovery_step, 0xffffffff);

	TRACE (ctlr, "FASTPOST: %d %d: sr 0x%x, ncmd %d, qli_cmd_rcnt %d\n",
		qti->target,
		qli->qli_lun,
		(__psint_t)request,
		ctlr->ql_ncmd,
		qli->qli_cmd_rcnt-1,0);

	IOLOCK(qli->qli_lun_mutex);

	atomicAddInt (&qti->req_count,-1);
	atomicAddInt(&qli->qli_cmd_rcnt, -1);

	if (qlfc_debug >= 99)
		TRACE (ctlr, "FAST CNT: qli_cmd_rcnt %d, req_count %d,ql_ncmd %d\n",qli->qli_cmd_rcnt,

	qti->req_count,ctlr->ql_ncmd,0,0,0);


	if ((qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) && (qli->qli_cmd_rcnt == 0)) {
		scsi_request_t *lun_forw;
		scsi_request_t *lun_back;

		lun_forw = qli->qli_awaitf;
		lun_back = qli->qli_awaitb;
		qli->qli_awaitf = qli->qli_awaitb = NULL;
		qli->qli_cmd_awcnt = 0;
		atomicClearInt(&qli->qli_dev_flags, DFLG_ABORT_IN_PROGRESS);
		atomicSetInt(&qli->qli_dev_flags, DFLG_SEND_MARKER);
		IOUNLOCK(qli->qli_lun_mutex);


		if (lun_forw) {
			IOLOCK(ctlr->waitQ_lock);
			if (ctlr->waitf)
			{
				ctlr->waitb->sr_ha = lun_forw;
				ctlr->waitb = lun_back;
			}
			else
			{
				ctlr->waitf = lun_forw;
				ctlr->waitb = lun_back;
			}
			ctlr->waitb->sr_ha = NULL;
			IOUNLOCK(ctlr->waitQ_lock);
		}
	}
	else {
		IOUNLOCK(qli->qli_lun_mutex);
	}

	/*
	 * does this completeion complete our QUIESCE_IN_PROGRESS
	 */
	if ((ctlr->flags & CFLG_QUIESCE_IN_PROGRESS) &&
	    (ctlr->ql_ncmd == 0))
	{
		atomicSetInt(&ctlr->flags, CFLG_QUIESCE_COMPLETE);
		atomicClearInt(&ctlr->flags, CFLG_QUIESCE_IN_PROGRESS);
		untimeout(ctlr->quiesce_in_progress_id);
		ctlr->quiesce_id = timeout(kill_quiesce, ctlr, ctlr->quiesce_time);
	}


#ifdef HEART_INVALIDATE_WAR
	/* if this IO resulted in a DMA to memory from IO (disk read) 	*/
	/* then invalide cache copies of the data 			*/
	if (request->sr_flags & SRF_DIR_IN) {
		if (request->sr_flags & SRF_MAPBP) {
			bp_heart_invalidate_war(request->sr_bp);
		}
		else {
			heart_invalidate_war(request->sr_buffer,request->sr_buflen);
		}
	}
#endif /* HEART_INVALIDATE WAR */


	/*
	** Queue the request for callback upon termination of interrupt
	** processing.  This closes a bug where I could see duplicate
	** responses.  Releasing the res_lock mutex permitted mbox_do_cmd
	** to get both mutexes.  The CFLG_IN_INTR flag was set, so the
	** processing of the mailbox command POLLED even though it wasn't
	** initiated in the interrupt handler.
	*/

	request->sr_status = SC_GOOD;
	request->sr_ha = NULL;
	request->sr_scsi_status = 0; /* Good Status */
	request->sr_resid = 0;

	/*
	** compl_* protected by res_lock
	*/

	if (ctlr->compl_forw) {
		ctlr->compl_back->sr_ha = request;
		ctlr->compl_back        = request;
		request->sr_ha  = NULL;
	}
	else
	{
		ctlr->compl_forw = request;
		ctlr->compl_back = request;
	}
}

/* FILE: flush.c */

static void
qlfc_flush_active_requests (QLFC_CONTROLLER *ctlr, uint sr_status, int loop_id)
{
	scsi_request_t		*queue_head = NULL;
	scsi_request_t		*queue_last = NULL;
	scsi_request_t		*request = NULL;
	qlfc_local_targ_info_t	*qti;

	if (!mutex_mine (&ctlr->target_map[loop_id].tm_mutex)) {
		cmn_err(CE_PANIC, "qlfc: Mutex not owned by caller.");
	}

	qti = ctlr->target_map[loop_id].tm_qti;
	if (qti) {

		/*
		 * Clean up queued active target commands
		 */
		IOLOCK(qti->target_mutex);
		request = qti->req_active;
		if (request) {
			if (queue_last) {
				queue_last->sr_ha = request;
				queue_last        = qti->req_last;
			}
			else {
	    			queue_head = request;
	    			queue_last = qti->req_last;
			}
			queue_last->sr_ha  = NULL;
			qti->req_active = qti->req_last = NULL;
		}

		TRACE (ctlr, "FLUSH   : %d ?: flushing %d active requests.\n",qti->target,qti->req_count,0,0,0,0);

		atomicAddInt (&ctlr->ql_ncmd,-qti->req_count);
		qti->req_count = 0;

		IOUNLOCK(qti->target_mutex);

		QL_MUTEX_EXIT(ctlr);

		/*
		** Run the queue of requests, calling the
		** notify routine.
		*/

		while (request = queue_head) {	/* yes, assignment */

			queue_head = request->sr_ha;
			if (!request->sr_status) request->sr_status = sr_status;
			qlfc_notify (ctlr, request, -1, -1);
		}

		QL_MUTEX_ENTER(ctlr);
	}
}

static void
qlfc_flush_queue (QLFC_CONTROLLER *ctlr, uint sr_status, int flush_this_one)
{
	int			loop_id;
	scsi_request_t		*queue_head = NULL;
	scsi_request_t		*queue_last = NULL;
	scsi_request_t		*request = NULL;
	qlfc_local_lun_info_t	*qli;
	qlfc_local_targ_info_t	*qti;


	TRACE (ctlr, "FLUSH   : flushing queues.\n",0,0,0,0,0,0);

	for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {

		if (flush_this_one == ALL_QUEUES) {
			mutex_lock (&ctlr->target_map[loop_id].tm_mutex,PRIBIO);
		}
		else if (flush_this_one != loop_id) continue;

		if (!mutex_mine (&ctlr->target_map[loop_id].tm_mutex)) {
			cmn_err(CE_PANIC, "qlfc: Mutex not owned by caller.");
		}

		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti) {

			/*
			 * Clean up queued active target commands
			 */
			IOLOCK(qti->target_mutex);
			request = qti->req_active;
			if (request) {
				if (queue_last) {
					queue_last->sr_ha = request;
					queue_last        = qti->req_last;
				}
				else {
		    			queue_head = request;
		    			queue_last = qti->req_last;
				}
				queue_last->sr_ha  = NULL;
				qti->req_active = qti->req_last = NULL;
			}

			TRACE (ctlr, "FLUSH   : %d ?: flushing %d active requests.\n",qti->target,qti->req_count,0,0,0,0);

			atomicAddInt (&ctlr->ql_ncmd,-qti->req_count);
			qti->req_count = 0;

			IOUNLOCK(qti->target_mutex);

			/*
			 * Clean up queued LUN commands
			 */

			qli = qti->local_lun_info;
			while (qli) {

	
				IOLOCK(qli->qli_lun_mutex);
				if (qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS && qli->qli_awaitf) {
					if (queue_head) {
						queue_last->sr_ha = qli->qli_awaitf;
						queue_last        = qli->qli_awaitb;
					}
					else {
						queue_head = qli->qli_awaitf;
						queue_last = qli->qli_awaitb;
					}
					queue_last->sr_ha  = NULL;		
					qli->qli_awaitf = qli->qli_awaitb = NULL;
					TRACE (ctlr, "FLUSH   : %d %d: flushing %d awaitq requests.\n",
						qti->target,qli->qli_lun,qli->qli_cmd_awcnt,0,0,0);
				}
				atomicClearInt(&qli->qli_dev_flags, DFLG_ABORT_IN_PROGRESS);

				if ((qli->qli_dev_flags & DFLG_INIT_IN_PROGRESS) && qli->qli_iwaitf) {
					if (queue_head) {
						queue_last->sr_ha = qli->qli_iwaitf;
						queue_last        = qli->qli_iwaitb;
					}
					else {
						queue_head = qli->qli_iwaitf;
						queue_last = qli->qli_iwaitb;
					}
					queue_last->sr_ha  = NULL;		
					qli->qli_iwaitf = qli->qli_iwaitb = NULL;
					TRACE (ctlr, "FLUSH   : %d %d: flushing %d iwaitq requests.\n",
						qti->target,qli->qli_lun,qli->qli_cmd_iwcnt,0,0,0);
				}
				atomicClearInt(&qli->qli_dev_flags, DFLG_INIT_IN_PROGRESS);

				qli->qli_cmd_awcnt = qli->qli_cmd_iwcnt = qli->qli_cmd_rcnt = 0;
				IOUNLOCK(qli->qli_lun_mutex);

				qli = qli->next;
			}
		}
		if (flush_this_one == ALL_QUEUES) mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
	}

	QL_MUTEX_EXIT(ctlr);

	/*
	** Run the queue of requests, calling the
	** notify routine.
	*/

        while (request = queue_head) {	/* yes, assignment */

		queue_head = request->sr_ha;
		if (!request->sr_status) request->sr_status = sr_status;
		qlfc_notify (ctlr, request, -1, -1);
        }

	QL_MUTEX_ENTER(ctlr);
}

/* FILE: halt.c */
void
qlfc_halt(void)
{
	QLFC_CONTROLLER *	ha;

	for (ha = qlfc_controllers; ha; ha = ha->next)
		qlfc_stop_risc (ha);
}

/* FILE: getluns.c */
/********************************************************************************
*										*
*	qlfc_getluns								*
*										*
*	Perform a report luns command to lun 0 of the current target.		*
*										*
*	Entry:									*
*		Called by qlfc_info()						*
*										*
*	Input:									*
*		ctlr - qlfc controller structure				*
*		qti - current local target info					*
*		req - scsi_request being used by qlfc_info			*
*		sema - semaphore being utilized by qlfc_info			*
*										*
*	Output:									*
*										*
*	Notes:									*
*		This code was lifted almost intact from fcadp.  If it doesn't	*
*		work, let's blame Jeremy!					*
*										*
********************************************************************************/

#define GLSZ ((QL_MAXLUNS * 8) + 8)

void
qlfc_getluns (QLFC_CONTROLLER *ctlr, qlfc_local_targ_info_t *qti,
	struct scsi_request *req, sema_t *sema)
{
	static uint8_t		 scsi[12]={0xA0, 0, 0, 0, 0, 0, GLSZ>>24,
					   GLSZ>>16, GLSZ>>8, (u_char) GLSZ, 0, 0};
	int			 i;
	struct report_luns_format {
		uint32_t	count;
		uint32_t	reserved;
		struct {
			uint16_t	lun_number;
			uint16_t	lun_levels[3];
		} lun_element[1];
	} *lun_list;
	uint8_t			*sense_buffer;
	int			 trycmd = 2;

	/*
	** Default to probe the first n luns if the command fails.
	**	This gets replaced with a valid lunmask if the
	**	command succeeds.
	*/

	if (! (ctlr->flags & CFLG_EXTENDED_LUN)) {
		for (i=0; i<QL_MAX_PROBED_LUN; i++)
			qti->lunmask[i/8] |= 1 << (i%8);
	}

	else {
		lun_list = kmem_zalloc(GLSZ, VM_CACHEALIGN);
		sense_buffer = kmem_zalloc(REQ_SENSE_DATA_LENGTH, 0);

		while (trycmd) {
			/*
			 * most of the fields are set in qlfc_info
			 */
			req->sr_command = scsi;
			req->sr_cmdlen = 12;     /* report luns command length */
			req->sr_flags = SRF_DIR_IN | SRF_FLUSH | SRF_AEN_ACK;
			req->sr_timeout = 30 * HZ;
			req->sr_buffer = (u_char *) lun_list;
			req->sr_buflen = (QL_MAXLUNS * 8) + 8;
			req->sr_sense = sense_buffer;
			req->sr_senselen = REQ_SENSE_DATA_LENGTH;

			qlfc_command (req);
			psema(sema, PRIBIO);

			/*
			 * Retry the REPORT LUNS on a unit attention
			 */
			if (req->sr_status == SC_GOOD &&
			    req->sr_sensegotten > 2 &&
			    req->sr_sense[2] == SC_UNIT_ATTN)
			{
				trycmd--;
			}
			else
			{
			    if ((req->sr_status == SC_GOOD) && (req->sr_scsi_status == ST_GOOD))
			    {
				IOLOCK (qti->target_mutex);
				bzero(qti->lunmask, sizeof(qti->lunmask));
				/*
				 * Go through LUN list.  For FCP, we are only interested in the
				 * top 16 bits of the lun
				 */
				for (i = 0; i < lun_list->count / 8; i++)
				{
				    int lun;
				    lun = lun_list->lun_element[i].lun_number;
				    if (lun < QL_MAXLUNS)
				    {
					qti->lunmask[lun/8] |= 1 << (lun % 8);
					qti->luncount++;
					TRACE (ctlr, "GETLUNS : lun %d exists.\n",lun,0,0,0,0,0);
				    }
				    else
				    {
					qlfc_cpmsg (ctlr->ctlr_vhdl, "Reported lun value (%d) out of range.\n",lun);
				    }
				}
				IOUNLOCK(qti->target_mutex);
			    }
			    trycmd = 0;
			}
		}
		kmem_free(lun_list, GLSZ);
		kmem_free(sense_buffer, REQ_SENSE_DATA_LENGTH);
	}


}

/* FILE: info.c */
/********************************************************************************
*										*
*	qlfc_info								*
*										*
*	Perform an inquiry on the specified lun vertex handle.			*
*										*
*	Entry:									*
*		Various.							*
*										*
*	Input:									*
*		ql_lun_vhdl - lun vertex handle					*
*										*
*	Output:									*
*		Returns a pointer to the scsi_target_info if successful.	*
*										*
*	Notes:									*
*		Describe anything else useful!					*
*										*
********************************************************************************/

struct scsi_target_info *
qlfc_info(vertex_hdl_t 	ql_lun_vhdl)
{
	QLFC_CONTROLLER		*ctlr;
	struct scsi_request	*req;
	static u_char		scsi[6];
	sema_t			*sema;
	struct scsi_target_info	*retval=NULL;
	scsi_target_info_t	*tinfo;
	scsi_lun_info_t		*lun_info;
	qlfc_local_lun_info_t	*qli;
	u_char			*sensep;
	int			retry = 2;
	int			busy_retry = 8;
	int			lun;

	lun_info = scsi_lun_info_get(ql_lun_vhdl);
	ctlr = SLI_CTLR(lun_info);
	qli = SLI_INFO(lun_info);
	ASSERT(qli != NULL);
	mutex_lock(&qli->qli_open_mutex, PRIBIO);

	tinfo = TINFO(lun_info);

	bzero (tinfo->si_inq, SCSI_INQUIRY_LEN);

	sensep  = kern_calloc(1,REQ_SENSE_DATA_LENGTH);
	sema = kern_calloc(1, sizeof(sema_t));
	init_sema(sema, 0, "ql", ql_lun_vhdl);

	req = kern_calloc(1, sizeof(*req));

	do {

		bzero(req, sizeof(*req));

		scsi[0] = INQUIRY_CMD;
		scsi[1] = scsi[2] = scsi[3] = scsi[5] = 0;
		scsi[4] = SCSI_INQUIRY_LEN;

		req->sr_lun_vhdl 	= ql_lun_vhdl;
		req->sr_target 		= SLI_TARG(lun_info);
		req->sr_lun 	= lun	= SLI_LUN(lun_info);

		req->sr_command = scsi;
		req->sr_cmdlen = 6;     /* Inquiry command length */
		req->sr_flags = SRF_DIR_IN | SRF_AEN_ACK;
		req->sr_timeout = 10*HZ;
		req->sr_buffer = tinfo->si_inq;
		req->sr_buflen = SCSI_INQUIRY_LEN;
		req->sr_sense = sensep;
		req->sr_senselen = REQ_SENSE_DATA_LENGTH;
		req->sr_notify = qlfc_done;
		req->sr_dev = sema;
#ifdef FLUSH
		DCACHE_INVAL(tinfo->si_inq, SCSI_INQUIRY_LEN);
#endif

		TRACE (ctlr, "QLFCINFO: sending inquiry to %d-%d\n",
			req->sr_target,req->sr_lun,0,0,0,0);
		qlfc_command(req);

		psema(sema, PRIBIO);

		if (req->sr_status == SC_GOOD && req->sr_scsi_status == ST_GOOD) {


			/*
			 * If the LUN is not supported, then return
			 * NULL. However, if this LUN is a candidate for
			 * failover we still may want to treat it as a valid
			 * device.
			 */

			if ((tinfo->si_inq[0] & 0xC0) == 0x40 &&
			    !fo_is_failover_candidate(tinfo->si_inq, ql_lun_vhdl)) {
			}
			else {
				scsi_device_update(tinfo->si_inq,ql_lun_vhdl);
				if (tinfo->si_inq[SCSI_INFO_BYTE] & SCSI_CTQ_BIT) {
					tinfo->si_ha_status |= SRH_MAPUSER | SRH_ALENLIST |
							      SRH_TAGQ | SRH_QERR0 ;
							    /* was   SRH_TAGQ | SRH_QERR0 | SRH_QERR1; */
					tinfo->si_maxq = MAX_QUEUE_DEPTH-1;
				}

				atomicClearInt(&qli->qli_dev_flags, DFLG_INITIALIZED);
				retval = tinfo;

				if (lun == 0) {
					qlfc_getluns (ctlr,
						      ctlr->target_map[req->sr_target].tm_qti,
						      req,
						      sema);
				}
			}
		}

		else if (req->sr_scsi_status == ST_BUSY) delay(HZ/2);

	} while (((req->sr_status == SC_ATTN || req->sr_status == SC_CMDTIME) && retry--)
		|| (req->sr_scsi_status == ST_BUSY && busy_retry--));


	freesema(sema);
	kern_free(sema);

	kern_free(req);
	kern_free(sensep);

	mutex_unlock(&qli->qli_open_mutex);

	return retval;
}
/* FILE: infrastructure.c */
/****************************************************************/
/****************************************************************/
/*
 * create the info structure associated with a
 * lun vertex
 */
static scsi_target_info_t *
qlfc_scsi_target_info_init(void)
{
	scsi_target_info_t 	*tinfo;

	/*
	 * allocate memory
	 */
	tinfo = (scsi_target_info_t *)kern_calloc(1,sizeof(scsi_target_info_t));

	tinfo->si_inq = (u_char * ) kmem_zalloc (SCSI_INQUIRY_LEN,
						VM_CACHEALIGN | VM_DIRECT);

	tinfo->si_sense = (u_char * )kmem_zalloc (REQ_SENSE_DATA_LENGTH,
						VM_CACHEALIGN | VM_DIRECT);

	return(tinfo);
}

/*
 * allocate and initialize the qlfc specific part of lun info
 */
static qlfc_local_lun_info_t *
qlfc_local_lun_info_init(void)
{
	qlfc_local_lun_info_t 	*info;

	info 	= (qlfc_local_lun_info_t *)kern_calloc(1,sizeof(qlfc_local_lun_info_t));
	ASSERT(info);

	/*
	 * By default assume the QERR bit of a device is set to 0.
	 */

	info->qli_dev_flags = 0;

	return info;
}

/*
 * allocate and initialize the qlfc specific part of lun info
 */
static qlfc_local_targ_info_t *
qlfc_local_targ_info_init(vertex_hdl_t targ_vhdl)
{
	qlfc_local_targ_info_t 	*info;

	info 	= (qlfc_local_targ_info_t *)kern_calloc(1,sizeof(qlfc_local_targ_info_t));
	ASSERT(info);

	info->req_active = NULL;
	info->req_last = NULL;
	info->req_count = 0;
	INITLOCK (&info->target_mutex,"qlfc_targ",targ_vhdl);
	return info;
}

/*
 * add a ql device vertex to the hardware graph
 *
 * Note: target_map mutex must be locked by caller!
 */
static vertex_hdl_t
qlfc_device_add (QLFC_CONTROLLER *ctlr, int targ, uint64_t node_name, uint64_t port_name, int lun)
{
	vertex_hdl_t		lun_vhdl;
	vertex_hdl_t		targ_vhdl;
	vertex_hdl_t		ctlr_vhdl = ctlr->ctlr_vhdl;
	scsi_lun_info_t		*lun_info;
	scsi_targ_info_t	*targ_info;
	qlfc_local_lun_info_t	*qli;
	qlfc_local_targ_info_t	*qti;

	if (lun == 0 && !mutex_mine(&ctlr->target_map[targ].tm_mutex)) {
		cmn_err(CE_PANIC, "qlfc: Mutex not owned by caller.");
	}

	if (node_name && qlfc_use_fabric_infrastructure) {
		lun_vhdl = scsi_fabric_lun_vhdl_get(ctlr_vhdl, node_name, fc_short_portname(node_name,port_name), lun);
	}
	else {
		lun_vhdl = scsi_lun_vhdl_get(ctlr_vhdl, targ, lun);
	}

	if (lun_vhdl == GRAPH_VERTEX_NONE) {
		if (node_name && qlfc_use_fabric_infrastructure) {
			TRACE (ctlr, "adding device node/port %llx/%llx, targ %d, lun %d\n", node_name, port_name, targ, lun, 0, 0);
		}
		else {
			TRACE (ctlr, "adding device targ %d, lun %d\n",targ,lun,0,0,0,0);
		}

		/*
		 * add the target vertex (if doesn't already exist)
		 */

		if (node_name && qlfc_use_fabric_infrastructure) {
			targ_vhdl = scsi_node_port_vertex_add(ctlr_vhdl, node_name, fc_short_portname(node_name,port_name));
		}
		else {
			targ_vhdl = scsi_targ_vertex_add(ctlr_vhdl,targ);
		}

		/*
		 * add the lun vertex
		 */

		lun_vhdl = scsi_lun_vertex_add(targ_vhdl,lun);
		TRACE (ctlr, "DEV_ADD : lun_vhdl %d, targ_vhdl %d, lun %d\n",(__psint_t)lun_vhdl,targ_vhdl,lun,0,0,0);


		/*
		 * get the lun info pointer
		 */

		lun_info = scsi_lun_info_get(lun_vhdl);
		TRACE (ctlr, "DEV_ADD : lun_info 0x%x\n",(__psint_t)lun_info,0,0,0,0,0);

		/*
		** initialize the qlfc specific targ info
		*/

		if ((qti = STI_INFO(SLI_TARG_INFO(lun_info))) == NULL) {
			targ_info = scsi_targ_info_get (targ_vhdl);
			STI_INFO(targ_info) = qti = qlfc_local_targ_info_init (targ_vhdl);
			qti->targ_vertex = targ_vhdl;
			qti->target = targ;
			STI_TARG(targ_info) = targ;
		}

		/*
		 * initialize the ql specific lun info
		 */

		SLI_INFO(lun_info) = qli = qlfc_local_lun_info_init();
		qli->qli_lun = lun;

		IOLOCK (qti->target_mutex);


		if (!qti->local_lun_info) {
			qti->local_lun_info = qti->last_local_lun_info = qli;
		}
		else {
			qti->last_local_lun_info->next = qli;
			qti->last_local_lun_info = qli;
		}
		qli->next = NULL;

		IOUNLOCK (qti->target_mutex);

		INITLOCK (&qli->qli_open_mutex,"qlfc_open",lun_vhdl);
		INITLOCK (&qli->qli_lun_mutex, "qlfc_lun",lun_vhdl);

		/*
		** Initialize the qlfc specific "target" info
		*/

		TINFO(lun_info)	= qlfc_scsi_target_info_init();

		/*
		** Now that the initialization is complete, fill in the qti value.
		*/

		ctlr->target_map[targ].tm_qti = qti;

	}
	return lun_vhdl;
}

/*
 * remove the ql device vertex and the path leading to it from
 * controller vertex ( target/#/lun/#).
 *
 * the basic idea is to remove the last vertex in target/#/lun/# subpath.
 * if this is the last vertex to be removed we don't need the "lun" part of
 * the path and we can remove that. this part of the logic is handled by
 * scsi_lun_vertex_remove
 *
 * if the "lun" vertex of the above path has been removed
 * scsi_lun_vertex_remove will return GRAPH_VERTEX_NONE in which case
 * we have targ/# subpath left and we can go ahead and remove the last
 * vertex corr. to specific target number.
 * as before we then check if there are any specific target number vertices.
 * if there aren't any we can remove the "target" vertex also. this part of
 * the logic is handled by scsi_targ_vertex_remove
 *
 * Note: target_map mutex must be locked by caller!
 */

static void
qlfc_device_remove(QLFC_CONTROLLER *ctlr, int targ, uint64_t node_name, uint64_t port_name, int lun)
{
	vertex_hdl_t		ctlr_vhdl, targ_vhdl, lun_vhdl;
	graph_error_t		rv;
	char			loc_str[128];
	scsi_lun_info_t		*lun_info;
	qlfc_local_lun_info_t	*qli, *prev, *current;
	qlfc_local_targ_info_t	*qti;
	scsi_target_info_t	*tinfo;

	int step=0;

	if (lun == 0 && !mutex_mine(&ctlr->target_map[targ].tm_mutex)) {
		cmn_err(CE_PANIC, "qlfc: Mutex not owned by caller.");
	}

	ctlr_vhdl = ctlr->ctlr_vhdl;
	TRACE (ctlr,"removing device targ %d, lun %d\n",targ,lun,0,0,0,0);
	if (node_name && qlfc_use_fabric_infrastructure) {
		sprintf(loc_str, NODE_PORT_NUM_FORMAT, node_name, fc_short_portname(node_name,port_name));
	}
	else {
		sprintf(loc_str, TARG_NUM_FORMAT, targ);
	}
	rv = hwgraph_traverse(ctlr_vhdl, loc_str, &targ_vhdl);

	if (rv != GRAPH_SUCCESS)
		cmn_err (CE_PANIC, "qlfc: cannot remove device, hwgraph traversal failed.");
	ASSERT(rv == GRAPH_SUCCESS);	rv = rv;

	hwgraph_vertex_unref(targ_vhdl);

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);
	/*
	 * Cleanup the qlfc_ specific lun info
	 */
	sprintf(loc_str, LUN_NUM_FORMAT, lun);
	rv = hwgraph_traverse(targ_vhdl, loc_str, &lun_vhdl);

	if (rv != GRAPH_SUCCESS || lun_vhdl == GRAPH_VERTEX_NONE)
		cmn_err (CE_PANIC, "qlfc: cannot remove device, hwgraph traversal failed or invalid lun vhdl.");
	ASSERT((rv == GRAPH_SUCCESS) && (lun_vhdl != GRAPH_VERTEX_NONE));	rv = rv;

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	lun_info = scsi_lun_info_get(lun_vhdl);
	qti = STI_INFO(SLI_TARG_INFO(lun_info));
	qli = SLI_INFO (lun_info);
	IOLOCK (qti->target_mutex);

	current = qti->local_lun_info;
	if (!current) panic ("qlfc: removing non-existant lli");


	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	prev = NULL;
	while (current) {
		if (current == qli) {
			if (!prev) qti->local_lun_info = current->next;
			else prev->next = current->next;
			if (current == qti->last_local_lun_info) qti->last_local_lun_info = prev;
			break;
		}
		prev = current;
		current = current->next;
	}

	IOUNLOCK (qti->target_mutex);


	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	mutex_destroy(&qli->qli_open_mutex);

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	mutex_destroy(&qli->qli_lun_mutex);

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	tinfo = TINFO(lun_info);
	kmem_free (tinfo->si_inq,SCSI_INQUIRY_LEN);
	kmem_free (tinfo->si_sense,REQ_SENSE_DATA_LENGTH);
	kern_free (tinfo);

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	kern_free(SLI_INFO(lun_info));

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);	/* 88888888888888888 */

	SLI_INFO(lun_info) = NULL;

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	hwgraph_vertex_unref(lun_vhdl);

	TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);
	/*
	** If we removed the last lun, remove the target vertex
	*/
	if (scsi_lun_vertex_remove(targ_vhdl,lun) == GRAPH_VERTEX_NONE) {
		TRACE (ctlr,"removed last lun.\n",0,0,0,0,0,0);
		mutex_destroy (&qti->target_mutex);
		ctlr->target_map[targ].tm_qti = NULL;
		ctlr->target_map[targ].tm_port_id = 0;
		ctlr->target_map[targ].tm_node_name = 0;
		ctlr->target_map[targ].tm_port_name = 0;
		TRACE (ctlr,"freeing local targ info: 0x%x\n",(__psint_t)qti,0,0,0,0,0);
		kern_free(qti);

		TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

		if (node_name && qlfc_use_fabric_infrastructure) {
			(void) scsi_node_port_vertex_remove(ctlr_vhdl, node_name, fc_short_portname(node_name,port_name));
		}
		else {
			(void) scsi_targ_vertex_remove(ctlr_vhdl,targ);
		}

		TRACE (ctlr,"removing device : step %d\n",++step,0,0,0,0,0);

	}
	return;
}

/*
 * create a vertex for the ql controller in the
 * hardware graph and allocate space for the info
 * the vertex passed in is our pci connection point
 */

static void
qlfc_controller_add (QLFC_CONTROLLER *ctlr)
{

	graph_error_t		rv;
	scsi_ctlr_info_t	*ctlr_info;
	char			ctlr_string[128];

	/*
	 * add the ql controller vertex to the hardware graph,
	 * if it has not been done already. the canonical path
	 * goes up through our connection point. convenience
	 * links are taken care of elsewhere.
	 */

	if ((strlen(EDGE_LBL_SCSI_CTLR)+4) >= sizeof(ctlr_string)) panic ("string is too long!\n");

	sprintf (ctlr_string, EDGE_LBL_SCSI_CTLR "/%d", ctlr->ctlr_which);

	rv = hwgraph_path_add ( ctlr->pci_vhdl,
				ctlr_string,
				&ctlr->ctlr_vhdl);

	sprintf (ctlr->hwgname, "%v", ctlr->ctlr_vhdl);

	ASSERT((rv == GRAPH_SUCCESS) &&
	       (ctlr->ctlr_vhdl != GRAPH_VERTEX_NONE));	rv = rv;

#if IP30
	/*
	 * At the moment, the IP30 boot sequence depends on
	 * using /scsi/ to find a disk.
	 */
	rv = hwgraph_edge_add(	ctlr->pci_vhdl, ctlr->ctlr_vhdl, EDGE_LBL_SCSI);
	ASSERT(rv == GRAPH_SUCCESS);	rv = rv;
#endif

	/*
	 * allocate memory for the info structure if it has not
	 * been done already
	 */
	ctlr_info = (scsi_ctlr_info_t *)hwgraph_fastinfo_get(ctlr->ctlr_vhdl);
	if (!ctlr_info) {
		ctlr_info 			= scsi_ctlr_info_init();
		SCI_CTLR_VHDL(ctlr_info)	= ctlr->ctlr_vhdl;

		SCI_ADAP(ctlr_info)		= ctlr->ctlr_which;

		SCI_ALLOC(ctlr_info)		= qlfc_alloc;
		SCI_COMMAND(ctlr_info)		= qlfc_command;
		SCI_FREE(ctlr_info)		= qlfc_free;
		SCI_DUMP(ctlr_info)		= qlfc_dump;
		SCI_INQ(ctlr_info)		= qlfc_info;
		SCI_IOCTL(ctlr_info)		= qlfc_ioctl;
		SCI_ABORT(ctlr_info)		= qlfc_abort;

		scsi_ctlr_info_put(ctlr->ctlr_vhdl,ctlr_info);
		scsi_bus_create(ctlr->ctlr_vhdl);
	}
	hwgraph_vertex_unref(ctlr->ctlr_vhdl);
	return;
}
/* FILE: util.c */
/*
 * get the pointer to host adapter info given the
 * vertex handle of the vertex  corr. to the
 * controller
 */
QLFC_CONTROLLER *
qlfc_ctlr_info_from_ctlr_get  (vertex_hdl_t ctlr_vhdl)
{
	scsi_ctlr_info_t	*ctlr_info;

	if (ctlr_vhdl == GRAPH_VERTEX_NONE)
		return NULL;

	ctlr_info = scsi_ctlr_info_get(ctlr_vhdl);
	ASSERT(ctlr_info);
	return(SCI_CTLR(ctlr_info));
}

/*
** Get the local target info pointer given a lun vhdl
*/

/* REFERENCED */
static qlfc_local_targ_info_t *
qlfc_target_info_from_lun_vhdl (vertex_hdl_t lun_vhdl)
{
	return (qlfc_local_targ_info_t *)
		STI_INFO(SLI_TARG_INFO(scsi_lun_info_get(lun_vhdl)));
}

/*
 * get the pointer to host adapter info given the
 * qlfc_ device vertex handle
 */
static QLFC_CONTROLLER *
qlfc_ctlr_from_lun_vhdl_get(vertex_hdl_t 	lun_vhdl)
{

	scsi_lun_info_t		*lun_info;

	lun_info = scsi_lun_info_get(lun_vhdl);
	ASSERT(lun_info);
	ASSERT(SLI_CTLR_INFO(lun_info));

	return(SLI_CTLR(lun_info));

}
/*
 * put the pointer to host adapter structure into the info
 * associated with the controller vertex
 */
static void
qlfc_ctlr_info_put( QLFC_CONTROLLER *ctlr)
{
	scsi_ctlr_info_t		*ctlr_info;

	/*
	 * the info structure associated with the controller
	 * vertex has been created when the vertex was added
	 */

	ctlr_info = scsi_ctlr_info_get(ctlr->ctlr_vhdl);
	ASSERT(ctlr_info);
	ASSERT(!SCI_CTLR(ctlr_info));

	SCI_INFO(ctlr_info) = ctlr;
}

/*
 * scsi target info is the info associated with the
 * lun vertex.get this info .
 */
static scsi_target_info_t *
qlfc_lun_target_info_get(vertex_hdl_t	lun_vhdl)
{
	scsi_lun_info_t		*lun_info = NULL;
	scsi_target_info_t	*tinfo;

	lun_info = scsi_lun_info_get(lun_vhdl);
	ASSERT(lun_info);

	tinfo = TINFO(lun_info);
	ASSERT(tinfo);

	return tinfo;
}

/* FILE: interrupt.c */
/*
** Entry-Point
** Function:    qlfc_intr
**
** Description: This is the interrupt service routine.  This is called
**      by the system to process a hardware interrupt for our
**      board, or by one of our internal routines that need to
**      check for an interrupt by polling.  These routines MUST
**      lock out interrupts (splx) before calling here.
**
**	This routine locks res_lock at the beginning and doesn't
**	release it until it's time for post processing.  Called routines
**	should not release and reacquire the lock to play games with
**	this.  Reorder your changes!
*/

void
qlfc_intr(QLFC_CONTROLLER *ctlr)
{
	CONTROLLER_REGS		*isp = (CONTROLLER_REGS *)ctlr->base_address;
	uint16_t		isr = 0;
	scsi_request_t		*compl = NULL;

	IOLOCK(ctlr->res_lock);

	if (ctlr->flags & CFLG_IN_INTR) panic ("qlfc: already in interrupt handler!\n");

	atomicSetInt(&ctlr->flags, CFLG_IN_INTR);

	if (!(ctlr->flags & CFLG_SHUTDOWN)) {

		/*
		** Determine what type (if any) of interrupt is
		** present.
		*/

		isr = QL_PCI_INH(&isp->isr);

		if (isr & ISR_RISC_INTERRUPT) {
			qlfc_service_interrupt(ctlr, isr);
			if ( compl = ctlr->compl_forw ) {	/* yes, assignment */
				ctlr->compl_forw = ctlr->compl_back = NULL;
			}

		}

		else if (isr & ISR_INTERRUPT) {	/* any other interrupt */
			TRACE (ctlr, "Unexpected interrupt from controller: isr: 0x\n",isr,0,0,0,0,0);
		}
	}

	atomicClearInt(&ctlr->flags, CFLG_IN_INTR);

	IOUNLOCK(ctlr->res_lock);

	/*
	** Notify caller(s) of command competions
	*/

	if (compl) {
		atomicClearInt (&ctlr->flags, CFLG_START_THROTTLE);
		qlfc_notify_responses(ctlr, compl);
	}

	/*
	** Some resources may have become available. See if there
	** are any requests to send.
	*/

	if (!(ctlr->flags & CFLG_DRAIN_IO) && ((ctlr->waitf) || (ctlr->req_forw)))
		qlfc_start_scsi(ctlr);
}


/*
** Function:    qlfc_service_interrupt
**
** Description: This routine actually services the interrupt.
**      There are two kinds of interrupts: 1) Responses available
**      and 2) a Mailbox request completed.  ALWAYS service the
**      Mailbox interrupts first.  Then process all responses we
**      have (may be more than one).  Do this by pulling off all
**      the completed responses and putting them in a linked list.
**      Then after we have them all, tell the adapter we have them
**      and do completion processing on the list.  This limits
**      the amount of time the adapter has to wait to post new
**      responses.
**
**	The driver should then read mailbox 0. If Mailbox 0 contains a
**	value of 0x8000 or greater then the Interrupt is for an Asynchronous
**	Event. If Mailbox 0 contains a value between 0x4000 and 0x7fff
**	then the value is reporting the results of a mailbox command.
**
**	If the interrupt is for a Mailbox Command completion or an
**	Asynchronous Event the driver must write a 0 to the Semaphore
**	register when it has finished reading the data from the Mailboxes.
**	Clearing the Semaphore informs the ISP that the driver has read
**	all the information associated with the Mailbox Command or Async.
**	Event. When the Semaphore is 0 the ISP is free to update the contents
**	of the Mailbox registers. The driver may clear the semaphore before
**	or after clearing the interrupt from the ISP.
**
**	If the Semaphore register contains a 0 then the interrupt is
**	reporting that the ISP has added an entry or entries to the
**	response queue and the driver should read mailbox 5 to determine
**	how many entries were added. The ISP will not add anymore entries
**	to the queue or update mailbox 5 until the driver has cleared the
**	interrupt from the ISP.
**
**	To clear an interrupt from the ISP the driver must write a value of
**	0x7000 to the Host Command and Control (HCCR) register of the ISP.
**
**	If the ISP has reported an interrupt to the host it will not modify
**	the contents of the Semaphore register, or Mailbox registers 0-3,5-7.
**	Until the driver has cleared the interrupt. However, the ISP will
**	continu to remove entries from the Request Queue so it is possible
** 	that the ISP may modify the contents of Mailbox 4 while an interrupt
**	is in progress.
**
*/

static void
qlfc_service_interrupt(QLFC_CONTROLLER *ctlr, uint16_t isr)
{
    	status_entry		*q_ptr;
    	scsi_request_t		*request;
	qlfc_local_lun_info_t	*qli;
	qlfc_local_targ_info_t	*qti;
    	CONTROLLER_REGS		*isp = (CONTROLLER_REGS *)ctlr->base_address;
	REQ_COOKIE		cookie;
	scsi_lun_info_t		*lun_info;
	scsi_targ_info_t	*targ_info;

	int			interrupts_serviced=0;
	int			requeue = 0;

	uint16_t		response_in=0;
	uint16_t		response_out=0;
	uint16_t		bus_sema;


    	/* Process all the mailbox interrupts and all responses.  */

	ctlr->compl_forw = ctlr->compl_back = NULL;

    	do  {
		/*
		 * Check for async mailbox events.
		 */
		bus_sema = QL_PCI_INH (&isp->bus_sema);

		TRACE (ctlr, "INTERRPT: isr 0x%x, sema 0x%x, ints %d\n",isr, bus_sema, ++interrupts_serviced,0,0,0);

		isr = 0;

		if (bus_sema & ISEM_LOCK) {
			/*
			** Servicing a mailbox interrupt may queue responses
			** on compl_forw.  This queue is protected by res_lock.
			*/
			qlfc_service_mbox_interrupt(ctlr);

			if (ctlr->flags & CFLG_ISP_PANIC) {
				QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
				break;
			}
		}

		else {
			atomicClearInt (&ctlr->flags, CFLG_SOLID_LIP);
			atomicClearInt (&ctlr->lip_resets, 0xffffffff);
		}

		/*
		**
		** CFLG_RESPONSE_QUEUE_UPDATE is set by do_mbox_cmd when POLLING for
		** mailbox interrupt.  This can happen if a mailbox command is ISSUED
		** while in this interrupt handler during notify response.  This
		** means that we spin on the completion of that command and the firmware
		** posts an "oh, while you're at it" completion in the response queue.
		** If we're using fast posting, this would be reporting a scsi
		*  command error.
		*/

		atomicClearInt(&ctlr->flags, CFLG_RESPONSE_QUEUE_UPDATE);

		/*
		 * If there aren't any completed responses then don't do any
		 * of the following. i.e. it was just a mailbox interrupt or
		 * spurious becuase right now I poll on mailbox commands.
		 */

		response_in = ctlr->response_in = QL_PCI_INH(&isp->mailbox5);

		response_out = ctlr->response_out;
		if ((response_in == ctlr->response_out) || !(ctlr->flags & CFLG_INITIALIZED)) {

#ifdef QL_SRAM_PARITY
/* make certain this code makes sense before enabling! It's probably broke as it!!! */
			uint16_t hccr=0;

			/* check for an sram parity error condition */
			hccr = QL_PCI_INH(&isp->hccr);
			if(hccr & HCCR_RISC_PARITY_ERROR)	{
				cmn_err(CE_WARN,"\n%s Q-logic sram parity error detected\n",
					ctlr->hwgname);
				if(hccr & HCCR_SRAM_PARITY_BANK0)
					cmn_err(CE_WARN,"%s Q-logic sram parity error on bank 0\n",
						ctlr->hwgname);
				if(hccr & HCCR_SRAM_PARITY_BANK1)
					cmn_err(CE_WARN,"%s Q-logic sram parity error on bank 1\n",
						ctlr->hwgname);
				IOUNLOCK(ctlr->res_lock);
				QL_MUTEX_ENTER(ctlr);
				qlfc_reset_interface(ctlr,1);
				qlfc_flush_queue(ctlr, SC_HARDERR, ALL_QUEUES);
				IOUNLOCK(ctlr->req_lock);
			}
#endif
			QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
			break;	/* out of main do-while loop */
		}

		/*
		** Remove all of the responses from the queue.
		*/

		while (response_out != response_in) {
#ifdef FLUSH
			status_entry	ql_iorb;

			ql_iorb = *(status_entry *)ctlr->response_ptr;

			/*
			 * there is a small race condition between a ql write to the
			 * second half of the cacheline and the cacheline invalidation
			 */
			DCACHE_INVAL(
				     (void *)((__psunsigned_t)ctlr->response_ptr & CACHE_SLINE_MASK),
				     CACHE_SLINE_SIZE);

			/*
			 * Get a pointer to the next response entry.
			 */
			q_ptr = &ql_iorb;
#else
			q_ptr = (status_entry *)ctlr->response_ptr;
#endif	/* FLUSH */

#if (defined(SN) || defined(IP30)) && !defined(USE_MAPPED_CONTROL_STREAM)
			qlfc_swap32 ((uint8_t *)q_ptr,IOCB_SIZE);
#endif

			TRACE (ctlr, "INTERRUPT: response in %d, response_out %d, response_base 0x%x, response_ptr 0x%x\n",
				response_in, response_out, (__psint_t)ctlr->response_base, (__psint_t) ctlr->response_ptr, 0,0);

			/*
			 * Check for DEAD response. If dead response, then exit
			 * the response processing loop. The LEGO architecure
			 * guarantees no relative completion order vs. issue order
			 * of write DMAs and PIOs. Consequently, the PIO to read
			 * the response head pointer register may actually
			 * complete before the DMA write to update the
			 * corresponding response block completes. Imagine the
			 * following scenario: Command A completes and causes an
			 * interrupt. While in the ISR command B completes, which
			 * we know about by reading the response tail pointer
			 * (mailbox 5), but the DMA write to update the response
			 * block hasn't yet completed. If at this point we bail
			 * out and await the next interrupt, we'll pick up the
			 * response for command B then. An interrupt is a barrier
			 * operation so we're guaranteed that the DMA write will
			 * have completed by the time we get the interrupt.
			 */
			if (((volatile status_entry *)q_ptr)->handle == 0xDEAD) {
				cmn_err(CE_WARN, "Premature response seen: handle 0x%x, q_ptr 0x%x.\n",
					q_ptr->handle, q_ptr);
				break;
			}

			/*
			 * Move the pointers for the next entry.
			 */
			if (response_out == (ctlr->ql_response_queue_depth - 1)) {
				response_out = 0;
				ctlr->response_ptr = ctlr->response_base;
			}
			else {
				response_out++;
				ctlr->response_ptr+=IOCB_SIZE;
			}

			/*
			 * Make sure the driver issued the command right.
			 */
			if ((q_ptr->hdr.flags & EF_ERROR_MASK) || q_ptr->hdr.entry_type != ET_STATUS) {
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Invalid status response received: "
					 "entry_cnt 0x%x, entry_type 0x%x, flags 0x%x, sys_def_1 0x%x\n",
					 q_ptr->hdr.entry_cnt, q_ptr->hdr.entry_type,
					 q_ptr->hdr.flags, q_ptr->hdr.sys_def_1);
				TRACE (ctlr, "INTR    : Invalid status response received: "
					 "entry_cnt 0x%x, entry_type 0x%x, flags 0x%x, sys_def_1 0x%x\n",
					 q_ptr->hdr.entry_cnt, q_ptr->hdr.entry_type,
					 q_ptr->hdr.flags, q_ptr->hdr.sys_def_1,0,0);
				atomicSetInt (&ctlr->flags, CFLG_BAD_STATUS|CFLG_DRAIN_IO);
				ctlr->drain_count = ctlr->ql_ncmd;
				ctlr->drain_timeout = DRAIN_TIME;
				continue;
			}

			if (q_ptr->hdr.flags & EF_ERROR_MASK) {
				if(q_ptr->hdr.flags & EF_BUSY)
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Detected busy condition\n");
				if(q_ptr->hdr.flags & EF_BAD_HEADER)
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Detected a bad header\n");
				if(q_ptr->hdr.flags & EF_BAD_PAYLOAD)
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Detected a bad payload\n");
				continue;
			}

			if (q_ptr->hdr.entry_cnt != 1) {
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Status response entry count too big: "
					 "entry_cnt 0x%x, entry_type 0x%x, flags 0x%x, sys_def_1 0x%x\n",
					 q_ptr->hdr.entry_cnt, q_ptr->hdr.entry_type,
					 q_ptr->hdr.flags, q_ptr->hdr.sys_def_1);
				continue;
			}

			cookie.data = q_ptr->handle;
			request = qlfc_deque_sr (ctlr, cookie.data);

			ctlr->ha_last_intr = lbolt/HZ;

			/*
			 * Mark handle as "DEAD"
			 */
			q_ptr->handle = 0xDEAD;

			if (request == NULL) continue;

			/*
			 * Update LUN status - if all aborted commands have been
			 * returned, transfer all commands queued during the abort
			 * phase into the wait Q.
			 */
			lun_info = scsi_lun_info_get (request->sr_lun_vhdl);
			targ_info = SLI_TARG_INFO(lun_info);
			qli = SLI_INFO(lun_info);
			qti = STI_INFO(targ_info);

			atomicAddInt(&ctlr->ql_ncmd,-1);

			if ((ctlr->flags & CFLG_DRAIN_IO) && (ctlr->ql_ncmd == 0)) {
				atomicClearInt (&ctlr->flags, CFLG_DRAIN_IO);
			}


			IOLOCK(qli->qli_lun_mutex);

			atomicAddInt(&qti->req_count,-1);
			atomicAddInt(&qli->qli_cmd_rcnt, -1);

			if (qlfc_debug >= 99) {
				TRACE (ctlr, "INTR CNT: qli_cmd_rcnt %d, req_count %d,ql_ncmd %d\n",qli->qli_cmd_rcnt,
					qti->req_count,ctlr->ql_ncmd,0,0,0);
			}

			if ((qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) && (qli->qli_cmd_rcnt == 0)) {
				scsi_request_t *lun_forw;
				scsi_request_t *lun_back;

				lun_forw = qli->qli_awaitf;
				lun_back = qli->qli_awaitb;
				qli->qli_awaitf = qli->qli_awaitb = NULL;
				qli->qli_cmd_awcnt = 0;
				atomicClearInt(&qli->qli_dev_flags, DFLG_ABORT_IN_PROGRESS);
				atomicSetInt(&qli->qli_dev_flags, DFLG_SEND_MARKER);
				IOUNLOCK(qli->qli_lun_mutex);

				if (lun_forw) {
					IOLOCK(ctlr->waitQ_lock);
					if (ctlr->waitf) {
						ctlr->waitb->sr_ha = lun_forw;
						ctlr->waitb = lun_back;
					}
					else {
						ctlr->waitf = lun_forw;
						ctlr->waitb = lun_back;
					}
					ctlr->waitb->sr_ha = NULL;
					IOUNLOCK(ctlr->waitQ_lock);
				}
			}
			else {
				IOUNLOCK(qli->qli_lun_mutex);
			}

			TRACE (ctlr, "INTR    : %d %d: sr 0x%x, ncmd %d, qli_cmd_rcnt %d, req_count %d\n",
				STI_TARG(targ_info),
				SLI_LUN(lun_info),
				(__psint_t)request, 
				ctlr->ql_ncmd,
				qli->qli_cmd_rcnt,qti->req_count);

			TRACE (ctlr, "INTR    : %d %d: comp status %s, state 0x%x, status 0x%x, scsi status 0x%x\n",
				STI_TARG(targ_info),
				SLI_LUN(lun_info),
				(__psint_t)(qlfc_completion_status_msg(q_ptr->completion_status)),
				q_ptr->state_flags,
				q_ptr->status_flags,
				q_ptr->fc_scsi_status);

			request->sr_status = SC_GOOD;
			request->sr_scsi_status = (q_ptr->fc_scsi_status & SCSI_STATUS_MASK);
			if (q_ptr->fc_scsi_status & FCS_RESIDUAL_UNDERRUN)
				request->sr_resid = q_ptr->residual;
			else
				request->sr_resid = 0;


			switch(q_ptr->completion_status)
			{
			case SCS_COMPLETE:	/* OK completion */
				atomicClearInt (&qti->recovery_step, 0xffffffff);
				atomicClearInt (&ctlr->recovery_step, 0xffffffff);
				break;

			case SCS_DATA_UNDERRUN:
				/*
				** Underruns are tricky.  The residual information is
				** what is returned by the device.  If the device indicates
				** that an underrun has transpired, we treat that as if the
				** request was okay, returning the residual in the sr.
				**
				** If the device doesn't flag an underrun, but the controller
				** does, then something is missing and we haven't a clue what.
				** Consequently, it is treated as a hardware error, hoping that
				** the dksc retry will correct it.

				qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "Data underrun.\n");

				*/

				TRACE (ctlr, "INTR    : UNDERRUN: residual %d\n",request->sr_resid,0,0,0,0,0);
				if (!(q_ptr->fc_scsi_status & FCS_RESIDUAL_UNDERRUN)) {
					goto comp_hard_error;
				}
				break;

			case SCS_PORT_LOGGED_OUT:
				/*
				** If port logged out, it might be due to SCN/PDBC.
				** We'll let this and port unavailable be retried.
				*/
				if (!(ctlr->flags & CFLG_PROBE_ACTIVE)) {
					qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "Port logged out.\n");
					if (!(ctlr->target_map[STI_TARG(targ_info)].tm_flags & (TM_AWOL_PENDING|TM_AWOL))) {
						qlfc_stop_target_queue (ctlr,STI_TARG(targ_info));
						qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_SET_TARGET_AWOL,STI_TARG(targ_info));
					}
				}
				request->sr_status = SC_CMDTIME;	/* allow retry */
				break;

			case SCS_PORT_UNAVAILABLE:
				if (!(ctlr->flags & CFLG_PROBE_ACTIVE)) {
					qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "Port unavailable.\n");
					if (!(ctlr->target_map[STI_TARG(targ_info)].tm_flags & (TM_AWOL_PENDING|TM_AWOL))) {
						qlfc_stop_target_queue (ctlr,STI_TARG(targ_info));
						qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_SET_TARGET_AWOL,STI_TARG(targ_info));
					}
				}
				request->sr_status = SC_CMDTIME;
				break;
			case SCS_DMA_ERROR:
#ifdef A64_BIT_OPERATION
				if (q_ptr->state_flags & SS_TRANSFER_COMPLETE) {
					qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info),
						 "INVALID DMA ERROR: "
						 "completion status [0x%x] (%s), "
						 "scsi status[0x%x], state flags[0x%x], status flags[0x%x]\n",
						 q_ptr->completion_status,
						 qlfc_completion_status_msg(q_ptr->completion_status),
						 q_ptr->fc_scsi_status,
						 q_ptr->state_flags,
						 q_ptr->status_flags);
					request->sr_status = SC_HARDERR;
				}
				else
#endif
					goto comp_hard_error;
				break;

			case SCS_RESET_OCCURRED:
			case SCS_ABORTED:
				TRACE (ctlr, "INTR    : ATTN.\n",0,0,0,0,0,0);
#if 0
				qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "Attention.\n");
#endif
				request->sr_status = SC_ATTN;
				atomicSetInt(&qli->qli_dev_flags, DFLG_SEND_MARKER);
				break;

			case SCS_TIMEOUT:
				TRACE (ctlr, "INTR    : TIMEOUT.\n",0,0,0,0,0,0);
				qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "Timeout.\n");
				request->sr_status = SC_CMDTIME;
				break;

			case SCS_DEVICE_QUEUE_FULL:
				TRACE (ctlr, "INTR    : QUEUE FULL, ncmd %d, req_count %d.\n",ctlr->ql_ncmd,qti->req_count,0,0,0,0);
				atomicSetInt (&ctlr->flags, CFLG_DRAIN_IO);
				qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), "QFULL\n");
				requeue = 1;
				break;

			case SCS_DATA_OVERRUN:
			default:
comp_hard_error:
				qlfc_tpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info),
					 "HARD ERROR: "
					 "completion status [0x%x] (%s), "
					 "scsi status[0x%x], state flags[0x%x], status flags[0x%x]\n",
					 q_ptr->completion_status,
					 qlfc_completion_status_msg(q_ptr->completion_status),
					 q_ptr->fc_scsi_status,
					 q_ptr->state_flags,
					 q_ptr->status_flags);
				request->sr_status = SC_HARDERR;
				break;
			} /* switch (q_ptr->completion_status) */

			/*
			 * does this completeion complete our QUIESCE_IN_PROGRESS
			 */
			if ((ctlr->flags & CFLG_QUIESCE_IN_PROGRESS) &&
			    (ctlr->ql_ncmd == 0)) {

				atomicSetInt(&ctlr->flags, CFLG_QUIESCE_COMPLETE);
				atomicClearInt(&ctlr->flags, CFLG_QUIESCE_IN_PROGRESS);
				untimeout(ctlr->quiesce_in_progress_id);
				ctlr->quiesce_id = timeout(kill_quiesce, ctlr, ctlr->quiesce_time);
			}


			/*
			 * If a SCSI bus reset occurred, then we need to
			 * issue a marker before issuing any new requests.
			 */
			if (q_ptr->completion_status == SCS_RESET_OCCURRED)
				atomicSetInt(&ctlr->flags, CFLG_SEND_MARKER);

			/*
			 * Save any request sense info.
			 */
			if (q_ptr->fc_scsi_status & FCS_SENSE_LENGTH_VALID) {
				atomicSetInt(&qli->qli_dev_flags, DFLG_CONTINGENT_ALLEGIANCE);

				/*
				 * Go abort all other queued commands if QERR=1
				 *	Should never happen.  Code remains as comment
				 *	for historical purposes.
				 *
				if (qli->qli_dev_flags & DFLG_BEHAVE_QERR1) {

					IOLOCK(qli->qli_lun_mutex);

					if ( (qli->qli_dev_flags & DFLG_ABORT_IN_PROGRESS) == 0 &&
					     (qli->qli_cmd_rcnt > 0)) {

						atomicSetInt(&qli->qli_dev_flags, DFLG_ABORT_IN_PROGRESS);
						IOUNLOCK (qli->qli_lun_mutex);
						qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_ABORT_DEVICE, (__psunsigned_t)lun_info);

					}
					else {
						IOUNLOCK (qli->qli_lun_mutex);
					}
				}
				 */

				if (request->sr_sense == NULL) {
					qlfc_tpmsg(ctlr->ctlr_vhdl,
						 STI_TARG(targ_info),
						 "No place for sense data (CDB 0x%x)\n",
						 request->sr_command[0]);
				}
				else {
					struct scsi_target_info *tinfo;
					int howmuch;

					qlfc_swap64(&q_ptr->req_sense_data[0],REQ_SENSE_DATA_LENGTH);


					if (q_ptr->req_sense_length > REQ_SENSE_DATA_LENGTH) {
						qlfc_lpmsg (ctlr->ctlr_vhdl, STI_TARG(targ_info), SLI_LUN(lun_info),
							"Sense data returned exceeds allocated size of %d.  Truncated.\n",
							REQ_SENSE_DATA_LENGTH);
						request->sr_sensegotten = REQ_SENSE_DATA_LENGTH;
					}
					else {
						request->sr_sensegotten = q_ptr->req_sense_length;
					}

					howmuch = (request->sr_senselen < request->sr_sensegotten) 
							? request->sr_senselen 
							: request->sr_sensegotten;

					TRACE (ctlr, "INTR    : bcopy (0x%x,0x%x,%d)\n",
						(__psint_t)q_ptr->req_sense_data,
						(__psint_t)request->sr_sense,
						howmuch,
						0,0,0);

					bcopy(q_ptr->req_sense_data,
					      request->sr_sense,
					      howmuch);

					TRACE (ctlr, "INTR    : sense: target %d: 0x%x 0x%x 0x%x\n",
						   STI_TARG(targ_info),
						   request->sr_sense[2],
						   request->sr_sense[12],
						   request->sr_sense[13],
						   0,0);
					#ifdef SENSE_PRINT
					qlfc_sensepr(ctlr->ctlr_vhdl,
						   STI_TARG(targ_info),
						   request->sr_sense[2],
						   request->sr_sense[12],
						   request->sr_sense[13]);
					#endif /* SENSE_PRINT */

					tinfo = qlfc_lun_target_info_get(request->sr_lun_vhdl);

					TRACE (ctlr, "INTR    : bcopy (0x%x,0x%x,%d)\n",(__psint_t)request->sr_sense,(__psint_t)tinfo->si_sense,howmuch,0,0,0);

					bcopy(request->sr_sense, tinfo->si_sense, howmuch);

					if (qli->qli_sense_callback)
						(*qli->qli_sense_callback)(request->sr_lun_vhdl, tinfo->si_sense);
				}
			}

			/*
			 * Link this response to be returned later.
			 */
			if (!requeue) {
				if (ctlr->compl_forw) {
					ctlr->compl_back->sr_ha = request;
				}
				else {
					ctlr->compl_forw = request;
				}
				ctlr->compl_back        = request;
				request->sr_ha = NULL;

				TRACE (ctlr, "INTR    : sr 0x%x, status %d, queued for notify.\n",
					(__psint_t)request,request->sr_status, 0,0,0,0);
			}
			else {
				qlfc_command (request);
				requeue = 0;
			}

			{
			char *r;
			int i;
			r = (char *)q_ptr;
			for (i=0; i<IOCB_SIZE; i++) *(r+i) = 0xee;
			}
		} /* while ((response_out != response_in)) */

		/*
		** Update the ISP's response tail pointer
		*/

		if (response_out != ctlr->response_out) {
			ctlr->response_out = response_out;
			QL_PCI_OUTH(&isp->mailbox5, ctlr->response_out);
		}

		QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
		flushbus();

		/* Get a new copy of the ISR. */
		isr = QL_PCI_INH(&isp->isr);

		/*
		** This bears repeating:
		**
		** CFLG_RESPONSE_QUEUE_UPDATE is set by do_mbox_cmd when POLLING for
		** mailbox interrupt.  This can happen if a mailbox command is ISSUED
		** while in this interrupt handler.
		*/

	} while ((isr & ISR_RISC_INTERRUPT) || (ctlr->flags & CFLG_RESPONSE_QUEUE_UPDATE));

}

static void
qlfc_notify_responses(QLFC_CONTROLLER *ctlr, scsi_request_t *forwp)
{
	qlfc_local_targ_info_t	*qti;
	scsi_lun_info_t		*lun_info;
	scsi_request_t		*request;

	while (request = forwp) { 	/* yes, assignment */
		forwp = request->sr_ha;

		lun_info = scsi_lun_info_get (request->sr_lun_vhdl);
		qti = STI_INFO(SLI_TARG_INFO(lun_info));

#ifdef HEART_INVALIDATE_WAR
		/* if this IO resulted in a DMA to memory from IO (disk read) 	*/
		/* then invalide cache copies of the data 			*/
		if (request->sr_flags & SRF_DIR_IN)
		{
			if (request->sr_flags & SRF_MAPBP)
			{
				bp_heart_invalidate_war(request->sr_bp);
			}
			else
			{
				heart_invalidate_war(request->sr_buffer,request->sr_buflen);
			}
		}
#endif /* HEART_INVALIDATE WAR */

		/*
		 * Callback now - As a result of callback processing we
		 * may have executed a mailbox command, which may have
		 * seen some IOCB completions. CFLG_RESPONSE_QUEUE_UPDATE will
		 * be set if so.
		 */
		qlfc_notify (ctlr, request, qti->target, SLI_LUN(lun_info));

	} /* while (request = forwp) */
}

/* FILE: nop.c */
#if 0
STATUS
qlfc_mbox_test (QLFC_CONTROLLER *ctlr)
{
	int		i;
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];

	uint16_t	test_pattern[7];

	for (i=0; i<7; i++) test_pattern[i] = (i<<12) + i;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 1, 1,
			MBOX_CMD_NOP,
			0,0,0,0,0,0,0);

	QL_MUTEX_EXIT(ctlr);

	TRACE (ctlr, "MBOXTEST: NOP test status: 0x%x, mbox_sts0 0x%x.\n",rc,mbox_sts[0],0,0,0,0);

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 8, 8,
			MBOX_CMD_MAILBOX_REGISTER_TEST,
			test_pattern[0],
			test_pattern[1],
			test_pattern[2],
			test_pattern[3],
			test_pattern[4],
			test_pattern[5],
			test_pattern[6]);

	QL_MUTEX_EXIT(ctlr);


	for (i=0; i<7; i++) {
		TRACE (ctlr,"MBOXTEST: REG test results: status: 0x%x, register %d: act 0x%x: exp 0x%x\n",
			rc,i,mbox_sts[i+1],test_pattern[i],0,0);
	}

	return rc;
}
#endif

/* FILE: misc.c */
static void
qlfc_enable_intrs(QLFC_CONTROLLER * ctlr)
{
	CONTROLLER_REGS * isp = (CONTROLLER_REGS *)ctlr->base_address;

	/*
	** Enable risc interrupts, and let the system see them.
	*/

	QL_PCI_OUTH(&isp->icr, (ICR_ENABLE_RISC | ICR_INTERRUPTS_ENABLED));
	DELAYBUS(50);
}

static void
qlfc_disable_intrs (QLFC_CONTROLLER *ctlr)
{
	CONTROLLER_REGS * isp = (CONTROLLER_REGS *)ctlr->base_address;

	/*
	** Disable all interrupts.
	*/

	QL_PCI_OUTH(&isp->icr, ICR_DISABLE_ALL_INTS);
	DELAYBUS(50);
}


static int
qlfc_extra_response_space ()
{
	int c=0;

	/*
	** Calculate max response size for mbox command
	*/

	c = max (c, sizeof(portDatabase_t));
	c = max (c, sizeof(positionMap_t));
	c = max (c, sizeof(sns_remove_all_rsp_t));
	c = max (c, sizeof(sns_register_port_type_rsp_t));
	c = max (c, sizeof(sns_register_fc4_types_rsp_t));
	c = max (c, sizeof(sns_register_cos_rsp_t));
	c = max (c, sizeof(sns_register_node_name_rsp_t));
	c = max (c, sizeof(sns_register_port_name_rsp_t));
	c = max (c, sizeof(sns_get_port_id4_rsp_t));
	c = max (c, sizeof(sns_get_port_id3_rsp_t));
	c = max (c, sizeof(sns_get_port_id2_rsp_t));
	c = max (c, sizeof(sns_get_port_id1_rsp_t));
	c = max (c, sizeof(sns_get_port_type_rsp_t));
	c = max (c, sizeof(sns_get_fc4_types_rsp_t));
	c = max (c, sizeof(sns_get_class_of_service_rsp_t));
	c = max (c, sizeof(sns_get_node_name_1_rsp_t));
	c = max (c, sizeof(sns_get_port_name_rsp_t));
	c = max (c, sizeof(sns_get_all_next_rsp_t));

	c += sizeof(uint64_t);
	c *= 2;

	return c;
}

static int
qlfc_extra_request_space ()
{
	int c=0;

	/*
	** Calculate max request size for mbox command
	*/

	c = max (c, sizeof(sns_get_all_next_cmd_t));
	c = max (c, sizeof(sns_get_port_name_cmd_t));
	c = max (c, sizeof(sns_get_node_name_1_cmd_t));
	c = max (c, sizeof(sns_get_class_of_service_cmd_t));
	c = max (c, sizeof(sns_get_fc4_types_cmd_t));
	c = max (c, sizeof(sns_get_port_type_cmd_t));
	c = max (c, sizeof(sns_get_port_id1_cmd_t));
	c = max (c, sizeof(sns_get_port_id2_cmd_t));
	c = max (c, sizeof(sns_get_port_id3_cmd_t));
	c = max (c, sizeof(sns_get_port_id4_cmd_t));
	c = max (c, sizeof(sns_regsiter_port_name_cmd_t));
	c = max (c, sizeof(sns_regsiter_node_name_cmd_t));
	c = max (c, sizeof(sns_regsiter_cos_cmd_t));
	c = max (c, sizeof(sns_regsiter_fc4_types_cmd_t));
	c = max (c, sizeof(sns_regsiter_port_type_cmd_t));
	c = max (c, sizeof(sns_remove_all_cmd_t));

	c += sizeof(uint64_t);
	c *= 2;

	return c;
}


static void
qlfc_swap64 (uint8_t *pointer, int count)
{
	/* munge 8 bytes at a time */
	/*
	** If not a multiple of 8 bytes,
	** the nearest lower multiple are
	** swapped.
	*/

	int	i;
	uint8_t * cpointer, *copyptr;
	uint8_t  temp[8];

	cpointer = copyptr = pointer;
	cpointer = pointer;
	copyptr = pointer;
	for (i = 0; i < count / 8; i++) {
		temp[7] = *copyptr++;
		temp[6] = *copyptr++;
		temp[5] = *copyptr++;
		temp[4] = *copyptr++;
		temp[3] = *copyptr++;
		temp[2] = *copyptr++;
		temp[1] = *copyptr++;
		temp[0] = *copyptr++;
		bcopy(&temp[0], cpointer, 8);
		cpointer = cpointer + 8;
	}
}


/*ARGSUSED*/
static void
fill_sg_elem(QLFC_CONTROLLER *ctlr, data_seg *dsp, alenaddr_t address, size_t length, int allow_prefetch)
{
	alenaddr_t new_address;

	new_address = (alenaddr_t)pciio_dmatrans_addr
	    ((dev_t)ctlr->pci_vhdl, NULL, address, length,
             PCIBR_VCHAN0	|
	     PCIBR_PREFETCH	|
             PCIIO_DMA_DATA	|
             PCIIO_DMA_A64	|
	     PCIIO_BYTE_STREAM);

#ifdef BRIDGE_B_PREFETCH_DATACORR_WAR
	if (!allow_prefetch)
		new_address &= ~PCI64_ATTR_PREF;
#endif

#if 0
	TRACE (ctlr,"DMATRANS: 0x%x -> 0x%x\n",(__psint_t)address,(__psint_t)new_address,0,0,0,0);
#endif
	dsp->base = new_address;
	dsp->count = length;
}

static void
qlfc_swap16 (uint8_t *pointer, int count)
{
	int i;
	uint8_t	c1;

	/* munge 2 bytes at a time */
	for (i=0; i<count; i+=sizeof(uint16_t)) {

		c1 = *(pointer+i);
		*(pointer+i) = *(pointer+i+1);
		*(pointer+i+1) = c1;
	}
}

/* REFERENCED */
static void
qlfc_swap32 (uint8_t *pointer, int count)
{
	/* munge 4 bytes at a time */
	int	i;
	uint8_t * cpointer, *copyptr;
	int	temp;

	cpointer = copyptr = pointer;
	cpointer = pointer;
	copyptr = pointer;
	for (i = 0; i < count / 4; i++) {
		temp = *((int *) copyptr);
		cpointer = (uint8_t * ) & temp;

		*(copyptr + 3) = *(cpointer + 0);
		*(copyptr + 2) = *(cpointer + 1);
		*(copyptr + 1) = *(cpointer + 2);
		*(copyptr + 0) = *(cpointer + 3);
		copyptr = copyptr + 4;
	}
	cpointer = pointer;
}



/********************************************************************************
*										*
*	qlfc_queue_sr								*
*										*
*	Queue a scsi_request structure on to the specified targets queue.	*
*										*
*	Entry:									*
*										*
*	Input:									*
*		ctlr - controller data structure				*
*		sr - scsi_request to be queued					*
*										*
*	Output:									*
*										*
*	Notes:									*
*		This routine should be modified to return OK/ERROR status	*
*		instead of panicing if cannot queue.				*
*										*
********************************************************************************/

static void
qlfc_queue_sr (QLFC_CONTROLLER *ctlr, scsi_request_t *sr)
{
	SR_SPARE		*spare, new_cookie, active_cookie, next_cookie;
	vertex_hdl_t		targ_vhdl;
	scsi_request_t		*active, *next;
	scsi_lun_info_t		*lun_info;
	qlfc_local_targ_info_t	*qti;

	spare = (SR_SPARE *)&sr->sr_spare;

	/*
	** Calculate the cookie value.
	*/

	lun_info = scsi_lun_info_get(sr->sr_lun_vhdl);

	targ_vhdl = STI_TARG_VHDL(SLI_TARG_INFO(lun_info));
	if (targ_vhdl > MAX_TARG_VHDL) {
		TRACE (ctlr, "PANIC   : targ_vhdl %d is too big: lun_info 0x%x, lun_vhdl 0x%x, sr 0x%x.\n",
			targ_vhdl,(__psint_t)lun_info,sr->sr_lun_vhdl,(__psint_t)sr,0,0);
		panic ("targ_vhdl too big!");
	}

	qti = STI_INFO(SLI_TARG_INFO(lun_info));

	IOLOCK (qti->target_mutex);

	new_cookie.field.cookie.field.target_vhdl = targ_vhdl;

	if (!qti->req_last) {	/* no requests on queue */
		new_cookie.field.cookie.field.value = MIN_COOKIE;
		spare->field.cookie.data = new_cookie.field.cookie.data;
		sr->sr_ha = NULL;
		qti->req_active = qti->req_last = sr;
	}

	else {
		active_cookie.field.cookie.data = ((SR_SPARE *) (&qti->req_last->sr_spare))->field.cookie.data;
		if (active_cookie.field.cookie.field.value < MAX_COOKIE) {
			new_cookie.field.cookie.field.value = active_cookie.field.cookie.field.value + 1;
			/*
			** This sr becomes the last sr.
			*/
			qti->req_last->sr_ha = sr;
			qti->req_last = sr;
			spare->field.cookie.data = new_cookie.field.cookie.data;
			sr->sr_ha = NULL;
		}

		else {	/* req_last is MAX_COOKIE */
			active_cookie.data = (__psint_t) qti->req_active->sr_spare;
			if (active_cookie.field.cookie.field.value > MIN_COOKIE) {
				new_cookie.field.cookie.field.value = MIN_COOKIE;
				sr->sr_ha = qti->req_active;
				qti->req_active = sr;
				spare->field.cookie.data = new_cookie.field.cookie.data;
			}

			else {		/* hope to find a slot in the middle! */
				active = qti->req_active;
				active_cookie.data = (__psint_t) active->sr_spare;
				while (next=active->sr_ha) {			/* while possibility of a hole */
					next_cookie.data = (__psint_t) next->sr_spare;
					if (active_cookie.field.cookie.field.value + 1 < next_cookie.field.cookie.field.value) {	/* found a hole */
						new_cookie.field.cookie.field.value = active_cookie.field.cookie.field.value + 1;
						sr->sr_ha = next;
						spare->field.cookie.data = new_cookie.field.cookie.data;
						active->sr_ha = sr;
						break;
					}
					active = next;
					active_cookie.data = next_cookie.data;
				}
				if (!next) {
					TRACE (ctlr, "PANIC   : cannot queue request: &qti->req_active 0x%x\n",
						(__psint_t)&qti->req_active,0,0,0,0,0);
					panic ("qlfc: Unable to queue scsi request.\n");
				}
			}
		}
	}
	IOUNLOCK (qti->target_mutex);
}

/********************************************************************************
*										*
*	qlfc_deque_sr								*
*										*
*	Remove a scsi request from the appropriate queue.			*
*										*
*	Entry:									*
*		Called during interrupt processing.				*
*										*
*	Input:									*
*		ctlr - pointer to the controller data structures		*
*		cookie_value - value returned in status  iocb (or via fast post)*
*										*
*	Output:									*
*		Returns a pointer to the dequeued scsi request (or NULL)	*
*										*
*	Notes:									*
*										*
********************************************************************************/

static scsi_request_t *
qlfc_deque_sr (QLFC_CONTROLLER *ctlr, int cookie_value)
{
	qlfc_local_targ_info_t	*qti;
	scsi_targ_info_t	*targ_info;
	scsi_request_t		*sr=NULL, *prev=NULL;
	vertex_hdl_t		targ_vhdl;
	SR_SPARE		sr_cookie;
	REQ_COOKIE		cookie;
	int			value;

	cookie.data = cookie_value;

	targ_vhdl = cookie.field.target_vhdl;
	value = cookie.field.value;

	targ_info = scsi_targ_info_get (targ_vhdl);
	if (targ_info) {
		qti = STI_INFO (targ_info);

		if (qti) {
			IOLOCK (qti->target_mutex);

			sr = qti->req_active;	/* first one */

			while (sr) {
				sr_cookie.data = (__psint_t) sr->sr_spare;
				if (sr_cookie.field.cookie.field.value == value) {
					if (!prev) qti->req_active = sr->sr_ha;
					else prev->sr_ha = sr->sr_ha;
					if (sr == qti->req_last) qti->req_last = prev;
					break;
				}
				prev = sr;
				sr = (scsi_request_t *) sr->sr_ha;
			}

			IOUNLOCK (qti->target_mutex);

			if (sr == NULL) {
				TRACE (ctlr, "deque failure: cookie 0x%x, targ_vhdl %d, value %d, &qti->req_active 0x%x\n",
					cookie.data, targ_vhdl, value, (__psint_t)&qti->req_active, 0,0);
			}
		}
		else {
			printf ("qlfc: possible spurious interrupt.  No local targ info (qti) for target vhdl %d\n",targ_vhdl);
		}
	}
	else {
		printf ("qlfc: possible spurious interrupt.  No target info for target vhdl %d\n",targ_vhdl);
	}
	return sr;
}

static void
assign_int16(uint16_t *ptr, uint16_t value)
{
	uint8_t *p = (uint8_t *)ptr;

	p[0] = (uint8_t) value;
	p[1] = (uint8_t) (value >> 8);
}

/* REFERENCED */
static void
assign_int32(uint32_t *ptr, uint32_t value)
{
	uint8_t *p = (uint8_t *)ptr;

	p[0] = (uint8_t) value;
	p[1] = (uint8_t) (value >> 8);
	p[2] = (uint8_t) (value >> 16);
	p[3] = (uint8_t) (value >> 24);
}

static void
assign_int64(uint64_t *ptr, uint64_t value)
{
	uint8_t *p = (uint8_t *)ptr;

	p[0] = (uint8_t) value;
	p[1] = (uint8_t) (value >> 8);
	p[2] = (uint8_t) (value >> 16);
	p[3] = (uint8_t) (value >> 24);
	p[4] = (uint8_t) (value >> 32);
	p[5] = (uint8_t) (value >> 40);
	p[6] = (uint8_t) (value >> 48);
	p[7] = (uint8_t) (value >> 56);
}
/* FILE: position.c */
STATUS
qlfc_get_position_map (QLFC_CONTROLLER *ctlr, positionMap_t *map)
{
	int		i;
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	uint8_t		*cmap;

	TRACE (ctlr, "GET_PMAP: entered.\n",0,0,0,0,0,0);

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_dma_rsp (ctlr, mbox_sts, 8, 1, MBOX_CMD_GET_POSITION_MAP, 0,
		(uint8_t *)map,sizeof(positionMap_t));


	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {

		if (mbox_sts[0] == 0x4000) {
			cmap = (uint8_t *)map;
			for (i=0; i<sizeof(positionMap_t); i+=8) {
				unsigned c0,c1, c2, c3, c4,c5,c6,c7;
				c0 = *(cmap+i+0);
				c1 = *(cmap+i+1);
				c2 = *(cmap+i+2);
				c3 = *(cmap+i+3);
				c4 = *(cmap+i+4);
				c5 = *(cmap+i+5);
				c6 = *(cmap+i+6);
				c7 = *(cmap+i+7);
				TRACE (ctlr,"GET_POSITION_MAP: %d-%d: %x %x %x %x\n",i,i+3,c0,c1,c2,c3);
				TRACE (ctlr,"GET_POSITION_MAP: %d-%d: %x %x %x %x\n",i+4,i+7,c4,c5,c6,c7);
				if (c0 == 0xff) break;
			}
		}
		else rc = ERROR;
	}

	TRACE (ctlr, "GET_PMAP: exit.\n",0,0,0,0,0,0);

	return rc;
}

/* FILE: nn_list.c */
STATUS
qlfc_get_node_name_list (QLFC_CONTROLLER *ctlr, port_node_name_list_t *nnl)
{
	int			i;
	STATUS			rc;
	uint16_t     		mbox_sts[MBOX_REGS];
	port_node_name_list_t	*rsp;

	TRACE (ctlr, "GET_NLST: enter.\n",0,0,0,0,0,0);

	rsp = (port_node_name_list_t *) kmem_zalloc (sizeof(port_node_name_list_t),
						     VM_CACHEALIGN | VM_DIRECT);

	if (!rsp) return ERROR;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_dma_rsp (ctlr, mbox_sts, 8, 2, MBOX_CMD_GET_PORT_NODE_NAME_LIST, 1,
		(uint8_t *)rsp,sizeof(port_node_name_list_t));

	TRACE (ctlr, "GET_NLST: rc %d, mbox0 0x%x\n",rc,mbox_sts[0],0,0,0,0);

#if 0
	printf ("GET_NLST: data bytes transferred: %d\n",mbox_sts[1]);
#endif
	if (rc == OK && mbox_sts[0] == 0x4000 && mbox_sts[1]) {
		for (i=0; i<PORT_NODE_NAME_LIST_ENTRIES; i++) {
			uint64_t	node_name;
			uint16_t	loop_id;
			int		j;

			assign_int16  (&rsp->entry[i].loop_id,
					rsp->entry[i].loop_id);
			for (j=0; j<4; j++) assign_int16 (&rsp->entry[i].node_name[j],
							   rsp->entry[i].node_name[j]);

			node_name =
				(uint64_t)rsp->entry[i].node_name[3]<<48 |
				(uint64_t)rsp->entry[i].node_name[2]<<32 |
				(uint64_t)rsp->entry[i].node_name[1]<<16 |
					  rsp->entry[i].node_name[0];

			loop_id = rsp->entry[i].loop_id;

			if (loop_id && node_name) {
				TRACE (ctlr, "GET_NLST: loop_id %d: 0x%x\n",loop_id,node_name,0,0,0,0);
			}
			else break;
		}
	}
	else rc = ERROR;


	QL_MUTEX_EXIT (ctlr);

	if (rc != ERROR && nnl) *nnl = *rsp;
	kmem_free (rsp,sizeof(port_node_name_list_t));

	return rc;
}
/* FILE: port_database.c */
/*REFERENCED*/
static STATUS
qlfc_get_port_database (QLFC_CONTROLLER *ctlr, int loop_id, portDatabase_t *db)
{
	int		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	portDatabase_t	*pd;

	pd = (portDatabase_t *) kmem_zalloc (sizeof(portDatabase_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!pd) return ERROR;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_dma_rsp (ctlr, mbox_sts, 8, 1, MBOX_CMD_GET_PORT_DATABASE, loop_id<<8,
		(uint8_t *)pd,sizeof(portDatabase_t));

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK && mbox_sts[0] == MBOX_STS_COMMAND_COMPLETE) {

		assign_int16 (&pd->pd_M_execution_throttle,pd->pd_M_execution_throttle);
		assign_int16 (&pd->pd_M_execution_count,pd->pd_M_execution_count);
		assign_int16 (&pd->pd_M_resource_allocation,pd->pd_M_resource_allocation);
		assign_int16 (&pd->pd_M_current_allocation,pd->pd_M_current_allocation);
		assign_int16 (&pd->pd_M_lun_abort_flags,pd->pd_M_lun_abort_flags);
		assign_int16 (&pd->pd_M_lun_stop_flags,pd->pd_M_lun_stop_flags);
		assign_int16 (&pd->pd_M_port_retry_timer,pd->pd_M_port_retry_timer);
		assign_int16 (&pd->pd_M_next_sequence_id,pd->pd_M_next_sequence_id);
		assign_int16 (&pd->pd_M_frame_count,pd->pd_M_frame_count);
		assign_int16 (&pd->pd_M_loop_id,pd->pd_M_loop_id);
		assign_int16 (&pd->pd_M_common_features,pd->pd_M_common_features);

		assign_int16 (&pd->pd_M_num_open_sequences,pd->pd_M_num_open_sequences);
		assign_int16 (&pd->pd_M_total_concurrent_sequences,pd->pd_M_total_concurrent_sequences);
		assign_int16 (&pd->pd_M_rel_offset_info_catagory_flags,pd->pd_M_rel_offset_info_catagory_flags);
		assign_int16 (&pd->pd_M_receive_data_size,pd->pd_M_receive_data_size);
		assign_int16 (&pd->pd_M_num_concurrent_sequences,pd->pd_M_num_concurrent_sequences);
		assign_int16 (&pd->pd_M_prli_payload_length,pd->pd_M_prli_payload_length);
		assign_int16 (&pd->pd_M_w0_prli_service_parameters,pd->pd_M_w0_prli_service_parameters);
		assign_int16 (&pd->pd_M_w3_prli_service_parameters,pd->pd_M_w3_prli_service_parameters);
		assign_int16 (&pd->pd_M_initiator_receipt_control_flags,pd->pd_M_initiator_receipt_control_flags);

		/* hard address gets special swapping.... */
#if 0
		{
		uint8_t		*c;
		c = (uint8_t *)&pd->pd_M_hard_address;
		assign_int16 ((uint16_t *)c,*c);
		c += 2;
		assign_int16 ((uint16_t *)c,*c);
		}
#endif

		if (qlfc_debug >= 99) {
#if 0
			printf ("GET_PORT_DB: lid %d (0x%x): pd_options:              0x%x\n", loop_id,loop_id,pd->pd_options);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_control:              0x%x\n", loop_id,loop_id,pd->pd_control);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_master_state:         0x%x\n", loop_id,loop_id,pd->pd_master_state);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_slave_state:          0x%x\n", loop_id,loop_id,pd->pd_slave_state);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_port_id:              0x%x\n", loop_id,loop_id,pd->pd_port_id);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_hard_address:       0x%x\n", loop_id,loop_id,pd->pd_M_hard_address);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_node_name:            0x%x\n", loop_id,loop_id,pd->pd_node_name);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_port_name:            0x%x\n", loop_id,loop_id,pd->pd_port_name);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_execution_throttle: 0x%x\n", loop_id,loop_id,pd->pd_M_execution_throttle);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_execution_count:    0x%x\n", loop_id,loop_id,pd->pd_M_execution_count);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_retry_count:          0x%x\n", loop_id,loop_id,pd->pd_retry_count);

			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_resource_allocation:0x%x\n", loop_id,loop_id,pd->pd_M_resource_allocation);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_current_allocation: 0x%x\n", loop_id,loop_id,pd->pd_M_current_allocation);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_common_features:    0x%x\n", loop_id,loop_id,pd->pd_M_common_features);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_receive_data_size:  0x%x\n", loop_id,loop_id,pd->pd_M_receive_data_size);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_num_open_sequences: 0x%x\n", loop_id,loop_id,pd->pd_M_num_open_sequences);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_lun_abort_flags:    0x%x\n", loop_id,loop_id,pd->pd_M_lun_abort_flags);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_lun_stop_flags:     0x%x\n", loop_id,loop_id,pd->pd_M_lun_stop_flags);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_port_retry_timer:   0x%x\n", loop_id,loop_id,pd->pd_M_port_retry_timer);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_next_sequence_id:   0x%x\n", loop_id,loop_id,pd->pd_M_next_sequence_id);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_frame_count:        0x%x\n", loop_id,loop_id,pd->pd_M_frame_count);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_prli_payload_length:0x%x\n", loop_id,loop_id,pd->pd_M_prli_payload_length);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_loop_id:            0x%x\n", loop_id,loop_id,pd->pd_M_loop_id);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_total_concurrent_sequences:     0x%x\n", loop_id,loop_id,pd->pd_M_total_concurrent_sequences);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_rel_offset_info_catagory_flags: 0x%x\n", loop_id,loop_id,pd->pd_M_rel_offset_info_catagory_flags);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_initiator_receipt_control_flags:0x%x\n", loop_id,loop_id,pd->pd_M_initiator_receipt_control_flags);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_num_concurrent_sequences:       0x%x\n", loop_id,loop_id,pd->pd_M_num_concurrent_sequences);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_w0_prli_service_parameters:     0x%x\n", loop_id,loop_id,pd->pd_M_w0_prli_service_parameters);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_w3_prli_service_parameters:     0x%x\n", loop_id,loop_id,pd->pd_M_w3_prli_service_parameters);

			printf ("\n");
#else
			printf ("GET_PORT_DB: lid %d (0x%x): pd_port_id:              0x%x\n", loop_id,loop_id,pd->pd_port_id);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_loop_id:            0x%x\n", loop_id,loop_id,pd->pd_M_loop_id);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_M_hard_address:       0x%x\n", loop_id,loop_id,pd->pd_M_hard_address);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_node_name:            0x%x\n", loop_id,loop_id,pd->pd_node_name);
			printf ("GET_PORT_DB: lid %d (0x%x): pd_port_name:            0x%x\n", loop_id,loop_id,pd->pd_port_name);
			printf ("\n");
#endif
		}
	}
	else rc = ERROR;

	if (rc != ERROR && db) *db = *pd;
	kmem_free (pd,sizeof(portDatabase_t));
	return rc;


}
/* FILE: port_name.c */
static STATUS
qlfc_get_node_name (QLFC_CONTROLLER *ctlr, unsigned loop_id, uint64_t *node_name, uint16_t *mbox_regs)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	uint64_t	name;
	uint16_t	*p16;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 2, 8,
			MBOX_CMD_GET_PORT_NAME,
			(loop_id << 8) | 1,
			0,
			0,
			0,
			0,
			0,
			0);

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {

		p16 = (uint16_t *)&name;

		assign_int16 (p16++,mbox_sts[2]);
		assign_int16 (p16++,mbox_sts[3]);
		assign_int16 (p16++,mbox_sts[6]);
		assign_int16 (p16,mbox_sts[7]);

		TRACE (ctlr, "NODENAME: loop id %d, nodename 0x%x\n",loop_id, name, 0,0,0,0);

		if (node_name) *node_name = name;
	}

	if (mbox_regs) {
		int i;
		for (i=0; i<MBOX_REGS; i++) *mbox_regs++ = mbox_sts[i];
	}

	return rc;
}

static STATUS
qlfc_get_port_name (QLFC_CONTROLLER *ctlr, unsigned loop_id, uint64_t *port_name, uint16_t *mbox_regs)
{
	STATUS		rc=ERROR;
	uint16_t     	mbox_sts[MBOX_REGS];
	uint64_t	name;
	uint16_t	*p16;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, &mbox_sts[0], 2, 8,
			MBOX_CMD_GET_PORT_NAME,
			loop_id << 8,
			0,
			0,
			0,
			0,
			0,
			0);

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {
		p16 = (uint16_t *)&name;

		assign_int16 (p16++,mbox_sts[2]);
		assign_int16 (p16++,mbox_sts[3]);
		assign_int16 (p16++,mbox_sts[6]);
		assign_int16 (p16,mbox_sts[7]);

		TRACE (ctlr, "PORTNAME: loop id %d, portname 0x%x\n",loop_id, name, 0,0,0,0);

		if (port_name) *port_name = name;
	}

	if (mbox_regs) {
		int i;
		for (i=0; i<MBOX_REGS; i++) *mbox_regs++ = mbox_sts[i];
	}

	return rc;
}
/* FILE: send_lfa.c */
STATUS
qlfc_send_lfa (QLFC_CONTROLLER *ctlr, lfa_cmd_hdr_t *lfa_command, lfa_rsp_hdr_t *lfa_response)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	int		lfa_command_length;
	int		lfa_response_length;

	lfa_command_length = (LFA_BASIC_HEADER_SZ / 2)
			   + lfa_command->lfa_subcommand_length;

	lfa_response_length = lfa_command->lfa_response_length;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_dma_cmd_rsp (ctlr, mbox_sts, 8, 8,
			MBOX_CMD_SEND_LFA,
			lfa_command_length,
			(uint8_t *)lfa_command,
			lfa_command_length*2 + ((lfa_command_length*2)%sizeof(uint64_t)),
			(uint8_t *)lfa_response,
			lfa_response_length*2 + ((lfa_response_length*2)%sizeof(uint64_t)));

	QL_MUTEX_EXIT(ctlr);

	TRACE (ctlr, "SEND_LFA: rc %d, resp: 0x%x\n",rc,lfa_response->lfa_response_code,0,0,0,0);

	if (lfa_response->lfa_response_code != 0x02000000) {	/* not accept */
		rc = ERROR;
	}

	return rc;
}

STATUS
qlfc_send_lfa_linit (QLFC_CONTROLLER *ctlr, uint32_t port_id, uint8_t lip4, uint8_t lip3, int init_func)
{
	STATUS			rc;
	int			response_alloc_length;
	lfa_linit_cmd_t		*lfa_cmd=NULL;
	lfa_linit_rsp_t		*lfa_rsp=NULL;

	TRACE (ctlr, "LINIT   : port id 0x%x, lip(%x,%x), login %d\n",port_id,lip4,lip3,init_func,0,0);

	lfa_cmd = (lfa_linit_cmd_t *) kmem_zalloc (sizeof(lfa_linit_cmd_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(lfa_linit_rsp_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	lfa_rsp = (lfa_linit_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!lfa_rsp || !lfa_cmd) {
		rc = ERROR;
	}
	else {
		lfa_cmd->linit_cmd_hdr.lfa_response_length = sizeof(lfa_linit_rsp_t) / 2;	/* NOT response_alloc_length */
		lfa_cmd->linit_cmd_hdr.lfa_response_buffer_addr_u = ctlr->response_misc_dmaptr >> 32;
		lfa_cmd->linit_cmd_hdr.lfa_response_buffer_addr_l = ctlr->response_misc_dmaptr & 0xffffffff;
		lfa_cmd->linit_cmd_hdr.lfa_subcommand = LFA_LINIT;
		lfa_cmd->linit_cmd_hdr.lfa_subcommand_length = 4;
		lfa_cmd->linit_cmd_hdr.lfa_loop_fabric_address = port_id;

		lfa_cmd->linit_init_func = init_func;
		lfa_cmd->linit_lip4 = lip4;
		lfa_cmd->linit_lip3 = lip3;

		rc = qlfc_send_lfa (ctlr, &lfa_cmd->linit_cmd_hdr, (lfa_rsp_hdr_t *)lfa_rsp);

		if (rc == OK) {
			if (lfa_rsp->linit_status != 1) rc = ERROR;
		}
	}

	if (lfa_cmd) kmem_free (lfa_cmd,sizeof(lfa_linit_cmd_t));
	if (lfa_rsp) kmem_free (lfa_rsp,response_alloc_length);

	if (rc == OK) delay (3*HZ);

	return rc;
}

/* FILE: send_sns.c */
STATUS
qlfc_send_sns (QLFC_CONTROLLER *ctlr, sns_cmd_hdr_t *sns_command, sns_rsp_cthdr_t *sns_response)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	int		sns_command_length;
	int		sns_response_length;

	sns_command_length = (SNS_BASIC_HEADER_SZ / 2)
			   + sns_command->sns_subcommand_length;

	sns_response_length = sns_command->sns_response_length;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_dma_cmd_rsp (ctlr, mbox_sts, 8, 8,
			MBOX_CMD_SEND_SNS,
			sns_command_length,
			(uint8_t *)sns_command,
			sns_command_length*2 + ((sns_command_length*2)%sizeof(uint64_t)),
			(uint8_t *)sns_response,
			sns_response_length*2 + ((sns_response_length*2)%sizeof(uint64_t)));

	QL_MUTEX_EXIT(ctlr);

	TRACE (ctlr, "SEND_SNS: rc %d, ct_resp: 0x%x\n",rc,sns_response->ct_response_code,0,0,0,0);

	if (sns_response->ct_response_code != 0x8002) {	/* not accept */
		rc = ERROR;
	}

	return rc;
}

STATUS
qlfc_send_sns_get_all_next (QLFC_CONTROLLER *ctlr, uint32_t port_id, sns_get_all_next_rsp_t *rsp)
{

	STATUS			rc;
	int			response_alloc_length;
	sns_get_all_next_cmd_t	*sns_cmd=NULL;
	sns_get_all_next_rsp_t	*sns_rsp=NULL;

	sns_cmd = (sns_get_all_next_cmd_t *) kmem_zalloc (sizeof(sns_get_all_next_cmd_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(sns_get_all_next_cmd_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	sns_rsp = (sns_get_all_next_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!sns_rsp || !sns_cmd) {
		rc = ERROR;
	}
	else {
		sns_cmd->gan_cmd_hdr.sns_response_length = sizeof(sns_get_all_next_rsp_t) / 2;	/* NOT response_alloc_length */
		sns_cmd->gan_cmd_hdr.sns_response_buffer_addr_u = ctlr->response_misc_dmaptr >> 32;
		sns_cmd->gan_cmd_hdr.sns_response_buffer_addr_l = ctlr->response_misc_dmaptr & 0xffffffff;
		sns_cmd->gan_cmd_hdr.sns_subcommand = SNS_GET_ALL_NEXT;
		sns_cmd->gan_cmd_hdr.sns_subcommand_length = 6;

		sns_cmd->gan_port_id = port_id;

		rc = qlfc_send_sns (ctlr, &sns_cmd->gan_cmd_hdr, (sns_rsp_cthdr_t *)sns_rsp);

		if (rc == OK) {
			assign_int64 (&sns_rsp->gan_port_name,sns_rsp->gan_port_name);
			assign_int64 (&sns_rsp->gan_node_name,sns_rsp->gan_node_name);
			if (rsp) *rsp = *sns_rsp;
		}
	}

	if (sns_cmd) kmem_free (sns_cmd,sizeof(sns_get_all_next_cmd_t));
	if (sns_rsp) kmem_free (sns_rsp,response_alloc_length);

	return rc;
}

STATUS
qlfc_send_get_node_name (QLFC_CONTROLLER *ctlr, uint32_t port_id, sns_get_node_name_1_rsp_t *rsp)
{

	STATUS				rc;
	int				response_alloc_length;
	sns_get_node_name_1_cmd_t	*sns_cmd=NULL;
	sns_get_node_name_1_rsp_t	*sns_rsp=NULL;

	sns_cmd = (sns_get_node_name_1_cmd_t *) kmem_zalloc (sizeof(sns_get_node_name_1_cmd_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(sns_get_node_name_1_rsp_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	sns_rsp = (sns_get_node_name_1_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!sns_cmd || !sns_rsp) {
		rc = ERROR;
	}
	else {
		sns_cmd->gnn1_cmd_hdr.sns_response_length = sizeof(sns_get_node_name_1_rsp_t) / 2;
		sns_cmd->gnn1_cmd_hdr.sns_response_buffer_addr_u = ctlr->response_misc_dmaptr >> 32;
		sns_cmd->gnn1_cmd_hdr.sns_response_buffer_addr_l = ctlr->response_misc_dmaptr & 0xffffffff;
		sns_cmd->gnn1_cmd_hdr.sns_subcommand = SNS_GET_NODE_NAME_1;
		sns_cmd->gnn1_cmd_hdr.sns_subcommand_length = 6;

		sns_cmd->gnn1_port_id = port_id;

		TRACE (ctlr, "GNN1    : port_id 0x%x\n",port_id,0,0,0,0,0);

		rc = qlfc_send_sns (ctlr, &sns_cmd->gnn1_cmd_hdr, (sns_rsp_cthdr_t *)sns_rsp);

		if (rc == OK) {
			if (rsp) *rsp = *sns_rsp;
			TRACE (ctlr,"GNN1    :OK, copyout.\n",0,0,0,0,0,0);
		}
	}

	if (sns_cmd) kmem_free (sns_cmd,sizeof(sns_get_node_name_1_cmd_t));
	if (sns_rsp) kmem_free (sns_rsp,response_alloc_length);

	return rc;
}

STATUS
qlfc_send_get_port_name (QLFC_CONTROLLER *ctlr, uint32_t port_id, sns_get_port_name_rsp_t *rsp)
{

	STATUS			rc;
	int			response_alloc_length;
	sns_get_port_name_cmd_t	*sns_cmd=NULL;
	sns_get_port_name_rsp_t	*sns_rsp=NULL;

	sns_cmd = (sns_get_port_name_cmd_t *) kmem_zalloc (sizeof(sns_get_port_name_cmd_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(sns_get_port_name_rsp_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	sns_rsp = (sns_get_port_name_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!sns_cmd || !sns_rsp) {
		rc = ERROR;
	}
	else {
		sns_cmd->gpn_cmd_hdr.sns_response_length = sizeof(sns_get_port_name_rsp_t) / 2;
		sns_cmd->gpn_cmd_hdr.sns_response_buffer_addr_u = ctlr->response_misc_dmaptr >> 32;
		sns_cmd->gpn_cmd_hdr.sns_response_buffer_addr_l = ctlr->response_misc_dmaptr & 0xffffffff;
		sns_cmd->gpn_cmd_hdr.sns_subcommand = SNS_GET_PORT_NAME;
		sns_cmd->gpn_cmd_hdr.sns_subcommand_length = 6;

		sns_cmd->gpn_port_id = port_id;

		TRACE (ctlr, "GPN     : port_id 0x%x\n",port_id,0,0,0,0,0);

		rc = qlfc_send_sns (ctlr, &sns_cmd->gpn_cmd_hdr, (sns_rsp_cthdr_t *)sns_rsp);

		if (rc == OK) {
			if (rsp) *rsp = *sns_rsp;
		}
	}

	if (sns_cmd) kmem_free (sns_cmd,sizeof(sns_get_port_name_cmd_t));
	if (sns_rsp) kmem_free (sns_rsp,response_alloc_length);

	return rc;
}

STATUS
qlfc_send_get_port_id3 (QLFC_CONTROLLER *ctlr, int fc4_protocol, sns_get_port_id3_rsp_t *rsp)
{

	STATUS			rc;
	int			response_alloc_length;
	sns_get_port_id3_cmd_t	*sns_cmd=NULL;
	sns_get_port_id3_rsp_t	*sns_rsp=NULL;

	sns_cmd = (sns_get_port_id3_cmd_t *) kmem_zalloc (sizeof(sns_get_port_id3_cmd_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(sns_get_port_id3_rsp_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	sns_rsp = (sns_get_port_id3_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (!sns_cmd || !sns_rsp) {
		rc = ERROR;
	}
	else {
		sns_cmd->gpid3_cmd_hdr.sns_response_length = sizeof(sns_get_port_id3_rsp_t) / 2;
		sns_cmd->gpid3_cmd_hdr.sns_response_buffer_addr_u = ctlr->response_misc_dmaptr >> 32;
		sns_cmd->gpid3_cmd_hdr.sns_response_buffer_addr_l = ctlr->response_misc_dmaptr & 0xffffffff;
		sns_cmd->gpid3_cmd_hdr.sns_subcommand = SNS_GET_PORT_ID_3;
		sns_cmd->gpid3_cmd_hdr.sns_subcommand_length = 6;
		sns_cmd->gpid3_cmd_hdr.sns_response_limit = GPID3_MAX_PORTS;

		sns_cmd->gpid3_fc4_protocol = fc4_protocol;

		rc = qlfc_send_sns (ctlr, &sns_cmd->gpid3_cmd_hdr, (sns_rsp_cthdr_t *)sns_rsp);

		if (rsp) {
			int i;
			if (rc == OK) {
				for (i=0; i<GPID3_MAX_PORTS; i++) {
					uint32_t *port, last=0;
					port = (uint32_t *)&sns_rsp->gpid3_port[i].gpid3_control_byte;

					TRACE (ctlr,"SNSGPID3: port %d: id  : 0x%x \n", i,*port,0,0,0,0);

					last = *port & 0xff000000;

					/*
					** this is a HACK....
					*/
					if (last && *(port+1) != 0xeeeeeeee) {	/* more */
						last = *(port+1);
						*(port+1) = *(port);
						*port = last;
						last = *port & 0xff000000;
					}

					if (*port == 0 || last == 0x80000000) break;
				}
			}
			*rsp = *sns_rsp;
		}
	}

	if (sns_cmd) kmem_free (sns_cmd,sizeof(sns_get_port_id3_cmd_t));
	if (sns_rsp) kmem_free (sns_rsp,response_alloc_length);

	return rc;
}

STATUS
qlfc_send_get_port_id_node_name (QLFC_CONTROLLER *ctlr, sns_get_port_id_node_name_rsp_t *rsp)
{
	STATUS				status;
	int				response_alloc_length;
	uint32_t			i,j;
	uint32_t			*port, port_id;
	sns_get_port_id3_rsp_t		*gpid3_rsp;
	sns_get_node_name_1_rsp_t	*gnn_rsp;

	if (!rsp) return ERROR;

	gpid3_rsp = (sns_get_port_id3_rsp_t *) kmem_zalloc (sizeof(sns_get_port_id3_rsp_t),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	response_alloc_length = sizeof(sns_get_node_name_1_rsp_t);
	response_alloc_length += response_alloc_length % sizeof(uint64_t);

	gnn_rsp = (sns_get_node_name_1_rsp_t *) kmem_zalloc (response_alloc_length,
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	/*
	** Currently, this command is emulated as the fabric
	** switch I'm using doesn't support the command.
	**
	**	Results are what counts, anyway!
	*/

	if (gpid3_rsp && gnn_rsp) {


		status = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);

		TRACE (ctlr, "GPIDNN  : enter, rsp 0x%x\n",(__psint_t)rsp,0,0,0,0,0);

		if (status == OK) {
			status = qlfc_send_get_port_id3 (ctlr, 8, gpid3_rsp);	/* SNS command */
		}
		if (status == OK) {
			for (i=0,j=0; i<GPID3_MAX_PORTS; i++) {
				port = (uint32_t *)&gpid3_rsp->gpid3_port[i].gpid3_control_byte;
				port_id = *port & 0xffffff;

				if (*port == 0) break;

				status = qlfc_send_get_node_name (ctlr, port_id, gnn_rsp);	/* SNS command */
				if (status == OK) {
					rsp->gpidnn_port[j].gpidnn_node_name    = gnn_rsp->gnn1_node_name;
					rsp->gpidnn_port[j].gpidnn_control_byte = gpid3_rsp->gpid3_port[i].gpid3_control_byte;
					rsp->gpidnn_port[j].gpidnn_port_id[0]   = gpid3_rsp->gpid3_port[i].gpid3_port_id[0];
					rsp->gpidnn_port[j].gpidnn_port_id[1]   = gpid3_rsp->gpid3_port[i].gpid3_port_id[1];
					rsp->gpidnn_port[j].gpidnn_port_id[2]   = gpid3_rsp->gpid3_port[i].gpid3_port_id[2];
					rsp->gpidnn_port[j].gpiddnn_reserved    = 0;
					j++;
				}
				else break;

				if (gpid3_rsp->gpid3_port[i].gpid3_control_byte) break;
			}
		}
	}

	if (gpid3_rsp) kmem_free (gpid3_rsp, sizeof(sns_get_port_id3_rsp_t));
	if (gnn_rsp)   kmem_free (gnn_rsp, response_alloc_length);

	TRACE (ctlr, "GPIDNN  : exit, rsp 0x%x, status %d\n",(__psint_t)rsp,status,0,0,0,0);

	return status;
}
/* FILE: demon.c */
static void qlfc_demon_error_recovery (QLFC_CONTROLLER *ctlr, __psunsigned_t arg);

static
void
qlfc_demon_take_lock (QLFC_CONTROLLER *ctlr)
{
	IOLOCK (ctlr->demon_lock);
}

static
void
qlfc_demon_free_lock (QLFC_CONTROLLER *ctlr)
{
	IOUNLOCK (ctlr->demon_lock);
}

static
STATUS
qlfc_demon_msg_enqueue_with_lock (QLFC_CONTROLLER *ctlr, int msg, __psunsigned_t arg)
{
	STATUS	status=OK;

	if (ctlr->demon_flags & (DEMON_TIMEOUT|DEMON_QUEUE_FULL)) {
		status = ERROR;
	}
	else {
		ctlr->demon_msg_count ++ ;
		ctlr->demon_msg_queue[ctlr->demon_msg_in  ].msg = msg;
		ctlr->demon_msg_queue[ctlr->demon_msg_in++].arg = arg;
		if (ctlr->demon_msg_in == DEMON_MAX_EVENTS)
			ctlr->demon_msg_in = 0;
		if (ctlr->demon_msg_in == ctlr->demon_msg_out)
			ctlr->demon_flags |= DEMON_QUEUE_FULL;
		vsema (&ctlr->demon_sema);
	}

	return status;
}

static
STATUS
qlfc_demon_msg_enqueue (QLFC_CONTROLLER *ctlr, int msg, __psunsigned_t arg)
{
	STATUS	status=OK;

	qlfc_demon_take_lock (ctlr);

	status = qlfc_demon_msg_enqueue_with_lock (ctlr,msg,arg);

	qlfc_demon_free_lock (ctlr);

	return status;
}

static
__psunsigned_t
qlfc_demon_msg_dequeue_arg (QLFC_CONTROLLER *ctlr)
{
	__psunsigned_t	arg;

	if (!ctlr->demon_msg_count) arg = DEMON_NO_MSG;
	else {

		arg = ctlr->demon_msg_queue[ctlr->demon_msg_out++].arg;
		if (ctlr->demon_msg_out == DEMON_MAX_EVENTS)
			ctlr->demon_msg_out = 0;
		ctlr->demon_flags &= ~DEMON_QUEUE_FULL;
		ctlr->demon_msg_count -- ;

	}
	qlfc_demon_free_lock (ctlr);
	return arg;
}

static
int
qlfc_demon_msg_dequeue_msg (QLFC_CONTROLLER *ctlr)
{
	int		msg;

	qlfc_demon_take_lock (ctlr);

	if (!ctlr->demon_msg_count) msg = DEMON_NO_MSG;
	else {
		msg = ctlr->demon_msg_queue[ctlr->demon_msg_out].msg;
	}
	return msg;
}

static void
qlfc_demon_notify_scn (QLFC_CONTROLLER *ctlr, int event, uint16_t mailbox1, uint16_t mailbox2)
{
	qlfc_demon_msg_enqueue_with_lock (ctlr, event, ((mailbox1<<16) | mailbox2));
}

static void
qlfc_demon_notify_pdbc (QLFC_CONTROLLER *ctlr, int event)
{

	qlfc_demon_take_lock (ctlr);

	ctlr->demon_pdbc_count ++;

	if (!(ctlr->flags & CFLG_PDBC_ACTIVE)) {	/* not being serviced */

		atomicSetInt (&ctlr->flags,CFLG_PDBC);	/* stops i/o */
		qlfc_demon_msg_enqueue_with_lock (ctlr, event, 0);

	}

	qlfc_demon_free_lock (ctlr);
}

static void
qlfc_demon_target_set_awol_pending (QLFC_CONTROLLER *ctlr, __psunsigned_t loop_id)
{
	mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
	qlfc_target_set_awol_pending (ctlr, loop_id);	/* unlocks mutex */

        if (!(ctlr->flags & CFLG_DRAIN_IO))
		qlfc_start_scsi (ctlr);
}

static void
qlfc_demon_set_awol_pending (QLFC_CONTROLLER *ctlr)
{
	int i;
	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		qlfc_demon_target_set_awol_pending (ctlr, (__psunsigned_t)i);
	}
}

/* ARGSUSED */
static
void
qlfc_demon_process_link_up (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	if ((ctlr->flags & (CFLG_LINK_MODE|CFLG_LOOP_UP)) != (CFLG_LINK_MODE|CFLG_LOOP_UP)) {

		atomicSetInt (&ctlr->flags, CFLG_LINK_MODE);
		atomicClearInt (&ctlr->flags, CFLG_LIP_RESET);

		qlfc_logout_fabric_ports (ctlr);	/* implicit logout due to lip, clears logged in bit */
	}
}

/* ARGSUSED */
static
void
qlfc_demon_process_loop_up (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
}

/* ARGSUSED */
static
void
qlfc_demon_process_loop_down (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	atomicClearInt (&ctlr->flags, CFLG_LOOP_UP);
	qlfc_demon_set_awol_pending (ctlr);
}

/* ARGSUSED */
static
void
qlfc_demon_process_lip_complete (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	atomicSetInt (&ctlr->flags, CFLG_LIP);
	atomicClearInt (&ctlr->flags, CFLG_LIP_RESET);
}

/* ARGSUSED */
static
void
qlfc_demon_process_lip_reset (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	/*
	** LIP RESET processing.
	**	Stop all queues.
	**	Logout fabric devices.
	**
	** AL queues can be restarted when the LIP completion occurs.
	** Fabric queues can be restarted only after processing SCN/PDBC
	** messages.  However, in general, shouldn't start anything until
	** SCN/PDBC processing occurs.  For AL devices (no fabric), we
	** still see the PDBC if the enable pdbc bit is set in init cb.
	*/

	atomicAddInt (&ctlr->lip_resets, 1);

	if (ctlr->lip_resets >= 5000) {
		/*
		** Shut down interrupts.
		*/
		TRACE (ctlr, "DMN_LIPRS: solid lip reset.  Shutting down.\n",0,0,0,0,0,0);
		QL_MUTEX_ENTER(ctlr);
		qlfc_disable_intrs (ctlr);
		QL_MUTEX_EXIT(ctlr);
		atomicClearInt (&ctlr->lip_resets, 0xffffffff);	/* zero */
		atomicSetInt (&ctlr->flags, CFLG_SOLID_LIP);
	}

	else if (ctlr->lip_resets == 1) {	/* don't need to repeat ourselves here.... */

		qlfc_demon_take_lock (ctlr);
		atomicSetInt (&ctlr->flags, CFLG_STOP_PROBE | CFLG_LIP_RESET);
		qlfc_demon_free_lock (ctlr);

		if (!(ctlr->flags & CFLG_ISP_PANIC)) {
			qlfc_stop_queues (ctlr);		/* stop issuing */
			qlfc_logout_fabric_ports (ctlr);	/* implicit logout due to lip, clears logged in bit */
		}

		/*
		** Note: CFLG_STOP_PROBE remains set!
		*/
	}
}

#if 0
static
int
qlfc_demon_count_queued_requests (QLFC_CONTROLLER *ctlr, qlfc_local_targ_info_t *qti)
{
	int i=0;
	qlfc_local_lun_info_t   *qli=qti->local_lun_info;

	while (qli) {

		if (qli->qli_iwaitf) {
			if (!qli->qli_cmd_iwcnt) {
				TRACE (ctlr,"iwaitf 0x%x, iwcnt 0\n",(__psint_t)qli->qli_iwaitf,0,0,0,0,0);
				panic ("iwaitf and not iwcnt\n");
			}
			i += qli->qli_cmd_iwcnt;
		}


		qli = qli->next;
	}

	return i;
}
#endif
	
/* REFERENCED */
static STATUS
qlfc_demon_lip_reset_target (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	qlfc_local_targ_info_t	*qti = (qlfc_local_targ_info_t *) arg;
    	uint16_t	mbox_sts[MBOX_REGS];
	STATUS		rc;
	int		loop_id = qti->target;

	qlfc_tpmsg(ctlr->ctlr_vhdl, loop_id, "Performing recovery, lip reset.\n");

	QL_MUTEX_ENTER(ctlr);
	rc = qlfc_mbox_cmd(ctlr, mbox_sts, 4, 1, MBOX_CMD_INITIATE_LIP_RESET,
			 loop_id << 8, MBOX_DELAY_TIME, 0, 0, 0, 0, 0);
	QL_MUTEX_EXIT(ctlr);
	return rc;
}

static void
qlfc_demon_abort_lun (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	uint16_t		mailbox1, mailbox2;
	uint16_t		mbox_sts[MBOX_REGS];
	scsi_lun_info_t		*lun_info;
	scsi_targ_info_t	*targ_info;

	lun_info = (scsi_lun_info_t *)arg;
	targ_info = SLI_TARG_INFO(lun_info);

	if (ctlr->flags & CFLG_EXTENDED_LUN) {
		mailbox1 = (uint16_t) ((STI_TARG(targ_info)) << 8);
		mailbox2 = (uint16_t) (SLI_LUN(lun_info));
	}
	else {
		mailbox1 = (uint16_t) ((STI_TARG(targ_info)) << 8) 
			 | (uint16_t) (SLI_LUN(lun_info));
		mailbox2 = 0;
	}

	QL_MUTEX_ENTER(ctlr);

	if (qlfc_mbox_cmd(ctlr, mbox_sts, 3, 2,
			MBOX_CMD_ABORT_DEVICE,
			mailbox1,
			mailbox2,
			0, 0, 0, 0, 0))
	{
		qlfc_lpmsg(ctlr->ctlr_vhdl, STI_TARG(targ_info), SLI_LUN(lun_info),
			 "MBOX_CMD_ABORT_DEVICE Command failed\n");
	}

	QL_MUTEX_EXIT(ctlr);
}

/* ARGSUSED */
static STATUS
qlfc_demon_lip_with_login (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
    	uint16_t	mbox_sts[MBOX_REGS];
	STATUS		rc;

	if (ctlr->flags & CFLG_LINK_MODE) {
		/* no point in lip if in link mode */
		qlfc_demon_error_recovery (ctlr, arg);	/* go to the next step */
	}
	else {
		qlfc_cpmsg(ctlr->ctlr_vhdl, "Performing recovery, lip with login.\n");
		QL_MUTEX_ENTER(ctlr);
		rc = qlfc_mbox_cmd(ctlr, mbox_sts, 2, 2, MBOX_CMD_LIP_FOLLOWED_BY_LOGIN,
				 0, 0, 0, 0, 0, 0, 0);
		QL_MUTEX_EXIT(ctlr);
	}
	return rc;
}

/* REFERENCED */
/* ARGSUSED */
static STATUS
qlfc_demon_reset_targets (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	uint16_t	mbox_sts[MBOX_REGS];
	STATUS		rc;

	qlfc_cpmsg(ctlr->ctlr_vhdl, "Performing recovery, reset all targets.\n");

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 4, 1, MBOX_CMD_RESET, 4,
				0,
				0,
				0,
				0,0,0);
	atomicSetInt (&ctlr->flags, CFLG_SEND_MARKER);

	QL_MUTEX_EXIT (ctlr);

	return rc;
}

static
STATUS
qlfc_demon_reset_target (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	uint16_t	mbox_sts[MBOX_REGS];
	STATUS		rc;
	int		loop_id;

	qlfc_local_targ_info_t	*qti = (qlfc_local_targ_info_t *) arg;
	loop_id = qti->target;

	qlfc_tpmsg(ctlr->ctlr_vhdl, loop_id, "Performing recovery, target reset.\n", loop_id);

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 3, 1, MBOX_CMD_SEND_TARGET_RESET, loop_id << 8,
				MBOX_DELAY_TIME,	/* delay in seconds */
				0,
				0,
				0,0,0);
	QL_MUTEX_EXIT (ctlr);

	return rc;
}

static
STATUS
qlfc_demon_bailout (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	int			loop_id;
	qlfc_local_targ_info_t	*qti = (qlfc_local_targ_info_t *)arg;

	qlfc_cpmsg(ctlr->ctlr_vhdl, "Performing recovery, bailout.\n");
	QL_MUTEX_ENTER(ctlr);
	qlfc_load_firmware (ctlr);		/* need start over completely! */
	atomicClearInt (&ctlr->flags, CFLG_ISP_PANIC);
	qlfc_reset_interface(ctlr,1);
	qlfc_flush_queue(ctlr, SC_CMDTIME, ALL_QUEUES);	/* We want to retry these commands */
	if (qti) atomicClearInt (&qti->recovery_step,0xffffffff);
	else {
		for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {
			qti = ctlr->target_map[loop_id].tm_qti;
			if (qti && qti->req_count) {
				atomicClearInt (&qti->recovery_step,0xffffffff);
			}
		}
	}
	QL_MUTEX_EXIT(ctlr);
	return OK;
}

static
STATUS
qlfc_demon_post_abort_device (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	int		loop_id;

	qlfc_local_targ_info_t	*qti = (qlfc_local_targ_info_t *) arg;
	loop_id = qti->target;

	TRACE (ctlr, "ABRT TSK: post processing for target %d\n",loop_id,0,0,0,0,0);
	qlfc_tpmsg(ctlr->ctlr_vhdl, loop_id, "Performing recovery, post abort task set.\n", loop_id);

	QL_MUTEX_ENTER(ctlr);
	mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
	qlfc_flush_queue(ctlr, SC_CMDTIME, loop_id);	/* We want to retry these commands */
	mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
	atomicClearInt (&qti->recovery_step,0xffffffff);
	QL_MUTEX_EXIT(ctlr);
	atomicClearInt (&ctlr->flags, CFLG_BAD_STATUS);
	return OK;
}

static
STATUS
qlfc_demon_abort_device (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	uint8_t		lun_active [QL_MAXLUNS];
	uint16_t	mbox_sts[MBOX_REGS];
	scsi_request_t	*request;
	STATUS		rc;
	int		i;
	int		loop_id;

	qlfc_local_targ_info_t	*qti;

	qti = (qlfc_local_targ_info_t *) arg;

	loop_id = qti->target;
	TRACE (ctlr, "ABRT TSK: aborting task set(s) for target %d\n",loop_id,0,0,0,0,0);
	qlfc_tpmsg(ctlr->ctlr_vhdl, loop_id, "Performing recovery, abort task set.\n", loop_id);

	for (i=0; i<QL_MAXLUNS; i++) lun_active[i] = 0;

	IOLOCK (qti->target_mutex);
	request = qti->req_active;
	while (request) {
		qlfc_local_lun_info_t *qli;
		qli = qti->local_lun_info;
		if (request->sr_lun < QL_MAXLUNS) {
			lun_active[request->sr_lun] = 1;
			while (qli) {
				if (qli->qli_lun == request->sr_lun) {
					atomicSetInt (&qli->qli_dev_flags, DFLG_SEND_MARKER);
					break;
				}
				qli = qli->next;
			}
		}
		request = request->sr_ha;	/* next one */
	}
	IOUNLOCK (qti->target_mutex);

	for (i=0; i<QL_MAXLUNS; i++) {
		if (lun_active[i]) {
			QL_MUTEX_ENTER(ctlr);

			rc = qlfc_mbox_cmd (ctlr, mbox_sts, 3, 1, MBOX_CMD_ABORT_TASK_SET, loop_id << 8 | (i&0xff),
						i, 0, 0, 0,0,0);
			QL_MUTEX_EXIT (ctlr);

			if (rc != OK) {
				TRACE (ctlr, "ABRT TSK: Unable to exec mailbox command.  Stopping.\n",0,0,0,0,0,0);
				if (qlfc_debug >= 99) {
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Unable to execute mailbox command to abort "
								    "the task set for target %d, lun %d\n",
							loop_id, i);
				}
				break;
			}
		}
	}

	return rc;
}

/* ARGSUSED */
static void
qlfc_demon_send_linit (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{

	int			loop_id, port;
	uint32_t		port_id;
	uint32_t		port_id_array [QL_MAX_PORTS];
	qlfc_local_targ_info_t	*qti;


	/*
	** Send an linit to each port of the fabric connected to
	** this controller which has stuck targets.
	**
	** Port id is generated by clearing the lower byte of the
	** port_id field, i.e., the alpa, retaining domain and area.
	**
	*/

	bzero (port_id_array, sizeof(port_id_array));

	for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {

		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti && qti->req_count) {
			port_id = ctlr->target_map[loop_id].tm_port_id;
			if (port_id & 0xff) {	/* alpa implies loop */
				port_id = port_id & 0xffff00;	/* clear alpa */
				/*
				** Fill the port_id_array with the port_id area/domain of stuck targets.
				** If port_id already in array, don't entry again.
				*/
				for (port=0; port < QL_MAX_PORTS; port++) {
					if (port_id_array[port] == 0) {
						port_id_array[port] = port_id;
						break;
					}
					else if (port_id_array[port] == port_id) break;
				}
			}
		}
	}

	/*
	** Send the linit for each entry in the port_id_array.
	*/

	for (port=0; port<QL_MAX_PORTS; port++) {
		if (port_id_array[port] == 0) break;
		qlfc_send_lfa_linit (ctlr, port_id_array[port], 0xf7, 0xf7, LINIT_FORCE_FLOGI);
	}
}

static void
qlfc_demon_step_error_recovery (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	STATUS			status;
	int			ctlr_step, target_step;
	int			loop_id;
	qlfc_local_targ_info_t	*qti;

	ctlr_step = ctlr->recovery_step;

	TRACE (ctlr, "STEP_REC: recovery step %d\n",ctlr_step,0,0,0,0,0);

	if (qlfc_debug >= 99) {
		qlfc_cpmsg (ctlr->ctlr_vhdl, "recovery step %d\n",ctlr_step);
	}

	switch (ctlr_step) {	/* note, step has been incremented by caller, it's one greater than step index into table */
		case 1:		/* locate targets in trouble and try abort device */
		case 2:		/* perform post abort device processing */
			if (!(ctlr->flags & CFLG_BAD_STATUS)) {
				qlfc_demon_error_recovery (ctlr,arg);	/* skip this step */
				break;
			}
		case 3:		/* try target reset */
			/*
			** If target has i/o outstanding, try to abort the device.
			** This may fall through if CFLG_BAD_STATUS not set.
			*/
			ctlr->drain_timeout = 10;
			for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {
				qti = ctlr->target_map[loop_id].tm_qti;
				if (qti && qti->req_count) {
					atomicAddInt (&qti->recovery_step,1);
					target_step = qti->recovery_step;
					if (target_step < 2) {
						if (ctlr_step == 1) {
							qlfc_demon_abort_device (ctlr, (__psunsigned_t) qti);
						}
						else if (ctlr_step == 2) {
							qlfc_demon_post_abort_device (ctlr, (__psunsigned_t) qti);
						}
						else if (ctlr_step == 3) {
							qlfc_demon_reset_target (ctlr, (__psunsigned_t) qti);
						}
					}
					else {
						qlfc_demon_error_recovery (ctlr,arg);	/* target resets aren't working */
						break;
					}
				}
			}
			break;
		case 4:		/* start of topology specific recovery: lip reset target / linit */
		case 5:
			/*
			** Detect if this is a fabric by querying for nameserver.
			** Presumption, either fabric or loop, not both.
			*/
			ctlr->drain_timeout = 10;
			status = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);
			if (status == OK) {	/* fabric */
				/* try linit */
				TRACE (ctlr, "STEP_REC: trying LINIT.\n",0,0,0,0,0,0);
				qlfc_demon_send_linit (ctlr, 0);
			}
			else {
				/* try lip followed by login */
				TRACE (ctlr, "STEP_REC: trying LIP/LOGIN.\n",0,0,0,0,0,0);
				qlfc_demon_lip_with_login (ctlr, 0);
			}
			break;
		default:	/* bailout */
			atomicClearInt (&ctlr->recovery_step, 0xffffffff);
			TRACE (ctlr, "STEP_REC: bailout.\n",0,0,0,0,0,0);
			qlfc_demon_bailout (ctlr, 0);
			break;
	}
}

static
void
qlfc_demon_error_recovery (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{

	atomicAddInt (&ctlr->recovery_step, 1);
	qlfc_demon_step_error_recovery (ctlr,arg);
	atomicClearInt (&ctlr->flags, CFLG_STEP_RECOVERY_ACTIVE);

	if (!(ctlr->flags & CFLG_DRAIN_IO)) {
		qlfc_start_scsi(ctlr);
	}
}

static
void
qlfc_demon_target_awol_recovery_fabric (QLFC_CONTROLLER *ctlr, __psunsigned_t loop_id)
{
	int				i;
	int				event_count;
	int				found_it;
	uint32_t			*port, port_id, last;
	STATUS				status;
	sns_get_port_id_node_name_rsp_t	*gpidnn_rsp;
	qlfc_local_targ_info_t		*qti;

	/*
	** CFLG_SCN_ACTIVE is not set.
	** This allows process_scn to see any
	** state change notifications.
	*/

	atomicSetInt (&ctlr->flags, CFLG_SCN|CFLG_STOP_PROBE);	/* STOP not cleared until we're done here */

	qlfc_demon_free_lock (ctlr);

	gpidnn_rsp = (sns_get_port_id_node_name_rsp_t *) kmem_zalloc (sizeof(sns_get_port_id_node_name_rsp_t),
				VM_CACHEALIGN | VM_DIRECT);

	while (ctlr->flags & CFLG_PROBE_ACTIVE) delay (HZ);

	TRACE (ctlr, "DEM_AWOL: set STOP_PROBE, PROBE_ACTIVE is clear\n",0,0,0,0,0,0);

	event_count = ctlr->demon_scn_count;	/* where we started */

start_over:

	status = qlfc_send_get_port_id_node_name (ctlr, gpidnn_rsp);

	TRACE (ctlr,"DEM_AWOL: get pidnn status %d\n",status,0,0,0,0,0);
	if (status == OK) {

		if (!(ctlr->target_map[loop_id].tm_flags & TM_RESERVED)) {
			mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
			qti = ctlr->target_map[loop_id].tm_qti;
			found_it = 0;

			if (qti && ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE) {

				last = 0;
				for (i=0; i<GPIDNN_MAX_PORTS && !last; i++) {
					port = (uint32_t *)&gpidnn_rsp->gpidnn_port[i].gpidnn_control_byte;
					port_id = *port & 0xffffff;
					last = gpidnn_rsp->gpidnn_port[i].gpidnn_control_byte;
					if (*port == 0) break;

					if (ctlr->target_map[loop_id].tm_node_name
					 == gpidnn_rsp->gpidnn_port[i].gpidnn_node_name) {

						found_it = 1;
						ctlr->target_map[loop_id].tm_port_id = port_id;	/* might have moved */
						break;
					}
				}
				if (!found_it) {
					if (!qti->awol_retries) {	/* didn't find it */
						qlfc_target_set_awol (ctlr, loop_id);
					}
					else {
						qti->awol_retries --;
						qti->awol_timeout = AWOL_TIMEOUT;
						TRACE (ctlr, "awol rec: again: target %d, setting awol_timeout to %d\n",
							qti->target,qti->awol_timeout,0,0,0,0);
					}
				}
				else {
					if (!(ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN)) {
						mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);

						if (qti->awol_retries == 0 && (port_id & 0xff)) { /* last attempt, try linit */
							qlfc_tpmsg (ctlr->ctlr_vhdl, qti->target, "trying linit.\n");
							status = qlfc_send_lfa_linit (ctlr, 
										      port_id&0xffff00, 
										      0xf7, 0xf7, LINIT_FORCE_FLOGI);
						}
						status = qlfc_login_fabric_port (ctlr,loop_id,port_id,NULL);
						mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
						qti = ctlr->target_map[loop_id].tm_qti;
						if (qti) {
							if (status == OK) {
								if (ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN) {
									qlfc_start_target_queue (ctlr,loop_id);	/* clears awol */
								}
							}
							else {
								if (event_count != ctlr->demon_scn_count) {
									event_count = ctlr->demon_scn_count;
									mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
									goto start_over;
								}
								else if (!qti->awol_retries) {	/* didn't find it */
									qlfc_target_set_awol (ctlr, loop_id);
								}
								else {
									qti->awol_retries --;
									qti->awol_timeout = AWOL_TIMEOUT;
									TRACE (ctlr, "awol rec: again: target %d, setting awol_timeout to %d\n",
										qti->target,qti->awol_timeout,0,0,0,0);
								}
							}
						}
					}
				}
			}
			mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		}
	}
	else {
		if (ctlr->ql_awol_count && !(ctlr->target_map[loop_id].tm_flags & TM_RESERVED)) {
			mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
			qti = ctlr->target_map[loop_id].tm_qti;
			if (qti) {
				if (!qti->awol_retries) {	/* didn't find it */
					qti->awol_timeout = AWOL_TIMEOUT;	/* try again later */
					TRACE (ctlr, "awol rec: target %d, setting awol_timeout to %d\n",qti->target,qti->awol_timeout,0,0,0,0);
					qti->awol_retries --;
				}
				else {
					qlfc_target_set_awol (ctlr, loop_id);
				}
			}
			mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		}
	}

	qlfc_demon_take_lock (ctlr);

	if (event_count != ctlr->demon_scn_count) {
		event_count = ctlr->demon_scn_count;
		qlfc_demon_free_lock (ctlr);
		goto start_over;
	}

	if (ctlr->demon_scn_count == 0) {
		atomicClearInt (&ctlr->flags, CFLG_SCN);
	}
	atomicClearInt (&ctlr->flags, CFLG_STOP_PROBE|CFLG_LIP);
	qlfc_demon_free_lock (ctlr);

	qlfc_start_loop_queues (ctlr);

	TRACE (ctlr, "DEM_AWOL: clear STOP_PROBE\n",0,0,0,0,0,0);

	kmem_free (gpidnn_rsp, sizeof(sns_get_port_id_node_name_rsp_t));

        if (!(ctlr->flags & CFLG_DRAIN_IO))
		qlfc_start_scsi (ctlr);
}

static void
qlfc_demon_target_awol_recovery_loop (QLFC_CONTROLLER *ctlr, __psunsigned_t loop_id)
{
	qlfc_local_targ_info_t		*qti;
	int				found_it;
	int				awol_step;
	STATUS				status;

	/*
	** The process:
	**
	**	Try logging in to the device for 3 attempts.
	**	If the 3rd login fails, try lip reset to the target.
	**	Repeat login, lip reset sequence for 3 tries.
	**	If cannot get it back, initiate a lip followed by login.
	**	Repeat entire sequence 2 times.
	**	Still not present, give up.
	**
	**	WHAT TO DO ABOUT THE TARGET_MAP MUTEX?
	*/

	qlfc_demon_free_lock (ctlr);

	if (!(ctlr->target_map[loop_id].tm_flags & TM_RESERVED)) {
		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti && !(ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE)) {

			found_it = 0;
			awol_step = qti->awol_step;

			switch (awol_step) {
				case 16:
				case 20:
					#if 0
					qlfc_demon_lip_with_login (ctlr, -1);	/* argument unused */
					break;
					#endif
				case 4:
				case 8:
				case 12:
				case 18:
					qlfc_demon_lip_reset_target (ctlr, (__psunsigned_t) qti);
					break;
				default:
					status = qlfc_login_loop_port (ctlr, loop_id, 1);	/* 1 forces login anyway */
					if (status == OK) {
						/*
						** Got it back.
						*/
						found_it = 1;
					}
					break;
			}


			if (found_it) {
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qti = ctlr->target_map[loop_id].tm_qti;
				if (qti) {
					if (ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN) {
						qlfc_start_target_queue (ctlr,loop_id);	/* clears awol */
					}
				}
				else {
					atomicAddInt (&ctlr->ql_awol_count,-1);
					TRACE (ctlr, "awol rec: bail/sort of: awol count %d\n",ctlr->ql_awol_count,0,0,0,0,0);
					if (qlfc_debug >= 99) {
						qlfc_cpmsg (ctlr->ctlr_vhdl,"awol rec: bail/sort of: awol count %d\n",ctlr->ql_awol_count);
					}
				}
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
			else if (awol_step < 24) {
				qti->awol_step ++;
				qti->awol_timeout = AWOL_TIMEOUT;
				TRACE (ctlr, "awol rec: again: target %d, setting awol_timeout to %d\n",
					qti->target,qti->awol_timeout,0,0,0,0);
			}
			else {	/* it's hopeless */
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qlfc_target_set_awol (ctlr, loop_id);
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
		}
	}
        if (!(ctlr->flags & CFLG_DRAIN_IO))
		qlfc_start_scsi (ctlr);
}

static
void
qlfc_demon_target_awol_recovery (QLFC_CONTROLLER *ctlr, __psunsigned_t loop_id)
{
	qlfc_local_targ_info_t		*qti;

	qlfc_demon_take_lock (ctlr);
	if (ctlr->flags & (CFLG_STOP_PROBE|CFLG_LIP_RESET)) {
		mutex_lock (&ctlr->target_map[loop_id].tm_mutex,PRIBIO);
		qti = ctlr->target_map[loop_id].tm_qti;
		if (qti) {
			if (qti->awol_giveup_retries <= 0) {
				qti->awol_giveup_retries = 0;
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qlfc_target_set_awol (ctlr, loop_id);
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
			else {
				qti->awol_giveup_retries --;
				qti->awol_timeout = 10;	/* try again later */
				TRACE (ctlr, "awol rec: target %d, setting awol_timeout to %d\n",qti->target,qti->awol_timeout,0,0,0,0);
			}
		}
		mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		qlfc_demon_free_lock (ctlr);
	}
		
	/*
	** recovery functions entered with demon lock taken
	*/

	else if (ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE) {
		qlfc_demon_target_awol_recovery_fabric (ctlr,loop_id);
	}
	else {
		qlfc_demon_target_awol_recovery_loop (ctlr,loop_id);
	}
}

/* ARGSUSED */
static
void
qlfc_demon_process_pdbc (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	int	demon_pdbc_count;

	qlfc_demon_take_lock (ctlr);
	demon_pdbc_count = ctlr->demon_pdbc_count;
	atomicSetInt (&ctlr->flags, CFLG_PDBC_ACTIVE);
	atomicClearInt (&ctlr->flags, CFLG_LIP);
	qlfc_demon_free_lock (ctlr);

	TRACE (ctlr, "DEM_PDBC: logging in fabric ports.\n",0,0,0,0,0,0);
	qlfc_login_fabric_ports (ctlr);
	qlfc_login_loop_ports (ctlr);
	
	TRACE (ctlr, "DEM_PDBC: starting loop queues.\n",0,0,0,0,0,0);
	qlfc_start_loop_queues (ctlr);

	atomicSetInt (&ctlr->flags, CFLG_LOOP_UP);

	qlfc_demon_take_lock (ctlr);
	if (demon_pdbc_count == ctlr->demon_pdbc_count) {	/* finished! */
		ctlr->demon_pdbc_count = 0;
		atomicClearInt (&ctlr->flags, CFLG_PDBC|CFLG_PDBC_ACTIVE|CFLG_STOP_PROBE);

		qlfc_demon_free_lock (ctlr);
	        if (!(ctlr->flags & CFLG_DRAIN_IO))
			qlfc_start_scsi (ctlr);
	}
	else {
		qlfc_demon_free_lock (ctlr);
		qlfc_demon_process_pdbc (ctlr, arg);
	}

	return;
}

/* ARGSUSED */
static
void
qlfc_demon_process_scn (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	int				i;
	int				loop_id;
	int				found_it;
	uint32_t			*port, port_id, last;
	STATUS				status;
	sns_get_port_id_node_name_rsp_t	*gpidnn_rsp;

	union change_notification {
		struct {
			uint32_t	addrfmt:8;
			uint32_t	domain:8;
			uint32_t	area:8;
			uint32_t	alpa:8;
		}		field;
		uint32_t	data;
	} scn, match;
/*
	Examples:
	Address format 0 specifies a complete port id:
		SCN: addrfmt 0, domain 0x1, area 0x17, alpa 0x43

	Address format 1 specifies a domain and area, alpa wildcarded:
		SCN: addrfmt 1, domain 0x2, area 0x15, alpa 0x0

	Address format 2 specifies a domain with area and alpa wildcarded:
		SCN: addrfmt 2, domain 0x2, area 0x0, alpa 0x0

	Address format 3 specifies a fabric wide event with everything wildcarded:
		SCN: addrfmt 3, domain 0x0, area 0x0, alpa 0x0
*/

	gpidnn_rsp = (sns_get_port_id_node_name_rsp_t *) kmem_zalloc (sizeof(sns_get_port_id_node_name_rsp_t),
				VM_CACHEALIGN | VM_DIRECT);

	qlfc_demon_take_lock (ctlr);
	if (ctlr->flags & (CFLG_STOP_PROBE|CFLG_LIP_RESET)) {
		qlfc_demon_free_lock (ctlr);
		kmem_free (gpidnn_rsp, sizeof(sns_get_port_id_node_name_rsp_t));
		delay (HZ);
		qlfc_demon_notify_scn (ctlr, DEMON_MSG_SCN, (arg&0xffff0000)>>16, arg&0xffff);	/* start me up again, later */
		TRACE (ctlr, "DEM_SCN : stop_probe 0x%x, lip reset 0x%x, trying later.\n",
			ctlr->flags&CFLG_STOP_PROBE, ctlr->flags&CFLG_LIP_RESET, 0,0,0,0);

		return;
	}
		
	atomicSetInt (&ctlr->flags, CFLG_SCN_ACTIVE|CFLG_STOP_PROBE);	/* STOP not cleared until we're done here */

	qlfc_demon_free_lock (ctlr);

	while (ctlr->flags & CFLG_PROBE_ACTIVE) delay (HZ);

	atomicSetInt (&ctlr->flags, CFLG_SCN);

	scn.data = arg;
	TRACE (ctlr,"DEM_SCN : addrfmt %d, domain 0x%x, area 0x%x, alpa 0x%x\n",
		scn.field.addrfmt, scn.field.domain, scn.field.area, scn.field.alpa,
		0,0);


	status = qlfc_send_get_port_id_node_name (ctlr, gpidnn_rsp);

	if (status == OK) {
		int have_mutex = 0;

		/*
		** Check to see the currently present devices effected by this scn
		** are still present.  If not, log them out and stop their queues.
		** It's up to probe to get rid of them.
		**
		** If they're still present, log them back in (if logged out).
		**
		** Note: no NEW devices are added at this point.  Probe required.
		**       Only targets associated with this SCN are impacted.
		*/

		found_it = 0;

		for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {

			if (ctlr->target_map[loop_id].tm_flags&TM_RESERVED) continue;
			mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
			have_mutex = 1;

			if (ctlr->target_map[loop_id].tm_flags & TM_FABRIC_DEVICE) {

				last = 0;

#if 0
qlfc_cpmsg (ctlr->ctlr_vhdl, "SCN: match id 0x%x, port_id 0x%x\n", scn.data, ctlr->target_map[loop_id].tm_port_id);
#endif

				/*
				** NEW:
				**	Check and see if target effected by this SCN.
				*/

				match.data = ctlr->target_map[loop_id].tm_port_id;
				switch (scn.field.addrfmt) {
					case 0:
						if (scn.field.domain != match.field.domain
						 || scn.field.area   != match.field.area
						 || scn.field.alpa   != match.field.alpa) {	/* no match */

							mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							have_mutex = 0;
							continue;	/* not impacted */
						}
						break;
					case 1:
						if (scn.field.domain != match.field.domain
						 || scn.field.area   != match.field.area) {	/* no match */

							mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							have_mutex = 0;
							continue;	/* not impacted */
						}
						break;
					case 2:
						if (scn.field.domain != match.field.domain) {	/* no match */

							mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							have_mutex = 0;
							continue;	/* not impacted */
						}
						break;
				}

#if 0
qlfc_cpmsg (ctlr->ctlr_vhdl, "SCN: match id 0x%x, port_id 0x%x is interesting.\n\n", scn.data, ctlr->target_map[loop_id].tm_port_id);
#endif


				/*
				** Scan gpidnn response looking for wwn match and adjust the port id
				*/

				for (i=0; i<GPIDNN_MAX_PORTS && !last; i++) {
					port = (uint32_t *)&gpidnn_rsp->gpidnn_port[i].gpidnn_control_byte;
					port_id = *port & 0xffffff;
					last = gpidnn_rsp->gpidnn_port[i].gpidnn_control_byte;
					if (*port == 0) break;

					if (ctlr->target_map[loop_id].tm_node_name
					 == gpidnn_rsp->gpidnn_port[i].gpidnn_node_name) {

						found_it = 1;
						ctlr->target_map[loop_id].tm_port_id = port_id;	/* might have moved */
						break;
					}
				}
				if (!found_it) {
					if (qlfc_debug >= 99) {
						qlfc_tpmsg (ctlr->ctlr_vhdl, loop_id, "target missing after SCN.\n");
					}
					if (!(ctlr->target_map[loop_id].tm_flags & (TM_AWOL_PENDING|TM_AWOL))) {
						qlfc_target_set_awol_pending (ctlr, loop_id);	/* unlocks tm_mutex */
						have_mutex = 0;
					}
				}
				else if (!(ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN)) {
					mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
					have_mutex = 0;
					status = qlfc_login_fabric_port (ctlr,loop_id,port_id,NULL);
					mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
					have_mutex = 1;
					if (status == OK) {
						if (ctlr->target_map[loop_id].tm_flags & TM_LOGGED_IN) {
							qlfc_start_target_queue (ctlr,loop_id);	/* clears awol */
						}
					}
					else if (!(ctlr->target_map[loop_id].tm_flags & (TM_AWOL_PENDING|TM_AWOL))) {
						qlfc_target_set_awol_pending (ctlr, loop_id);	/* unlocks tm_mutex */
						have_mutex = 0;
					}
				}
			}
			if (have_mutex) {
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
				have_mutex = 0;
			}
		}
	}

	atomicClearInt (&ctlr->flags, CFLG_STOP_PROBE|CFLG_LIP|CFLG_SCN);

	TRACE (ctlr, "DEM_SCN : clear STOP_PROBE\n",0,0,0,0,0,0);

	kmem_free (gpidnn_rsp, sizeof(sns_get_port_id_node_name_rsp_t));

        if (!(ctlr->flags & CFLG_DRAIN_IO))
		qlfc_start_scsi (ctlr);
}

/* ARGSUSED */
static
void
qlfc_demon_reload_controller (QLFC_CONTROLLER *ctlr, __psunsigned_t arg)
{
	int		rc;
	__psint_t	reason = arg;
	int		retries_remaining, global_retries;

	if (!(ctlr->flags & CFLG_INITIALIZED)) return;

	atomicClearInt (&ctlr->flags, CFLG_LOOP_UP);
	if (reason == -1) {
		qlfc_cpmsg(ctlr->ctlr_vhdl, "Fatal fibre channel controller firmware error.  Reloading.\n");
	}
	else if (reason == 4) {
		qlfc_cpmsg(ctlr->ctlr_vhdl, "Fatal fibre channel controller firmware error.  Reloading.\n");
		qlfc_stop_queues (ctlr);
		qlfc_logout_fabric_ports (ctlr);
	}

	QL_MUTEX_ENTER(ctlr);

	global_retries    = 3;
	retries_remaining = 3;

	do {	/* global retries */
		do {	/* retries_remaining && rc */
			rc = qlfc_load_firmware (ctlr);		/* need start over completely! */
			if (rc) {
				if (retries_remaining) {
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Unable to reload controller firmware! Retrying.\n");
					delay (HZ);
				}
			}
			retries_remaining--;
		} while (rc && retries_remaining);

		if (rc) {
			if (!global_retries)
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Unable to reload controller firmware! Giving up.\n");
			delay (HZ);
		}

		else {
			int reset=0;
			atomicClearInt (&ctlr->flags, CFLG_ISP_PANIC);
			do {	/* retries_remaining && rc */
				rc = qlfc_reset_interface(ctlr,reset);
				if (rc) {
					if (retries_remaining) {
						qlfc_cpmsg(ctlr->ctlr_vhdl, "Unable to reset interface! Retrying.\n");
						delay (HZ);
						reset = 1;
					}
				}
				retries_remaining--;
			} while (rc && retries_remaining);
			if (rc) {
				if (!global_retries)
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Unable to reset interface! Giving up!.\n");
				delay (HZ/2);
			}
		}
		global_retries --;
	} while (rc && global_retries);

	qlfc_flush_queue(ctlr, SC_CMDTIME, ALL_QUEUES);	/* We want to retry these commands */

	if (!rc) atomicClearInt (&ctlr->flags, CFLG_ISP_PANIC);

	QL_MUTEX_EXIT(ctlr);

}


static
void
qlfc_demon (QLFC_CONTROLLER *ctlr)
{

	int		message;
	__psunsigned_t	arg;

	while (! (ctlr->demon_flags & DEMON_SHUTDOWN)) {
		TRACE (ctlr, "DEMON   : psema.\n",0,0,0,0,0,0);

		psema (&ctlr->demon_sema,PRIBIO);
		message = qlfc_demon_msg_dequeue_msg (ctlr);
		arg	= qlfc_demon_msg_dequeue_arg (ctlr);

		trace_on (ctlr);

		TRACE (ctlr, "DEMON   : back with the living, msg_out %d, flags 0x%x, msg 0x%x.\n",
			ctlr->demon_msg_out,ctlr->demon_flags,message,0,0,0);

		switch (message) {
			case DEMON_NO_MSG:	/* effectively a marker */
				atomicClearInt(&ctlr->demon_flags, DEMON_TIMEOUT);
				break;
			case DEMON_MSG_LIP_RESET:
				TRACE (ctlr,"DEMON   : lip reset, %d.\n",ctlr->lip_resets,0,0,0,0,0);
				qlfc_demon_process_lip_reset (ctlr, arg);
				break;
			case DEMON_MSG_LIP:
				TRACE (ctlr,"DEMON   : lip.\n",0,0,0,0,0,0);
				qlfc_demon_process_lip_complete (ctlr, arg);
				break;
			case DEMON_MSG_SCN:
				TRACE (ctlr,"DEMON   : scn.\n",0,0,0,0,0,0);
				qlfc_demon_process_scn (ctlr, arg);
				break;
			case DEMON_MSG_PDBC:
				TRACE (ctlr,"DEMON   : pdbc.\n",0,0,0,0,0,0);
				qlfc_demon_process_pdbc (ctlr, arg);
				break;
			case DEMON_MSG_SYSTEM_ERROR:
				TRACE (ctlr, "DEMON   : system error, reloading controller.\n",0,0,0,0,0,0);
				qlfc_demon_reload_controller (ctlr, arg);
				break;
			case DEMON_MSG_LINK_UP:
				TRACE (ctlr,"DEMON   : link up.\n",0,0,0,0,0,0);
				qlfc_demon_process_link_up (ctlr, arg);
				break;
			case DEMON_MSG_LOOP_UP:
				TRACE (ctlr,"DEMON   : loop up.\n",0,0,0,0,0,0);
				qlfc_demon_process_loop_up (ctlr, arg);
				break;
			case DEMON_MSG_LOOP_DOWN:
				TRACE (ctlr,"DEMON   : loop down.\n",0,0,0,0,0,0);
				qlfc_demon_process_loop_down (ctlr, arg);
				break;
			case DEMON_MSG_ERROR_RECOVERY:
				TRACE (ctlr, "DEMON   : performing error recovery.\n",0,0,0,0,0,0);
				qlfc_demon_error_recovery (ctlr, arg);
				break;
			case DEMON_MSG_SET_TARGET_AWOL:
				TRACE (ctlr, "DEMON   : marking loop id %d as missing\n",arg,0,0,0,0,0);
				qlfc_demon_target_set_awol_pending (ctlr, arg);
				break;
			case DEMON_MSG_TARGET_AWOL:
				TRACE (ctlr, "DEMON   : target missing, loop id %d\n",arg,0,0,0,0,0);
				qlfc_demon_target_awol_recovery (ctlr, arg);
				break;
			case DEMON_MSG_ABORT_DEVICE:
				TRACE (ctlr, "DEMON   : abort target, lun_info 0x%x\n",arg,0,0,0,0,0);
				qlfc_demon_abort_lun (ctlr, arg);
				break;
			default:
				break;
		}
	}
}

static
void
qlfc_demon_init (QLFC_CONTROLLER *ctlr)
{

	TRACE (ctlr, "DEMON   : initializing.  Controller %d-%d.\n",
		ctlr->ctlr_number, ctlr->ctlr_which,0,0,0,0);

	atomicClearInt (&ctlr->flags, CFLG_DEMON_NOT_READY);

	qlfc_demon (ctlr);

	TRACE (ctlr, "DEMON   : exit.  Controller %d-%d.\n",
		ctlr->ctlr_number, ctlr->ctlr_which,0,0,0,0);

	sthread_exit();
}
/* FILE: ioctl.c */
/* Entry-Point
** Function:    qlfc_ioctl
**
** Description: Perform host adapter I/O control operations.
*/
int
qlfc_ioctl(vertex_hdl_t qlfc_ctlr_vhdl, uint cmd, scsi_ha_op_t *op)
{
	QLFC_CONTROLLER *ctlr = qlfc_ctlr_info_from_ctlr_get(qlfc_ctlr_vhdl);
        int err = 0; /* assume no errors */
	int state = NO_QUIESCE_IN_PROGRESS;

	/* Now, execute passed command ...  */
    	switch(cmd)
        {
		case SOP_LIPRST:
			break;

		case SOP_LIP:
			if (qlfc_do_lip (qlfc_ctlr_vhdl)) {
				err = EIO;
			}
			break;

    		case SOP_RESET:
			if (qlfc_do_reset(qlfc_ctlr_vhdl)) {
				err = EIO;
				qlfc_cpmsg(qlfc_ctlr_vhdl, "SCSI bus reset failed\n");
			}
			break;

        	case SOP_SCAN:
	    		scan_bus(qlfc_ctlr_vhdl);
	    		break;

    		case SOP_DEBUGLEVEL:      /* set debug print count */
			if (op->sb_arg == 1) {
				qlfc_cpmsg (qlfc_ctlr_vhdl, "Controller wwn is 0x%x\n",ctlr->port_name);
			}
			else {
				qlfc_debug = op->sb_arg;
			}
	    		break;

        	case SOP_QUIESCE:

			/*
			 * set the quiesce in progress flag in the ctlr structure
			 * set two timeouts
			 *     1. timeout to succesfully quiesce bus
			 *     2. timeout for max time to allow quiesce
			 * if quiesce in progress replace timeouts with
			 * new ones
			 * sb_opt = time for quiesce attempt
			 * sb_arg = time for quiesce to be active
			 */

	    		if( (op->sb_opt == 0) || (op->sb_arg == 0) ) {
				err = EINVAL;
				break;
	    		}
	    		/* is this to extend the quiesce */
	    		if(ctlr->flags & CFLG_QUIESCE_COMPLETE) {
				untimeout(ctlr->quiesce_id);
				ctlr->quiesce_id =
				timeout(kill_quiesce, ctlr, op->sb_arg);
				break;
	    		}

	    		if (ctlr->ql_ncmd == 0) {
				atomicSetInt(&ctlr->flags, CFLG_QUIESCE_COMPLETE);
                		atomicClearInt(&ctlr->flags, CFLG_QUIESCE_IN_PROGRESS);
				ctlr->quiesce_id = timeout(kill_quiesce, ctlr, op->sb_arg);
	     		} else {
	         		atomicSetInt(&ctlr->flags, CFLG_QUIESCE_IN_PROGRESS);
	         		ctlr->quiesce_in_progress_id =
					timeout(kill_quiesce,ctlr, op->sb_opt);
                 		ctlr->quiesce_time = op->sb_arg;
	     		}
	     		break;


	        case SOP_UN_QUIESCE:
        		/*
			 * reset the in progress and complete flags
        		 * reset the timeouts
			 * reset the controller state and rescan the bus
			 * attempt to start pending commands
			 */
	    		kill_quiesce(ctlr);
            		break;

        	case SOP_QUIESCE_STATE:
        		/* report the queice state		*/
	    		if (ctlr->flags & CFLG_QUIESCE_IN_PROGRESS)
				state = QUIESCE_IN_PROGRESS;
			else
	    		if (ctlr->flags & CFLG_QUIESCE_COMPLETE)
				state = QUIESCE_IS_COMPLETE;

            		copyout(&state,(void *)op->sb_addr,sizeof(int));
	    		break;

#if 0	/* not needed, retain for historical perspectives */
		case SOP_SET_BEHAVE:
		{
			vertex_hdl_t		lun_vhdl;
			qlfc_local_lun_info_t	*qli;
		  
			switch(op->sb_opt) {
				case LUN_QERR: {
					u_char targ = (op->sb_arg >> SOP_QERR_TARGET_SHFT) & 0xFF;
					u_char lun  = (op->sb_arg >> SOP_QERR_LUN_SHFT) & 0xFF;
					u_char qerr = (op->sb_arg >> SOP_QERR_VAL_SHFT) & 0xFF;

					if ((ctlr->target_map[targ].tm_flags & TM_FABRIC_DEVICE) && qlfc_use_fabric_infrastructure) {
						lun_vhdl = scsi_fabric_lun_vhdl_get(ctlr->ctlr_vhdl,
							ctlr->target_map[targ].tm_node_name, 
							fc_short_portname(ctlr->target_map[targ].tm_node_name,ctlr->target_map[targ].tm_port_name),
							lun);
					}
					else {
						lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, targ, lun);
					}

					if (lun_vhdl == GRAPH_VERTEX_NONE) {
						qlfc_cpmsg (ctlr->ctlr_vhdl, 
							"SOP_SET_BEHAVE LUN_QERR failed: targ/lun %d/%d not valid\n", targ, lun);
						err = EINVAL;
						break;
					}
					qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
					if (qli) {
						if (qlfc_debug >= 99) {
							qlfc_cpmsg (ctlr->ctlr_vhdl, "IOCTL   : QERR1 %d for targ %d, lun %d\n",
								qerr,targ,lun,0,0,0);
						}
						TRACE (ctlr, "IOCTL   : QERR1 %d for targ %d, lun %d\n",
							qerr,targ,lun,0,0,0);
						if (qerr) {
							atomicSetInt(&qli->qli_dev_flags, DFLG_BEHAVE_QERR1);
						}
						else {
							atomicClearInt(&qli->qli_dev_flags, DFLG_BEHAVE_QERR1);
						}
					}
					else err = EINVAL;
					break;
				}
				default:
					qlfc_cpmsg (ctlr->ctlr_vhdl, "Illegal SOP_SET_BEHAVE sub-code 0x%x\n", op->sb_opt);
					err = EINVAL;
			}
			
			break;
		}
#endif

		default:
			err = EINVAL;
    	}

    	return(err);     /* nothing to return */
}

/* FILE: killQuiesce.c */
static void
kill_quiesce(QLFC_CONTROLLER *ctlr)
{
	atomicClearInt(&ctlr->flags, (CFLG_QUIESCE_IN_PROGRESS | CFLG_QUIESCE_COMPLETE));
	if(ctlr->quiesce_in_progress_id) {
		untimeout(ctlr->quiesce_in_progress_id);
		ctlr->quiesce_in_progress_id = NULL;
	}
	if(ctlr->quiesce_id) {
		untimeout(ctlr->quiesce_id);
		ctlr->quiesce_id = NULL;
		ctlr->quiesce_time = NULL;
	}
	/* try to startup and pending commands */
	if (!(ctlr->flags & CFLG_DRAIN_IO))
		qlfc_start_scsi(ctlr);
}

/* FILE: mbox.c */
/* Function:    qlfc_mbox_cmd()
**
** Return:  OK   no error
**      ERROR    error
**
** Description: This routine issues a mailbox command.
**      We're passed a list of mailboxes and the number of
**      mailboxes to read and the number to write.
**
** Note: I've added a special case (that may not be any problem)
** to skip loading mailbox 4 if the command is INITIALIZE RESPONSE QUEUE.
*/

static int
qlfc_mbox_cmd (QLFC_CONTROLLER *ctlr,
	    uint16_t *mbox_sts,
	    uint8_t out_cnt, uint8_t in_cnt,
	    uint16_t reg0, uint16_t reg1, uint16_t reg2, uint16_t reg3,
	    uint16_t reg4, uint16_t reg5, uint16_t reg6, uint16_t reg7)
{
	int rc;

	QL_MUTEX_EXIT(ctlr);
    	IOLOCK(ctlr->mbox_lock);

	rc =  qlfc_do_mbox_cmd (ctlr,mbox_sts,out_cnt,in_cnt,
			reg0,reg1,reg2,reg3,reg4,reg5,reg6,reg7);

	QL_MUTEX_ENTER(ctlr);	/* reacquire locks */
	IOUNLOCK(ctlr->mbox_lock);

	return rc;
}

/* ARGSUSED */
static int
qlfc_mbox_dma_cmd_rsp (QLFC_CONTROLLER *ctlr,
		uint16_t *mbox_sts, uint8_t out_cnt, uint8_t in_cnt,
		uint16_t reg0, uint16_t reg1,
		uint8_t *first_addr, int first_length,
		uint8_t *second_addr, int second_length)
{

	/*
	**	first_addr -
	**		this is address from which command data
	**		will be extracted
	**		regs 2,3,6,7 will have the request_misc_dmaptr
	**
	**	first_length -
	**		this is how many bytes are copied to request_misc
	**
	**	second_addr -
	**		this is the address to which response data
	**		will be returned
	**
	**	second_length -
	**		how much response data is returned
	**
	**	It is PRESUMED that the response_misc_dmaptr address has
	**	ALREADY BEEN ENCODED in the command data, above!
	*/

	int i;
	int rc=ERROR;
	__psunsigned_t	translated_addr = (__psunsigned_t)ctlr->request_misc_dmaptr;

	/*
	** Set up the dma into the request buffer if length non-zero.
	*/

	if (first_addr && first_length && second_addr && second_length) {

		QL_MUTEX_EXIT(ctlr);
    		IOLOCK(ctlr->mbox_lock);

		bcopy (first_addr, ctlr->request_misc, first_length);

#if 0
		TRACE (ctlr, "MBcmdrsp: first addr: 0x%x, first len %d, second addr 0x%x, second len %d\n",
			(__psint_t)first_addr,first_length,(__psint_t)second_addr,second_length,0,0);

		for (i=0; i<first_length && i < 32; i+=4) {
			TRACE (ctlr, "MBcmdrsp: cmd %d-%d: 0x%x 0x%x 0x%x 0x%x\n",
				i,i+3,
				first_addr[i+0],
				first_addr[i+1],
				first_addr[i+2],
				first_addr[i+3]);
		}
#endif

		/*
		** Initialize the response buffer.
		*/

		for (i=0; i<second_length; i++) *((uint8_t *)(ctlr->response_misc)+i) = 0xee;

		/*
		** Execute the mailbox command.
		*/

		rc = qlfc_do_mbox_cmd (ctlr,mbox_sts,8,8,
			reg0, reg1,
			(uint16_t)((__psunsigned_t)translated_addr >> 16),
			(uint16_t)((__psunsigned_t)translated_addr & 0xFFFF),
			0,0,
			(uint16_t) ((__psunsigned_t)translated_addr >> 48),
			(uint16_t) ((__psunsigned_t)translated_addr >> 32));

		/*
		** Extract the response data.
		*/

		bcopy (ctlr->response_misc, second_addr, second_length);
#if 0
		/*
		** Dump the first 40 ints, unswapped, and swapped.
		*/

		i = second_length / 4;
		if (i > 40) i = 40;
		i = i + (4-(i%4));
		printf ("MBcmdrsp: UNSWAPPED\n");
		for (k=0; k<i; k+=4) {
			uint32_t *j=(uint32_t *)second_addr;
			printf ("MBcmdrsp: 0x%x 0x%x 0x%x 0x%x\n",
				*(j+k+0),
				*(j+k+1),
				*(j+k+2),
				*(j+k+3));
		}
#endif

		qlfc_swap64 (second_addr, second_length);

#if 0
		printf ("MBcmdrsp: SWAPPED\n");
		i = second_length / 4;
		if (i > 40) i = 40;
		i = i + (4-(i%4));
		for (k=0; k<i; k+=4) {
			uint32_t *j=(uint32_t *)second_addr;
			printf ("MBcmdrsp: 0x%x 0x%x 0x%x 0x%x\n",
				*(j+k+0),
				*(j+k+1),
				*(j+k+2),
				*(j+k+3));
		}
#endif

		QL_MUTEX_ENTER(ctlr);	/* reacquire locks */
		IOUNLOCK(ctlr->mbox_lock);

	}

	return rc;
}

/* ARGSUSED */
static int
qlfc_mbox_dma_rsp (QLFC_CONTROLLER *ctlr,
		uint16_t *mbox_sts, uint8_t out_cnt, uint8_t inc_cnt,
		uint16_t reg0, uint16_t reg1,
		uint8_t *first_addr, int first_length)
{

	/*
	**	first_addr -
	**		this is address to which response data
	**		will be returned
	**		regs 2,3,6,7 will have the response_misc_dmaptr
	**
	**	first_length -
	**		this is how many bytes are copied from response_misc
	*/

	int rc=ERROR;
	__psunsigned_t	translated_addr = (__psunsigned_t)ctlr->response_misc_dmaptr;

	/*
	** Set up the dma into the response buffer if length non-zero.
	*/

	if (first_addr && first_length) {

		QL_MUTEX_EXIT(ctlr);
    		IOLOCK(ctlr->mbox_lock);

		/*
		** Execute the mailbox command.
		*/

		rc = qlfc_do_mbox_cmd (ctlr,mbox_sts,8,8,
			reg0, reg1,
			(uint16_t)((__psunsigned_t)translated_addr >> 16),
			(uint16_t)((__psunsigned_t)translated_addr & 0xFFFF),
			0,0,
			(uint16_t) ((__psunsigned_t)translated_addr >> 48),
			(uint16_t) ((__psunsigned_t)translated_addr >> 32));

		/*
		** Extract the response data.
		*/
		bcopy (ctlr->response_misc, first_addr, first_length);
		qlfc_swap64 (first_addr, first_length);

		QL_MUTEX_ENTER(ctlr);	/* reacquire locks */
		IOUNLOCK(ctlr->mbox_lock);

	}

	return rc;
}

/* ARGSUSED */
/* REFERENCED */
static int
qlfc_mbox_dma_cmd (QLFC_CONTROLLER *ctlr,
		uint16_t *mbox_sts, uint8_t out_cnt, uint8_t inc_cnt,
		uint16_t reg0, uint16_t reg1,
		uint8_t *first_addr, int first_length)
{

	/*
	**	first_addr -
	**		this is address from which command data
	**		will be extracted
	**		regs 2,3,6,7 will have the request_misc_dmaptr
	**
	**	first_length -
	**		this is how many bytes are copied to request_misc
	*/

	int rc=ERROR;
	__psunsigned_t	translated_addr = (__psunsigned_t)ctlr->request_misc_dmaptr;

	/*
	** Set up the dma into the request buffer if length non-zero.
	*/

	if (first_addr && first_length) {

		QL_MUTEX_EXIT(ctlr);
    		IOLOCK(ctlr->mbox_lock);

		bcopy (first_addr, ctlr->request_misc, first_length);

		/*
		** Execute the mailbox command.
		*/

		rc = qlfc_do_mbox_cmd (ctlr,mbox_sts,8,8,
			reg0, reg1,
			(uint16_t)((__psunsigned_t)translated_addr >> 16),
			(uint16_t)((__psunsigned_t)translated_addr & 0xFFFF),
			0,0,
			(uint16_t) ((__psunsigned_t)translated_addr >> 48),
			(uint16_t) ((__psunsigned_t)translated_addr >> 32));

		QL_MUTEX_ENTER(ctlr);	/* reacquire locks */
		IOUNLOCK(ctlr->mbox_lock);

	}
	return rc;
}

static int
qlfc_do_mbox_cmd(QLFC_CONTROLLER * ctlr,
	    uint16_t *mbox_sts,
	    uint8_t out_cnt, uint8_t in_cnt,
	    uint16_t reg0, uint16_t reg1, uint16_t reg2, uint16_t reg3,
	    uint16_t reg4, uint16_t reg5, uint16_t reg6, uint16_t reg7)
{
    	CONTROLLER_REGS * isp      = (CONTROLLER_REGS *)ctlr->base_address;
    	int	  complete  = OK;
    	int	  retry_cnt = 0;
	int       intr_retry_cnt = 0;
    	uint16_t   isr;
	uint16_t   bus_sema;
	uint16_t   hccr;

	if ((ctlr->flags & CFLG_ISP_PANIC) && reg0 != MBOX_CMD_EXECUTE_FIRMWARE && reg0 != MBOX_CMD_WRITE_RAM_WORD) {
		return ERROR;
	}

	QL_MUTEX_ENTER(ctlr);

/*
** Just for grins, clear out ALL the return status.
** This might catch a bug later where I try to use a return
** parameter that I wasn't given (would be left over from previous).
*/
    	mbox_sts[0] = 0;
    	mbox_sts[1] = 0;
    	mbox_sts[2] = 0;
    	mbox_sts[3] = 0;
    	mbox_sts[4] = 0;
    	mbox_sts[5] = 0;
    	mbox_sts[6] = 0;
    	mbox_sts[7] = 0;

	/*
	** Assure that the hccr HCCR_HOST_INT bit is clear
	**	Of course, at this time, I haven't a clue what
	**	I should do if it's NOT!  :)
	*/

	hccr = QL_PCI_INH(&isp->hccr);
	if (hccr & HCCR_HOST_INT) {
		TRACE (ctlr, "MBOX: protocol violation: host interrupt bit is set.  hccr: 0x%x.\n",hccr,0,0,0,0,0);
	}

	/*
	** Load the mailbox registers.
	**	Note: 4 and 5 are the request and
	**	      response queue pointers and are
	**	      not normally updated.
	*/

	switch (out_cnt) {
	    case 8:
		QL_PCI_OUTH(&isp->mailbox7, reg7);
	    case 7:
		QL_PCI_OUTH(&isp->mailbox6, reg6);
	    case 6:
		if (reg0 == MBOX_CMD_INITIALIZE_FIRMWARE)
			QL_PCI_OUTH(&isp->mailbox5, reg5);
	    case 5:
		if (reg0 == MBOX_CMD_INITIALIZE_FIRMWARE ||
		    reg0 == MBOX_CMD_LOAD_RAM ||
		    reg0 == MBOX_CMD_LOAD_RISC_RAM) {
			QL_PCI_OUTH(&isp->mailbox4, reg4);
		}
		TRACE (ctlr, "MBOX CMD: 4-7: %s: 0x%x 0x%x 0x%x 0x%x\n",
			(__psint_t)mbox_cmd_string+(QLFC_MBOX_CMD_STRING_LEN*(reg0&0xff)),
			reg4,
			reg5,
			reg6,
			reg7,
			0);
	    case 4:
		QL_PCI_OUTH(&isp->mailbox3, reg3);
	    case 3:
		QL_PCI_OUTH(&isp->mailbox2, reg2);
	    case 2:
		QL_PCI_OUTH(&isp->mailbox1, reg1);
	    case 1:
		QL_PCI_OUTH(&isp->mailbox0, reg0);
		if (reg0 != MBOX_CMD_WRITE_RAM_WORD) {
			TRACE (ctlr, "MBOX CMD: 0-3: %s: 0x%x 0x%x 0x%x 0x%x\n",
				(__psint_t)mbox_cmd_string+(QLFC_MBOX_CMD_STRING_LEN*(reg0&0xff)),
				reg0,
				reg1,
				reg2,
				reg3,
				0);
		}
	}

	/*
	 * Never called from ISR.  If dumping, then poll for mbox
	 * completion. Else wait on semaphore.
	 */
	if (ctlr->flags & (CFLG_DUMPING)) {
		atomicSetInt(&ctlr->flags, CFLG_MAIL_BOX_POLLING);
		atomicClearInt(&ctlr->flags, CFLG_MAIL_BOX_WAITING);
	}
	else {
		ctlr->mbox_timeout_info = MAILBOX_COMMAND_TIMEOUT;	/* seconds before mbox commands timeout */
		atomicSetInt(&ctlr->flags, CFLG_MAIL_BOX_WAITING);
		atomicClearInt(&ctlr->flags, CFLG_MAIL_BOX_POLLING);
	}
	atomicClearInt(&ctlr->flags, CFLG_MAIL_BOX_DONE);

	/* Wake up the isp.  */

	QL_PCI_OUTH(&isp->hccr, HCCR_CMD_SET_HOST_INT);


	QL_MUTEX_EXIT(ctlr);

	/*
	 * Wait for mailbox done semaphore ......
	 */
	if (ctlr->flags & CFLG_MAIL_BOX_WAITING) {
		psema(&ctlr->mbox_done_sema, PRIBIO);
		atomicClearInt(&ctlr->flags, CFLG_MAIL_BOX_WAITING);
		if ((reg0 = ctlr->mailbox[0]) != MBOX_STS_COMMAND_COMPLETE) {	/* yes, assignment */
			isr = QL_PCI_INH(&isp->isr);
			bus_sema = QL_PCI_INH(&isp->bus_sema);
			hccr = QL_PCI_INH(&isp->hccr);
			complete = ERROR;

		}
	}

	else {	/* poll for the completion 'cause we're in i/h */
again:
		retry_cnt = 100000;
		intr_retry_cnt = 100;
		while( !(ctlr->flags & CFLG_MAIL_BOX_DONE) ) {
			DELAY(10);
			if ((isr = QL_PCI_INH(&isp->isr)) & ISR_RISC_INTERRUPT) {
				IOLOCK(ctlr->res_lock);
				if (ctlr->flags & CFLG_MAIL_BOX_DONE) {
					IOUNLOCK(ctlr->res_lock);
					continue;
				}
				if ((bus_sema = QL_PCI_INH(&isp->bus_sema)) & ISEM_LOCK) {
					if ((ctlr->flags & (CFLG_IN_INTR | CFLG_DUMPING)) || (--intr_retry_cnt <= 0)) {
						TRACE (ctlr, "MBOXPOLL: calling qlfc_service_mbox_interrupt()\n",0,0,0,0,0,0);
						qlfc_service_mbox_interrupt(ctlr);
						if (ctlr->flags & CFLG_DUMPING)
							QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
					}
				}
				else {
					uint16_t response_in_new = QL_PCI_INH(&isp->mailbox5);

					if (ctlr->response_in != response_in_new) {
						ctlr->response_in = response_in_new;
						atomicSetInt(&ctlr->flags, CFLG_RESPONSE_QUEUE_UPDATE);
					}
					QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
				}
				IOUNLOCK(ctlr->res_lock);
			}
			if ( --retry_cnt <= 0) {
				reg0 = QL_PCI_INH(&isp->mailbox0);
				isr = QL_PCI_INH(&isp->isr);
				bus_sema = QL_PCI_INH(&isp->bus_sema);
				hccr = QL_PCI_INH(&isp->hccr);
				if ((hccr & HCCR_HOST_INT) && (isr & ISR_RISC_INTERRUPT)) {
					qlfc_cpmsg(ctlr->ctlr_vhdl, "Mailbox timeout "
						                "(m0 0x%x, isr 0x%x, bus_sema 0x%x, hccr 0x%x) "
						                "resetting ISR and retrying\n",
						 reg0, isr, bus_sema, hccr);
					QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
					goto again;
				}
				complete = ERROR;
				TRACE (ctlr,"MBOX    : timeout, m0 0x%x, isr 0x%x, bus_sema 0x%x, hccr 0x%x\n",
					reg0, isr, bus_sema, hccr,0,0);
				qlfc_cpmsg(ctlr->ctlr_vhdl,
					 "Mailbox timeout: intr_retry_cnt %d, m0 0x%x, isr 0x%x, bus_sema 0x%x, hccr 0x%x\n",
					 intr_retry_cnt, reg0, isr, bus_sema, hccr);
				goto done;
			}
		}
	}
done:

	/*
	** Load up response registers
	*/

	switch (in_cnt) {

		case 8:
		    mbox_sts[7] = ctlr->mailbox[7];

		case 7:
		    mbox_sts[6] = ctlr->mailbox[6];
		case 6:
		    mbox_sts[5] = ctlr->mailbox[5];
		case 5:
		    if (reg0 != MBOX_CMD_INIT_RESPONSE_QUEUE)
			mbox_sts[4] = ctlr->mailbox[4];
		case 4:
		    mbox_sts[3] = ctlr->mailbox[3];
		case 3:
		    mbox_sts[2] = ctlr->mailbox[2];
		case 2:
		    mbox_sts[1] = ctlr->mailbox[1];
		case 1:
		    mbox_sts[0] = ctlr->mailbox[0];
	}

    	return complete;
}

static void
qlfc_service_mbox_interrupt(QLFC_CONTROLLER *ctlr)
{
	CONTROLLER_REGS		*isp = (CONTROLLER_REGS *)ctlr->base_address;
	uint16_t		 mailbox0, mailbox1, mailbox2, mailbox3;
	scsi_request_t		*sr;
	REQ_COOKIE		 cookie;


	mailbox0 = QL_PCI_INH(&isp->mailbox0);

	if (mailbox0 != MBOX_ASTS_LIP_RESET) {
		atomicClearInt (&ctlr->flags, CFLG_SOLID_LIP);
		atomicClearInt (&ctlr->lip_resets, 0xffffffff);
	}

	/*
	** Process mailbox interrupt.
	*/

	switch (mailbox0) {

	case MBOX_ASTS_COMMAND_COMPLETE:
		mailbox2 = QL_PCI_INH(&isp->mailbox2);
		mailbox1 = QL_PCI_INH(&isp->mailbox1);

		cookie.data = mailbox2 << 16 | mailbox1;

		TRACE (ctlr, "MBOX    : status %x, COMMAND COMPLETE: cookie 0x%x\n",mailbox0,cookie.data,0,0,0,0);

		sr = qlfc_deque_sr (ctlr, cookie.data);
		if (!sr) {
			qlfc_cpmsg(ctlr->ctlr_vhdl, "duplicate cookie, IGNORED.\n");
			#if 1
				TRACE (ctlr, "CRITICAL: deque failure, cookie 0x%x\n",cookie.data,0,0,0,0,0);
			#else
				TRACE (ctlr, "PANIC   : deque failure, cookie 0x%x\n",cookie.data,0,0,0,0,0);
				panic ("qlfc: service_mbox_interrupt: could not find scsi request\n");
			#endif
		}

		else qlfc_cmd_complete_fast_post(ctlr, sr);
		break;

	case MBOX_ASTS_LINK_MODE_IS_UP:
		qlfc_cpmsg(ctlr->ctlr_vhdl, "MBOX: link mode is up.\n");
		TRACE (ctlr, "MBOX    : status %x, MBOX_ASTS_LINK_MODE_IS_UP\n",mailbox0,0,0,0,0,0);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_LINK_UP, 0);
		break;

	case MBOX_ASTS_CONFIGURATION_CHANGE:
		mailbox1 = QL_PCI_INH(&isp->mailbox1);
		TRACE (ctlr, "MBOX    : status %x, MBOX_ASTS_CONFIGURATION_CHANGE %d\n",mailbox0,mailbox1,0,0,0,0);
		if (qlfc_debug >= 99) {
			qlfc_cpmsg(ctlr->ctlr_vhdl, "MBOX: configuration change: value %d\n",mailbox1);
		}
		break;

	case MBOX_ASTS_SYSTEM_ERROR:
		mailbox1 = QL_PCI_INH(&isp->mailbox1);
		mailbox2 = QL_PCI_INH(&isp->mailbox2);
		mailbox3 = QL_PCI_INH(&isp->mailbox3);
		TRACE (ctlr, "MBOX    : status %x, 0x%x 0x%x 0x%x, ASTS_SYSTEM_ERROR\n",mailbox0,mailbox1,mailbox2,mailbox3,0,0);
		atomicSetInt (&ctlr->flags, CFLG_ISP_PANIC);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_SYSTEM_ERROR, -1);
		break;

	case MBOX_ASTS_LIP_RESET:
		TRACE (ctlr, "MBOX    : status %x, ASTS_LIP_RESET\n",mailbox0,0,0,0,0,0);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_LIP_RESET, 0);
		if (!(ctlr->flags & CFLG_SEND_MARKER) && qlfc_debug >= 99)
			qlfc_cpmsg(ctlr->ctlr_vhdl, "LIP RESET Occurred.\n");
		atomicSetInt(&ctlr->flags, CFLG_SEND_MARKER);
		atomicClearInt (&ctlr->flags, CFLG_LINK_MODE);
		break;

	case MBOX_ASTS_LIP_OCCURRED:
		TRACE (ctlr, "MBOX    : status %x, ASTS_LIP\n",mailbox0,0,0,0,0,0);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_LIP, 0);
		if (qlfc_debug >= 99) qlfc_cpmsg(ctlr->ctlr_vhdl, "LIP Occurred.\n");
		atomicClearInt (&ctlr->flags, CFLG_LINK_MODE);
		break;

	case MBOX_ASTS_LOOP_UP:
		TRACE (ctlr, "MBOX    : status %x, ASTS_LOOP_UP\n",mailbox0,0,0,0,0,0);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_LOOP_UP, 0);
		if (qlfc_debug >= 99) qlfc_cpmsg(ctlr->ctlr_vhdl, "LOOP UP Detected.\n");
		break;

	case MBOX_ASTS_LOOP_DOWN:
		TRACE (ctlr, "MBOX    : status %x, ASTS_LOOP_DOWN\n",mailbox0,0,0,0,0,0);
		qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_LOOP_DOWN, 0);
		if (qlfc_debug >= 99) qlfc_cpmsg(ctlr->ctlr_vhdl, "LOOP Down Detected.\n");
		break;

	case MBOX_ASTS_CHANGE_NOTIFICATION:
		mailbox1 = QL_PCI_INH(&isp->mailbox1);
		mailbox2 = QL_PCI_INH(&isp->mailbox2);
		TRACE (ctlr, "MBOX    : status %x, ASTS_CHANGE_NOTIFICATION\n",mailbox0,mailbox1,mailbox2,0,0,0);
		qlfc_demon_notify_scn (ctlr, DEMON_MSG_SCN, mailbox1, mailbox2);
		if (qlfc_debug >= 3) qlfc_cpmsg(ctlr->ctlr_vhdl, "Change Notification, 0x%x 0x%x.\n",mailbox1,mailbox2);
		break;

	case MBOX_ASTS_PORT_DATABASE_CHANGE:
		mailbox1 = QL_PCI_INH(&isp->mailbox1);
		mailbox2 = QL_PCI_INH(&isp->mailbox2);
		TRACE (ctlr, "MBOX    : status %x, ASTS_PORT_DATABASE_CHANGE\n",mailbox0,mailbox1,mailbox2,0,0,0);
		qlfc_demon_notify_pdbc (ctlr, DEMON_MSG_PDBC);
		if (qlfc_debug >= 99) qlfc_cpmsg(ctlr->ctlr_vhdl, "Port Data Base Change Detected, 0x%x 0x%x.\n",mailbox1, mailbox2);
		break;

	default:
		if ((mailbox0 >= 0x4000) && (mailbox0 <= 0x7FFF)) {
			TRACE (ctlr, "MBOX    : status %x, mailbox command completion.\n",
				mailbox0,0,0,0,0,0);
			ctlr->mailbox[7] = QL_PCI_INH(&isp->mailbox7);
			ctlr->mailbox[6] = QL_PCI_INH(&isp->mailbox6);
			ctlr->mailbox[5] = QL_PCI_INH(&isp->mailbox5);
			ctlr->mailbox[4] = QL_PCI_INH(&isp->mailbox4);
			ctlr->mailbox[3] = QL_PCI_INH(&isp->mailbox3);
			ctlr->mailbox[2] = QL_PCI_INH(&isp->mailbox2);
			ctlr->mailbox[1] = QL_PCI_INH(&isp->mailbox1);
			ctlr->mailbox[0] = QL_PCI_INH(&isp->mailbox0);

			atomicSetInt(&ctlr->flags, CFLG_MAIL_BOX_DONE);	/* Flag polling command done. */

			if (mailbox0 == MBOX_STS_INVALID_COMMAND)
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Invalid mailbox command: 0x%x\n", mailbox0);

			if (ctlr->flags & CFLG_MAIL_BOX_WAITING)
				vsema(&ctlr->mbox_done_sema);
		}
		else {
			TRACE (ctlr, "MBOX    : unexpected mailbox status 0x%x\n",mailbox0,0,0,0,0,0);
			qlfc_cpmsg (ctlr->ctlr_vhdl, "Unexpected mailbox interrupt status: 0x%x\n",mailbox0);
		}
		break;
	}

	QL_PCI_OUTH(&isp->bus_sema, 0);

	/*
	** TEST CODE SUGGESTED BY SHISHIR
	*/

	mailbox0 = QL_PCI_INH(&isp->mailbox0);
}
/* FILE: print.c */
static void
qlfc_cpmsg(dev_t ctlr_vhdl, char *fmt, ...)
{
	va_list		  ap;
	inventory_t      *pinv;

	if ((hwgraph_info_get_LBL(ctlr_vhdl, INFO_LBL_INVENT, (arbitrary_info_t *)&pinv) == GRAPH_SUCCESS) &&
	    (pinv->inv_controller != (major_t)-1))
		cmn_err(CE_CONT, "ql%d: ", pinv->inv_controller);
	else {
		QLFC_CONTROLLER *ctlr = qlfc_ctlr_info_from_ctlr_get (ctlr_vhdl);
		cmn_err(CE_CONT, "%s: ", ctlr->hwgname);
	}
	va_start(ap, fmt);
	icmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}

static void
qlfc_tpmsg(dev_t ctlr_vhdl, u_char target, char *fmt, ...)
{
	va_list		ap;
	inventory_t	*pinv;
	QLFC_CONTROLLER	*ctlr = qlfc_ctlr_info_from_ctlr_get (ctlr_vhdl);

	if ((hwgraph_info_get_LBL(ctlr_vhdl, INFO_LBL_INVENT, (arbitrary_info_t *)&pinv) == GRAPH_SUCCESS) &&
	    (pinv->inv_controller != (major_t)-1))
		if (ctlr->target_map[target].tm_flags & TM_FABRIC_DEVICE) {
			cmn_err(CE_CONT, "ql%dd(0x%x): ", pinv->inv_controller, ctlr->target_map[target].tm_node_name);
		}
		else {
			cmn_err(CE_CONT, "ql%dd%d: ", pinv->inv_controller, target);
		}
	else {
		if (ctlr->target_map[target].tm_flags & TM_FABRIC_DEVICE) {
			cmn_err(CE_CONT, "%s/%s/0x%x: ", ctlr->hwgname, EDGE_LBL_TARGET, ctlr->target_map[target].tm_node_name);
		}
		else {
			cmn_err(CE_CONT, "%s/%s/%d: ", ctlr->hwgname, EDGE_LBL_TARGET, target);
		}
	}
	va_start(ap, fmt);
	icmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}

static void
qlfc_lpmsg(dev_t ctlr_vhdl, u_char target, u_char lun, char *fmt, ...)
{
	va_list		ap;
	inventory_t	*pinv;
	QLFC_CONTROLLER	*ctlr = qlfc_ctlr_info_from_ctlr_get (ctlr_vhdl);

	if ((hwgraph_info_get_LBL(ctlr_vhdl, INFO_LBL_INVENT, (arbitrary_info_t *)&pinv) == GRAPH_SUCCESS) &&
	    (pinv->inv_controller != (major_t)-1))
		if (ctlr->target_map[target].tm_flags & TM_FABRIC_DEVICE) {
			cmn_err(CE_CONT, "ql%dd(0x%x)l%d: ", pinv->inv_controller, ctlr->target_map[target].tm_node_name, lun);
		}
		else {
			cmn_err(CE_CONT, "ql%dd%dl%d: ", pinv->inv_controller, target, lun);
		}
	else {
		if (ctlr->target_map[target].tm_flags & TM_FABRIC_DEVICE) {
			cmn_err(CE_CONT, "%s/%s/(0x%x)/%d: ", ctlr->hwgname, EDGE_LBL_TARGET, ctlr->target_map[target].tm_node_name, lun);
		}
		else {
			cmn_err(CE_CONT, "%s/%s/%d/%d: ", ctlr->hwgname, EDGE_LBL_TARGET, target, lun);
		}
	}
	va_start(ap, fmt);
	icmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}

/* REFERENCED */
static void
DBG(int level, char *format, ...)
{
	va_list ap;
	char tempstr[200];

	if (qlfc_debug >= level)
	{
		va_start(ap, format);
		vsprintf(tempstr, format, ap);
		printf(tempstr);
		va_end(ap);
	}
}

static char *
qlfc_completion_status_msg(uint completion_status)
{
	struct {
		int  code;
		char *msg;
	} qlfc_comp_msg[] = {
	{ SCS_COMPLETE,			"SCS_COMPLETE" },
	{ SCS_DMA_ERROR,		"SCS_DMA_ERROR" },
	{ SCS_RESET_OCCURRED,		"SCS_RESET_OCCURRED" },
	{ SCS_ABORTED,			"SCS_ABORTED" },
	{ SCS_TIMEOUT,			"SCS_TIMEOUT" },
	{ SCS_DATA_OVERRUN,		"SCS_DATA_OVERRUN" },
	{ SCS_DATA_UNDERRUN,		"SCS_DATA_UNDERRUN" },
	{ SCS_DEVICE_QUEUE_FULL,	"SCS_DEVICE_QUEUE_FULL" },
	{ SCS_PORT_UNAVAILABLE,		"SCS_PORT_UNAVAILABLE" },
	{ SCS_PORT_LOGGED_OUT,		"SCS_PORT_LOGGED_OUT" },
	{ SCS_PORT_CONFIGURATION_CHANGED,"SCS_PORT_CONFIGURATION_CHANGED" },
	{ -1,				"Unknown" }
	};

	int i;

	for (i = 0; qlfc_comp_msg[i].code != -1; ++i)
		if (completion_status == qlfc_comp_msg[i].code)
			return(qlfc_comp_msg[i].msg);
	return(qlfc_comp_msg[i].msg);
}
/* FILE: probe.c */
static void
qlfc_clear_reprobe (QLFC_CONTROLLER *ctlr)
{
	int i;
	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		if (ctlr->target_map[i].tm_flags & TM_RESERVED) continue;
		mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
		if (ctlr->target_map[i].tm_qti) {	/* local targ info exists for device */
			atomicClearInt (&ctlr->target_map[i].tm_flags, TM_REPROBE);	/* used to delete missing */
		}
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}
}

static void
qlfc_set_reprobe (QLFC_CONTROLLER *ctlr)
{
	int i;
	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		if (ctlr->target_map[i].tm_flags & TM_RESERVED) continue;
		mutex_lock (&ctlr->target_map[i].tm_mutex, PRIBIO);
		if (ctlr->target_map[i].tm_qti) {	/* local targ info exists for device */
			atomicSetInt (&ctlr->target_map[i].tm_flags, TM_REPROBE);	/* used to delete missing */
		}
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}
}

static void
qlfc_probe_remove_unprobed (QLFC_CONTROLLER *ctlr)
{
	/*
	** Clean up, remove all missing devices.
	** (We are probing, after all, this IS
	** the desired effect.)
	*/

	int			loop_id, lun;
	vertex_hdl_t		lun_vhdl;
	uint64_t		node_name, port_name;
	qlfc_local_lun_info_t	*qli;

	for (loop_id=0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {
		if (ctlr->target_map[loop_id].tm_flags&TM_RESERVED) continue;
		mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
		if (ctlr->target_map[loop_id].tm_flags&TM_REPROBE) {
			int fabric_device;
			/*
			 * If target no longer exists, remove all the LUN vertices
			 * also. Make sure no one has the LUN open before removing it --
			 * check ref count.
			 */
			TRACE (ctlr, "PROBE   : clean up : removing missing loop id %d\n",loop_id,0,0,0,0,0);
			atomicClearInt (&ctlr->target_map[loop_id].tm_flags, TM_LOGGED_IN);
			fabric_device = ctlr->target_map[loop_id].tm_flags&TM_FABRIC_DEVICE;
			node_name = ctlr->target_map[loop_id].tm_node_name;
			port_name = ctlr->target_map[loop_id].tm_port_name;
			for (lun = 0; lun < QL_MAXLUNS; lun ++) {
				if (fabric_device && qlfc_use_fabric_infrastructure) {
					lun_vhdl = scsi_fabric_lun_vhdl_get(ctlr->ctlr_vhdl,
						node_name, fc_short_portname(node_name,port_name), lun);
				}
				else {
					lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, loop_id, lun);
				}
				if (lun_vhdl != GRAPH_VERTEX_NONE) {
					qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
					if (qli->qli_ref_count == 0) {
						if (fabric_device) {
							qlfc_device_remove(ctlr, loop_id, 
								node_name, port_name, lun);
						}
						else {
							qlfc_device_remove(ctlr, loop_id, 0, 0, lun);
						}
						TRACE (ctlr, "PROBE   : removed missing loop id %d, lun %d\n",loop_id,lun,0,0,0,0);
					}
				}
			}
			atomicClearInt (&ctlr->target_map[loop_id].tm_flags, 0xffffffff);
		}
		mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
	}
}

static void
scan_bus(vertex_hdl_t qlfc_ctlr_vhdl)
{
	int disks_found = 0;

	QLFC_CONTROLLER *ctlr = qlfc_ctlr_info_from_ctlr_get (qlfc_ctlr_vhdl);

	QL_CPMSGS(qlfc_ctlr_vhdl, "Re-probing SCSI bus\n");

	disks_found = qlfc_probe_bus(ctlr);
	qlfc_cpmsg(qlfc_ctlr_vhdl, "%d FCP targets.\n",disks_found);
}

STATUS
qlfc_get_controller_loop_id (QLFC_CONTROLLER *ctlr, uint32_t *loop_id, uint32_t *port_id)
{
	STATUS		rc;
	uint16_t     	mbox_sts[MBOX_REGS];
	uint32_t	lid, pid;

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd (ctlr, mbox_sts, 1, 4,
			MBOX_CMD_GET_LOOP_ID,
			0, 0, 0, 0, 0, 0,
			0);

	QL_MUTEX_EXIT(ctlr);

	if (rc == OK) {

		pid = mbox_sts[3] << 16 | mbox_sts[2];
		lid = mbox_sts[1] & 0xff;

		TRACE (ctlr,"LOOP_ID : 0x%x 0x%x 0x%x 0x%x : 0x%x 0x%x\n",
			mbox_sts[0],
			mbox_sts[1],
			mbox_sts[2],
			mbox_sts[3],
			lid,
			pid);

		if (loop_id) *loop_id = lid;
		if (port_id) *port_id = pid;
	}

	return rc;
}

static int
qlfc_probe_bus (QLFC_CONTROLLER *ctlr)
{
	int			i;
	int			lock_taken=0;
	int			loop_devices_found = 0;
	int			fabric_devices_found = 0;
	int			lun;
	int			state_change_disabled = 0;
	STATUS			status;
	vertex_hdl_t		lun_vhdl;
	qlfc_local_lun_info_t	*qli;
	positionMap_t		*position_map=NULL;
	int			pmap_entries;
	sns_get_port_id3_rsp_t	*gpid3_rsp=NULL;
	int			loop_id;
	uint64_t		node_name;
	uint64_t		port_name;
	int			delay_count;
	uint32_t		ctlr_loop_id, ctlr_port_id;
	qlfc_local_targ_info_t	*qti;

	TRACE (ctlr, "PROBE   : begin.\n",0,0,0,0,0,0);

	if (qlfc_probe_wait_loop_up <= 10) qlfc_probe_wait_loop_up = 60;

	delay_count = qlfc_probe_wait_loop_up;

	if (!(ctlr->flags & CFLG_LOOP_UP))
		if (qlfc_debug >= 98) {
			qlfc_cpmsg(ctlr->ctlr_vhdl, "Loop is down.  Waiting %d seconds for loop up.\n",delay_count);
		}

	while (!(ctlr->flags & CFLG_LOOP_UP) && delay_count--) {
		TRACE (ctlr,"PROBE   : waiting for loop up: %d\n",
			delay_count,0,0,0,0,0);
		delay (HZ);
	}

restart_loop_probe:

	loop_devices_found = 0;

	if (ctlr->flags & CFLG_LOOP_UP) {

		if (!gpid3_rsp) {
			gpid3_rsp = (sns_get_port_id3_rsp_t *) kmem_zalloc (sizeof(sns_get_port_id3_rsp_t),
				VM_CACHEALIGN | VM_DIRECT);
		}

		if (!position_map) {
			position_map = (positionMap_t *) kmem_zalloc (sizeof(positionMap_t),
				VM_CACHEALIGN | VM_DIRECT);
		}

		if (!lock_taken) {
			mutex_lock(&ctlr->probe_lock, PRIBIO);
			lock_taken = 1;
		}

		if (ctlr->flags & CFLG_STOP_PROBE) {
			TRACE (ctlr, "PROBE   : loop probe: pause probe set.\n",0,0,0,0,0,0);
			atomicClearInt (&ctlr->flags, CFLG_PROBE_ACTIVE);
			delay_count=60;	/* seconds */
			while (delay_count-- && (ctlr->flags & CFLG_STOP_PROBE)) {
				TRACE (ctlr, "PROBE   : waiting: %d.\n",delay_count,0,0,0,0,0);
				delay (HZ);
			}
			qlfc_demon_take_lock (ctlr);
			if (ctlr->flags & CFLG_STOP_PROBE) {
				qlfc_demon_free_lock (ctlr);
				TRACE (ctlr, "PROBE   : stop probe remains set.  Try again later.\n",0,0,0,0,0,0);
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Demon remains active after 60 seconds.  Try again later.\n");
				goto out;
			}
			TRACE (ctlr, "PROBE   : resuming loop probe.\n",0,0,0,0,0,0);
			atomicSetInt(&ctlr->flags, CFLG_PROBE_ACTIVE);
			qlfc_demon_free_lock (ctlr);
		}


		/*
		** Disable State Change Notifications until finished with probe.
		*/
		
		if (!state_change_disabled) status = qlfc_set_scn (ctlr, 0xff);
		if (status == OK) state_change_disabled = 1;

		/*
		** Mark device as being reprobed.
		** If the reprobe flag is not cleared,
		** i.e., the device hasn't been discovered
		** this time, it'll be removed if possible.
		*/

		qlfc_set_reprobe (ctlr);	/* set TM_REPROBE in all target map entries */

		if (!(ctlr->flags & CFLG_LINK_MODE)) {
			status = qlfc_get_controller_loop_id (ctlr, &ctlr_loop_id, &ctlr_port_id);

			if (status == OK) status = qlfc_get_position_map (ctlr, position_map);

			pmap_entries = position_map->pm_count;
			if (status == OK && pmap_entries != 0xff) {

				for (i=0; i<pmap_entries; i++) {
					if ((loop_id=position_map->pm_alpa[i]) != 0xff) {
						lun = 0;
						loop_id = ALPA_2_TID(loop_id);

						#if 0
						status = qlfc_get_port_database (ctlr, loop_id, &database);
						if (status != OK) continue;
						#endif

						if (loop_id == ctlr_loop_id) continue;
						if (loop_id > 0x7d) continue; /* indicates sns */

						status = qlfc_get_node_name (ctlr, loop_id, &node_name, NULL);
						if (status != OK) continue;
						status = qlfc_get_port_name (ctlr, loop_id, &port_name, NULL);
						if (status != OK) continue;


						mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
						lun_vhdl = qlfc_device_add(ctlr, loop_id, 0, 0, lun);
						mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
						if (!qlfc_info(lun_vhdl)) {
							/*
							 * If loop_id no longer exists, remove all the LUN vertices
							 * also. Make sure no one has the LUN open before removing it --
							 * check ref count.
							 */
							mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
							atomicClearInt (&ctlr->target_map[loop_id].tm_flags, TM_LOGGED_IN);
							for (lun = 0; lun < QL_MAXLUNS; lun ++) {
								if ((lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, loop_id, lun))
								 != GRAPH_VERTEX_NONE) {
									qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
									if (qli->qli_ref_count == 0)
										qlfc_device_remove(ctlr, loop_id, 0, 0, lun);
								}
								if (lun == 0) mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							}
						}
						else {
							mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
							atomicClearInt (&ctlr->target_map[loop_id].tm_flags,TM_REPROBE|TM_FABRIC_DEVICE);
							atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_LOGGED_IN);
							ctlr->target_map[loop_id].tm_node_name = node_name;
							ctlr->target_map[loop_id].tm_port_name = port_name;
							loop_devices_found ++;
							qti = ctlr->target_map[loop_id].tm_qti;
							mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							for( lun=1; lun < QL_MAXLUNS; lun ++) {
								if (qti && qti->lunmask[lun/8] & (1<<(lun%8))) {
									TRACE (ctlr,"PROBE   : Probing loop_id %d, lun %d.\n",loop_id,lun,0,0,0,0);
									lun_vhdl = qlfc_device_add(ctlr, loop_id, 0, 0, lun);
									if (!qlfc_info(lun_vhdl)) {
										qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
										if (qli->qli_ref_count == 0)
											qlfc_device_remove(ctlr, loop_id, 0, 0, lun);
									}
								}
							}
						}
	
	
					}
					if (ctlr->flags & CFLG_STOP_PROBE) goto restart_loop_probe;
				}
			}
			else {
				TRACE (ctlr,"PROBE   : unable to get loop position map or host id during probe.\n",0,0,0,0,0,0);
			}
		}

		TRACE (ctlr, "PROBE   : starting fabric probe.\n",0,0,0,0,0,0);

restart_fabric_probe:

		fabric_devices_found = 0;

		if (ctlr->flags & CFLG_STOP_PROBE) {
			TRACE (ctlr, "PROBE   : fabric probe: pause probe set.\n",0,0,0,0,0,0);
			atomicClearInt (&ctlr->flags, CFLG_PROBE_ACTIVE);
			delay_count=60;	/* 60 seconds */
			while (delay_count-- && (ctlr->flags & CFLG_STOP_PROBE)) {
				delay (HZ);
			}
			qlfc_demon_take_lock (ctlr);
			if (ctlr->flags & CFLG_STOP_PROBE) {
				qlfc_demon_free_lock (ctlr);
				TRACE (ctlr, "PROBE   : stop probe remains set.  Try again later.\n",0,0,0,0,0,0);
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Demon remains active after 60 seconds.  Try again later.\n");
				goto out;
			}
			TRACE (ctlr, "PROBE   : resuming fabric probe.\n",0,0,0,0,0,0);
			atomicSetInt (&ctlr->flags, CFLG_PROBE_ACTIVE);
			qlfc_demon_free_lock (ctlr);

		}

		status = qlfc_get_port_name (ctlr, QL_FABRIC_FL_PORT, NULL, NULL);

		if (status == OK) status = qlfc_send_get_port_id3 (ctlr, 8, gpid3_rsp);
		if (status == OK) {
			uint32_t *port, port_id;

			for (i=0; i<GPID3_MAX_PORTS; i++) {
				sns_get_node_name_1_rsp_t	gnn_rsp;
				sns_get_port_name_rsp_t		gpn_rsp;
				int last=0;
				port = (uint32_t *)&gpid3_rsp->gpid3_port[i].gpid3_control_byte;
				port_id = *port & 0xffffff;
				last = *port & 0xff000000;
				if (*port == 0) break;

				status = qlfc_send_get_node_name (ctlr, port_id, &gnn_rsp);
				if (status == OK) status = qlfc_send_get_port_name (ctlr, port_id, &gpn_rsp);
				if (status == OK) {
					node_name = gnn_rsp.gnn1_node_name;
					port_name = gpn_rsp.gpn_port_name;
retry_assign_loop_id:
					loop_id = qlfc_assign_loop_id (ctlr, port_id, node_name, port_name);
					if (loop_id != -1) {
						uint16_t mbox[MBOX_REGS];
						/* target mutex locked by assign loop id */
						lun = 0;
						/* don't log in if already logged in */
						mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
						if (!(ctlr->target_map[loop_id].tm_flags&TM_LOGGED_IN)) {
							if (qlfc_login_fabric_port (ctlr, loop_id, port_id, &mbox[0]) != OK) {
								qlfc_free_loop_id (ctlr, loop_id);
								if (mbox[0] == MBOX_STS_LOOP_ID_USED) {
									/* Firmware can assign a fabric loop id to Brocade switch */
									qlfc_reserve_loop_id (ctlr, loop_id);
									goto retry_assign_loop_id;
								}
								else if (mbox[0] == MBOX_STS_PORT_ID_USED) {
									/* shouldn't happen */
									qlfc_cpmsg (ctlr->ctlr_vhdl, "PROBE: Port id already allocated to "
												     "loop id %d.\n",mbox[1]
										   );
								}
								else if (mbox[0] == MBOX_STS_ALL_IDS_IN_USE) {
									qlfc_cpmsg (ctlr->ctlr_vhdl, "PROBE: Encountered more targets "
												     "than available loop ids.\n"
										   );
									break;
								}
								continue;
							}
						}
						mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
						lun_vhdl = qlfc_device_add(ctlr, loop_id,
									   node_name,
									   port_name,
									   lun);
						mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
						if (!qlfc_info(lun_vhdl)) {
							/*
							 * If target no longer exists, remove all the LUN vertices
							 * also. Make sure no one has the LUN open before removing it --
							 * check ref count.
							 */
							TRACE (ctlr, "PROBE   : removing missing loop id %d\n",loop_id,0,0,0,0,0);
							mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
							for (lun = 0; lun < QL_MAXLUNS; lun ++) {

								if (qlfc_use_fabric_infrastructure) {
									lun_vhdl = scsi_fabric_lun_vhdl_get(ctlr->ctlr_vhdl,
										node_name, 
										fc_short_portname(node_name,port_name), 
										lun);
								}
								else {
									lun_vhdl = scsi_lun_vhdl_get(ctlr->ctlr_vhdl, loop_id, lun);
								}

								if (lun_vhdl != GRAPH_VERTEX_NONE) {
									qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
									if (qli->qli_ref_count == 0) {
										TRACE (ctlr, "PROBE   : removed missing loop id %d, lun %d\n",
											loop_id,lun,0,0,0,0);
										qlfc_device_remove(ctlr, loop_id,
												   node_name,
												   port_name,
												   lun);
									}
								}





							}
							atomicClearInt (&ctlr->target_map[loop_id].tm_flags, TM_REPROBE);
							mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
							qlfc_logout_fabric_port (ctlr, loop_id);
						}
						else {
							atomicClearInt (&ctlr->target_map[loop_id].tm_flags, TM_REPROBE);
							atomicSetInt (&ctlr->target_map[loop_id].tm_flags, TM_FABRIC_DEVICE);
							fabric_devices_found ++;
							qti = ctlr->target_map[loop_id].tm_qti;
							for( lun=1; lun < QL_MAXLUNS; lun ++) {
								if (qti && qti->lunmask[lun/8] & (1<<(lun%8))) {
									TRACE (ctlr,"PROBE   : Probing target %d, lun %d.\n",loop_id,lun,0,0,0,0);
									lun_vhdl = qlfc_device_add(ctlr, loop_id,
												   gnn_rsp.gnn1_node_name,
												   gpn_rsp.gpn_port_name,
												   lun);
									if (!qlfc_info(lun_vhdl)) {
										qli = SLI_INFO(scsi_lun_info_get(lun_vhdl));
										if (qli->qli_ref_count == 0)
											qlfc_device_remove(ctlr, loop_id,
												gnn_rsp.gnn1_node_name,
												gpn_rsp.gpn_port_name,
												lun);
									}
								}
							}
						}
					}
				}
				if (last) break;
				else if (ctlr->flags & CFLG_STOP_PROBE) goto restart_fabric_probe;
			}
		}
		qlfc_probe_remove_unprobed (ctlr);	/* get rid of missing devices */
	}
	else {
		TRACE (ctlr,"PROBE   : loop is down\n",0,0,0,0,0,0);
		qlfc_set_reprobe (ctlr);
		qlfc_probe_remove_unprobed (ctlr);	/* get rid of missing devices */
	}

out:
	if (gpid3_rsp) kmem_free (gpid3_rsp, sizeof(sns_get_port_id3_rsp_t));
	if (position_map) kmem_free (position_map,sizeof(positionMap_t));

	qlfc_clear_reprobe (ctlr);
	atomicClearInt (&ctlr->flags, CFLG_PROBE_ACTIVE);
	if (state_change_disabled) qlfc_set_scn (ctlr, 1);	/* re-enable state change notifications */
	if (lock_taken) mutex_unlock (&ctlr->probe_lock);
	TRACE (ctlr, "PROBE   : exiting, devices found %d\n",loop_devices_found+fabric_devices_found,0,0,0,0,0);
	return loop_devices_found+fabric_devices_found;
}

/* FILE: queueSpace.c */
static void
qlfc_queue_space(QLFC_CONTROLLER *ctlr, CONTROLLER_REGS *isp)
{
    	u_short mailbox4 = 0;

    	mailbox4 = QL_PCI_INH(&isp->mailbox4);
    	ctlr->request_out = mailbox4;

	/*
	** If the pointers are the same, then ALL the slots are available.
	** We have to account for wrap condition, so check both conditions.
	*/
    	if (ctlr->request_in == ctlr->request_out) {
        	ctlr->queue_space = ctlr->ql_request_queue_depth - 1;
    	} else if (ctlr->request_in > ctlr->request_out) {
        	ctlr->queue_space = ((ctlr->ql_request_queue_depth - 1) -
            	(ctlr->request_in - ctlr->request_out));
    	} else {
        	ctlr->queue_space = (ctlr->request_out - ctlr->request_in) - 1;
    	}

	if (qlfc_debug >= 99) {
		TRACE (ctlr,"QUE SPC : %d\n",ctlr->queue_space,0,0,0,0,0);
	}
}

/* FILE: revBBridge.c */
#ifdef BRIDGE_B_DATACORR_WAR
/* Stub for Bridge Rev B data corruption bub */
/* ARGSUSED */
int qlfc_bridge_rev_b_war(vertex_hdl_t pcibr_vhdl)
{
	return 0;
}
#endif
/* FILE: sensePrint.c */
#ifdef SENSE_PRINT
static void
qlfc_sensepr(vertex_hdl_t ctlr_vhdl, int unit, int key, int asc, int asq)
{
        qlfc_cpmsg(ctlr_vhdl, "Sense: unit %d: ", unit);
	if (key & 0x80)
		cmn_err(CE_CONT, "FMK ");
	if (key & 0x40)
		cmn_err(CE_CONT, "EOM ");
	if (key & 0x20)
		cmn_err(CE_CONT, "ILI ");
	cmn_err(CE_CONT, "sense key 0x%x (%s) asc 0x%x",
		key & 0xF, scsi_key_msgtab[key & 0xF], asc);
	if (asc < SC_NUMADDSENSE && scsi_addit_msgtab[asc] != NULL)
		cmn_err(CE_CONT, " (%s)", scsi_addit_msgtab[asc]);
	cmn_err(CE_CONT, " asq 0x%x\n", asq);
}
#endif

/* FILE: setDefaults.c */
/*
** Function:    qlfc_set_defaults
**
** Description: Sets the default parameters.
*/
static void
qlfc_set_defaults(QLFC_CONTROLLER *ctlr)
{
    	Default_parameters	*params = &ctlr->ql_defaults;
	char			*dev_admin;

	if (dev_admin = device_admin_info_get(ctlr->ctlr_vhdl, "qlfc_request_queue_depth")) {
		ctlr->ql_request_queue_depth = atoi(dev_admin);
		if ((ctlr->ql_request_queue_depth * IOCB_SIZE) < NBPP) {
			cmn_err(CE_WARN,"tried to set ql_request_queue_depth too low\n");
			ctlr->ql_request_queue_depth = NBPP / IOCB_SIZE;
			cmn_err(CE_WARN,"set to %d instead\n",ctlr->ql_request_queue_depth);
		}
		QL_CPMSGS(ctlr->ctlr_vhdl,
			  "DEVICE ADMIN: ql_request_queue_depth changed from %d to %d.\n",
			  REQUEST_QUEUE_DEPTH, ctlr->ql_request_queue_depth);
	}
	else {
		ctlr->ql_request_queue_depth = REQUEST_QUEUE_DEPTH;
	}

        if (dev_admin = device_admin_info_get(ctlr->ctlr_vhdl, "qlfc_response_queue_depth")) {
		ctlr->ql_response_queue_depth = atoi(dev_admin);
		if((ctlr->ql_response_queue_depth * IOCB_SIZE) < NBPP) {
			cmn_err(CE_WARN,"tried to set ql_response_queue_depth too low\n");
				ctlr->ql_response_queue_depth = NBPP / IOCB_SIZE;
			cmn_err(CE_WARN,"set to %d instead\n",ctlr->ql_response_queue_depth);
		}
		QL_CPMSGS(ctlr->ctlr_vhdl,
			"DEVICE ADMIN: ql_response_queue_depth changed from %d to %d.\n",
			RESPONSE_QUEUE_DEPTH, ctlr->ql_response_queue_depth);
	}
	else {
                ctlr->ql_response_queue_depth = RESPONSE_QUEUE_DEPTH;
	}

	if (dev_admin = device_admin_info_get(ctlr->ctlr_vhdl, "ql_hostid")) {
		params->fw_params.hardID = atoi(dev_admin);
		QL_CPMSGS(ctlr->ctlr_vhdl,
			  "DEVICE ADMIN: ql_hostid changed from %d to %d.\n",
			  INITIATOR_HARD_ID, params->fw_params.hardID);
	}
	else {
		params->fw_params.hardID =  INITIATOR_HARD_ID;
#ifdef scsquest
		if (arg_qlhostid[0]) {
			if ((arg_qlhostid[0] >= '0') && (arg_qlhostid[0] <= '9')) {
				params->fw_params.hardID = arg_qlhostid[0] - '0';
			}
			else {
				if ((arg_qlhostid[0] >= 'a') && (arg_qlhostid[0] <= 'f')) {
					params->fw_params.hardID = arg_qlhostid[0] - 'a' + 10;
				}
			}
			else {
				if ((arg_qlhostid[0] >= 'A') && (arg_qlhostid[0] <= 'F')) {
					params->fw_params.hardID = arg_qlhostid[0] - 'A' + 10;
				}
			}
		}
#endif
	}

	/*
	** Set up Risc Firmware Option defaults.
	*/

	if ( qLogicDeviceIds[ctlr->ctlr_type_index] == QLOGIC_2200) {
		params->fw_params.fwOption1.bits.extendedCB         = 1;
		params->fw_params.fwOption1.bits.nameOption         = 0;	/* clear: name is both port and node name */
		params->fw_params.fwOption0.bits.enableHardLoopID   = 0;
		params->fw_params.fwOption0.bits.enableFullDuplex   = qlfc_use_full_duplex;
	}
	else {
		params->fw_params.fwOption1.bits.extendedCB         = 0;
		params->fw_params.fwOption1.bits.nameOption         = 0;	/* clear: name is both port and node name */
		params->fw_params.fwOption0.bits.enableHardLoopID   = 0;
		params->fw_params.fwOption0.bits.enableFullDuplex   = 0;
	}

/* common options */
	params->fw_params.version = 1;
	params->fw_params.reserved0 = 0;

	params->fw_params.fwOption0.bits.enableFairness     = 1;	/* was 1 */
	params->fw_params.fwOption0.bits.enableFastposting  = 1;
	params->fw_params.fwOption0.bits.enableTargetMode   = 0;
	params->fw_params.fwOption0.bits.disableInitator    = 0;
	params->fw_params.fwOption0.bits.enableADISC        = 0;
	params->fw_params.fwOption0.bits.enableTGTDVCtype   = 0;

	params->fw_params.fwOption1.bits.enablePDBChange    = 1;
	params->fw_params.fwOption1.bits.disbaleInitialLIP  = 0;
	params->fw_params.fwOption1.bits.descendIDSearch    = 1;
	params->fw_params.fwOption1.bits.stopPortQueueFull  = 0;
	params->fw_params.fwOption1.bits.previousLoopId     = 0;
	params->fw_params.fwOption1.bits.fullLoginOnLIP     = 1;

	params->fw_params.maxFrameSize      = DEF_FRAME_SIZE;


	params->fw_params.maxIOCBperPort    = MAX_QUEUE_DEPTH;
	params->fw_params.executionThrottle = EXECUTION_THROTTLE; 	/* ff */
	params->fw_params.retryCount        = RETRY_COUNT;		/* 00 */
	params->fw_params.retryDelay        = RETRY_DELAY;		/* 00 */

	/*
	**  Do the Alignment of bytes to 32 bit word.
	**
	**  This field is used by fw to present the FC WWN.
	**  This must be unique and generally read from NVRAM.
	**  The values loaded here are GOOD ENOUGH to let the
	**  controller exist on a fabric with other QL controllers
	**  in the same host.  The file function qlfcInitIsp()
	**  reads the nvram and resets the portName to the value
	**  read.
	*/

	params->fw_params.portName[0]       = 0x20;
	params->fw_params.portName[1]       = 0;
	params->fw_params.portName[2]       = 0;
	params->fw_params.portName[3]       = 0xe0;

	params->fw_params.portName[4]       = 0x8b;
	params->fw_params.portName[5]       = 0;
	params->fw_params.portName[6]       = 0;
	params->fw_params.portName[7]       = ctlr->ctlr_number;


	params->fw_params.reserved1         = 0;
	params->fw_params.inquiry_data      = 0;
	params->fw_params.loginTimeout      = 0;
	params->fw_params.nodeName[0]       = 0;
	params->fw_params.nodeName[1]       = 0;
	params->fw_params.nodeName[2]       = 0;
	params->fw_params.nodeName[3]       = 0;
	params->fw_params.nodeName[4]       = 0;
	params->fw_params.nodeName[5]       = 0;
	params->fw_params.nodeName[6]       = 0;
	params->fw_params.nodeName[7]       = 0;

	params->Delay_after_reset = DELAY_AFTER_RESET;

}

/* FILE: startScsi.c */
/* Function:    qlfc_start_scsi
**
** Description: This routine tries to start up any new requests we have
**      pending on our internal queue.  This may be new commands
**      or it may be ones that we couldn't start berfore because
**      the adapter queues were full.
*/
static void
qlfc_start_scsi(QLFC_CONTROLLER *ctlr)
{
    	scsi_request_t	*request;
    	CONTROLLER_REGS *isp = (CONTROLLER_REGS *)ctlr->base_address;
    	alenlist_t	alenlist;
    	alenaddr_t	address;
    	size_t		length;
    	data_seg	dseg;

    	int		entries = 0, index;
	int		allow_prefetch;
	int		iocbs_issued = 0;

	/*
	 * Some other cpu is running this code so our job will get
	 * started by that cpu.
	 */
	if (mutex_trylock(&ctlr->req_lock) == 0) {
		return;
	}

	/*
	** Various conditions might be active which infer that new requests
	** should not be started.  If any of those flags are set, just exit.
	*/
    	if (ctlr->flags & CFLG_NO_START) {
		TRACE (ctlr, "START   : start inhibit flag(s) set: 0x%x\n",ctlr->flags & CFLG_NO_START,0,0,0,0,0);
            	IOUNLOCK(ctlr->req_lock);
	    	return;
	}

	/*
	 * Check the wait queue. If there's something in there, move
	 * it all over to the request queue.
	 */


        IOLOCK(ctlr->waitQ_lock);

        if (ctlr->waitf) {
	        if (ctlr->req_forw) {
			ctlr->req_back->sr_ha = ctlr->waitf;
		}
		else {
			ctlr->req_forw = ctlr->waitf;
		}
		ctlr->req_back = ctlr->waitb;
		ctlr->waitf = ctlr->waitb = NULL;
		ctlr->req_back->sr_ha = NULL;
	}

        IOUNLOCK(ctlr->waitQ_lock);

	/*
	 * If there is nothing on the list for us to do, then get out now.
	 */
    	if ((request = ctlr->req_forw) == NULL) {
        	IOUNLOCK(ctlr->req_lock);
		return;
    	}

	/*
	** If we're going to do a scatter/gather operation we may need
	** to use more than one queue entry (one for the request and one
	** or more for the scatter/gather table).  So, if we have less than
	** two queue slots available (might need one for a marker too)
	** then zap the number of queue entries so we'll go look again.
	** This is kindof a kludge, but ...  we's does whats we needs to do.
	*/

	/*
	** Determine how many entries are available on the request queue.
	**
	** This is kinda neat -- instead of always checking if there are
	** any new slots RIGHT NOW, we keep track of how many slots there
	** were the last time we checked.  Since we're the only one filling
	** these, there will always be AT LEAST that many still available.
	** Once we reach that number, theres no sense in issuing any more
	** until he's taken some.  So, when we filled up the number of slots
	** available on our last check, THEN we check again (to see if there
	** are anymore slots available).
	**
	** If we filled all the slots on the last pass, then check to see
	** how many slots there are NOW!
	*/

    	if ( ctlr->queue_space <= 2) {
		qlfc_queue_space(ctlr, isp);
    	}

	/*
	 * If we need to send a global Marker do it now
	 */
    	if (ctlr->queue_space != 0 && ctlr->flags & CFLG_SEND_MARKER) {
		marker_entry *q_ptr;

		TRACE (ctlr, "START   : sending MARKER, SYNC_ALL.\n",0,0,0,0,0,0);

		atomicClearInt(&ctlr->flags, CFLG_SEND_MARKER);
		q_ptr = (marker_entry *) ctlr->request_ptr;
		if (ctlr->request_in == (ctlr->ql_request_queue_depth - 1)) {
	    		ctlr->request_in  = 0;
	    		ctlr->request_ptr = ctlr->request_base;
		}
		else {
	    		ctlr->request_in++;
	    		ctlr->request_ptr+=IOCB_SIZE;
		}

		bzero(q_ptr, sizeof(marker_entry));

		q_ptr->hdr.entry_type   = ET_MARKER;
		q_ptr->hdr.entry_cnt    = 1;
		q_ptr->hdr.flags    = 0;
		q_ptr->hdr.sys_def_1    = ctlr->request_in;

		q_ptr->target_id    = 0;
		q_ptr->target_lun   = 0;
		q_ptr->target_lun_extended   = 0;
		q_ptr->modifier     = MM_SYNC_ALL;

#if (defined(IP30) || defined(SN)) && !defined(USE_MAPPED_CONTROL_STREAM)
		qlfc_swap32 ((uint8_t * )q_ptr, 64);
#endif
#ifdef FLUSH
		/* flush the entry out of the cache */
		MR_DCACHE_WB_INVAL(q_ptr, sizeof(marker_entry));
#endif
		/* Tell isp it's got a new I/O request...  */
		QL_PCI_OUTH(&isp->mailbox4, ctlr->request_in);

		/* One less I/O slot.  */
		ctlr->queue_space--;
iocbs_issued ++;
    	}

	/*
	** See if we can start one or more requests.
	** This means there's room in the queue, we have another request
	** to start, and we have a request structure for the request.
	*/
	while ( ctlr->queue_space && ctlr->req_forw ) {

		int			i;
		command_entry		*q_ptr;
		scsi_lun_info_t		*lun_info;
		scsi_targ_info_t	*targ_info;
		qlfc_local_lun_info_t	*qli;
		qlfc_local_targ_info_t	*qti;
		SR_SPARE		*spare;
		int			target, lun;

		request = ctlr->req_forw;

		lun_info = scsi_lun_info_get(request->sr_lun_vhdl);
		qli = SLI_INFO(lun_info);

		targ_info = SLI_TARG_INFO(lun_info);
		qti = STI_INFO(targ_info);

		target = STI_TARG(targ_info);
		lun = SLI_LUN(lun_info);

		spare = (SR_SPARE *)&request->sr_spare;

		/*
		 * Send a DEVICE SYNCH marker if we need to
		 */
		if ((qli->qli_dev_flags & (DFLG_SEND_MARKER|DFLG_ABORT_IN_PROGRESS)) == DFLG_SEND_MARKER) {
			marker_entry *q_ptr;

			TRACE (ctlr, "START   : sending MARKER, SYNC_DEV.\n",0,0,0,0,0,0);

			atomicClearInt(&qli->qli_dev_flags, DFLG_SEND_MARKER);

			q_ptr = (marker_entry * )ctlr->request_ptr;
			if (ctlr->request_in == (ctlr->ql_request_queue_depth - 1)) {
				ctlr->request_in  = 0;
				ctlr->request_ptr = ctlr->request_base;
			}
			else {
				ctlr->request_in++;
				ctlr->request_ptr+=IOCB_SIZE;
			}

			bzero(q_ptr, sizeof(marker_entry));
			q_ptr->hdr.entry_type   = ET_MARKER;
			q_ptr->hdr.entry_cnt    = 1;
			q_ptr->hdr.flags        = 0;
			q_ptr->hdr.sys_def_1    = ctlr->request_in;

			q_ptr->target_id    = target;
			if (ctlr->flags & CFLG_EXTENDED_LUN) {
				q_ptr->target_lun_extended   = lun;
				q_ptr->target_lun   = 0;
			}
			else {
				q_ptr->target_lun_extended   = 0;
				q_ptr->target_lun   = lun;
			}
			q_ptr->modifier     = MM_SYNC_DEVICE;

#if (defined(IP30) || defined(SN)) && !defined(USE_MAPPED_CONTROL_STREAM)
			qlfc_swap32 ((uint8_t * )q_ptr, 64);
#endif

#ifdef FLUSH
			/* flush the entry out of the cache */
			MR_DCACHE_WB_INVAL(q_ptr, sizeof(marker_entry));
#endif

			/* Tell isp it's got a new I/O request...  */
			QL_PCI_OUTH(&isp->mailbox4, ctlr->request_in);

			/* One less I/O slot.  */
			ctlr->queue_space--;
iocbs_issued++;
		}

		allow_prefetch = 1;

		if (request->sr_buflen) {

			/*
			 * If buf describes a user virtual address,
			 * dksc will have constructed a alenlist,
			 * stuck it in bp->b_private and set the
			 * SRF_ALENLIST flag.
			 */
			if (request->sr_flags & SRF_ALENLIST) {
				if (!IS_KUSEG(request->sr_buffer)) {
					panic("qlfc_start_scsi: address not UVADDR\n");
				}
				alenlist = (alenlist_t)((buf_t *)(request->sr_bp))->b_private;
				alenlist_cursor_init(alenlist, 0, NULL);
			}
			else if ( !(request->sr_flags & SRF_MAP) && (request->sr_flags & SRF_MAPBP)) {
				if (BP_ISMAPPED(((buf_t *)(request->sr_bp)))) {
					panic("qlfc_start_scsi: buffer is mapped\n");
				}
				alenlist = ctlr->alen_p;
#ifdef FLUSH
				bp_dcache_wbinval((buf_t *)(request->sr_bp));
#endif
				buf_to_alenlist(alenlist, request->sr_bp, AL_NOCOMPACT);
	    		}
			else {
				alenlist = ctlr->alen_p;
#ifdef FLUSH
				if (request->sr_flags & SRF_DIR_IN) {
		    			DCACHE_INVAL(request->sr_buffer, request->sr_buflen);
				}
				else {
		    			MR_DCACHE_WB_INVAL(request->sr_buffer, request->sr_buflen);
				}
#endif

				if (IS_KUSEG(request->sr_buffer)) {
					panic("qlfc_start_scsi: address is a UVADDR\n");
				}
				if ( kvaddr_to_alenlist(alenlist,
						   (caddr_t)request->sr_buffer,
						   request->sr_buflen, AL_NOCOMPACT) == NULL) {
					TRACE (ctlr,"STRTSCSI: kvaddr_to_alenlist failed.\n",0,0,0,0,0,0);
					/* Dequeue the request and respond to it. */
					ctlr->req_forw  = request->sr_ha;
					if (ctlr->req_forw == NULL) {
						ctlr->req_back = NULL;
					}
					request->sr_status = SC_REQUEST;
					qlfc_notify (ctlr, request, target, lun);
					continue;
				}
	    		}
	    		entries = alenlist_size(alenlist);

			/*
			 * Ensure request doesn't span more than 254
			 * continuation entries, i.e. length doesn't
			 * exceed 20.8 MB.  If it exceeds the
			 * max. length, unlink the request and toss it
			 * now.
			 */
			if (((entries - IOCB_SEGS) / CONTINUATION_SEGS + 1) > MAX_CONTINUATION_ENTRIES) {
				qlfc_cpmsg(ctlr->ctlr_vhdl, "Transfer length (0x%x) too large; "
					 "spans more than %d continuation entries.\n",
					 request->sr_buflen, MAX_CONTINUATION_ENTRIES);

				/* Dequeue the request and respond to it. */
				ctlr->req_forw  = request->sr_ha;
				if (ctlr->req_forw == NULL) {
					ctlr->req_back = NULL;
				}
				request->sr_status = SC_REQUEST;
				qlfc_notify (ctlr, request, target, lun);
				continue;
			}


			if ((((entries - IOCB_SEGS) / CONTINUATION_SEGS) + 1) >= ctlr->queue_space) {
				/*
				** Check actual available queue space.
				*/
				qlfc_queue_space(ctlr, isp);
				if ((((entries - IOCB_SEGS) / CONTINUATION_SEGS) + 1) >= ctlr->queue_space) {
		    			ctlr->queue_space = 0;
					cmn_err(CE_WARN, "qlfc_start_scsi: "
						         "No room for continuation entry. "
						         "Consider increasing ql_request_queue_depth\n");
		    			break;
				}
			}

#ifdef BRIDGE_B_PREFETCH_DATACORR_WAR
			/*
			 * If rev B bridge, disable prefetch
			 * for all pages comprising that I/O
			 */
			if ( ctlr->bridge_revnum == (u_short)-1 || ctlr->bridge_revnum <= BRIDGE_REV_B ) {
				TRACE (ctlr, "NO PREFETCH: request 0x%x\n",(__psint_t)request,0,0,0,0,0);
				allow_prefetch = 0;
			}
#endif
		}

		/*
		** Remove the request from the request queue.
		*/

	    	ctlr->req_forw  = request->sr_ha;

		if (ctlr->req_forw == NULL) {
		        ctlr->req_back = NULL;
		}

	    	/*
		** Initialize the request info structure for this request.
		**
		**	Each request receives a minimum timeout value of qlfc_watchdog_time.
		*/

	    	spare->field.timeout = (request->sr_timeout / HZ) + qlfc_watchdog_time;

	    	atomicAddInt (&ctlr->ql_ncmd,     1);

		IOLOCK(qli->qli_lun_mutex);
		atomicAddInt (&qli->qli_cmd_rcnt, 1);
	    	atomicAddInt (&qti->req_count,    1);

		if (qlfc_debug >= 99) {
			TRACE (ctlr, "STARTCNT: qli_cmd_rcnt %d, req_count %d,ql_ncmd %d\n",qli->qli_cmd_rcnt,
				qti->req_count,ctlr->ql_ncmd,0,0,0);
		}

		IOUNLOCK(qli->qli_lun_mutex);

		/* Get a pointer to the queue entry for the command.  */
		q_ptr = (command_entry * )ctlr->request_ptr;


		/* Move the internal pointers for the request queue.  */
		if (ctlr->request_in == (ctlr->ql_request_queue_depth - 1)) {
		    	ctlr->request_in  = 0;
		    	ctlr->request_ptr = ctlr->request_base;

		}
		else {
		    	ctlr->request_in++;
		    	ctlr->request_ptr+=IOCB_SIZE;
		}

		bzero(q_ptr, IOCB_SIZE);
		/* Fill in the command header.  */
		q_ptr->hdr.entry_type   = ET_COMMAND;
		q_ptr->hdr.entry_cnt    = 1;
		q_ptr->hdr.flags    = 0;
		q_ptr->hdr.sys_def_1    = ctlr->request_in;

		/*
		** Queue the request on the active queue.
		*/

		qlfc_queue_sr (ctlr, request);

		/* Fill in the rest of the command data.  */
		q_ptr->handle       = spare->field.cookie.data;
		q_ptr->target_id    = target;
		if (ctlr->flags & CFLG_EXTENDED_LUN) {
			q_ptr->target_lun_extended   = lun;
			q_ptr->target_lun   = 0;
		}
		else {
			q_ptr->target_lun   = lun;
			q_ptr->target_lun_extended   = 0;
		}
		q_ptr->control_flags = 0;
#ifndef A64_BIT_OPERATION
		q_ptr->reserved     = 0;
#endif
		if (spare->field.timeout) {
		    	q_ptr->time_out = 0; /* timeouts are handle in the driver */
		}
		else {
		    	q_ptr->time_out = DEFAULT_TIMEOUT;
		}

		/* Copy over the requests SCSI cdb.  */

		switch (request->sr_cmdlen) {
			case SC_CLASS2_SZ:
		    		q_ptr->cdb11 = request->sr_command[11];
		    		q_ptr->cdb10 = request->sr_command[10];
			case SC_CLASS1_SZ:
		    		q_ptr->cdb9 = request->sr_command[9];
		    		q_ptr->cdb8 = request->sr_command[8];
		    		q_ptr->cdb7 = request->sr_command[7];
		    		q_ptr->cdb6 = request->sr_command[6];
			case SC_CLASS0_SZ:
		    		q_ptr->cdb5 = request->sr_command[5];
		    		q_ptr->cdb4 = request->sr_command[4];
		    		q_ptr->cdb3 = request->sr_command[3];
		    		q_ptr->cdb2 = request->sr_command[2];
		    		q_ptr->cdb1 = request->sr_command[1];
		    		q_ptr->cdb0 = request->sr_command[0];
		}
		if (request->sr_buflen) {
			q_ptr->total_xfer_count = request->sr_buflen;
			if (!(request->sr_flags & SRF_DIR_IN)) {
				q_ptr->control_flags |= CF_WRITE;

#ifdef FLUSH
				if ( (request->sr_flags & SRF_MAP) &&
				  !(request->sr_flags & SRF_MAPBP)) {
					if (request->sr_buffer) {
						MR_DCACHE_WB_INVAL(request->sr_buffer, request->sr_buflen);
					}
				}
#endif
			}
			else {
				q_ptr->control_flags |= CF_READ;
#ifdef FLUSH
				if (request->sr_buffer) {
					DCACHE_INVAL(request->sr_buffer, request->sr_buflen);
				}
#endif
			}
		}
		else {	/* buflen */
			q_ptr->segment_cnt = 0;
			q_ptr->total_xfer_count = 0;
		}

		/* In FC we always support Tag */
		/* Set the type of Tag Command */

		switch (request->sr_tag) {
			case SC_TAG_HEAD:
				q_ptr->control_flags |= CF_HEAD_TAG;
			    	break;
			case SC_TAG_ORDERED:
				q_ptr->control_flags |= CF_ORDERED_TAG;
			    	break;
			default:
			    	q_ptr->control_flags |= CF_SIMPLE_TAG;
			    	break;
		}

		/* If this is a scatter/gather request, then we need */
		/* to adjust the data address and lengths for the    */
		/* scatter entries. 				 */

		/* If there are more than IOCB_SEGS entries in the scatter*/
		/* gather list, then we're going to be using some         */
		/* contiuation queue entries.  So, tell the command entry */
		/* how many continuations to expect.  Add two to include  */
		/* the command.*/

		if (entries > IOCB_SEGS) {
			int	i = 1;
			if ((entries - IOCB_SEGS) % CONTINUATION_SEGS) {
				i = 2;
			}
			q_ptr->hdr.entry_cnt = ((entries - IOCB_SEGS) / CONTINUATION_SEGS) + i;
		}

		/* Also, update the segment count with the number of */
		/* entries in the scatter/gather list.  */

		q_ptr->segment_cnt = entries;

		/* The command queue entry has room for some         */
		/* scatter/gather entries.  Fill them in.            */
		for (i = 0; i < IOCB_SEGS && entries; i++, entries--) {
			data_seg	*dsp;

			if (alenlist_get(alenlist, NULL, (size_t)(1<<30),
					 &address, &length, 0) != ALENLIST_SUCCESS) {
				panic("qlfc - bad alen conversion, alen 0x0x, sreq 0x%x, br 0x%x\n",
				      alenlist, request, request->sr_bp);
			}
			dsp = &dseg;
			fill_sg_elem(ctlr, dsp, address, length, allow_prefetch);
#ifdef A64_BIT_OPERATION
			switch (i) {
		           case 0:
				q_ptr->base_high_32_0 = dsp->base >> 32;
				q_ptr->base_low_32_0  = dsp->base;
				q_ptr->count_0  = dsp->count;
				break;
		           case 1:
				q_ptr->base_high_32_1 = dsp->base >> 32;
				q_ptr->base_low_32_1  = dsp->base;
				q_ptr->count_1  = dsp->count;
				break;
			}
#else
			q_ptr->dseg[i].base_low_32  = dsp->base;
			q_ptr->dseg[i].count = dsp->count;
#endif
		}

		/* If there is still more entries in the scatter/gather */
		/* list, then we need to use some continuation queue    */
		/* entries.  While there are more entries in the        */
		/* scatter/gather table, allocate a continuation queue  */
		/* entry and fill it in.  */

		index = IOCB_SEGS; /* set the index for the */

		/* next sg element pair  */
		while (entries) {
			continuation_entry * c_ptr;

			/* Get a pointer to the next queue entry for the */
			/* continuation of the command.  */
			c_ptr = (continuation_entry * )ctlr->request_ptr;

			/* Move the internal pointers for the request queue.  */
			if (ctlr->request_in == (ctlr->ql_request_queue_depth - 1)) {
		    		ctlr->request_in  = 0;
		    		ctlr->request_ptr = ctlr->request_base;

			}
			else {
		    		ctlr->request_in++;
		    		ctlr->request_ptr+=IOCB_SIZE;
			}

			/* Fill in the continuation header.  */
			/*zero out the entry. might take out later */
			bzero(c_ptr, sizeof(continuation_entry));
			c_ptr->hdr.entry_type   = ET_CONTINUATION;
			c_ptr->hdr.entry_cnt    = 1;
			c_ptr->hdr.flags    = 0;
			c_ptr->hdr.sys_def_1    = ctlr->request_in;
#ifndef A64_BIT_OPERATION
			c_ptr->reserved     = 0;
#endif
			/* Fill in the scatter entries for this continuation. */

			for (i = 0; i < CONTINUATION_SEGS && entries; i++, entries--) {
		    		data_seg	*dsp;

		    		if (alenlist_get(alenlist, NULL, (size_t)(1<<30),
				    &address, &length, 0) != ALENLIST_SUCCESS) {
					printf("ql - bad alen conversion\n");
		    		}
		    		dsp = &dseg;
				fill_sg_elem(ctlr, dsp, address, length, allow_prefetch);
#ifdef A64_BIT_OPERATION
				switch (i) {
					case 0:
						c_ptr->base_high_32_0 = dsp->base >> 32;
						c_ptr->base_low_32_0  = dsp->base;
						c_ptr->count_0  = dsp->count;
						break;
					case 1:
						c_ptr->base_high_32_1 = dsp->base >> 32;
						c_ptr->base_low_32_1  = dsp->base;
						c_ptr->count_1  = dsp->count;
						break;
					case 2:
						c_ptr->base_high_32_2 = dsp->base >> 32;
						c_ptr->base_low_32_2  = dsp->base;
						c_ptr->count_2  = dsp->count;
						break;
					case 3:
						c_ptr->base_high_32_3 = dsp->base >> 32;
						c_ptr->base_low_32_3  = dsp->base;
						c_ptr->count_3  = dsp->count;
						break;
					case 4:
						c_ptr->base_high_32_4 = dsp->base >> 32;
						c_ptr->base_low_32_4  = dsp->base;
						c_ptr->count_4  = dsp->count;
						break;
				}
#else
		    		c_ptr->dseg[i].base_low_32  = dsp->base;
		    		c_ptr->dseg[i].count = dsp->count;
#endif
		    		index++;
			}

			ctlr->queue_space--;	/* One less I/O slot. */
iocbs_issued++;

			/*flush the entry out of the cache */
#if (defined(IP30) || defined(SN)) && !defined(USE_MAPPED_CONTROL_STREAM)
			qlfc_swap32 ((uint8_t * )c_ptr, 64);
#endif
#ifdef FLUSH
			MR_DCACHE_WB_INVAL(c_ptr, sizeof(continuation_entry));
#endif
		}

#if (defined(IP30) || defined(SN)) && !defined(USE_MAPPED_CONTROL_STREAM)

		qlfc_swap32 ((uint8_t * )q_ptr, 64);
#endif
		/*flush the entry out of the cache */
#ifdef FLUSH
		MR_DCACHE_WB_INVAL(q_ptr, sizeof(command_entry));
#endif

		/* Tell isp it's got a new I/O request... */
		/* Wake up */
		QL_PCI_OUTH(&isp->mailbox4, ctlr->request_in);

		ctlr->queue_space--;		/* One less I/O slot.  */
		iocbs_issued++;

		/*
		** Make the trace entry.
		*/

		TRACE (ctlr, "START   : %d %d: sr 0x%x, len %d, %s, cookie 0x%x\n",
			target,
			lun,
			(__psint_t)request,
			request->sr_buflen,
			(__psint_t)qlfc_cdb_string+(QLFC_CDB_STRING_LEN*request->sr_command[0]),
			spare->field.cookie.data);

		/*
		** Check the waitQ to see if something's been queued
		** as we hold the req_lock and since the last time
		** we checked the waitQ.
		*/

		if (ctlr->req_forw == NULL) {
			IOLOCK(ctlr->waitQ_lock);
			if (ctlr->waitf) {
				ctlr->req_forw = ctlr->waitf;
				ctlr->req_back = ctlr->waitb;
				ctlr->waitf = ctlr->waitb = NULL;
				ctlr->req_back->sr_ha = NULL;
			}
			IOUNLOCK(ctlr->waitQ_lock);
		}

#if 0
		if (iocbs_issued > 100 || ctlr->ql_ncmd > 8) {
			atomicSetInt (&ctlr->flags, CFLG_START_THROTTLE);
			break;
		}
#endif
	}

	IOUNLOCK(ctlr->req_lock);
}

/* FILE: timer.c */
static void
qlfc_watchdog_timer(QLFC_CONTROLLER *ctlr)
{
	int			s;
	int			bailout;
	scsi_request_t		*request;
	qlfc_local_targ_info_t	*qti;
	CONTROLLER_REGS		*isp;
	int			loop_id;
	uint16_t		isr;

	if (qlfc_watchdog_time < 1) qlfc_watchdog_time = 1;
start:
	bailout = 0;
	isp = (CONTROLLER_REGS *)ctlr->base_address;

	isr = QL_PCI_INH(&isp->isr);
	if (!(ctlr->flags & CFLG_IN_INTR) && (isr & ISR_RISC_INTERRUPT)) {
		qlfc_intr (ctlr);
	}

	s = ctlr->flags & (CFLG_MAIL_BOX_POLLING | CFLG_MAIL_BOX_WAITING | CFLG_MAIL_BOX_DONE);
	if (s == CFLG_MAIL_BOX_WAITING) s = 1;
	else s = 0;

	traceif (ctlr, 1, "WDOG    : ctlr flags 0x%x, ncmd %d, mbox %d, solid lip 0x%x, awol count %d, isr 0x%x\n",
			(__psint_t)ctlr->flags, ctlr->ql_ncmd,
			s,
			(ctlr->flags & CFLG_SOLID_LIP),
			ctlr->ql_awol_count,
			isr);

	/*
	** Check for solid lip condition. If encountered, re-enable interrupts
	** and exit.
	*/

	if (ctlr->flags & CFLG_SOLID_LIP) {
		QL_MUTEX_ENTER (ctlr);
		atomicClearInt (&ctlr->flags, CFLG_SOLID_LIP);
		qlfc_enable_intrs (ctlr);
		QL_MUTEX_EXIT (ctlr);
		goto out;
	}

	/*
	** Check for pending mailbox command
	*/

	if (s) {

		IOLOCK(ctlr->res_lock);

		/*
		** Check again.
		*/

		request = NULL;

		if (CFLG_MAIL_BOX_WAITING ==
			(ctlr->flags & (CFLG_MAIL_BOX_POLLING | CFLG_MAIL_BOX_WAITING | CFLG_MAIL_BOX_DONE))) {

			if (ctlr->mbox_timeout_info > 0) {
				/*
				** Read the semaphore and hccr to determine if a response
				** is available in mailbox 0
				*/
				if ((QL_PCI_INH(&isp->bus_sema)) & ISEM_LOCK) {	/* missed an int, somehow */
					TRACE (ctlr, "TIMEOUT : missed a mailbox interrupt, somehow.\n",0,0,0,0,0,0);
					printf ("qlfc: TIMEOUT: missed a mailbox interrupt, somehow, icr 0x%x.\n", QL_PCI_INH(&isp->icr));
					qlfc_service_mbox_interrupt(ctlr);
					QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
					if (request = ctlr->compl_forw) {	/* yes, assignment */
						ctlr->compl_forw = ctlr->compl_back = NULL;
					}
				}
				else {
					ctlr->mbox_timeout_info -= qlfc_watchdog_time;
				}
			}
			else {

				/*
				** Read the semaphore and hccr to determine if a response
				** is available in mailbox 0
				*/
				if ((QL_PCI_INH(&isp->bus_sema)) & ISEM_LOCK) {	/* missed an int, somehow */
					TRACE (ctlr, "TIMEOUT : missed a mailbox interrupt, somehow.\n",0,0,0,0,0,0);
					printf ("qlfc: TIMEOUT: missed a mailbox interrupt, somehow.\n");
					qlfc_service_mbox_interrupt(ctlr);
					QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_RISC_INT);
					if (request = ctlr->compl_forw) {	/* yes, assignment */
						ctlr->compl_forw = ctlr->compl_back = NULL;
					}
				}
				else {
					TRACE (ctlr, "WDOG    : Mailbox command timeout, doing vsema!\n",
						0,0,0,0,0,0);

					qlfc_cpmsg(ctlr->ctlr_vhdl, "MBOX: Mailbox command timeout: reset controller!\n");
					atomicSetInt (&ctlr->flags, CFLG_ISP_PANIC);
					qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_SYSTEM_ERROR, -1);

					ctlr->mailbox[7] = QL_PCI_INH(&ctlr->base_address->mailbox7);
					ctlr->mailbox[6] = QL_PCI_INH(&ctlr->base_address->mailbox6);
					ctlr->mailbox[5] = QL_PCI_INH(&ctlr->base_address->mailbox5);
					ctlr->mailbox[4] = QL_PCI_INH(&ctlr->base_address->mailbox4);
					ctlr->mailbox[3] = QL_PCI_INH(&ctlr->base_address->mailbox3);
					ctlr->mailbox[2] = QL_PCI_INH(&ctlr->base_address->mailbox2);
					ctlr->mailbox[1] = QL_PCI_INH(&ctlr->base_address->mailbox1);
					ctlr->mailbox[0] = QL_PCI_INH(&ctlr->base_address->mailbox0);
					QL_PCI_OUTH(&isp->hccr, HCCR_CMD_CLEAR_HOST_INT);
					vsema(&ctlr->mbox_done_sema);
				}
			}
			bailout = ctlr->flags & CFLG_DRAIN_IO;
		}
		IOUNLOCK (ctlr->res_lock);

		/*
		** Don't worry about starting up requests via qlfc_start_scsi()
		*/
		if (request) {
			qlfc_notify_responses(ctlr, request);
		}
	}

	/*
	** Check for pending SCSI commands
	**
	**	If a request is stuck on the queue, we step through recovery for
	**	the controller, having the demon perform various actions based on the
	**	setting of the recovery_step variable in the controller structure.
	**
	**	While the DRAIN_IO flag is set, no new i/o gets started.
	**	Further subtractions of the timeout value of active requests
	**	are deferred until DRAIN_IO is cleared.
	**
	**	If forward progress is made on draining the queue, the
	**	drain timeout value is reset and we keep waiting.  This allows
	**	for SLOW devices (perhaps performing recovery) to have an
	**	opportunity to complete requests.
	**
	**	If no forward progress is made, upon drain timeout a message
	**	is queued to the demon informing it to initiate the first/next
	**	step of error recovery.
	**
	**	DRAIN_IO is only cleared when the last active request is
	**	removed from the queue, either via flushing the queue,
	**	some form of completion from the ql, or the timeout routine
	**	detecting ncmd to be zero.
	**
	**	The CFLG_STEP_RECOVERY_ACTIVE is used to synchronize the execution
	**	of a recovery step between the demon and the timeout routine.
	**	Essentially, it is set by this timeout routine and timeout processing
	**	stops until it is cleared by the demon.  It is the demon's 
	**	responsibility to reset or set the drain_timeout to a meaningful 
	**	non-zero value based on the chosen recovery action.
	**
	**	This code also restarts teh AWOL processing for a device.
	*/

	if (bailout || (ctlr->flags & CFLG_STEP_RECOVERY_ACTIVE)) goto out;

	if (ctlr->ql_ncmd || ctlr->ql_awol_count) {

		if (ctlr->flags & CFLG_DRAIN_IO) {
			if (ctlr->drain_count == ctlr->ql_ncmd) {	/* no change */
				ctlr->drain_timeout -= qlfc_watchdog_time;
				TRACE (ctlr,"WDOG    : no drain progress, timeout %d\n",
					ctlr->drain_timeout,0,0,0,0,0);
				if (ctlr->drain_count == 1 || ctlr->drain_timeout <= 0) {
					ctlr->drain_timeout = 0;
					atomicSetInt (&ctlr->flags, CFLG_STEP_RECOVERY_ACTIVE);
					qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_ERROR_RECOVERY, 0);
				}
			}
			else {	/* progress, reset command count and timeout */
				TRACE (ctlr,"WDOG    : drain progress, drain_count %d, commands %d\n",
					ctlr->drain_count,ctlr->ql_ncmd,0,0,0,0);
				ctlr->drain_count = ctlr->ql_ncmd;
				ctlr->drain_timeout = DRAIN_TIME;
			}
			mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
		}
		else {
			for (loop_id = 0; loop_id < QL_MAX_LOOP_IDS; loop_id++) {
				if (ctlr->target_map[loop_id].tm_flags & TM_RESERVED) continue;
				mutex_lock (&ctlr->target_map[loop_id].tm_mutex, PRIBIO);
				qti = ctlr->target_map[loop_id].tm_qti;
				if (qti) {
					IOLOCK (qti->target_mutex);
					if (qti->awol_timeout) {
						qti->awol_timeout -= qlfc_watchdog_time;
						if (qti->awol_timeout <= 0) {		/* gone awol, try to get it back */
							qti->awol_timeout = 0;
							TRACE (ctlr, "WDOG    : target %d, setting awol_timeout to zero, sending demon msg\n",
								qti->target,0,0,0,0,0);
							qlfc_demon_msg_enqueue (ctlr, DEMON_MSG_TARGET_AWOL, qti->target);
						}
					}
					else if (qti->req_count) {
						if (qlfc_debug >= 98) {
							TRACE (ctlr, "WDOG    : ctlr 0x%x, tvhdl %d, count %d, drain 0x%x, drain to %d\n",
								(__psint_t)ctlr,
								qti->targ_vertex,
								qti->req_count,
								(ctlr->flags & CFLG_DRAIN_IO),ctlr->drain_timeout,0);
						}
						request = qti->req_active;
						while (request) {
							SR_SPARE	*spare;
							int		timeout;
							spare = (SR_SPARE *)&request->sr_spare;
						    	timeout = (spare->field.timeout -= qlfc_watchdog_time);

							TRACE (ctlr, "WDOG    : sr 0x%x, timeout %d\n",
								(__psint_t)request,spare->field.timeout,0,0,0,0);

							if (timeout <= 0) {
								atomicSetInt (&ctlr->flags, CFLG_DRAIN_IO);
								ctlr->drain_count = ctlr->ql_ncmd;
								ctlr->drain_timeout = DRAIN_TIME;
								qlfc_tpmsg (
									ctlr->ctlr_vhdl, qti->target,
									"command timeout: 0x%x\n",
									request->sr_command[0]
									);
								break;
							}
							request = request->sr_ha;
						}
					}
					IOUNLOCK (qti->target_mutex);
				}
				mutex_unlock (&ctlr->target_map[loop_id].tm_mutex);
			}
		}

	}
	if (!ctlr->ql_ncmd) {
		atomicClearInt (&ctlr->flags, CFLG_DRAIN_IO);
	}


	if (!(ctlr->flags & CFLG_DRAIN_IO) && ((ctlr->waitf) || (ctlr->req_forw))) {
		qlfc_start_scsi(ctlr);
	}

out:
	delay (qlfc_watchdog_time*HZ);
	goto start;
}
/* FILE: init.c */
/********************************************************************************
*										*
*	qlfcInit								*
*										*
*	Initialize an ISP on the PCI bus.					*
*										*
*	Entry:									*
*		Called by qlfcAttach						*
*										*
*	Input:									*
*		ctlr - pointer to the controller structure			*
*										*
*	Output:									*
*		Returns 0, success						*
*			!0, failure						*
*										*
*	Notes:									*
*										*
********************************************************************************/

/* Function:    qlfcInit
**
** Description: Initialize specific ISP board.
** Returns: 0 = success
**      1 = failure
*/
static int
qlfcInit (QLFC_CONTROLLER *ctlr)
{
	STATUS		rc=OK;
	int		i;

	/* Allocate and init memory for the adapter structure.  */


#ifdef PIO_TRACE
	ctlr->ql_log_vals = (uint *)kmem_zalloc (sizeof(uint) * QL_LOG_CNT,
		VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);
#endif

	qlfc_ctlr_info_put (ctlr);

	INITLOCK(&ctlr->res_lock,   "qlfc_response",   ctlr->ctlr_vhdl);
	INITLOCK(&ctlr->req_lock,   "qlfc_request",    ctlr->ctlr_vhdl);
	INITLOCK(&ctlr->mbox_lock,  "qlfc_mbox_lock",  ctlr->ctlr_vhdl);
	INITLOCK(&ctlr->waitQ_lock, "qlfc_waitQ_lock", ctlr->ctlr_vhdl);
	INITLOCK(&ctlr->probe_lock, "qlfc_probe_lock", ctlr->ctlr_vhdl);
	INITLOCK(&ctlr->demon_lock, "qlfc_demon_lock", ctlr->ctlr_vhdl);

	for (i=0; i<QL_MAX_LOOP_IDS; i++) {
		/* init the lock even if reserved.  Might prevent a panic. */
		INITLOCK (&ctlr->target_map[i].tm_mutex, "qlfc_tmap", ctlr->ctlr_vhdl);
	}

	initnsema(&ctlr->demon_sema,     0, "qlfc_demon");
	initnsema(&ctlr->mbox_done_sema, 0, "qlfc_mboxsema");


	/* Clear the state flags.  */
	ctlr->flags = 0;

	/* put together the alenlist */
	ctlr->alen_p = alenlist_create(0);
	alenlist_grow(ctlr->alen_p, v.v_maxdmasz);

	/* Init the raw request queue to empty.  */
	ctlr->req_forw = NULL;
	ctlr->req_back = NULL;

	atomicClearInt(&(ctlr->ql_ncmd), 0xffffffff);

	/*
	** Start the service thread
	*/

	atomicSetInt (&ctlr->flags, CFLG_DEMON_NOT_READY);

	sthread_create ("qlfc",0,0,0,
		-10,	/* some priority, TBD later */
		KT_PS,	/* priority scheduled */
		(st_func_t *)qlfc_demon_init,
		(void *)ctlr, (void *)NULL, (void *)NULL, (void *)NULL);

	i = 40;	/* wait no more than 20 seconds */
	while ((ctlr->flags & CFLG_DEMON_NOT_READY) && i) {
		delay (HZ/2);
		i --;
	}
	if ((ctlr->flags & CFLG_DEMON_NOT_READY)) {
		qlfc_cpmsg(ctlr->ctlr_vhdl, "demon initialization failure!\n");
		rc = 4;
		goto init_failed;
	}


	pciio_intr_connect(
			pciio_intr_alloc (ctlr->pci_vhdl,
					    device_desc_default_get(vhdl_to_dev(ctlr->pci_vhdl)),
					    PCIIO_INTR_LINE_A,
					    ctlr->ctlr_vhdl),
			(intr_func_t)qlfc_intr,
			(intr_arg_t)ctlr,
			(void *)NULL);

	if (qlfcInitIsp(ctlr)) {
		ctlr->flags = CFLG_SHUTDOWN;
		rc = 2;
		goto init_failed;
	}
#ifdef scsquest
	/* We added a parameter that can be used to LOGICALLY */
	/* disable the board.  If they set this flag, then we shouldn't */
	/* publish this adapter board to the SCSI drivers.  However, */
	/* we do need to be able to open the device in order to update */
	/* the parameters to re-enable this board. */
	/* So say we failed the initialization, and check whether the */
	/* actual initialization of the adapter failed (with INITIALIZED */
	/* flag) in open routine.  */

	if (!ctlr->ql_defaults.HA_Enable) {
		rc = 3;
		goto init_failed;
	}
#endif

#ifdef IP30	/* remember this board for ACFAIL */
	baseio_qlsave(ctlr->ctlr_vhdl);
#endif

	/* Indicate we found the board */
	return OK;

init_failed:
	if (qlfc_debug >= 99)
		qlfc_cpmsg(ctlr->ctlr_vhdl, "qlfcInit failed (%d)\n", rc);
	return ERROR;
}



/* Function:    qlfcInitIsp()
**
** Description: Initialize the ISP chip on a specific board.
**              Loads the F/W Option Paramter Block.
**              Waits for F/W State to be Ready.
** Returns: 0 = success
**      1 = failure
*/

static int
qlfcInitIsp(QLFC_CONTROLLER * ctlr)
{
	int		rc = 0;
	int		howmuch, request_pages, response_pages;
	uint8_t		*portname;
	uint64_t	pn;

	QL_MUTEX_ENTER(ctlr);
	qlfc_set_defaults(ctlr);

	howmuch = IOCB_SIZE*ctlr->ql_response_queue_depth;
	howmuch += qlfc_extra_response_space();
	ctlr->response_size = howmuch;

	response_pages = btoc (howmuch);

	TRACE (ctlr, "INIT    : response size %d, pages %d\n",ctlr->response_size,response_pages,0,0,0,0);

	ctlr->response_base = kvpalloc (response_pages, 
					VM_UNCACHED | VM_DIRECT | VM_NOSLEEP | VM_PHYSCONTIG, 
					0);

    	if (ctlr->response_base == NULL) {
		rc = 1;
		goto init_done;
    	}


	howmuch = IOCB_SIZE*ctlr->ql_request_queue_depth;
	howmuch += qlfc_extra_request_space();
	ctlr->request_size = howmuch;

	request_pages = btoc (howmuch);

	TRACE (ctlr, "INIT    : request  size %d, pages %d\n",ctlr->request_size,request_pages,0,0,0,0);

	ctlr->request_base = kvpalloc (request_pages, 
				       VM_UNCACHED | VM_DIRECT | VM_NOSLEEP | VM_PHYSCONTIG, 
				       0);

    	if(ctlr->request_base == NULL) {
		kvpfree (ctlr->response_base,response_pages);
		ctlr->response_base = NULL;
		rc = 2;
		goto init_done;
    	}

	ctlr->request_misc = ctlr->request_base + (IOCB_SIZE*ctlr->ql_request_queue_depth);
	ctlr->response_misc = ctlr->response_base + (IOCB_SIZE*ctlr->ql_response_queue_depth);

    	ctlr->request_ptr = ctlr->request_base;
    	ctlr->response_ptr = ctlr->response_base;
    	ctlr->response_out = 0;

	/* Initialize request queue indexes.  */

    	ctlr->queue_space = 0;
    	ctlr->request_in  = 0;
    	ctlr->request_out = 0;

	/*
	** Read the nvram data and extract the node name.
	*/

	TRACE (ctlr, "INIT    : reading nvram.\n",0,0,0,0,0,0);

	nvram_rd (ctlr);
	portname = &ctlr->nvram_buf.nvram_data.fw_params.portName[0];
	qlfc_swap16 (portname,sizeof(uint64_t));

	pn = ((uint64_t)portname[0] << 56) |
	     ((uint64_t)portname[1] << 48) |
	     ((uint64_t)portname[2] << 40) |
	     ((uint64_t)portname[3] << 32) |
	     ((uint64_t)portname[4] << 24) |
	     ((uint64_t)portname[5] << 16) |
	     ((uint64_t)portname[6] << 8 ) |
	      (uint64_t)portname[7]       ;

	/*
	** Crude verification on portname
	*/
	if ((portname[0] == 0x20 || portname[0] == 0x21) &&
	    (portname[2] == 0 && portname[3] == 0xe0 && portname[4] == 0x8b)) {

		bcopy (portname,&ctlr->ql_defaults.fw_params.portName[0],8);
		ctlr->port_name = pn;
	}
	else {
		qlfc_cpmsg(ctlr->ctlr_vhdl, "NVRAM portname 0x%x is invalid.\n",pn);
	}

	/*
	** Download fw and Set init param block
	**	Note: MUST INITIALLY BE CALLED BEFORE qlfc_reset_interface.
	*/

    	if (qlfc_load_firmware (ctlr)) {
        	rc = 3;
		goto init_done;
    	}

   	/*
	 * Determine whether a SCSI bus reset is to be done
	 */
    	if (qlfc_reset_interface(ctlr, 1)) {
		rc = 4;
		goto init_done;
    	}

 init_done:
	QL_MUTEX_EXIT(ctlr);
	if (rc) {
		if (qlfc_debug >= 99) qlfc_cpmsg(ctlr->ctlr_vhdl, "qlfcInitIsp failed (%d)\n", rc);
	}
   	return(rc);
}


#ifdef QL_SRAM_PARITY
/* Function:    qlfc_enable_sram_parity()
**
** Description: Enable sram parity checking.
** initialize the sram if requested to
*/
static void
qlfc_enable_sram_parity (QLFC_CONTROLLER *ctlr, int init_sram)
{

init_sram = init_sram;
ctlr = ctlr;
#if 0

        uint16_t mbox_sts[MBOX_REGS];
    	CONTROLLER_REGS *isp = (CONTROLLER_REGS *)ctlr->base_address;

    	QL_PCI_OUTH(&isp->hccr,
		HCCR_SRAM_PARITY_ENABLE |
		HCCR_SRAM_PARITY_BANK2  |
		HCCR_SRAM_PARITY_BANK1  |
		HCCR_SRAM_PARITY_BANK0);

	if(init_sram) {

    		QL_PCI_OUTH(&isp->hccr, HCCR_CMD_PAUSE);
		flushbus();

		QL_PCI_OUTH(&isp->r3,0xabcd);				 /* data pattern */
       		QL_PCI_OUTH(&isp->r4,0x8000); 				 /* no. of words to initialize */
       	 	QL_PCI_OUTH(&isp->r8,0x0016);				 /* start addr of routine */
       	 	QL_PCI_OUTH(&isp->r9,0x0449);				 /* exit point after mailbox */
        	QL_PCI_OUTH(&isp->rar1,0x1000);				 /* first address to clear memory */
        	QL_PCI_OUTH(&isp->control_dma_address_counter_0,0x43a4); /* movb r3,[rar1++]	; clear memory */
       	 	QL_PCI_OUTH(&isp->control_dma_address_counter_1,0x8421); /* dec	 r4		; decrement counter */
        	QL_PCI_OUTH(&isp->control_dma_address_counter_2,0x08c2); /* jnz	 [r8]		; JIF not done with all addresses */
        	QL_PCI_OUTH(&isp->control_dma_address_counter_3,0x097a); /* jmp	 [r9]		; goto Exit point when done */
		flushbus();
    		QL_PCI_OUTH(&isp->hccr, HCCR_CMD_RELEASE);
		flushbus();

        	if (qlfc_mbox_cmd(ctlr, mbox_sts, 2, 1,
				MBOX_CMD_EXECUTE_FIRMWARE,
				0x0016, /* start of program */
				0,0,0,0,0,0)) {
#ifdef DEBUG
			cmn_err(CE_WARN,
				"qlfcInitIsp - EXECUTE FIRMWARE Command Failed");
#endif
        	}
	}
#endif

}
#endif /* QL_SRAM_PARITY */


/* FILE: genesis.c */
/********************************************************************************
*										*
*	qlfc_init								*
*										*
*	Performs software initialization in prior to executing the driver.	*
*										*
*	Entry:									*
*		Called during system initialization by xxxx			*
*										*
*	Input:									*
*										*
*	Output:									*
*		Registers qlfc_ as a PCI driver to be notified for qLogic	*
*		fibre channel adapters.						*
*										*
*	Notes:									*
*		Registers for qLogic devices.  Using wildcard registration	*
*		for all qLogic controllers results in memory corruption and	*
*		system panics.  Don't do this!!!!!				*
*										*
********************************************************************************/

void
qlfc_init(void)
{
	INITLOCK (&qlfc_attach_semaphore, "qlfctach", 0);
	pciio_driver_register (QLOGIC_VENDOR_ID, QLOGIC_2200, "qlfc_", 0);
#ifdef QL2100
	pciio_driver_register (QLOGIC_VENDOR_ID, QLOGIC_2100, "qlfc_", 0);
#endif
}

/********************************************************************************
*										*
*	qlfc_setup_pci								*
*										*
*	Called to attach a supported controller to the infrastructure.		*
*										*
*	Entry:									*
*		Called by qlfcAttach().						*
*										*
*	Input:									*
*		ctlr - pointer to the controller data structure			*
*										*
*	Output:									*
*										*
*	Notes:									*
*										*
********************************************************************************/

static void
qlfc_setup_pci ( QLFC_CONTROLLER *ctlr )
{
	CONTROLLER_REGS	*mem_addr;
	pciio_piomap_t	rmap = 0;
	device_desc_t   qlfc_dev_desc;
	uint16_t	command;
	int		ctlr_which = 0;	/* for now */
	int		count0 = 3;
	int		count1 = 0;

	/*
	** mem_addr winds up in the host adapter structure
	*/

	mem_addr = (CONTROLLER_REGS *) pciio_pio_addr(
					ctlr->pci_vhdl,
					0,
					PCIIO_SPACE_WIN(WINDOW),
					0,
					sizeof(*mem_addr),
					&rmap,
					0);

#if SN || IP30
	if (pcibr_rrb_alloc(ctlr->pci_vhdl, &count0, &count1) < 0)
		cmn_err(CE_PANIC, "qlfc: Unable to get Bridge RRB's");

	TRACE (ctlr, "RRBALLOC: requested 3/0, got %d/%d\n",count0,count1,0,0,0,0);
#endif

	/*
	** set up the adapter to map its registers in the appropriate address space
	*/

	command = (uint16_t) pciio_config_get (ctlr->pci_vhdl, PCI_CFG_COMMAND, sizeof(uint16_t));

	if (!(command & ADDRESS_SPACE_TO_ENABLE)) {

		pciio_config_set (
			ctlr->pci_vhdl,
			PCI_CFG_COMMAND,
			sizeof(uint16_t),
			(uint64_t)(command | ADDRESS_SPACE_TO_ENABLE));

	}

	/*
	** Establish the latency timer and the cache line size.
	**
	**	Note: may need to increase cache line size if controller
	**	does anything "funny" during dma transactions.
	*/

	pciio_config_set (ctlr->pci_vhdl, PCI_CFG_LATENCY_TIMER, sizeof(uint8_t), (uint64_t) 240);
	pciio_config_set (ctlr->pci_vhdl, PCI_CFG_CACHE_LINE, sizeof(uint8_t), (uint64_t) 128);

	ctlr->revision = (uint8_t) pciio_config_get (ctlr->pci_vhdl, PCI_CFG_REV_ID, sizeof(uint8_t));

	ctlr->bridge_revnum = pcibr_asic_rev (ctlr->pci_vhdl);


	qlfc_dev_desc = device_desc_dup(ctlr->pci_vhdl);
	device_desc_intr_name_set(qlfc_dev_desc, QLOGIC_DEV_DESC_NAME);
	device_desc_default_set(ctlr->pci_vhdl,qlfc_dev_desc);

	ctlr->base_address = mem_addr;
	ctlr->ctlr_which = ctlr_which;

	qlfc_disable_intrs (ctlr);

	TRACE (ctlr, "qlfc_setup_pci: exit\n",0,0,0,0,0,0);

	return;
}


/********************************************************************************
*										*
*	qlfcAttach								*
*										*
*	Called to attach a supported controller to the infrastructure.		*
*										*
*	Entry:									*
*		Called by qlfc_attach().					*
*										*
*	Input:									*
*		pci_vhdl - vertex handle of the qLogic fibre channel adapter.	*
*		ctlr_number - count of controlers encountered so far		*
*		ctlr_index - index into controller type id array		*
*										*
*	Output:									*
*										*
*	Notes:									*
*										*
********************************************************************************/

static void
qlfcAttach (vertex_hdl_t pci_vhdl, int ctlr_number, int ctlr_index)
{
	int inventory_type = INV_QL;
	int disks_found=0;
	QLFC_CONTROLLER *ctlr;

	ctlr = (QLFC_CONTROLLER *) kmem_zalloc (
				sizeof(QLFC_CONTROLLER),
				VM_CACHEALIGN | VM_NOSLEEP | VM_DIRECT);

	if (ctlr) {
		mutex_lock (&qlfc_attach_semaphore, PRIBIO);
		if (!qlfc_controllers) {
			qlfc_controllers = ctlr;
		}
		else {
			qlfc_last_controller->next = ctlr;
		}
		qlfc_last_controller = ctlr;
		ctlr->next = NULL;

		mutex_unlock (&qlfc_attach_semaphore);

		ctlr->ctlr_number = ctlr_number;
		ctlr->pci_vhdl = pci_vhdl;
		ctlr->ctlr_type_index = ctlr_index;

		/*
		** Initialize the eye catchers.
		*/
		strncpy (ctlr->demon_queue_ec,		"dmnque",	8);
		strncpy (ctlr->mutex_ec,		"mutex",	8);
		strncpy (ctlr->trace_ec,		"trace",	8);
		strncpy (ctlr->demon_ec,		"demon",	8);
		strncpy (ctlr->timeout_ec,		"timeout",	8);
		strncpy (ctlr->stats_ec,		"stats",	8);
		strncpy (ctlr->alen_ec,			"alenlst",	8);
		strncpy (ctlr->defaults_ec,		"default",	8);
		strncpy (ctlr->target_map_ec,		"targmap",	8);
		strncpy (ctlr->queues_ec,		"queue",	8);
		strncpy (ctlr->dmamap_ec,		"dmamap",	8);
		strncpy (ctlr->queue_depth_ec,		"q_depth",	8);
		strncpy (ctlr->dma_ptrs_ec,		"dmaptrs",	8);
		strncpy (ctlr->controller_ec,		"ctlr",		8);
		strncpy (ctlr->controller_regs_ec,	"ctlrreg",	8);
		strncpy (ctlr->quiesce_ec,		"quiesce",	8);
		strncpy (ctlr->flags_ec,		"flags",	8);
		strncpy (ctlr->reqrsp_ptrs_ec,		"req_rsp",	8);

		/*
		** Set a default name into the hwgname buffer.
		*/

		sprintf (ctlr->hwgname, "qlfc%d", ctlr_number);

		/*
		** Initialize the trace buffer.
		*/

		trace_init (ctlr);
		TRACE (ctlr, "Initializing controller, index %d\n",ctlr->ctlr_number,0,0,0,0,0);

		/*
		** Reserve dedicated target map entries.
		*/

		ctlr->target_map[QL_FABRIC_FL_PORT  ].tm_flags |= TM_RESERVED;
		ctlr->target_map[QL_FABRIC_RESERVED1].tm_flags |= TM_RESERVED;
		ctlr->target_map[QL_FABRIC_RESERVED2].tm_flags |= TM_RESERVED;
		ctlr->target_map[QL_FABRIC_RESERVED3].tm_flags |= TM_RESERVED;

		qlfc_setup_pci (ctlr);
		qlfc_controller_add (ctlr);

		if (qLogicDeviceIds[ctlr->ctlr_type_index] == QLOGIC_2200) {
			inventory_type = INV_QL_2200;
		}
#ifdef QL2100
		else if (qLogicDeviceIds[ctlr->ctlr_type_index] == QLOGIC_2100) {
			inventory_type = INV_QL_2100;
		}
#endif

		device_inventory_add (ctlr->ctlr_vhdl, INV_DISK, INV_SCSICONTROL, -1, 0, inventory_type);

		sthread_create ("qlfc_timer",0,0,0,
			-10,	/* some priority, TBD later */
			KT_PS,	/* priority scheduled */
			(st_func_t *)qlfc_watchdog_timer,
			(void *)ctlr, (void *)NULL, (void *)NULL, (void *)NULL);

		if (qlfcInit (ctlr) == ERROR) return;

		if (ctlr->flags & CFLG_INITIALIZED) {
			disks_found = qlfc_probe_bus(ctlr);
		}

		qlfc_cpmsg(ctlr->ctlr_vhdl, "%d FCP targets.\n",disks_found);
	}
}



/********************************************************************************
*										*
*	qlfc_attach								*
*										*
*	Attach routine called when a qLogic host adapter is located.		*
*										*
*	Entry:									*
*		Called during system initialization by xxxx			*
*										*
*	Input:									*
*		pci_vhdl - the vertex handle of the qLogic host adapter.	*
*										*
*	Output:									*
*		returns:							*
*			 0 - okay						*
*			-1 - error						*
*										*
*	Notes:									*
*										*
********************************************************************************/

int
qlfc_attach (vertex_hdl_t pci_vhdl)
{
	int		device_id;
	int		i;
	int		rc = -1;
	int		ctlr_number;

	/*
	** Determine if this particular device is one of the supported
	** qLogic FC controllers.
	*/

	device_id = (int) pciio_config_get (pci_vhdl, PCI_CFG_DEVICE_ID, sizeof(uint16_t));

	for (i=0; ; i++) {
		if (device_id == qLogicDeviceIds[i]) {

			ctlr_number = atomicAddInt (&qlfc_ctlr_number,1);

			/*
			** Attach the device.
			*/

			qlfcAttach (pci_vhdl, ctlr_number, i);
			rc = 0;

			break;

		}
		if (qLogicDeviceIds[i] == 0xffff) break;
	}

	return rc;
}

/********************************************************************************
*										*
*	qlfc_detach								*
*										*
*	Attach routine called when a qLogic fibre channel adapter is to be	*
*	"removed".								*
*										*
*	Entry:									*
*		Called by kernel by xxxx					*
*										*
*	Input:									*
*		pp - pci_record structure					*
*		*e - an edt_t							*
*		reason - some reason						*
*										*
*	Output:									*
*		returns:							*
*			0 - okay						*
*										*
*	Notes:									*
*		Functionality is not really implemented.			*
*										*
********************************************************************************/

int
qlfc_detach (struct pci_record *pp, edt_t *e, __int32_t reason)
{
	/* XXX- when detaching, we must piomap_free cmap and rmap if
	 * they are not NULL, or we will have a leak of piomap
	 * resources.  */
	printf("qlfc_detach: does nothing - pci_record %x edt %x reason %x\n",
		pp, e, reason);
	/* remove driver and instance of boards in system */

	atomicClearInt (&qlfc_ctlr_number,(int)-1);

	return(0);
}


/* FILE: stop.c */
int
qlfc_stop_risc (QLFC_CONTROLLER *ctlr)
{

	int rc=0;
	int i;
	int retry_count;
	uint16_t mailbox0;

	CONTROLLER_REGS *isp = (CONTROLLER_REGS *)ctlr->base_address;

	TRACE (ctlr, "disabling interrupts\n",0,0,0,0,0,0);
	QL_PCI_OUTH(&isp->icr, ICR_DISABLE_ALL_INTS); /* disable interrupts */

	/*
	** Pause RISC and wait till it's paused.
	*/

	TRACE (ctlr,"pausing risc\n",0,0,0,0,0,0);
	QL_PCI_OUTH(&isp->hccr, HCCR_CMD_PAUSE);
	flushbus();
	for (retry_count = 10000; ; ) {
		DELAY(100);	/* .1 ms */
		if (QL_PCI_INH(&isp->hccr) & HCCR_PAUSE)
			break;
		if (--retry_count <= 0) {
			rc = 1;
			goto done;
		}
	}

	/*
	** Reset ISP and wait till it's reset.
	*/

	TRACE (ctlr,"performing soft reset\n",0,0,0,0,0,0);
	QL_PCI_OUTH(&isp->icsr, ICSR_SOFT_RESET);
	DELAYBUS(50);

	for (retry_count = 10000; ; ) {
		int hccr;
		hccr = QL_PCI_INH(&isp->hccr);
		if (!(hccr & HCCR_RESET)) break;
		DELAY(100);	/* .1 ms */
		if (--retry_count <= 0) {
			rc = 2;
			goto done;
		}
	}

	for (i = 0;i < QL_RESET; i++) {
		mailbox0 = QL_PCI_INH(&isp->mailbox0);
		TRACE (ctlr,"checking mbox 0 for busy (0x%x) status: 0x%x\n",MBOX_STS_BUSY,mailbox0,0,0,0,0);
		if (mailbox0 != MBOX_STS_BUSY) break;
		DELAY(1000);	/* 1 ms */
	}

	DELAY (100);

	if (mailbox0 == MBOX_STS_BUSY) rc = 3;

done:
	return rc;
}

/* FILE: loadFirmware.c */
static int
qlfc_load_firmware (QLFC_CONTROLLER *ctlr)
{

	qLogicFirmware_t	*fw;
	uint16_t		mbox_sts[MBOX_REGS];
	CONTROLLER_REGS		*isp = (CONTROLLER_REGS *)ctlr->base_address;
	int			count;
	uint16_t 		*w_ptr;
	uint16_t		checksum;
	int			rc;
	uint8_t			*fw_version;

	atomicClearInt(&ctlr->flags, CFLG_INITIALIZED);

	rc = qlfc_stop_risc (ctlr);
	if (rc) goto download_failed;

	TRACE (ctlr, "LOAD FW :setting hccr reset\n",0,0,0,0,0,0);
	QL_PCI_OUTH(&isp->hccr, HCCR_CMD_RESET);
	flushbus();
	DELAY(10);

	TRACE (ctlr, "LOAD FW :releasing isp\n",0,0,0,0,0,0);
	QL_PCI_OUTH(&isp->hccr, HCCR_CMD_RELEASE);
	DELAYBUS(50);

	qlfc_enable_sram_parity (ctlr,1);

	qlfc_enable_intrs (ctlr);

	/*
	** Read the risc mailbox registers. 
	**
	**				 2200				 2100
	**	--------------------------------------------------------------
	**	mbox 0 -		0x0000 (rdy), 0x0004 (busy)	0x0000
	**	mbox 1 - 	'IS'	0x4953				0x4953
	**	mbox 2 -	'P '	0x5020				0x5020
	**	mbox 3 -	'  '	0x2020				0x2020
	**	mbox 4 -		0x0001	ISP rev level		0x0001
	**	mbox 5 -		0x0001	RISC rev level		0x0008
	**	mbox 6 -		0x0404	FB/FPM rev levels	0x0303
	**	mbox 7 -		0x0002	RISC ROM rev level	0x0007
	*/

	mbox_sts[7] = ctlr->mailbox[7] = QL_PCI_INH(&isp->mailbox7);
	mbox_sts[6] = ctlr->mailbox[6] = QL_PCI_INH(&isp->mailbox6);
	mbox_sts[5] = ctlr->mailbox[5] = QL_PCI_INH(&isp->mailbox5);
	mbox_sts[4] = ctlr->mailbox[4] = QL_PCI_INH(&isp->mailbox4);
	mbox_sts[3] = ctlr->mailbox[3] = QL_PCI_INH(&isp->mailbox3);
	mbox_sts[2] = ctlr->mailbox[2] = QL_PCI_INH(&isp->mailbox2);
	mbox_sts[1] = ctlr->mailbox[1] = QL_PCI_INH(&isp->mailbox1);
	mbox_sts[0] = ctlr->mailbox[0] = QL_PCI_INH(&isp->mailbox0);

	for (count=0; count < MBOX_REGS; count++) {
		TRACE (ctlr,"LOAD FW :mbox %d after reset: 0x%x\n",count,mbox_sts[count],0,0,0,0);
	}

	/*
	** At this point, the RISC is running, but not any loaded firmware.
	** We can now either load new firmware or restart the previously loaded
	** firmware. In this case, we load firmware!
	*/

	if (qLogicDeviceIds[ctlr->ctlr_type_index] == QLOGIC_2200) {
		if (mbox_sts[7] == 0x0002) {	/* new controller type */
			TRACE (ctlr,"LOAD FW :QL2200 detected via mbox registers.\n",0,0,0,0,0,0);
		}
	}

	ctlr->qlfw = fw = &ql_firmware[ctlr->ctlr_type_index];
	if (*(fw->extended_lun)) {
		atomicSetInt (&ctlr->flags, CFLG_EXTENDED_LUN);
	}

	fw_version = fw->firmware_version;

	qlfc_cpmsg(ctlr->ctlr_vhdl, "Firmware version: %d.%d.%d\n",
		(int) *(fw_version+0),
		(int) *(fw_version+1),
		(int) *(fw_version+2));


	/*
	** Load executable firmware one word at a time, computing
	** checksum as we go.
	*/

	w_ptr = fw->risc_code01;

	TRACE (ctlr,"LOAD FW :fw length 0x%x, fw addr 0x%x, fw source addr 0x%x\n",
		*(fw->risc_code_length01),
		*(fw->risc_code_addr01),
		(__psint_t)w_ptr,
		0,0,0);

	checksum = 0;
	for (count = 0; count < *(fw->risc_code_length01); count++) {
		if (count >= 8) trace_off (ctlr);
		if (qlfc_mbox_cmd(ctlr, mbox_sts, 3, 3,
				MBOX_CMD_WRITE_RAM_WORD,
				(uint16_t)(*(fw->risc_code_addr01) + count),  /* risc addr */
				*w_ptr,				      /* risc data */
				0,0,0,0,0))
		{
			rc = 4;
			trace_on (ctlr);
			goto download_failed;
		}
		checksum += *w_ptr++;
	}

	trace_on (ctlr);

	/* Start ISP firmware.  */
	if (qlfc_mbox_cmd(ctlr, mbox_sts, 2, 1,
			MBOX_CMD_EXECUTE_FIRMWARE,
			*(fw->risc_code_addr01),
			0,0,0,0,0,0))
	{
		rc = 5;
		goto download_failed;
	}

	return(0);

download_failed:
	TRACE (ctlr, "LOAD FW :Firmware load failure, code %d.\n",rc,0,0,0,0,0);
	qlfc_cpmsg(ctlr->ctlr_vhdl, "qlfc_load_firmware failed (%d)\n", rc);
	return(rc);
}

/* FILE: reset.c */
#ifdef IP30
/*
 * IP30 calls qlfc_reset() on ac power fail interrupt
 */
int
qlfc_reset(vertex_hdl_t qlfc_ctlr_vhdl)
{
    	QLFC_CONTROLLER	*ctlr = qlfc_ctlr_info_from_ctlr_get (qlfc_ctlr_vhdl);
	qlfc_stop_risc (ctlr);
	return 0;
}
#endif

static STATUS
qlfc_reset_interface(QLFC_CONTROLLER *ctlr, int reset)
{
	uint16_t		mbox_sts[MBOX_REGS];
	CONTROLLER_REGS		*isp = (CONTROLLER_REGS *)ctlr->base_address;
	int			i;
	qlfc_local_lun_info_t	*qli;
	int			rc = OK;

	uint16_t		risc_code_addr01 =
					*(ql_firmware[ctlr->ctlr_type_index].risc_code_addr01);

	init_fw_param_block	*p_initblock;
   	Default_parameters	*n_ptr = &ctlr->ql_defaults;
	u_char			timeout, timecount, cable;
	status_entry		*status;
	qlfc_local_targ_info_t	*qti;

	TRACE (ctlr, "Resetting controller, %d, rsp q depth %d.\n",reset,
		ctlr->ql_response_queue_depth,0,0,0,0);

       /*
	*  reset CFLG_INITIALIZED for adapter
        *  reset DFLG_INITIALIZED for all attached devices
	*  reset DFLG_CONTINGENT_ALLEGIANCE for all attached devices
        *  disable interrupt
        *  pause RISC
        *  reset ISP
        *  clear the CMD & DATA DMA chan
        *  wait for risc reset
        */

	atomicClearInt(&ctlr->flags, CFLG_INITIALIZED);

	for (i = 0; i < QL_MAX_LOOP_IDS; i++) {

		mutex_lock (&ctlr->target_map[i].tm_mutex,PRIBIO);
		qti = ctlr->target_map[i].tm_qti;
		if (qti) {

			qli = qti->local_lun_info;
			while (qli) {
				TRACE (ctlr,"RESET   : clearing dev flags for %d/%d\n",qti->target,qli->qli_lun,0,0,0,0);
				atomicClearInt (&qli->qli_dev_flags, (DFLG_INITIALIZED | DFLG_CONTINGENT_ALLEGIANCE));
				qli = qli->next;
			}
		}
		mutex_unlock (&ctlr->target_map[i].tm_mutex);
	}

	ctlr->queue_space = 0;
	ctlr->request_in  = 0;
	ctlr->request_out = 0;
	ctlr->response_out = 0;
	ctlr->request_ptr = ctlr->request_base;
	ctlr->response_ptr = ctlr->response_base;

	/*
	 * Mark all responses as dead.
	 */


	for (status = (status_entry *)ctlr->response_base, i = 0;
		i < ctlr->ql_response_queue_depth; ) {

			status->handle = 0xDEAD;
			i++;
			status = (status_entry *)(ctlr->response_base+(i*IOCB_SIZE));
	}

	if (reset) {			/* actually want to stop and restart firmware */
		rc = qlfc_stop_risc (ctlr);
		if (rc) goto reset_failed;

		qlfc_enable_sram_parity (ctlr,0);

		TRACE (ctlr, "RESET   : releasing lock semaphore\n",0,0,0,0,0,0);
		QL_PCI_OUTH (&isp->bus_sema, ISEM_RELEASE);
		DELAYBUS(50);

		TRACE (ctlr, "RESET   : releasing processer\n",0,0,0,0,0,0);
		QL_PCI_OUTH (&isp->hccr, HCCR_CMD_RELEASE);
		DELAYBUS(50);

		/* At this point, the firmware is up and running and we're ready to */
		/* initialze for normal startup.  If you are bringing up the ISP */
		/* device for the first time, leave normal_startup undefined until */
		/* you have worked out the bugs of getting the firmware loaded and */
		/* running.  */

		TRACE (ctlr,"RESET   : enabling interrupts\n",0,0,0,0,0,0);
		qlfc_enable_intrs (ctlr);

		/* Start ISP firmware.  */
		TRACE (ctlr,"RESET   : starting firmware at address 0x%x\n",risc_code_addr01,0,0,0,0,0);
		if (qlfc_mbox_cmd (ctlr, mbox_sts, 2, 1,
				MBOX_CMD_EXECUTE_FIRMWARE,
				risc_code_addr01,
				0,0,0,0,0,0)) {
			rc = 13;
			goto reset_failed;
		}
	}

	/*
	** Make sure fimrware is executing. This command will
	** return good status only if
        ** firmware is loaded and executing properly. State
	** should be zero.
	*/

	TRACE (ctlr,"RESET   : getting firmware state\n",0,0,0,0,0,0);
	if (qlfc_mbox_cmd(ctlr, mbox_sts, 1, 2,
			MBOX_CMD_GET_FIRMWARE_STATE,
			0,0,0,0,0,0,0)) {
		rc = 14;
		goto reset_failed;
        }
	TRACE (ctlr,"RESET   : Firmware State = %x \n", mbox_sts[1],0,0,0,0,0);

	/*
	** Get Physical Address for Response Queue.
	*/

	if (ctlr->response_dmaptr == NULL) {
		ctlr->response_dmamap = pciio_dmamap_alloc(ctlr->pci_vhdl, NULL,
							 ctlr->response_size,
							 PCIIO_DMA_CMD |
							 PCIIO_WORD_VALUES);
		ctlr->response_dmaptr = pciio_dmamap_addr(ctlr->response_dmamap, kvtophys(ctlr->response_base),
							 ctlr->response_size);

		ctlr->response_misc_dmaptr = ctlr->response_dmaptr
					   + (IOCB_SIZE*ctlr->ql_response_queue_depth);

		TRACE (ctlr,"MAPADDR : response_dmaptr 0x%x, response misc dmaptr 0x%x\n",
			(__psint_t)ctlr->response_dmaptr,
			(__psint_t)ctlr->response_misc_dmaptr,0,0,0,0);
	}

	bzero(ctlr->response_base,ctlr->ql_response_queue_depth*IOCB_SIZE);

	for (i=0; i<ctlr->ql_response_queue_depth*IOCB_SIZE; i++) *(ctlr->response_base+i) = 0xee;

#ifdef FLUSH
	MRW_DCACHE_WB_INVAL(ctlr->response_base,
		ctlr->ql_response_queue_depth*IOCB_SIZE);
#endif	/* FLUSH */


	/*
	** Get Physical Address for Request Queue.
	*/


	if (ctlr->request_dmaptr == NULL) {
		ctlr->request_dmamap = pciio_dmamap_alloc(ctlr->pci_vhdl, NULL,
							ctlr->request_size,
							PCIIO_DMA_CMD |
							PCIIO_WORD_VALUES);
		ctlr->request_dmaptr = pciio_dmamap_addr(ctlr->request_dmamap, kvtophys(ctlr->request_base),
							 ctlr->request_size);
		ctlr->request_misc_dmaptr = ctlr->request_dmaptr
					  + (IOCB_SIZE*ctlr->ql_request_queue_depth);
		TRACE (ctlr,"MAPADDR : request_dmaptr  0x%x, request misc dmaptr 0x%x\n",
				(__psint_t)ctlr->request_dmaptr,
				(__psint_t)ctlr->request_misc_dmaptr,0,0,0,0);
	}

	bzero(ctlr->request_base,ctlr->ql_request_queue_depth*IOCB_SIZE);

	/*
	**  Build Initialization Control Block in
	**  Request Queue Area
	*/

	p_initblock = (init_fw_param_block *) ctlr->request_base;

	p_initblock->version		= n_ptr->fw_params.version;
	p_initblock->reserved0		= n_ptr->fw_params.reserved0;
	p_initblock->fwOption0		= n_ptr->fw_params.fwOption0.byte;
	p_initblock->fwOption1		= n_ptr->fw_params.fwOption1.byte;

	p_initblock->maxFrameSize	= n_ptr->fw_params.maxFrameSize;
	p_initblock->maxIOCBperPort	= n_ptr->fw_params.maxIOCBperPort;
	p_initblock->executionThrottle	= n_ptr->fw_params.executionThrottle;
	p_initblock->retryCount		= n_ptr->fw_params.retryCount;
	p_initblock->retryDelay		= n_ptr->fw_params.retryDelay;

	p_initblock->portName0		= n_ptr->fw_params.portName[0];
	p_initblock->portName1		= n_ptr->fw_params.portName[1];
	p_initblock->portName2		= n_ptr->fw_params.portName[2];
	p_initblock->portName3		= n_ptr->fw_params.portName[3];

	p_initblock->portName4		= n_ptr->fw_params.portName[4];
	p_initblock->portName5		= n_ptr->fw_params.portName[5];
	p_initblock->portName6		= n_ptr->fw_params.portName[6];
	p_initblock->portName7		= n_ptr->fw_params.portName[7];

	p_initblock->hardID		= n_ptr->fw_params.hardID;
	p_initblock->reserved1		= n_ptr->fw_params.reserved1;
	p_initblock->inquiry_data	= n_ptr->fw_params.inquiry_data;
	p_initblock->loginTimeout	= n_ptr->fw_params.loginTimeout;

	p_initblock->nodeName7   	= n_ptr->fw_params.nodeName[7];
	p_initblock->nodeName6   	= n_ptr->fw_params.nodeName[6];
	p_initblock->nodeName5   	= n_ptr->fw_params.nodeName[5];
	p_initblock->nodeName4   	= n_ptr->fw_params.nodeName[4];
	p_initblock->nodeName3   	= n_ptr->fw_params.nodeName[3];
	p_initblock->nodeName2   	= n_ptr->fw_params.nodeName[2];
	p_initblock->nodeName1   	= n_ptr->fw_params.nodeName[1];
	p_initblock->nodeName0   	= n_ptr->fw_params.nodeName[0];

	p_initblock->request_out	= ctlr->request_out;
	p_initblock->response_in	= ctlr->response_out;

	p_initblock->requestQ_depth	= ctlr->ql_request_queue_depth;
	p_initblock->responseQ_depth	= ctlr->ql_response_queue_depth;

	p_initblock->requestQ_base_addrlow	= ctlr->request_dmaptr & 0xffffffff;
	p_initblock->requestQ_base_addrhigh	= ctlr->request_dmaptr >> 32;

	p_initblock->responseQ_base_addrlow	= ctlr->response_dmaptr & 0xffffffff;
	p_initblock->responseQ_base_addrhigh	= ctlr->response_dmaptr >> 32;

	p_initblock->reserved4				= 0;
	p_initblock->reserved5a				= 0;
	bzero (&p_initblock->reserved5b,sizeof(p_initblock->reserved5b));

	/*
	** Extended control block options only.  Ignored if extendedCB not set.
	*/

	/* target driver only, zero here */
	p_initblock->lun_timeout			= 0;
	p_initblock->immediate_notify_resource_count	= 0;
	p_initblock->command_resource_count		= 0;
	p_initblock->lun_enables			= 0;

	/* interrupt batching */
	p_initblock->interrupt_delay_timer		= 0;
	p_initblock->response_accumulation_timer	= 0;

	p_initblock->additional_fw_opts.reserved			= 0;
	p_initblock->additional_fw_opts.enableReadXFR_RDY		= 0;
	p_initblock->additional_fw_opts.nonParticipateHardAddressFail	= 1;

	/* class 2 support */
	p_initblock->additional_fw_opts.enableClass2			= 0;
	p_initblock->additional_fw_opts.enableACK0			= 0;

	/* fc-tape support */
	p_initblock->additional_fw_opts.enableFCTape			= 0;
	p_initblock->additional_fw_opts.enableCommandReferenceNumber	= 0;
	p_initblock->additional_fw_opts.enableFibreChannelConfirm	= 0;

	/* connectionOptions: 0, loop only; 1, pt-pt only; 2, loop preferred; 3 pt-pt preferred */

	p_initblock->additional_fw_opts.connectionOptions		= qlfc_use_connection_mode;
	p_initblock->additional_fw_opts.operationMode			= 0;


#ifdef FLUSH
	MRW_DCACHE_WB_INVAL(ctlr->request_base, IOCB_SIZE);
#endif	/* FLUSH */


	/* Send Initialization Control Block */

	TRACE (ctlr,"RESET   : sending init control block: 0x%x, responseQ_base_addrlow 0x%x, high 0x%x\n",
		(__psint_t)ctlr->request_dmaptr,
		p_initblock->responseQ_base_addrlow,
		p_initblock->responseQ_base_addrhigh,
		0,0,0);

	if (qlfc_mbox_cmd(ctlr, mbox_sts, 8, 6,
			MBOX_CMD_INITIALIZE_FIRMWARE,
			0,
			(uint16_t)((u_long)ctlr->request_dmaptr >> 16),
			(uint16_t)((u_long)ctlr->request_dmaptr & 0xFFFF),
			ctlr->request_in,
			ctlr->request_out,
			(uint16_t) ((u_long)ctlr->request_dmaptr >> 48),
			(uint16_t) ((u_long)ctlr->request_dmaptr >> 32)))
	{
		TRACE (ctlr, "RESET   : initialize firmware failure, 0x%x, 0x%x, 0x%x.\n",
			mbox_sts[0],mbox_sts[4],mbox_sts[5],0,0,0);
		rc = 15;
		goto reset_failed;
	}

	bzero(ctlr->request_base,ctlr->ql_request_queue_depth*IOCB_SIZE);


	/*
	** Wait for upto loginTimeout to get f/w state to be ready.
	** if timeout is value is zero assume 2 *RTOV = 4 seconds.
	*/

	timeout = n_ptr->fw_params.loginTimeout;
	if (timeout == 0)
		timeout = 4;

	for (timecount = 0, cable = 1; timecount < 15; timecount++) {

	        /* After 7 seconds, break out if still no cable detected */

		if (timecount == 7 && cable == 0) {
			rc = 17;
			goto reset_failed;
		}

		for (i = 0; i < 100; i++) {		/* 10 loops per second */
			/* Wait for a tenth of a sec */
			delay (HZ/100);
			/* Check for Lip Complete */
			if (qlfc_mbox_cmd(ctlr, mbox_sts, 1, 2,
				MBOX_CMD_GET_FIRMWARE_STATE,
				0,0,0,0,0,0,0)) {

				rc = 16;
				goto reset_failed;
			}

			TRACE (ctlr, "RESET   : fw state %d, connection status %d\n",
				(__psint_t)mbox_sts[1],
				(__psint_t)mbox_sts[2],
				0,0,0,0);

			if (mbox_sts[1] == FW_READY) {
				goto LipDone;
			}
			else if (mbox_sts[1] == FW_LOSS_OF_SYNC) {
				cable = 0;
			}
			else {
				cable = 1;
			}
		}
	}


LipDone:
	trace_on (ctlr);
	/* Indicate we successfully initialized this ISP! */
 	atomicSetInt(&ctlr->flags, CFLG_INITIALIZED);
	TRACE (ctlr,"RESET   : interface reset okay\n",0,0,0,0,0,0);
	return OK;

reset_failed:
	trace_on (ctlr);
	TRACE (ctlr,"RESET   : interface reset failure: %d\n",rc,0,0,0,0,0);
	qlfc_mbox_cmd(ctlr, mbox_sts, 1, 8,
			MBOX_CMD_STOP_FIRMWARE,
			0,0,0,0,0,0,0);

	if (qlfc_debug >= 99)
		qlfc_cpmsg(ctlr->ctlr_vhdl, "qlfc_reset_interface failed (%d)\n", rc);
	return rc;
}


static STATUS
qlfc_do_reset (vertex_hdl_t qlfc_ctlr_vhdl)
{
	STATUS		rc;
	int		drain_loops;
    	QLFC_CONTROLLER	*ctlr = qlfc_ctlr_info_from_ctlr_get (qlfc_ctlr_vhdl);

	if (ctlr == NULL) return ERROR;	/* fail */

	atomicClearInt (&ctlr->flags, CFLG_SHUTDOWN);

	qlfc_stop_queues (ctlr);
	
	drain_loops = 5;
	while (ctlr->ql_ncmd && drain_loops) {
		delay (HZ);
		drain_loops --;
	}

	if (ctlr->flags & CFLG_LINK_MODE) {
		qlfc_logout_fabric_ports (ctlr);
	}

	QL_MUTEX_ENTER(ctlr);
	rc = qlfc_load_firmware (ctlr);
	atomicClearInt (&ctlr->flags, CFLG_ISP_PANIC);
	rc = qlfc_reset_interface(ctlr,1);
	qlfc_flush_queue(ctlr, SC_CMDTIME, ALL_QUEUES);	/* We want to retry these commands */
	QL_MUTEX_EXIT(ctlr);

	return rc;

}

static STATUS
qlfc_do_lip (vertex_hdl_t qlfc_ctlr_vhdl)
{
    	QLFC_CONTROLLER	*ctlr = qlfc_ctlr_info_from_ctlr_get (qlfc_ctlr_vhdl);
    	uint16_t	mbox_sts[MBOX_REGS];
	STATUS		rc;
	int		drain_loops;

	/* Note: may desire to remove test on LINK_MODE here, hence leaving other refs in */
    	if (ctlr == NULL || !(ctlr->flags & CFLG_INITIALIZED) || (ctlr->flags & CFLG_LINK_MODE))
		return ERROR; /*fail*/

	qlfc_demon_take_lock (ctlr);
	if (ctlr->flags & (CFLG_LIP_RESET|CFLG_STOP_PROBE)) {
		qlfc_demon_free_lock (ctlr);
		qlfc_cpmsg (qlfc_ctlr_vhdl,"Activity in progress that precludes initiating lip.  Try again later.\n");
		return ERROR;
	}
	if (!(ctlr->flags & CFLG_LINK_MODE)) {
		atomicSetInt (&ctlr->flags, CFLG_LIP_RESET|CFLG_STOP_PROBE);	/* likely will be doing probe real soon now */
	}
	else {
		atomicSetInt (&ctlr->flags, CFLG_STOP_PROBE);	/* likely will be doing probe real soon now */
	}
	qlfc_demon_free_lock (ctlr);

	while (ctlr->flags & CFLG_PROBE_ACTIVE) delay (HZ);

	qlfc_stop_queues (ctlr);		/* stop issuing */
	drain_loops = 5;
	while (ctlr->ql_ncmd && drain_loops) {
		delay (HZ);
		drain_loops --;
	}

	if (ctlr->flags & CFLG_LINK_MODE) {
		qlfc_logout_fabric_ports (ctlr);
	}

	QL_MUTEX_ENTER(ctlr);

	rc = qlfc_mbox_cmd(ctlr, mbox_sts, 2, 2, MBOX_CMD_LIP_FOLLOWED_BY_LOGIN,
			 0, 0, 0, 0, 0, 0, 0);

	QL_MUTEX_EXIT(ctlr);

    	return rc;
}

/* FILE: nv.c */
/*
 * NVRAM delay loop for timing.
 */
static void
nv_delay()
{
	us_delaybus (50);
}


/*
 * Write word to NVRAM.
 */
/* REFERENCED */
static void
write_nvram( QLFC_CONTROLLER *ctlr, uint8_t addr, uint16_t data )
{

    uint16_t   w0;
    uint32_t   ww0;

    /* Enable writes. */

    nv_write( ctlr, NV_DATA_OUT );        /* Write a one bit. */
    nv_write( ctlr, 0 );          /* Write two zero bits. */
    nv_write( ctlr, 0 );
    for( w0=0; w0 < NV_NUM_ADDR_BITS; w0++ )       /* Write 2 one bits and */
    nv_write( ctlr, NV_DATA_OUT );                       /* 4 don't care bits. */
    nv_deselect(ctlr);                                 /* Deselect chip to start write. */

    /* Erase location. */

    ww0 = NV_ERASE_OP | addr;
    ww0 <<= 16;
    nv_cmd( ctlr, 3+NV_NUM_ADDR_BITS, ww0 );
    nv_rdy(ctlr);

    /* Write data to location. */

    ww0 = NV_WRITE_OP | addr;
    ww0 <<= 16;
    ww0 |= data;
    nv_cmd( ctlr, 3+NV_NUM_ADDR_BITS+16, ww0 );
    nv_rdy(ctlr);

    /* Disable writes. */

    nv_write( ctlr, NV_DATA_OUT );        /* Write a one bit. */
    for( w0=0; w0 < NV_NUM_ADDR_BITS+2; w0++ )       /* Write 4 zeros bits and */
    nv_write( ctlr, 0 );          /* 4 don't care bits. */

    nv_deselect(ctlr);          /* Deselect chip. */
}

/*
 * Read word from NVRAM.
 */
static uint16_t
read_nvram( QLFC_CONTROLLER *ctlr, uint8_t addr )
{
    uint32_t ww0;
    uint16_t w0, w1, w2;

    ww0 = NV_READ_OP | addr;
    ww0 <<= 16;
    nv_cmd( ctlr, 3+NV_NUM_ADDR_BITS, ww0 );
    for( w1=w2=0; w2 < 16; w2++ )
    {
        w0 = NV_SELECT | NV_CLOCK;  /* Select and clock chip. */
        QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
        nv_delay();         /* Go delay for chip. */
        w1 <<= 1;           /* Shift in zero bit. */
        w0 = QL_PCI_INH ( &ctlr->base_address->isp_nvram );    /* Get register data. */
        if ( w0 & NV_DATA_IN )      /* Test for one bit. */
           w1 |= 1;            /* Set it in data word. */
        w0 = NV_SELECT;         /* Remove clock. */
        QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
        nv_delay();         /* Go delay for chip. */
    }
    nv_deselect(ctlr);          /* Deselect chip. */
    return( w1 );
}


/*
 * Wait for erase/write to finish.
 */
static void
nv_rdy( QLFC_CONTROLLER *ctlr )
{
    uint16_t w0;

    nv_deselect(ctlr);          /* Deselect chip to start it. */
    w0 = NV_SELECT;         /* Select chip. */
    QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );

    while( !(w0 & NV_DATA_IN) )     /* Wait for ready. */
    {
       nv_delay();         /* Go delay for chip. */
       w0 = QL_PCI_INH ( &ctlr->base_address->isp_nvram );    /* Get NVRAM register. */
    }

    nv_deselect(ctlr);          /* Deselect chip to start it. */
}

/*
 * Load command, address and data if desired.
 */
static void
nv_cmd( QLFC_CONTROLLER *ctlr, uint8_t count, uint32_t ww0 )
{
    uint16_t w1;

    while ( count-- )           /* Loop until all data transfered. */
    {
#ifdef SEVEN_BIT_ADDR
    if ( ww0 & 0x02000000 )     /* Test for bit in data. */
#else
    if ( ww0 & 0x04000000 )     /* Test for bit in data. */
#endif
        w1 = NV_DATA_OUT;       /* Set output bit for NVRAM. */
    else
        w1 = 0;         /* Else, zero bit. */
    nv_write( ctlr, w1 );         /* Position data for next bit. */
    ww0 <<= 1;
    }
}

/*
 * NVRAM deselect.
 */
static void
nv_deselect( QLFC_CONTROLLER *ctlr )
{
   uint16_t w0;

    w0 = NV_DESELECT;           /* Deselect chip. */
    QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
    nv_delay();             /* Go delay for chip. */
}

/*
 * Write and clock bit to NVRAM.
 */
static void
nv_write( QLFC_CONTROLLER *ctlr, uint16_t w0 )
{
    w0 |= NV_SELECT;            /* Add in chip select. */
    QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
    nv_delay();             /* Go delay for chip. */
    w0 |= NV_CLOCK;         /* Set clock for data. */
    QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
    nv_delay();             /* Go delay for chip. */
    w0 &= ~NV_CLOCK;            /* Remove clock.    */
    QL_PCI_OUTH ( &ctlr->base_address->isp_nvram, w0 );
    nv_delay();             /* Go delay for chip. */
}

/*
 * Reads NVRAM contents into buffer. Confirms that NVRAM checksum is valid.
 */
static void
nvram_rd( QLFC_CONTROLLER *ctlr )
{
	uint8_t   chksum;
	uint8_t   i;
	uint16_t  data;

	chksum = 0;
	data = 0;
	for( i=0; i < NVRAM_SIZE / sizeof(uint16_t); i++ ) {
		/*
		* Set NVRAM address register and
		* issues function call.
		*/
		ctlr->nvram_buf.w[i] = read_nvram( ctlr, i );
		/*
		* Add received word to checksum.
		*/
		data |= ctlr->nvram_buf.w[i];
		chksum += ctlr->nvram_buf.w[i];
		chksum += ctlr->nvram_buf.w[i] >> 8;
	}

	/*
	* Ending checksum not zero tell
	* the user about it.
	if ( chksum ) {
		printf( "\nNOVRAM Checksum Error\n" );
	}
	else if ( !data ) {
		printf( "\nNOVRAM Contains Invalid Data\n" );
	}
	*/
}

