/*
 * Copyright 1995,1996 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */

/*
 * $Revision: 1.8 $	$Date: 1997/11/05 21:34:11 $
 *
 * $Log: hipdbg.c,v $
 * Revision 1.8  1997/11/05 21:34:11  avr
 * This checkin adds some mods made by Bill Sparks with a few of my changes.
 * It adds support for the commands:
 * escape to shell
 * ld mbuf
 * ld bpconfig
 * ld bpstats
 * ld bpjob
 *
 * Revision 1.7  1997/08/14 23:51:01  avr
 * This checkin fixes some build problems that ocurred with the switch to
 * kudzu.
 *
 * Revision 1.6  1997/08/03 20:30:17  jimp
 * moved hip_errors.h to kern/sys to make it easier to find
 *
 * Revision 1.5  1997/07/23 20:44:48  avr
 * This adds light weight timer functionality for performance measuring.
 *
 * Revision 1.4  1997/07/23 20:32:18  avr
 * Changed the prompt string from "rr_dbg>" to "hipdbg>".
 *
 * Revision 1.3  1997/07/22 22:43:03  jimp
 * Removed startup query of CPCI bus - if the linc boot prom crashed,
 * state isn't set up so can't query CPCI bus which causes the debugger
 * to crash immediately after launch.
 *
 * Revision 1.2  1997/07/22 09:50:09  jimp
 * Added CDIE trace, hex printouts of some values, fixed ldump_all
 * to really dump everything.
 *
 * Revision 1.40  1997/07/02 20:40:40  avr
 * This fixes a seg fault problem, memory leaks, a bug in ld bpjob and also
 * adds error checking on the command entered.
 *
 * Revision 1.39  1997/06/11 22:09:53  jimp
 * fixed a tab.
 *
 * Revision 1.36  1997/06/05 01:49:18  avr
 * This adds a new trace call for de/assign of ulps.
 *
 * Revision 1.35  1997/06/03 07:43:03  jimp
 * support for xmit_retry counter in "hipcntl status"
 *
 * Revision 1.34  1997/05/21 02:16:34  jimp
 * moved BP_DESC from hostp->dma_flags to hostp->flags
 *
 * Revision 1.33  1997/03/17 00:27:02  jimp
 * removed bounds check on ld mem
 *
 * Revision 1.32  1997/03/14  15:13:27  jimp
 * v1.50 - beta4 release
 *
 * Revision 1.30  1997/02/28  02:07:02  jimp
 * better b2h decode (required -DHIPPI_BP), fixed trace dump bug
 *
 * Revision 1.29  1997/02/21  18:25:39  jimp
 * v1.21 - final (?) error counters, fixed inc of pkt, conn, bytes, on errors, fixed man pgs, and other interfaces
 *
 * Revision 1.20  1997/01/20  07:14:12  jimp
 * first cut at rrdbg support for linc tracing
 *
 * Revision 1.18  1996/12/19  22:40:31  irene
 * Minor printf bug.
 *
 * Revision 1.17  1996/11/27  00:09:44  irene
 * Final (because I'm sick of this) fix for the NIC-derived pathname
 * possibilities. Try all possible combinations of hippi_serial, hippi,
 * xwidget, _, giving up and advising user to check "-i ioslot" arg or
 * use "-h hwgraph-name" arg.
 *
 * Revision 1.16  1996/11/14  06:22:29  irene
 * Yet another hwgraph node name change.
 * Map larger window to work around pcibr_xtalk_addr bug.
 *
 * Revision 1.15  1996/10/03  23:09:53  irene
 * hwgraph pathname change: xwidget now _
 *
 * Revision 1.10  1996/08/02  22:13:59  irene
 * LCSR ignore errors bit was in wrong place.
 *
 * Revision 1.1  1996/06/04  17:54:37  irene
 * First pass at Lego mmaps, using usrpci.
 *
 * Revision 1.0  1996/05/14  19:02:42  irene
 * No Message Supplied
 *
 * Revision 1.1  1996/05/11  01:38:49  irene
 * Added a couple of trace buf decoders, for Write DMA attn.
 *
 * Revision 1.0  1996/05/08  23:39:56  irene
 * No Message Supplied
 *
 *
 */

/*
 * This is a bringup utility cum debugger for the RioGrande card
 * or road runner chip. The program uses a tcl interface.
 *
 * Written by: Irene Kuffel, 2/96
 *
 * See help_h() for list of commands and syntax
 * See init_NIC() for board initialization and mem mapping.
 * See help_?*()  for syntax & what each command is about.
 *
 */


/* 
 * Some defines for LCDEFS in Makefile:
 *	RR_DEBUG    - for printing bunch of early debug stuff
 *	G2P_SIM	    - simulating on a Indigo2 gio2pci adaptor, compiled for 5.3
 *		      Not defining this will pick up code for running on
 *		      Lego/ficus.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/sbd.h>
#include <sys/mc.h>
#include "hippibp.h"
#ifdef G2P_SIM
#include "gio2pci.h"
#else
#include "sys/PCI/linc.h"
#include "../../firm/linc/include/r4650.h"
#include "../../firm/linc/include/eframe.h"
#include "../../firm/linc/include/hippi_sw.h"
#include "../../kern/sys/hip_errors.h"
#include "../../firm/linc/include/bypass.h"
#endif
#include "tcl.h"

#ifdef G2P_SIM
/* host DMA buffer: */
#define RR_DMA_BASE    PHYS_RAMBASE+0x0f00000	/* last MB on a 16 MB system */
#define RR_DMA_SIZE    0x0100000
#else
#define RR_DMA_BASE	(uint_t) 0x98000000	/* SDRAM loc 0, cont. pref. */
#define RR_DMA_SIZE	0x400000	/* 4 MB of SDRAM */
#endif

/* Board DMA buffer: 256KB board, do DMA to last 128KB */
#define BOARD_DMA_ADDR	0x20000
#define BOARD_DMA_MAX	0xFFFF


#define RR_DARDHI_BASE	0x1000
#define RR_DAWRHI_BASE	RR_DARDHI_BASE + (16 * sizeof (rr_dadesc_t))
#define RR_DARDLO_BASE	RR_DAWRHI_BASE + (16 * sizeof (rr_dadesc_t))
#define RR_DAWRLO_BASE	RR_DARDLO_BASE + (16 * sizeof (rr_dadesc_t))

/* XXX - need to set noswap bits in these ? */
#define DMA_READ_STATE	0x86 /* threshold=8, active=1, noswap=1 */
#define DMA_WRITE_STATE	0x20016
		/* disable producer compare, threshold=1, active=1, noswap=1 */

#define TRACE_BUFSIZ 4096

Tcl_Interp * interp;
Tcl_DString command;
int gotPartial;
char buf[1024];
char cmd_buf[1024];
char namebuf[512];

void breakloop();
void cleanexit();
void init_NIC();
void run_basic_tests();
void registerTclCommands();
int  hexToUInt (char* s);

char *progname;
char *devname;

int  linc_running;
int  dev_supplied;

#ifdef G2P_SIM
int  slot;		/* GIO PCI slot, default is slot 0 */
#else
int  ioslot = 3;
int  module = 1;
int  slot = -1;
#endif
int  mmemfd = -1;	/* to memory mapper device */
int  cmemfd = -1;	/* to PCI config space mapper device */
int  breakpoint;
jmp_buf jmp_env;

#ifndef G2P_SIM
/* main Linc State stuct */
volatile state_t *lstate;
volatile trace_t *ltracep;
/* ptr to mapped LINC PCI Memory space */
volatile char *lmemsp = (volatile char *)-1;
/* ptr to mapped LINC PCI Config space */
volatile pci_cfg_hdr_t *lcfgsp = (volatile pci_cfg_hdr_t *)-1;
#endif

/* ptr to mapped RR PCI Memory space */
volatile rr_pci_mem_t *memsp = (volatile rr_pci_mem_t *)-1;
/* ptr to mapped RR PCI Config space */
volatile pci_cfg_hdr_t *cfgsp = (volatile pci_cfg_hdr_t *)-1;

volatile void * dma_base;
#ifdef G2P_SIM
volatile u_int * sbcreg;	/* GIO2PCI status-bytecount register */
#endif

void help_halt();
void help_bp();
void help_r();
void help_d();
void help_ss();
void help_set();
void help_ww();
void help_ws();
void help_wb();
void help_q();
void help_cfg();
void help_dload();
#ifndef G2P_SIM
void help_lww();
void help_lwl();
void help_lrw();
void help_lset();
void help_lrefresh();
void help_lfcache();
void help_lintr();
void help_ld();
void help_ldload();

void ldump_mem(u_int addr, int nwords);
void ldump_pcicfg();
void ldump_control();
void ldump_lmcfg();
void ldump_dma();
void ldump_rr2l();
void ldump_l2rr();
void ldump_b2h();
void ldump_d2b();
void ldump_opposite();
void ldump_blk();
void ldump_ablk();
void ldump_wblk();
void ldump_mbuf();
void ldump_fpbuf();
void ldump_hostp();
void ldump_wirep();
void ldump_stats();
void ldump_bpconfig();
void ldump_bpstats();
void ldump_sdq();
void ldump_state();
void ldump_regs();
#endif

void dump_dma();
void dump_assist();


/* This page of routines tells what this program is all about. */
void
usage()
{
#ifdef G2P_SIM
    printf ("USAGE: %s -s <PCI slotnumber>\n", progname);
#else
    printf ("USAGE: %s < -S | -D> [-l] [-m <module>] [-i <ioslot>] [-h <hwgraph pathname>]\n", progname);
    printf ("\t\twhere -S or -D specifies src (LINC1)  or dst (LINC0).\n");
    printf ("\t\t      <ioslot> specifies I/O slot in cardcage (default=3)\n");
    printf ("\t\t      <module> specifies module in system (default = 1)\n");
    printf ("\t\t      -l says LINC is running, so don't do reset/init.\n");
    printf ("\t\t      -h supplies the hwgraph pathname in case of an improperly-programmed NIC.\n");
#endif
}

void
help_h()
{
	printf (
"The following special commands have been added to TCL for %s\n",
		progname);
	printf ("\th    - help\n");
	printf ("\t!    - escape to shell\n");
	printf ("\tq    - quit\n");
	printf (" ------------- ROAD RUNNER COMMANDS -------------\n");
	printf ("\thalt - halt Roadrunner internal CPU\n");
	printf ("\tbp   - set/display/clear breakpoint\n");
	printf ("\tr    - resume\n");
	printf ("\tss   - singlestep roadrunner CPU\n");
	printf ("\td    - display this or that\n");
	printf ("\tset  - set register values\n");
	printf ("\tww   - write 4-byte word to SRAM\n");
	printf ("\tws   - write 2-byte shortword to SRAM\n");
	printf ("\twb   - write a byte to SRAM\n");
	printf ("\tcfg  - write a word to PCI cfg space\n");
	printf ("\tdload- download a RR fw file.\n");
#ifndef G2P_SIM
	printf (" ------------------ LINC COMMANDS ----------------\n");
	printf ("\tlww      - write a word  to  LINC mem space\n");
	printf ("\tlwl      - write a long word  to  LINC mem space\n");
	printf ("\tlrw      - read  a word from LINC mem space\n");
	printf ("\tlset     - set a linc register - NOT IMPLEMENTED YET\n");
	printf ("\tlrefresh - write the sequence for SDRAM refresh\n");
	printf ("\tlfcache  - flush the 4640's data cache\n");
	printf ("\tlintr    - interrupt the 4640 processor\n");
	printf ("\tld       - version of display tailored to the linc\n");
	printf ("\tldload   - LINC download - NOT IMPLEMENTED\n");
#endif
	printf (
"Detailed help is available with \"help <cmd1> [<cmd2> ...]\"\n\n");
}

int
help(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int i;
    char * cmd;

    if (argc == 1) {
	help_h();
	return TCL_OK;
    }

    i = 0;
    while (++i < argc) {
	cmd = argv[i];
	if (!strcmp (cmd, "h"))
	    help_h();
	else if (!strcmp (cmd, "halt"))
	    help_halt();
	else if (!strcmp (cmd, "quit"))
	    help_q();
	else if (!strcmp (cmd, "bp"))
	    help_bp();
	else if (!strcmp (cmd, "r"))
	    help_r();
	else if (!strcmp (cmd, "d"))
	    help_d();
	else if (!strcmp (cmd, "ss"))
	    help_ss();
	else if (!strcmp (cmd, "set"))
	    help_set();
	else if (!strcmp (cmd, "ww"))
	    help_ww();
	else if (!strcmp (cmd, "ws"))
	    help_ws();
	else if (!strcmp (cmd, "wb"))
	    help_wb();
	else if (!strcmp (cmd, "cfg"))
	    help_cfg();
	else if (!strcmp (cmd, "dload"))
	    help_dload();
#ifndef G2P_SIM
	else if (!strcmp (cmd, "lww"))
	    help_lww();
	else if (!strcmp (cmd, "lwl"))
	    help_lwl();
	else if (!strcmp (cmd, "lrw"))
	    help_lrw();
	else if (!strcmp (cmd, "lset"))
	    help_lset();
	else if (!strcmp (cmd, "lrefresh"))
	    help_lrefresh();
	else if (!strcmp (cmd, "lfcache"))
	    help_lfcache();
	else if (!strcmp (cmd, "lintr"))
	    help_lintr();
	else if (!strcmp (cmd, "ld"))
	    help_ld();
	else if (!strcmp (cmd, "ldload"))
	    help_ldload();
#endif
	else {
	    printf ("No such command \"%s.\"\n", cmd);
	    help_h();
	}
    }
    return TCL_OK;
}

main(int argc, char *argv[])
{
    int retcode;
    char *prompt;
    char *cmd;
    int c;

    progname = argv[0];
    setbuf(stdout, 0);
    setbuf(stderr, 0);

    /* Check if we are running as root */
    if (getuid() != 0) {
       printf("Must be root to execute %s\n",argv[0]);
       exit(1);
    }

#ifdef G2P_SIM
    while ((c = getopt(argc, argv, "s:")) != EOF) {
#else
    while ((c = getopt(argc, argv, "i:h:lSDm:")) != EOF) {
#endif
	switch (c) {
#ifndef G2P_SIM
	    case 'l':
	    	linc_running = 1;
		break;
	    case 'S':
	    case 'D':
		if (slot != -1) {
		    printf ("-D or -S should only be specified once!\n");
		    usage();
		    exit(1);
		}
		if (c == 'S')
		    slot = 1;
		else
		    slot = 0;
		break;
	    case 'i':
		ioslot = atoi (optarg);
		break;
	    case 'h':
		strcpy (namebuf, optarg);
		dev_supplied = 1;
		break;
  	    case 'm':
	        module = strtoul(optarg, NULL,0);
		break;
#else
	    case 's':
		slot = atoi (optarg);
		break;
#endif
	    default:
		usage();
		exit(1);
	}
    }

#ifdef G2P_SIM
    devname = "/dev/mmem";
#else
    if (slot < 0) {
	printf ("Which LINC? You must specify \"-S\" or \"-D\"\n");
	usage();
	exit (1);
    }
    /*
     *	On the HIPPI-Serial card, the dst LINC answers to slot 0
     *	and the src LINC to slot 1.
     */
    if (!dev_supplied)
        sprintf (namebuf, "/hw/module/%d/slot/io%d/hippi_serial/pci", 
		 module, ioslot);

    if (slot == 0)
	strcat (namebuf, "/0/usrpci");
    else
	strcat (namebuf, "/1/usrpci");
    devname = namebuf;
#endif

    sigset (SIGINT, cleanexit);
    /* ---- Set up memory mappings to the card & initialize it ---- */
    init_NIC();

#if 0 /* Not to be run on operational system! */
    /* ---- Run some basic tests on the Roadrunner --------------- */
    run_basic_tests();
#endif

    /* ---- Set up TCL initialization. ---------------------------- */
    interp = Tcl_CreateInterp();
    Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
    Tcl_Init(interp);
    Tcl_DStringInit(&command);
    registerTclCommands();

    prompt = "hipdbg> ";

    printf("\n");
    printf("+--------------------------------------------------------------+\n");
    printf("|  The 4640's cache is not flushed by default. To get the      |\n");
    printf("|  most current state, execute \"lfcache\".                    |\n");
    printf("+--------------------------------------------------------------+\n\n");
	   

    /* ---- Simple little main loop ------------------------------- */
    while (1) {
	int n;
	char *cur,
	     *cur2;
	Tcl_CmdInfo info;
	printf (prompt);

	/* get a command */
	if ((n = read (0, buf, sizeof buf - 1)) <= 0)
	    exit(1);
	buf[n] = '\0';

	if (buf[0] == '\n')
	    continue;
	
	cur = buf;
	cur2 = cmd_buf;
	while((*cur != '\0') && (*cur != ' ') && (*cur != '\n')) {
	    *cur2++ = *cur++;
	}
	*cur2 = '\0';

	if (Tcl_GetCommandInfo(interp, cmd_buf, &info)) {
	    retcode = Tcl_RecordAndEval(interp, buf, 0);
	    
	    if (retcode != TCL_OK) {
		fprintf (stderr, interp->result);
		fprintf (stderr, "\n");
	    }
	    else if (interp->result != 0) {
		printf (interp->result);
	    }
	}
	else
	    printf("Invalid command: %s",buf);
	    
    } /* end of main loop */
}


/* Set up memory mappings to the card & initialize it */
#ifdef G2P_SIM
/* This is the Indigo2 gio2pci simulation version */
void
init_NIC()
{
    volatile u_int * gio64_arb;
    int	    i;

    if ((mmemfd = open(devname, O_RDWR)) < 0) {
	fprintf (stderr, "Error opening %s: %s\n",
		 devname, strerror(errno));
	exit(1);
    }
    cfgsp = (pci_cfg_hdr_t *) mmap (NULL, sizeof (pci_cfg_hdr_t),
				    PROT_READ | PROT_WRITE, MAP_SHARED,
				    mmemfd, slot?PCI_CONFIG1:PCI_CONFIG0);
    if ((int)cfgsp == -1) {
	fprintf(stderr, "Couldn't mmap PCI CFG space through /dev/mmem: %s\n",
		strerror(errno));
	exit(1);
    }
    sbcreg = (uint_t *) mmap (NULL, 4,
			      PROT_READ | PROT_WRITE, MAP_SHARED,
			      mmemfd, PCI_BYTECNT);
    if ((int)sbcreg == -1) {
	fprintf(stderr, "Couldn't mmap PCI stat/bytecnt reg: %s\n",
		strerror(errno));
	exit(1);
    }
    printf ("PCI status/byte-count register = %08x\n", *sbcreg);

    /* Write enable parity-chk & SERR report to cmd/status register */
    printf ("Writing config space cmd/status reg\n");
    cfgsp->stat_cmd = 0xFFFF0146;

    printf ("Reading slot %d PCI Config space:\n", slot);
    printf ("\tDevice/Vendor ID:\t\t%08x\n", cfgsp->dev_vend_id);
    assert(cfgsp->dev_vend_id == 0x0001120F);

    printf ("\tPCI Status/Cmd:\t\t\t%08x\n", cfgsp->stat_cmd);
    printf ("\tClassCode/RevID:\t\t%08x\n", cfgsp->cc_rev);
    printf ("\tBIST/HdrTyp/Lat/Cache:\t\t%08x\n",*&cfgsp->bhlc.i);

    /* Initialize base memory register with all ones and
	read it back to determine its size */
    cfgsp->bar0 = 0xffffffff;
    printf ("\tBaseAddress0 after writing all 1's:\t%08x\n", cfgsp->bar0);
    cfgsp->bar1 = 0xffffffff;
    printf ("\tBaseAddress1 after writing all 1's:\t%08x\n", cfgsp->bar1);
    printf ("\tMaxLat/MinGnt/IntPin/IntLine:\t%08x\n",
	    *(uint_t*)&cfgsp->max_lat);

    cfgsp->bar0 = (PCI_MEM + (slot*RR_PCI_MEM_SIZE)) & 0x1FFFFFFF;
    printf ("\nBaseAddress0 after config to %08x = %08x\n",
	     (PCI_MEM + (slot*RR_PCI_MEM_SIZE)) & 0x1FFFFFFF,
	     cfgsp->bar0);

    /* OK, now set up mapping to PCI memory space */
    memsp = (rr_pci_mem_t *) mmap (NULL, RR_PCI_MEM_SIZE,
				    PROT_READ | PROT_WRITE, MAP_SHARED, mmemfd,
	    slot?(PCI_MEM + RR_PCI_MEM_SIZE):PCI_MEM);

    if ((int)memsp == -1) {
	fprintf(stderr, "Couldn't mmap PCI CFG space through /dev/mmem: %s\n",
		strerror(errno));
	exit(1);
    }

    /* Set the GIO64_ARB register to enable DMA */
    gio64_arb = (volatile u_int *)
			mmap( 0, 0x1000,
				PROT_READ|PROT_WRITE,MAP_PRIVATE,
				mmemfd, 0xbfa00000 ) + (0x84/sizeof(u_int));
    if (slot == 1)
	*gio64_arb = ( *gio64_arb |
			GIO64_ARB_EXP1_MST | GIO64_ARB_EXP1_PIPED |
			GIO64_ARB_EXP1_RT);
    else
	*gio64_arb = ( *gio64_arb |
			GIO64_ARB_EXP0_MST | GIO64_ARB_EXP0_PIPED |
			GIO64_ARB_EXP0_RT);

    *sbcreg = 0xffff;	/* Byte count = 64K-1, L.endian = false */

    dma_base = mmap (0, RR_DMA_SIZE, PROT_READ|PROT_WRITE,
		     MAP_PRIVATE, mmemfd, 
		     PHYS_TO_K1(RR_DMA_BASE));

#ifdef RR_DEBUG
    /* Initialize first 4K of this to zeros for simulation - that
     * is where we are claiming the linc descr table to be.
     */
    bzero (dma_base, 4096);
#endif

    breakpoint = memsp->breakpoint_reg; 
    if (breakpoint & 1) /* disabled */
	breakpoint = -1;
}
#else
/* This one is the real thing */
void
init_NIC()
{
    int	    i;
    char    dname[512];
    uint_t  bar;

    /* First get the LINC config space */
    strcpy(dname,devname);
    strcat(dname,"/config");
    if ((cmemfd = open(dname, O_RDWR)) < 0) {
	fprintf (stderr, "Error opening %s: %s\n", dname, strerror(errno));
	if (dev_supplied) {
	    exit(1);
	}
	/* OK, assume a screwed up NIC. Try all the known
	 * possibilities so far. "hippi_serial" failed, so try
	 * "hippi", "xwidget", "_"
	 */
        sprintf (devname, "/hw/module/1/slot/io%d/hippi/pci/%d/usrpci",
		 ioslot, slot);
	printf ("Trying %s...\n", devname);
	strcpy(dname, devname);
	strcat(dname, "/config");
	if ((cmemfd = open(dname, O_RDWR)) > 0)
	    goto success;
	fprintf (stderr, "Error opening %s: %s\n", dname, strerror(errno));

        sprintf (devname, "/hw/module/1/slot/io%d/xwidget/pci/%d/usrpci",
		 ioslot, slot);
	printf ("Trying %s...\n", devname);
	strcpy(dname, devname);
	strcat(dname, "/config");
	if ((cmemfd = open(dname, O_RDWR)) > 0)
	    goto success;
	fprintf (stderr, "Error opening %s: %s\n", dname, strerror(errno));

        sprintf (devname, "/hw/module/1/slot/io%d/_/pci/%d/usrpci",
		 ioslot, slot);
	printf ("Trying %s...\n", devname);
	strcpy(dname, devname);
	strcat(dname, "/config");
	if ((cmemfd = open(dname, O_RDWR)) > 0)
	    goto success;
	fprintf (stderr, "Error opening %s: %s\n", dname, strerror(errno));

	fprintf (stderr,
"Tried all known hwgraph-name possibilities for hippi in ioslot %d\n", ioslot);
	fprintf (stderr, "Check your \"-i <ioslot>\" argument,\n");
	fprintf (stderr, "or use \"-h <hwgraph name>\" to supply the pathname.\n");
	exit(1);
    }

success:

    lcfgsp = (pci_cfg_hdr_t *) mmap (NULL, sizeof(pci_cfg_hdr_t),
				    PROT_READ | PROT_WRITE, MAP_SHARED,

				    cmemfd, 0);
    if ((int)lcfgsp == -1) {
	fprintf(stderr, "Couldn't mmap LINC PCI CFG space through %s: %s\n",
		dname, strerror(errno));
	exit(1);
    }

    lcfgsp = (pci_cfg_hdr_t *) ((char *)lcfgsp + (slot * 0x1000));
    printf ("Reading slot %d LINC PCI Config space:\n", slot);
    printf ("\tLINC Device/Vendor ID:\t\t%08x\n", lcfgsp->dev_vend_id);
    assert(lcfgsp->dev_vend_id == 0x000210a9);
    printf ("\tLINC PCI Status/Cmd:\t\t%08x\n", lcfgsp->stat_cmd);
    bar = *(uint_t*)&lcfgsp->bar0;
    printf ("\tLINC Base Addr Reg 0:\t\t%08x\n", bar);

    /* Now mmap the LINC memory space: 128 MB gets us SDRAM, 
     * CPCI cfg & pio spaces, through to LINC misc regs 
     */

    strcpy(dname,devname);
    strcat(dname,"/mem32");
    if ((mmemfd = open(dname, O_RDWR)) < 0) {
	fprintf (stderr, "Error opening %s: %s\n",
		 dname, strerror(errno));
	exit(1);
    }
    lmemsp = (char *) mmap (NULL, LINC_PCI_MEM_SIZE * 2,
				    PROT_READ | PROT_WRITE, MAP_SHARED,
				    mmemfd, bar);
    if ((int)lmemsp == -1) {
	fprintf(stderr, "Couldn't mmap LINC PCI MEM space through %s: %s\n",
		dname, strerror(errno));
	exit(1);
    }

    if (!linc_running) {
    	/* Hold 4640 in reset, CPCI reset off, leave IGNORE_ERRORS on. */
    	*(volatile uint_t *) (lmemsp + LINC_LCSR) = 0x00800008;

    	/* CCSR = EN_MAST_TIMEOUT|MEM_SPACE_EN|MASTER_EN|ARB_EN[1:0] */
    	*(volatile uint_t *) (lmemsp + LINC_CCSR) = 0x0005b000;

    	/* clear CERR by reading */
    	i = *(volatile uint_t *) (lmemsp + LINC_CERR);

    	/* enable SDRAM access */
    	*(volatile uint_t *) (lmemsp + LINC_PCCSR) = LINC_PCCSR_MEM_SPACE ;

    	/* Enable TDAVL in bytebus PLD */
    	*(volatile uint_t *) (lmemsp + LINC_BBCSR) = 0x0100ffff;
    	*(volatile uint_t *) (lmemsp + 0x7e0000c) = 1;
    }
    else {
	lstate = (volatile state_t *)(lmemsp + STATE_BASE);
	ltracep = (volatile trace_t *)(lmemsp + LINC_TRACE_BASE);
    }

    /* Get ptr to the RR's PCI config space */
    cfgsp = (volatile pci_cfg_hdr_t *)(lmemsp + LINC_CPCI_CONFIG_ADDR + 0x800);

    /* And ptr to the RR mem space */
    memsp = (volatile rr_pci_mem_t *) (lmemsp + LINC_CPCI_PIO_ADDR);

    printf ("Reading slot %d RR PCI Config space:\n", slot);
    printf ("\tDevice/Vendor ID:\t\t%08x\n", cfgsp->dev_vend_id);
    assert(cfgsp->dev_vend_id == 0x0001120F);

    printf ("\tPCI Status/Cmd:\t\t\t%08x\n", cfgsp->stat_cmd);
    printf ("\tClassCode/RevID:\t\t%08x\n", cfgsp->cc_rev);
    printf ("\tBIST/HdrTyp/Lat/Cache:\t\t%08x\n",*&cfgsp->bhlc.i);

    if (!linc_running) {
    	printf ("Enabling MASTER_EN and MEM_SPACE in CPCI cmd/status reg.\n");
    	cfgsp->stat_cmd |= 6;

    	/* Initialize base memory register with all ones and
	   read it back to determine its size */
    	cfgsp->bar0 = 0xffffffff;
    	printf ("\tBaseAddress0 after writing all 1's:\t%08x\n", cfgsp->bar0);

    	cfgsp->bar0 = 0;
    	printf ("\nBaseAddress0 after setting to %08x is %08x\n",
		 0, cfgsp->bar0);
    }
    else
      	printf ("\nBaseAddress0 = %08x\n", cfgsp->bar0);

    printf ("\tMaxLat/MinGnt/IntPin/IntLine:\t\t%08x\n",
	    *(uint_t*)&lcfgsp->max_lat);

    /* Confirm that CPCI pio mem is set up correctly by reading 
     * RR Dev/Vend ID through mem space */
    printf ("\tRR Dev/VendID through mem space = \t%08x\n",
	    * (volatile uint_t *)memsp);

    dma_base = lmemsp;

    breakpoint = memsp->breakpoint_reg; 
    if (breakpoint & 1) /* disabled */
	breakpoint = -1;
}
#endif

#ifdef G2P_SIM
/* REAL basic tests, not to be run on an operational system. */
void
run_basic_tests()
{
    uint_t i, j;

    /* ---------------------- TEST # 1 ----------------------------
     * Read magic number from EEPROM. EEPROM is byte-wide on 8-byte
     * mem bus, so address increments by 8 for each byte.
     */
    printf ("\n---- Test 1: Checking EEPROM magic words:\n");
    printf ("Setting window base reg to EEPROM\n");
    memsp->win_base_reg = 0x80000000;	/* point 2K window to EEPROM */
    printf ("Writing local ctrl reg to enable EEPROM access\n");
    memsp->misc_local_ctrl_reg &= ~RR_SRAM_ACCESS;
    i = ( (memsp->rr_sram_window[0] & 0xFF0000) |
	 ((memsp->rr_sram_window[2] >>  8) & 0x00FF0000) |
	 ((memsp->rr_sram_window[4] >> 16) & 0x0000FF00) |
	 ((memsp->rr_sram_window[6] >> 24) & 0x000000FF));
    j = i;
    printf ("EEPROM word 0 = %08x (expected 66666666)\n", i);
    printf ("EEPROM word 0 = %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x\n",
	    memsp->rr_sram_window[0], memsp->rr_sram_window[1],
	    memsp->rr_sram_window[2], memsp->rr_sram_window[3],
	    memsp->rr_sram_window[4], memsp->rr_sram_window[5],
	    memsp->rr_sram_window[6], memsp->rr_sram_window[7]);

    i = ( (memsp->rr_sram_window[8] & 0xFF0000) |
	 ((memsp->rr_sram_window[10] >>  8) & 0x00FF0000) |
	 ((memsp->rr_sram_window[12] >> 16) & 0x0000FF00) |
	 ((memsp->rr_sram_window[14] >> 24) & 0x000000FF));
    j += i;
    printf ("EEPROM word 1 = %08x (expected 99999999)\n", i);

    i = ( (memsp->rr_sram_window[16] & 0xFF0000) |
	 ((memsp->rr_sram_window[18] >>  8) & 0x00FF0000) |
	 ((memsp->rr_sram_window[20] >> 16) & 0x0000FF00) |
	 ((memsp->rr_sram_window[22] >> 24) & 0x000000FF));
    j += i;
    printf ("EEPROM word 2 = %08x (SRAM size)\n", i);

    i = ( (memsp->rr_sram_window[24] & 0xFF0000) |
	 ((memsp->rr_sram_window[26] >>  8) & 0x00FF0000) |
	 ((memsp->rr_sram_window[28] >> 16) & 0x0000FF00) |
	 ((memsp->rr_sram_window[30] >> 24) & 0x000000FF));
    j += i;
    printf ("EEPROM word 3 = %08x (bootstrap code address)\n", i);

    i = ( (memsp->rr_sram_window[32] & 0xFF0000) |
	 ((memsp->rr_sram_window[34] >>  8) & 0x00FF0000) |
	 ((memsp->rr_sram_window[36] >> 16) & 0x0000FF00) |
	 ((memsp->rr_sram_window[38] >> 24) & 0x000000FF));
    printf ("EEPROM word 4 = %08x (checksum should be %08x)\n", 
	    i, ~(j - 1));
 
   /* Reset local control reg to access SRAM instead of EEPROM */
    memsp->misc_local_ctrl_reg |= RR_SRAM_ACCESS;

    /* ---------------------- TEST # 2 --------------------------
     * Write a sequence of patterns to timer reference register
     * and read them back.
     */
    printf ("---- Test 2: write and read timer reference register:\n");
    memsp->timerref_reg = 0x55555555;
    printf ("Wrote 55555555, read back %08x\n", memsp->timerref_reg);
    memsp->timerref_reg = 0xAAAAAAAA;
    printf ("Wrote AAAAAAAA, read back %08x\n", memsp->timerref_reg);
    memsp->timerref_reg = 1;
    printf ("Wrote 00000001, read back %08x\n", memsp->timerref_reg);

}
#endif

#ifdef RR_DEBUG
/* Special DMA tests. Should not be run on operational board and
 * firmware.
 */

/* Simple little test to see if the DMA read write works. 
 * Only arg is the length of the DMA. We initialize "len" bytes
 * in the reserved dma area with some pattern, tell the board
 * to dma read that data, then tell it to dma write the data to a
 * second buffer in the dma reserved area. Then we compare bytes.
 */
 int
dmatest(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int len;
    int i;
    register volatile char * cp1;
    register volatile char * cp2;
    uint_t  *ip;
    uint_t addr, addr2, offset;
    volatile char    *lbuf;

#ifdef G2P_SIM
    if (argc != 2)
    {
	printf ("USAGE: dmatest <len_in_bytes>\n");
	return TCL_ERROR;
    }
#else
    if ((argc < 2) || (argc > 3))
    {
	printf ("USAGE: dmatest <len_in_bytes> [<host_addr>, default=%x]\n",
		RR_DMA_BASE);
	return TCL_ERROR;
    }
#endif
    len = strtoul(argv[1], NULL, 0);
    if (len > BOARD_DMA_MAX)
    {
	printf ("Length argument exceeds max (128KB)\n");
	return TCL_ERROR;
    }
    if (argc == 3) {
	addr = strtoul(argv[2], NULL, 0);
	offset = addr & 0x03ffffff;
	if (!(addr & 0x80000000)) {
	    printf ("Illegal SDRAM address, high-order bit must be on.\n");
	    return TCL_ERROR;	    
	}
	if (offset + len > 0x3fffff) {
	    printf ("address/length requested falls outside SDRAM bufmem.\n");
	    return TCL_ERROR;
	}
	lbuf = (char *)dma_base + offset;
    }
    else {
	addr = RR_DMA_BASE;
	lbuf = (volatile char *)dma_base;
    }

    if ((addr & 0xffff0000) != ((addr+len) & 0xffff0000)) {
	printf ("RR cannot dma across host 64K boundaries.\n");
	return TCL_ERROR;
    }
    /* initialize 1st "len" bytes in *dma_base with some pattern */
    for (i = 0, cp1 = (char *)lbuf; i < len; i++)
	*cp1++ = (i & 0xff);

    /* Make sure DMA assist is halted, and reset both dma channels */
    memsp->dma_assist_regs[7] = 0;
    memsp->dma_read_state = 1;
    memsp->dma_write_state = 1;

    memsp->dma_read_host_hi = 0;
    memsp->dma_read_host_lo = addr;
    memsp->dma_read_local = BOARD_DMA_ADDR;
    memsp->dma_read_len = len;
    memsp->dma_read_state = DMA_READ_STATE;

    printf ("Started DMA read, host %x to board %x, len=%x\n",
	    addr, BOARD_DMA_ADDR, len);

    sigset (SIGINT, breakloop);
    if (setjmp(jmp_env) == 0) {
	while (memsp->dma_read_state & 4) {
	    sleep (1);
	    printf (".");	
	}
	printf ("\nRead DMA complete.\n");
    }
    else {
	printf ("\nRead DMA hung? (interrupted)\n");
	dump_dma();
	return TCL_OK;
    }
    cp1 = (char *) lbuf;

    /*
     * Now set up DMA write from same part of RR SRAM to 
     * 2nd "len" bytes in *dma_base. Start with next 64K region.
     * If hit end of "bufmem", go to top.
     */
    addr2 = (addr + len + 0xffff) & 0xffff0000;
#ifndef G2P_SIM
    if ((addr2 & 0x3ffffff) > 0x3fffff) {
	addr2 &= 0xfc000000;
	cp2 = dma_base;
    }
    else
#endif
	cp2 = lbuf + (addr2 - addr);

    memsp->dma_write_host_hi = 0;
    memsp->dma_write_host_lo = addr2;
    memsp->dma_write_local = BOARD_DMA_ADDR;
    memsp->dma_write_len = len;

    memsp->dma_write_state = DMA_WRITE_STATE;
    printf ("Started DMA write, board %x to host %x, len=%x\n",
	    BOARD_DMA_ADDR, addr2, len);

    sigset (SIGINT, breakloop);
    if (setjmp(jmp_env) == 0) {
	while (memsp->dma_write_state & 4) {	/* still active */
	    sleep (1);
	    printf (".");
	}
	printf ("\nWrite DMA complete.\n");
    }
    else {
	printf ("\nWrite DMA hung? (interrupted)\n");
	dump_dma();
    }

    sigset(SIGINT, cleanexit);
    /* check data integrity */

    printf ("Checking Data integrity:\n");
    ip = (uint_t *) cp1;
    printf ("Src [%08x]:  %08x  %08x  %08x  %08x\n", 
	     ip, ip[0], ip[1], ip[2], ip[3]);
    ip = (uint_t *) cp2;
    printf ("Dst [%08x]:  %08x  %08x  %08x  %08x\n", 
	     ip, ip[0], ip[1], ip[2], ip[3]);

    for (i = 0; i < len; i++) {
	if (*cp1++ != *cp2++) {
	    printf ("DMA src and dst data differs at location 0x%x\n", i);
	    break;
	}
    }
    if (i == len)
	printf ("Data in source and dest buffers match\n");

    /* PIO read 1st few words of SRAM location 0x20000 */
    printf ("PIO read of DMA target location in RR:\n");
    len = (len + 3) >> 2;
    if (len > 8)
	len = 8;
    memsp->win_base_reg = BOARD_DMA_ADDR;
    printf ("\t0x%08x: ", BOARD_DMA_ADDR);
    for (i = 0; i < len; i++) {
	if (i == 4) {
	    printf ("\n\t0x%08x: ", BOARD_DMA_ADDR+16);
	}
	printf ("  %08x", memsp->rr_sram_window[i]);
    }
    printf ("\n");

    return TCL_OK;
}



/* Initialize the DMA assist rings and registers */
void
init_DA()
{
    /* Make sure DMA assist is halted */
    memsp->dma_assist_regs[7] = 0;

    /* Reset Read and WRite DMA engines */
    memsp->dma_read_state = 1;	/* write-only reset bit */
    memsp->dma_write_state = 1;	/* write-only reset bit */

    /* Set up DMA assist rings */
    memsp->dma_assist_regs[15] = RR_DARDHI_BASE;

    /* Read Hi Cons=prod=ref= rdhi ring base */
    memsp->dma_assist_regs[0] = RR_DARDHI_BASE;
    memsp->dma_assist_regs[1] = RR_DARDHI_BASE;
    memsp->dma_assist_regs[2] = RR_DARDHI_BASE;

    /* Write Hi Cons=prod=ref= wrhi ring base */
    memsp->dma_assist_regs[4] = RR_DAWRHI_BASE;
    memsp->dma_assist_regs[5] = RR_DAWRHI_BASE;
    memsp->dma_assist_regs[6] = RR_DAWRHI_BASE;

    /* Read Lo Cons=prod=ref= rdlo ring base */
    memsp->dma_assist_regs[8] = RR_DARDLO_BASE;
    memsp->dma_assist_regs[9] = RR_DARDLO_BASE;
    memsp->dma_assist_regs[10] = RR_DARDLO_BASE;

    /* Write Lo Cons=prod=ref= wrlo ring base */
    memsp->dma_assist_regs[12] = RR_DAWRLO_BASE;
    memsp->dma_assist_regs[13] = RR_DAWRLO_BASE;
    memsp->dma_assist_regs[14] = RR_DAWRLO_BASE;

}

/* Program DMA read assist */
int
datest1(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    rr_dadesc_t	    * dap;

    init_DA();
    
    /* Set 2K memory window to base of DA rings */
    memsp->win_base_reg = RR_DARDHI_BASE;
    dap = (rr_dadesc_t *) memsp->rr_sram_window;

    /* Set up 3 RdHi descriptors, for 0x100, 0x200, 0x300 bytes */
    dap->da_hostaddr = RR_DMA_BASE;
    dap->da_localaddr = BOARD_DMA_ADDR;
    dap->da_len = 0x100;
    dap->da_state = DMA_READ_STATE;

    dap++;
    dap->da_hostaddr = RR_DMA_BASE + 0x100;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x100;
    dap->da_len = 0x200;
    dap->da_state = DMA_READ_STATE;

    dap++;
    dap->da_hostaddr = RR_DMA_BASE + 0x300;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x300;
    dap->da_len = 0x300;
    dap->da_state = DMA_READ_STATE;

    memsp->event_reg = 0;   /* clear all the random sw* garbage bits */
    printf ("Event reg before queueing DA descriptors = %08x\n",
	     memsp->event_reg);
    printf ("Assist Read Hi done event is %08x\n", RREVT_ASST_RDHIDONE);

    /* Advance producer & reference registers */
    memsp->dma_assist_regs[0] = RR_DARDHI_BASE + (3 * sizeof (rr_dadesc_t));
    memsp->dma_assist_regs[2] = RR_DARDHI_BASE + (2 * sizeof (rr_dadesc_t));

    /* assist state = use compressed format, enable */
    memsp->dma_assist_regs[7] = 9;

    printf ("Queued 3 DMA descriptors, waiting for event notification");

    sigset (SIGINT, breakloop);
    if (setjmp(jmp_env) == 0) {
	/* OK, we should get the event notification when it is done */
	while ((memsp->event_reg & RREVT_ASST_RDHIDONE) == 0) {
	    printf (".");
	    sleep (1);
	}
    }
    sigset(SIGINT, cleanexit);
    printf ("\n");

    printf ("RdHi Consumer = %08x, Producer = %08x, Reference = %08x\n",
	     memsp->dma_assist_regs[1], memsp->dma_assist_regs[0],
	     memsp->dma_assist_regs[2]);
    printf ("Event reg BEFORE adjusting reference = %08x\n",
	     memsp->event_reg);

    /* adjust reference to producer, this should turn off event */
    memsp->dma_assist_regs[2] = memsp->dma_assist_regs[0];
    printf ("Event reg AFTER adjusting reference = %08x\n",
	     memsp->event_reg);

    return TCL_OK;
}

/*
 * This test is to find out if RR h/w actually tries to load the 
 * DA descriptors into the DMA registers if we ask for zero length
 * DMAs.
 */
int
datest2(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    rr_dadesc_t	    * dap;

    init_DA();

    /* Set 2K memory window to base of DA rings */
    memsp->win_base_reg = RR_DARDHI_BASE;
    dap = (rr_dadesc_t *) memsp->rr_sram_window;

    /* Set up 3 RdHi descriptors with different addresses but all zero len */
    dap->da_hostaddr = RR_DMA_BASE;
    dap->da_localaddr = BOARD_DMA_ADDR;
    dap->da_len = 0;
    dap->da_state = DMA_READ_STATE;

    dap++;
    dap->da_hostaddr = RR_DMA_BASE + 0x10;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x10;
    dap->da_len = 0;
    dap->da_state = DMA_READ_STATE;

    dap++;
    dap->da_hostaddr = RR_DMA_BASE + 0x20;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x20;
    dap->da_len = 0;
    dap->da_state = DMA_READ_STATE;
 
    /* Advance producer & reference registers */
    memsp->dma_assist_regs[0] = RR_DARDHI_BASE + (3 * sizeof (rr_dadesc_t));
    memsp->dma_assist_regs[2] = RR_DARDHI_BASE + (2 * sizeof (rr_dadesc_t));

    /* assist state = use compressed format, enable */
    memsp->dma_assist_regs[7] = 9;

    printf ("Queued 3 DMA descriptors, waiting for event notification");

    sigset (SIGINT, breakloop);
    if (setjmp(jmp_env) == 0) {
	/* OK, we should get the event notification when it is done */
	while ((memsp->event_reg & RREVT_ASST_RDHIDONE) == 0) {
	    printf (".");
	    sleep (1);
	}
    }
    sigset(SIGINT, cleanexit);
    printf ("\n");

    printf ("RdHi Consumer = %08x, Producer = %08x, Reference = %08x\n",
	     memsp->dma_assist_regs[1], memsp->dma_assist_regs[0],
	     memsp->dma_assist_regs[2]);
    printf ("Event reg BEFORE adjusting reference = %08x\n",
	     memsp->event_reg);
    /* adjust reference to producer, this should turn off event */
    memsp->dma_assist_regs[2] = memsp->dma_assist_regs[0];

    printf ("Event reg AFTER adjusting reference = %08x\n",
	     memsp->event_reg);

    /* Now dump the Read DMA registers to see what the last loaded
     * addresses were.
     */
    printf ("DMA host addr = %08x %08x, local address = %08x\n",
	    memsp->dma_read_host_hi, memsp->dma_read_host_lo,
	    memsp->dma_read_local);
    printf ("DMA read state = %08x\n", memsp->dma_read_state);

    return TCL_OK;
}

/*
 * This test is to determine where the DA Cons register ends up
 * when we get a DMA attention: queue 3 DA descriptors,
 * the second of which has a deliberate bad host address - this
 * should generate a PCI Master abort by the RR. 
 * Where is DA Cons reg - the bad one or is that "consumed" and
 * DA Cons reg points after it.
 */
int
datest3(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    rr_dadesc_t	    * dap;

    init_DA();

    /* Set 2K memory window to base of DA rings */
    memsp->win_base_reg = RR_DARDHI_BASE;
    dap = (rr_dadesc_t *) memsp->rr_sram_window;

    dap->da_hostaddr = RR_DMA_BASE;
    dap->da_localaddr = BOARD_DMA_ADDR;
    dap->da_len = 0x20;
    dap->da_state = DMA_READ_STATE;

    /* Second descriptor has deliberate bad host address */
    dap++;
    dap->da_hostaddr = 0x80000000;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x20;
    dap->da_len = 20;
    dap->da_state = DMA_READ_STATE;
#if 0
    /* Third one is good. */
    dap++;
    dap->da_hostaddr = RR_DMA_BASE + 0x20;
    dap->da_localaddr = BOARD_DMA_ADDR + 0x20;
    dap->da_len = 20;
    dap->da_state = DMA_READ_STATE;
 #endif
    /* Advance producer & reference registers */
    memsp->dma_assist_regs[0] = RR_DARDHI_BASE + (2 * sizeof (rr_dadesc_t));
    memsp->dma_assist_regs[2] = RR_DARDHI_BASE + (1 * sizeof (rr_dadesc_t));

    /* assist state = use compressed format, enable */
    memsp->dma_assist_regs[7] = 9;

    printf ("Queued 3 DMA descriptors, waiting for event notification");
    sigset (SIGINT, breakloop);
    if (setjmp(jmp_env) == 0) {
	/* OK, we should get the event notification when it is done */
        while (!(memsp->event_reg & 
		(RREVT_ASST_RDHIDONE | RREVT_DMAREADERR))) {
	    printf (".");
	    sleep (1);
	}
    }
    printf ("\n");
    sigset(SIGINT, cleanexit);

    printf ("RdHi Consumer = %08x, Producer = %08x, Reference = %08x\n",
	     memsp->dma_assist_regs[1], memsp->dma_assist_regs[0],
	     memsp->dma_assist_regs[2]);

    printf ("DMA host addr = %08x %08x, local address = %08x\n",
	    memsp->dma_read_host_hi, memsp->dma_read_host_lo,
	    memsp->dma_read_local);

    printf ("DMA read state = %08x\n", memsp->dma_read_state);

    return TCL_OK;
}



int
walking1_test(uint_t addr, int break_on_failure)
{
    unsigned int i, j;

    /* Move the SRAM window to the right 2KB segment */
    memsp->win_base_reg = addr;

    i = 1;
    while ( i != 0) {
	memsp->win_data_reg = i;
	j = memsp->win_data_reg;
	if (i != j) {
	    printf ("Walking1 test to SRAM address %x failed, wrote %x, read %x\n",
		    addr, i, j);
	    if (break_on_failure)
	       return -1;
	}
	i = (i << 1);
    }
    return 0;
}

int
walking0_test(uint_t addr, int break_on_failure)
{
    unsigned int i, j;
    int k;

    /* Move the SRAM window to the right 2KB segment */
    memsp->win_base_reg = addr;
    i = 0xfffffffe;
    for (k = 0; k < 32; k++) {
	memsp->win_data_reg = i;
	j = memsp->win_data_reg;
	if (i != j) {
	    printf ("Walking0 test to SRAM address %x failed, wrote %x, read %x\n",
		    addr, i, j);
	    if (break_on_failure)
	       return -1;
	}
	i = ((i << 1) | 1);
    }
    return 0;
}

/* For i = 4,8,16,... write value = address. Then read everything back.
 */
int 
walking1_addr(int break_on_failure)
{
    int i;
    int addr;
    int word;
    int errs = 0;

    memsp->win_base_reg = 0;
    i = 1;
    while (i < 0x10000) {
	/* Shift window if needed */
	while ((i << 2) >= (memsp->win_base_reg + 2048))
	    memsp->win_base_reg += 2048;
	memsp->rr_sram_window[i % 512] = i;
	i = i << 1;
    }

    memsp->win_base_reg = 0;
    i = 1;
    while (i < 0x10000) {
	/* Shift window if needed */
	while ((i << 2) >= (memsp->win_base_reg + 2048))
	    memsp->win_base_reg += 2048;
	word = memsp->rr_sram_window[i % 512];
	if (word != i) {
	    printf ("Walking 1 address test failed at addr %x, expected %x, read %x.\n",
		    i, word);
	    if (break_on_failure)
	        return -1;
	    errs++;
	}
	i = i << 1;
    }
    if (errs == 0)
        printf ("walking 1 address test passed.\n");
    return errs;
}

/* for each word, write its address into the word. Then read everything
 * back. */
int 
data_is_addr_test(int break_on_failure)
{
    int i;
    int addr;
    int word;
    int errs = 0;

    /* First pass, write its address to every word. */
    memsp->win_base_reg = 0;
    i = 0; 
    addr = 0;
    while (i < 0x10000) {
	/* Shift window if needed */
	if (addr >= (memsp->win_base_reg + 2048))
	    memsp->win_base_reg += 2048;
	memsp->rr_sram_window[i % 512] = addr;
	i++;
	addr += 4;
    }

    /* Second pass, read every word and compare to address. */
    memsp->win_base_reg = 0;
    i = 0; 
    addr = 0;
    while (i < 0x10000) {
	/* Shift window if needed */
	if (addr >= (memsp->win_base_reg + 2048))
	    memsp->win_base_reg += 2048;
	word = memsp->rr_sram_window[i % 512];
	if (word != addr) {
	    printf ("Data_is_address test failed at location %x: wrote %x, read %x\n",
		    addr, addr, word);
	    if (break_on_failure)
	        return -1;
	    errs++;
	}
	i++;
	addr += 4;
    }

    if (errs == 0)
        printf ("Data_is_address test passed.\n");
    return errs;
}

/* Write pattern all over SRAM, then read it back */
int
fill_and_check (uint_t pattern, int break_on_failure)
{
    int i, j;
    uint_t word;
    int errs = 0;

    for (i = 0; i < 128; i++) {
	/* Point window at relevant 2K segment */
	memsp->win_base_reg = i * 2048;
	for (j = 0; j < 512; j++)
	    memsp->rr_sram_window[j] = pattern;
    }

    for (i = 0; i < 128; i++) {
	/* Point window at relevant 2K segment */
	memsp->win_base_reg = i * 2048;
	for (j = 0; j < 512; j++) {
	    word = memsp->rr_sram_window[j];
	    if (word != pattern) {
		printf ("fill_and_check test failed at location %x: wrote %x, read %x.\n",
			(i*2048 + j*4), pattern, word);
		if (break_on_failure)
		    return -1;
		errs++;
	    }
	}
    }
    if (errs)
        return -1;
    else {
	printf ("fill_and_check with pattern of %x passed.\n", pattern);
        return 0;
    }
}


/* -------- Test 1: walking 1 to various locations ------------- */
int
sramtest1(int break_on_failure)
{
    int err = 0;
    int i;

    for (i = 0; i < 256; i++) {
	if (walking1_test ((i * 1028), break_on_failure) != 0) {
	    if (break_on_failure)
	        return 1;
	    err++;
	}
    }
    if (err == 0)
        printf ("SRAM Test 1 (walking 1) passed.\n");
    return err;
}


/* -------- Test 2: walking 0 to various locations ------------- */
int
sramtest2(int break_on_failure)
{
    int err = 0;
    int i;

    for (i = 0; i < 256; i++) {
	if (walking0_test ((i * 1028), break_on_failure) != 0) {
	    if (break_on_failure)
	        return 1;
	    err++;
	}
    }
    if (err == 0)
        printf ("SRAM Test 2 (walking 0) passed.\n");
    return err;
}


/* USAGE: sram [continue_flag] */
int
sramtest(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int break_on_failure = 1;
    int i;
    int err = 0;
    int test = -1;
    int loop = 1;
    int saved_hc;

    if (argc > 1) {
	test = atoi (argv[1]);
	if (argc > 2) {
	    break_on_failure = atoi (argv[2]);
	    if (argc > 3) {
		loop = atoi (argv[3]);
	    }
	}
    }
#if 0
    if (argc < 4) {
	/* Turn off parity checking */
	saved_hc = memsp->misc_host_ctrl_reg;
	memsp->misc_host_ctrl_reg = 0xe302;
    }
#endif

starttest:
    switch (test) {
    case 0:
	err = data_is_addr_test (break_on_failure); break;

    case 1:
	err = sramtest1 (break_on_failure); break;

    case 2:
	err = sramtest2 (break_on_failure); break;

    case 3:
	err = fill_and_check(0x55555555, break_on_failure); break;

    case 4:
	err = fill_and_check(0xaaaaaaaa, break_on_failure); break;

    case 5:
        err = fill_and_check(0xffffffff, break_on_failure); break;

    case 6:
	err = fill_and_check(0, break_on_failure); break;

    case 7:
	err = walking1_addr(break_on_failure); break;

    default:
	err = data_is_addr_test (break_on_failure);
	if (err & break_on_failure)
	    break;
	err += sramtest1 (break_on_failure);
	if (err & break_on_failure)
	    break;
	err += sramtest2 (break_on_failure);
	if (err & break_on_failure)
	    break;
	err += fill_and_check(0x55555555, break_on_failure);
	if (err & break_on_failure)
	    break;
	err += fill_and_check(0xaaaaaaaa, break_on_failure);
	if (err & break_on_failure)
	    break;
        err += fill_and_check(0xffffffff, break_on_failure);
	if (err & break_on_failure)
	    break;
	err += fill_and_check(0, break_on_failure);
	if (err & break_on_failure)
	    break;
	err += walking1_addr(break_on_failure);
	if (err & break_on_failure)
	    break;
    }

#if 0
    if (argc < 4)
      /* Restore parity checking? */
      memsp->misc_host_ctrl_reg = saved_hc;
#endif

    if (--loop > 0) {
	if (((break_on_failure) && (!err)) || (!break_on_failure))
	    goto starttest;
    }

    if (err)
        return TCL_ERROR;
    else
        return TCL_OK;
}
#endif /* RR_DEBUG */

/*
 * Display PCI Config space headers
 */
void
dump_pcicfg()
{
    printf ("PCI Config space headers:\n");
    printf ("\tDevice/Vendor ID:\t\t%08x\n", cfgsp->dev_vend_id);
    printf ("\tPCI Status/Cmd:\t\t\t%08x\n", cfgsp->stat_cmd);
    printf ("\tClassCode/RevID:\t\t%08x\n",  cfgsp->cc_rev);
    printf ("\tBIST/HdrTyp/Lat/Cache:\t\t%08x\n",*&cfgsp->bhlc.i);
    printf ("\tBaseAddress0:\t\t\t%08x\n", cfgsp->bar0);
    printf ("\tBaseAddress1:\t\t\t%08x\n", cfgsp->bar1);
    printf ("\tMaxLat/MinGnt/IntPin/IntLine:\t%08x\n",
	    *(uint_t*)&cfgsp->max_lat);
}

/*
 * Display state and PC if halted.
 */
void
dump_state()
{
    uint_t state;

    state = memsp->misc_host_ctrl_reg;
    if (state & RR_HALTED)
	printf ("Roadrunner CPU halted at PC = %x\n", memsp->prog_counter_reg);
    printf ("Misc host control reg = %08x\n", state);
    if (state) {
	printf ("\tbits on = (");
	if (state & INVAL_OPCODE)
	    printf ("INVAL_OPCODE,");
	if (state & CPU_PARITY_ERR)
	    printf ("CPU_PARITY_ERR,");
	if (state & RR_HALT_INSTR)
	    printf ("HALT_INSTR,");
	if (state & RR_HALTED)
	    printf ("HALTED,");
	if (state & RR_SINGLESTEP)
	    printf ("RR_SINGLESTEP");
	if (state & RR_HALT)
	    printf ("RR_HALT");
	if (state & RR_HARDRESET)
	    printf ("RR_HARDRESET");
	if (state & ENABLE_SWAP)
	    printf ("ENABLE_SWAP");
	if (state & INTR_STATE)
	    printf ("INTR_STATE");
	printf (")\n");
    }
}
/*
 * Display the rr's general control registers.
 */
void
dump_control()
{
    uint_t i;

    /* TBD XXX - call out the bits in each of these registers */

    printf("Roadrunner Control Registers:\n");

    i = memsp->misc_host_ctrl_reg;
    printf("\tMisc Host Control Register = \t%08x\n", i);

    i = memsp->misc_local_ctrl_reg;
    printf("\tMisc Local Control Register = \t%08x\n", i);

    i = memsp->pci_state_reg;
    printf("\tPCI State Register = \t\t%08x\n", i);

    i = memsp->prog_counter_reg;
    printf("\tProgram Counter Register = \t%08x\n", i);

    i = memsp->breakpoint_reg;
    printf("\tBreakpoint Register = \t\t%08x\n", i);

    i = memsp->timer_reg;
    printf("\tTimer Register = \t\t%08x\n", i);

    i = memsp->timerref_reg;
    printf("\tTimer Reference Register = \t%08x\n", i);

    i = memsp->event_reg;
    printf("\tEvent Register = \t\t%08x\n", i);

    i = memsp->mbox_event_reg;
    printf("\tMailbox Event Register = \t%08x\n", i);

    i = memsp->win_base_reg;
    printf("\tWindow Base Register = \t\t%08x\n", i);
#if 0
    i = memsp->win_data_reg;
    printf("\tWindow Data Register = \t\t%08x\n", i);
#endif
    i = memsp->recv_state_reg;
    printf("\tReceive State Register = \t%08x\n", i);

    i = memsp->xmit_state_reg;
    printf("\tXmit State Register = \t\t%08x\n", i);

    i = memsp->hippi_ovrhd_reg;
    printf("\tHIPPI Overhead Register = \t%08x\n", i);
}

/* Display local memory configuration registers */
void
dump_lmcfg()
{
    uint_t  i;

    printf("Roadrunner Local Memory Configuration registers:\n");

    i = memsp->local_mem_cfg_regs[0];
    printf("\tReceive Buffer Base = \t\t%08x\n", i);
    i = memsp->local_mem_cfg_regs[1];
    printf("\tReceive Buffer Producer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[2];
    printf("\tReceive Buffer Consumer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[3];
    printf("\tCPU Priority = \t\t\t%08x\n", i);

    i = memsp->local_mem_cfg_regs[4];
    printf("\tTransmit Buffer Base = \t\t%08x\n", i);
    i = memsp->local_mem_cfg_regs[5];
    printf("\tTransmit Buffer Producer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[6];
    printf("\tTransmit Buffer Consumer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[7];
    printf("\tDelay Line State = \t\t%08x\n", i);

    i = memsp->local_mem_cfg_regs[8];
    printf("\tHIPPI Receive Descriptor Producer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[9];
    printf("\tHIPPI Receive Descriptor Consumer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[10];
    printf("\tHIPPI Receive Descriptor Reference = \t%08x\n", i);
    /* 11 unused */

    i = memsp->local_mem_cfg_regs[12];
    printf("\tHIPPI Transmit Descriptor Producer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[13];
    printf("\tHIPPI Transmit Descriptor Consumer = \t%08x\n", i);
    i = memsp->local_mem_cfg_regs[14];
    printf("\tHIPPI Transmit Descriptor Reference = \t%08x\n", i);

}
void
dump_dma()
{
    printf ("Roadrunner Host DMA registers\n");

    printf ("\tDMA write host addr = %08x %08x\n",
	    memsp->dma_write_host_hi, memsp->dma_write_host_lo);
    printf ("\tDMA write local addr = %08x\n", memsp->dma_write_local);
    printf ("\tDMA write length = %08x\n", memsp->dma_write_len);
    printf ("\tDMA write state = %08x\n", memsp->dma_write_state);

    printf ("\tDMA read host addr = %08x %08x\n",
	    memsp->dma_read_host_hi, memsp->dma_read_host_lo);
    printf ("\tDMA read local addr = %08x\n", memsp->dma_read_local);
    printf ("\tDMA read length = %08x\n", memsp->dma_read_len);
    printf ("\tDMA read state = %08x\n", memsp->dma_read_state);
}

void
dump_assist()
{
    int i;

    printf ("\n Roadrunner DMA Assist registers:\n");
    for (i = 0; i < 16; ) {
	printf ("\t[%02d]%08x", i, memsp->dma_assist_regs[i]);
	if ((++i & 3) == 0)
	    printf ("\n");
    }
}

/*
 * dump_gca()
 *
 * Pretty print the contents of the General Communications area.
 * First 32 locations are mailboxes. Next 32 general purpose comm
 * area.
 */
void
dump_gca()
{
    int i;

    printf ("\nGeneral Communications Area:\n");
    for (i = 0; i < 64; ) {
	printf ("\t[%02d]%08x", i, memsp->rr_gca.i[i]);
	if ((++i & 3) == 0)
	    printf ("\n");
    }
}

void
dump_regs()
{
    int i;

    printf ("\nRoadrunner internal CPU registers:\n");
    for (i = 0; i < 32; ) {
	printf ("\t[r%02d]%08x", i, memsp->rr_gpr[i]);
	if ((++i & 3) == 0)
	    printf ("\n");
    }
}

void
dump_all()
{
    dump_pcicfg();
    dump_state();
    dump_control();
    dump_lmcfg();
    dump_regs();
    dump_dma();
    dump_assist();
    dump_gca();
}

/*
 * Display RR local memory (SRAM on card). The RR only
 * allows us a 2KB window into the SRAM. We move the window
 * by writing the Base Window Register (one of the "General
 * Control" registers).
 */
void
dump_mem(int addr, int nwords)
{
    int i, j;
    int offset, count;

#ifdef RR_DEBUG
    printf ("Dumping Roadrunner memory, location %08x, %d words\n",
	    addr, nwords);
#endif

    addr &= 0xFFFFFFFC; /* word dump only, so adjust addr accordingly */
    j = 0;	/* newline counter, every 4 words */

    while (nwords > 0) {
	/* Move the SRAM window to the right 2KB segment */
	memsp->win_base_reg = (addr & 0xFFFFF800);
	offset = (addr & 0x7FF) >> 2; /* word index inside window */
	count = 512 - offset;	      /* # words to end of window */
	if (count > nwords)
	    count = nwords;

	for (i = 0; i < count; i++) {
	    if ((j++ & 3) == 0)
		printf ("\n[%08x]: ", addr);
	    printf ("  %08x", memsp->rr_sram_window[offset++]);
	    addr += 4;
	}
	nwords -= count;
    }
    printf ("\n\n");
}

/* Pretty-print one trace entry for source RR */
void
d_st(int ix, uint_t a0, uint_t a1, uint_t a2, uint_t a3)
{
    int	opcode;
    int	time;
    int eoc;

    opcode = (a0 >> 24);
    time   = (a0 & 0xffffff);

    printf ("    [%03x, %06x] : ", ix, time);

    switch (opcode) {
	case 1:	printf ("INIT\n");
		break;
	case 2: printf ("FETCH (lincXDp=%08x, RdLoProd=%08x)\n", a1, a2);
		break;
	case 3: printf ("RDLODONE (lincXDp=%08x, RdLoRef=%08x)\n", a1, a2);
		break;
	case 4: printf ("SW3 (lincXDp=%08x, localXDP=%08x)\n", a1, a2);
		break;
	case 5: printf ("RDHIDMA queued (haddr=%08x, laddr=%08x, len=0x%x)\n",
			a1, a2,a3);
		break;
	case 6: printf ("RDHIDMA done (RefDescr=%08x, laddr=%08x)\n", a1, a2);
		break;
	case 7: printf ("WRLODMA queued (haddr=%08x, laddr=%08x, len=0x%x)\n",
			a1, a2, a3);
		break;
	case 8: printf ("WRLODMA done (RefDescr=%08x, laddr=%08x)\n", a1, a2);
		break;
	case 9: eoc = (a2 >> 24);
		printf ("EVTMSG (lincXDp=%08x, evt_opcode=", a1);
		switch (eoc) {
		    case 0: printf ("OK)\n"); break;
		    case 1: printf ("FLUSHED)\n"); break;
		    case 3: printf ("CONN_TIMEO)\n"); break;
		    case 4: printf ("DST_DISCON)\n"); break;
		    case 5: printf ("CONN_REJ)\n"); break;
		    case 7: printf ("SRC_PERR)\n"); break;
		    default: printf ("0x%x)\n", eoc); break;
		}
		break;
	case 10: printf ("TXDONE (TxDesc Ref=%08x, Cons=%08x, Prod=%08x)\n",
			 a1, a2, a3);
		 break;
	case 11: printf ("TXATTN (TxD word0=%08x, word1=%08x, TxStatus=%08x)\n",
			 a1, a2, a3);
		 break;	
	case 12: printf ("TIMEO (TxDesc Ref=%08x, Cons=%08x, Prod=%08x)\n",
			 a1, a2, a3);
		 break;
	case 13: printf ("RXATTN (RxStateReg=%08x)\n", a1);
		 break;	
	case 14: printf ("UNEXPECTED EVENT (%d): PANIC\n", a1);
		 break;
	default: printf ("Unknown trace opcode=0x%x: a1=%08x,a2=%08x,a3=%08x\n",
			 opcode, a1, a2, a3);
    }
}

/* Pretty-print one trace entry for dst RR */
void
d_dt(int ix, uint_t a0, uint_t a1, uint_t a2, uint_t a3)
{
    int	opcode;
    int	time;
    int eoc;

    opcode = (a0 >> 24);
    time   = (a0 & 0xffffff);

    printf ("    [%03x, %06x] : ", ix, time);

    switch (opcode) {
	case 1:	printf ("INIT\n");
		break;
	case 2: printf ("RCV ATTN (State=%08x, RcvDescProd=%x)\n", a1, a2);
		break;
	case 3: printf ("RCV EVT HNDLR (RcvDescCons=%08x, RcvDescRef=%08x, RcvDescProd=%08x)\n",
			 a1, a2, a3);
		break;
	case 4: printf("DA WAIT FOR DESC (Descr Cons=%08x, Prod=%08x, Ref=%08x)\n", 
			a1, a2, a3);
		break;
	case 5: printf("DA WAIT FOR DATA (Descr Cons=%08x, Prod=%08x, Ref=%08x)\n", 
			a1, a2,a3);
		break;
	case 6: printf ("RD DMA DONE (Linc's descr cons = %08x, data cons = %08x)\n", 
			a1, a2);
		break;
	case 7: printf ("WR DMA DONE (Host addr = %08x, local addr = %08x, len = 0x%x)\n",
			a1, a2, a3);
		break;
	case 8: printf ("Writing LINC descr (%08x, %08x, %08x)\n",
			a1, a2, a3);
		break;
	case 9: printf ("Writing LINC data (haddr=%08x, laddr=%08x, len=%08x)\n",
			a1, a2, a3);
		break;
	case 10: printf ("Timeout (rcv desc prod=%08x, cons=%08x, ref=%08x)\n",
			 a1, a2, a3);
	    	break;
	case 11: printf ("LDATA_WAIT_STATE\n");
		 break;
	case 12: printf ("Write DMA attn (Haddr=%08x, Laddr=%08x, len=%d)\n",
			a1, a2, a3);
		 break;
	case 13: printf ("UNEXPECTED EVENT(%d): PANIC\n", a1);
		 break;
	case 14: printf ("Write DMA attn (dma state=%08x, asst consumer=%08x, asst state=%08x)\n",
			a1, a2, a3);
		 break;
	case 15: printf ("send_desc()\n");
		 break;
	default: printf ("Unknown trace opcode=0x%x: a1=%08x,a2=%08x,a3=%08x\n",
			opcode, a1, a2, a3);
    }
}

/*
 * Dump RR's trace buffer. Argument "src" is 1 if we are to use
 * srcRR's trace opcodes. 0 means we are dumping dstRR's trace.
 */
int
dump_trace(int src)
{
    int	    trace_ix, j;
    uint_t  *ip;
    uint_t  a0,a1,a2,a3;

    char    *tbuf;

    if ((tbuf = malloc(TRACE_BUFSIZ)) == 0)
    {
	printf ("Couldn't malloc trace buffer!\n");
	return -1;
    }

    /* First get trace index at location 0x204 in SRAM. The f/w
     * keeps trace_ix as a byte offset into trace buffer.
     */
    memsp->win_base_reg = 0x204;
    trace_ix = memsp->win_data_reg;

    if (src)
        printf ("Source RR trace buffer:\n");
    else
        printf ("Destination RR trace buffer:\n");
    /* Now point the 2K shared mem window into the 
     * trace buffer, at location 0x1000, and read it all
     * into tbuf
     */
    for (j = 0; j < (TRACE_BUFSIZ / 2048); j++) {
	memsp->win_base_reg = (0x1000 + (j * 2048));
	memcpy (tbuf+(j*2048), (const void *) (memsp->rr_sram_window), 2048);
    }

    ip = (uint_t *)tbuf;
    ip += (trace_ix >> 2);
    j = trace_ix;
    while (j < TRACE_BUFSIZ) {
	a0 = *ip++;
	a1 = *ip++;
	a2 = *ip++;
	a3 = *ip++;
	if (src)
    	    d_st(j, a0,a1,a2,a3);
	else
    	    d_dt(j, a0,a1,a2,a3);
	j += 16;
    }

    j = 0;
    ip = (uint_t*) tbuf;
    while (j < trace_ix) {
	a0 = *ip++;
	a1 = *ip++;
	a2 = *ip++;
	a3 = *ip++;
	if (src)
    	    d_st(j, a0,a1,a2,a3);
	else
    	    d_dt(j, a0,a1,a2,a3);
	j += 16;
    }

    free(tbuf);
    return 0;

}

void
help_d()
{
	printf ("Display Roadrunniner registers and memory:\n");
	printf ("\td pcicfg  -- PCI Config space headers\n");
	printf ("\td state   -- State of Roadrunner CPU\n");
	printf ("\td control -- general control registers\n");
	printf ("\td lmcfg   -- local memory config registers\n");
	printf ("\td regs    -- RR's internal CPU registers\n");
	printf ("\td dma     -- DMA registers\n");
	printf ("\td assist  -- DMA assist (DMA FIFOs) registers\n");
	printf ("\td gca     -- general communications (mbox) area\n");
	printf ("\td         -- all of the above\n");
	printf ("\td mem <SRAM offset> <# words> -- RR memory\n\n");
	printf ("\td trace   -- trace buffer\n\n");
}

int
dump(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    char * cmd;
    int  n;

    if (argc == 1) {
	dump_all();
	return TCL_OK;
    }
    cmd = argv[1];
    n = strlen(cmd);

    /* Try to pick unique starting letters for cmd names
     * to make abbreviations easier.
     */
    if (strncmp (cmd, "pcicfg", n) == 0) {
	dump_pcicfg();
	return TCL_OK;
    } else if (strncmp (cmd, "state", n) == 0) {
	dump_state();
	return TCL_OK;
    } else if (strncmp (cmd, "control", n) == 0) {
	dump_control();
	return TCL_OK;
    } else if (strncmp (cmd, "lmcfg", n) == 0) {
	dump_lmcfg();
	return TCL_OK;
    } else if (strncmp (cmd, "dma", n) == 0) {
	dump_dma();
	return TCL_OK;
    } else if (strncmp (cmd, "assist", n) == 0) {
	dump_assist();
	return TCL_OK;
    } else if (strncmp (cmd, "regs", n) == 0) {
	dump_regs();
	return TCL_OK;
    } else if (strncmp (cmd, "gca", n) == 0) {
	dump_gca();
	return TCL_OK;
    } else if (strncmp (cmd, "mem", n) == 0) {
	int	addr, numwords;
	if (argc != 4) {
	    interp->result = "USAGE: dump mem <SRAM offset> <# words>";
	    return TCL_ERROR;
	}
	addr = strtoul(argv[2], NULL, 0);
	if ((addr < 0) || (addr > RR_MAX_SRAM)) {
	    printf ("\"%s\" is not a legal SRAM offset. Range is 0 to %x\n",
		    argv[2], RR_MAX_SRAM);
	    return TCL_ERROR;
	}
	numwords = strtoul(argv[3], NULL, 0);
	dump_mem(addr, numwords);
	return TCL_OK;
    } else if (strncmp (cmd, "trace", n) == 0) {
	if (dump_trace(slot) == 0)
	    return TCL_OK;
	else
	    return TCL_ERROR;
    }
    printf ("argument \"%s\" not supported.\n", cmd);
    help_d();
    return TCL_ERROR;
}

void
help_q()
{
    printf("q - quit this program (%s) after appropriate cleanup action\n",
	    progname);	    
}


int
quit(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    cleanexit();

    /* UNREACHED */
    return TCL_OK;
}

int
shell(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    pid_t mypid;
    char shellnam[40], *shell, *namep;
    int status;
 
    if ((mypid = fork()) == 0) {
        for (mypid = 3; mypid < 20; mypid++)
           (void) close(mypid);
        (void) signal(SIGINT, SIG_DFL);
        (void) signal(SIGQUIT, SIG_DFL);
        shell = getenv("SHELL");
        if (shell == NULL)
           shell = "/sbin/ksh";
        namep = strrchr(shell,'/');
        if (namep == NULL)
           namep = shell;
        (void) strcpy(shellnam,"-");
        (void) strcat(shellnam, ++namep);
        if (strcmp(namep, "sh") != 0)
           shellnam[0] = '+';
        if (argc > 1) {
           execl(shell,shellnam,"-c",argv[0],(char *)0);
        } else {
           execl(shell,shellnam,(char *)0);
        }
	exit(1);
    }
    if (mypid > 0)
       while (wait(&status) != mypid)
                        ;
    return TCL_OK;
}

void
help_halt()
{
    printf ("h (no args) - halt the Roadrunner's internal CPU.\n\n");
}

/* Halt the RR internal CPU */
int
halt(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    uint_t i;

    if (argc == 1) {
	memsp->misc_host_ctrl_reg = RR_HALT;
	/* Read it back and make sure it's halted */
	while (1) {
	    i = memsp->misc_host_ctrl_reg;
	    if (i & RR_HALTED)
		break;
	}
	memsp->prog_counter_reg;
	printf ("Roadrunner CPU halted, PC = 0x%08x\n",
		memsp->prog_counter_reg);
	return TCL_OK;
    }
    help_h();
    return TCL_ERROR;
}

void
help_ss()
{
    printf ("ss - Single step the Roadrunner CPU\n\n");
}

int
ss(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    uint_t i;
    uint_t pc;

    i = memsp->misc_host_ctrl_reg;
    if (!(i & RR_HALTED)) {
	interp->result = "Error: Roadrunner CPU running - halt first before single-stepping.\n";
	return TCL_ERROR;
    }

    pc = memsp->prog_counter_reg;

    memsp->misc_host_ctrl_reg = (RR_SINGLESTEP|RR_HALT);
    /* this bit is supposed to clear after the singlestep */
    while (memsp->misc_host_ctrl_reg & RR_SINGLESTEP)
	;

#ifdef RR_DEBUG
    printf ("RR_DEBUG: After single step, host_ctrl_reg=%08x, pc=%08x\n", 
	    i = memsp->misc_host_ctrl_reg, memsp->prog_counter_reg);
    /* Processor should be showing halted status and 
     * pc should have advanced. */
    if (pc == memsp->prog_counter_reg)
	printf ("RR_DEBUG: ?!?!?! PC hasn't changed!!\n");
    if (!(i & RR_HALTED))
	printf ("RR_DEBUG: Host control reg doesn't have HALTED bit on.\n");
#endif
    printf ("After singlestep: PC = %08x\n", memsp->prog_counter_reg);
    return TCL_OK;         
}

void
help_r()
{
    printf ("Resume execution of Roadrunner CPU:\n");
    printf ("\tr	- resume at current PC\n");
    printf ("\tr <addr> - load <addr> to PC before resuming\n\n");
}

/*
 * resume operation of RR internal CPU, possibly at a 
 * different instruction
 */

int
resume(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    uint_t i;
    uint_t pc;
    uint_t curpc;

    i = memsp->misc_host_ctrl_reg;

    if (!(i & RR_HALTED)) {
	interp->result = "Error: Roadrunner CPU is already running.\n";
	return TCL_ERROR;
    }

    if (argc > 2) {
	printf ("Too many arguments\n");
	help_r();
	return TCL_ERROR;
    }

    curpc = memsp->prog_counter_reg;
    if (argc == 2) {
	pc = strtoul(argv[1], NULL, 0);
	if (! legal_iaddr(pc)) {
	    printf ("%08x is not a legal instruction for an address.\n");
	    return TCL_ERROR;
	}
	memsp->prog_counter_reg = pc;
    }

    /* if currently halted at a breakpoint, we need to single step
     * past the bp before resuming - RR refuses to resume at a bp
     */
    if (curpc == breakpoint)
	memsp->misc_host_ctrl_reg = RR_HALT | RR_SINGLESTEP;

    memsp->misc_host_ctrl_reg = 0;

    /* 
     * If breakpoint is set, we wait for it to trigger,
     * but just in case it never does, set signal handler
     * to break loop on intr. 
     */     
    if (breakpoint != -1) {
	printf ("Waiting for breakpoint to trigger, ^C to interrupt\n");
	sigset (SIGINT, breakloop);
	if (setjmp(jmp_env) != 0) {
	    /* restore exit handler */
	    sigset(SIGINT, cleanexit);
	    return TCL_OK;
	}
	while (!(memsp->misc_host_ctrl_reg & RR_HALTED))
	    sleep (1);
	sigset(SIGINT, cleanexit);

	pc = memsp->prog_counter_reg;
	if (pc == breakpoint)
	    printf ("Roadrunner CPU halted at breakpoint %08x\n", pc);
	else
	    printf ("Roadrunner CPU halted at PC %08x\n", pc);
    }
    return TCL_OK;
}

void
help_bp()
{
    printf ("Set/clear display breakpoint:\n");
    printf ("\tbp        - display existing breakpoint\n");
    printf ("\tbp <addr> - set breakpoint.\n");
    printf ("\tbp -1     - clear breakpoint.\n");
    printf ("NOTE:  There can be only one breakpoint at a time. Setting\n");
    printf ("       a bp automatically clears any previous bp.\n");
    printf ("       Once a breakpoint is tripped, the processor refuses\n");
    printf ("       to resume execution unless you clear the bp, set a new\n");
    printf ("       bp, or singlestep past the bp before resuming.\n\n");
}

/* Set/clear/display breakpoint */
int
bp(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int i;

    if (argc == 1) {
	if (breakpoint == -1)
	    printf ("There are no breakpoints set\n");
	else
	    printf ("Breakpoint set at %08x\n", breakpoint);
	return TCL_OK;
    }

    if (argc > 2) {
	printf ("Too many arguments to \"bp\" command\n");
	help_bp();
	return TCL_ERROR;
    }

    i = strtoul(argv[1], NULL, 0);

    if (i == -1) {  /* clear breakpoint */
	breakpoint = -1;
	memsp->breakpoint_reg = 1;  /* a "1" in lsb disables BP */
	return TCL_OK;
    } else {	    /* set breakpoint */
	if (!legal_iaddr(i)) {
	    printf (
"\"%s\" is not a legal SRAM address for a f/w instruction.\n", argv[1]);
	    help_bp();
	    return TCL_ERROR;
	}
	memsp->breakpoint_reg = breakpoint = i;
	printf ("Breakpoint %08x set.\n", breakpoint);
	return TCL_OK;
    }
}

void
help_ww()
{
    printf ("ww <address> <value> - Write (4-byte) word to SRAM\n");
    printf ("\t<address> must be a word-aligned offset in SRAM\n\n");
    printf ("\t<value> can be in decimal, octal (0...), or hex (0x...)\n\n");
}

/* Write word to SRAM */
int
ww(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int addr;
    int value;

    if (argc != 3) {
	printf ("Incorrect number of arguments for command \"ww\".\n");
	help_ww();
	return TCL_ERROR;
    }

    addr = strtoul(argv[1], NULL, 0);
    if ((addr < 0) || (addr > RR_MAX_SRAM)) {
	printf ("%s is not a legal address\n", argv[1]);
	return TCL_ERROR;
    }
    if (addr & 3) {
	printf ("address must be word-aligned.\n");
	return TCL_ERROR;
    }

    value = strtoul(argv[2], NULL, 0);

    /* 
     * For a word write we can simply go through the window data reg.
     */
    memsp->win_base_reg = addr;
    memsp->win_data_reg = value;
    assert (memsp->win_data_reg == value);
    return TCL_OK;
}

void
help_ws()
{
    printf ("ws <address> <value> - Write (2-byte) short word to SRAM\n");
    printf ("\t<address> must be a shortword-aligned offset in SRAM\n\n");
    printf ("\t<value> can be in decimal, octal (0...), or hex (0x...)\n\n");
}

/* Write short word to SRAM */
int
ws(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int addr;
    int value;
    int offset;
    ushort_t * sp;

    if (argc != 3) {
	printf ("Incorrect number of arguments for command \"ws\".\n");
	help_ws();
	return TCL_ERROR;
    }

    addr = strtoul(argv[1], NULL, 0);
    if ((addr < 0) || (addr > RR_MAX_SRAM)) {
	printf ("%s is not a legal address\n", argv[1]);
	return TCL_ERROR;
    }
    if (addr & 1) {
	printf ("address must be shortword-aligned.\n");
	return TCL_ERROR;
    }

    value = strtoul(argv[2], NULL, 0);
    if (value & 0xFFFF0000) {
	printf ("value \"%s\" is out of bounds \n");
	return TCL_ERROR;
    }
    /* 
     * Move the SRAM window to the 2KB segment containing this
     * short word.
     */
    memsp->win_base_reg = (addr & 0xFFFFF800);
    offset = (addr & 0x7FF) >> 1;
    sp = (ushort_t *) (memsp->rr_sram_window);
    sp[offset] = value;
    assert (sp[offset] = value);
    return TCL_OK;
}

void
help_wb()
{
    printf ("wb <address> <value> - Write a byte to SRAM\n");
    printf ("\t<address> must be an offset in SRAM\n\n");
    printf ("\t<value> can be in decimal, octal (0...), or hex (0x...)\n\n");
}

/* Write a byte to SRAM */
int
wb(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int addr;
    int value;
    int offset;
    uchar_t * cp;

    if (argc != 3) {
	printf ("Incorrect number of arguments for command \"wb\".\n");
	help_wb();
	return TCL_ERROR;
    }

    addr = strtoul(argv[1], NULL, 0);
    if ((addr < 0) || (addr > RR_MAX_SRAM)) {
	printf ("%s is not a legal SRAM offset\n", argv[1]);
	return TCL_ERROR;
    }

    value = strtoul(argv[2], NULL, 0);
    if (value & 0xFFFFFF00) {
	printf ("value \"%s\" is out of bounds \n");
	return TCL_ERROR;
    }

    /* 
     * Move the SRAM window to the 2KB segment containing this
     * short word.
     */
    memsp->win_base_reg = (addr & 0xFFFFF800);
    offset = (addr & 0x7FF);
    cp = (uchar_t *) (memsp->rr_sram_window);
    cp[offset] = value;
    assert (cp[offset] = value);
    return TCL_OK;
}

void
help_set()
{
    printf ("set <reg> <value> - set a roadrunner register\n");
    printf ("\twhere <reg> can be an address offset or one of the following:\n");
    printf ("\t\tr0-r31\t- one of the roadrunner internal CPU registers\n");

    printf ("\t\thc\t- miscellaneous host control register\n");
    printf ("\t\tlc\t- miscellaneous local control register\n");
    printf ("\t\tpc\t- program counter register\n");
    printf ("\t\tbp\t- breakpoint register\n");
    printf ("\t\tcp\t- CPU priority register\n");

    printf ("\t\twbase\t- window base register\n");
    printf ("\t\twdata\t- window data register\n");

    printf ("\t\thrs\t- HIPPI receive state register\n");
    printf ("\t\thts\t- HIPPI transmit state register\n");
    printf ("\t\thov\t- HIPPI overhead register\n");

    printf ("\t\twhahi\t- Write DMA Host Address High register\n");
    printf ("\t\twhalo\t- Write DMA Host Address Low  register\n");
    printf ("\t\twla\t- Write DMA local address register\n");
    printf ("\t\twlen\t- Write DMA length register\n");
    printf ("\t\twstat\t - Write DMA state register\n");

    printf ("\t\trhahi\t- Read DMA Host Address High register\n");
    printf ("\t\trhalo\t- Read DMA Host Address Low  register\n");
    printf ("\t\trla\t- Read DMA local address register\n");
    printf ("\t\trlen\t- Read DMA length register\n");
    printf ("\t\trstat\t - Read DMA state register\n");

    printf ("\t\trbbase\t- Receive Buffer Base register\n");
    printf ("\t\trbp\t- Receive Buffer Producer register\n");
    printf ("\t\trbc\t- Receive Buffer Consumer\n");

    printf ("\t\ttbbase\t- Transmit Buffer Base register\n");
    printf ("\t\ttbp\t- Transmit Buffer Producer register\n");
    printf ("\t\ttbc\t- Transmit Buffer Consumer register\n");

    printf ("\t\trdp\t- Receive Desc Producer register\n");
    printf ("\t\trdc\t- Receive Desc Consumer register\n");
    printf ("\t\trdr\t- Receive Desc Reference register\n");

    printf ("\t\ttdp\t- Transmit Desc Producer register\n");
    printf ("\t\ttdc\t- Transmit Desc Consumer register\n");
    printf ("\t\ttdr\t- Transmit Desc Reference register\n");
}

/* Set one of the RR registers */
int
set(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    unsigned long reg;
    unsigned long value;
    char * cp;

    if (argc != 3) {
	printf ("USAGE: set <reg> <value>\n");
	return TCL_ERROR;
    }

    value = strtoul (argv[2], 0, 0);

    /* is register specified as an offset in PCI shared mem space ? */
    if (isdigit(argv[1][0])) {
	reg = strtoul (argv[1], 0, 0);
	if ((reg & 3) || (reg > 0x1fc)) {
	    printf ("\"%s\" is not a valid register offset\n", argv[1]);
	    return TCL_ERROR;
	}
	cp = (char *) memsp;
	cp += reg;
	* (unsigned long *) cp = value;
	return TCL_OK;
    }
    /* else register specified by name */

    if (argv[1][0] == 'r') {

	/* CPU regs r0 though r31 */
	if (isdigit(argv[1][1])) {
	    int i;

	    i = atoi(&argv[1][1]);
	    if ((i < 0) || (i > 31)) {
		printf ("\"%s\" is not a valid register value [r0 - r31]\n",
			argv[1]);
		return TCL_ERROR;
	    }
	    memsp->rr_gpr[i] = value;
	    return TCL_OK;
	}

	if (!strcmp (argv[1], "rhahi"))
	    memsp->dma_read_host_hi = value;
	else if (!strcmp (argv[1], "rhalo"))
	    memsp->dma_read_host_lo = value;
	else if (!strcmp (argv[1], "rla"))
	    memsp->dma_read_local = value;
	else if (!strcmp (argv[1], "rlen"))
	    memsp->dma_read_len = value;
	else if (!strcmp (argv[1], "rstat"))
	    memsp->dma_read_state = value;
	else if (!strcmp (argv[1], "rbbase"))	/* recv buf base */
	    memsp->local_mem_cfg_regs[0] = value;
	else if (!strcmp (argv[1], "rbp"))	/* recv buf producer */
	    memsp->local_mem_cfg_regs[1] = value;
	else if (!strcmp (argv[1], "rbc"))	/* recv buf consumer */
	    memsp->local_mem_cfg_regs[2] = value;
	else if (!strcmp (argv[1], "rdp"))	/* recv desc producer */
	    memsp->local_mem_cfg_regs[8] = value;
	else if (!strcmp (argv[1], "rdc"))	/* recv desc consumer */
	    memsp->local_mem_cfg_regs[9] = value;
	else if (!strcmp (argv[1], "rdr"))	/* recv desc reference */
	    memsp->local_mem_cfg_regs[10] = value;
	else {
	    printf ("\"%s\" is not a valid register name\n", argv[1]);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    else if (argv[1][0] == 'w') {
	if (!strcmp (argv[1], "whahi"))
	    memsp->dma_write_host_hi = value;
	else if (!strcmp (argv[1], "whalo"))
	    memsp->dma_write_host_lo = value;
	else if (!strcmp (argv[1], "wla"))
	    memsp->dma_write_local = value;
	else if (!strcmp (argv[1], "wlen"))
	    memsp->dma_write_len = value;
	else if (!strcmp (argv[1], "wstat"))
	    memsp->dma_write_state = value;
	else if (!strcmp (argv[1], "wbase"))
	    memsp->win_base_reg = value;
	else if (!strcmp (argv[1], "wdata"))
	    memsp->win_data_reg = value;
	else {
	    printf ("\"%s\" is not a valid register name\n", argv[1]);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    else if (argv[1][0] == 't') {
	if (!strcmp (argv[1], "tbbase"))	/* xmit buf base */
	    memsp->local_mem_cfg_regs[4] = value;
	else if (!strcmp (argv[1], "tbp"))	/* xmit buf producer */
	    memsp->local_mem_cfg_regs[5] = value;
	else if (!strcmp (argv[1], "tbc"))	/* xmit buf consumer */
	    memsp->local_mem_cfg_regs[6] = value;
	else if (!strcmp (argv[1], "tdp"))	/* xmit desc producer */
	    memsp->local_mem_cfg_regs[12] = value;
	else if (!strcmp (argv[1], "tdc"))	/* xmit desc consumer */
	    memsp->local_mem_cfg_regs[13] = value;
	else if (!strcmp (argv[1], "tdr"))	/* xmit desc reference */
	    memsp->local_mem_cfg_regs[14] = value;
	else {
	    printf ("\"%s\" is not a valid register name\n", argv[1]);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    else if (argv[1][0] == 'h') {
	if (!strcmp (argv[1], "hc"))
	    memsp->misc_host_ctrl_reg = value;
	else if (!strcmp (argv[1], "hrs"))
	    memsp->misc_host_ctrl_reg = value;
	else if (!strcmp (argv[1], "hts"))
	    memsp->misc_host_ctrl_reg = value;
	else if (!strcmp (argv[1], "hov"))
	    memsp->misc_host_ctrl_reg = value;
	else {
	    printf ("\"%s\" is not a valid register name\n", argv[1]);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    else if (!strcmp (argv[1], "lc")) {
	    memsp->misc_local_ctrl_reg = value;
	    return TCL_OK;
    }
    else if (!strcmp (argv[1], "pc")) {
	    memsp->prog_counter_reg = value;
	    return TCL_OK;
    }
    else if (!strcmp (argv[1], "bp")) {
	    memsp->breakpoint_reg = value;
	    return TCL_OK;
    }
    else if (!strcmp (argv[1], "cp")) {	/* CPU priority reg */
	    memsp->local_mem_cfg_regs[3] = value;
	    return TCL_OK;
    }
    printf ("\"%s\" is not a valid register name\n", argv[1]);
    return TCL_ERROR;
}

void
help_cfg()
{
printf ("cfg <reg_offset> <value>\n");
printf ("\t<reg_offset> is offset of a PCI Config space register.\n");
printf ("\t<value> is the value to write to it.\n");
printf ("\t\tThis command goes through the 4K shared memory window\n");
printf ("\t\trather than the PCI config space, so registers which\n");
printf ("\t\tare read-only in PCI config space are writeable with\n");
printf ("\t\tthis command.\n");
}

int
cfg(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int offset;
    uint_t value;

    if (argc != 3) {
	fprintf (stderr, "USAGE: cfg <reg_offset> <value>");
	return TCL_ERROR;
    }

    offset = strtoul (argv[1],0,0);
    value  = strtoul (argv[2],0,0);

    if (offset & 3) {
	fprintf (stderr, 
		"Register offset \"%s\" is invalid (not word-aligned)\n",
		 argv[1]);
	return TCL_ERROR;
    }
    if (offset > 0x3c) {
	fprintf (stderr, "Invalid offset, valid range is 0 to 0x3f.\n");
	return TCL_ERROR;
    }

    memsp->cfg_regs[(offset>>2)] = value;

    return TCL_OK;
}

#ifndef G2P_SIM
/*
 * Quick and dirty means of writing/reading LINC address space.
 * Real basic, no pretty printing, no range-checking.
 * rrdbg's focus is the RR, not the LINC, use rpw3's tool to do
 * fancy stuff with the LINC.
 */
void
help_lww()
{
printf ("lww <linc_addr> <value>\n");
printf ("\t<linc_addr> is offset in LINC PPCI address space.\n");
printf ("\t<value> is the value to write to it.\n");
printf ("\t\tThis command goes through the usrpci mem32 space to\n");
printf ("\t\tset any PPCI-accessible register, SDRAM or EEPROM location.\n");
}

int
lww(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int offset;
    uint_t value;

    if (argc != 3) {
	fprintf (stderr, "USAGE: lww <linc_addr> <value>");
	return TCL_ERROR;
    }

    offset = strtoul (argv[1],0,0);
    value  = strtoul (argv[2],0,0);

    if (offset & 3) {
	fprintf (stderr, 
		"Linc address \"%s\" is invalid (not word-aligned)\n",
		 argv[1]);
	return TCL_ERROR;
    }

    *(__uint32_t *) (lmemsp + offset) = value;

    return TCL_OK;
}

void
help_lwl()
{
printf ("lwl  <linc_addr> <hi_value> <lo_value>");
printf ("\t<linc_addr> is offset in LINC PPCI address space.\n");
printf ("\t<hi_value> is the value to write to the upper 32 bits of the 64 bit write\n");
printf ("\t<lo_value> is the value to write to the lower 32 bits of the 64 bit write\n");
printf ("\t\tThis command goes through the usrpci mem32 space.\n");
printf ("\t\tIt is only valid for mailbox writes or SDRAM accesses (NOT Linc registers\n");
}

int
lwl(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int offset;
    uint_t hival, loval;
    __uint64_t value;

    if (argc != 4) {
	fprintf (stderr, "USAGE: lwl <linc_addr> <hi_value> <lo_value>");
	return TCL_ERROR;
    }

    offset = strtoul (argv[1],0,0);
    hival  = strtoul (argv[2],0,0);
    loval  = strtoul (argv[3],0,0);

    if (offset & 7) {
	fprintf (stderr, 
		"Linc address \"%s\" is invalid (not doubleword-aligned)\n",
		 argv[1]);
	return TCL_ERROR;
    }

    value = (__uint64_t)hival << 32;
    value |= loval;
    *(__uint64_t *) (lmemsp + offset) = value;

    return TCL_OK;
}

void
help_lrw()
{
    printf ("lrw <linc_addr> [<count>]\n");
    printf ("\t<linc_addr> is offset in LINC PPCI address space.\n");
    printf ("\t<count> is number of sequential words to read. Default is 1.\n");
    printf ("\t\tThis command goes through the usrpci mem32 space to\n");
    printf ("\t\tread any PPCI-accessible register, SDRAM or EEPROM location.\n");
}

int
lrw(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    int offset;
    int	count;
    __uint32_t * ip;
    int i = 0;

    if ((argc > 3) || (argc < 2)) {
	fprintf (stderr, "USAGE: lrw <linc_addr> [<count>]");
	return TCL_ERROR;
    }

    offset = strtoul (argv[1],0,0);
    if (argc == 3) 
	count = strtoul (argv[2],0,0);
    else
	count = 1;

    if (offset & 3) {
	fprintf (stderr, 
		"Linc address \"%s\" is invalid (not word-aligned)\n",
		 argv[1]);
	return TCL_ERROR;
    }

    ldump_mem((u_int)offset, count);
    return TCL_OK;
}

void
help_lset()
{
    printf ("NOT IMPLEMENTED YET\n");
    printf ("lset <reg_name> <value> - set a linc register\n");
    printf ("\twhere <reg> can be an address offset or one of the following:\n");
    printf ("\t\tNOT IMPLEMENTED YET\n\n");
    printf ("\t<reg_name> is a valid linc register abbreviated name, as defined in\n");
    printf ("\tLINC ASIC Specification, Rev 2.4, Table 18, page 62.\n");
    printf ("\tDMA register abbreviated names are taken from section names\n");
    printf ("\t\tThis command goes through the usrpci mem32 space to\n");
    printf ("\t\tset a PPCI-accessible linc register.\n");
}

int
lset(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    printf ("lset\n");
    return TCL_OK;
}


/* lrefresh - write sequence needed for refreshing LINC SDRAM */
void
help_lrefresh()
{
printf ("lrefresh\n");
printf ("\tCommand takes no args, writes sequence needed to LINC's\n");
printf ("\tBMC and BMO registers to prep SDRAM. Then writes all of\n");
printf ("\tSDRAM, to initialize parity.\n");
}
int
lrefresh(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    volatile uint_t * bmo;
    volatile uint_t * ip;
    int	    i;

    if (argc != 1) {
	fprintf (stderr, "USAGE: lrefresh");
	return TCL_ERROR;
    }
    * (volatile uint_t *) (lmemsp + LINC_BUFMEM_CONTROL) = 0x0410d2ba;
    bmo = (volatile uint_t *) (lmemsp + LINC_BUFMEM_OPERATION);

    *bmo = 0x1000;	    /* precharge */
    while ((*bmo) & 0x1000)   /* wait for completion */
	;

    *bmo = 0x823;	    /* mode-set, mode-lat=2,mode-burst=3 */
    while ((*bmo) & 0x800)    /* wait for completion */
	;

    *bmo = 0x2000;	    /* refresh */
    while ((*bmo) & 0x2000)   /* wait for completion */
	;

    *bmo = 0x2000;	    /* refresh again */
    while ((*bmo) & 0x2000)   /* wait for completion */
	;

    /* clear all 4MB of SDRAM */
    i = 0;
    ip = (volatile uint_t *)lmemsp;
    while (i++ < 0x100000)
	*ip++ = 0;

    return TCL_OK;
}

/* ldload : Download a file into LINC bufmem. */
void
help_ldload()
{
printf ("ldload <filename> <startaddress>\n");
printf ("Downloads a file into LINC SDRAM.\n");
printf ("\t<filename> is a file containing hex strings, 1 word per line.\n");
printf ("\t<startaddress> is the address in SDRAM to writing to.\n");
}
int
ldload(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    uint_t startaddr;
    int offset, line, len, i;
    char    aline[256];
    uint_t * uintp;
    char *buf, *from, *to;
    FILE *fp;

    if ((argc < 2) || (argc > 3)) {
	fprintf (stderr, "USAGE: ldload <fname> <startaddr>");
	return TCL_ERROR;
    }

    if (argc == 3) {
	startaddr = strtoul (argv[2], 0, 0);
	if ((startaddr == 0) || (startaddr & 3)) {
	    fprintf (stderr, "starting address has to be non-zero and word aligned\n");
	    return TCL_ERROR;
	}
    }
    else
	startaddr = 4;

    fp = fopen (argv[1], "r");
    if (fp == 0) {
	fprintf (stderr, "Couldn't open file %s: %s\n", 
		 argv[1], strerror(errno));
	return TCL_ERROR;
    }

    line = 0;
    buf = malloc (64 * 1024);
    uintp = (uint_t *) buf;
    len = 0;

    while (fgets (aline, sizeof(aline), fp)) {
	if (++line > (16*1024)) {
	    fprintf (stderr, "File exceeds 64KB\n");
	    free (buf);
	    return TCL_ERROR;
	}
	if (strlen (aline) != 9) {
	    fprintf (stderr, "File %s line %d: bad format\n", 
			     argv[1], line);
	    free (buf);
	    return TCL_ERROR;
	}
	*uintp++ = hexToUInt(aline);
	len += 4;
    }

    printf ("Begin download of %d(0x%x) bytes to SDRAM address 0x%x\n",
	    len, len, startaddr);
    printf ("bcopy(%x,%x,%x)\n", buf, lmemsp+startaddr, len);
    printf ("Firmware downloaded\n");

    free (buf);
    return TCL_OK;    
}

void
pp_strace_q(u_int qual)
{


}

void
pp_dtrace_q(u_int qual)
{



}

/* Pretty-print one trace entry for source Linc */
u_int
ld_st(int ix, volatile trace_t *trace, u_int last_time)
{
    int	opcode;
    u_int time;

    /* mult by 4 because fw shifts time by 4 bits to not loose the 
     * high order bits due to the opcode.
     */
    time = (u_int)((trace->s.time/(((float)CPU_HZ/1000.)/2000.)) * 16);
    opcode = trace->s.op;
    printf ("    [0x%03x, 0x%07x][+%08d] : ", ix, trace->s.time, time-last_time);
    printf ("0x%8x,0x%8x,0x%8x: ", trace->i[1], trace->i[2], trace->i[3]);

    switch (opcode) 
	{
	  case TOP_DMA0: 
	  case TOP_DMA1:
	  case TOP_DMA_B2H:
	    if (opcode == TOP_DMA0) {
		if (trace->s.arg2 == 0x3fff0000)
		    printf ("DMA_FLSH:");
		else
		    printf ("DMA0:");
	    }
	    else if (opcode == TOP_DMA1) {
		if (trace->s.arg2 == 0x3fff0000)
		    printf ("DMA_FLSH:\n");
		else
		    printf ("DMA1:");
	    }
	    else
		printf ("DMAB2H:");
	    switch (trace->s.arg0) {
	      case T_DMA_MISC:       printf("MISC      "); break;
	      case T_DMA_BP_DATA:    printf("BP_DATA   "); break;
	      case T_DMA_FP_D1:      printf("FP_D1     "); break;
	      case T_DMA_FP_D2:      printf("FP_D2     "); break;
	      case T_DMA_FP_DESCHDR: printf("FP_DESCHD "); break;
	      case T_DMA_FP_DESCD2:  printf("FP_DESCD2 "); break;
	      case T_DMA_BP_DESC:    printf("BP_DESC   "); break;
	      case T_DMA_GET_FPBUFS: printf("GET_FPBUF "); break;
	      case T_DMA_IP_SMBUF:   printf("IP_SMBUF  "); break;
	      case T_DMA_IP_LGBUFS:  printf("IP_LGBUFS "); break;
	      case T_DMA_D2B_DATA:   printf("D2B_DATA  "); break;
	      case T_DMA_D2B_DESC:   printf("D2B_DESC  "); break;
	      default: printf("%d", trace->s.arg0);
		break;
	    }
	    printf(" local=0x%08x, host_lo=0x%08x,len=0x%08x\n",
		   trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	        
	  case TOP_IDMA:
	    printf("IDMA: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_MISC:
	    printf("MISC:");
	    switch(trace->s.arg0) {
	      case T_MISC_INIT:
		printf("INIT\n");
		break;
	      case T_MISC_INTR:
		printf("INTR TO HOST\n");
		break;
	      case T_MISC_LEDS:
		printf("SET LEDS\n");
		break;
	      case T_MISC_ULP:
		printf("ASSGN_ULP enable: %d, fp stack: 0x%x, ulp: 0x%x\n", trace->s.arg1,
		       trace->s.arg2, trace->s.arg3);
		break;
	      case T_MISC_DIE:
	        printf("DIE: code = %d, auxdata = 0x%x (%d)\n", trace->s.arg2, 
		       trace->s.arg3, trace->s.arg3);
		break;
	      case T_TIMER_INFO: 
		printf("TIMER_INFO timer %d took %.2f usec\n",
		       trace->s.arg1,
		       (trace->s.arg2/(((float)CPU_HZ/1000.)/2000.)) * 16);
	        break;
	      case T_TIMER_ERR: 
		printf("TIMER_ERROR timer %d had time %.2f usec and set %d\n",
		       trace->s.arg1,
		       (trace->s.arg2/(((float)CPU_HZ/1000.)/2000.)) * 16,
		       trace->s.arg3);
	        break;
	      default:
		printf("XXXX %d: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		       opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    }
	    break;

	  case TOP_WBLK:
	    printf("WBLK: INVALID op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_SBLK:
	    {
	      u_int flags = trace->s.arg0;
	      printf("SBLK: ");
	      printf("len=0x%06x,stk=%d,d2bs=%d, fbrst=%3d,dp=0x%06x, ",
		     trace->s.arg1, trace->s.arg2>>24, trace->s.arg2 & 0xffffff, 
		     trace->s.arg3>>24, trace->s.arg3 & 0xffffff);
	      printf("Flags ");
	      if (flags & SBLK_NEOP) printf(" NEOP");
	      if (flags & SBLK_NEOC) printf(" NEOC");
	      if (flags & SBLK_NACK) printf(" NACK");
	      if (flags & SBLK_BEGPC) printf(" BEGPC");
	      if (flags & SBLK_SOC) printf(" SOC");
	      if (flags & SBLK_BP) printf(" BP");
	      if (flags & SBLK_REM) printf(" REM");
	      printf("\n");
	      
	      break;
	    }

	  case TOP_L2RR:
	    {
		int i = 0;
		printf("L2RR: ");
		if (trace->s.op & SRRD_MB) i += printf(" MORE");
		if (trace->s.op & SRRD_DD) i += printf(" DUMMY");
		if (trace->s.op & SRRD_SI) i += printf(" SAME_IFLD");
		if (trace->s.op & SRRD_PC) i += printf(" PERM_CONN");
		if (trace->s.op & SRRD_WN) i += printf(" WRAP_NEXT");
		if (trace->s.op & SRRD_CC) i += printf(" CONT_CONN");
		if (i >40)
		    printf("\n");
		printf ("len=0x%x,Ifield=0x%08x,dp=0x%08x\n",
			trace->s.arg1, trace->s.arg2, trace->s.arg3);
		break;
	    }
	  case TOP_RR2L:
	    {
		int flags = trace->s.arg1;

		printf("SACK_ST:    "); 
		switch(trace->s.arg0) {
		  case SW_IDLE: printf(" ST_IDLE"); break;
		  case SW_NEOC: printf(" ST_NEOC"); break;
		  case SW_NEOP: printf(" ST_NEOP"); break;
		  case SW_ERR: printf(" ST_ERR"); break;
		}
		printf(" ack_flags: ");
		if (flags & SBLK_NEOP) printf(" NEOP");
		if (flags & SBLK_NEOC) printf(" NEOC");
		if (flags & SBLK_NACK) printf(" NACK");
		if (flags & SBLK_BEGPC) printf(" BEGPC");
		if (flags & SBLK_SOC) printf(" SOC");
		if (flags & SBLK_BP) printf(" BP");
		if (flags & SBLK_REM) printf(" REMAINDER");

		printf("rr2l_flags= ");
		flags = trace->s.arg2 & SRRS_OP_MASK;
		if (flags == SRRS_OP_XMIT_OK)    printf(" XMIT_OK");
		if (flags  == SRRS_OP_DESC_FLUSHED) printf(" DESC_FLUSHED");
		if (flags == SRRS_OP_CONN_TIMEO) printf(" CONN_TIMEO");
		if (flags == SRRS_OP_DST_DISCON) printf(" DST_DISCON");
		if (flags == SRRS_OP_CONN_REJ)   printf(" CONN_REJ");
		if (flags == SRRS_OP_SRC_PERR)   printf(" SRC_PERR");
		printf (" l2rr_addr=0x%x,a3=%x\n",
			trace->s.arg2 & SRRS_ADDR_MASK, trace->s.arg3);	
		break;
	    }

	  case TOP_HFSM_ST:
	    {
		int flags = trace->s.arg1 >> 16;
		int msg_flags = trace->s.arg1 & 0xffff;

		printf("HFSM_ST: ");
		switch (trace->s.arg0) {
		  case SH_IDLE: printf(" IDLE"); break;
		  case SH_D2B_ACTIVE: printf(" D2B_ACTIVE"); break; 	
		  case SH_D2B_FULL: printf(" D2B_FULL"); break; 	
		  case SH_BP_ACTIVE: printf(" BP_ACTIVE"); break;
		  case SH_BP_FULL: printf(" BP_FULL"); break;
		}
		if (flags & SF_NEW_D2B) printf(" NEW_D2B");

		if (msg_flags & SMF_READY) printf(" READY");
		if (msg_flags & SMF_PENDING) printf(" PENDING");
		if (msg_flags & SMF_MIDDLE) printf(" MIDDLE");
		if (msg_flags & SMF_NEED_IFIELD) printf(" NEED_IFIELD");
		if (msg_flags & SMF_NEOP) printf(" NEOP");
		if (msg_flags & SMF_NEOC) printf(" NEOC");
		if (msg_flags & SMF_CS_VALID) printf(" CS_VALID");
		printf("chnks=%d, chnks_left=%d, top 16 bits of cur_job=0x%x, rem_len=0x%x\n",
		       trace->s.arg2 >> 16,  trace->s.arg2 & 0xffff,
		       trace->s.arg3 >> 16,  trace->s.arg3 & 0xffff);
		break;
	    }

	  case TOP_WFSM_ST:
	    {
		int flags = trace->s.arg1;

		printf("WFSM:  ");
		switch(trace->s.arg0) {
		  case SW_IDLE: printf("IDLE "); break;
		  case SW_NEOC: printf("NEOC "); break;
		  case SW_NEOP: printf("NEOP "); break;
		  case SW_ERR: printf("ERR "); break;
		}
		if (flags & SBLK_NEOP) printf(" NEOP");
		if (flags & SBLK_NEOC) printf(" NEOC");
		if (flags & SBLK_NACK) printf(" NACK");
		if (flags & SBLK_BEGPC) printf(" BEGPC");
		if (flags & SBLK_SOC) printf(" SOC");
		if (flags & SBLK_BP) printf(" BP");
		if (flags & SBLK_REM) printf(" REMAINDER");
		printf("\n");
		break;
	    }
	  case TOP_BP_DESC:
	    printf("BP_DESC: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_D2B:
	    {
		hip_d2b_t h;
		memcpy ((void*)&h, (const void *)&trace->s.arg2, sizeof(hip_b2h_t));
		printf("D2B_HEAD    "); 
		if (h.hd.flags & HIP_D2B_NEOC)
		    printf("NEOC ");
		if (h.hd.flags & HIP_D2B_NEOP)
		    printf("NEOP ");
		if (h.hd.flags & HIP_D2B_IFLD)
		    printf("IFLD ");
		if (h.hd.flags & HIP_D2B_NACK)
		    printf("NACK ");
		if (h.hd.flags & HIP_D2B_BEGPC)
		    printf("BEGPC ");
		printf("chk_off=%d, chunks=%d, stack=%d, fburst=%d\n",
		       h.hd.sumoff, h.hd.chunks, h.hd.stk, h.hd.fburst);
		break;
	    }
	  case TOP_PKT_DROP:
	    printf("PKT_DROP: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  default: printf ("Unknown trace opcode=0x%x: a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
			 opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	}
    return(time);
}

/* Pretty-print one trace entry for destination Linc */
u_int
ld_dt(int ix, volatile trace_t *trace, u_int last_time)
{
    int	opcode;
    u_int time;

    /* mult by 4 because fw shifts time by 4 bits to not loose the 
     * high order bits due to the opcode.
     */
    time = (u_int)((trace->s.time/(((float)CPU_HZ/1000.)/2000.)) * 16);
    opcode = trace->s.op;
    printf ("    [0x%03x, 0x%07x][+%08d] : ", ix, trace->s.time, time-last_time);

    switch (opcode) 
	{
	  case TOP_DMA0: 
	  case TOP_DMA1:
	  case TOP_DMA_B2H:
	    if (opcode == TOP_DMA0)
		printf ("DMA0:");
	    else if (opcode == TOP_DMA1)
		printf ("DMA1:");
	    else 
		printf ("DMA :B2H       ");
	    switch (trace->s.arg0) {
	      case T_DMA_MISC:       printf("MISC      "); break;
	      case T_DMA_BP_DATA:    printf("BP_DATA   "); break;
	      case T_DMA_FP_D1:      printf("FP_D1     "); break;
	      case T_DMA_FP_D2:      printf("FP_D2     "); break;
	      case T_DMA_FP_DESCHDR: printf("FP_DESCHD "); break;
	      case T_DMA_FP_DESCD2:  printf("FP_DESCD2 "); break;
	      case T_DMA_BP_DESC:    printf("BP_DESC   "); break;
	      case T_DMA_GET_FPBUFS: printf("GET_FPBUF "); break;
	      case T_DMA_IP_SMBUF:   printf("IP_SMBUF  "); break;
	      case T_DMA_IP_LGBUFS:  printf("IP_LGBUFS "); break;
	      case T_DMA_D2B_DATA:   printf("D2B_DATA  "); break;
	      case T_DMA_D2B_DESC:   printf("D2B_DESC  "); break;
	      default: printf("%d", trace->s.arg0);
	    }
	    printf(" local=0x%08x, host_lo=0x%08x,len=0x%08x\n",
		   trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	        
	  case TOP_IDMA:
	    printf("IDMA: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_MISC:
	    printf("MISC:");
	    switch(trace->s.arg0) {
	      case T_MISC_INIT:
		printf("INIT\n");
		break;
	      case T_MISC_INTR:
		printf("INTR TO HOST\n");
		break;
	      case T_MISC_LEDS:
		printf("SET LEDS\n");
		break;
	      case T_MISC_ULP:
		printf("DASSGN_ULP enable: %d, fp stack: 0x%x, ulp: 0x%x\n", trace->s.arg1,
		       trace->s.arg2, trace->s.arg3);
		break;
	      case T_MISC_DIE:
	        printf("DIE: code = %d, auxdata = 0x%x (%d)\n", trace->s.arg2, 
		       trace->s.arg3, trace->s.arg3);
		break;
	      case T_TIMER_INFO: 
		printf("TIMER_INFO timer %d took %.2f usec\n",
		       trace->s.arg1,
		       (trace->s.arg2/(((float)CPU_HZ/1000.)/2000.)) * 16);
	        break;
	      case T_TIMER_ERR: 
		printf("TIMER_ERROR timer %d had time %.2f usec and set %d\n",
		       trace->s.arg1,
		       (trace->s.arg2/(((float)CPU_HZ/1000.)/2000.)) * 16,
		       trace->s.arg3);
	        break;
	      default:
		printf("XXXX %d: op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		       opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    }
	    break;

	  case TOP_WBLK:
	    printf("WBLK:  op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_SBLK:
	    
	    printf("SBLK: INVALID - op=%d, a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;

	  case TOP_L2RR:
	    printf("L2RR: descp=0x%x, dp=0x%x\n", trace->s.arg2, trace->s.arg3);
	    break;
	  case TOP_RR2L:
	    {
		int op = trace->s.arg2 & ~DRRD_ERR_MASK;
		int err = trace->s.arg2 & DRRD_ERR_MASK;
		int i = 0;

		printf("RR2L: ");
		printf ("flags=");
		if (op & DRRD_IFP)           i += printf(" IFP");
		if (op & DRRD_EOP)           i += printf(" EOP");
		if (op & DRRD_PAUSE_NO_DESC) i += printf(" PSE_NODSC");
		if (op & DRRD_PAUSE_NO_BUFF) i += printf(" PSE_NOBUF");
		if(i > 20) printf("\n\t");
		else printf(", err=");
		if (err & DRRD_NO_PKT_RCV)    i += printf(" NO_PKTRCV");
		if (err & DRRD_NO_BURST_RCV)  i += printf(" NO_BRTRCV");
		if (err & DRRD_LAST_WORD_ODD) i += printf(" LST_WDODD");
		if (err & DRRD_FBURST_SHORT)  i += printf(" FBRST_SHT");
		if (err & DRRD_READY_ERR)     i += printf(" RDY_ERR ");
		if (err & DRRD_LINKRDY_ERR)   i += printf(" LNRY_ERR");
		if (err & DRRD_FLAG_ERR)      i += printf(" FLAG_ERR");
		if (err & DRRD_FRAMING_ERR)   i += printf(" FRAM_ERR");
		if (err & DRRD_LLRC_ERR)      i += printf(" LLRC_ERR");
		if (err & DRRD_PAR_ERR)       i += printf(" PAR_ERR ");
		if(i > 60) printf("\n\t");
		if (err & DRRD_BURST_SIZE_MASK) {
		    printf(" BRST_SZ_MASK");
		}
		if (err & DRRD_ERR_ST_MASK)  {
		    printf(" ERR_ST_MASK");
		}
		printf (", len=0x%08x, dp=0x%08x\n", trace->s.arg1, trace->s.arg3);
	    }

	  case TOP_HFSM_ST:
	    {
		int flags = trace->s.arg1 >> 16;
		int dma_flags = trace->s.arg1 & 0xffff;

		printf("HFSM: ");
		switch(trace->s.arg0) {
		  case DH_IDLE: printf("IDLE "); break; 
		  case DH_WAIT_FPHDR: printf("WAIT_FPHDR "); break;
		  case DH_WAIT_FPBUF: printf("WAIT_FPBUF "); break;
		  case DH_FP: printf("FP "); break;
		  case DH_LE: printf("LE "); break;
		  case DH_BP: printf("BP "); break;
		  case DH_FEOP: printf("FEOP "); break;
 		  default: printf("XXX=%d\n", trace->s.arg0);
		}
		printf("flags: ");
		if (flags & DF_NEOP) printf("NEOP ");
		if (flags & DF_HOST_KNOWS) printf("HOST_KN ");
		if (flags & DF_NEW_D2B) printf("NEW_D2B ");
		if (flags & DF_PENDING) printf("PENDING ");
		if (flags & DF_STUFFING) printf("STUFFING ");
		if (flags & DF_BP_DESC) printf("BP_DESC ");
		printf("dma_flags: ");
		if (dma_flags & DDMA_START_PKT) printf("S_PKT ");
		if (dma_flags & DDMA_CHAIN_CS) printf("CH_CS ");
		if (dma_flags & DDMA_SAVE_CS) printf("SA_CS ");

		printf("remlen=0x%x, curlen=0x%x, CSmBLn=0x%x, CLgB=0x%x\n",
		       trace->s.arg2 >> 24,  trace->s.arg2 & 0xffffff,
		       trace->s.arg3 >> 16,  trace->s.arg3 & 0xffff);
		
		break;
	    }

	  case TOP_WFSM_ST:
	    {
		int op = trace->s.arg3 & ~DRRD_ERR_MASK;
		int err = trace->s.arg3 & DRRD_ERR_MASK;
		int i =0;
		printf("WFSM_ST    "); 
		switch (trace->s.arg0) {
		  case DW_NEOP: printf("NEOP "); break;
		  case DW_NEOC: printf("NEOC "); break;
		  case DW_NO_FP: printf("NOFP "); break;
		}
		printf("rr2l.addr=0x%x, rr2l.len=0x%x, ",trace->s.arg1, trace->s.arg2);
		printf ("flags=");
		if (op & DRRD_IFP)           i += printf(" IFP");
		if (op & DRRD_EOP)           i += printf(" EOP");
		if (op & DRRD_PAUSE_NO_DESC) i += printf(" PSE_NODSC");
		if (op & DRRD_PAUSE_NO_BUFF) i += printf(" PSE_NOBUF");
		if(i > 20) printf("\n\t");
		else printf(", err=");
		if (err & DRRD_NO_PKT_RCV)    i += printf(" NO_PKTRCV");
		if (err & DRRD_NO_BURST_RCV)  i += printf(" NO_BRTRCV");
		if (err & DRRD_LAST_WORD_ODD) i += printf(" LST_WDODD");
		if (err & DRRD_FBURST_SHORT)  i += printf(" FBRST_SHT");
		if (err & DRRD_READY_ERR)     i += printf(" RDY_ERR ");
		if (err & DRRD_LINKRDY_ERR)   i += printf(" LNRY_ERR");
		if (err & DRRD_FLAG_ERR)      i += printf(" FLAG_ERR");
		if (err & DRRD_FRAMING_ERR)   i += printf(" FRAM_ERR");
		if (err & DRRD_LLRC_ERR)      i += printf(" LLRC_ERR");
		if (err & DRRD_PAR_ERR)       i += printf(" PAR_ERR ");
		if(i > 60) printf("\n\t");
		if (err & DRRD_BURST_SIZE_MASK) {
		    printf(" BRST_SZ_MASK");
		}
		if (err & DRRD_ERR_ST_MASK)  {
		    printf(" ERR_ST_MASK");
		}
		printf("\n");
		break;
	    }
	    
	  case TOP_BP_DESC:
	    printf("BP_DESC    "); 
	    printf ("a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		    trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;

	  case TOP_D2B:
	    if (trace->s.arg0 == T_D2B_FPHDR) {
	        printf(" D2B - FPHDR");
		printf(" d2bp=0x%6x, haddr=0x%08x, op=0x%x, stk=%d, len=%06d bytes\n", 
		       trace->s.arg1,
		       trace->s.arg2, 
		       (trace->s.arg3>>8) & 0xf0,
		       (trace->s.arg3>>8) & 0x0f,
		       trace->s.arg3>>16);
	    }
	    if (trace->s.arg0 == T_D2B_FPBUF) {
	        printf("D2B - FPBUF");
		printf(" d2bp=0x%6x, haddr=0x%08x, op=0x%x, stk=%d, len=%06d c2b's\n", 
		       trace->s.arg1,
		       trace->s.arg2, 
		       (trace->s.arg3>>8) & 0xf0,
		       (trace->s.arg3>>8) & 0x0f,
		       (trace->s.arg3>>16)/sizeof(hip_c2b_t));
	    }
	    break;
	    
	  case TOP_PKT_DROP:
	    printf("PKT_DROP    "); 
	    printf ("a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
		    trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	    break;

	  default: printf ("Unknown trace opcode=0x%x: a0=%01x,a1=%07x,a2=%08x,a3=%08x\n",
			   opcode, trace->s.arg0, trace->s.arg1, trace->s.arg2, trace->s.arg3);
	}
    return(time);
}

/* Pretty-print one trace entry for dst Linc */

/*
 * Dump Linc's trace buffer. 
 */
int
ldump_trace()
{
    int	    i, j, trace_ix;
    volatile trace_t *ltp;
    volatile trace_t *tp, *tbuf;
    u_int last_time = 0;


    if ((tbuf = malloc(sizeof(trace_t)*LINC_TRACE_BUF_SIZE)) == 0)    {
	printf ("Couldn't malloc trace buffer!\n");
	return -1;
    }

    if (slot)
        printf ("Source Linc trace buffer:\n");
    else
        printf ("Destination Linc trace buffer:\n");

    printf("Doing a data cache flush to get complete Trace\n");
    if (intr_4640() == TCL_ERROR) {
	printf("WARNING - CACHE FLUSH FAILED - POSSIBLY INCOMPLETE STATE IN TRACE BUFFER\n");
    }
	
    memcpy ((char *)tbuf, 
	    (const void *) ltracep, sizeof(trace_t) * LINC_TRACE_BUF_SIZE);
    printf("\tbase of trace buffer = 0x%x\n", LINC_TRACE_BASE);

    if (lstate->flags & FLAG_GOT_INIT) {
	/* ltp = linc address of trace tail (put pointer) */
	printf("Firmware is initialized. Getting current head from State struct\n");
	ltp = *(volatile trace_t**)
	    ((~0xc0000000 & (u_int)lstate->traceput) + (u_int)lmemsp);

	j = ltp - lstate->trace_basep;
    
	if (j > LINC_TRACE_BUF_SIZE) {
	    printf("WARNING: calculated trace index out of range = %d - setting to 0\n", j);
	    tp = tbuf;
	    trace_ix = j = 0;
	}
	else {
	    /* xlate to host address space */
	    tp = tbuf + j;
	    trace_ix = j;
	}
    }
    else {
	int i;
	u_int last_time = 0;
	printf("WARNING: firmware not up or hasn't gotten hcmd_init.\n");
	printf("\ttrying to derive queue head from time stamp\n");
	printf("\tIf data looks random, then probably the firmware just needs\n");
	printf("\tto be initialized (hipcntl startup). If data is inconsistent\n");
	printf("\tthen the cache flush was not successful\n");

	intr_4640();		/* flush the data cache */

	trace_ix = 0;
	tp = tbuf;
	last_time = tp->s.time;
	for (i = 0; i <= LINC_TRACE_BUF_SIZE; i++) {
	    u_int time = tp->s.time;
	    if (time < last_time) {
		trace_ix = i;
		break;
	    }
	    last_time = time;
	    tp++;
	    if (tp == tbuf + LINC_TRACE_BUF_SIZE)
		tp = tbuf;
	}
	j = trace_ix;
	tp = tbuf + trace_ix;
    }

    while (j < LINC_TRACE_BUF_SIZE) {
	if (slot)
    	    last_time = ld_st(j, tp, last_time);
	else
    	    last_time = ld_dt(j, tp, last_time);
	j++;
	tp++;
    }

    j = 0;
    tp = ltracep;
    while (j < trace_ix) {
	if (slot)
    	    last_time = ld_st(j, tp, last_time);
	else
    	    last_time = ld_dt(j, tp, last_time);
	j++;
	tp++;
    }

    free((void*)tbuf);
    return 0;

}

void
ldump_mem(u_int addr, int nwords)
{
    __uint32_t * ip;
    int i = 0;

    printf("addr = 0x%x, nwords = 0x%x\n", addr, nwords);

    if (addr & 3) {
	fprintf (stderr, "Linc address \"0x%x\" is invalid (not word-aligned)\n", addr);
	return;
    }

    ip = (__uint32_t *) (lmemsp + addr);


    printf("Reading from Linc Memory:\n");
    while (i < nwords) {
	if (i & 3)
	    printf ("  0x%08x", *ip);
	else
	    printf ("\n\t0x%08x: 0x%08x", (char *)ip - lmemsp, *ip);
	ip++;
	i++;
    }
    printf ("\n");
}

void
ldump_pcicfg() 
{


}

void
ldump_state() 
{
    u_int i;
    state_t *state;

    state = (state_t*)malloc(sizeof(state_t));
    memcpy ((char *)state, 
	    (const void *) lstate, sizeof(state_t));

    if (slot) printf("Source State Structure\n");
    else printf("Destination State Structure\n");
    printf("----------------------------------------------------------\n");

    printf("\nflushing cache to get consistent state\n\n");
    intr_4640();
    printf("Linc Main State Struct at 0x%x:\n", &lstate->flags);
    printf("Finite State Machine Pointers & main state\n");
    printf("\tflags\t\t");
    if (state->flags & FLAG_ACCEPT)         printf(" ACCEPT");
    if (state->flags & FLAG_ENB_LE)         printf(" ENB_LE");
    if (state->flags & FLAG_HIPPI_PH)       printf(" HIPPI_PH");
    
    if (state->flags & FLAG_NEED_HOST_INTR) printf(" NEED_INTR");
    if (state->flags & FLAG_BLOCK_INTR)     printf(" BLK_INTR");
    if (state->flags & FLAG_ASLEEP)         printf(" ASLEEP");

    if (state->flags & FLAG_CHECK_OPP_EN)   printf(" CHECK_OPP_EN");
    if (state->flags & FLAG_CHECK_RR_EN)    printf(" CHECK_RR_EN");
    if (state->flags & FLAG_PUSH_TO_OPP)    printf(" PUSH_TO_OPP");

    if (state->flags & FLAG_GLINK_UP)       printf(" GLINK_UP");
    if (state->flags & FLAG_RR_UP)          printf(" RR_UP");
    if (state->flags & FLAG_OPPOSITE_UP)    printf(" OPP_UP");

    if (state->flags & FLAG_GOT_INIT)       printf(" GOT_INIT");
    if (state->flags & FLAG_LOOPBACK)       printf(" LOOPBACK");
    if (state->flags & FLAG_FATAL_ERROR)    printf(" FATAL_ERROR");
    printf("\n");

    if (slot) {	 /* source */
	printf("\thostp\t\t 0x%x\n", state->hostp);
	printf("\twirep\t\t 0x%x\n", state->wirep);
	printf("\tmailboxes\t 0x%x\n", state->mb);
	printf("\theap\t\t 0x%x\n", state->heap);
	printf("\tzero (for WARs)\t 0x%x\n", state->zero);
	printf("\tblksize\t\t 0x%x\n", state->blksize);
	printf("\tleds\t\t 0x%x\n", state->leds);
	printf("\told_byte_cnt\t 0x%x\n", state->old_byte_cnt);
	printf("\n");
	printf("\told_cmdid\t 0x%x\n", state->old_cmdid);
	printf("\tnbpp\t\t 0x%x\n", state->nbpp);
	printf("\thcmd\t\t 0x%x\n", state->hcmd);
	printf("\tstats\t\t 0x%x\n", state->stats);
	printf("\tbpconfig\t 0x%x\n", state->bpconfig);
	printf("\tbpstats\t\t 0x%x\n", state->bpstats);
	printf("\ttrace_base\t 0x%x\n", state->trace_basep);
	if (state->flags & FLAG_GOT_INIT) {
	    printf("\ttrace_put\t 0x%x\n", *(trace_t**)(K0_TO_PHYS(state->traceput)+lmemsp));
	}
	else
	    printf("\ttrace_put\t invalid - firmware not gotten INIT\n");

	printf("\nHost Queue Pointers/State\n");
	printf("\td2b\t\t 0x%x\n", state->d2b);
	printf("\tb2h\t\t 0x%x\n", state->b2h);
	printf("\tdata_M\t\t 0x%x\n", state->data_M);
	printf("\tdata_M_endp\t 0x%x\n", state->data_M_endp);
	printf("\tdata_M_len\t 0x%x\n", state->data_M_len);
	printf("\tsleep_addr_lo\t 0x%x\n", state->sleep_addr_lo);
	printf("\tsleep_addr_hi\t 0x%x\n", state->sleep_addr_hi);
	printf("\tpoll_timer\t 0x%x\n", state->poll_timer);
	printf("\tsleep_timer\t 0x%x\n", state->sleep_timer);
	printf("\tb2h_timer\t 0x%x\n", state->b2h_timer);

	printf("\nRoadrunner Queue Pointers/State\n");	
	printf("\trr_mem\t\t 0x%x\n", state->rr_mem);
	printf("\trr_config\t 0x%x\n", state->rr_config);
	printf("\tl2rr_len\t 0x%x\n", state->l2rr_len);
	printf("\trr2l_len\t 0x%x\n", state->rr2l_len);
	printf("\tsl2rr\t\t 0x%x\n", state->sl2rr);
	printf("\tsrr2l\t\t 0x%x\n", state->srr2l);
	printf("\tsl2rr_ack\t 0x%x\n", state->sl2rr_ack);

	printf("\nBypass State\n");
	printf("\tjob\t\t 0x%x\n", state->job);
	printf("\tfreemap\t\t 0x%x\n", state->freemap);
	printf("\thostx\t\t 0x%x\n", state->hostx);
	printf("\tsdq\t\t 0x%x\n", state->sdq);
	printf("\tdma_statusp\t 0x%x\n", state->dma_statusp);

	printf("\nOpposite Queue\n");
	printf("\topposite_st\t 0x%x\n", state->opposite_st);
	printf("\tlocal_st\t 0x%x\n", state->local_st);
	printf("\topposite_cnt\t 0x%x\n", state->opposite_cnt);
	printf("\topposite_addr\t 0x%x\n", state->opposite_addr);


	printf("\nDebug/Performance State & Counters\n");
	printf("\tmax_loop_time\t 0x%x\n", state->max_loop_time);
	printf("\tloop_timer\t 0x%x\n", state->loop_timer);
    }
    else {  /* destination */
	printf("\thostp\t\t 0x%x\n", state->hostp);
	printf("\twirep\t\t 0x%x\n", state->wirep);
	printf("\tmailboxes\t 0x%x\n", state->mb);
	printf("\theap\t\t 0x%x\n", state->heap);
	printf("\tzero (for WARs)\t 0x%x\n", state->zero);
	printf("\tblksize\t\t 0x%x\n", state->blksize);
	printf("\tleds\t\t 0x%x\n", state->leds);
	printf("\told_byte_cnt\t 0x%x\n", state->old_byte_cnt);
	printf("\n");
	printf("\told_cmdid\t 0x%x\n", state->old_cmdid);
	printf("\tnbpp\t\t 0x%x\n", state->nbpp);
	printf("\thcmd\t\t 0x%x\n", state->hcmd);
	printf("\tstats\t\t 0x%x\n", state->stats);
	printf("\tbpconfig\t 0x%x\n", state->bpconfig);
	printf("\tbpstats\t\t 0x%x\n", state->bpstats);
	printf("\ttrace_base\t 0x%x\n", state->trace_basep);
	if (state->flags & FLAG_GOT_INIT) {
	    printf("\ttrace_put\t 0x%x\n", *(trace_t**)(K0_TO_PHYS(state->traceput)+lmemsp));
	}
	else
	    printf("\ttrace_put\t invalid - firmware not gotten INIT\n");

	printf("\nHost Queue Pointers/State\n");
	printf("\td2b\t\t 0x%x\n", state->d2b);
	printf("\tb2h\t\t 0x%x\n", state->b2h);
	printf("\tdata_M\t\t 0x%x\n", state->data_M);
	printf("\tdata_M_endp\t 0x%x\n", state->data_M_endp);
	printf("\tdata_M_len\t 0x%x\n", state->data_M_len);
	printf("\tsleep_addr_lo\t 0x%x\n", state->sleep_addr_lo);
	printf("\tsleep_addr_hi\t 0x%x\n", state->sleep_addr_hi);
	printf("\tpoll_timer\t 0x%x\n", state->poll_timer);
	printf("\tsleep_timer\t 0x%x\n", state->sleep_timer);
	printf("\tb2h_timer\t 0x%x\n", state->b2h_timer);

	printf("\nRoadrunner Queue Pointers/State\n");	
	printf("\trr_mem\t\t 0x%x\n", state->rr_mem);
	printf("\trr_config\t 0x%x\n", state->rr_config);
	printf("\trr2l_len\t 0x%x\n", state->rr2l_len);
	printf("\tdl2rr\t\t 0x%x\n", state->dl2rr);
	printf("\tdrr2l\t\t 0x%x\n", state->drr2l);

	printf("\nBypass State\n");
	printf("\tjob\t\t 0x%x\n", state->job);
	printf("\tport\t\t 0x%x\n", state->port);
	printf("\tfreemap\t\t 0x%x\n", state->freemap);
	printf("\tdma_statusp\t 0x%x\n", state->dma_statusp);
	printf("\tbp_dst_desc\t 0x%x\n", state->bp_dst_desc);
	printf("\tbpseqnum\t 0x%x\n", state->bpseqnum);

	printf("\nHost Buffer Storage\n");
	printf("\tsm_buf\t\t 0x%x\n", state->sm_buf);
	printf("\tlg_buf\t\t 0x%x\n", state->lg_buf);
	printf("\tfpbuf\t\t 0x%x\n", state->fpbuf);
	printf("\tmbuf_state\t 0x%x\n", state->mbuf_state);
	printf("\tfpbuf_state\t 0x%x\n", state->fpbuf_state);

	printf("\nOpposite Queue\n");
	printf("\topposite_st\t 0x%x\n", state->opposite_st);
	printf("\tlocal_st\t 0x%x\n", state->local_st);
	printf("\topposite_cnt\t 0x%x\n", state->opposite_cnt);
	printf("\topposite_addr\t 0x%x\n", state->opposite_addr);


	printf("\nDebug/Performance State & Counters\n");
	printf("\tmax_loop_time\t 0x%x\n", state->max_loop_time);
	printf("\tloop_timer\t 0x%x\n", state->loop_timer);
    }

    free(state);
}

void
ldump_control() 
{


}

void
ldump_lmcfg() 
{


}

void
ldump_dma() 
{


}


/* dump Roadrunner to Linc queue and state */
void
ldump_rr2l() 
{
    int i;
    if (slot) { /* source */
	volatile src_rr2l_t *lbuf;
	src_rr2l_t *buf, *tail;
	u_int found = 0;
	u_int vb;
	
	printf("Source Roadrunner to Linc Queue:\n");

	buf = (u_int*)malloc(sizeof(src_rr2l_t)*SRC_RR2L_SIZE);

	printf("lstate = 0x%x\n", lstate);
	printf("lstate->srr2l = 0x%x\n", lstate->srr2l);

	/* get ptr to struct */
	lbuf = (volatile src_rr2l_t*)
	    (K1_TO_PHYS(lstate->srr2l) + (u_int)lmemsp);

	printf("Reading from 0x%x\n", lstate->srr2l);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(src_rr2l_t) * SRC_RR2L_SIZE);

	/* find the tail of the buffer */
	tail = buf;
	vb = *tail & SRRS_VB;
	tail++;
	for (i = 0; i < SRC_RR2L_SIZE; i++) {
	    if ( (*tail & SRRS_VB) == vb) {
		found++;
		break;
	    }
	    vb = *tail & SRRS_VB;
	    tail++;
	    if (tail == buf + SRC_RR2L_SIZE-1)
		tail = buf;
	}
    
	if (!found)  {
	    printf("WARNING: Could not find tail of queue\n");
	    return;
	}
	
	/* found tail, start at end and pretty print
	 * entire queue.
	 */
	for (i = 0; i < SRC_RR2L_SIZE-1; i++) {
	    int op = *tail & SRRS_OP_MASK;

	    printf ("    [0x%06x] :", (u_int)K1_TO_PHYS(lstate->srr2l) + 
		    ((u_int)(tail - buf)*sizeof(src_rr2l_t)));

	    if (((u_int)*tail == 0x80000000) || ((u_int)*tail == 0x7fffffff))
		printf("BAD, = 0x%x\n",  *tail & SRRS_ADDR_MASK);
	    else {
		switch (op) 
		    {
		      case SRRS_OP_XMIT_OK: printf(" XMIT_OK"); break;
		      case SRRS_OP_DESC_FLUSHED: printf(" DESC_FLUSHED"); break;
		      case SRRS_OP_CONN_TIMEO: printf(" CONN_TIMEO"); break;
		      case SRRS_OP_DST_DISCON: printf(" DST_DISCON"); break;
		      case SRRS_OP_CONN_REJ: printf(" CONN_REJ"); break;
		      case SRRS_OP_SRC_PERR: printf(" SRC_PERR"); break;
		      default: printf("XXXX");
		    }
		printf(" SL2RR ack addr = 0x%08x\n", *tail & SRRS_ADDR_MASK);
	    }
	    tail++;
	    if (tail == buf + SRC_RR2L_SIZE-1)
		tail = buf;
	}
	printf("\n");

	/* printf state struct */
	{
	    volatile srr2l_state_t *lbuf;
	    srr2l_state_t *buf;
	
	    printf("\nDestination RR2L State:\n");

	    buf = (srr2l_state_t*)malloc(sizeof(srr2l_state_t));

	    /* get ptr to struct */
	    lbuf = (volatile srr2l_state_t*)
	      (K0_TO_PHYS(lstate->srr2l_state)+ (u_int)lmemsp);
    
	    printf("Reading from 0x%x\n", K0_TO_PHYS(lstate->srr2l_state));

	    /* read the buffer into a local buffer */
	    memcpy ((char *)buf, 
		    (const void *) lbuf, sizeof(srr2l_state_t));

	    printf("\tget   = 0x%x\n", buf->get);
	    printf("\tbase  = 0x%x\n", buf->base);
	    printf("\tend   = 0x%x\n", buf->end);

	    free(buf);
	}
	
	free(buf);
    }
    else { /* destination */
	volatile dst_rr2l_t *lbuf;
	dst_rr2l_t *buf, *tail;
	u_int found = 0;
	u_int vb;
	
	printf("Destination Roadrunner to Linc Queue:\n");

	buf = (dst_rr2l_t*)malloc(sizeof(dst_rr2l_t)*DST_RR2L_SIZE);

	/* get ptr to struct */
	lbuf = (volatile dst_rr2l_t*)
	    (K1_TO_PHYS(lstate->drr2l)+ (u_int)lmemsp);
    
	printf("Reading from 0x%x\n", lstate->drr2l);

	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(dst_rr2l_t) * DST_RR2L_SIZE);

	/* find the tail of the buffer */
	tail = buf;
	vb = tail->flag & DRRD_VB;
	tail++;
	for (i = 0; i < DST_RR2L_SIZE; i++) {
	    if ( (tail->flag & DRRD_VB) == vb) {
		found++;
		break;
	    }
	    vb = tail->flag & SRRS_VB;
	    tail++;
	    if (tail == buf + DST_RR2L_SIZE-1)
		tail = buf;
	}

	if (!found)  {
	    printf("WARNING: Could not find tail of queue\n");
	    return;
	}
	
	/* found tail, start at end and pretty print
	 * entire queue.
	 */
	for (i = 0; i < DST_RR2L_SIZE-1; i++) {
	    int op = tail->flag & ~DRRD_ERR_MASK;
	    int err = tail->flag & DRRD_ERR_MASK;

	    printf ("    [0x%03x] :", tail - buf);
	    if ((u_int)tail->addr & 0xf0000000) {
		/* probably bad */
		printf("BAD, 0x%08x, 0x%08x, 0x%x\n", 
		       tail->addr, tail->len, tail->flag);
	    }
	    else {
		printf ("Addr=0x%08x, Len=0x%08x, flags=", 
			tail->addr, tail->len);

		if (tail->flag & DRRD_IFP)           printf(" IFP");
		if (tail->flag & DRRD_EOP)           printf(" EOP");
		if (tail->flag & DRRD_PAUSE_NO_DESC) printf(" PSE_NODSC");
		if (tail->flag & DRRD_PAUSE_NO_BUFF) printf(" PSE_NOBUF");
		if (tail->flag & DRRD_NO_PKT_RCV)    printf(" NO_PKTRCV");
		if (tail->flag & DRRD_NO_BURST_RCV)  printf(" NO_BRTRCV");
		if (tail->flag & DRRD_LAST_WORD_ODD) printf(" LST_WDODD");
		if (tail->flag & DRRD_FBURST_SHORT)  printf(" FBRST_SHT");
		if (tail->flag & DRRD_READY_ERR)     printf(" RDY_ERR ");
		if (tail->flag & DRRD_LINKRDY_ERR)   printf(" LNRY_ERR");
		if (tail->flag & DRRD_FLAG_ERR)      printf(" FLAG_ERR");
		if (tail->flag & DRRD_FRAMING_ERR)   printf(" FRAM_ERR");
		if (tail->flag & DRRD_LLRC_ERR)      printf(" LLRC_ERR");
		if (tail->flag & DRRD_PAR_ERR)       printf(" PAR_ERR ");
		if (tail->flag & DRRD_BURST_SIZE_MASK) {
		    printf(" BRST_SZ_MASK");
		}
		if (tail->flag & DRRD_ERR_ST_MASK)  {
		    printf(" ERR_ST_MASK");
		}
		printf("\n");
	    }

	    tail++;
	    if (tail == buf + SRC_RR2L_SIZE-1)
		tail = buf;
	}
	printf("\n");

	/* printf state struct */
	{
	    volatile drr2l_state_t *lbuf;
	    drr2l_state_t *buf;
	
	    printf("\nDestination RR2L State:\n");

	    buf = (drr2l_state_t*)malloc(sizeof(drr2l_state_t));

	    /* get ptr to struct */
	    lbuf = (volatile drr2l_state_t*)
	      (K0_TO_PHYS(lstate->drr2l_state)+ (u_int)lmemsp);
    
	    printf("Reading from 0x%x\n", K0_TO_PHYS(lstate->drr2l_state));

	    /* read the buffer into a local buffer */
	    memcpy ((char *)buf, 
		    (const void *) lbuf, sizeof(drr2l_state_t));

	    printf("\tget   = 0x%x, \toffset = 0x%x\n", 
		   buf->get, buf->get - buf->base);
	    printf("\tbase  = 0x%x\n", buf->base);
	    printf("\tend   = 0x%x\n", buf->end);

	    free(buf);
	}

	free(buf);
    }
}

/* dump queue and state 
 * For source it also prints SL2RR_ACK queue and state
 */
void
ldump_l2rr() 
{
    int i;
    u_int base;

    if (slot) { /* source */
	volatile src_l2rr_t *lbuf;
	src_l2rr_t *buf, *tail;
	u_int found = 0;
	u_int vb;
	
	printf("Source Linc to Roadrunner Queue:\n");

	buf = (src_l2rr_t*)malloc(sizeof(src_l2rr_t)*SRC_L2RR_SIZE);
	
	/* get ptr to struct */
	base = K1_TO_PHYS(lstate->sl2rr);
	lbuf = (volatile src_l2rr_t*)
	    (base + (u_int)lmemsp);
	
	printf("Reading from SDRAM, addr = 0x%x\n", lstate->sl2rr);
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(src_l2rr_t) * SRC_L2RR_SIZE);

	/* find the tail of the buffer */
	tail = buf;
	vb = tail->op_len & SRRD_VB;
	tail++;
	for (i = 0; i <= SRC_L2RR_SIZE-1; i++) {
	    if ( (tail->op_len & SRRD_VB) == vb) {
		found++;
		break;
	    }
	    vb = tail->op_len & SRRD_VB;
	    tail++;
	    if (tail == buf + SRC_L2RR_SIZE-1)
		tail = buf;
	}
    
	if (!found)  {
	    printf("WARNING: Could not find tail of queue\n");
	    return;
	}

	printf("Found tail of queue at 0x%x\n", (u_int)tail & 0xffffff);

	/* found tail, start at end and pretty print
	 * entire queue.
	 */
	for (i = 0; i < SRC_L2RR_SIZE-1; i++) {
	    u_int op = tail->op_len & ~SRRD_LEN_MASK;
	    int len = tail->op_len & SRRD_LEN_MASK;

	    printf ("    [0x%06x] :", base + ((u_int)(tail - buf)*4*sizeof(u_int)));
	    if ((u_int)tail->addr == 0xffffffff)
		printf("XXXX: 0x%05x, 0x%08x, 0x%08x, 0x%08x\n", 
		       len, tail->addr, tail->ifield, op);
	    else {
		printf ("Op=0x%02x,Len=0x%05x, DataAddr=0x%08x, Ifield=0x%08x: ",
			op>>24, len, tail->addr, tail->ifield);
		if (op & SRRD_MB) printf(" MORE");
		if (op & SRRD_CC) printf(" CONT_CONN");
		if (op & SRRD_PC) printf(" PERM_CONN");
		if (op & SRRD_DD) printf(" DUMMY");
		if (op & SRRD_WN) printf(" WRAP_NEXT");
		if (op & SRRD_SI) printf(" SAME_IFLD");
		printf("\n");
	    }
	    tail++;
	    if (tail == buf + SRC_L2RR_SIZE-1)
		tail = buf;
	    }
	printf("\n");

	{
	    sl2rr_state_t st_buf;
	    volatile sl2rr_state_t *lst_buf;

	    printf("\nSource L2RR State\n");
	    printf("\tlstate = 0x%x\n", lstate);
	    printf("\tlstate->sl2rr_state = 0x%x\n", lstate->sl2rr_state);
	    
	    lst_buf = (volatile sl2rr_state_t*)
	      (K0_TO_PHYS(lstate->sl2rr_state)+ (u_int)lmemsp);
    
	    printf("Reading from 0x%x\n", K0_TO_PHYS(lstate->sl2rr_state));

	    /* read the buffer into a local buffer */
	    memcpy ((char *)&st_buf, 
		    (const void *) lst_buf, sizeof(sl2rr_state_t));

	    printf("\tflags:");
	    if (st_buf.flags & SL2RR_VB) printf("VB ");
	    if (st_buf.flags & SL2RR_PC) printf("PC ");
	    printf("\n");
	    
	    printf("\tbase\t0x%x\n", st_buf.base);
	    printf("\tput\t0x%x\n", st_buf.put);
	    printf("\tget\t0x%x\n", st_buf.get);
	    printf("\tend\t0x%x\n", st_buf.end);
	}

	free(buf);
    }

    else {   /* destination */
	volatile dst_l2rr_t *lbuf;
	dst_l2rr_t *buf;
	
	printf("Destination Linc Consumer Pointers for RR2L Queue:\n");

	buf = (dst_l2rr_t*)malloc(sizeof(dst_l2rr_t));
	
	/* get ptr to struct */
	base = (u_int)K1_TO_PHYS(lstate->dl2rr);

	lbuf = (volatile dst_l2rr_t*)
	    (base + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(dst_l2rr_t));

	printf("reading from 0x%x\n", base);
	printf("DescRingConsumer = 0x%08x\n", buf->desc_ring_consumer);
	printf("DescRingBase     = 0x%08x\n", lstate->drr2l);
	printf("DescRingEnd      = 0x%08x\n", 
	       lstate->drr2l + lstate->rr2l_len);
	printf("\n");
	printf("DataRingConsumer = 0x%08x\n", buf->data_ring_consumer);
	printf("DataRingBase     = 0x%08x\n", lstate->data_M);
	printf("DataRingEnd      = 0x%08x\n", lstate->data_M_endp);
	printf("DataRingSize     = %d\n", lstate->data_M_len * 4);
	printf("\n");

	free(buf);
    }
}

/* dump queue and state */
void
ldump_b2h() 
{

    volatile hip_b2h_t *lbuf;
    hip_b2h_t *buf, *tail;
    int i = 0;
    int last_seq;
    
    volatile b2h_state_t *lst;
    b2h_state_t *st;
    
	
    printf("Dumping B2H queue\n");
    buf = (hip_b2h_t*)malloc(sizeof(hip_b2h_t)*B2H_SIZE);
    /* get ptr to struct */
    lbuf = (volatile hip_b2h_t*)
	(K0_TO_PHYS(lstate->b2h) + (u_int)lmemsp);

    printf("reading b2h queue from 0x%x\n", K0_TO_PHYS(lstate->b2h));
    /* read the buffer into a local buffer */
    memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(hip_b2h_t)*B2H_SIZE);
    
    st = (b2h_state_t*)malloc(sizeof(b2h_state_t));
    /* get ptr to struct */
    lst = (volatile b2h_state_t*)
	(K0_TO_PHYS(lstate->b2h_state) + (u_int)lmemsp);

    printf("reading b2h state from 0x%x\n", K0_TO_PHYS(lstate->b2h_state));
    /* read the buffer into a local buffer */
    memcpy ((char *)st, 
	    (const void *) lst, sizeof(b2h_state_t));

    /* find start of queue */

    last_seq = buf[0].b2h_sn;
    tail = buf+1;
    for(i=1; i < B2H_SIZE; i++) {
	if (tail->b2h_sn != last_seq+1)
	    break;
	last_seq = tail->b2h_sn;
	tail++;
	if (tail == buf+B2H_SIZE)
	    tail = buf;
    }
    printf("scanning queue for break in seq num - tail = 0x%x\n", 
	   (((u_int)tail - (u_int)buf) + (u_int)st->basep) & 0xffffff);
    printf("state struct has                    - tail = 0x%x\n", 
	   (u_int)(st->put) & 0xffffff);

    if (st->put < st->endp && st->put > st->basep) {
	printf("using tail in state struct\n");
	tail = (hip_b2h_t*) 
	    ((K0_TO_PHYS(st->put) - K0_TO_PHYS(st->basep)) + (u_int)buf);
    }
    else 
	printf("using tail derived from seqnum in queue\n");

    for (i = 0; i < B2H_SIZE; i++) {
	printf("    [0x%06x] 0x%02x, 0x%02x, 0x%04x, 0x%08x\t", 
	       (((u_int)tail - (u_int)buf) + (u_int)K0_TO_PHYS(st->basep)) & 0xffffff,
	       tail->b2h_sn, tail->b2h_op, 
	       tail->b2hu.b2hu_s, tail->b2h_l);
	switch (tail->b2h_op & HIP_B2H_OPMASK) {
	  case HIP_B2H_NOP:
	    printf(":NOP\n");
	    break;
	  case HIP_B2H_ODONE:
	    printf(":ODONE, stk=%d, num=%d,status=0x%x, b2h_l=0x%x\n", 
		   tail->b2h_op & ~HIP_B2H_OPMASK,
		   tail->b2hu.b2h_odone.b2hod_n,
		   tail->b2hu.b2h_odone.b2hod_status,
		   tail->b2h_l);
	    break;
	  case HIP_B2H_IN:
	    printf(":IN, stk=%d, ", tail->b2h_op & ~HIP_B2H_OPMASK);
	    if ( (tail->b2h_op & ~HIP_B2H_OPMASK) == HIP_STACK_LE) 
		printf("LgBfs=0x%x, SmBfWords=0x%x, LenWds=0x%x, chksum=0x%x\n", 
		       tail->b2hu.b2h_in.b2hi_pages,
		       tail->b2hu.b2h_in.b2hi_words,
		       tail->b2h_l >> 16,
		       tail->b2h_l & 0xffff);
	    else {
		printf("xfr_len=0x%x, ", tail->b2hu.b2hu_s);
		printf("d2_len=0x%x (%d)\n", tail->b2h_l, tail->b2h_l);
	    }
	    break;
	  case HIP_B2H_IN_DONE:
	    printf(":IN_DONE ");
	    if ((tail->b2h_op & ~HIP_B2H_OPMASK) == HIP_STACK_LE) {
		printf("STK_LE, num_lg_bufs=%d, len_sm_bufs=%d words, total_len=%d (0x%x) words, cksum=0x%4x, Check=",
		       tail->b2h_pages,
		       tail->b2h_words,
		       tail->b2h_l>>16, tail->b2h_l>>16,
		       tail->b2h_l & 0xffff);
		if ( ((tail->b2h_pages*16384)+tail->b2h_words*4) ==  ((tail->b2h_l>>16)*4))
		    printf("EXACT\n");
		else if (((tail->b2h_pages*16384)+tail->b2h_words*4) >=  ((tail->b2h_l>>16)*4))
		    printf("EXTRA\n");
		else
		    printf("ERROR - TO LITTLE\n");
		       
		    
	    }
	    else {
		printf("stk=%d, ", (tail->b2h_op & ~HIP_B2H_OPMASK));
		   
		if (tail->b2hu.b2hu_s) {
		    printf("flags=");
		    if (tail->b2hu.b2hu_s & B2H_ISTAT_I)
			printf("IFLD ");
		    if (tail->b2hu.b2hu_s & B2H_ISTAT_MORE)
			printf("MORE ");
		}
		printf("len=0x%x (%d)\n", tail->b2h_l, tail->b2h_l);
	    }
	    if ((int)tail->b2h_l < 0) {
		printf("ERROR OCCURRED: ");
		
		if (tail->b2h_l & B2H_IERR_PARITY) printf("PARITY ");
		if (tail->b2h_l & B2H_IERR_LLRC) printf("LLRC ");
		if (tail->b2h_l & B2H_IERR_SEQ) printf("SEQ ");
		if (tail->b2h_l & B2H_IERR_SYNC) printf("SYNC ");
		if (tail->b2h_l & B2H_IERR_ILBURST) printf("ILBURST ");
		if (tail->b2h_l & B2H_IERR_SDIC) printf("SDIC ");
		printf("\n");
	    }
	    break;
	  case HIP_B2H_BP_PORTINT:
	    printf(":BP_PORTINT ");
	    printf("port_id = %d, intr_cnt=0x%x\n", 
		   tail->b2hu.b2h_bp_portint.portid,
		   tail->b2h_l);
	    break;
	  default:
	    printf("XXX op=0x%x", tail->b2h_op & HIP_B2H_OPMASK);
	    printf("b2h_l=0x%x\n", tail->b2h_l);
	}
	tail++;
	if (tail == buf+B2H_SIZE)
	    tail = buf;
    }
    printf("\n\nB2H State\n");
    printf("\tseqnum       0x%x\n", st->seqnum);
    printf("\tqueued       0x%x\n", st->queued);
    printf("\n");
    printf("\tput          0x%x\n", st->put);
    printf("\tbasep        0x%x\n", st->basep);
    printf("\tendp         0x%x\n", st->endp);
    printf("\thostp_lo     0x%x\n", st->hostp_lo);
    printf("\thostp_hi     0x%x\n", st->hostp_hi);
    printf("\thost_off     0x%x\n", st->host_off);
    printf("\thost_end     0x%x\n", st->host_end);
    printf("\n");

    free(buf);
    free(st);
}

/* dump queue and state */
void
ldump_d2b() 
{
    volatile hip_d2b_t *lbuf;
    hip_d2b_t *buf, *tail;
    volatile d2b_state_t *ld2b_st;
    d2b_state_t *d2b_st;
    int i = 0;
    int last_seq;
	
    printf("Dumping D2B queue\n");
    buf = (hip_d2b_t*)malloc(sizeof(hip_d2b_t)*LOCAL_D2B_LEN);

    /* get ptr to struct */
    lbuf = (volatile hip_d2b_t*)
	(K0_TO_PHYS(lstate->d2b) + (u_int)lmemsp);

    printf("reading d2b queue from 0x%x\n", K0_TO_PHYS(lstate->d2b));
    /* read the buffer into a local buffer */
    memcpy ((char *)buf, 
	    (const void *) lbuf, sizeof(hip_d2b_t)*LOCAL_D2B_LEN);

    /* find start of queue by querying D2B state*/
    d2b_st = (d2b_state_t*)malloc(sizeof(d2b_state_t));
	
    /* get ptr to struct */
    ld2b_st = (volatile d2b_state_t*)
	    (K0_TO_PHYS(lstate->d2b_state) + (u_int)lmemsp);
    
    printf("reading D2B state from 0x%x\n", K0_TO_PHYS(lstate->d2b_state));
    /* read the buffer into a local buffer */
    memcpy ((char *)d2b_st, 
	    (const void *) ld2b_st, sizeof(d2b_state_t));
    if (K0_TO_PHYS(d2b_st->get) > SDRAM_SIZE) {
	printf("WARNING: D2B get pointer is invalid - printing from base of queue\n");
	tail = buf;
    }
    else 
	tail = (hip_d2b_t*) 
	    ((K0_TO_PHYS(d2b_st->get) - K0_TO_PHYS(d2b_st->basep)) + (u_int)buf);

    printf("Starting at tail = 0x%x\n", K0_TO_PHYS(d2b_st->get));

    if (slot) {
	hip_d2b_t *dbuf = (hip_d2b_t*)buf;
	hip_d2b_t *dtail = (hip_d2b_t*)tail;

	if (dtail->hd.flags != HIP_D2B_BAD) {
	    printf("QUEUE NOT COMPLETE - last element in local queue not End-of-Queue\n");
	    printf("\tstarting dump at end of local buffer\n");
	    tail = dbuf + (LOCAL_D2B_LEN-1);
	}
	else {
	    dtail++;		/* point one past actual tail */
	    if (dtail == dbuf + LOCAL_D2B_LEN)
		dtail = dbuf;
	}
	    
	for (i = 0; i < LOCAL_D2B_LEN; i++) {
	    int stack = dtail->hd.stk;
	    int flags = dtail->hd.flags;

	    printf("    [0x%03x] [0x%6x] ", dtail - dbuf, 
		   K0_TO_PHYS(lstate->d2b) +(dtail - dbuf)*4*sizeof(u_int));
	    if (flags == HIP_D2B_BAD) {
		printf("*******  BAD_D2B - END OF QUEUE ********\n");
	    }
	    else {
		if (flags & HIP_D2B_RDY) { /* header d2b */
		    printf("HDR: chunks=%d, fburst=0x%x, sumoff=0x%x",
			   dtail->hd.chunks, dtail->hd.fburst, dtail->hd.sumoff);
		    switch(stack) {
		      case HIP_STACK_LE: printf(" STACK_LE");
		      case HIP_STACK_IPI3: printf(" STACK_IPI3");
		      case HIP_STACK_RAW: printf(" STACK_RAW");
		      case HIP_STACK_ST: printf(" STACK_ST");
		      case HIP_STACK_FP: printf(" STACK_FP=%d", stack);
		    }
		    if (flags & HIP_D2B_IFLD) printf(" IFLD");
		    if (flags & HIP_D2B_NEOC) printf(" NEOC");
		    if (flags & HIP_D2B_NEOP) printf(" NEOP");
		    if (flags & HIP_D2B_NACK) printf(" NACK");
		    if (flags & HIP_D2B_BEGPC) printf(" BEGPC");
		    printf("\n");
		
		}
		else {		/* data d2b */
		    printf("  DTA: len=0x%05x, addr=0x%08x,%08x\n", 
			   dtail->sg.len, (u_int)(dtail->sg.addr>>32),
			   (u_int)dtail->sg.addr);
		}
	    }
	    dtail++;
	    if (dtail == dbuf + LOCAL_D2B_LEN)
		dtail = dbuf;
	}
	
    }
    else { /* dest */
	hip_c2b_t *cbuf = (hip_c2b_t*)buf;
	hip_c2b_t *ctail = (hip_c2b_t*)tail;

	for (i = 0; i < LOCAL_D2B_LEN; i++) {
	    int stack = ctail->c2b_op & HIP_C2B_STMASK;

	    printf("    [0x%03x] [0x%6x] ", ctail - cbuf, 
		   K0_TO_PHYS(lstate->d2b) +(ctail - cbuf)*4*sizeof(u_int));

	    switch(ctail->c2b_op & HIP_C2B_OPMASK) {
	      case HIP_C2B_SML:
		printf("SML:  ");
		if (stack == HIP_STACK_LE) {
		    printf("LE:    len=0x%x, addr=0x%08llx,%08x\n",
			   ctail->c2b_param,
 			   ctail->c2b_addr>>32,
			   (u_int)ctail->c2b_addr);
		}
		else {
		    printf("FPHDR: stk=%d, len=0x%x, addr=0x%08llx,%08x\n",
			   ctail->c2b_op & HIP_C2B_STMASK,
			   ctail->c2b_param,
 			   ctail->c2b_addr>>32,
			   (u_int)ctail->c2b_addr);
		}
		break;
	      case HIP_C2B_BIG:
		printf("BIG:  ");
		printf("MBUF:  len=0x%x, addr=0x%08llx,%08x\n",
		       ctail->c2b_param,
		       ctail->c2b_addr>>32,
		       (u_int)ctail->c2b_addr);
		break;
	      case HIP_C2B_WRAP:
		printf("WRAP: ???? NOT SUPPORTED ????\n");
		break;
	      case HIP_C2B_READ:
		printf("RLST: ");
		printf("FPBUF: stk=%d, len=0x%x, addr=0x%08llx,%08x\n",
		       ctail->c2b_op & HIP_C2B_STMASK,
		       ctail->c2b_param,
		       ctail->c2b_addr>>32,
		       (u_int)ctail->c2b_addr);
		
		break;
	      default:
		printf("XXX - unknown opcode 0x%x, 0x%x, 0x%x, 0x%x\n",
		       *(u_int*)ctail, *(u_int*)(ctail+1), 
		       *(u_int*)(ctail+2), *(u_int*)(ctail+3));
	    }
	    ctail++;
	    if (ctail == cbuf + LOCAL_D2B_LEN)
		ctail = cbuf;
	}
    }
    printf("\n\nD2B State\n");
    printf("\tget          0x%x (offset = 0x%x)\n", d2b_st->get, d2b_st->get - d2b_st->basep);
    printf("\tbasep        0x%x\n", d2b_st->basep);
    printf("\tendp         0x%x\n", d2b_st->endp);
    printf("\thostp_lo     0x%x\n", d2b_st->hostp_lo);
    printf("\thostp_hi     0x%x\n", d2b_st->hostp_hi);
    printf("\thost_off     0x%x\n", d2b_st->host_off);
    printf("\thost_end     0x%x\n", d2b_st->host_end);
    printf("\tst_in_cache  0x%x\n", d2b_st->st_in_cache);
    printf("\tend_valid    0x%x\n", d2b_st->end_valid);
    printf("\n");

    free(buf);
    free(d2b_st);
}

void
ldump_opposite() 
{
    volatile opposite_t *lbuf;
    opposite_t *buf;
	
    printf("State of queues between Opposite Lincs:\n");
    buf = (opposite_t*)malloc(sizeof(opposite_t));

    printf("\tLocal State:\n");    
    printf("\t\tOpposite Addr    0x%08x\n", lstate->opposite_addr);
    printf("\t\tOpposite Count   %d\n", lstate->opposite_cnt);
    printf("\n");
	
    printf("\tLocal State being sent to opposite linc\n");
    /* get ptr to struct */
    lbuf = (volatile opposite_t*)
	    (K1_TO_PHYS(lstate->local_st) + (u_int)lmemsp);
    
    /* read the buffer into a local buffer */
    memcpy ((char *)buf, (const void *) lbuf, sizeof(opposite_t));
    printf("\t\tcount            %d\n", buf->cnt);
    printf("\t\tflags            0x%08x\n", buf->flags);

    printf("\t Opposite Linc's State:\n");
    /* get ptr to struct */
    lbuf = (volatile opposite_t*)
	    (K1_TO_PHYS(lstate->opposite_st) + (u_int)lmemsp);
    
    /* read the buffer into a local buffer */
    memcpy ((char *)buf, (const void *) lbuf, sizeof(opposite_t));
    printf("\t\tcount            %d\n", buf->cnt);
    printf("\t\tflags            0x%08x\n", buf->flags);
    printf("\n");

    free(buf);
}

void
ldump_blk() 
{
    if (slot) {
	volatile src_blk_t *lbuf;
	src_blk_t *buf;
	
	printf("Source Block Structure:\n");
	buf = (src_blk_t*)malloc(sizeof(src_blk_t));
	
	/* get ptr to struct */
	lbuf = (volatile src_blk_t*)
	    (K1_TO_PHYS(lstate->sblk) + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(src_blk_t));
    
	printf("\tDataPtr      0x%x\n", buf->dp);
	printf("\tFlags        ");
	if (buf->flags & SBLK_NEOP) printf(" NEOP");
	if (buf->flags & SBLK_NEOC) printf(" NEOC");
	if (buf->flags & SBLK_NACK) printf(" NACK");
	if (buf->flags & SBLK_BEGPC) printf(" BEGPC");
	if (buf->flags & SBLK_SOC) printf(" SOC");
	if (buf->flags & SBLK_BP) printf(" BP");
	if (buf->flags & SBLK_REM) printf(" REMAINDER");
	printf("\n");
	printf("\tDMA flags    ");
	if (buf->dma_flags & SDMA_START_PKT) printf(" START_PKT");
	if (buf->dma_flags & SDMA_CHAIN_CS) printf(" CHAIN_CS");
	if (buf->dma_flags & SDMA_SAVE_CS) printf(" SAVE_CS");
	if (buf->dma_flags & SDMA_INT_DONE) printf(" INT_DONE");
	printf("\n");
	printf("\tstack        %d\n", buf->stack);
	printf("\tNum of D2Bs  %d\n", buf->num_d2bs);
	printf("\tTail Pad     %d\n", buf->tail_pad);
	printf("\tFirst Burst  %d\n", buf->fburst);
	printf("\tIfield       0x%x\n", buf->ifield);
	printf("\tlength       %d (0x%x)\n", buf->len, buf->len);

	free(buf);
    }
    else 
	printf("ERROR: Not valid for Destination\n");
}
	

void
ldump_ablk() 
{
    volatile ack_blk_t *lbuf;
    ack_blk_t *buf;

    if (!slot) {
	printf("Destination Ack Block Structure (to host interface):\n");
	buf = (ack_blk_t*)malloc(sizeof(ack_blk_t));
	
	/* get ptr to struct */
	lbuf = (volatile ack_blk_t*)
	    (K1_TO_PHYS(lstate->ablk) + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(ack_blk_t));
    
	printf("\tDataPtr      0x%08x\n", buf->dp);
	printf("\tFlags        ");
	if (buf->flags & DBLK_READY)      printf(" READY");
	if (buf->flags & DBLK_EOP)        printf(" EOP");
	if (buf->flags & DBLK_IFIELD)     printf(" IFIELD");
	if (buf->flags & DBLK_FP_VALID)   printf(" FP_VAL");
	if (buf->flags & DBLK_ADDR_VALID) printf(" ADDR_VAL");
	if (buf->flags & DBLK_P_BIT)      printf(" P_BIT");
	if (buf->flags & DBLK_B_BIT)      printf(" B_BIT");
	printf("\n");

	printf("\tError Flags  ");
	if (buf->flags & DBLK_ERR_MASK) {
	    if (buf->flags & DBLK_ERR_PARITY) printf(" PARITY");
	    if (buf->flags & DBLK_ERR_LLRC)   printf(" LLRC");
	    if (buf->flags & DBLK_ERR_SEQ)    printf(" SEQ");
	    if (buf->flags & DBLK_ERR_SYNC)   printf(" SYNC");
	    if (buf->flags & DBLK_ILBURST)    printf(" ILBURST");
	    if (buf->flags & DBLK_SDIC)       printf(" SDIC");
	}
	printf("\n");
	printf("\tBytes avail   0x%x (%d)\n", buf->avail, buf->avail);
	printf("\tD1 size       %d\n", buf->d1_size);
	printf("\tD2 size       0x%x (%d)\n", buf->d2_size, buf->d2_size);
	printf("\tOffset to D2  %d\n", buf->d2_offset);
	printf("\tulp           %d\n", buf->ulp);
	printf("\n");
	printf("\tbp_job        %d\n", buf->bp_job);
	printf("\tle_sm_buf_len 0x%x (%d)\n", buf->le_sm_buf_len);
	printf("\tle_lg_bufs    %d\n", buf->le_lg_bufs);
	printf("\tpad           %d\n", buf->pad);
	printf("\tbp_dsc_add_hi 0x%x\n", buf->bp_desc_addr_hi);
	printf("\tbp_dsc_add_lo 0x%x\n", buf->bp_desc_addr_lo);

	free(buf);
    }
    else {
	printf("ERROR: Not valid for Source\n");
    }
}

void
ldump_wblk() 
{
    volatile wire_blk_t *lbuf;
    wire_blk_t *buf;

    if (!slot) {
	printf("Destination Wire Block Structure (from RR interface):\n");
	buf = (wire_blk_t*)malloc(sizeof(wire_blk_t));
	
	/* get ptr to struct */
	lbuf = (volatile wire_blk_t*)
	    (K1_TO_PHYS(lstate->wblk) + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(wire_blk_t));
    
	printf("\tDataPtr      0x%08x\n", buf->dp);
	printf("\tFlags        ");
	if (buf->flags & DBLK_READY)      printf(" READY");
	if (buf->flags & DBLK_EOP)        printf(" EOP");
	if (buf->flags & DBLK_IFIELD)     printf(" IFIELD");
	if (buf->flags & DBLK_FP_VALID)   printf(" FP_VAL");
	if (buf->flags & DBLK_ADDR_VALID) printf(" ADDR_VAL");
	if (buf->flags & DBLK_P_BIT)      printf(" P_BIT");
	if (buf->flags & DBLK_B_BIT)      printf(" B_BIT");

	printf("\n");
	printf("\tError Flags  ");
	if (buf->flags & DBLK_ERR_MASK) {
	    if (buf->flags & DBLK_ERR_PARITY) printf(" PARITY");
	    if (buf->flags & DBLK_ERR_LLRC)   printf(" LLRC");
	    if (buf->flags & DBLK_ERR_SEQ)    printf(" SEQ");
	    if (buf->flags & DBLK_ERR_SYNC)   printf(" SYNC");
	    if (buf->flags & DBLK_ILBURST)    printf(" ILBURST");
	    if (buf->flags & DBLK_SDIC)       printf(" SDIC");
	}
	printf("\n");
	printf("\tBytes needed  %d\n", buf->bytes_needed);
	printf("\tIfield        0x%08x\n", buf->ifield);
	printf("\tulp           %d\n", buf->ulp);
	printf("\tavail         0x%x  (%d)\n", buf->avail, buf->avail);
	printf("\tD2 size       0x%x (%d)\n", buf->d2_size, buf->d2_size);
	printf("\tOffset to D2  %d\n", buf->d2_offset);
	printf("\tD1 size       %d\n", buf->d1_size);
	printf("\tpad           %d\n", buf->pad);

	free(buf);
    }
    else {
	printf("ERROR: Not valid for Source\n");
    }
}

void
ldump_mbuf() 
{
    int i;
    mbuf_state_t *buf;
    volatile mbuf_state_t *lbuf;
    mbuf_t *sm_list,
	   *lg_list,
	   *cur;
    volatile mbuf_t *lsm_list,
	            *llg_list;

    mbuf_t *sm_base = (mbuf_t*)(((uint32_t)lstate->sm_buf & 0x0fffffff) + lmemsp),
	   *lg_base = (mbuf_t*)(((uint32_t)lstate->lg_buf & 0x0fffffff) + lmemsp),
	   *data;

    if (slot) {
	printf("ERROR: not valid for source\n");
	return;
    }

    buf = (mbuf_state_t *)malloc(sizeof(mbuf_state_t));
    sm_list = (mbuf_t *)malloc(sizeof(mbuf_t) * HIP_MAX_SML);
    lg_list = (mbuf_t *)malloc(sizeof(mbuf_t) * HIP_MAX_BIG);

    lbuf = (volatile mbuf_state_t*)
	(K1_TO_PHYS(lstate->mbuf_state) + (u_int)lmemsp);
    lsm_list = (volatile mbuf_t *)
	(K1_TO_PHYS(lstate->sm_buf) + (u_int)lmemsp);
    llg_list = (volatile mbuf_t *)
	(K1_TO_PHYS(lstate->lg_buf) + (u_int)lmemsp);

    printf("reading state from 0x%x\nsmall mbuf list from 0x%x\n"
	   "large mbuf list from 0x%x\n", lbuf, lsm_list, llg_list);
    /* read the memory into a local buffers */
    memcpy((char *)buf, (const void *) lbuf, sizeof(mbuf_state_t));
    memcpy((char *)sm_list, (const void *)lsm_list, sizeof(mbuf_t) * HIP_MAX_SML);
    memcpy((char *)lg_list, (const void *)llg_list, sizeof(mbuf_t) * HIP_MAX_BIG); 

    printf("Small MBUF State\nput index = %d, get index = %d\n",
	   buf->sm_put - buf->sm_mbuf_basep,
	   buf->sm_get - buf->sm_mbuf_basep);
    cur = sm_list;
    for (i = 0; i < HIP_MAX_SML; i++) {
	printf("%03d: addr_hi = 0x%08x, addr_lo = 0x%08x, len = 0x%08x\n",
	       i, cur->addr_hi, cur->addr_lo, cur->len);
	cur++;
    }
    
    printf("\nLarge MBUF State\nput index = %d, get index = %d\n",
	   buf->lg_put - buf->lg_mbuf_basep,
	   buf->lg_get - buf->lg_mbuf_basep);
    cur = lg_list;
    for (i = 0; i < HIP_MAX_BIG; i++) {
	printf("%03d: addr_hi = 0x%08x, addr_lo = 0x%08x, len = 0x%08x\n",
	       i, cur->addr_hi, cur->addr_lo, cur->len);
	cur++;
    }

    free(buf);
    free(sm_list);
    free(lg_list);
}

void
ldump_fpbuf() 
{
    int i;
    if (slot)
	printf("ERROR: not valid for source\n");
    else {
	u_int get;
	{
	    dst_host_t *buf;
	    volatile dst_host_t *lbuf;

	    printf("FP Stack State\n");

	    buf = (dst_host_t*)malloc(sizeof(dst_host_t));
	
	    /* get ptr to struct */
	    lbuf = (volatile dst_host_t*)
		(K1_TO_PHYS(lstate->hostp) + (u_int)lmemsp);
    
	    /* read the buffer into a local buffer */
	    memcpy ((char *)buf, 
		    (const void *) lbuf, sizeof(dst_host_t));

	    for (i=0; i < HIP_N_STACKS; i++) {
		stk_state_t *stackp = &buf->fpstk[i];
		printf("  Stack %d\n", i);
		printf("\tflags:\t");
		if (stackp->flags & FP_STK_ENABLED) printf(" ENABLED");
		if (stackp->flags & FP_STK_HDR_VAL) printf(" HDR_VAL");
		if (stackp->flags & FP_STK_FPBUF_VAL) printf(" FPBUF_VAL");
		printf("\n");
	    
		printf("\thdr_addr_hi:\t 0x%x\n", stackp->hdr_addr_hi);
		printf("\thdr_addr_lo:\t 0x%x\n", stackp->hdr_addr_lo);
		printf("\tfpbuf_addr_hi:\t 0x%x\n", stackp->fpbuf_addr_hi);
		printf("\tfpbuf_addr_lo:\t 0x%x\n", stackp->fpbuf_addr_lo);
		printf("\tfpbuf_len:\t 0x%x\n", stackp->fpbuf_len);
	    }
	    printf("\n");

	    free(buf);
	}
	{
	    fpbuf_state_t *buf;
	    volatile fpbuf_state_t *lbuf;
	    
	    printf("FPBUF State\n");
	    buf = (fpbuf_state_t*)malloc(sizeof(fpbuf_state_t));
	
	    /* get ptr to struct */
	    lbuf = (volatile fpbuf_state_t*)
		(K1_TO_PHYS(lstate->fpbuf_state) + (u_int)lmemsp);

	    printf("reading from 0x%x\n", lbuf);
	    /* read the buffer into a local buffer */
	    memcpy ((char *)buf, 
		    (const void *) lbuf, sizeof(fpbuf_state_t));
	
	    printf("\tnum_valid = %d\n", buf->num_valid);
	    printf("\tget       = 0x%x\n", buf->get);
	    printf("\tbasep     = 0x%x\n", buf->basep);
	    get = buf->get - buf->basep;
	
	    free(buf);
	}
	{
	    hip_c2b_t *buf;
	    volatile hip_c2b_t *lbuf;

	    printf("C2B list of FP Buffers\n");
	    buf = (hip_c2b_t*)malloc(sizeof(hip_c2b_t)*FPBUF_DMA_LEN);
	
	    /* get ptr to struct */
	    lbuf = (volatile hip_c2b_t*)
		(K1_TO_PHYS(lstate->fpbuf) + (u_int)lmemsp);
    
	    /* read the buffer into a local buffer */
	    memcpy ((char *)buf, 
		    (const void *) lbuf, sizeof(hip_c2b_t)*FPBUF_DMA_LEN);
	    
	    for (i = 0; i < FPBUF_DMA_LEN/sizeof(hip_c2b_t); i++) {
		printf("\tc2b_param\t0x%x", buf->c2b_param);
		printf("\tc2b_op\t0x%x", buf->c2b_op);
		printf("\tc2b_addr\t0x%llx", buf->c2b_addr);
		if (i == get)
		    printf(" <=== get ptr\n");
		else
		    printf("\n");
			
	    }

	    free(buf);
	}
    }
}

void
ldump_hostp() 
{
    if (slot) { /* source */
	volatile src_host_t *lbuf;
	src_host_t *buf;

	printf("Source Host Finite State Machine State\n");
	buf = (src_host_t*)malloc(sizeof(src_host_t));
	
	/* get ptr to struct */
	lbuf = (volatile src_host_t*)
	    (K1_TO_PHYS(lstate->hostp) + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(src_host_t));
    
	printf("\tState          ");
	switch(buf->st) {
	  case SH_IDLE: printf("IDLE"); break;
	  case SH_D2B_ACTIVE: printf("D2B_ACTIVE"); break;
	  case SH_D2B_FULL: printf("D2B_FULL"); break;
	  case SH_BP_ACTIVE: printf("BP_ACTIVE"); break;
	  case SH_BP_FULL: printf("BP_FULL"); break;
	  default: printf("XXXX\n");
	}
	printf("\n");
	printf("\tflags          ");
	if (buf->flags & SF_NEW_D2B) printf(" NEW_D2B");
	printf("\n");

	printf("\tmsg_flags      ");
	if (buf->msg_flags & SMF_READY) printf(" READY");
	if (buf->msg_flags & SMF_PENDING) printf(" PENDING");
	if (buf->msg_flags & SMF_MIDDLE) printf(" MIDDLE");
	if (buf->msg_flags & SMF_NEED_IFIELD) printf(" NEED_IFIELD");
	if (buf->msg_flags & SMF_NEOP) printf(" NEOP");
	if (buf->msg_flags & SMF_NEOC) printf(" NEOC");
	if (buf->msg_flags & SMF_CS_VALID) printf(" CS_VALID");
	printf("\n");
	printf("Data State\n");
	printf("\t*dp_put;       0x%08x\n", buf->dp_put);
	printf("\t*dp_noack;     0x%08x\n", buf->dp_noack);
	printf("\trem_len;       %d\n", buf->rem_len);
	printf("\t*basep         0x%08x\n", buf->basep);
	printf("\t*endp          0x%08x\n", buf->endp);
	printf("\tdata_M_len     %d\n", buf->data_M_len);
	printf("\n");
	printf("Chunk State\n");
	printf("\tchunks         %d\n",  buf->chunks);
	printf("\tchunks_left    %d\n", buf->chunks_left);
	printf("\tcksum_offs;    %d\n", buf->cksum_offs);
	printf("\n");
	printf("Bypass State\n");
	printf("\tcur_job        %d\n", buf->cur_job);
	printf("\tjob_vector     0x%08x\n", buf->job_vector);
	printf("\tbp_ulp         0x%08x\n", buf->bp_ulp);
	printf("\t*freemap       0x%08x\n", buf->freemap);
	printf("\t*job           0x%08x\n", buf->job);
	printf("\t*hostx         0x%08x\n", buf->hostx);
	printf("\t*sdq           0x%08x\n", buf->sdq);
	printf("\n");
	printf("Statistics pointers\n");
	printf("\t*stats         0x%08x\n", buf->stats);
	printf("\t*bpstats       0x%08x\n", buf->bpstats);
	
	free(buf);
    }
    else { /* destination */
	volatile dst_host_t *lbuf;
	dst_host_t *buf;

	printf("Destination Host Finite State Machine State\n");
	buf = (dst_host_t*)malloc(sizeof(dst_host_t));
	
	/* get ptr to struct */
	lbuf = (volatile dst_host_t*)
	    (K1_TO_PHYS(lstate->hostp) + (u_int)lmemsp);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(dst_host_t));
    
	printf("\tState          ");
	switch(buf->st) {
	  case DH_IDLE: printf("IDLE\n"); break;
	  case DH_WAIT_FPHDR: printf("WAIT_FPHDR\n"); break;
	  case DH_WAIT_FPBUF: printf("WAIT_FPBUF\n"); break;
	  case DH_FP: printf("FP\n"); break;
	  case DH_LE: printf("LE\n"); break;
	  case DH_BP: printf("BP\n"); break;
	  case DH_FEOP: printf("FEOP\n"); break;
	  default: printf("XXXX\n");
	}
	printf("\tflags          ");
	if (buf->flags & DF_NEOP) printf(" NEOP");
	if (buf->flags & DF_HOST_KNOWS) printf(" HOST_KNOWS");
	if (buf->flags & DF_NEW_D2B) printf(" NEW_D2B");
	if (buf->flags & DF_PENDING) printf(" PENDING");
	if (buf->flags & DF_STUFFING) printf(" STUFFING");
	if (buf->flags & DF_BP_DESC) printf(" BP_DESC");
	printf("\n");

	printf("\tdma_flags      ");
	if (buf->dma_flags & DDMA_START_PKT) printf(" START_PKT");
	if (buf->dma_flags & DDMA_CHAIN_CS) printf(" CHAIN_CS");
	if (buf->dma_flags & DDMA_SAVE_CS) printf(" SAVE_CS");
	printf("\n");

	printf("Data State\n");
	printf("\t*dp_put        0x%08x\n", buf->dp_put);
	printf("\t*basep         0x%08x\n", buf->basep);
	printf("\t*endp          0x%08x\n", buf->endp);
	printf("\tdata_M_len     %d\n", buf->data_M_len);
	printf("\n");

	printf("Stack State\n");
	printf("\tstack          %d\n", buf->stack);
	printf("\t*stackp        0x%08x\n", buf->stackp);
	printf("\n");

	printf("Length State\n");
	printf("\ttotal_len      0x%x (%d)\n", buf->total_len, buf->total_len);
	printf("\trem_len        %d\n", buf->rem_len);
	printf("\tcur_len        0x%x (%d)\n", buf->cur_len, buf->cur_len); 
	printf("\tcur_sm_buf_len %d\n", buf->cur_sm_buf_len);	
	printf("\tcur_lg_bufs    %d\n", buf->cur_lg_bufs);
	printf("\n");

	printf("Bypass State\n");
	printf("\tjob_vector     0x%08x\n", buf->job_vector);
	printf("\tbp_ulp         0x%08x\n", buf->bp_ulp);
	printf("\t*freemap       0x%08x\n", buf->freemap);
	printf("\t*job           0x%08x\n", buf->job);
	printf("\t*port          0x%08x\n", buf->port);
	printf("\t*bpseqnum      0x%08x\n", buf->bpseqnum);
	printf("\n");

	printf("Statistics pointers\n");
	printf("\t*stats         0x%08x\n", buf->stats);
	printf("\t*bpstats       0x%08x\n", buf->bpstats);
	printf("\n");
	
	printf("\tfpstk addr     0x%08x\n", ((u_int*)&(buf->fpstk[0]) - (u_int*)buf) + K1_TO_PHYS(lstate->hostp));
	printf("\tulptostk addr  0x%08x\n", ((u_int*)&(buf->ulptostk[0]) - (u_int*)buf) + K1_TO_PHYS(lstate->hostp));

	free(buf);
    }
}

void
ldump_wirep() 
{

    if (slot) { /* source */
	volatile src_wire_t *lbuf;
	src_wire_t *buf;

	buf = (src_wire_t*)malloc(sizeof(src_wire_t));
	/* get ptr to struct */
	lbuf = (volatile src_wire_t*)
	    (K1_TO_PHYS(lstate->wirep) + (u_int)lmemsp);

	printf("Reading from 0x%x\n", (u_int)lstate->wirep & 0xffffff);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(src_wire_t));

	printf("Source Wire Finite State Machine\n");
	printf("\tState           ");
	switch(buf->st) {
	  case SW_IDLE: printf("IDLE "); break;
	  case SW_NEOC: printf("NEOC "); break;
	  case SW_NEOP: printf("NEOP "); break;
	  case SW_ERR : printf("ERR  "); break;
	  default: printf("State=%d\n", buf->st);
	}
	printf("\n");
	printf("\tFlags           ");
	if (buf->flags & SBLK_NEOP) printf(" NEOP");
	if (buf->flags & SBLK_NEOC) printf(" NEOC");
	if (buf->flags & SBLK_NACK) printf(" NACK");
	if (buf->flags & SBLK_BEGPC) printf(" BEGPC");
	if (buf->flags & SBLK_SOC) printf(" SOC");
	if (buf->flags & SBLK_BP) printf(" BP");
	if (buf->flags & SBLK_REM) printf(" REMAINDER");
	printf("\n\n");
	printf("\tAck FSM state   ");
	switch(buf->ack_st) {
	  case SW_IDLE: printf("IDLE "); break;
	  case SW_NEOC: printf("NEOC "); break;
	  case SW_NEOP: printf("NEOP "); break;
	  case SW_ERR : printf("ERR  "); break;
	  default: printf("State=%d\n", buf->st);
	}
	printf("\n");
	printf("\tFlags           ");
	if (buf->ack_flags & SBLK_NEOP) printf(" NEOP");
	if (buf->ack_flags & SBLK_NEOC) printf(" NEOC");
	if (buf->ack_flags & SBLK_NACK) printf(" NACK");
	if (buf->ack_flags & SBLK_BEGPC) printf(" BEGPC");
	if (buf->ack_flags & SBLK_SOC) printf(" SOC");
	if (buf->ack_flags & SBLK_BP) printf(" BP");
	if (buf->ack_flags & SBLK_REM) printf(" REMAINDER");
	printf("\n\n");
	
	free(buf);
    }
    else { /* destination */
	volatile dst_wire_t *lbuf;
	dst_wire_t *buf;

	buf = (dst_wire_t*)malloc(sizeof(dst_wire_t));
	/* get ptr to struct */
	lbuf = (volatile dst_wire_t*)
	    (K1_TO_PHYS(lstate->wirep) + (u_int)lmemsp);

	printf("Reading from 0x%x\n", (u_int)lstate->wirep & 0xffffff);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(dst_wire_t));

	printf("Destination Wire Finite State Machine\n");
	printf("\tState          ");
	switch(buf->st) {
	  case DW_NEOP: printf("NEOP "); break;
	  case DW_NEOC: printf("NEOC "); break;
	  case DW_NO_FP: printf("NO_FP "); break;
	  default: printf("State=%d\n", buf->st);
	}
	printf("\n");
	printf("\tblks_sent      0x%x\n", buf->blks_sent);
	printf("\told_blks_sent  0x%x\n", buf->old_blks_sent);

	free(buf);
    }

}

void
ldump_stats() 
{
  volatile hippi_stats_t *lbuf;
  hippi_stats_t *buf;

  buf = (hippi_stats_t*)malloc(sizeof(hippi_stats_t));

  if (slot) { /* source */
	printf("Source Statistics\n");
	
	/* get ptr to struct */
	lbuf = (volatile hippi_stats_t*)
	    (K1_TO_PHYS(lstate->stats) + (u_int)lmemsp);

	printf("Reading from 0x%x\n", (u_int)lstate->stats & 0xffffff);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(hippi_stats_t));

	printf("\tflags: ");
	if(buf->hst_flags & HST_FLAG_LOOPBACK) printf(" LOOPBACK");
	if(buf->hst_flags & HST_FLAG_DST_ACCEPT) printf(" DST_ACCEPT");
	if(buf->hst_flags & HST_FLAG_DST_PKTIN) printf(" DST_PKTIN");
	if(buf->hst_flags & HST_FLAG_DST_REQIN) printf(" DST_REQIN");
	if(buf->hst_flags & HST_FLAG_SRC_REQOUT) printf(" SRC_REQOUT");
	if(buf->hst_flags & HST_FLAG_SRC_CONIN) printf(" SRC_CONIN");
	if(buf->hst_flags & HST_FLAG_DST_LNK_RDY) printf(" DST_LNK_RDY");
	if(buf->hst_flags & HST_FLAG_DST_FLAG_SYNC) printf(" DST_FLAG_SYNC");
	if(buf->hst_flags & HST_FLAG_DST_OH8_SYNC) printf(" DST_OH8_SYNC");
	if(buf->hst_flags & HST_FLAG_DST_SIG_DET) printf(" DST_SIG_DET");
	printf("\n");
	printf("\tsrc_conns:\t%u\n", buf->hst_s_conns);
	printf("\tsrc_pkts:\t%u\n", buf->hst_s_packets);
	printf("\n");
	printf("\trejects:\t%u\n", buf->sf.hip_s.rejects);
	printf("\txmit_retry:\t%u\n", buf->sf.hip_s.xmit_retry);
	printf("\tresvd0:\t%u\n", buf->sf.hip_s.resvd0);
	printf("\tglink_resets:\t%u\n", buf->sf.hip_s.glink_resets);
	printf("\tglink_err:\t%u\n", buf->sf.hip_s.glink_err);
	printf("\ttimeo:\t\t%u\n", buf->sf.hip_s.timeo);
	printf("\tconnls:\t\t%u\n", buf->sf.hip_s.connls);
	printf("\tpar_err:\t%u\n", buf->sf.hip_s.par_err);
	printf("\tresvd1[4]:\t%u\n", buf->sf.hip_s.resvd1[4]);
	printf("\tnumbytes_hi:\t%u\n", buf->sf.hip_s.numbytes_hi);
	printf("\tnumbytes_lo:\t%u\n", buf->sf.hip_s.numbytes_lo);
	printf("\n");

  }

  else { /* dest */
	printf("Destination Statistics\n");
	
	/* get ptr to struct */
	lbuf = (volatile hippi_stats_t*)
	    (K1_TO_PHYS(lstate->stats) + (u_int)lmemsp);
    
	printf("Reading from 0x%x\n", (u_int)lstate->stats & 0xffffff);
    
	/* read the buffer into a local buffer */
	memcpy ((char *)buf, 
		(const void *) lbuf, sizeof(hippi_stats_t));
    
	printf("\tflags: ");
	if(buf->hst_flags & HST_FLAG_LOOPBACK) printf(" LOOPBACK");
	if(buf->hst_flags & HST_FLAG_DST_ACCEPT) printf(" DST_ACCEPT");
	if(buf->hst_flags & HST_FLAG_DST_PKTIN) printf(" DST_PKTIN");
	if(buf->hst_flags & HST_FLAG_DST_REQIN) printf(" DST_REQIN");
	if(buf->hst_flags & HST_FLAG_SRC_REQOUT) printf(" SRC_REQOUT");
	if(buf->hst_flags & HST_FLAG_SRC_CONIN) printf(" SRC_CONIN");
	if(buf->hst_flags & HST_FLAG_DST_LNK_RDY) printf(" DST_LNK_RDY");
	if(buf->hst_flags & HST_FLAG_DST_FLAG_SYNC) printf(" DST_FLAG_SYNC");
	if(buf->hst_flags & HST_FLAG_DST_OH8_SYNC) printf(" DST_OH8_SYNC");
	if(buf->hst_flags & HST_FLAG_DST_SIG_DET) printf(" DST_SIG_DET");
	printf("\n");
	printf("\thst_d_conns:\t%u\n", buf->hst_d_conns);
	printf("\thst_d_pkts:\t%u\n", buf->hst_d_packets);
	printf("\n");
	printf("\tbadulps:\t%u\n", buf->df.hip_s.badulps);
	printf("\tledrop:\t\t%u\n", buf->df.hip_s.ledrop);
	printf("\tllrc:\t\t%u\n", buf->df.hip_s.llrc);
	printf("\tpar_err:\t%u\n", buf->df.hip_s.par_err);
	printf("\tframe_state_err:\t%u\n", buf->df.hip_s.frame_state_err);
	printf("\tflag_err:\t%u\n", buf->df.hip_s.flag_err);
	printf("\tillbrst:\t%u\n", buf->df.hip_s.illbrst);
	printf("\tpkt_lnklst_err:\t%u\n", buf->df.hip_s.pkt_lnklost_err);
	printf("\tnullconn:\t%u\n", buf->df.hip_s.nullconn);
	printf("\trdy_err:\t%u\n", buf->df.hip_s.rdy_err);
	printf("\tbad_pkt_st_err:\t%u\n", buf->df.hip_s.bad_pkt_st_err);
	printf("\tresvd:\t\t%u\n", buf->df.hip_s.resvd);
	printf("\tnumbytes_hi:\t%u\n", buf->df.hip_s.numbytes_hi);
	printf("\tnumbytes_lo:\t%u\n", buf->df.hip_s.numbytes_lo);
	printf("\n");

  }

  free(buf);
}

void
ldump_bpconfig()
{
  /* bypass configuration */
  volatile hip_bp_fw_config_t *lbuf;
  hip_bp_fw_config_t *buf;

  buf = (hip_bp_fw_config_t*)malloc(sizeof(hip_bp_fw_config_t));
 /* get a ptr to the data */
  lbuf=(volatile hip_bp_fw_config_t*)
        (K1_TO_PHYS(lstate->bpconfig)+(u_int)lmemsp);

  memcpy((char*)buf,(const void *)lbuf,sizeof(hip_bp_fw_config_t));
  printf("\tnum_jobs:\t%u\n",buf->num_jobs);
  printf("\tnum_ports:\t%u\n",buf->num_ports);
  printf("\thostx_base:\t0x%x\n",buf->hostx_base);
  printf("\thostx_size:\t%u\n",buf->hostx_size);
  printf("\tdfl_base:\t0x%x\n",buf->dfl_base);
  printf("\tdfl_size:\t%u\n",buf->dfl_size);
  printf("\tsfm_base:\t0x%x\n",buf->sfm_base);
  printf("\tsfm_size\t%u\n",buf->sfm_size);
  printf("\tdfm_base\t0x%x\n",buf->dfm_base);
  printf("\tdfm_size\t%u\n",buf->dfm_size);
  printf("\tbpstat_base\t0x%x\n",buf->bpstat_base);
  printf("\tbpstat_size\t%u\n",buf->bpstat_size);
  printf("\tsdq_base\t0x%x\n",buf->sdq_base);
  printf("\tsdq_size\t%u\n",buf->sdq_size);
  printf("\tbpjob_base\t0x%x\n",buf->bpjob_base);
  printf("\tbpjob_size\t%u\n",buf->bpjob_size);
  printf("\tdma_status\t%u\n",buf->dma_status);
  printf("\tmailbox_base\t0x%x\n",buf->mailbox_base);
  printf("\tmailbox_size\t%u\n",buf->mailbox_size);
  free (buf);
}

void
ldump_bpstats() 
{

  /* bypass statictics */
  volatile hippibp_stats_t *lbuf;
  hippibp_stats_t *buf;
  int i=0;
  int cnt=0;

  buf = (hippibp_stats_t*)malloc(sizeof(hippibp_stats_t));
  lbuf = (volatile hippibp_stats_t*)
            (K1_TO_PHYS(lstate->bpstats) + (u_int)lmemsp);

  printf("Reading from 0x%x\n", (u_int)lstate->stats & 0xffffff); 
  /* read the buffer into a local buffer */
  memcpy ((char *)buf,
          (const void *) lbuf, sizeof(hippibp_stats_t));
  printf("\tJobs:\n\t");
  for (i=0,cnt=1; i < HIPPIBP_MAX_JOBS; i++, cnt++) {
    printf("%d: ", i);
    if ((buf->hst_bp_job_vec>>(31-i)) & 0x1)
       printf("BUSY ");
    else 
       printf("IDLE ");
    if ((cnt & 6) == 6) printf("\n\t"), cnt=0;
  }
  printf("\n");
  printf("\tbp_ulp:\t%u\n",buf->hst_bp_ulp);
  if (slot) { /* source */
	printf("Source Bypass Statistics\n");
    	printf("\tbp_descs:\t%u\n",buf->hst_s_bp_descs);
    	printf("\tbp_packets:\t%u\n",buf->hst_s_bp_packets);
    	printf("\tbp_byte_count:\t%u\n",(u_int)buf->hst_s_bp_byte_count);
	printf("Source Bypass Errors\n");
    	printf("\tbp_desc_hostx_err:\t%u\n",buf->hst_s_bp_desc_hostx_err);
    	printf("\tbp_desc_bufx_err:\t%u\n",buf->hst_s_bp_desc_bufx_err);
    	printf("\tbp_desc_opcode_err:\t%u\n",buf->hst_s_bp_desc_opcode_err);
    	printf("\tbp_desc_addr_err:\t%u\n",buf->hst_s_bp_desc_addr_err);

  } else {
 	printf("Destination Bypass Statistics\n");
	printf("\tbp_descs:\t%u\n",buf->hst_d_bp_descs);	
  	printf("\tbp_packets:\t%u\n",buf->hst_d_bp_packets); 
   	printf("\tbp_byte_count:\t%u\n",(u_int)buf->hst_d_bp_byte_count);
	printf("Destination Bypass Errors\n");
   	printf("\tbp_port_err\t%u\n",buf->hst_d_bp_port_err);
  	printf("\tbp_job_err\t%u\n",buf->hst_d_bp_job_err);
  	printf("\tbp_no_pgs_err:\t%u\n",buf->hst_d_bp_no_pgs_err);
  	printf("\tbp_bufx_err:\t%u\n",buf->hst_d_bp_bufx_err);
  	printf("\tbp_auth_err:\t%u\n",buf->hst_d_bp_auth_err);
  	printf("\tbp_off_err:\t%u\n",buf->hst_d_bp_off_err);
  	printf("\tbp_opcode_err:\t%u\n",buf->hst_d_bp_opcode_err);
  	printf("\tbp_vers_err:\t%u\n",buf->hst_d_bp_vers_err);
  	printf("\tbp_seq_err:\t%u\n",buf->hst_d_bp_seq_err);
  }
  free(buf);
}

void
ldump_bpjob(int jobnum) 
{

  volatile bp_job_state_t *lbuf;
  bp_job_state_t *buf;
  buf = (bp_job_state_t*)malloc(sizeof(bp_job_state_t));
  lbuf = (volatile bp_job_state_t*)
            (K1_TO_PHYS(lstate->job) + (u_int)lmemsp);
  printf("Reading from 0x%x\n", (u_int)lstate->stats & 0xffffff); 
  /* read the buffer into a local buffer */
  memcpy ((char *)buf,(const void *)lbuf,sizeof(bp_job_state_t));
  if ((u_int)K1_TO_PHYS(buf->sdq_head) == (u_int)K1_TO_PHYS(buf->sdq_end))
      printf("\n\tqueue is empty\n");
  else {
      printf("\tsdq_head:\t0x%x\n",buf->sdq_head);
      printf("\tsdq_end:\t0x%x\n",buf->sdq_end);
      printf("\tauth[1..3]:\t0x%x 0x%x 0x%x\n",
	     buf->auth[0],buf->auth[1],buf->auth[2]);
      printf("\tfm_entry_size:\t%u\n",buf->fm_entry_size);
      printf("\tack_hostport:\t0x%x\n",buf->ack_hostport);
      /* for at the bypass descriptors for the job in question */
      {
	  volatile hippi_bp_desc *zbuf;
	  hippi_bp_desc *dbuf;
	  dbuf=(hippi_bp_desc *)malloc(sizeof(hippi_bp_desc));
	  zbuf=(volatile hippi_bp_desc *)
	      (K1_TO_PHYS(&buf->sdq_head[jobnum])+ (u_int)lmemsp);
	  memcpy((char*)dbuf,(const void *)zbuf,sizeof(hippi_bp_desc));
	  free (dbuf);
      }
  }
  free (buf);
}

void
ldump_regs() 
{


}

void
ldump_all()
{
    ldump_pcicfg();
    ldump_control();
    ldump_lmcfg();
    ldump_dma();
    ldump_trace();
    ldump_state();
    ldump_hostp();
    ldump_wirep();
    ldump_blk();
    ldump_ablk();
    ldump_wblk();
    ldump_mbuf();
    ldump_fpbuf();
    ldump_stats();
    ldump_bpconfig();
    ldump_bpstats();

    /* ldump_bpjob(0); -- NOT YET */
    ldump_rr2l();
    ldump_l2rr();
    ldump_b2h();
    ldump_d2b();
    ldump_opposite();
}



void
help_ld()
{
    printf ("Display Linc registers:\n");
    printf ("\tld pcicfg  -- PCI Config space headers      - NOT DONE YET\n");
    printf ("\tld control -- general control registers     - NOT DONE YET\n");
    printf ("\tld lmcfg   -- local memory config registers - NOT DONE YET\n");
    printf ("\tld dma     -- DMA registers                 - NOT DONE YET\n");
    printf ("\tld regs    -- all Linc registers            - NOT DONE YET\n\n");
    printf ("Display Linc memory:\n");
    printf ("\tld mem <SDRAM offset> <# words> -- Linc SDRAM memory\n\n");
    printf ("Display Linc Firmware State:\n");
    printf ("\tld trace   -- trace buffer\n\n");
    printf ("\tld state   -- main firmware state structure\n");
    printf ("\tld hostp   -- host finite state machine's state\n");
    printf ("\tld wirep   -- wire finite state machine's state\n");
    printf ("\tld blk     -- source Linc's block state\n");
    printf ("\tld ablk    -- destination Linc's ack block state\n");
    printf ("\tld wblk    -- destination Linc's wire block state\n");
    printf ("\tld mbuf    -- destination Linc's mbuf queue\n");
    printf ("\tld fpbuf   -- destination Linc's FP buffer queue\n");
    printf ("\tld bpjob <num> - bypass job state\n\n");
    printf ("\tld stats   -- statistics counters\n");
    printf ("\tld bpstats -- bypass statistics counters\n");
    printf ("\n");
    printf ("Display Linc Queues:\n");
    printf ("\tld rr2l    -- Roadrunner to Linc queue and state\n");
    printf ("\tld l2rr    -- Linc to Roadrunner queue and state\n");
    printf ("\tld b2h     -- board to host queue and state\n");
    printf ("\tld d2b     -- data to board queue and state\n");
    printf ("\tld opp     -- opposite's queue\n");
    printf ("\tld all     -- all above - USE WITH CARE - dumps a LOT of state (doesn't dump all mem)\n");
    
}

int
ldump(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    char * cmd;
    int  n;
    
    if (argc == 1) {
        printf("invalid number of params\n");
	return TCL_OK;
    }
    cmd = argv[1];
    n = strlen(cmd);

    /* Try to pick unique starting letters for cmd names
     * to make abbreviations easier.
     */
    if (strncmp (cmd, "pcicfg", n) == 0) {
	ldump_pcicfg();
	return TCL_OK;
    } else if (strncmp (cmd, "state", n) == 0) {
	ldump_state();
	return TCL_OK;
    } else if (strncmp (cmd, "control", n) == 0) {
	ldump_control();
	return TCL_OK;
    } else if (strncmp (cmd, "lmcfg", n) == 0) {
	ldump_lmcfg();
	return TCL_OK;
    } else if (strncmp (cmd, "dma", n) == 0) {
	ldump_dma();
	return TCL_OK;
    } else if (strncmp (cmd, "rr2l", n) == 0) {
	ldump_rr2l();
	return TCL_OK;
    } else if (strncmp (cmd, "l2rr", n) == 0) {
	ldump_l2rr();
	return TCL_OK;
    } else if (strncmp (cmd, "b2h", n) == 0) {
	ldump_b2h();
	return TCL_OK;
    } else if (strncmp (cmd, "d2b", n) == 0) {
	ldump_d2b();
	return TCL_OK;
    } else if (strncmp (cmd, "opposite", n) == 0) {
	ldump_opposite();
	return TCL_OK;
    } else if (strncmp (cmd, "blk", n) == 0) {
	ldump_blk();
	return TCL_OK;
    } else if (strncmp (cmd, "ablk", n) == 0) {
	ldump_ablk();
	return TCL_OK;
    } else if (strncmp (cmd, "all", n) == 0) {
	ldump_all();
	return TCL_OK;
    } else if (strncmp (cmd, "wblk", n) == 0) {
	ldump_wblk();
	return TCL_OK;
    } else if (strncmp (cmd, "mbuf", n) == 0) {
	ldump_mbuf();
	return TCL_OK;
    } else if (strncmp (cmd, "fpbuf", n) == 0) {
	ldump_fpbuf();
	return TCL_OK;
    } else if (strncmp (cmd, "hostp", n) == 0) {
	ldump_hostp();
	return TCL_OK;
    } else if (strncmp (cmd, "wirep", n) == 0) {
	ldump_wirep();
	return TCL_OK;
    } else if (strncmp (cmd, "stats", n) == 0) {
	ldump_stats();
	return TCL_OK;
    } else if (strncmp (cmd, "bpconfig", n) == 0) {
        ldump_bpconfig();
        return TCL_OK;
    } else if (strncmp (cmd, "bpstats", n) == 0) {
	ldump_bpstats();
	return TCL_OK;
    } else if (strncmp (cmd, "bpjob", n) == 0) {
	int job;
	if (argc != 3) {
	    interp->result = "USAGE: ldump bpjob <job_num>";
	    return TCL_ERROR;
	}
	job = strtoul(argv[2], NULL, 0);
	if ((job < 0) || (job > HIPPIBP_MAX_JOBS)) {
	    printf ("\"%s\" is not a legal Job number. Range is 0 to %x\n",
		    argv[2], HIPPIBP_MAX_JOBS);
	    return TCL_ERROR;
	}
	ldump_bpjob(job);
	return TCL_OK;
    } else if (strncmp (cmd, "regs", n) == 0) {
	ldump_regs();
	return TCL_OK;
    } else if (strncmp (cmd, "mem", n) == 0) {
	u_int	addr;
	int     numwords;
	if ( !(argc == 4 || argc == 3)) {
	    interp->result = "USAGE: ldump mem <SDRAM offset> <# words>";
	    return TCL_ERROR;
	}
	if (argc == 3)
	    numwords = 32;
	else
	    numwords = strtoul(argv[3], NULL, 0);
	addr = strtoul(argv[2], NULL, 0);
	if ((addr < 0)) {
	    printf ("\"%s\" is not a legal SDRAM offset. Range is 0 to %x\n",
		    argv[2], SDRAM_SIZE);
	    return TCL_ERROR;
	}
	ldump_mem(addr, numwords);
	return TCL_OK;
    } else if (strncmp (cmd, "trace", n) == 0) {
	if (ldump_trace() == 0)
	    return TCL_OK;
	else
	    return TCL_ERROR;
    } else if (strncmp (cmd, "all", n) == 0) {
	ldump_all();
    }
    printf ("argument \"%s\" not supported.\n", cmd);
    help_ld();
    return TCL_ERROR;
}

void
help_lintr()
{
    printf ("Interrupt the 4640\n");
    printf ("\tCommand requires that 4640 firmware be operational\n");
    printf ("\tInterrupts the 4640, which causes the firmware to\n");
    printf ("\tflush its data cache.\n");
    printf ("\t\tIf running GDB image - causes firmware to hit a breakpoint\n");
    printf ("\t\tIf running prod. image - firmware continues after flush\n");
    printf ("\t\tWorks exactly the same as \"lfcache\"\n");
}


int
intr_4640(void)
{
    volatile u_int *cisr;
    
    cisr = (volatile u_int*)(lmemsp + LINC_CISR);
    *cisr = LINC_CISR_FIRM_INTR;

    sginap(1);

    if (*cisr & LINC_CISR_FIRM_INTR) {
	printf("WARNING: Linc did not clear interrupt to show successful cache flush\n");
	return TCL_ERROR;
    }
    else
	printf("Linc cache flush was successful\n");
    sginap(10);
    return TCL_OK;


}


int
lintr(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    return intr_4640();
}

void
help_lfcache()
{
    printf ("Flush data in 4640 data cache to SDRAM\n");
    printf ("\tCommand requires that 4640 firmware be operational\n");
    printf ("\tInterrupts the 4640, which causes the firmware to\n");
    printf ("\tflush its data cache.\n");
    printf ("\t\tIf running GDB image - causes firmware to hit a breakpoint\n");
    printf ("\t\tIf running prod. image - firmware continues after flush\n");
    printf ("\t\tWorks exactly the same as \"lintr\"\n");
}

#endif /* G2P_SIM */


void
help_dload()
{
printf ("dload <filename> <startaddress>\n");
printf ("\t<filename> is a file containing hex strings, 1 word per line.\n");
printf ("\t\tFirst line contains the start PC - this is not downloaded\n");
printf ("\t\tinto SRAM, but rather loaded into the PC before the RR\n");
printf ("\t\tis restarted.\n");
printf ("\t<startaddress> is the address in SRAM to start loading the\n");
printf ("\t\tsubsequent words of the f/w file.\n");
}

int
dload(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    uint_t startaddr;
    uint_t startPC;
    int offset, line, len, i, j;
    char    aline[256];
    uint_t * uintp;
    char *buf, *from, *to;
    FILE *fp;

    if ((argc < 2) || (argc > 3)) {
	fprintf (stderr, "USAGE: dload <fname> <startaddr>");
	return TCL_ERROR;
    }

    if (argc == 3) {
	startaddr = strtoul (argv[2], 0, 0);
	if ((startaddr == 0) || (startaddr & 3)) {
	    fprintf (stderr, "starting address has to be non-zero and word aligned\n");
	    return TCL_ERROR;
	}
    }
    else
	startaddr = 4;

    fp = fopen (argv[1], "r");
    if (fp == 0) {
	fprintf (stderr, "Couldn't open file %s: %s\n", 
		 argv[1], strerror(errno));
	return TCL_ERROR;
    }

    line = 0;
    if (fgets (aline, sizeof(aline), fp) == 0) {
	fprintf (stderr, "File %s is empty.\n", argv[1]);
	return TCL_ERROR;
    }
    if (strlen (aline) != 9) {
	fprintf (stderr, "File %s line %d: bad format\n", argv[1], line+1);
	return TCL_ERROR;
    }
    startPC = hexToUInt (aline);

    buf = malloc (64 * 1024);
    uintp = (uint_t *) buf;
    len = 0;

    while (fgets (aline, sizeof(aline), fp)) {
	if (++line > (16*1024)) {
	    fprintf (stderr, "F/w file exceeds 64KB\n");
	    free (buf);
	    return TCL_ERROR;
	}
	if (strlen (aline) != 9) {
	    fprintf (stderr, "File %s line %d: bad format\n", 
			     argv[1], line+1);
	    free (buf);
	    return TCL_ERROR;
	}
	*uintp++ = hexToUInt(aline);
	len += 4;
    }

    /* Halt the RR cpu first */
    printf ("Halting RR PC\n");
    memsp->misc_host_ctrl_reg = RR_HALT;
    /* Read it back and make sure it's halted */
    while (1) {
	i = memsp->misc_host_ctrl_reg;
	if (i & RR_HALTED)
	    break;
    }
    printf ("Roadrunner halted.\n");

    /* Zero out all 256 KB of SRAM to make sure we have good 
     * parity throughout. */
    printf ("Zeroing out SRAM.\n");
    for (i = 0; i < 128; i++) {
	memsp->win_base_reg = i * 2048;
	j = 0;
	while (j < 512) 
	    memsp->rr_sram_window[j++] = 0;
    }

    printf ("Begin download of %d(0x%x) bytes to SRAM address 0x%x\n",
	    len, len, startaddr);
    /*  Move the SRAM window to the right 2KB segment */
    memsp->win_base_reg = (startaddr & 0xFFFFF800);
    offset = startaddr & 0x7ff;
    from = buf;
    while (len > 0) {
	i = 0x800 - offset;	/* length to end of window */
	if (i > len)
	    i = len;
	to = (char * ) (memsp->rr_sram_window);
	to += offset;
	printf ("bcopy(%x,%x,%x)\n", from, to, i);
	bcopy (from, to, i);
	offset = 0;
	from += i;
	memsp->win_base_reg += 0x800;
	len -= i;
    }
    printf ("Firmware downloaded\n");

    /* Now write the PC and start the RR */
    memsp->prog_counter_reg = startPC;
    if (breakpoint != -1) {
	printf ("Breakpoint is currently set at 0x%08x\n", breakpoint);
	printf ("Clear breakpoint before restart? [n] : ");
	buf[0] = '\0';
	read (0, buf, sizeof buf - 1);
	if ((buf[0] == 'y') || (buf[0] == 'Y')) {
	    breakpoint = -1;
	    memsp->breakpoint_reg = 1;
	}
    }
    memsp->misc_host_ctrl_reg &= ~RR_HALT;
    printf ("Roadrunner restarted at PC=%08x\n", startPC);

    free (buf);
    return TCL_OK;    
}

void
registerTclCommands()
{
    Tcl_CreateCommand(interp, "h", help,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "q", quit,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "!", shell,
                      (ClientData) NULL,
                      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "help", help,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "?", help,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ------------ Program Control Commands ------------------ */
    Tcl_CreateCommand(interp, "halt", halt,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "bp", bp,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "ss", ss,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "r", resume,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ------------ Read memory and registers ----------------- */
    Tcl_CreateCommand(interp, "d", dump,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ------------ Write memory and registers ---------------- */
    Tcl_CreateCommand(interp, "ww", ww,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "ws", ws,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "wb", wb,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "set", set,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ----------- Write PCI Config registers ------------------ */
    Tcl_CreateCommand(interp, "cfg", cfg,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
#ifdef RR_DEBUG
    Tcl_CreateCommand(interp, "dma", dmatest,	/* test DMA */
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "da1", datest1,	/* DMA Assist Test 1 */
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "da2", datest2,	/* DMA Assist Test 2 */
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "sram", sramtest, /* Test SRAM */
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
#endif	/* RR_DEBUG */

    /* ------------ Write/Read LINC memory and registers ----------------- */
#ifndef G2P_SIM
    Tcl_CreateCommand(interp, "ld", ldump,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lww", lww,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lwl", lwl,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lrw", lrw,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lset", lset,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lrefresh", lrefresh,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateCommand(interp, "ldump", ldump,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ------------  LINC cache control/program control ------ */
    Tcl_CreateCommand(interp, "lintr", lintr,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "lfcache", lintr,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);








    Tcl_CreateCommand(interp, "ldload", ldload,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);

    /* ------------ Read LINC queues------------------------- */


    /* ------------ Write/Read LINC config space ----------------- */

#endif

    /* ------------- Download RR firmware --------------------- */
    Tcl_CreateCommand(interp, "dload", dload,
		      (ClientData) NULL,
		      (Tcl_CmdDeleteProc *)NULL);
}

void
cleanexit()
{
    char buf[80];

    /* if breakpoint is set, clear it before leaving
     * so we don't leave board hanging.
     */
    if (breakpoint != -1) {
	printf ("Breakpoint is currently set at %08x\n", breakpoint);
	printf ("Clear breakpoint before exit? ");
	read (0, buf, sizeof buf - 1);
	if ((buf[0] == 'y') || (buf[0] == 'Y')) {
	    breakpoint = -1;
	    memsp->breakpoint_reg = 1;
	}
    }
#ifdef G2P_SIM
    if (memsp != (volatile rr_pci_mem_t *)-1)
	munmap ((void *)memsp, RR_PCI_MEM_SIZE);
    if (cfgsp != (volatile pci_cfg_hdr_t *)-1)
	munmap ((void *)cfgsp, sizeof (pci_cfg_hdr_t));
    if (sbcreg != (volatile u_int *)-1)
	munmap ((void *)sbcreg, 4);
#else
    if (lmemsp != (volatile char *)-1)
	munmap ((void *)lmemsp, LINC_PCI_MEM_SIZE);
    if (lcfgsp != (volatile pci_cfg_hdr_t *)-1)
	munmap ((void *)lcfgsp, sizeof (pci_cfg_hdr_t));
#endif
    exit(0);
}

void
breakloop(int sig)
{
    if (sig == SIGINT)
	longjmp(jmp_env, 1);
}

/* -------------------- Misc tools and utilities ------------------ */

/*
 * legal_iaddr(addr)
 * returns 1 if addr looks right for an SRAM instruction address,
 * 0 otherwise.
 */
int
legal_iaddr(int addr)
{
    /* XXX TBD: more checks when we pin down where f/w lives */
    if ((addr < 0) || (addr & 0x3))
	return 0;
    return 1;
}

/*
 *  Convert a single ascii hex character to a 4 bit value.
 */
char
hexToNibble (char c)
{
    if ( c >= '0' && c <= '9' )
	return (char) ( c - '0' );
    else if ( c >= 'A' && c <= 'F' )
	return (char) ( c - 'A' + 10 );
    else if ( c >= 'a' && c <= 'f' )
	return (char) ( c - 'a' + 10 );
    else
	return 0;
}

/*
 * Convert 8 bytes of ascii hex characters into a 32-bit value.
 */

int
hexToUInt (char* s)
{
    char * cp;
    uint_t result;
    int i;

    cp = (char *) &result;
    for (i = 0; i < 4; i++) {
	*cp = ((hexToNibble (s[0]) << 4) + hexToNibble (s[1]));
	s += 2;
	cp++;
    }
    return result;
}

