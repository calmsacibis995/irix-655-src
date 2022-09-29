;
; **************************************************************************
; *									   *
; * 		 Copyright (C) 1994 Silicon Graphics, Inc.		   *
; *									   *
; *  These coded instructions, statements, and computer programs  contain  *
; *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
; *  are protected by Federal copyright law.  They  may  not be disclosed  *
; *  to  third  parties  or copied or duplicated in any form, in whole or  *
; *  in part, without the prior written consent of Silicon Graphics, Inc.  *
; *									   * 
; **************************************************************************
;
; ehip.s
;
; Firmware for Everest HIPPI
;
; $Revision: 1.23 $
;
	.file	"ehip.s"

	.equ	AM29030,1

	.include "std.h"
	.include "ehiphw.h"

	.include "ehip.h"

	.sect	code,text,absolute FIRM_TEXT
	.sect	bss,bss,absolute FIRM_SBSS
	.sect	dbss,bss,absolute FIRM_DBSS

	; the least significant two bits are reserved so we can have
	; up to 4 different "types" of firmware for specials etc.
	.equ	VERS,((((((VERS_Y-93)*12+VERS_M)*32+VERS_D)*24+VERS_H)*60+VERS_MM) & 0xFFFFFC )

	.equ	DEBUG,0
	.equ	P0_DEBUG,1
	; .equ	TAILFILL,1


; ----------------------------------
;         Source DRAM data area
; ----------------------------------

	.use	bss

; Host command area lives here.

host_cmd:	.block	HC_CMD_RSIZE		; from host command block
host_resp:	.block	HC_CMD_WSIZE		; to host response block
host_stat_buf:	.block	HST_STATS_SIZE		; to DMA stats

; same as host init command...

hostp_b2h:	.block	8		; address of b2h in host
b2h_len:	.block	4
iflags:		.block	4		; initial setting of op_flags
hostp_d2b:	.block	8
d2b_len:	.block	4
host_nbpp_mlen:	.block	4
hostp_c2b:	.block	8
hostp_stat:	.block	8

		; Cache line align C2B area...
		.block	( (.+(NBPCL-1)) & ~(NBPCL-1) ) - .

c2b:		.block	C2B_B_SIZE
c2b_end:
		; C2B area needs to be at least two cache lines
		CK ( C2B_B_SIZE >= 2* NBPCL )

		; C2B end should be (well) before half block boundary.
		CK ( c2b_end < SRC_DRAM+TPDRAM_HBLKSIZE )

		; D2B area must start in second half block and
		; must be (host) cache line boundary

		.if ( . >= SRC_DRAM+TPDRAM_HBLKSIZE )
		.block	( (.+(NBPCL-1)) & ~(NBPCL-1) ) - .
		.else
		.block	( SRC_DRAM+TPDRAM_HBLKSIZE-. )
		.endif

d2b:		.block	D2B_B_SIZE
d2b_end:
		
c2b_read:	.block	4096+16		; 2MB reads

; bypass buffers -- must be in sync with ../kern/mips/hippi.c


bp_structs_start:			
bp_config_region:       .block  BP_CONFIG_REGION
bp_hostx:	.block	BP_MAX_HOSTX*BP_MAX_JOBS*4        ; max fields in ifield lookup table
bp_stats:	.block	BP_STATS_BUF*4               ; bypass status fields
bp_job_state:	.block  BP_JOB_SIZE*BP_MAX_JOBS

;; add enough to round to a 16 K page (for ease of accounting)

		.block  ((.+(DEFAULT_NBPP-1)) & ~(DEFAULT_NBPP-1) ) - . 

bp_sdqueue:     .block  BP_SDQ_SIZE*BP_MAX_JOBS ; source descriptor queue per job
bp_dfreelist:   .block  BP_DFL_SIZE*BP_MAX_JOBS ; freelist per job
bp_sfreemap:    .block  BP_SFM_SIZE*BP_MAX_JOBS ; source lookup table per job to get phys addr
bp_dfreemap:    .block  BP_DFM_SIZE*BP_MAX_JOBS ; dest lookup table per job to get phys addr

;  bypass control state for each job and port.

bp_port_state:	 .block  BP_PORT_SIZE*BP_MAX_PORTS

bp_slot_table:   .block BP_SLOT_TABLE_SIZE*BP_MAX_JOBS ; <get, end, seq #> for each slot


bp_structs_end:	 

		.equ	BP_STRUCTS_SIZE,.-bp_structs_start
	

		CK ( ( . - FIRM_SBSS ) < SBSS_SIZE )




; ----------------------------------
; Destination DRAM data area
; ----------------------------------
	.use	dbss

b2h:		.block	B2H_B_SIZE
b2h_end:


; header bufs for stacks:

hdrbufs:	.block	(16*8)

; c2b read host physical pages

c2b_read_tbl:	.block	(16*8)

; map from ULP to stack number...

ulptostk:	.block	256*4
stktoulp:	.block	16*4		; XXX: needed???

; if_hip pool of mbufs...

ifhip_sml:	.block	(MAX_SML+1)*8
ifhip_sml_end:
ifhip_big:	.block	(MAX_BIG+1)*8
ifhip_big_end:

;;; temporary storage for bypass destination descriptor for DMA
dst_bp_desc:	.block	BP_DESC_SIZE

		CK ( ( . - FIRM_DBSS ) < DBSS_SIZE )


; Execution starts here.
;	This is assembled as if it begins at FIRM_TEXT (where it will
;	eventually run).  But, it is loaded and first executed in flash.
;	This works because most 29k jumps are PC-relative.

	.eject

	.global	_main
	.use	code

	.equ	vectbl, .		; overwrite startup code in DRAM with
					; vector table.
_main:
	CK	(. % (4*4)) == 0 	; avoid cache-enable-jmp bug
cacheon:mtsrim	CFG, CFG_ILU
	mtsrim	CPS, PS_INTSOFF		; ints off!
	mtsrim	OPS, PS_INTSOFF
	sub	gr1,gr1,gr1		; clear stack pointer
	inv				; invalidate instr cache.
	nop
	const	gr64,255*4
	const	gr65,(255-65)-2
	CK	(.-cacheon) >= (2*4*4)  ;avoid cache-enable-jmp bug

	jmp	main2
	 nop
	nop
	nop

	; We need to reserve some fixed locations up here for
	; revision codes and other such stuff.

	; This must show up at offset 0x30 in EEPROM

vers:	.word	VERS

	CK	( vers == FIRM_TEXT+0x30)

rcsid:	.ascii	"$Revision: 1.23 $"
	.byte	0
	.align	16

	CK	(. % (4*4)) == 0 	; avoid cache-enable-jmp bug

	mtsrim	CFG, CFG_ILU
	mtsrim	CPS, PS_INTSOFF		; ints off!
	mtsrim	OPS, PS_INTSOFF
	inv				; invalidate instr cache.

	nop
	nop
	nop
	nop

	jmp	.		; kenp wants this handy
	nop

main2:
		; make sure DRAMs aren't contending on their serial buses
		; this also nukes board signature.
	
	constx	t0,(SRC_DRAM|OFFS_XFER)
	constx	t1,(DST_DRAM|OFFS_XFER)
	constx	t2,XFER_WTB
	constx	t3,XFER_RDA

	store	0,WORD,t2,t0
	store	0,WORD,t3,t0
	store	0,WORD,t2,t1
	store	0,WORD,t3,t1

	constx	t1,host_cmd+hc_sign	; nuke board signature early on
	store	0,WORD,zero,t1

clrregs:
	mtsr	IPA,gr64		; clear all regs
	const	gr0,0
	jmpfdec	gr65,clrregs
	 sub	gr64,gr64,4
	const	gr64,0
	const	gr65,0
		

	;-------- power-on self tests --------------------------

	constx	t0,SRC_DRAM
	constx	t1,DST_DRAM
	constx	t2,(DRAM_SIZE/4)-2
	constx	t3,0x01020304
	constx	t4,0x07070707
memtest1:
	store	0,WORD,t3,t0				; write all of memory
	add	t3,t3,t4				; with pattern
	store	0,WORD,t3,t1
	add	t3,t3,t4
	add	t0,t0,4
	jmpfdec	t2,memtest1
	 add	t1,t1,4

	constx	t0,(SRC_DRAM+OFFS_NPARITY)		; memory tests
	constx	t1,(DST_DRAM+OFFS_NPARITY)
	constx	t2,(DRAM_SIZE/4)-2
	constx	t3,MEM_ERR				; clr parity err reg
	store	0,WORD,t3,t3
	constx	t3,0x01020304
	constx	t4,0x07070707
memtest2:
	const	diagfail,DIAGFAIL_SRC
	load	0,WORD,v0,t0				; check src word
	cpeq	v0,v0,t3
	jmpf	v0,(fatal2-FIRM_TEXT+EEPROM)
	 add	t0,t0,4

	constx	v0,MEM_ERR				; check parity
	load	0,WORD,v0,v0
	and	v0,v0,15
	cpeq	v0,v0,0
	.if P0_DEBUG == 0				; 
	jmpf	v0,(fatal2-FIRM_TEXT+EEPROM)
	 nop
	.endif ; ! P0_DEBUG

	const	diagfail,DIAGFAIL_DST
	add	t3,t3,t4
	load	0,WORD,v0,t1				; check dest word
	cpeq	v0,v0,t3
	jmpf	v0,(fatal2-FIRM_TEXT+EEPROM)
	 add	t1,t1,4

	constx	v0,MEM_ERR				; check parity
	load	0,WORD,v0,v0
	and	v0,v0,15
	cpeq	v0,v0,0
	.if P0_DEBUG == 0
	jmpf	v0,(fatal2-FIRM_TEXT+EEPROM)
	 nop
	.endif ; ! P0_DEBUG

	jmpfdec	t2,memtest2
	 add	t3,t3,t4


	;-------------------------------------------------------


		; clear out RAM, initialize with bad parity

	constx	t0,WT_WRONG_PAR		; set up so wrong parity will be
	const	t1,0x0F			; written to all of DRAM
	store	0,WORD,t1,t0		; this traps uninitialized accesses.


	constx	t0,SDRAM_NPARITY	; initialize DRAMs
	constx	t1,DDRAM_NPARITY
	constx	t2,(DRAM_SIZE/4)-2
	constx	t3,0xbad00bad
clrmem:	store	0,WORD,t3,t0
	store	0,WORD,t3,t1
	add	t0,t0,4
	jmpfdec	t2,clrmem
	 add	t1,t1,4
	
		; set up important registers

	constx	p_regs_0ws,REGS_0WS
	constx	p_regs_1ws,REGS_1WS
	constx	p_regs_hs,REGS_HS
	constx	stat_p,host_stat_buf
	constx	bpstat_p,bp_stats
	
	HWREGP	p_dma_host_hi,DMA_HOST_HI
	HWREGP	p_dma_host_lo,DMA_HOST_LO
	HWREGP	p_dma_host_ctl,DMA_HOST_CTL
	HWREGP	p_int_enab,INT_ENAB

	constx	dst_dram_end,(DST_DRAM+DRAM_SIZE)
	constx	dst_dram_size,DST_BUF_SIZE


		; checksum and copy the firmware to DRAM

	const	v0,0			; accumulating checksum
	constx	t0,EEPROM
	constx	t1,FIRM_TEXT
	constx	t2,(FIRM_SIZE/4)-2
ckprom:	load	0,WORD,t3,t0
	add	t0,t0,4
	store	0,WORD,t3,t1
	add	t1,t1,4
	jmpfdec	t2,ckprom
	 add	v0,v0,t3
	
	inv				; invalidate instruction cache

		; v0 has results of checksum so
		; write sign/version now

	HWREGP	t1,BD_CTL
	store	0,WORD,zero,t1		; zero out BD_CTL before signature

	constx	t1,host_cmd+hc_vers	; put version in signature
	const	t0,VERS
	store	0,WORD,t0,t1

	.if 0 ;	XXX: check firmware checksum!!
	cpeq	t0,v0,0			; is firmware bad???
	jmpt	t0,ckprom5
	 nop
	
	jmp	fatal2			; fail on bad firmware
	 const	diagfail, DIAGFAIL_EEPROM

ckprom5:
	.endif


;; initialize bypass registers and memory 

ramclear:
; clear bypass internal registers
	mov	bp_cur_sjob, zero
	mov	dst_bp_port, zero
	mov	dst_bp_dq_tail_lo, zero
	mov	dst_bp_dq_tail_hi, zero
	mov	dst_bp_port, zero
	mov	src_wdog_bp_pkts, zero
	constx	src_wdog_old_bp_pkts, -1

	constx	bp_job_structs, bp_job_state
	constx	bp_port_structs, bp_port_state

	;; clear all bypass memory structures
	constx  v0, bp_structs_start
	constx  v2, ((bp_structs_end-bp_structs_start)/4 - 1)
clr_bp_structs:	
	store	0,WORD, zero, v0
	jmpfdec v2, clr_bp_structs
	 add   v0, v0, 4

	;; make all jobs invalid
	mov	bp_state, zero
	;; make all port's invalid

	mov	t0, bp_port_structs
	constx	t1, BP_MAX_PORTS-2

clr_bp_port_struct:	
	store	0,WORD,zero, t0
	jmpfdec	t1, clr_bp_port_struct
	 add	t0, t0, BP_PORT_SIZE

	;; clear descriptor queue to invalid entries
	constx	t0, bp_sdqueue	
	constx	t1, (BP_SDQ_SIZE * BP_MAX_JOBS / 4) - 2
	constx	t2, HIP_BP_DQ_INV
	
clr_dq:	
	store	0,WORD, t2, t0
	jmpfdec t1, clr_dq
	 add   t0, t0, 4

set_bp_config_str:
        ; initialize dma_status to "dma is inactive"
        constx  bp_config, bp_config_region
        add     t0, bp_config, dma_status
	constx	t1, DMA_INACTIVE
        store   0, WORD, t1, t0

        constx  v0, BP_MAX_JOBS
        constx  v1, BP_MAX_PORTS
        constx  v2, bp_hostx-host_cmd
        constx  v3, BP_MAX_HOSTX *4
        constx  v4, bp_dfreelist-host_cmd
	constx	v5, BP_DFL_SIZE
        constx  v6, bp_sfreemap-host_cmd
        constx 	v7, BP_SFM_SIZE
        constx  v8, bp_dfreemap-host_cmd
	constx	v9, BP_DFM_SIZE
        constx  v10, bp_stats-host_cmd
	constx  v11, BP_STATS_BUF*4
        constx  v12, bp_sdqueue-host_cmd
        constx 	v13, BP_SDQ_SIZE
        constx  v14, bp_job_state-host_cmd
        constx  v15, BP_JOB_SIZE 

        ;; store the values into the "result" region for driver to read
        mtsrim  CR, 16-1
        storem  0, WORD, v0, bp_config

;; end of "initialize bypass registers and memory"


	constx	t1,host_cmd+hc_sign	; put signature in response
	constx	t2,HIP_SIGN
	store	0,WORD,t2,t1

		; set up interrupt vectors
	
	constx	t2,NUM_V-2
	constx	t0,badint
	constx	v0,vectbl
	mov	t1,v0
setiv:	store	0,WORD,t0,t1		; put badint into all trap handlers
	jmpfdec	t2,setiv
	 add	t1,t1,4
	
	mtsrim	CR, LEN_VEC-1		; copy vectbl from EEPROM into DRAM
	const	t1,(intvecs-FIRM_TEXT+EEPROM)
	loadm	0,WORD,v1,t1
	mtsrim	CR, LEN_VEC-1
	storem	0,WORD,v1,v0

	mtsr	VAB, v0

		; Initialize ATE wrap-around points

	constx	t0,DST_LOWADDR
	constx	t1,THDST_START
	store	0,WORD,t0,t1			; set up dst DRAM wrap-around
	constx	t0,SRC_LOWADDR
	constx	t1,FHSRC_START
	store	0,WORD,t0,t1			; set up src DRAM wrap-around

		; Initialize SMI mode on the serial port Bs.
	constx	t0,SRC_DRAM+OFFS_XFER
	constx	t1,XFER_CLR_BMR
	store	0,WORD,t1,t0
	constx	t0,DST_DRAM+OFFS_XFER
	store	0,WORD,t1,t0

		; Initialize Interrupt Mask
	constx	inte_shadow,(INT_TH_PAR|INT_ATE_OVRRUN|INT_SRC_STOP_EXE|INT_DEBUG)
	store	0,WORD,inte_shadow,p_int_enab

		; Initialize 29K timer
	
	constx	t0,TIME_INTVL
	mtsr	TMC,t0
	CK	TMR_IE > 0xffff
	consth	t0,TIME_INTVL | TMR_IE
	mtsr	TMR,t0

		; Initialize sw

	constx	state, ST_SLEEP
	const	state2, 0
	const	host_o_cmdid,0
	constx	src_wdog_tval,SRC_WDOG_INIT
	constx	src_wdog_blktag,0x0000007F
	constx	dst_wdog_tval,DST_WDOG_INIT

	const	t0,(HST_STATS_SIZE/4)-2			; zero out statistics
	constx	t1,host_stat_buf
init_sw2:
	store	0,WORD,zero,t1
	jmpfdec	t0,init_sw2
	 add	t1,t1,4
	
	const	t0,(HST_BPSTATS_SIZE/4)-2			; zero out bypass statistics
	constx	t1,bp_stats

init_sw2a:
	store	0,WORD,zero,t1
	jmpfdec	t0,init_sw2a
	 add	t1,t1,4
	
		; Initialize hardware

		; SOURCE/DESTINATION initialization

		; leaves destination rejecting connections and
		; source in WAIT mode

	.equ	DST_OP_ARMS,(DST_CTL_I_FIELD|DST_CTL_SDIC_LOST|DST_CTL_EONC|DST_CTL_EOP|DST_CTL_THRESH)
	.equ	DST_OP_EN,(DST_CTL_IDATA|DST_CTL_FF|DST_CTL_SEQERR)
	.equ	DST_OP_AUTODISC,(DST_CTL_DIS_ILLBURST|DST_CTL_DIS_SYNC|DST_CTL_DIS_LLRC|DST_CTL_DIS_PARITY|DST_CTL_DIS_SEQ_ERR|DST_CTL_DIS_SDIC_LOST)
	.equ	DST_OPERATIONAL, ( DST_OP_ARMS | DST_OP_EN | DST_OP_AUTODISC )

	HWREGP	t1,SRC_MODE_SEL
	constx	t0,SRC_MODE_RST
	store	0,WORD,t0,t1			; reset s2020

	HWREGP	t2,SRC_MODE_SEL
	constx	t0,SRC_MODE_WAIT		; put s2020 in WAIT mode
	store	0,WORD,t0,t2

	HWREGP	t1,DST_MODE_SEL			; s2021 run mode
	constx	t0,DST_MODE_RUN
	store	0,WORD,t0,t1

	constx	t0,0
	HWREGP	t1,DST_CONNECT
	store	0,WORD,t0,t1			; reject connections

	constx	t0,0				; stay quiet for now
	HWREGP	t1,DST_CONTROL
	store	0,WORD,t0,t1

	HWREGP	t1,DST_THRESH			; set destination threshold
	constx	t0,DST_OP_THRESHOLD
	store	0,WORD,t0,t1

	HWREGP	t1,DST_WIE_EN
	HWREGP	t2,DST_WOKI_FLAGS
	store	0,WORD,zero,t1			; reset WIE_EN bit
	constx	t0,(128<<2)
	store	0,WORD,t0,t2			; set PAE (don't really care)
	constx	t0,(DST_WOKI_PAF<<2)
	store	0,WORD,t0,t2			; set PAF in dest woki fifo
	const	t0,1
	store	0,WORD,t0,t1			; set WIE_EN bit again

	HWREGP	t1,SRC_WOKI_SHIFT		; clear source WOKI FIFO
	HWREGP	t2,SRC_RT
clrsfifo:
	store	0,WORD,t1,t1
	load	0,WORD,t0,t2
	tbit	t0,t0,SRC_RT_WOKI_NE
	jmpt	t0,clrsfifo
	 nop

	 HWREGP	t1,DST_WOKI_LO			; clear dest WOKI FIFO
	HWREGP	t2,INT_33
clrdfifo:
	load	0,WORD,t0,t1
	load	0,WORD,t0,t2
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpt	t0,clrdfifo
	 nop

	constx	t0,XFER_MWTB			; xfer cycle for status block
	constx	t1,(DST_LOWADDR-TPDRAM_BLKSIZE+OFFS_XFER)
	store	0,WORD,t0,t1

	HWREGP	t1,DST_STAT_BLOCK		; do status block command
	store	0,WORD,t1,t1			; this cleans things up

	HWREGP	t1,INT_33			; spin waiting for stat block
waitstat1:
	load	0,WORD,t0,t1
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpf	t0,waitstat1
	 nop
	
	HWREGP	t1,DST_WOKI_LO			; pull that woki
	load	0,WORD,t0,t1

	.if DEBUG				; ASSERT( dst FIFO empty );
	HWREGP	t1,INT_33
	load	0,WORD,t0,t1
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpf	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	.if DEBUG
	constx	t0,XFER_MWTB			; in case you want to see this
	constx	t1,(DST_LOWADDR+OFFS_XFER)	; block (also zeros tap)
	store	0,WORD,t0,t1
	.endif ; DEBUG

	HWREGP	t1,DST_RT_CLR
	load	0,WORD,t0,t1			; clear DST RT register
	tbit	t0,t0,DST_RT_SRCAV		; do we have dst SDIC?
	jmpt	t0,dstinit2
	 nop
	INCSTAT	hst_d_sdic_lost
	jmp	dstinit3
	 or	state2,state2,ST2_DST_DRAIN
dstinit2:
	call	raddr,dst_reset_rdy
	 nop
dstinit3:
	HWREGP	t1,SRC_RT			; do we have src DSIC?
	load	0,WORD,t0,t1
	tbit	t0,t0,SRC_RT_STOP_DSIC
	jmpt	t0,reset_hippi1
	 or	state2,state2,ST2_SRC_NODSIC

	HWREGP	t1,SRC_WOKI_WR			; get SCZ bit set!
	constx	t0,SWOKI_FLUSH
	store	0,WORD,t0,t1
	
	 HWREGP	t1,SRC_CONTROL
	constx	t0,(SRC_CTL_SHIFT|SRC_CTL_GO)	; init source machine
	store	0,WORD,t0,t1

	andn	state2,state2,ST2_SRC_NODSIC	; put s2020 in RUN mode
	HWREGP	t2,SRC_MODE_SEL
	constx	t0,SRC_MODE_RUN
	store	0,WORD,t0,t2
reset_hippi1:

		; spin waiting for first HCMD from host
		; (flash yellow and green LEDs)

	HWREGP	t1,BD_CTL
	HWREGP	t2,INT_33
startwt:
	add	t3,t3,1			; flash yellow LED
	srl	t0,t3,(18-INT33_LED_YELLOW_BT)
	and	t0,t0,(INT33_LED_YELLOW|INT33_LED_GREEN)
	or	t0,t0,INT33_DQ_MASK_DAT
	store	0,WORD,t0,t2

	load	0,WORD,v1,t1
	tbit	t0,v1,BDCTL_29K_INT
	jmpf	t0,startwt
	 nop
	
	const	t0,INT33_DQ_MASK_DAT		; turn off green/yellow
	store	0,WORD,t0,t2

	constx	t1, host_cmd+hc_cmd
	load	0,WORD,v0,t1			; get command
	cpeq	t1, v0, HCMD_INIT
	jmpf	t1, startwt2
	 nop

		; Interrupts on.  I've put code between
		; the setting of the mask and this to allow the
		; interrupt signals some time to propagate through
		; the 29K's synchronizers.  XXX

	mtsrim	CPS,PS_INTSON

		; jump to execution in RAM

	constx	t0, mlop0	; was constx  t0, ramstart
	jmpi	t0
	 nop

startwt2:
	HWREGP	t1,BD_CTL
	store	0,WORD,zero,t1		; zero out BD_CTL

	cpeq	t1, v0, HCMD_EXEC
	jmpt	t1, startwt3
	 nop
	
		; EXEC commands are called with interrupts off.
		; They can do a return (jmpi raddr) if they want
		; to jump back into the loop.

	call	raddr,fatal		; first cmd must be INIT or EXEC
	 nop

startwt3:
		; jmp to execution address given in HCMD_EXEC

	constx	t1, host_cmd+hc_cmd_exec
	load	0,WORD,v0,t1

	calli	raddr,v0		; in case it wants to return
	 nop
	
	HWREGP	t1,BD_CTL
	jmp	startwt
	 nop


;	this is here because we apparantly don't have enough initialization
;	code to overwrite with the vector table.  probably have to copy
;	firmware ahead of the table or do something else.

	.block	512

; interrupt vectors:
intvecs:
	.word	badint		; 00 illegal opcode
	.word	badint		; 01 unaligned access
	.word	badint		; 02 out of range
	.word	badint		; 03 coprocessor not present
	.word	badint		; 04 coprocessor exception
	.word	badint		; 05 protection violation
	.word	buserr		; 06 instruction access violation
	.word	buserr		; 07 data access violation
	.word	badint		; 08 user-mode instr tlb miss
	.word	badint		; 09 user-mode data tlb miss
	.word	badint		; 10 super-mode instr tlb miss
	.word	badint		; 11 super-mode data tlb miss
	.word	badint		; 12 instruction tlb protection violation
	.word	badint		; 13 data tlb protection violation
	.word	timerint	; 14 29kinternal timer
	.word	badint		; 15 trace
	.word	src_stop_int	; 16 intr0: Source stop bit execute
	.word	dma_dn_int	; 17 intr1: DMA complete/cmd-buffer ready
	.word	badint		; 18 intr2: Dest WOKI not empty
	.word	badint		; 19 intr3: Host interrupt
	.word	trap0		; 20 trap0: debug trap
	.word	trap1		; 21 trap1: error conditions

	.equ	LEN_VEC, (. - intvecs)/4

; the rest of the vector table is filled with "badint", the catch-all.

; make sure this wasn't overwritten by trap table
	CK (NUM_V*4 + FIRM_TEXT) < . 

; command table for host commands

cmd_tbl:.word	hcmd_done		;00	NOP
	.word	hcmd_init		;01	set parameters
	.word	hcmd_exec		;02	execute downloaded code
	.word	hcmd_params		;03	set op flags and params
	.word	hcmd_wakeup		;04	wake up!
	.word	hcmd_asgn_ulp		;05	assign ulp
	.word	hcmd_dsgn_ulp		;06	deassign ulp
	.word	hcmd_stat		;07	update status flags
	.word	hcmd_job		;08	update bypass virtual channel state
	.word	hcmd_port		;09	update bypass process port state
	.word	hcmd_bp_config		;10    	update bypass config
	.word	hcmd_bp_int_ack		;11	dest bypass interrupt ack
cmd_tbl_end:


; command table for c2b's
c2b_cmd_tbl:
	.word	proc_c2b_empty
	.word	proc_c2b_sml
	.word	proc_c2b_big
	.word	proc_c2b_wrap
	.word	proc_c2b_read
c2b_cmd_tbl_end:

; execution in DRAM starts here.  We must be running out of DRAM
; before we can do DMAs.
;
	



;-------------------------------------------
;     MAIN Loop
;-------------------------------------------

mlop0:

;-------------------------------------------
;	Handle host interrupt commands
;-------------------------------------------

; check for host-to-board command:

	constx	t1,host_cmd+hc_cmd_id
	load	0,WORD,v1,t1			; get command ID

	; a new cmd id tells us we have work to do

	cpeq	t0,host_o_cmdid,v1		; same ID?
	jmpt	t0,mlop1			; no new command if same...
	 sub	t1,t1,(hc_cmd_id-hc_cmd)

	load	0,WORD,v0,t1			; get command

	mov	host_o_cmdid,v1			; save old ID

		; act on host command
	
	cpltu	t0,v0,(cmd_tbl_end-cmd_tbl)/4	; valid command???
	jmpf	t0,fatal
	 constx	t0,cmd_tbl

	sll	t1,v0,2
	add	t0,t1,t0
	load	0,WORD,t2,t0			; get command address

	jmpi	t2				; do command!
	 nop



hcmd_job:

	; initialize a job slot for the bypass
	; The job structure is initialized and the bit is enabled in bp_state
	; The source descriptor queue and destination freelist are initialized
	; with invalid entries.

	;hcmd_job assumes the data field contains a struct with:
	;	  __uint32_t  enable;	        /* enable/disable job  */
	;			enable  = HCMD_JOB_ENABLE
	;			disable = (!= HCMD_JOB_ENABLE)
	;	  __uint32_t  job;		/* job slot number */
	;	  __uint32_t  fm_entry_size;	/* size of a data page, in bytes*/
	;	  __uint32_t  auth[3];	        /* authentication number for job */
        ;         __uint32_t  ack_hostx;        /* ack hostx to insert into outgoing msg */
        ;         __uint32_t  ack_port;       /* ack port to insert into outgoing msg */
        ;

	constx	t0,host_cmd+hc_cmd_data
	mtsrim	CR,8-1
	loadm	0,WORD,v0,t0

	cpeq	t0, v0, HCMD_JOB_ENABLE
	jmpf    t0, hcmd_job_disable
	 mov	bp_cj_job, v1
	mov	bp_cj_bufx_entry_size, v2
	mov     bp_cj_auth0, v3
	mov     bp_cj_auth1, v4
	mov     bp_cj_auth2, v5

	;; shift the ack_hostx into 31:24
	sll	v6, v6, BP_ACK_HOST_SHIFT
	;; or with the ack_port, which should be in 15:0
	;; make sure that cruft in 31:16 is cleared out
	constx	t0,    BP_ACK_PORT_MASK
	and	v7, v7, t0
	or      bp_cj_ack_hostport, v6, v7

	; shift job bit into position
	constx  v0, 0x80000000
	srl	v0, v0, bp_cj_job
	or      bp_state, bp_state, v0


	;;  initialize sdq state
	;; bp_cj_sdq_base = bp_sdqueue + (2^BP_SDQ_SIZE_POW2*bp_cj_job)
	;; source desc page size = BP_SDQ_SIZE_POW2 (BP_SDQ_SIZE)

	constx	bp_cj_sdq_base, bp_sdqueue	
	sll	t0, bp_cj_job, BP_SDQ_SIZE_POW2
	add	bp_cj_sdq_base, bp_cj_sdq_base, t0

	;; point head at last word of first descriptor
	add	bp_cj_sdq_head, bp_cj_sdq_base, BP_DESC_SIZE
	sub	bp_cj_sdq_head, bp_cj_sdq_head, 4
	
	;; bp_cj_sdq_end = bp_cj_sdq_base + BP_SDQ_SIZE
	constx	t0, BP_SDQ_SIZE
	add	bp_cj_sdq_end, bp_cj_sdq_base, t0
	
	;; set up the bp_slot_table for this job to -1's
	sll	t1, bp_cj_job, BP_SLOT_TABLE_SIZE_POW2
	constx	t0, bp_slot_table
	add, 	t0, t0, t1

	constx	t1, ((BP_MAX_SLOTS*BP_SLOT_TABLE_ENTRY_SIZE)/4) - 2
	constx	t2, -1

hcmd_setup_slot_tab:
	store	0, WORD, t2, t0		
	jmpfdec t1, hcmd_setup_slot_tab
	  add	t0, t0, 4		

	;; store job state to memory
	;; [bp_job_structs + bp_cj_job*2^BP_JOB_SIZE_POW2] <- new structure
	sll	t1, bp_cj_job, BP_JOB_SIZE_POW2
	add	t0, t1, bp_job_structs
;	mtsrim	CR, BP_TOTAL_JOB_REGS-1			
;	storem	0, WORD, %%(bp_job_reg_base), t0

	store	0, WORD, bp_cj_sdq_head, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_sdq_base, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_sdq_end, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_ack_hostport, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_bufx_entry_size, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_auth0, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_auth1, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_auth2, t0
	add	t0, t0, 4
	store	0, WORD, bp_cj_job, t0
	
	jmp     hcmd_done
	  nop

hcmd_job_disable:
	;; disable the job
	constx  t0, 0x80000000
	srl	t0, t0, bp_cj_job
	andn	bp_state, bp_state, t0

	;; clear destination freelist to invalid entries
	sll	t1, bp_cj_job, BP_DFL_SIZE_POW2
	constx	t0, bp_dfreelist
	add	t0, t0, t1
	
	;; clear descriptor queue to invalid entries
	constx	t0, bp_sdqueue	
	sll	t1, bp_cj_job, BP_SDQ_SIZE_POW2
	add	t0, t0, t1

	constx	t1, (BP_SDQ_SIZE/4) - 2
	constx	t2, HIP_BP_DQ_INV
	
hcmd_job_clr_dq:	
	store	0,WORD, t2, t0
	jmpfdec t1, hcmd_job_clr_dq
	  add   t0, t0, 4
	
	jmp     hcmd_done
	  nop

hcmd_port:

	;; configure a bypass port slot
	;;  assumes a data structure in the hcmd_data field of the form:
	;struct {
	;	 struct {
	;;	     __uint32_t opcode      :  4
	;;#define   HIP_BP_PORT_DISABLE       0
	;;#define   HIP_BP_PORT_ENABLE         1
	;	     __uint32_t unused      : 12
	;	     __uint32_t init_bufx    : 16
	;	 }  s;
	;	__uint32_t  job;		/* job this port is attached to */
	;	__uint32_t  port;		/* bypass destination buffer index */
	; 	__uint32_t  ddq_hi;	        /* high half of phys addr for dest descq */
	;	__uint32_t  ddq_lo;	        /* low half of phys addr for dest descq */
	;	__uint32_t  ddq_size;		/* size of dest descriptor page, in bytes*/
	;; }

		
	mtsrim	CR,7-1				; copy 7 words of cmd_data
	constx	t0,host_cmd+hc_cmd_data
	loadm	0,WORD,v0,t0
	
	mov	bp_cp_job, v1
	srl	t0, v0, BP_PORT_OPCODE_SHIFT	; shift opcode into place

	cpeq	t1, t0, HIP_BP_PORT_DISABLE	; is disable command?
	jmpt    t1, hcmd_port_disable
	  mov     bp_cp_port, v2


	cpeq	t1, t0, HIP_BP_PORT_ENABLE	
	constx	bp_cp_state, BP_PORT_VAL
	mov	bp_cp_d_data_tail, zero
	mov	bp_cp_data_bufx, zero

	mov	bp_cp_dq_tail, zero
	mov	bp_cp_dq_base_hi, v3
	mov	bp_cp_dq_base_lo, v4
	mov	bp_cp_dq_size, v5

	;; not actually port of port structure
	mov	bp_cp_d_data_base_hi, zero
	mov	bp_cp_d_data_base_lo, zero

	;; init interrupt counts
	mov	bp_cp_int_cnt, zero

	;; store results to memory
	;; [bp_port_structs + bp_cp_port*2^BP_PORT_SIZE_POW2] <- new structure
	sll	t1, bp_cp_port, BP_PORT_SIZE_POW2

	add	t0, bp_port_structs, t1
;	mtsrim	CR, BP_TOTAL_PORT_REGS-1			
;	storem	0, WORD, %%(bp_port_reg_base), t0

	store	0, WORD, bp_cp_state, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_job, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_d_data_tail, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_data_bufx , t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_dq_base_lo, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_dq_base_hi, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_dq_size, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_dq_tail, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_int_cnt, t0
	add	t0, t0, 4
	store	0, WORD, bp_cp_port, t0
	
	jmp     hcmd_done
	  nop

	
hcmd_port_disable:
	;; disable the port
	;; get port state
	sll	t1, bp_cp_port, BP_PORT_SIZE_POW2
	add	t2, bp_port_structs, t1
	add	t3, t2, (bp_cp_state-bp_port_reg_base)*4
	load	0, WORD, bp_cp_state, t3

	;; disable port & store
	andn	bp_cp_state, bp_cp_state, BP_PORT_VAL
	store	0, WORD, bp_cp_state, t3

	jmp     hcmd_done
	  nop	

hcmd_bp_config:
	;; configure the bypass global parameters
	;; assumes a data structure of the form:
	;;  struct {
    	;;	__uint32_t  ulp;            /* enable/disable job  */
  	;;	} bp_init;

	;; this command trashes ALL jobs currently running
	constx	t0,host_cmd+hc_cmd_data
	load	0,WORD,bp_state,t0
	constx	t0, 0x0000ff	
	and	bp_state, bp_state, t0
	jmp     hcmd_done
	  nop


hcmd_bp_int_ack:
	;; interupt ack for destination port interrupt
	;; expects buffer to contain:	
	;;  struct { 
	;;	__uint32_t  portid;         /* portid interrupt to ack */
    	;;	__uint32_t  cookie;         /* interrupt cookie from FW */
  	;;	} bp_portint_ack;


	mtsrim	CR,2-1				; copy 2 words of cmd_data
	constx	t0,host_cmd+hc_cmd_data
	loadm	0,WORD,v0,t0

	mov	bp_cp_port, v0

	;; retreive current int cnt and post another int if needed.
	sll	t1, bp_cp_port, BP_PORT_SIZE_POW2
	add	t0, bp_port_structs, t1
	add	t0, t0, (bp_cp_int_cnt-bp_port_reg_base)*4
	load	0, WORD,bp_cp_int_cnt, t0
	cpeq	t1, v1, bp_cp_int_cnt

	jmpt	t1, hcmd_bp_portint_past_new_ints
	  andn	bp_cp_state, bp_cp_state, BP_PORT_INT_PENDING

	;; send an interrupt for unprocessed received int descriptors
	or	bp_cp_state, bp_cp_state, BP_PORT_INT_PENDING
	constx	v0, B2H_PORTINT
	or	v0, v0, bp_cp_port
	mov	v1, bp_cp_int_cnt
	call	raddr,queue_b2h
	 or	state,state,(ST_B2H_PUSH|ST_HINT)
	
hcmd_bp_portint_past_new_ints:					
	jmp     hcmd_done
	  nop
	


	;; hcmd_init assumes the following structure in the args:
	;;   struct {                      /* initialization parameters */
    	;;	__uint32_t  b2h_buf_hi;
    	;;	__uint32_t  b2h_buf;        /* B2H buffer */
    	;;	__uint32_t  b2h_len;        /* B2H_LEN */
    	;;	__uint32_t  iflags;         /* initialization flags (see above) */
    	;;	__uint32_t  d2b_buf_hi;
    	;;	__uint32_t  d2b_buf;
    	;;	__uint32_t  d2b_len;        /* D2B_LEN (# of d2b's in ring) */
    	;;	__uint32_t  host_nbpp_mlen; /* Put (NBPP|MLEN) here */
    	;;	__uint32_t  c2b_buf_hi;
    	;;	__uint32_t  c2b_buf;
    	;;	__uint32_t  stat_buf_hi;
    	;;	__uint32_t  stat_buf;       /* HCMD_FET_STATS */
  	;;	} init;

hcmd_init:
		; Save all these important addresses and values

	constx	t0,host_cmd+hc_cmd_data
	constx	t1,hostp_b2h
	constx	t2, 12-2

hcmd_init_load_struct:	
	load	0,WORD,v0,t0	; using single load/stores because of store hw bug
	store	0,WORD,v0,t1
	add	t0, t0, 4
	jmpfdec	t2, hcmd_init_load_struct
	 add	t1, t1, 4
	
   .ifdef FEW_VS
      .print "** Error: I'm assuming lots of v registers"
      .err
   .endif

;	Initialize Software!

		; Init b2h stuff.

	const	b2h_queued,0
	const	b2h_h_offs,0
	const	b2h_sn,1
	constx	t0,b2h_len
	load	0,WORD,b2h_h_lim,t0
	sll	b2h_h_lim,b2h_h_lim,3

		; Initialize d2b stuff.

	constx	d2b_nxt,d2b
	mov	d2b_valid,d2b_nxt
	mov	d2b_send,d2b_nxt
	mov	d2b_out,d2b_nxt
	constx	t0,(D2B_RDY|D2B_BAD)
	store	0,WORD,t0,d2b_nxt		; initialize with "un-ready" hd
	constx	t0,d2b_len
	load	0,WORD,t0,t0			; get d2b_len
	sll	t0,t0,3				; bytes
	constx	t1,D2B_B_SIZE
	cpgt	t1,t0,t1
	jmpt	t1,fatal			; we can't fit d2b on-board
	 add	d2b_lim,t0,d2b_nxt		; compute d2b_lim
	
	;; set host page size (nbpp) by loading value from memory, and masking off
	;; bottom 12 bits (4 KB boundary). also set mbuf length
	constx	src_d2b_blksz,SRC_D2B_BLK
	constx	t0,host_nbpp_mlen
	load	0,WORD,t1,t0
	constx	t0,0xfff
	and	mlen,t1,t0
	andn	nbpp,t1,t0

	mov	t1,nbpp

	;; set src_d2b_blksz in units of page size
hcmd_init1:
	srl	t1,t1,1
	sll	t0,t1,31
	jmpf	t0,hcmd_init1
	 srl	src_d2b_blksz,src_d2b_blksz,1

		; Initialize c2b stuff
	
	const	c2b_hnxt,0
	const	c2b_avail,0
	

		; Initialize states

	constx	sdma_state,SDMAST_IDLE
	constx	ddma_state,DDMAST_IDLE
	const	dst_dma_flags,DWOKI_EOP
	constx	dma_client,DMA_CLIENT_NONE
	constx	dfifo_state,DFIFOST_NONE
	constx	state,ST_SLEEP

		; Initialize source

	constx	src_in,SRC_LOWADDR
	mov	src_send,src_in
	mov	src_dmaed,src_in
	const	src_tag_in,0
	const	last_swoki,0
	constx	t0,(SRC_LOWADDR+OFFS_XFER)
	constx	t1,XFER_RDA
	store	0,WORD,t1,t0

	; SRC_LOWADDR must start on a low half block boundary for this
	; code to work...otherwise use split read.
	CK ( (SRC_LOWADDR & BLKADDR_QSF) == 0 )

		; Initialize destination
	
	constx	ifhip_sml_in,ifhip_sml		; initialize pool of bufs
	mov	ifhip_sml_out,ifhip_sml_in
	constx	ifhip_big_in,ifhip_big
	mov	ifhip_big_out,ifhip_big_in
	constx	dst_in,DST_LOWADDR
	mov	dst_out,dst_in
	const	dfifo_state,DFIFOST_NONE
	const	dst_dma_rdlst_len,0

	; DST_LOWADDR must start on a low half block boundary for this
	; code to work...otherwise use split read.
	CK ( (DST_LOWADDR & BLKADDR_QSF) == 0 )


	.if DEBUG
	HWREGP	t0,INT_33			; ASSERT( dst woki fifo empty )
	load	0,WORD,t0,t0
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpf	t0,.+16
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	HWREGP	t1,DST_RT_CLR
	load	0,WORD,t0,t1
	and	t0,t0,DST_RT_SDIC_LOST
	.if ST2_DST_DRAIN_BT > DST_RT_SDIC_LOST_BT
	sll	t0,t0,(ST2_DST_DRAIN_BT - DST_RT_SDIC_LOST_BT)
	.endif
	.if ST2_DST_DRAIN_BT < DST_RT_SDIC_LOST_BT
	srl	t0,t0,(DST_RT_SDIC_LOST_BT - ST2_DST_DRAIN_BT)
	.endif
	or	state2,state2,t0

	constx	t0,XFER_MWTB			; xfer cycle for status block
	constx	t1,(DST_LOWADDR-TPDRAM_BLKSIZE+OFFS_XFER)
	store	0,WORD,t0,t1

	HWREGP	t1,DST_STAT_BLOCK		; do status block command
	store	0,WORD,t1,t1			; to clean up dst woki pipe

	HWREGP	t1,INT_33			; spin waiting for stat block
waitstat2:
	load	0,WORD,t0,t1
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpf	t0,waitstat2
	 nop
	
	HWREGP	t1,DST_WOKI_LO			; pull that woki
	load	0,WORD,t0,t1

	constx	t0,XFER_MWTB			; reset again dst xfer cycle
	constx	t1,(DST_LOWADDR+OFFS_XFER-TPDRAM_BLKSIZE)
	store	0,WORD,t0,t1

	call	raddr,dst_reset_rdy
	 nop

	constx	t0,DST_OPERATIONAL		; enable destination WOKIs
	HWREGP	t1,DST_CONTROL
	store	0,WORD,t0,t1

		; Initialize stack associations

	const	t0,256-2
	constx	t1,ulptostk
	constx	t2,0xFF
init_sw0:
	store	0,WORD,t2,t1
	jmpfdec	t0,init_sw0
	 add	t1,t1,4

	const	t0,15-2
	constx	t1,stktoulp
	constx	t2,c2b_read_tbl
init_sw1:
	store	0,WORD,zero,t1			; zero out stktoulp table
	store	0,WORD,zero,t2			; zero out c2b_read_tbl[0..15]
	add	t2,t2,4
	store	0,WORD,zero,t2
	add	t2,t2,4
	jmpfdec	t0,init_sw1
	 add	t1,t1,4
	
	constx	t1,host_cmd+hc_sign		; erase signature now
	store	0,WORD,zero,t1

	constx	t1,iflags			; now take care of flags
	jmp	hcmd_params0
	 load	0,WORD,t1,t1



hcmd_asgn_ulp:
	constx	t1,host_cmd+hc_cmd_data	
	load	0,WORD,t1,t1
	srl	t2,t1,16			; ulp_id
	and	t1,t1,0x0F			; stk

	constx	t0,(ulptostk/4)
	add	t0,t0,t2
	sll	t0,t0,2	
	store	0,WORD,t1,t0			; ulptostk[ulp]=stk

	constx	t0,(stktoulp/4)
	add	t0,t0,t1
	sll	t0,t0,2
	store	0,WORD,t2,t0			; stktoulp[stk]=ulp

	cpeq	t0,t1,HIP_STACK_RAW
	srl	t0,t0,31-ST2_HIPPI_PH_BT
	or	state2,state2,t0

	jmp	hcmd_done
	 nop


hcmd_dsgn_ulp:
	constx	t1,host_cmd+hc_cmd_data
	load	0,WORD,t1,t1
	srl	t2,t1,16			; ulp_id
	and	t1,t1,0x0F			; stk

	constx	t0,(ulptostk/4)
	const	t3,0xFF
	add	t0,t0,t2
	sll	t0,t0,2
	store	0,WORD,t3,t0			; ulptostk[ulp]=0xFF

	constx	t0,(stktoulp/4)
	add	t0,t0,t1
	sll	t0,t0,2
	store	0,WORD,zero,t0			; stktoulp[stk]=0

	cpeq	t0,t1,HIP_STACK_RAW
	srl	t0,t0,31-ST2_HIPPI_PH_BT
	andn	state2,state2,t0

	jmp	hcmd_done
	 nop



	;; hcmd_params assumes the following structure as argument:
	;; #define HIP_FLAG_ACCEPT         0x01    /* accepting connections */
	;; #define HIP_FLAG_IF_UP          0x02    /* accept HIPPI-LE network traffic */
	;; #define HIP_FLAG_NODISC         0x04    /* no disconnect on parity/LLRC err */
	;;
	;;	  struct {
        ;;		__uint32_t  flags;  /* operational flags */
    	;;		int stimeo;         /* how long until source times out */
    	;;		int dtimeo;         /* how long until dest times out */
  	;;		} params;

hcmd_params:
	constx	t0,host_cmd+hc_cmd_data	
	load	0,WORD,t1,t0			; get new flags
	add	t0,t0,4
	load	0,WORD,src_wdog_tval,t0		; get new src timeo
	add	t0,t0,4
	load	0,WORD,dst_wdog_tval,t0		; get new dst timeo

hcmd_params0:
	tbit	t0,t1,HIP_FLAG_ACCEPT
	jmpf	t0,hcmd_params_off
	 nop

	tbit	t0,op_flags,OPF_ACCEPT
	jmpt	t0,hcmd_params1			; don't do if already on!
	 nop

	or	op_flags,op_flags,OPF_ACCEPT
	
	constx	t0,DST_CONN_ACCEPT
	HWREGP	t2,DST_CONNECT
	store	0,WORD,t0,t2			; accept connections

	jmp	hcmd_params1

hcmd_params_off:
		; Turn board off.

	 tbit	t0,op_flags,OPF_ACCEPT
	jmpf	t0,hcmd_params1			; don't do if already off!
	 nop

	andn	op_flags,op_flags,OPF_ACCEPT
	
	constx	t0,0
	HWREGP	t2,DST_CONNECT
	store	0,WORD,t0,t2			; reject connections

hcmd_params1:

	constx	t0,OPF_ENB_LE			; set/reset HIPPI-LE flag
	andn	op_flags,op_flags,t0
	srl	t0,t1,HIP_FLAG_IF_UP_BT
	and	t0,t0,1
	.if OPF_ENB_LE_BT != 0
	sll	t0,t0,OPF_ENB_LE_BT
	.endif
	or	op_flags,op_flags,t0

	tbit	t0,t0,OPF_ENB_LE
	jmpt	t0,hcmd_params2
	 nop
	
	constx	ifhip_sml_in,ifhip_sml		; clearing flag? throw out bufs
	mov	ifhip_sml_out,ifhip_sml_in
	constx	ifhip_big_in,ifhip_big
	mov	ifhip_big_out,ifhip_big_in

hcmd_params2:
	constx	t2,DST_OPERATIONAL
	tbit	t0,t1,HIP_FLAG_NODISC
	jmpf	t0,hcmd_params2a
	 andn	op_flags,op_flags,OPF_NODISC
	
	or	op_flags,op_flags,OPF_NODISC
	constx	t0,(DST_CTL_DIS_LLRC|DST_CTL_DIS_PARITY)
	andn	t2,t2,t0

hcmd_params2a:
	HWREGP	t0,DST_CONTROL
	store	0,WORD,t2,t0

	jmp	hcmd_done
	 nop


hcmd_exec:
		; EXEC call.  Downloaded code can return to normal
		; operation by doing a "jmpi raddr."

	constx	t1,host_cmd+hc_cmd_exec
	load	0,WORD,t1,t1			; get execute address
	calli	raddr,t1
	 nop
	jmp	hcmd_done			; in case it returns
	 nop

hcmd_stat:
	; update statistic flags

	call	raddr,st_flags
	 nop
	
	;; store into status region
	constx	t0, BP_JOB_MASK
	and	t0, bp_state, t0
	BPMOVETOSTAT	t0, hst_bp_job_vect
	and	t0, bp_state, BP_ULP_MASK
	BPMOVETOSTAT	t0, hst_bp_ulp
	
	jmp	hcmd_done
	 nop

hcmd_wakeup:
		; WAKE UP!
	andn	state,state,ST_SLEEP

		; Start polling timers!

	SET_TIMER	spoll_timer,2	; we want this to expire immediately.
	SET_TIMER	dpoll_timer,200

	constx	t0,SLEEP_TM
	SET_TIMER	sleep_timer,t0
	

hcmd_done:
		; Done!  Put ID into response area

	constx	t1,host_cmd+hc_cmd_ack
	store	0,WORD,host_o_cmdid,t1

mlop1:


	; //-----------------------------
	; // Handle Polling
	; //-----------------------------

	; if ( ! asleep ) {

	tbit	t0,state,ST_SLEEP
	jmpt	t0,mlop2
	 nop
	
	;	if ( SRC-Polling-Timer-Expired ) {

	CHECK_TIMER	t0,spoll_timer
	jmpf	t0,mlop1aa
	
	;		Set-Polling-Timer-Again;
	;		SRC-Poll-Flag := TRUE;

	 constx	t0,SPOLL_TM
	SET_TIMER	spoll_timer,t0
	or	state,state,ST_SRC_POLL

	;	}
mlop1aa:
	;	if ( DST-Polling-Timer-Expired ) {

	CHECK_TIMER	t0,dpoll_timer
	jmpf	t0,mlop1a

	;		Set-Polling-Timer-Again;
	;		DST-Poll-Flag := TRUE;

	 constx	t0,DPOLL_TM
	SET_TIMER	dpoll_timer,t0
	or	state,state,ST_DST_POLL

	;	}
mlop1a:
	;	if ( Sleep-Timer-Expired ) {

	CHECK_TIMER	t0,sleep_timer
	jmpf	t0,mlop2
	 nop
	
	;		asleep = 1;
	;		Queue-B2H( SLEEP );
	;		B2H-Push-Flag := TRUE;
	;		SRC-Poll-Flag := TRUE;
	;		DST-Poll-Flag := TRUE;

	const	v0,B2H_SLEEP
	call	raddr,queue_b2h
	 consth	v0,B2H_SLEEP
	constx	t0,(ST_SRC_POLL|ST_DST_POLL|ST_SLEEP|ST_B2H_PUSH)
	or	state,state,t0

	;	}
	; }

mlop2:
	; // ----------------------------------------------------------
	; // Handle DMA state machines.  These machines are
	; // only called when DMA is idle.  This is complicated
	; // by the fact that these state machines must "arbitrate"
	; // for the one DMA engine.

	; // The state machines are broken up into ???-DMA-Done()
	; // and ???-DMA().  ???-DMA-Done() does the absolute
	; // minimum work needed for that "DMA client" to relenquish
	; // the DMA engine (transfer cycles, saving XSUM regs), etc.
	; // The ???-DMA() finishes the work and (possibly) starts
	; // a new DMA.
	; // ----------------------------------------------------------

	; if (  ! DMA-Done-Interrupt-Enabled && DMA-Done ) {
	tbit	t0,state,ST_DMA_DNINT
	jmpt	t0,mlop3
	 nop

	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_NOTDN
	jmpt	t0,mlop3
	 nop
	
	; 	switch ( DMA-Client ) {

	cpeq	t0,dma_client,DMA_CLIENT_SRC
	jmpf	t0,mlop2c
	 nop

	; 	case SRC:
	; 		SRC-DMA-Done();

	call	raddr,src_dma_dn
	 nop
	
	; 		if ( B2H-Push-Flag )
	tbit	t0,state,ST_B2H_PUSH
	jmpf	t0,mlop2a
	 nop
	
	; 			DMA-B2H();
	call	raddr,write_b2h
	 nop

mlop2a:
	; 		if ( DST-DMA() )
	; 			break;
	call	raddr,dst_dma
	 nop
	jmpt	v0,mlop3
	 nop

	; 		else if ( SRC-DMA() )
	; 			break;
	call	raddr,src_dma
	 nop
	jmpt	v0,mlop3
	 nop

	; 		else
	; 			DMA-Client := IDLE;

	jmp	mlop3
	 const	dma_client,DMA_CLIENT_NONE

	; 		break

mlop2c:
	cpeq	t0,dma_client,DMA_CLIENT_DST
	jmpf	t0,mlop2f
	 nop

	; 	case DST:
	; 		DST-DMA-Done();
	call	raddr,dst_dma_dn
	 nop
	
	; 		if ( B2H-Push-Flag )
	; 			DMA-B2H();

	tbit	t0,state,ST_B2H_PUSH
	jmpf	t0,mlop2d
	 nop
	
	call	raddr,write_b2h
	 nop

mlop2d:
	; 		if ( SRC-DMA() )
	;			break;

	call	raddr,src_dma
	 nop
	jmpt	v0,mlop3
	 nop

	; 		else if ( DST-DMA() )
	;			break;
	call	raddr,dst_dma
	 nop
	jmpt	v0,mlop3
	 nop

	; 		else
	; 			DMA-Client := IDLE;

	jmp	mlop3
	 const	dma_client,DMA_CLIENT_NONE

	; 		break;

mlop2f:
	; 	case IDLE:
	; 		if ( B2H-Queue-Len > 0 || B2H-Push-Flag )

	tbit	t0,state,ST_B2H_PUSH
	cpgt	t1,b2h_queued,0
	or	t0,t1,t0
	jmpf	t0,mlop2g
	 nop
	; 			DMA-B2H();
	call	raddr,write_b2h
	 nop

mlop2g:
	; 		if ( DST-DMA() )
	;			break;
	call	raddr,dst_dma
	 nop
	jmpt	v0,mlop3
	 nop

	; 		else if ( SRC-DMA() )
	;			break;
	call	raddr,src_dma
	 nop
	jmpt	v0,mlop3
	 nop
	
	const	dma_client,DMA_CLIENT_NONE

	; 		break;
	; 	}
	; }


mlop3:

	; // -----------------------------------------
	; // Do we have a packet to send?  Stuff WOKI
	; // -----------------------------------------

	; if ( send_blk ) {

	cpeq	t0,send_blk,0
	jmpt	t0,mlop4

	;    if ( SOURCE-DRAIN || NO-DSIC ) {

	 tbit	t0,state2,ST2_SRC_DRAIN
	jmpt	t0,mlop3drn
	 tbit	t0,state2,ST2_SRC_NODSIC
	jmpf	t0,mlop3a0
	 nop

	;		// Discard until end-of-connection
	
	INCSTAT	hst_s_dsic_lost
	const	src_err,B2H_OSTAT_DSIC
	or	state2,state2,ST2_SRC_DRAIN


mlop3drn:
	;		if ( d2b_chunks.O == 0 ) {

	jmpt	send_flags,mlop3j
	 nop

	;			Queue-B2H( ODONE, status = src_err );

	load	0,WORD,t0,d2b_send
	and	v0,t0,0x0F			; get stack in there
	tbit	t0,t0,D2B_NACK
	jmpt	t0,mlop3anack
	 or	v0,v0,(B2H_ODONE>>16)
	sll	v0,v0,16
	or	v0,v0,1
	call	raddr,queue_b2h
	 or	v0,v0,src_err

	or	state,state,(ST_B2H_PUSH|ST_HINT)
mlop3anack:
	;			d2b_send += d2b_send->hd.chunks+1;
	;			src_tag++;
	;			d2b_out = d2b_send;

	load	0,WORD,t0,d2b_send
	srl	t0,t0,D2B_CHUNKS_SHIFT		; t0 = d2b_send->hd.chunks
	add	t0,t0,1			; +1
	sll	t0,t0,3			; *8
	add	d2b_send,d2b_send,t0		; d2b_send += 8*(chunks+1)

	cpge	t0,d2b_send,d2b_lim		; wrap d2b_send?
	jmpf	t0,mlop3k
	 addu	src_tag_in,src_tag_in,1		; src_tag_in++;
	
	constx	t0,d2b				; wrap d2b_send!
	sub	t0,d2b_lim,t0
	sub	d2b_send,d2b_send,t0

mlop3k:
	mov	d2b_out,d2b_send

	;			if ( (send_flags & (NEOP|NEOC) ) == 0 ) {

	tbit	t0,send_flags,D2B_NEOC
	jmpt	t0,mlop3j
	 tbit	t0,send_flags,D2B_NEOP
	jmpt	t0,mlop3j
	 nop

	;				SOURCE-DRAIN := FALSE;

	andn	state2,state2,ST2_SRC_DRAIN

	;				if ( ! DSIC-Lost ) {
	;					(start up source machine)
	tbit	t0,state2,ST2_SRC_NODSIC
	jmpt	t0,mlop3j
	 nop
	
	constx	t0,(SRC_CTL_GO|SRC_CTL_SHIFT)
	HWREGP	t1,SRC_CONTROL
	store	0,WORD,t0,t1

	andn	state2,state2,ST2_SRC_SHFT

	constx	t0,SWOKI_FLUSH		; get SCZ set again!
	HWREGP	t1,SRC_WOKI_WR
	store	0,WORD,t0,t1

	;				}
	;			}

mlop3j:
	;		}
	;		XFER-CYCLE( send_blk );

	constx	t1,OFFS_XFER
	call	raddr,src_read_xfer
	 add	v0,send_blk,t1
	

	;		src_send = send_blk;
	mov	src_send,send_blk

	;		send_blk = 0;
	;		break;
	jmp	mlop4
	 const	send_blk,0

	;    }
	;    else { // ! DRAIN


mlop3a0:

	;	if ( send_cksum_offs >= 0 ) {

	jmpt	send_cksum_offs,mlop3a4

	;		// merge two values from checksummer hardware

	 constx	t1,0x1FFFF
	and	send_cksum_lo,send_cksum_lo,t1
	and	send_cksum_hi,send_cksum_hi,t1
	const	t1,0xFFFF
	add	send_cksum_lo,send_cksum_lo,send_cksum_hi
	srl	t0,send_cksum_lo,16
	and	send_cksum_lo,send_cksum_lo,t1
	add	send_cksum_lo,send_cksum_lo,t0
	srl	t0,send_cksum_lo,16
	and	send_cksum_lo,send_cksum_lo,t1
	add	send_cksum_lo,send_cksum_lo,t0

	xor	send_cksum_lo,send_cksum_lo,t1	; complement cksum

	add	t3,src_send,send_cksum_offs	; t3 gets address in packet
	CK ( D2B_FN64ALIGN_BT > 2 )
	srl	t0,send_flags,(D2B_FN64ALIGN_BT-2)
	and	t0,t0,4
	add	t3,t3,t0			; skip alignment cruft
	andn	t3,t3,2				; header where cksum is to go
	constx	t0,(SRC_DRAM+DRAM_SIZE)		; wrap?
	cpge	t0,t3,t0
	jmpf	t0,mlop3a1
	 constx	t0,SRC_BUF_SIZE
	sub	t3,t3,t0
mlop3a1:
	load	0,WORD,t4,t3
	sll	t0,send_cksum_offs,30
	jmpt	t0,mlop3a2
	 nop

	and	t4,t4,t1			; put in high word
	sll	t0,send_cksum_lo,16
	jmp	mlop3a3
	 or	t4,t4,t0
mlop3a2:
	andn	t4,t4,t1			; put in low word
	or	t4,t4,send_cksum_lo
mlop3a3:
	store	0,WORD,t4,t3

	;	}

mlop3a4:
	;	Push-Source-WOKI's( tag = src_tag );

	HWREGP	t4,SRC_WOKI_WR

	sub	t1,send_blk,src_send		; t1 = (send_blk-src_send)
	cpge	t0,t1,0				; t1 >= 0
	jmpt	t0,mlop3a
	 constx	t0,SRC_BUF_SIZE
	add	t1,t1,t0
mlop3a:

	tbit	t0,last_swoki,SWOKI_KEEPCON
	jmpt	t0,mlop3b
	 nop
	INCSTAT	hst_s_conns
mlop3b:

	; we're building a source WOKI in t1...start with length.

	; t1 has length in bytes...convert to words
	srl	t1,t1,2
	

	sll	t3,src_tag_in,SWOKI_TAGSHFT	; put in transfer tag
	tbit	t5, send_flags, D2B_BP_PKT	; but if bypass, put in previous tag
	jmpf	t5, mlop3b_nbp
	  constx	t5, SWOKI_TAG_MASK

	and     t3, last_swoki, t5

mlop3b_nbp:
	or	t1,t1,t3

	;; if bypass packet, set bit in WOKI WORD
	srl	t0, send_flags, D2B_BP_PKT_BT
	and	t0, t0, 1
	sll	t0, t0, SWOKI_BPSHFT
	or	t1, t1, t0
	
	; set NEOP bit if this is not last block in packet.
	srl	t0,send_flags,31
	sll	t0,t0,SWOKI_KEEPPKT_BT
	or	t1,t1,t0

		; put NEOC/NEOP flags from D2B into WOKI

	CK	(SWOKI_KEEPPKT_BT-D2B_NEOP_BT)==(SWOKI_KEEPCON_BT-D2B_NEOC_BT)
	CK	D2B_NEOC_BT > D2B_NEOP_BT
	srl	t0,send_flags,D2B_NEOP_BT	; t0 = (send_flags >> NEOP_BT)
	and	t0,t0,((D2B_NEOC|D2B_NEOP)>>D2B_NEOP_BT)
	sll	t0,t0,SWOKI_KEEPPKT_BT
	or	t1,t1,t0			; t1 |= <flags from d2b_flags>

		; get ready to stuff woki, first check if we need stop
	
	mtsrim	CPS, PS_INTSOFF			; INTS off!

	tbit	t0,state,ST_SSTOP_PEND		; stop pending?
	jmpt	t0,mlop3d

	.if 0		; XXX: always stuff a STOP woki-- for now

	 HWREGP	t0,SRC_BLK_ADD			; figure if far enough ahead
	load	0,WORD,t0,t0			; t0 = ( src_send - BLKADDR)
	constx	t2,BLKADDR_MASK
	and	t0,t0,t2
	constx	t2,SRC_DRAM
	add	t0,t0,t2
	sub	t0,src_send,t0			;	  mod SRC_BUF_SIZE
	cpge	t2,t0,0
	jmpt	t2,mlop3c
	 constx	t2,SRC_BUF_SIZE
	add	t0,t0,t2
mlop3c:
	constx	t2,(1024)			; XXX: "far enough ahead?!"
	cpgt	t2,t0,t2
	jmpt	t2,mlop3d

	.endif

		; save address so that interrupt handler will
		; know where to do XFER cycle
	 constx	t2,OFFS_XFER
	add	src_stop_addr,t2,src_send

		; put in stop woki
	or	state,state,ST_SSTOP_PEND

	constx	t0,(SWOKI_FLUSH|SWOKI_STOP)
	or	t0,t0,t3			; t3 starts as src_tag<<23
	constx	t2,(SWOKI_KEEPCON|SWOKI_KEEPPKT)
	and	t2,t2,last_swoki
	or	t0,t0,t2
	store	0,WORD,t0,t4			; stuff STOP WOKI

mlop3d:
	tbit	t0,send_flags,D2B_FN64ALIGN	; need to skip first 32 bits?
	jmpf	t0,mlop3da

	 constx	t0,(SWOKI_FLUSH|1)

	tbit	t5, send_flags, D2B_BP_PKT	;  if bypass, put in previous tag
	jmpf	t5, mlop3d_bp

	constx	t5, SWOKI_TAG_MASK
	and     t3, last_swoki, t5


mlop3d_bp:	
	or	t0,t0,t3			; t3 starts as src_tag<<23
	constx	t2,(SWOKI_KEEPCON|SWOKI_KEEPPKT)
	and	t2,t2,last_swoki
	or	t0,t0,t2
	store	0,WORD,t0,t4			; stuff 4FLUSH WOKI

	sub	t1,t1,1				; adjust woki XXX
	add	src_send,src_send,4		; adjust src_send

mlop3da:
	cpeq	t0,send_fburst,0		; stuff a WOKI for short-
	jmpt	t0,mlop3daa			; burst first
	 srl    t0,last_swoki,SWOKI_KEEPCON_BT
	and	t0,t0,1
	xor	t0,t0,1
	add	send_fburst,send_fburst,t0
	
	.if DEBUG
	tbit	t0,last_swoki,SWOKI_KEEPPKT	; ASSERT( begin-of-packet );
	jmpf	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	constx	t0,(SWOKI_KEEPCON|SWOKI_KEEPPKT)
	or	last_swoki,send_fburst,t0
	sll     t0,src_tag_in,SWOKI_TAGSHFT     ; put in transfer tag
	or	last_swoki,last_swoki,t0

	store	0,WORD,last_swoki,t4		; stuff a woki for short burst

	sub	t1,t1,send_fburst		; trim it from "real" WOKI

	sll	send_fburst,send_fburst,2
	add	src_send,src_send,send_fburst	; adjust src_send

	constx	t0,SWOKI_MAX_LEN
	and	t0,t1,t0
	cpeq	t0,t0,0
	jmpt	t0,mlop3dde
	 nop
						; trimmed to zero? skip stuff
mlop3daa:

	tbit	t0,t1,SWOKI_KEEPPKT		; KEEPPKT? trim length to burst
	jmpf	t0,mlop3ddd			; multiple.
	 srl	t0,last_swoki,SWOKI_KEEPCON_BT
	and	t0,t0,1
	xor	t0,t0,1				; t0 = last_swoki.KEEPCON ? 0:1

	sub	t1,t1,t0			; don't trim off i-field
	andn	t1,t1,255			; len &= ~1023;
	add	t1,t1,t0
	
	constx	t0,SWOKI_KEEPCON		; enforce KEEPPKT --> KEEPCONN
	or	t1,t1,t0

	constx	t0,SWOKI_MAX_LEN
	and	t0,t1,t0
	cpeq	t0,t0,0
	jmpt	t0,mlop3dde			; trimmed to zero? skip stuff
	 nop

mlop3ddd:
	store	0,WORD,t1,t4			; stuff ACTUAL WOKI
mlop3dde:
	mov	last_swoki,t1

	tbit	t0,send_flags,D2B_BN64ALIGN	; need to skip last 32 bits?
	jmpf	t0,mlop3db

	 constx	t0,(SWOKI_FLUSH|1)
	constx	t2,(SWOKI_TAG_MASK|SWOKI_KEEPCON|SWOKI_KEEPPKT)
	and	t2,t2,last_swoki
	or	t0,t0,t2
	store	0,WORD,t0,t4			; stuff 4FLUSH WOKI
	add	src_send,src_send,4		; adjust src_send

mlop3db:
	mtsrim	CPS, PS_INTSON			; ints on

	constx	t0,SWOKI_MAX_LEN
	and	t1,t1,t0
	sll	t1,t1,2				; back to byte count
	add	src_send,src_send,t1		; src_send += length
	constx	t0,SRC_DRAM+DRAM_SIZE
	cpge	t0,src_send,t0			; off edge?
	jmpf	t0,mlop3e
	 constx	t0,SRC_BUF_SIZE
	sub	src_send,src_send,t0

mlop3e:
	;; if bypass packet, don't update d2b structures
	tbit	t0, send_flags, D2B_BP_PKT
	jmpf	t0, mlop3e1
	 nop	 
	INCSTAT	hst_s_packets
	BPINCSTAT hst_s_bp_packets
	add	src_wdog_bp_pkts, src_wdog_bp_pkts, 1
	
	jmp	mlop3g
	 nop

mlop3e1:	

	;	d2b_send->??.tag = src_tag;     // XXX: happens many times

	add	d2b_send,d2b_send,4		; d2b_send->tag = src_tag_in;
	load	0,WORD,t0,d2b_send
	or	t0,t0,t3
	store	0,WORD,t0,d2b_send
	sub	d2b_send,d2b_send,4



	;	if ( d2b_chunks.O == 0 /* last block of packet */ ) {
	jmpt	send_flags,mlop3g
	 nop

	tbit	t0,d2b_flags,D2B_NEOP
	jmpt	t0,mlop3h
	 nop
	INCSTAT	hst_s_packets
mlop3h:

	;		d2b_send += d2b_send->hd.chunks+1;
	;		src_tag++;

	load	0,WORD,t0,d2b_send
	srl	t0,t0,D2B_CHUNKS_SHIFT		; t0 = d2b_send->hd.chunks
	add	t0,t0,1			; +1
	sll	t0,t0,3			; *8
	add	d2b_send,d2b_send,t0		; d2b_send += 8*(chunks+1)

	cpge	t0,d2b_send,d2b_lim		; wrap d2b_send?
	jmpf	t0,mlop3g
	 addu	src_tag_in,src_tag_in,1		; src_tag_in++;
	
	constx	t0,d2b				; wrap d2b_send!
	sub	t0,d2b_lim,t0
	sub	d2b_send,d2b_send,t0

	;	}

mlop3g:
	;	send_blk = 0;

	const	send_blk,0

	;    }

	; }


mlop4:
	; // ------------------------------------------------------
	; // Source Processing:
	; //
	; // Check source for unscheduled stops or to see if outputs
	; // are done.  Use tag at front of FIFO
	; // ------------------------------------------------------

	; t2 := <SRC-WOKI-At-Head>                 	; must be read FIRST

	HWREGP	t4,SRC_WOKI_RD
	load	0,WORD,t2,t4				; t2 = front WOKI

	; t1 := <SRC-Status-Register>

	HWREGP	t4,SRC_RT
	load	0,WORD,t1,t4				; t1 = SRC real time

	; if ( DSIC-Lost || SOURCE-DRAIN ) {

	;	XXX: tricky logic warning--- t3 holds drain flag
	;	     for more tests,

	tbit	t0,state2,ST2_SRC_NODSIC
	jmpt	t0,src0
	 tbit	t3,state2,ST2_SRC_DRAIN
	jmpf	t3,src1

src0:
	;	// Modes in which we expect source machine to be stopped.

	;	if ( ! SOURCE-DRAIN && t1.DSIC-Found ) {

	 tbit	t0,t1,SRC_RT_STOP_DSIC
	jmpt	t0,src0b
	 nop
	jmpt	t3,src5		; other test will fail too

	;		S2020-MODE-SEL := RUN;

	 const	t0,SRC_MODE_RUN
	HWREGP	t3,SRC_MODE_SEL
	store	0,WORD,t0,t3

	;		DSIC-Lost := FALSE;

	andn	state2,state2,ST2_SRC_NODSIC

	;		SRC-Control := <PRIME|GO| <SHIFT-FLAG?>;

	tbit	t0,state2,ST2_SRC_SHFT
	jmpf	t0,src0a
	 const	t0,(SRC_CTL_PRIME|SRC_CTL_GO)
	
	or	t0,t0,SRC_CTL_SHIFT

	;		SHIFT-FLAG := 0;

	andn	state2,state2,ST2_SRC_SHFT

	constx	t1,SWOKI_FLUSH		; get SCZ bit set again.
	HWREGP	t3,SRC_WOKI_WR
	store	0,WORD,t1,t3

src0a:
	HWREGP	t3,SRC_CONTROL
	jmp	src5
	 store	0,WORD,t0,t3

	;	}
src0b:
	;	else if ( ! t1.DSIC-Found && ! DSIC-Lost ) {

	; XXX: No tests necessary!  check tricky logic above

	;		S2020-MODE-SEL := WAIT;

	const	t0,SRC_MODE_WAIT
	HWREGP	t3,SRC_MODE_SEL
	store	0,WORD,t0,t3

	;		DSIC-Lost := TRUE;

	jmp	src5
	 or	state2,state2,ST2_SRC_NODSIC

	;	}

	; }
src1:
	; else if ( t1.WOKI_FIFO_EMPTY && t1.SCZ && ! t1.STOPPED ) {

	tbit	t0,t1,SRC_RT_WOKI_NE
	jmpt	t0,src3
	 tbit	t0,t1,SRC_RT_SCZERO
	jmpf	t0,src3
	 nop

	load	0,WORD,t1,t4				; double check STOP bit
							; (to avoid race)

	tbit	t0,t1,SRC_RT_STOPPED
	jmpt	t0,src3
	 nop

	;	// XXX: could coalesce ODONE's here
	;	WHILE ( d2b_out != d2b_send && d2b_out->tag <MOD src_tag ) {

src2:
	 cpeq	t0,d2b_out,d2b_send
	jmpt	t0,src5
	 nop
	add	d2b_out,d2b_out,4
	load	0,WORD,t3,d2b_out
	sub	d2b_out,d2b_out,4
	srl	t3,t3,SWOKI_TAGSHFT
	sub	t0,t3,src_tag_in
	sll	t0,t0,SWOKI_TAGSHFT
	jmpf	t0,src5
	 nop

	;		Queue-B2H( ODONE( 1, stk=d2b_out->stk, status=OKAY ));
	
	load	0,WORD,t3,d2b_out

	.if DEBUG
	tbit	t0,t3,D2B_RDY		; ASSERT( d2b_out->hd.flags & RDY );
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	tbit	t0,t3,D2B_NACK
	jmpt	t0,src2b

	 and	t0,t3,0x0F				; get d2b_out->stk
	sll	t0,t0,16
	constx	v0,(B2H_ODONE|B2H_OSTAT_GOOD|1)		; Queue-B2H( ...
	or	v0,v0,t0

	and	t0,t3,0x0F				; get d2b_out->stk
	cpeq	t0,t0,HIP_STACK_LE
	jmpf	t0,src2a
	 nop

	call	raddr,queue_b2h_di
	 nop
	jmp	src2b
	 nop

src2a:
	call	raddr,queue_b2h
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

src2b:
	
	
;	.if DEBUG
;	store	0,WORD,zero,d2b_out
;	.endif

	;		d2b_out += 8*(d2b_out->chunks + 1);
	
	srl	t3,t3,D2B_CHUNKS_SHIFT
	add	t3,t3,1
	sll	t3,t3,3
	add	d2b_out,d2b_out,t3

	cpge	t0,d2b_out,d2b_lim			; wrap d2b_out?
	jmpf	t0,src2

	 constx	t0,d2b
	sub	t0,d2b_lim,t0
	jmp	src2
	 sub	d2b_out,d2b_out,t0

	;	}
	; }
	; else {

src3:
	; // WOKI FIFO not finished or src stopped.  Acknowledge all
	; // the D2B's which we KNOW are completed.  If source is stopped,
	; // go recover.

	; if ( ! t1.STOPPED && same-woki-tag-and-blkaddr-for-X-timer-ticks ) {

	; // Time out transmits that have not moved in a while...

	tbit	t0,t1,SRC_RT_STOPPED
	jmpt	t0,src3a
	 HWREGP	t4,SRC_BLK_ADD		; t4 = <blk-addr>:<tag>
	load	0,WORD,t4,t4
	constx	t0,(BLKADDR_MASK|BLKADDR_QSF)
	and	t4,t4,t0
	srl	t0,t2,SWOKI_TAGSHFT
	or	t4,t4,t0

	cpeq	t0,t4,src_wdog_blktag
	jmpf	t0,src3a
	 mov	src_wdog_blktag,t4

	cpeq	t0, src_wdog_bp_pkts, src_wdog_old_bp_pkts
	jmpt	t0, src3_bp
	 nop

	mov	src_wdog_old_bp_pkts, src_wdog_bp_pkts
	jmp	src3a

src3_bp:	
	cpgt	t0,src_wdog_cdown,0
	jmpt	t0,src3b

	;	// Time-out transmission
	;	Stop-and-Disconnect();
	 HWREGP	t4,SRC_CONTROL
	const	t0,SRC_CTL_STOP
	store	0,WORD,t0,t4

	;	t2 = <SRC-WOKI-At-Head>;
	HWREGP	t4,SRC_WOKI_RD
	load	0,WORD,t2,t4

	;	t1 = <SRC-Realtime-Register>;
	HWREGP	t4,SRC_RT
	load	0,WORD,t1,t4

	; }

src3a:
	 mov	src_wdog_cdown,src_wdog_tval

src3b:
	;	// XXX: could coalesce ODONE's here
	;	WHILE ( d2b_out != d2b_send && d2b_out->tag <MOD t2.tag ) {
	cpeq	t0,d2b_out,d2b_send
	jmpt	t0,src3e
	 nop

	add	d2b_out,d2b_out,4
	load	0,WORD,t3,d2b_out
	sub	d2b_out,d2b_out,4

	srl	t0,t2,SWOKI_TAGSHFT
	sll	t0,t0,SWOKI_TAGSHFT
	sub	t0,t3,t0

	jmpf	t0,src3e
	 nop

	;		Queue-B2H( ODONE( 1, stk=d2b_out->stk, status=OKAY ));
	
	load	0,WORD,t3,d2b_out

	.if DEBUG
	tbit	t0,t3,D2B_RDY		; ASSERT( d2b_out->hd.flags & RDY );
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	tbit	t0,t3,D2B_NACK
	jmpt	t0,src3d

	 and	t0,t3,0x0F				; get d2b_out->stk
	sll	t0,t0,16
	constx	v0,(B2H_ODONE|B2H_OSTAT_GOOD|1)		; Queue-B2H( ...
	or	v0,v0,t0

	and	t0,t3,0x0F
	cpeq	t0,t0,HIP_STACK_LE
	jmpf	t0,src3c
	 nop

	call	raddr,queue_b2h_di
	 nop
	jmp	src3d
	 nop

src3c:
;                           if(t2 << SWOKI_BP_BT)  {  // for debugging bp WOKI
	tbit	t0, t2, SWOKI_BP
	jmpf	t0, not_bypass_WOKI
;                                                  several nops;
	  nop
	nop
	nop
	nop
	nop
;                                          }

	

not_bypass_WOKI:
	call	raddr,queue_b2h
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

src3d:

	;		d2b_out += 8*(d2b_out->chunks + 1);
	
	srl	t3,t3,D2B_CHUNKS_SHIFT
	add	t3,t3,1
	sll	t3,t3,3
	add	d2b_out,d2b_out,t3

	cpge	t0,d2b_out,d2b_lim			; wrap d2b_out?
	jmpf	t0,src3b

	 constx	t0,d2b
	sub	t0,d2b_lim,t0
	jmp	src3b
	 sub	d2b_out,d2b_out,t0

	;	}
src3e:
	; 	if ( t1.STOPPED )

	tbit	t0,t1,SRC_RT_STOPPED
	jmpt	t0,recover_source	; jmps back to src5
	 nop

	; }
src5:

mlop5:

	; // ---------------------------------------------
	; // Process any C2B's we have
	; // ---------------------------------------------
	; if ( c2b_avail ) {

	cpgt	t0,c2b_avail,0
	jmpf	t0,mlop6
	 nop

	; 	Process-C2Bs();

	constx	t4,c2b
proc_c2b_1:
	load	0,WORD,t2,t4				; get C2B
	add	t4,t4,4
	load	0,WORD,t3,t4
	add	t4,t4,4

	srl	t1,t2,(C2B_OP_SHIFT+2)			; look up cmd
	and	t1,t1,(C2B_OPMASK>>2)
	constx	t0,c2b_cmd_tbl
	add	t0,t0,t1
	load	0,WORD,t0,t0
	jmpi	t0					; jump
	 nop

proc_c2b_sml:
	srl	t0,t2,C2B_OP_SHIFT			; which stack?
	and	t1,t0,C2B_STMASK
	cpeq	t0,t1,HIP_STACK_LE
	jmpf	t0,proc_c2b_1a

	 tbit	t0,op_flags,OPF_ENB_LE
	jmpf	t0,proc_c2b_nxt

	 constx	t0,ifhip_sml_end
	
	store	0,WORD,t2,ifhip_sml_in
	add	ifhip_sml_in,ifhip_sml_in,4
	store	0,WORD,t3,ifhip_sml_in
	add	ifhip_sml_in,ifhip_sml_in,4

	cpge	t0,ifhip_sml_in,t0			; wrap?
	jmpf	t0,proc_c2b_nxt
	 constx	t0,(ifhip_sml_end-ifhip_sml)
	jmp	proc_c2b_nxt
	 sub	ifhip_sml_in,ifhip_sml_in,t0

proc_c2b_1a:
	sll	t1,t1,3					; hdr buf for a stack
	constx	t0,hdrbufs
	add	t1,t1,t0
	store	0,WORD,t2,t1
	add	t1,t1,4
	jmp	proc_c2b_nxt
	 store	0,WORD,t3,t1

proc_c2b_big:

	.if DEBUG
	srl	t0,t2,C2B_OP_SHIFT			; ASSERT( stack == LE )
	and	t1,t0,C2B_STMASK
	cpeq	t0,t1,HIP_STACK_LE
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	tbit	t0,op_flags,OPF_ENB_LE
	jmpf	t0,proc_c2b_nxt

	 constx	t0,ifhip_big_end
	
	store	0,WORD,t2,ifhip_big_in
	add	ifhip_big_in,ifhip_big_in,4
	store	0,WORD,t3,ifhip_big_in
	add	ifhip_big_in,ifhip_big_in,4

	cpge	t0,ifhip_big_in,t0
	jmpf	t0,proc_c2b_nxt
	 constx	t0,(ifhip_big_end-ifhip_big)
	jmp	proc_c2b_nxt
	 sub	ifhip_big_in,ifhip_big_in,t0

proc_c2b_wrap:
	
	or	state,state,ST_DST_POLL		; must poll for more

	const	c2b_avail,0
	jmp	mlop6
	 const	c2b_hnxt,0

proc_c2b_empty:

	jmp	mlop6
	 const	c2b_avail,0

proc_c2b_read:

	constx	t0,c2b_read_tbl
	srl	t1,t2,(C2B_OP_SHIFT-3)
	and	t1,t1,(C2B_STMASK<<3)
	add	t1,t1,t0

	store	0,WORD,t2,t1
	add	t1,t1,4
	jmp	proc_c2b_nxt
	 store	0,WORD,t3,t1

proc_c2b_nxt:

	; push out delayed interrupts as C2B's (other than empty) are found
	constx	t0,HINT_TM
	SET_TIMER	hint_timer,t0

	sub	c2b_avail,c2b_avail,8
	cpgt	t0,c2b_avail,0
	jmpt	t0,proc_c2b_1
	 add	c2b_hnxt,c2b_hnxt,8

	; 	if ( more-C2Bs-out-there )
	; 		DST-Polling-Flag := TRUE;

	or	state,state,ST_DST_POLL

	;	c2b_avail = 0;
	;	<will be zero>

	.if	DEBUG
	cpeq	t0,c2b_avail,0
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	; }

mlop6:
	; // --------------------------------------
	; // Check destination
	; // --------------------------------------

	; switch ( DST-FIFO-State ) {

	cpeq	t0,dfifo_state,DFIFOST_NONE
	jmpt	t0,dfifo0
	 cpeq	t0,dfifo_state,DFIFOST_FLUSH
	jmpt	t0,dfifo2
	 nop
	jmp	dfifo3
	 nop

	; case DST-NONE:

dfifo0:
	;	if ( DST-WOKI Empty && DST-DRAIN-FLAG ) {
	HWREGP	t0,INT_33				; get woki NE flag
	load	0,WORD,t0,t0
	tbit	t0,t0,INT33_DST_WOKI_NE
	jmpt	t0,dfifo0a

	 tbit	t0,state2,ST2_DST_DRAIN
	jmpf	t0,dfifo9

	 HWREGP	t1,DST_CONNECT
	load	0,WORD,t0,t1
	tbit	t0,t0,DST_CONN_CONNIN			; ! CONNIN?
	jmpt	t0,dfifo9

	 HWREGP	t1,DST_RT_CLR
	load	0,WORD,t0,t1
	 tbit	t0,t0,DST_RT_SRCAV			; SRCAV?
	jmpf	t0,dfifo9

	 cpeq	t0,ddma_state,DDMAST_DMA_FP		; wait until DMA of
	jmpt	t0,dfifo9				; big chunks is cmplt
	 nop
	
	call	raddr,dst_reset_rdy			; reset READYs!
	 nop

	 tbit	t0,op_flags,OPF_ACCEPT			; accept/reject
	jmpt	t0,dfifo00				; connections
	 const	t0,DST_CONN_ACCEPT
	const	t0,0	; (REJECT)
dfifo00:
	HWREGP	t1,DST_CONNECT
	 store	0,WORD,t0,t1

	jmp	dfifo9
	 andn	state2,state2,(ST2_DST_DRAIN|ST2_DST_FEOP)

	; 	}
	;	else if ( DST-WOKI-NE ) {

dfifo0a:
	HWREGP	t0,DST_WOKI_LO				; read WOKI
	load	0,WORD,t1,t0				; into <t1,t2>
	add	t0,t0,4
	CK ( DST_WOKI_LO+4 == DST_WOKI_HI )
	load	0,WORD,t2,t0

	constx	t0,DWOKI_COUNT_BITS			; mask count bits
	and	dfifo_len,t2,t0

	; XXX: process woki flags
	;	- error cases
	;	- end of empty connection

	;		if ( ERROR-flags ) {

	.equ	DWOKI_BAD_BITS,(DWOKI_AD_SEQ_ERR|DWOKI_AD_PARITY|DWOKI_AD_LLRC_ERR|DWOKI_AD_SYNC|DWOKI_AD_ILBRST|DWOKI_EONC|DWOKI_SDIC_LOSS)

	constx	t0,DWOKI_BAD_BITS
	and	dfifo_flags,t1,t0
	cpeq	t0,dfifo_flags,0
	jmpf	t0,dfifo_err

	;		}
	; 		else if ( <WOKI> is fill-and-flush / garbage ) {

	 and	t3,t1,DWOKI_DTYPE_MASK
	cpeq	t0,t3,DWOKI_DTYPE_DATA
	jmpt	t0,dfifo0b

	 cpeq	t0,t3,DWOKI_DTYPE_STATUS
	jmpf	t0,dfifo0aa
	 nop

	mov	dst_statblock,dst_in
dfifo0aa:

	; 			dst_in += <WOKI>.len;
	; 			goto DST-NONE;

	.if DEBUG
	cpeq	t0,dfifo_residue,0		; woki garbage shouldn't
	jmpt	t0,.+12				; break up our packets
	nop
	call	raddr,fatal
	nop
	.endif

	jmp	dfifo0
	 add	dst_in,dst_in,dfifo_len

	; 		}
dfifo0b:
	; 		else {
	; 			dfifo_len = <WOKI>.len;
	; 			dfifo_flags = <WOKI>.flags;


	constx	t0,(DWOKI_EOP|DWOKI_IFIELD|DWOKI_THRESH)
	and	dfifo_flags,t1,t0

	tbit	t0,dfifo_flags,DWOKI_IFIELD	; we pick up I-fields
	jmpf	t0,dfifo0c			; separately even though
	 nop					; I-field bug was fixed.

	INCSTAT	hst_d_conns

	.if DEBUG
	cpeq	t0,dfifo_len,4			
	jmpt	t0,.+16
	 HWREGP	t0,DST_CONTROL
	call	raddr,fatal
	 load	0,WORD,v0,t0
	.endif ; DEBUG

	mov	dfifo_ifield,dst_in		; save address, pick up I later
	jmp	dfifo0				
	 add	dst_in,dst_in,dfifo_len


dfifo0c:

	;			if ( spurious-woki )
	;				residue += dfifo_len;
	;				break;

	cpeq	t0,dfifo_flags,0
	jmpf	t0,dfifo0d
	 nop
	
	; spurious woki-- save value for later

	.if DEBUG
	add	dst_weirdwokis,dst_weirdwokis,1
	.endif

	jmp	dfifo0
	 add	dfifo_residue,dfifo_residue,dfifo_len


dfifo0d:

	; we wrap dst_in only now...

	cpge	t0,dst_in,dst_dram_end			; wrap dst_in?
	jmpf	t0,dfifo0e
	 nop

	sub	dst_in,dst_in,dst_dram_size

dfifo0e:
	;			if ( DST-DRAIN )
	tbit	t0,state2,ST2_DST_DRAIN
	jmpf	t0,dfifo0eee
	 tbit	t0,state2,ST2_DST_FEOP
	
	.if DEBUG
	add	dst_drained_wokis,dst_drained_wokis,1
	.endif ; DEBUG

	jmp	dfifo9
	 add	dst_in,dst_in,dfifo_len

dfifo0eee:
	;			else if ( ST2_DST_FEOP )

	jmpf	t0,dfifo0g
	 tbit	t0,dfifo_flags,DWOKI_EOP
	
	add	dfifo_len,dfifo_len,dfifo_residue
	const	dfifo_residue,0

	mov	v0,dfifo_len
	srl	t0,dfifo_flags,DWOKI_IFIELD_BT-2
	and	t0,t0,4
	call	raddr,dst_ready
	 sub	v0,v0,t0

	add	dst_in,dst_in,dfifo_len

	cpge	t0,dst_in,dst_dram_end			; wrap dst_in?
	jmpf	t0,dfifo0eef
	 nop

	sub	dst_in,dst_in,dst_dram_size
dfifo0eef:
	
	tbit	t0,dfifo_flags,DWOKI_EOP
	jmpf	t0,dfifo9
	 nop
	
	jmp	dfifo9
	 andn	state2,state2,(ST2_DST_FEOP|ST2_DST_NBOP)

dfifo0g:
	;			else if ( END-OF-PACKET )
	;				goto FLUSH;

	jmpf	t0,dfifo0f
	 add	dfifo_len,dfifo_len,dfifo_residue

	; end-of-packet?	goto FLUSH the end


	cpeq	t0,dfifo_len,0			; we can spurious 0 len
	jmpf	t0,dfifo0ee			; EOP wokis in error
	tbit	t0,state2,ST2_DST_NBOP		; situations.  discard.
	jmpf	t0,dfifo9

dfifo0ee:

	 mov	dfifo_rdys,dfifo_len
	const	dfifo_residue,0

	INCSTAT	hst_d_packets

	jmp	dfifo2
	 const	dfifo_state,DFIFOST_FLUSH

dfifo0f:
	;			else { /* THRESHOLD */

	.if DEBUG
	tbit	t0,dfifo_flags,DWOKI_THRESH
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	; threshold? Send up threhold minus a block
	; so we don't have to flush and we're guaranteed
	; to have something to send up if we get an EOP.
	; Plus, we don't have to do a flush because the
	; end of the shortened epoch is guaranteed to be
	; transferred.

	const	dfifo_residue,TPDRAM_BLKSIZE
	sub	dfifo_len,dfifo_len,dfifo_residue

	.if DEBUG
	jmpf	dfifo_len,.+12		; ASSERT( dfifo_len > BLKSIZE );
	 nop
	call	raddr,fatal
	 nop
	.endif


	jmp	dfifo2_parse
	 mov	dfifo_rdys,dfifo_len

	;			}
	;		}
	;	}
	;	break;

dfifo2:

	; case DST-FLUSH:
	; 	if ( not-guaranteed-end-of-chunk-is-transfered ) {

	HWREGP	t0,DST_BLK_ADD
	load	0,WORD,t2,t0			; find blockaddr
	add	t1,dst_in,dfifo_len		; word after last word
	cpge	t0,t1,dst_dram_end
	jmpf	t0,dfifo2aa
	 nop
	sub	t1,t1,dst_dram_size
dfifo2aa:
	constx	t0,(BLKADDR_MASK|BLKADDR_QSF)
	and	t1,t1,t0
	and	t2,t2,t0
	cpgt	t0,t2,t1
	jmpf	t0,dfifo2a
	 nop
	add	t1,t1,dst_dram_size
dfifo2a:
	sub	t0,t1,t2
	srl	t0,t0,BLKADDR_QSF_BT
	sub	t3, t0, 1
	cpltu	t0, t3, 4
;	cplt	t0,t0,4
	jmpf	t0,dfifo2c

	; 		Poke-FF-Bit;

	 HWREGP	t0,DST_FILL_FLUSH
	consth	t1,(DST_FF_SOT|DST_FF_AOT)		; XXX
	store	0,WORD,t1,t0

	jmp	dfifo9
	 nop

	; 	}
	; 	else {
dfifo2c:
	;		// fix up parity in word after last word.
	;		// this is so DMA won't crash if it's
	;		// rounded up a word.

	add	t1,dst_in,dfifo_len		; word after last word
	cpge	t0,t1,dst_dram_end
	jmpf	t0,dfifo2ca
	 nop
	sub	t1,t1,dst_dram_size
dfifo2ca:
	constx	t2,OFFS_NPARITY
	add	t1,t1,t2
	load	0,WORD,t0,t1
	sub	t1,t1,t2
	store	0,WORD,t0,t1

dfifo2_parse:

	; 		if ( beginning-of-packet )

	tbit	t0,state2,ST2_DST_NBOP
	jmpt	t0,dfifo2e
	 const	dfifo_hdrlen,0

	;		**************************
	;		*     Parse-Header;	 *
	;		**************************

	tbit	t0,state2,ST2_HIPPI_PH		; HIPPI-PH active?
	jmpt	t0,dfifo2e
	 constn	dfifo_ulp,-1
	load	0,WORD,t1,dst_in		; get FP header in t1
	srl	dfifo_ulp,t1,HIPPI_FP_ULP_SHIFT	; get HIPPI ULP
	const	t0,HIPPI_FP_D1SIZE_MASK
	and	t0,t0,t1			; get D1-area-size (in bytes)
	add	dfifo_hdrlen,t0,8		; hdrlen = sizeof(FP) + D1-size
	add	t2,dst_in,4			; get D2-size from FP hdr
	cpge	t0,t2,dst_dram_end
	jmpf	t0,dfifo2dh
	 cpeq	t0,dfifo_ulp,HIPPI_ULP_LE	; parse further if HIPPI-LE
	sub	t2,t2,dst_dram_size		; wrap t2?
dfifo2dh:
	load	0,WORD,dfifo_d2_size,t2
	jmpf	t0,dfifo2e			; parse further if HIPPI-LE...
	 add	t2,dst_in,dfifo_hdrlen		; t2 points at first of D2 area
	cpge	t0,t2,dst_dram_end		; wrap t2?
	jmpf	t0,dfifo2da
	 add	t0,dfifo_hdrlen,28		; t0=fp+d1+28 (snap+min IP)
	sub	t2,t2,dst_dram_size
dfifo2da:
	cpge	t0,dfifo_len,t0			; full snap & minimum IP hdr?
	jmpf	t0,dfifo2e			; bail-out if runt
	 const	t0,0xAAAA0300
	load	0,WORD,t1,t2			; check word 1 of SNAP
	consth	t0,0xAAAA0300
	cpeq	t0,t0,t1
	jmpf	t0,dfifo2di
	 add	t2,t2,4
	cpge	t0,t2,dst_dram_end		; wrap t2?
	jmpf	t0,dfifo2db
	 nop
	sub	t2,t2,dst_dram_size
dfifo2db:
	load	0,WORD,t1,t2
	constx	t0,0x00000800			; check word 2 of SNAP
	cpeq	t0,t0,t1
	jmpf	t0,dfifo2di
	 add	t2,t2,4
	cpge	t0,t2,dst_dram_end		; wrap t2?
	jmpf	t0,dfifo2dc
	 nop
	sub	t2,t2,dst_dram_size
dfifo2dc:
	load	0,WORD,t1,t2			; get first word of IP hdr

	 add	t2,t2,8
	cpge	t0,t2,dst_dram_end		; wrap t2?
	jmpf	t0,dfifo2de
	 nop
	sub	t2,t2,dst_dram_size
dfifo2de:
	srl	t0,t1,22
	and	t0,t0,0x3C			; header IP length

	add	dfifo_hdrlen,dfifo_hdrlen,8	; sizeof(SNAP)
	add	dfifo_hdrlen,dfifo_hdrlen,t0	; sizeof(IP hdr)

	load	0,WORD,t1,t2			; get IP proto
	srl	t1,t1,16
	and	t1,t1,0xFF

	add	t2,t2,t0			; get to TCP word 3 (if TCP)
	add	t2,t2,4				; (we already added 8)
						; check for wrap below...

	cpeq	t0,t1,IP_PROTO_TCP		; TCP?
	jmpt	t0,dfifo2dg
	 cpeq	t0,t1,IP_PROTO_UDP		; UDP?
	jmpf	t0,dfifo2di
	 nop

	; UDP case...
	jmp	dfifo2di
	 add	dfifo_hdrlen,dfifo_hdrlen,8	; sizoef(UDP header)

dfifo2dg:
	; TCP case...

	cpge	t0,t2,dst_dram_end		; wrap t2?
	jmpf	t0,dfifo2df
	 add	t0,dfifo_hdrlen,20
	sub	t2,t2,dst_dram_size
dfifo2df:
	cpge	t0,dfifo_len,t0			; make sure we have min TCP hdr
	jmpf	t0,dfifo2e
	 nop
	load	0,WORD,t1,t2			; get word 3 of TCP hdr
	srl	t0,t1,26			; get HLEN
	and	t0,t0,0x3C			; get header len
	add	dfifo_hdrlen,dfifo_hdrlen,t0

dfifo2di:

	.ifdef TAILFILL
	sub	t0,nbpp,1			; t0 = (NBPP-1)
	add	t1,dfifo_d2_size,32		; size = d2size + 32 (FP/LE)
	and	t0,t0,t1			; t0 = size & (NBPP-1)
	cpgt	t1,t0,dfifo_hdrlen		; don't use tailfill if:
	jmpf	t1,dfifo2e			; t0 <= dfifo_hdrlen
	 cpgt	t1,t0,mlen
	jmpt	t1,dfifo2e			; t0 > MLEN
	 and	t1,t0,3
	cpeq	t1,t1,0				; t0 & 3 != 0
	jmpf	t1,dfifo2e
	 nop
	mov	dfifo_hdrlen,t0
	.endif ; TAILFILL

dfifo2e:

	; 		DST-FIFO-State := DST-Avail;

	const	dfifo_state,DFIFOST_AVAIL

	srl	t0,state2,ST2_DST_NBOP_BT
	and	t0,t0,1
	sll	t0,t0,DFIFOFL_NBOP_BT
	or	dfifo_flags,dfifo_flags,t0

	; 		not-beginning-of-packet-flag = ( dfifo_flags & NEOP );

	tbit	t0,dfifo_flags,DWOKI_EOP
	jmpt	t0,dfifo2g
	 andn	state2,state2,ST2_DST_NBOP
	or	state2,state2,ST2_DST_NBOP
dfifo2g:

	; 	}
	; 	break;

dfifo3:

	; case DST-Avail:
	; case DST-Error:
	; 	// Do nothing until DST-DMA machine takes over from here.

	; 	break;
	; }

dfifo9:
	; Break:


	; // -------------------
	; // to-host interrupts
	; // ------------------
mlop7:
	tbit	t0,state,ST_DHINT
	jmpf	t0,mlop8
	 nop
	
	CHECK_TIMER	t0,hint_timer
	jmpf	t0,mlop8
	 nop

	or	state,state,(ST_HINT|ST_B2H_PUSH)
	andn	state,state,ST_DHINT

mlop8:
	jmp	mlop0
	 nop






; -----------------------------------------------
; Handle destination errors
; -----------------------------------------------

;	DWOKI_BAD_BITS,(DWOKI_AD_SEQ_ERR|DWOKI_AD_PARITY|DWOKI_AD_LLRC_ERR|DWOKI_AD_SYNC|DWOKI_AD_ILBRST|DWOKI_EONC|DWOKI_SDIC_LOSS)

dfifo_err:
	const	dfifo_errs,0			; start clean of errors

	tbit	t0,t1,DWOKI_AD_SEQ_ERR
	jmpf	t0,dfifo_err1a
	 nop
	INCSTAT	hst_d_seq_err
	or	dfifo_errs,dfifo_errs,B2H_IERR_SEQ
dfifo_err1a:
	tbit	t0,t1,DWOKI_AD_PARITY
	jmpf	t0,dfifo_err1b
	 nop
	INCSTAT	hst_d_par_err
	or	dfifo_errs,dfifo_errs,B2H_IERR_PARITY
dfifo_err1b:
	tbit	t0,t1,DWOKI_AD_LLRC_ERR
	jmpf	t0,dfifo_err1c
	 nop
	INCSTAT	hst_d_llrc
	or	dfifo_errs,dfifo_errs,B2H_IERR_LLRC
dfifo_err1c:
	tbit	t0,t1,DWOKI_AD_SYNC
	jmpf	t0,dfifo_err1d
	 nop
	INCSTAT	hst_d_sync
	or	dfifo_errs,dfifo_errs,B2H_IERR_SYNC
dfifo_err1d:
	tbit	t0,t1,DWOKI_AD_ILBRST
	jmpf	t0,dfifo_err1e
	 nop
	INCSTAT	hst_d_illbrst
	or	dfifo_errs,dfifo_errs,B2H_IERR_ILBURST
dfifo_err1e:
	tbit	t0,t1,DWOKI_SDIC_LOSS
	jmpf	t0,dfifo_err1f
	 nop
	INCSTAT	hst_d_sdic_lost
	or	dfifo_errs,dfifo_errs,B2H_IERR_SDIC
dfifo_err1f:
	tbit	t0,t1,DWOKI_EONC
	jmpf	t0,dfifo_err1g
	 nop
	INCSTAT	hst_d_nullconn
dfifo_err1g:

	add	dfifo_len,dfifo_len,dfifo_residue
	mov	dfifo_rdys,dfifo_len
	const	dfifo_residue,0

	.if 0
	HWREGP	t2,DST_STAT_BLOCK		; get a status block
	store	0,WORD,t2,t2
	.endif

	constx	t0,(DWOKI_AD_SEQ_ERR|DWOKI_AD_SYNC|DWOKI_AD_ILBRST|DWOKI_SDIC_LOSS)
	and	t0,t1,t0
	cpeq	t0,t0,0
	jmpt	t0,dfifo_err2
	 const	dfifo_state,DFIFOST_ERR

		; hold off connections until we drain destination FIFO
		; and can reset READY counters.

	or	state2,state2,ST2_DST_DRAIN
	HWREGP	t2,DST_CONNECT
	const	t0,DST_CONN_MAN			; "manual" mode
	jmp	dfifo_err2a
	 store	0,WORD,t0,t2			; (hold off connections)

dfifo_err2:
	tbit	t0,op_flags,OPF_NODISC
	jmpf	t0,dfifo_err2a
	 nop
	
	; Soft errors when OPF_NODISC is set.

	; ASSERT( error occurred on data );

	.if DEBUG
	and	t2,t1,DWOKI_DTYPE_MASK
	cpeq	t0,t2,DWOKI_DTYPE_DATA
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

		; state2.ST2_DST_NBOP --> dfifo_flags.DFIFOFL_NBOP

	srl	t0,state2,ST2_DST_NBOP_BT
	and	t0,t0,1
	sll	t0,t0,DFIFOFL_NBOP_BT
	or	dfifo_flags,dfifo_flags,t0

		; if ( I-field ) dfifo_rdys -= 4;

	srl	t0,t1,DWOKI_IFIELD_BT-2
	and	t0,t0,4
	sub	dfifo_rdys,dfifo_rdys,t0

		; ! woki.EOP --> state2.ST2_DST_FEOP

	.if DWOKI_EOP_BT >= ST2_DST_FEOP_BT
	srl	t0,t1,DWOKI_EOP_BT-ST2_DST_FEOP_BT
	.else
	sll	t0,t1,ST2_DST_FEOP_BT-DWOKI_EOP_BT
	.endif
	and	t0,t0,ST2_DST_FEOP
	xor	t0,t0,ST2_DST_FEOP
	or	state2,state2,t0

	jmp	dfifo9
	 nop


dfifo_err2a:
	and	t2,t1,DWOKI_DTYPE_MASK
	cpeq	t0,t2,DWOKI_DTYPE_DATA
	jmpt	t0,dfifo_err3
	 cpeq	t0,t2,DWOKI_DTYPE_SEQERR
	
	jmpf	t0,dfifo_err2b
	 const	dfifo_rdys,0			; not valid data

	jmp	dfifo_err4
	 mov	dst_seqeword_addr,dst_in

dfifo_err2b:
	cpeq	t0,t2,DWOKI_DTYPE_STATUS
	jmpf	t0,dfifo_err4
	 nop
	
	jmp	dfifo_err4
	 mov	dst_statblock,dst_in

dfifo_err3:
	tbit	t0,t1,DWOKI_IFIELD
	jmpf	t0,dfifo_err4
	 nop
	
	sub	dfifo_rdys,dfifo_rdys,4

dfifo_err4:

	srl	t0,state2,ST2_DST_NBOP_BT
	and	t0,t0,1
	sll	t0,t0,DFIFOFL_NBOP_BT
	or	dfifo_flags,dfifo_flags,t0

	jmp	dfifo9
	 andn	state2,state2,ST2_DST_NBOP


;--------------------------------------------
; Queue-B2H-DelayedInt()
;	destroys t0,v0
;--------------------------------------------

queue_b2h_di:

	tbit	t0,state,ST_DHINT
	jmpt	t0,queue_b2h
	 constx	t0,HINT_TM
	SET_TIMER	hint_timer,t0
	or	state,state,ST_DHINT

;--------------------------------------------
; Queue-B2H()
;	destroys t0,v0
; assumes:
; v0 = bottom 24 bits already setup for 1st word of b2h
; v1 = 2nd word of b2h
;--------------------------------------------

queue_b2h:
	sll	t0,b2h_sn,B2H_SN_SHIFT			; put in serial num
	add	b2h_sn,b2h_sn,1
	or	v0,v0,t0
	
	cpge	t0,b2h_queued,B2H_THRESHOLD		; set B2H_PUSH if
	srl	t0,t0,(31-ST_B2H_PUSH_BT)		; over threshold
	or	state,state,t0

	.if DEBUG
	constx	t0,B2H_B_SIZE				; make sure no ovflw
	cpge	t0,b2h_queued,t0
	jmpf	t0,.+16
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	constx	t0,b2h
	add	t0,t0,b2h_queued
	store	0,WORD,v0,t0
	add	b2h_queued,b2h_queued,8
	add	t0,t0,4
	jmpi	raddr
	 store	0,WORD,v1,t0


;----------------------------------------
; sched_bp_rd
;	DMA's from base+offset of host memory into local
;	memory data ring buffer. DMA wrapping is done by ATE hw.
;
;	Launches DMA for the D2 data region, then writes D1 region into buffer.
;
;input: 
;	bp_cur_sjob -- current source job number 
;	bp_s_desc_zeroth -- desc[3]
;	bp_s_desc_first -- desc[2]
;	bp_s_desc_second -- desc[2]
;	bp_s_desc_third -- desc[0]
;
;	trashes:
;	t0-t4, v0-v15
;	t5  = hold opcode
;	t6 = End of data buffer region
;	v14 = length in bytes of d2 section  (except in bulk_block case)
;	v15 = offset in bytes from host page index
;	
;uses initial values of:
;	src_in -- pointer to head of where DMA should stuff data.
;	
;affects registers:
;	dma_client -- sets it to DMA_CLIENT_SRC
;	src_in  -- sets it to one word past the tail of the incoming DMA.
;	bp_cp_d_data_base(lo/hi) -- set to host address to start dma at.
;
;;affects local memory:	
;	p_dma_host_lo	
;	p_dma_host_hi
;	p_dma_host_ctl
;----------------------------------------


; Schedule-SRC-BP-Packet()
; {

sched_bp_rd:
;       // start a data transfer for the D2 section.
;       // create i-field, fp, and D1 section from source descriptor
;       // In general, check only source descriptor fields, not dest.
;       // want to pass all unused fields through to packet so that
;       // users can use unused fields as payload.


;       if( desc.src_hostx >= MAX_HOSTX)
;               goto drop_pkt_hostx;
	constx	t1, BP_DESC_HOSTX_MASK
	and     t0, bp_s_desc_second, t1
	srl	t0, t0, BP_DESC_HOSTX_SHIFT
	cpge	t1, t0, BP_MAX_HOSTX
	jmpt	t1, sched_bp_rd_drop_pkt_ifield

;       if(X == 1 || G == 1) // don't support GET or X-rated mode
;               goto drop_pkt_bad_opcode;
	  tbit	t0, bp_s_desc_third, BP_OP_X
	tbit	t1, bp_s_desc_third, BP_OP_G
	or	t0, t0, t1
	jmpt	t0, sched_bp_rd_drop_pkt_opcode

;       if(desc.Z != 0)
;               goto drop_pkt_bad_opcode;
	  tbit	t0, bp_s_desc_third, BP_OP_Z
	jmpt	t0, sched_bp_rd_drop_pkt_opcode

;       if(desc.d != 0)
;               goto drop_pkt_bad_opcode;
	  tbit	t0, bp_s_desc_third, BP_OP_d
	jmpt	t0, sched_bp_rd_drop_pkt_opcode



;       // Done with basic descriptor sanity checking

;       load_job_data(cur_sjob) -> Job->bufx_size;
	  sll	t1, bp_cur_sjob, BP_JOB_SIZE_POW2
	add	t4, bp_job_structs, t1
	add	t5, t4, (bp_cj_bufx_entry_size-bp_job_reg_base)*4
	load	0, WORD, bp_cj_bufx_entry_size, t5


;       host_length = desc.length<<2;
	constx        t0, BP_DESC_LENGTH_MASK
	and	v14, bp_s_desc_third, t0
	sll	v14, v14, BP_W_TO_BYTES


;       if(desc.S == 0 && host_length == 0)
;               goto drop_pkt_bad_len;
	tbit	t1, bp_s_desc_third, BP_OP_S
	nand	t1, t1, t1
	cpeq	t2, v14, zero
	and	t0, t1, t2
	jmpt	t0, sched_bp_rd_drop_pkt_addr
	



bp_s_check_payload_size:
;       if (host_length != 0) {
	cpneq	t0, v14, zero
	jmpf	t0, bp_s_past_dma_payload

bp_s_non_zero_len:
;               if(desc.src_bufx >= MAX_BUFX)
;                       goto drop_pkt_bad_src_bufx;
	  constx	t1, BP_DESC_SRC_BUFX_MASK
	and	bp_cp_data_bufx, bp_s_desc_first, t1
	srl	bp_cp_data_bufx, bp_cp_data_bufx, BP_DESC_SRC_BUFX_SHIFT
	constx	t2, BP_SFM_ENTRIES
	cpge	t0, bp_cp_data_bufx, t2
	jmpt	t0, sched_bp_rd_drop_pkt_bufx

;               host_offset = desc.src_offset<<2;  // convert to bytes;
	  constx	t0, BP_DESC_SRC_OFF_MASK
	and	v15, bp_s_desc_first, t0
	sll	v15, v15, BP_W_TO_BYTES

;       // process length
;               if (desc.S == 0)  { // bulk move
	tbit	t0, bp_s_desc_third, BP_OP_S
	jmpt	t0, bp_check_off_len
	  nop
;                       host_length = Job->bufx_size;
	mov	v14, bp_cj_bufx_entry_size
;                       if (desc.length == 0) //  zero microblocks is not allowed.
;                               goto drop_pkt_bad_len_off;

	cpeq	t0, v14, zero
	jmpt	t0, sched_bp_rd_drop_pkt_addr
;               }


bp_check_off_len:
;       if (host_length << 29) // must be long word aligned
;               goto drop_pkt_len;
	sll	t0, v14, W_ALIGN_CHECK
	jmpt	t0, sched_bp_rd_drop_pkt_addr


;       if(S == 0) { // multi-pkt, can't move more than one bufx size of data
	  tbit 	t0, bp_s_desc_third, BP_OP_S
	jmpt	t0, dma_buf_addr_calc
;          if(host_length > Job->bufx_size)
;              goto drop_pkt_length;
	cpgt	t0, v14, bp_cj_bufx_entry_size
	jmpt	t0, sched_bp_rd_drop_pkt_addr
;       }


	;;--------------------------
	;;   t5 contains aux_host_length
	;;   t4 contains buf_addr for priming the ATE
	;;-----------------------------

dma_buf_addr_calc:
;               buf_addr = src_in + BP_PKT_HDR_LEN;
	  add	t4, src_in, BP_PKT_HEADER_LEN	



;               if (host_offset<<29) {  // odd word offset
	  sll	t0, v15, W_ALIGN_CHECK
	jmpf	t0, bp_lw_aligned_offset
	  nop
;                       aux_host_length = host_length + 8; // copy extra word at beginning and end
	add	t5, v14, 8
;                       host_offset -= 4; // long word align
	sub	v15, v15, 4
;                       buf_addr -= 4;  // overwrite the padding word
	sub	t4, t4, 4
	jmp	check_buff_wrap
	  nop
;               }
;               else   {  // even_word offset
;              	        aux_host_length = host_length;
bp_lw_aligned_offset:
	mov	t5, v14
;               }

;               if (buf_addr >= SRC_LOW + SRC_BUF_SZ)
;                       buf_addr = buf_addr - data_buffer_size + 4;
check_buff_wrap:
	constx	t6, SRC_LOWADDR + SRC_BUF_SIZE
	add	t0, t4, 4
	cpge	t0, t0, t6
	jmpf	t0, bp_s_start_dma
	  constx	t1, SRC_BUF_SIZE
	sub	t4, t4, t1

	;;--------------------------
	;;   t2 will contain 1st dma length; t3 will contain 2nd dma-length
	;;-----------------------------

bp_s_start_dma:
;               /// start DMA -- this assumes DMA engine has 2 deep instruction pipe,
;               /// so launch 2nd dma immediately
;               2nd_dma_length = host_offset + aux_host_length - Job->bufx_size;
	add	t0, v15, t5
	sub	t3, t0, bp_cj_bufx_entry_size

;               if (2nd_dma_length > 0) {
	cpgt	t0, t3, zero
	jmpf	t0, bp_s_no_second_dma
	  nop
;                         1st_dma_length = aux_host_length - 2nd_dma_length;
	sub	t2, t5, t3
;                       if ( (desc.src_bufx+1) > MAX_BUFX)
;                               goto drop_pkt_bad_bufx
	add	t0, bp_cp_data_bufx, 1
	constx	t1, BP_SFM_ENTRIES
	cpgt	t0, t0, t1
	jmpt	t0, sched_bp_rd_drop_pkt_bufx
	  nop

;                       store_dma_status(ENABLE_DATA, SRC, 2, Port->bufx, Port->job);
        constx  t0, DMA_ACTIVE_DATA_SRC
	constx	t1, DMA_STATUS_2_PG_DMA
	or	t0, t0, t1
        and     t1, bp_cur_sjob, DMA_STATUS_JOB_MASK
        sll     t1, t1, DMA_STATUS_JOB_SHIFT
        or      t0, t0, t1
        ;; bp_cp_data_bufx should already be in lower DMA_STATUS_PORT_PGX_MASK bits
        constx  t1, DMA_STATUS_PORT_PGX_MASK
        and     t1, bp_cp_data_bufx, t1
        or      t0, t0, t1
        ; store it in the dma_status word
        add     t1, bp_config, dma_status
        store   0, WORD, t0, t1
	jmp	past_store_dma_status
	  nop
;               }


;               else  {	    // no 2nd DMA: 2nd_dma_len would be set -ve
bp_s_no_second_dma:
;                         1st_dma_length = aux_host_length;
	mov	t2, t5

;                         2nd_dma_length = 0; 
	mov	t3, zero

;                       store_dma_status(ENABLE_DATA, SRC, 1, Port->bufx, Port->job);
        constx  t0, DMA_ACTIVE_DATA_SRC
        and     t1, bp_cur_sjob, DMA_STATUS_JOB_MASK
        sll     t1, t1, DMA_STATUS_JOB_SHIFT
        or      t0, t0, t1
        ;; bp_cp_data_bufx should already be in lower DMA_STATUS_PORT_PGX_MASK bits
        constx  t1, DMA_STATUS_PORT_PGX_MASK
        and     t1, bp_cp_data_bufx, t1
        or      t0, t0, t1
        ; store it in the dma_status word
        add     t1, bp_config, dma_status
        store   0, WORD, t0, t1
;               }


past_store_dma_status:
;               host_base_ptr = dst_freemap[desc.src_bufx] << convert_to_pow2(Job->bufx_size);
;               // addr = bp_sfreemap + bp_cur_sjob*2^BP_SFM_SIZE_POW2 + bp_cp_data_bufx*4
;               // then shift it because it's in 16 KB granularity
	constx	t0, bp_sfreemap
	sll	t1, bp_cur_sjob, BP_SFM_SIZE_POW2
	add	t0, t0, t1
	sll	t1, bp_cp_data_bufx, 2	  	
	add	t0, t0, t1
	load	0, WORD, t0, t0

	sll	bp_cp_d_data_base_lo, t0, BP_FM_SHIFT_LO
	srl	bp_cp_d_data_base_hi, t0, BP_FM_SHIFT_HI


	;;--------------------------
	;;   t5 is now free
	;;-----------------------------


;               store_host_dma_addr(host_offset+host_base_ptr);
sched_bp_rd_store_dma_addr:
	store	0,WORD,bp_cp_d_data_base_hi,p_dma_host_hi  
	add	bp_cp_d_data_base_lo, bp_cp_d_data_base_lo, v15
	store	0,WORD,bp_cp_d_data_base_lo,p_dma_host_lo

;               SETUP_ATE_ENGINES(buf_addr);
        constx  t0,OFFS_XFER
        add     t0, t4, t0
        tbit    t1,t0,BLKADDR_QSF
        jmpt    t1,sched_bp_rd0a
          constx  t1,TPDRAM_BLKSIZE
        sub     t0,t0,t1

	;;--------------------------
	;;   t4 is now free
	;;-----------------------------


sched_bp_rd0a:
        constx  t1,XFER_MWTB
        store   0,WORD,t1,t0                    ; XFER, rdy to read from host

;               store_dma_len&GO(host_length);
        mov     t0, t2
        consth  t0, (DMAC_GO|DMAC_RD)
        store   0,WORD,t0,p_dma_host_ctl        ; GO!

;               if (2nd_dma_length > 0) {
        cple    t0, t3, zero
	jmpt    t0, bp_s_past_dma_payload
	  nop

;                       host_base_ptr =
;                               dst_freemap[desc.src_bufx+1] << convert_to_pow2(Job->bufx_size);
;               // addr = bp_sfreemap + bp_cur_sjob*2^BP_SFM_SIZE_POW2 + (bp_cp_data_bufx+1)*4
;               // then shift it because it's in 16 KB granularity
          constx        t0, bp_sfreemap
        sll     t1, bp_cur_sjob, BP_SFM_SIZE_POW2
        add     t0, t0, t1
	add	t4, bp_cp_data_bufx, 1
        sll     t1, t4, 2
        add     t0, t0, t1
        load    0, WORD, t0, t0

        sll     bp_dbl_dma_d_data_base_lo, t0, BP_FM_SHIFT_LO
        srl     bp_dbl_dma_d_data_base_hi, t0, BP_FM_SHIFT_HI

;                       while(!ok_to_write_2nd_dma_commmand) {};
sched_bp_rd_test_DCBF:
        load  0, WORD, t5, p_dma_host_ctl
        tbit    t5, t5, DMAC_DCBF
        jmpt    t5, sched_bp_rd_test_DCBF

;                       store_host_dma_addr(host_base_ptr); // always zero offset
	store   0, WORD, bp_dbl_dma_d_data_base_hi, p_dma_host_hi
	store   0, WORD, bp_dbl_dma_d_data_base_lo, p_dma_host_lo

;                       store_dma_len&GO(host_length);
	mov	t1, t3
	consth  t1, (DMAC_GO|DMAC_RD)
	store   0, WORD, t1, p_dma_host_ctl        ; GO!
;               }
;        }


bp_s_past_dma_payload:
; // setup D1 header
;       sdma_dn_bp_pkt_len = host_length; // store for later increment of statistics
	mov	curr_src_bp_pkt_len, v14

;       pkt_hdr = (uint*)src_in;
;
;       // Packet format is:
;       // 0 = dummy word
;       // 1 = Ifield
;       // 2 = FP0
;       // 3 = FP1
;       // 4 = HDR0
;       // 5 = HDR1
;       // 6 = HDR2 = seqnum
;       // 7 = HDR3 = src_bufx
;       // 8 = HDR4 = src_offset
;       // 9 = HDR5 = dst_bufx
;       // 10 = HDR6 = dst_offset
;       // 11 = HDR7 = auth[0]
;       // 12 = HDR8 = auth[1]
;       // 13 = HDR9 = auth[2]
;       // 14 = HDR10 = checksum
;       // 15 = HDR11 = dummy word

;       pkt_hdr[dummy] = 0;
	mov	%%(v0+BP_PKT_DUMMY/4),zero

;       pkt_hdr[Ifield] = host_table[desc.hostx];
	constx	t1, BP_DESC_HOSTX_MASK
        and     t0, bp_s_desc_second, t1
	srl	t0, t0, BP_DESC_HOSTX_SHIFT
	;; no need to mask, since was at MSB 
        sll     t0, t0, 2                          ; change to bytes from words
        constx  t1, bp_hostx
	sll     t4, bp_cur_sjob, BP_MAX_HOSTX_POW2 + 2  ; need bytes from words
	add     t1, t4, t1
	add     t0, t1, t0
        load    0,WORD, %%(v0+BP_PKT_IFIELD/4), t0

;       // FP0_BASE sets p=1, b=0, d2_offset = 0, d1_size = 6 long words
;       pkt_hdr[FP0] = BYPASS_FP0_BASE | bp_ulp;
        sll     t0, bp_state, HIPPI_FP_ULP_SHIFT
        constx  t1, HIP_BP_FP_BASE
        or      %%(v0+BP_PKT_FP_HI/4), t0, t1

;       pkt_hdr[FP1] = host_length;
	mov	%%(v0+BP_PKT_FP_LO/4), v14


;       load_job_info(cur_sjob) -> sdq_base, sdq_end, src_hostport, auth[2:0];
        sll     t1, bp_cur_sjob, BP_JOB_SIZE_POW2
        add     t4, bp_job_structs, t1
        mtsrim  CR,(bp_cj_auth2 - bp_cj_sdq_base)
        add     t5, t4, (bp_cj_sdq_base-bp_job_reg_base)*4
        loadm   0,WORD,bp_cj_sdq_base,t5

	
	
;       tmp = desc[3] & DESC_OPCODE_MASK;
	constx	t1, BP_PKT_OPCODE_MASK
	and	t0, bp_s_desc_third, t1   ; note that this makes the vers# 0

;       tmp |= (BP_VERSION << BP_PKT_VERSION_SHIFT)
	constx	t1, BP_VERSION
	sll	t1, t1, BP_PKT_VERSION_SHIFT
	or	t0, t0, t1

;       tmp |= Job->src_hostport;
	srl	t1, bp_cj_ack_hostport, BP_ACK_HOST_SHIFT
	and	t1, t1, BP_ACK_HOST_MASK
	sll	t1, t1, BP_PKT_SRC_HOSTX_SHIFT
	or	t0, t0, t1
	constx	t4, BP_ACK_PORT_MASK
	and	t1, bp_cj_ack_hostport, t4
	or	t0, t0, t1
	
;       pkt_hdr[HDR0] = tmp;
	mov	%%(v0+BP_PKT_FIRST_WORD/4), t0

;       pkt_hdr[HDR1] = desc[2];
	mov     %%(v0+BP_PKT_SECOND_WORD/4), bp_s_desc_second

;       pkt_hdr[HDR2] = desc[3];
	mov     %%(v0+BP_PKT_THIRD_WORD/4), bp_s_desc_third

;       pkt_hdr[HDR3] = desc[1]>>16;
	srl	%%(v0+BP_PKT_SRC_BUFX/4), bp_s_desc_first, BP_DESC_SRC_BUFX_SHIFT
	
;       pkt_hdr[HDR4] = desc[1] & SRC_OFFSET_MASK;
	constx	t0, BP_DESC_SRC_OFF_MASK
	and	%%(v0+BP_PKT_SRC_OFF/4), bp_s_desc_first, t0
	
;       pkt_hdr[HDR5] = desc[0]>>16;
 	srl	%%(v0+BP_PKT_DST_BUFX/4), bp_s_desc_zeroth, BP_DESC_DST_BUFX_SHIFT

;       pkt_hdr[HDR6] = desc[0] & DST_OFFSET_MASK;
	constx	t0, BP_DESC_DST_OFF_MASK
	and     %%(v0+BP_PKT_DST_OFF/4), bp_s_desc_zeroth, t0

;       pkt_hdr[HDR7] = Job->auth[0];
	mov	%%(v0+BP_PKT_AUTH0/4), bp_cj_auth0

;       pkt_hdr[HDR8] = Job->auth[1];
	mov	%%(v0+BP_PKT_AUTH1/4), bp_cj_auth1

;       pkt_hdr[HDR9] = Job->auth[2];
	mov	%%(v0+BP_PKT_AUTH2/4), bp_cj_auth2

;       pkt_hdr[HDR10] = 0; // don't set checksum
	mov     %%(v0+BP_PKT_CHKSUM/4),zero

;       pkt_hdr[HDR11] = 0;     // dummy word to allow long word dma's
;                               // of odd word aligned data
	mov     %%(v0+BP_PKT_PADDING/4),zero

	constx	t4, SRC_LOWADDR + SRC_BUF_SIZE


;               store_pkt_hdr_with_wrap(src_in+BP_HDR_SIZE) <- pkt_hdr;

	mov	t6, src_in
	store	0,WORD,v0,t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w1
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w1:	
	store	0,WORD, v1, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w2
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w2:	
	store	0,WORD, v2, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w3
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w3:	
	store	0,WORD, v3, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w4
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w4:	
	store	0,WORD, v4, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w5
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w5:	
	store	0,WORD, v5, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w6
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w6:	
	store	0,WORD, v6, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w7
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w7:	
	store	0,WORD, v7, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w8
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w8:	
	store	0,WORD, v8, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w9
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w9:	
	store	0,WORD, v9, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w10
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w10:	
	store	0,WORD, v10, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w11	
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w11:	
	store	0,WORD, v11, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w12	
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w12:	
	store	0,WORD, v12, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w13	
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w13:	
	store	0,WORD, v13, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w14	
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w14:	
	store	0,WORD, v14, t6
	add	t6, t6, 4
	cpge	t0, t6, t4
	jmpf	t0, sched_bp_rd_w15	
	 nop
	constx	t6, SRC_LOWADDR

sched_bp_rd_w15:	
 	store	0,WORD, v15, t6

 
sched_bp_rd_d1_dn:		

; // store back descriptor info
;
;        if(S == 0 && desc.length != 1) {
	tbit	t0, bp_s_desc_third, BP_OP_S
	nand	t0, t0, t0
	constx	t1, BP_DESC_LENGTH_MASK
	and	t1, bp_s_desc_third, t1
	cpneq	t1, t1, 1
	and	t0, t0, t1
	jmpf	t0, single_or_last_ublock	
	  nop
	
;                desc.length--;
	sub	bp_s_desc_third, bp_s_desc_third, 1

;                desc.opcode.F = 0;
	constx	t0, 1
	sll	t0, t0, BP_OP_F_BT
	nor	t0, t0, t0
	and	bp_s_desc_third, bp_s_desc_third,  t0

;                desc.src_bufx++;
	constx	t0, BP_DESC_SRC_BUFX_INCREMENT
	add	bp_s_desc_first, bp_s_desc_first, t0
	
;                desc.dst_bufx++;
	constx	t0, BP_DESC_DST_BUFX_INCREMENT
	add	bp_s_desc_zeroth, bp_s_desc_zeroth, t0

;                store_desc(Job->bp_sdqp) <- desc[0],desc[1],desc[3];

	sub	t0, bp_cj_sdq_head, FIRST_WORD_OFFSET
	
	store	0, WORD, bp_s_desc_zeroth, t0
	add	t0, t0, 4
	store	0, WORD, bp_s_desc_first, t0
	add	t0, t0, 4
	store	0, WORD, bp_s_desc_second, t0
	add	t0, t0, 4
	store	0, WORD, bp_s_desc_third, t0
	
	jmp	store_src_in
	  nop
;        }


;       else {  // single packet mode or last of multi-packet microblocks
single_or_last_ublock:
;               *(Job->bp_sdqp[3]) = -1;
	constx	t0, HIP_BP_DQ_INV
	store	0, WORD, t0, bp_cj_sdq_head	

;               inc_status(desc_consumed);
	BPINCSTAT       hst_s_bp_descs

;               Job->bp_sdqp += SIZE_OF_BP_DESC;
	add     bp_cj_sdq_head, bp_cj_sdq_head, BP_DESC_SIZE

;               if ( Job->bp_sdqp >= Job->bp_sdq_end)
;                       Job->bp_sdqp -= SDQ_SIZE;
        cpge    t0, bp_cj_sdq_head, bp_cj_sdq_end
	jmpf    t0, store_sdq_head
	  add   t0, bp_cj_sdq_base, BP_DESC_SIZE
	sub   bp_cj_sdq_head, t0, 4

;               store_job(cur_sjob) <- Job->bp_sdqp;
store_sdq_head:
        sll     t1, bp_cur_sjob, BP_JOB_SIZE_POW2
        add     t0, bp_job_structs, t1
        add     t5, t0, (bp_cj_sdq_head-bp_job_reg_base)*4
        store   0, WORD, bp_cj_sdq_head, t5             ; save head of descriptor queue

;       }




;          // don't wrap src_in until after dma is complete
store_src_in:
;          src_in += BP_PKT_HDR + sdma_dn_bp_pkt_len;
	add	src_in, src_in, BP_PKT_HEADER_LEN
	add	src_in, src_in, curr_src_bp_pkt_len	;; length of the pkt that has gone out
;         DMA-Client := SRC;
        const   dma_client,DMA_CLIENT_SRC

;         SRC-DMA_State = SRC-BP-ACTIVE;
        const   sdma_state,SDMAST_BP_ACTIVE
	jmpi	raddr
	  nop



sched_bp_rd_drop_pkt_ifield:
	BPINCSTAT	hst_s_bp_desc_ifield_err
	jmp	sched_bp_rd_drop_pkt
	nop

	
sched_bp_rd_drop_pkt_bufx:
	BPINCSTAT	hst_s_bp_desc_bufx_err
	jmp	sched_bp_rd_drop_pkt
	nop

	
sched_bp_rd_drop_pkt_opcode:
	BPINCSTAT	hst_s_bp_desc_opcode_err
	jmp	sched_bp_rd_drop_pkt
	nop

	
sched_bp_rd_drop_pkt_addr:
	BPINCSTAT	hst_s_bp_desc_addr_err
	jmp	sched_bp_rd_drop_pkt
	nop


sched_bp_rd_drop_pkt:	
;               *(Job->bp_sdqp[3]) = -1;
	constx	t0, HIP_BP_DQ_INV
	store	0, WORD, t0, bp_cj_sdq_head	

;       load_job_info(cur_sjob) -> sdq_base, sdq_end;
        sll     t1, bp_cur_sjob, BP_JOB_SIZE_POW2
        add     t4, bp_job_structs, t1
        mtsrim  CR,(bp_cj_sdq_end - bp_cj_sdq_base)
        add     t5, t4, (bp_cj_sdq_base-bp_job_reg_base)*4
        loadm   0,WORD,bp_cj_sdq_base,t5


;               Job->bp_sdqp += SIZE_OF_BP_DESC;
	add     bp_cj_sdq_head, bp_cj_sdq_head, BP_DESC_SIZE

;               if ( Job->bp_sdqp >= Job->bp_sdq_end)
;                       Job->bp_sdqp -= SDQ_SIZE;
        cpge    t0, bp_cj_sdq_head, bp_cj_sdq_end
	jmpf    t0, store_sdq_head
	  add   t0, bp_cj_sdq_base, BP_DESC_SIZE
	sub   bp_cj_sdq_head, t0, 4

;               store_job(cur_sjob) <- Job->bp_sdqp;
        sll     t1, bp_cur_sjob, BP_JOB_SIZE_POW2
        add     t0, bp_job_structs, t1
        add     t5, t0, (bp_cj_sdq_head-bp_job_reg_base)*4
        store   0, WORD, bp_cj_sdq_head, t5             ; save head of descriptor queue

	; 			SRC-DMA-state = SDMAST_IDLE
	BPINCSTAT	hst_s_bp_packets
	add	src_wdog_bp_pkts, src_wdog_bp_pkts, 1

	BP_RESET_DMA_STATUS	; tell the driver that no DMA is going on
	const	sdma_state,SDMAST_IDLE
	jmpi	raddr
	const	dma_client,DMA_CLIENT_NONE


	
;----------------------------------------
; sched_d2b_rd
;	DMA's from base of host memory d2b + offset into local
;	memory d2b base + offset. The later is just d2b_valid,
;	the host offset = d2b_valid - &d2b[0], the base address 
;	is hostp_d2b
;	
;	
;uses initial values of:
;	v0 == amount to DMA, in bytes
;		if enough room without wrapping, get a cacheline
;		else get what's left.
;		
;	d2b_valid -- pointer to head of where DMA should stuff data.
;	hostp_d2b -- physical address of base of d2b region
;	
;affects registers:
;	dma_client -- sets it to DMA_CLIENT_SRC
;	d2b_valid -- points to end of new DMA
;
;;affects local memory:	
;	p_dma_host_lo	
;	p_dma_host_hi
;	p_dma_host_ctl
;		
;
;----------------------------------------
sched_d2b_rd:
	const	dma_client,DMA_CLIENT_SRC

	constx	t0,OFFS_XFER			; dma starting at d2b_valid
	add	t0,d2b_valid,t0
	tbit	t2,t0,BLKADDR_QSF
	jmpt	t2,sched_d2b_rd0a
	 constx	t2,TPDRAM_BLKSIZE
	sub	t0,t0,t2
sched_d2b_rd0a:
	constx	t2,XFER_MWTB
	store	0,WORD,t2,t0			; XFER, rdy to read from host

	constx	t1,hostp_d2b+4
	load	0,WORD,t2,t1
	sub	t1,t1,4
	load	0,WORD,t1,t1			; <t1,t2> = host address
	constx	t0,d2b
	sub	t0,d2b_valid,t0			; t0 = d2b_valid - &d2b[0]
	add	t2,t2,t0
	store	0,WORD,t2,p_dma_host_lo
	addc	t1,t1,0
	store	0,WORD,t1,p_dma_host_hi		; host address of d2b_valid

	mov	t1,d2b_valid			; save...

	add	d2b_valid,d2b_valid,v0		; d2b_valid += v0
	add	d2b_valid,d2b_valid,(NBPCL-1)
	andn	d2b_valid,d2b_valid,(NBPCL-1)	; round up to next cacheline
	cpge	t0,d2b_valid,d2b_lim		; past end of d2b? truncate
	jmpf	t0,sched_d2b_rd1
	 nop
	mov	d2b_valid,d2b_lim		; DMA to end
sched_d2b_rd1:

	sub	t1,d2b_valid,t1			; length of dma
	consth	t1,(DMAC_NPREF|DMAC_GO|DMAC_RD)
	jmpi	raddr
	 store	0,WORD,t1,p_dma_host_ctl	; GO!


;-------------------------------------------
; write_b2h
;-------------------------------------------

write_b2h:

	cpgt	t0,b2h_queued,0
	jmpf	t0,write_b2h_4

	 constx	t0,(b2h+OFFS_XFER)
	constx	t1,XFER_RDA
	store	0,WORD,t1,t0			; Xfer cycle to write b2h
	CK ( (b2h & BLKADDR_QSF) == 0 )

	add	t2,b2h_h_offs,b2h_queued
	cpgt	t2,t2,b2h_h_lim
	jmpf	t2,write_b2h_2			; jmp if not wrapping
	 nop
	
	; Must DMA B2H in two parts...

	constx	t0,hostp_b2h
	load	0,WORD,t1,t0
	add	t0,t0,4
	load	0,WORD,t2,t0			; get host address in <t1,t2>
	add	t2,t2,b2h_h_offs
	store	0,WORD,t2,p_dma_host_lo
	addc	t1,t1,0
	store	0,WORD,t1,p_dma_host_hi
	sub	t0,b2h_h_lim,b2h_h_offs
	cpeq	t1,t0,0
	jmpt	t1,write_b2h_0
	 sub	b2h_queued,b2h_queued,t0	; adjust b2h_queued
	consth	t0,(DMAC_NPREF|DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl	; GO!
write_b2h_0:
	const	b2h_h_offs,0

	; spin waiting for DMA command buffer to empty
write_b2h_1:	
	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_DCBF
	jmpt	t0,write_b2h_1
	 nop
	
	; drop down here and finish rest...TAP point is in proper place.

write_b2h_2:
	constx	t0,hostp_b2h
	load	0,WORD,t1,t0
	add	t0,t0,4
	load	0,WORD,t2,t0			; get host address in <t1,t2>
	add	t2,t2,b2h_h_offs
	store	0,WORD,t2,p_dma_host_lo
	addc	t1,t1,0
	store	0,WORD,t1,p_dma_host_hi
	add	b2h_h_offs,b2h_h_offs,b2h_queued
	consth	b2h_queued,(DMAC_NPREF|DMAC_GO|DMAC_WR)
	store	0,WORD,b2h_queued,p_dma_host_ctl
	const	b2h_queued,0

	; spin waiting for this DMA to complete...
write_b2h_3:
	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_NOTDN
	jmpt	t0,write_b2h_3
	 nop
	
write_b2h_4:
	tbit	t0,state,ST_HINT
	jmpf	t0,write_b2h_9
	
	 HWREGP	t1,BD_CTL
	const	t0,BDCTL_HOST_INT
	store	0,WORD,t0,t1
	andn	state,state,(ST_HINT|ST_DHINT)

write_b2h_9:
	jmpi	raddr
	 andn	state,state,ST_B2H_PUSH


;; -------------------------------------------
;; 
;; sched_nxt_src
;;
;;
;; ---------------------------------------------

sched_nxt_src:
	;	// Start or continue packet!

	const	dma_client,DMA_CLIENT_SRC

	;	if ( d2b_chunks == 0 ) {
	cpeq	t0,d2b_chunks,0
	jmpf	t0,sched_nxt_src2
	 load	0,WORD,t1,d2b_nxt			; t1 = *d2b_nxt
	
	;	// start as (opposed to continue) packet

	; 	// get flags and length from head
	; 	d2b_chunks = d2b_nxt->hd.chunks;
	; 	d2b_flags = d2b_nxt->hd.flags;

	.if DEBUG
	tbit	t0,t1,D2B_RDY
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	mov	d2b_flags,t1			; get flags and chunks
	srl	d2b_chunks,d2b_flags,D2B_CHUNKS_SHIFT
	cpeq	t0,d2b_chunks,0
	jmpf	t0,sched_nxt_src1
	 nop

	; 	zero length D2B...  pretend we've dma'ed zero length.
	; 	DMA done will get executed and stuff zero length WOKI
	; 	which will have the effect we need.  (XXX: I hope.)

	const	d2b_blkchunks,0
	store	0,WORD,src_in,src_in		; put good (parity) word here.

	;	d2b_nxt->??.addr = (src_in-SRC_DRAM);
	;	d2b_nxt++;

	add	d2b_nxt,d2b_nxt,4
	sll	t0,src_in,(31-DRAM_SIZE_BT)
	srl	t0,t0,(31-DRAM_SIZE_BT)
	store	0,WORD,t0,d2b_nxt
	add	d2b_nxt,d2b_nxt,4
	cpge	t0,d2b_nxt,d2b_lim
	jmpf	t0,sched_nxt_src9		; wrap d2b_nxt?
	 mov	src_dmaed,src_in
	
	const	d2b_nxt,d2b
	jmp	sched_nxt_src9
	 consth	d2b_nxt,d2b
	
sched_nxt_src1:
	;	// save start (offset) address in d2b, used in
	;	// recovery.

	;	d2b_nxt->??.addr = (src_in-SRC_DRAM);
	;	d2b_nxt++;

	add	d2b_nxt,d2b_nxt,4
	load	0,WORD,d2b_word1,d2b_nxt		; gotta save this...
	sll	t0,src_in,(31-DRAM_SIZE_BT)
	srl	t0,t0,(31-DRAM_SIZE_BT)
	store	0,WORD,t0,d2b_nxt
	add	d2b_nxt,d2b_nxt,4
	cpge	t0,d2b_nxt,d2b_lim
	jmpf	t0,sched_nxt_src2
	 mov	src_dmaed,src_in
	
	constx	d2b_nxt,d2b

	;	}
sched_nxt_src2:

	; d2b_blklen = MIN( src_d2b_blksz, d2b_chunks );

	cpgt	t0,d2b_chunks,src_d2b_blksz
	jmpf	t0,sched_nxt_src3
	 mov	d2b_blkchunks,d2b_chunks
	mov	d2b_blkchunks,src_d2b_blksz
sched_nxt_src3:
	
	; d2b_chunks -= d2b_blkchunks;

	sub	d2b_chunks,d2b_chunks,d2b_blkchunks

	; Schedule-First-Src-DMA-Chunk();
	; d2b_nxt++;
	; d2b_blkchunks--;

	.if DEBUG
	sll	t0,src_in,29		; ASSERT( src_in on 64-bit boundary );
	jmpf	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	constx	t1,OFFS_XFER
	add	t1,t1,src_in
	tbit	t0,t1,BLKADDR_QSF
	jmpt	t0,sched_nxt_src3a
	 constx	t0,TPDRAM_BLKSIZE
	sub	t1,t1,t0
sched_nxt_src3a:
	constx	t0,XFER_MWTB
	store	0,WORD,t0,t1			; do transfer cycles

	load	0,WORD,t1,d2b_nxt		; get first chunk
	add	d2b_nxt,d2b_nxt,4
	load	0,WORD,t2,d2b_nxt
	and	t3,t1,255
	store	0,WORD,t3,p_dma_host_hi
	add	d2b_nxt,d2b_nxt,4		; d2b_nxt++;

	and	t0,t2,4				; if not 64-bit aligned.
	srl	t1,t1,16			; t1 = chunk length
	add	t1,t1,t0			; not 64-bit aligned? len += 4,
	sub	t2,t2,t0			; addr -= 4.

	store	0,WORD,t2,p_dma_host_lo

	constx	t3,D2B_FN64ALIGN		; XXX: more alignment cruft!
	andn	d2b_flags,d2b_flags,t3
	sll	t0,t0,(D2B_FN64ALIGN_BT-2)
	or	d2b_flags,d2b_flags,t0		; possibly set D2B_FN64ALIGN

	add	src_in,src_in,t1		; src_in += chunk length

	add	t1,t1,7
	consth	t1,(DMAC_CLCK|DMAC_OPSUM|DMAC_GO|DMAC_RD)
	store	0,WORD,t1,p_dma_host_ctl	; DMA GO!

	cpge	t0,d2b_nxt,d2b_lim		; wrap d2b_nxt?
	jmpf	t0,sched_nxt_src4
	 sub	d2b_blkchunks,d2b_blkchunks,1	; d2b_blkchunks--;
	constx	d2b_nxt,d2b
sched_nxt_src4:

	; if ( dma_blkchunks > 0 ) {

	cpgt	t0,d2b_blkchunks,0
	jmpf	t0,sched_nxt_src9
	 nop
	
		; Get next chunk ready in DMA pipeline registers

		; spin waiting for DMA command buffer to empty
sched_nxt_src5:
	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_DCBF
	jmpt	t0,sched_nxt_src5
	 nop

	;	Pre-Schedule-2nd-Chunk();

	load	0,WORD,t1,d2b_nxt		;
	add	d2b_nxt,d2b_nxt,4
	load	0,WORD,t2,d2b_nxt
	add	d2b_nxt,d2b_nxt,4		; d2b_nxt++;

	and	t3,t1,255
	store	0,WORD,t3,p_dma_host_hi
	store	0,WORD,t2,p_dma_host_lo
	srl	t1,t1,16
	add	src_in,src_in,t1		; src_in += length
	add	t1,t1,7				; round up.
	consth  t1,(DMAC_OPSUM|DMAC_GO|DMAC_RD)
	store	0,WORD,t1,p_dma_host_ctl

	cpge	t0,d2b_nxt,d2b_lim		; wrap d2b_nxt?
	jmpf	t0,sched_nxt_src6
	 sub	d2b_blkchunks,d2b_blkchunks,1	; d2b_blkchunks--
	constx	d2b_nxt,d2b
sched_nxt_src6:
	;	if ( d2b_blkchunks > 0 )
	;		Enable-DMA-Done-Interrupts();

	cpgt	t0,d2b_blkchunks,0
	jmpf	t0,sched_nxt_src9
	 nop

		; set up DMA command buffer ready interrupt

		; this can be unprotected since only interrupt
		; that mucks with this register IS DCBE. XXX
	
	or	state,state,ST_DMA_DNINT
	or	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab

	; }

sched_nxt_src9:
	jmpi	raddr
	 nop



dst_dma_tbl:
	.word	dst_dma_idle
	.word	dst_dma_c2b
	.word	dst_dma_le
	.word	dst_dma_hd
	.word	dst_dma_need_rd
	.word	dst_dma_c2b_fp
	.word	dst_dma_rdlst
	.word	dst_dma_fp
	.word	dst_dma_incmpl_rd

dst_dma:
	; ----------------------------
	; DST-DMA()
	; ---------------------------

	; switch ( DST-DMA-State ) {

	constx	t0,dst_dma_tbl
	add	t0,t0,ddma_state
	load	0,WORD,t0,t0
	jmpi	t0
	 nop

dst_dma_c2b:

	; case DST-DMA-C2B:
	;	DST-DMA-State := IDLE;
	;	//FALLTHROUGH....
	const	ddma_state,DDMAST_IDLE

dst_dma_idle:

	; case DST-IDLE:

	;	if ( DST-FIFO-State == DST-FIFO-ERR ) {

	cpeq	t0,dfifo_state,DFIFOST_ERR
	jmpf	t0,dst_dma009
	 cpeq	t0,dfifo_state,DFIFOST_AVAIL

	;		// in HIPPI-PH, notify of error on first burst.

	;		if ( HIPPI-PH || NBOP )

	tbit	t0,state2,ST2_HIPPI_PH
	jmpf	t0,dst_dma007

	;			Queue-B2H( B2H_IN, stk=ulp?, s=0, l=-1 );

	 constx	v0,(B2H_IN|(HIP_STACK_RAW<<16))
	mov	raddr2,raddr
	constn	v1,-256
	call	raddr,queue_b2h
	 or	v1,v1,dfifo_errs
	mov	raddr,raddr2

	jmp	dst_dma_drop
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

dst_dma007:
	tbit	t0,dfifo_flags,DFIFOFL_NBOP
	jmpf	t0,dst_dma008
	
	 constx	t0,(ulptostk/4)				; look up stk from ulp
	add	t0,t0,dfifo_ulp
	sll	t0,t0,2
	load	0,WORD,dst_dma_stk,t0
	cple	t0,dst_dma_stk,15			; valid if <= 15. XXX
	jmpf	t0,dst_dma008
	
	 constx	v0,B2H_IN
	sll	t0,dst_dma_stk,16
	or	v0,v0,t0
	mov	raddr2,raddr
	constn	v1,-256
	call	raddr,queue_b2h
	 or	v1,v1,dfifo_errs
	mov	raddr,raddr2

	jmp	dst_dma_drop
	 or	state,state,(ST_B2H_PUSH|ST_HINT)


dst_dma008:
	;		drop packet;
	jmp	dst_dma_drop
	 nop

	;	}
dst_dma009:
;                 if (DST-FIFO-State == DST-FIFO-AVAIL && Bypass-is-enabled) {
	jmpf	t0,dst_dma2

;                   if (ulp == HIPPI-BP) {
;                     // Should be a bypass packet
	  and     t0, bp_state, BP_ULP_MASK
	cpeq    t0, dfifo_ulp, t0
	jmpf    t0, dst_dma_chk_le
;                   if(no-bp-job-enabled)
;                       goto drop-hippi-bp-bad-job
	  constx t0, BP_JOB_MASK
	and	t0, bp_state, t0
	cpeq	t0, t0, zero
	jmpt	t0, dst_dma_bp_drop_job


	;; ---------------------------------------
	;; Packet appears to be a bypass packet!
	;; load v4 to v13 with pkt bp header (chksum and padding not loaded)
	;; t6 = opcode
	;; t4 = pointer to dfreelist job struct storage area
	;; since v0 through v3 are trashed in several existing
	;; subroutines, we copy the first few words of pkt as
	;; follows:
	;; v10 = 1st word of pkt
	;; v11 = 2nd word
	;; v12 = 3rd word
	;; v13 = 4th word
	;; v15 = offset for start of packet from host base page.

	;; configures descriptor address and data to be dma'ed after this dma is complete.
	;; --------------------------------------

	;	    // Should be a bypass packet
;                   load_d1_header(opcode_word to auth2); // chksum and pad not loaded
	  add	t0, dst_in, 8			     ; skip FP header
	cpge    t1, t0, dst_dram_end
	jmpf	t1, past_wrap_due_to_FP
	  nop
	sub	t0, t0, dst_dram_size			; point to first pkt_hdr word

past_wrap_due_to_FP:
	add	t1, dst_in, dfifo_hdrlen
	cpge	t1, t1, dst_dram_end
	jmpt	t1, load_pkt_hdr_wrd_by_wrd
	 nop
	
	  mtsrim	CR,((BP_PKT_AUTH2 - BP_PKT_D1_START)/4) 
	loadm	0, WORD, v4, t0
	jmp	pkt_hdr_loaded

load_pkt_hdr_wrd_by_wrd:
	mov	t6, t0		; t0 has addrs of pkt hdr with FP skipped 
	load	0, WORD, v4, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd1
	  nop
	sub	t6, dst_dram_end, dst_dram_size

load_pkt_wd1:
	load	0, WORD, v5, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd2
	  nop
	sub	t6, dst_dram_end, dst_dram_size

load_pkt_wd2:
	load	0, WORD, v6, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd3
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd3:
	load	0, WORD, v7, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd4
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd4:
	load	0, WORD, v8, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd5
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd5:
	load	0, WORD, v9, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd6
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd6:
	load	0, WORD, v10, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd7
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd7:
	load	0, WORD, v11, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd8
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd8:
	load	0, WORD, v12, t6
	add	t6, t6, 4
	cpge	t0, t6, dst_dram_end
	jmpf	t0, load_pkt_wd9
	  nop
	sub	t6, dst_dram_end, dst_dram_size
	
load_pkt_wd9:
	load	0, WORD, v13, t6
	


pkt_hdr_loaded:
	;; ------------------------------------
	;; header of packet is in v4-v13, starting at FIRST word, ending at AUTH2
	;; v0 is trashed just before XFR-CYCLE-TOHOST
	;; ------------------------------------

	;; the following code uses t6 as the "opcode word" in too many places!
	mov	t6, v4

;                   if (HIPPI-BP-Version == incorrect)
;                       goto drop-hippi-bp-bad-version
	constx	t0, BP_PKT_VERSION_MASK
	and	t1, t6, t0
	srl	t1, t1, BP_PKT_VERSION_SHIFT
	cpeq	t0, t1, BP_VERSION
	jmpf	t0, dst_dma_bp_drop_vers_num

;                   if (Port-not-in-bounds)
;                       goto drop-hippi-bp-bad-port
	  constx t0, BP_PKT_DST_PORT_MASK
	and	bp_cp_port, v5, t0
	constx	t1, BP_MAX_PORTS
	cpge	t0, bp_cp_port, t1
	jmpt	t0,dst_dma_bp_drop_port

	;	    load_port_state(pkt->port) -> state, job, datap, data_tail, bufx;
	;; Note that this does not load desc queue info!
	
	  sll	t2, bp_cp_port, BP_PORT_SIZE_POW2
	mtsrim	CR,(bp_cp_data_bufx-bp_cp_state)
	add	t2, t2, bp_port_structs
	loadm	0,WORD,bp_cp_state,t2

	;; ---------------------------------------
	;; enough of port structure is loaded
	;; ---------------------------------------

	;	    if (Port-not-enabled)
	;		goto drop-hippi-bp-bad-port
	tbit	t1,bp_cp_state, BP_PORT_VAL
	jmpf	t1, dst_dma_bp_drop_port

	;	    if(job-not-enabled) // assumes job_enable in internal register
	;		goto drop-hippi-bp-bad-job
	 sll	t0, bp_state, bp_cp_job
	jmpf	t0, dst_dma_bp_drop_job
	 
	;	    load_job_state(Port->job) -> bufx_size, auth<2:0>  
	sll	t0, bp_cp_job, BP_JOB_SIZE_POW2	
	add	t1, t0, bp_job_structs
	mtsrim	CR, (bp_cj_auth2 - bp_cj_bufx_entry_size)
	add	t0, t1, (bp_cj_bufx_entry_size - bp_job_reg_base)*4
	loadm	0,WORD, bp_cj_bufx_entry_size, t0

	;; ---------------------------------------
	;; authentication is loaded 
	;; ---------------------------------------

	;	    if(authentication-not-correct)
	;		goto drop-hippi-bp-bad-auth
	cpeq	t1, bp_cj_auth0, v11
	cpeq	t2, bp_cj_auth1, v12
	cpeq	t3, bp_cj_auth2, v13
	and	t0, t2, t1
	and	t0, t3, t0
	jmpf	t0, dst_dma_bp_drop_auth


	;	    if(X_BIT == 1 || MBZ == 1 || G_BIT == 1 || d_BIT == 1 )
	;		  goto drop-hippi-bp-bad-opcode
	 tbit	t1, t6, BP_OP_X
	tbit	t2, t6, BP_OP_Z
	tbit	t3, t6, BP_OP_G
	tbit	t4, t6, BP_OP_d
	or	t0, t1, t2
	or	t0, t0, t3
	or	t0, t0, t4
	jmpt	t0, dst_dma_bp_drop_op		; invalid opcode cond is complete



;                   if(MAC_layer->len != 0)  {
	cpeq	t0, dfifo_d2_size, zero
	jmpt	t0, dst_dma_bp_past_dma
	  nop

;                       // get base phys addr and offset (put in Port struct, but don't store)
	;; Dynamic Mode
	;		  host->offset = pkt->dst_offset << WORDS_TO_BYTES; 
	mov	v15, %%(v4 + (BP_PKT_DST_OFF-BP_PKT_D1_START)/4)
	sll	v15, v15, BP_W_TO_BYTES		; convert w to bytes
	
;			  if (!OP_S) {
	tbit	t0, t6, BP_OP_S
	jmpt	t0, dst_dma_bp_done_seq_chk
	
;				load_seqnum_state(pkt->slot, Port->job) -> max_seqnum, exp_seqnum;
	;; get the slot number 
	  constx	t0, BP_PKT_SLOT_MASK
	and	bp_glob_slot, v5, t0
	srl	bp_glob_slot, bp_glob_slot, BP_PKT_SLOT_SHIFT
	constx	t0, bp_slot_table
	sll	t1, bp_cp_job, BP_SLOT_TABLE_SIZE_POW2
	add, 	t0, t0, t1
	;; go to the proper slot in this job's table
	sll	t1, bp_glob_slot, BP_SLOT_TABLE_ENTRY_SIZE_POW2
	add	t5, t0, t1

	mtsrim	CR, 2-1				; load 2 words
	loadm	0, WORD, bp_slot_max_seqnum, t5

	;; -----------------------------------
	;; t5 = seqnum_state_ptr
	;; -----------------------------------

;				if (OP_F) {
	tbit	t0, t6, BP_OP_F
	jmpf	t0, dst_dma_bp_chk_seqnum
;					if( !(exp_seqnum == 0 || exp_seqnum == -1)) {
;					   // last block packet lost a microblock
;					   inc_stat(dst_bp_seq_err);
	  cpeq	t0, bp_slot_exp_seqnum, zero
	constx	t2, -1
	cpeq	t1, bp_slot_exp_seqnum, t2
	nor	t0, t0, t1
	jmpf	t0, dst_dma_bp_no_seq_err
 	  nop
	BPINCSTAT	hst_d_bp_seq_err	; previous pkt in error, not this one!
;                                       }

	
dst_dma_bp_no_seq_err:		
;					max_seqnum = pkt->seqnum;
;					exp_seqnum = pkt->seqnum;
	constx	t0, BP_PKT_SEQNUM_MASK
	and	bp_slot_max_seqnum, v6, t0
	mov	bp_slot_exp_seqnum, bp_slot_max_seqnum
;				}

dst_dma_bp_chk_seqnum:	
;				if(exp_seqnum != pkt->seqnum)
	constx  t0, BP_PKT_SEQNUM_MASK
	and	t2, v6, t0
	cpeq	t0, t2, bp_slot_exp_seqnum
;					goto drop-hippi-bp-bad-seqnum;
	jmpf	t0, dst_dma_bp_drop_bad_seq

;			  	exp_seqnum--;
	  sub	bp_slot_exp_seqnum, bp_slot_exp_seqnum, 1

;			  	store_seqnum(pkt->slot, Port->job) -> max_seqnum, exp_seqnum;
;	mtsrim	CR, 2-1				; store 2 words
;	storem	0, WORD, bp_slot_max_seqnum, t5

	store	0, WORD, bp_slot_max_seqnum, t5
	add	t0, t5, 4
	store	0, WORD, bp_slot_exp_seqnum, t0
	
;			  }

dst_dma_bp_done_seq_chk:	
;			  Port->bufx = pkt->dst_bufx;
	mov	bp_cp_data_bufx, v9

;			  if(bufx >= MAX_BUFX); // bufx not in bounds (do unsigned compare)
;				goto drop-hippi-bp-bad_bufx
	constx	t0, BP_DFM_ENTRIES
	cpgeu	t2, bp_cp_data_bufx, t0
	jmpt	t2, dst_dma_bp_drop_dfl
		
;			  // both offset and length cacheline aligned (IO4 bug requires this)
;			  if(((host->offset && NBPCL-1) != 0)
;				|| ((MAC_layer->len && NBPCL-1) != 0)) 
;				goto drop-hippi-bp-bad-offset;
	 and	t0, v15, NBPCL-1
	cpneq	t0, t0, zero
	and	t1, dfifo_d2_size, NBPCL-1
	cpneq	t1, t1, zero
	or	t2, t0, t1
	jmpt	t2, dst_dma_bp_drop_off
	  nop	
	jmp	dst_dma_bp_got_bufx
;		    }

	;; -----------------------------------
	;; t5 is free
	;; -----------------------------------


dst_dma_bp_got_bufx:		
;		    // have offset 
;		    // check if data fits in bufx entry 
;		    if(host->offset + MAC_layer->len > job->bufx_entry_size)
;			goto drop-hippi-bp-bad-offset
	add	t1, v15, dfifo_d2_size
	cpgt	t0, t1, bp_cj_bufx_entry_size
	jmpt	t0, dst_dma_bp_drop_off

;		    // store before xlate to avoid race conditions
;		    // must translate bufx to pfn everytime because driver
;		    // might have written dead page to freemap location
;		    store_dma_status(ENABLE, Port->bufx, Port->job);
          sll     t1, bp_cp_job, DMA_STATUS_JOB_SHIFT
        constx  t0, DMA_ACTIVE_DATA_DEST
        or      t2, t0, t1
        or      t0, t2, bp_cp_data_bufx
        ; store it in the dma_status word
        add     t1, bp_config, dma_status
        store   0, WORD, t0, t1
	
;		    Port->data_basep = bufx_to_paddr(Port->bufx, DESTINATION);
	;; fetch data page physical address from bufx destination freemap
	;; then shift it because it's in 16 KB granularity
	sll	t1, bp_cp_job, BP_DFM_SIZE_POW2
	constx	t0, bp_dfreemap
	add	t0, t0, t1
	sll	t1, bp_cp_data_bufx, 2		; convert to bytes
	add	t0, t0, t1
	load	0, WORD, t1, t0
	
	srl	bp_cp_d_data_base_hi, t1, BP_FM_SHIFT_HI
	sll	bp_cp_d_data_base_lo, t1, BP_FM_SHIFT_LO


	;; -----------------------------
	;; Configure DMA engine
	;; -----------------------------

        ;                   // START DMA
	;		    src = dst_in + dfifo_hdrlen
	;		    if(src > dst_dram_end)
	;			src -= dst_dram_size;
	 add	v0, dst_in, dfifo_hdrlen	; remove D1 section and wrap if needed
	cpge	t1, v0, dst_dram_end
	jmpf	t1, dst_dma_bp_past_buf_wrap
	 store	0,WORD,bp_cp_d_data_base_hi,p_dma_host_hi  ; host hi address for dma

	sub	v0, v0, dst_dram_size

dst_dma_bp_past_buf_wrap:	

	; 	    XFER-CYCLE-TOHOST(src);
	mov	raddr2,raddr
	constx	t1,OFFS_XFER
	call	raddr,dst_read_xfer
	 add	v0,v0,t1
	mov	raddr,raddr2

;	 		    des = Port->data_basep + host->offset;
;		            start_dma(src, des, DESTINATION, len);
	;;      this can't roll because base is on page boundary
	add	t0, bp_cp_d_data_base_lo, v15
	store	0,WORD,t0,p_dma_host_lo

	mov	t0, dfifo_d2_size                ; load length
	consth	t0, (DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl	; GO!

;                   }  // if MAC_layer length is non-zero


		
dst_dma_bp_past_dma:	
	
;		    // DMA is started! 
;		    // Time to deal with descriptor generation
;		    // Load rest of port state and check if need to send an interrupt

;		    dst_pkt_len = MAC_layer->len;
	mov	curr_dst_bp_pkt_len, dfifo_d2_size  ; already in bytes

;		    load_port_state(dq_base, dq_tail, dq_size, int_cnt)
	sll	t2, bp_cp_port, BP_PORT_SIZE_POW2
	add	t2, t2, bp_port_structs
	add	t5, t2, ((bp_cp_dq_base_lo - bp_port_reg_base)*4)
	mtsrim	CR,(bp_cp_int_cnt-bp_cp_dq_base_lo)  ; loads 5 words
	loadm	0,WORD,bp_cp_dq_base_lo,t5

	;; --------------------------------
	;; t5 = ptr to mem location of bp_cp_dq_base_lo
	;; --------------------------------
	
;		    if(OP_I && (OP_S || exp_seqnum == 0))) {
	tbit	t0, t6, BP_OP_I
	tbit	t1, t6, BP_OP_S
	cpeq	t2, bp_slot_exp_seqnum, zero
	or	t1, t1, t2
	and	t0, t0, t1
	jmpf	t0, dst_dma_bp_past_host_int
	  nop
;			Port->intr_cnt++;
	add	bp_cp_int_cnt, bp_cp_int_cnt, 1

;			if(Port->state != INT_PENDING) {
	tbit	t0, bp_cp_state, BP_PORT_INT_PENDING
	jmpt	t1, dst_dma_bp_past_host_int
	  nop

	;-----------------------------------
	;; queue host interrupt and b2h packet
;				Port->state = INT_PENDING;
	or	bp_cp_state, bp_cp_state, BP_PORT_INT_PENDING

;				queue_b2h(B2H_PORTINT, Port->intr_cnt, pkt->port);
;				hippi_state |= B2H_PUSH | HINT;
	;; requires v0, v1, trashes t0
	constx	v0, B2H_PORTINT
	or	v0, v0, bp_cp_port
	mov	v1, bp_cp_int_cnt
	mov	raddr2,raddr
	call	raddr,queue_b2h
	or	state,state,(ST_B2H_PUSH|ST_HINT)
	mov	raddr,raddr2
	;-----------------------------------
;			}

;			store_port_state(pkt->port) -> intr_cnt;
	add	t0, t5, ((bp_cp_int_cnt -  bp_cp_dq_base_lo)*4)
	store	0, WORD, bp_cp_int_cnt, t0
;		    }
	
dst_dma_bp_past_host_int:		
;		    if (OP_D && (OP_S || exp_seqnum == 0)) { // exp_seqnum decremented earlier
	tbit	t3, t6, BP_OP_D
	tbit	t0, t6, BP_OP_S
	cpeq	t1, bp_slot_exp_seqnum, zero
	or	t0, t0, t1
	and	t0, t0, t3
	jmpf	t0, dst_dma_bp_past_desc
	 nop
;			// PREPARE DESCRIPTOR
;			bp_state |= PUSH_DESC
	constx	t0, BP_PORT_PUSH_DESC
	or	bp_state, bp_state, t0
;			// copy header in reverse order for desc  - do this to preserve
;                       // all possible header payload bits.
	;; create descriptor in v11-v14
	; v11: desc[0]
	; v12: desc[1]
	; v13: desc[2]
	; v14: desc[3]

;       src_host_port = pkt.i[0] & HOST_PORT_MASK;
	constx	t0, BP_DESC_HOSTX_MASK
	constx	t1, BP_DESC_PORT_MASK
	or	t0, t0, t1
	and	t1, v4, t0
	;------------------------
	;  t1 now has src_hostport
	;-------------------------

		
;       length = MAC_layer->len >> bytes_to_words;
	srl	t2, dfifo_d2_size, BP_BYTES_TO_W	
	;------------------------
	;  t2 now has length
	;-------------------------


;       desc[3] = pkt.i[0] & ~(SEQNUM_MASK);
	mov	v14, v4
	constx	t0, BP_DESC_LENGTH_MASK
	andn	v14, v14, t0
;       desc[3] |= length;
	or	v14, v14, t2
	
;       desc[2] = pkt.i[1] & ~(HOST_PORT_MASK);  // clear the field
	mov	v13, v5
	constx	t0, BP_DESC_HOST_PORT_MASK
	andn	v13, v13, t0
;       desc[2] |= src_host_port;
	or	v13, v13, t1
	
	
	;------------------------------
	; t1 and t2 are free now
	;--------------------------------



;       desc[1] = (pkt.i[3] << BUFX_SHIFT) | pkt.i[4];
	mov	v12, v7
	sll	v12, v12, BP_DESC_SRC_BUFX_SHIFT
	constx	t0, BP_DESC_SRC_OFF_MASK
	and	t1, v8, t0
	or	v12, v12, t1

;       desc[0] = (pkt.i[5] << BUFX_SHIFT) | pkt.i[6];
	mov	v11, v9
	sll	v11, v11, BP_DESC_DST_BUFX_SHIFT
	constx  t0, BP_DESC_DST_OFF_MASK
	and	t1, v10, t0
	or	v11, v11, t1


;       if(length != 0)  {
	cpneq	t0, dfifo_d2_size, zero
	jmpf	t0, dst_dma_bp_desc_done

;				if(!OP_S) { // Multiple Packets
	  tbit	t0, t6, BP_OP_S
	jmpt	t0, dst_dma_bp_desc_done

;					desc[3] = (desc[3] & ~LENGTH_MASK) | max_seqnum
	  constx	t0, BP_DESC_LENGTH_MASK		; clear it first
	andn	v14, v14, t0
	or	v14, v14, bp_slot_max_seqnum

;                                       desc[0] &= ~(BUFX_MASK);   // clear dst_bufx field
	constx	t0, BP_DESC_DST_BUFX_MASK
	andn	v11, v11, t0

;                                       desc[0] |= (Port->bufx - max_seqnum + 1) << BUFX_SHIFT;
	sub	t1, bp_cp_data_bufx, bp_slot_max_seqnum
	add	t1, t1, 1
	sll	t1, t1, BP_DESC_DST_BUFX_SHIFT
	or	v11, v11, t1

;                                       desc[1] &= ~(BUFX_MASK);   // clear src_bufx field
	constx	t0, BP_DESC_SRC_BUFX_MASK
	andn	v12, v12, t0

;                                       desc[1] |= (pkt.i[3] - max_seqnum + 1) << BUFX_SHIFT
	constx	t0, BP_DESC_SRC_BUFX_MASK
	nand	t0, t0, t0
	and	t1, v7, t0
	sub	t1, t1, bp_slot_max_seqnum
	add	t1, t1, 1
	sll	t1, t1, BP_DESC_SRC_BUFX_SHIFT
	or	v12, v12, t1
;				}
;         } // end non-zero payload

	

;			// DESCRIPTOR IS COMPLETE
dst_dma_bp_desc_done:	
;			store_desc(desc);
	constx	t0, dst_bp_desc
	;;  removed because of h/w bug
;	mtsrim	CR, (BP_DESC_SIZE/4)-1    
;	storem	0, WORD, v11, t0

	store	0, WORD, v11, t0
	add	t0, t0, 4
	store	0, WORD, v12, t0
	add	t0, t0, 4
	store	0, WORD, v13, t0
	add	t0, t0, 4
	store	0, WORD, v14, t0
		
;		    	// setup info for dst_dma_dn (required because hcmd's can trash regs)
;			// dq_tail, port_num, job
;		    	dst_dq_tail = Port->dq_base + Port->dq_tail;
	add	dst_bp_dq_tail_lo, bp_cp_dq_base_lo, bp_cp_dq_tail
	mov	dst_bp_dq_tail_hi, bp_cp_dq_base_hi
;			dst_dma_dn_port = pkt->port;
;			dst_dma_dn_job = Port->job;
	mov	dst_bp_job, bp_cp_job
	mov	dst_bp_port, bp_cp_port

;		    	Port->dq_tail += BP_DESC_SIZE;
	add	bp_cp_dq_tail, bp_cp_dq_tail, BP_DESC_SIZE

;		    	if (Port->dq_tail >= Job->dq_size) {
;				Port->dq_tail = 0;
	cpge	t0, bp_cp_dq_tail, bp_cp_dq_size
	jmpf	t0, dst_dma_bp_past_dqwrap
	  nop
	mov	bp_cp_dq_tail, zero
;		    	}

dst_dma_bp_past_dqwrap:		
;			store_port_state(pkt->port) -> dq_tail;
	add	t0, t5, ((bp_cp_dq_tail -  bp_cp_dq_base_lo)*4)
	store	0, WORD, bp_cp_dq_tail, t0
;		    }
	
dst_dma_bp_past_desc:	
	;; -------------------------------------
	;; Clean up state for dfifo and copy out parameters I might care about
	;; ------------------------------------
		
	add	dst_in, dst_in, dfifo_len

	cpge	t0,dst_in,dst_dram_end			; wrap dst_in?
	jmpf	t0,dst_dma_bp_dstinwrap
	  nop
	sub	dst_in,dst_in,dst_dram_size

dst_dma_bp_dstinwrap:	
	const	ddma_state,DDMAST_DMA_BP
	const	dma_client,DMA_CLIENT_DST

	mov	dst_out,dst_in
	mov	dst_dma_rdys,dfifo_rdys

	;; let dfifo state machine go
	const	dfifo_state,DFIFOST_NONE
	
	; 		return 1;

	jmpi	raddr
	  cpeq	v0,v0,v0
	

	
dst_dma_chk_le:	
	cpeq	t0,dfifo_ulp,HIPPI_ULP_LE
	jmpf	t0,dst_dma0k

	 cpgt	t0,dfifo_hdrlen,mlen
	jmpt	t0,dst_dma_ledrop

	; accepting HIPPI-LE packets?
	 tbit	t0,op_flags,OPF_ENB_LE
	jmpt	t0,dst_dma010
	 nop

dst_dma_ledrop:
	INCSTAT	hst_d_ledrop
	jmp	dst_dma_drop
	 nop

	; check for MINimum network buffers before beginning

dst_dma010:
	 sub	t0,ifhip_big_in,ifhip_big_out
	jmpf	t0,dst_dma011
	 sub	t1,ifhip_sml_in,ifhip_sml_out
	constx	t2,(ifhip_big_end-ifhip_big)
	add	t0,t0,t2
dst_dma011:
	jmpf	t1,dst_dma012
	 cpge	t0,t0,(MIN_BIG*8)
	constx	t2,(ifhip_sml_end-ifhip_sml)
	add	t1,t1,t2
dst_dma012:
	cpge	t1,t1,(MIN_SML*8)
	and	t0,t0,t1
	jmpf	t0,dst_dma_ledrop
	 nop


dst_dma0:

	; double-check d2_size

	add	t0,dfifo_d2_size,32		; drop
	cpltu	t0,dfifo_len,t0			; if ( dfifo_len < d2_size+32 )
	jmpt	t0,dst_dma_ledrop
	
	 constx	t0,MAX_LE_D2SIZE		; drop if d2_size too big
	cpgtu	t0,dfifo_d2_size,t0		; if ( d2_size > 64K+8 )
	jmpt	t0,dst_dma_ledrop
	 nop
	
	;	    // Pick up info on packet and release
	;	    // DST-FIFO machine to parse next header.

	;	    dst_dma_blklen = dfifo_d2_size+32;
	;	    dst_dma_ulp = dfifo_ulp;
	;	    (etc.)
	;	    dst_out = dst_in;
	;	    dst_in += dfifo_len;
	;	    DST-FIFO-State := DST-FIFO-NONE;

	; use value in d2_size for DMA so we don't have to DMA
	; the padding word(s).

	add	dst_dma_len,dfifo_d2_size,35
	andn	dst_dma_len,dst_dma_len,3
	mov	dst_dma_blklen,dst_dma_len

	mov	dst_dma_ulp,dfifo_ulp
	mov	dst_dma_hdrlen,dfifo_hdrlen
	mov	dst_dma_ifield,dfifo_ifield
	mov	dst_dma_rdys,dfifo_rdys
	mov	dst_dma_flags,dfifo_flags
	mov	dst_out,dst_in
	add	dst_in,dst_in,dfifo_len
	const	dfifo_state,DFIFOST_NONE

	; 	    XFER-CYCLE-TOHOST();

	mov	raddr2,raddr
	constx	v0,OFFS_XFER
	call	raddr,dst_read_xfer
	 add	v0,v0,dst_out
	mov	raddr,raddr2

	.if DEBUG
	tbit	t0,dst_dma_flags,DWOKI_EOP	; ASSERT we have end-of-packet
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	.if DEBUG
	cplt	t0,dst_dma_blklen,32			; sane length?
	jmpf	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	;	    DST-DMA-State := DST-DMA-LE;
	;	    DMA-Client := DST;

	const	ddma_state,DDMAST_DMA_LE
	const	dma_client,DMA_CLIENT_DST

	; 	    Schedule-HDR-To-Host();

	cple	t0,dst_dma_blklen,mlen			; small? do entire
	jmpf	t0,dst_dma0a				; packet in one mbuf
	 load	0,WORD,t1,ifhip_sml_out			; get small buffer---
	mov	dst_dma_hdrlen,dst_dma_blklen
dst_dma0a:
	srl	dst_b2h_1,dst_dma_hdrlen,2		; start creating b2h
	add	ifhip_sml_out,ifhip_sml_out,4
	load	0,WORD,t2,ifhip_sml_out
	add	ifhip_sml_out,ifhip_sml_out,4
	and	t0,t1,255
	store	0,WORD,t0,p_dma_host_hi			; program small buffer
	constx	t0,ifhip_sml_end			; address
	store	0,WORD,t2,p_dma_host_lo
	cpge	t0,ifhip_sml_out,t0			; wrap?
	jmpf	t0,dst_dma0b
	 add	t0,dst_dma_hdrlen,4			; program length
	constx	ifhip_sml_out,ifhip_sml
dst_dma0b:
	consth	t0,(DMAC_CLCK|DMAC_OPSUM|DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl		; GO!  (hdr buf)
	sub	dst_dma_blklen,dst_dma_blklen,dst_dma_hdrlen
	const	dst_dma_cksum_slop,0

	;	    if ( (dst_hdrlen & 4) && still-a-body-to-move ) {

	cpgt	t0,dst_dma_blklen,0
	jmpf	t0,dst_dma0i
	 sll	t0,dst_dma_hdrlen,31-2
	jmpf	t0,dst_dma0c

	;		// We've got an odd word header, schedule
	;		// a transfer cycle someway.
	;		Set-Up-DMA-Complete-Interrupt;

	add	dst_dma_xfer_addr,dst_out,dst_dma_hdrlen
	cpge	t0,dst_dma_xfer_addr,dst_dram_end	; wrap?
	jmpf	t0,dst_dma0bb
	 or	state,state,ST_DMA_DNINT
	sub	dst_dma_xfer_addr,dst_dma_xfer_addr,dst_dram_size
dst_dma0bb:
	or	inte_shadow,inte_shadow,INT_DMA_DONE
	jmp	dst_dma0i
	 store	0,WORD,inte_shadow,p_int_enab

	;	    }
dst_dma0c:
	;	    else if ( still-a-body-to-move ) {

	;		Pre-Schedule-Next-Chunk();

	load	0,WORD,t0,p_dma_host_ctl		; wait for cmd buffer
	tbit	t0,t0,DMAC_DCBF
	jmpt	t0,dst_dma0c

	; program a big chunk...

	 const	t0,0x100
	add	dst_b2h_1,dst_b2h_1,t0			; increment pages field

	load	0,WORD,t1,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	load	0,WORD,t2,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	and	t0,t1,255
	store	0,WORD,t0,p_dma_host_hi			; program big buffer
	constx	t0,ifhip_big_end			; address
	store	0,WORD,t2,p_dma_host_lo
	cpge	t0,ifhip_big_out,t0			; wrap?
	jmpf	t0,dst_dma0g
	 mov	t1,nbpp
	constx	ifhip_big_out,ifhip_big
dst_dma0g:
	cpgt	t0,dst_dma_blklen,t1
	jmpt	t0,dst_dma0h
	 nop

	 mov	t1,dst_dma_blklen

dst_dma0h:
	sub	dst_dma_blklen,dst_dma_blklen,t1
	add	t1,t1,4
	consth	t1,(DMAC_OPSUM|DMAC_GO|DMAC_WR)
	store	0,WORD,t1,p_dma_host_ctl

	;		    if ( still-more-to-move )

	cpgt	t0,dst_dma_blklen,0
	jmpf	t0,dst_dma0i
	 nop
	;			Set-Up-DCBF-Interrupt;

	or	state,state,ST_DMA_DNINT
	or	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab

	;		}

dst_dma0i:
	; 		return 1;
	jmpi	raddr
	 cpeq	v0,v0,v0

	;	}

dst_dma0k:
	;	else if ( DST-FIFO-State == DST-FIFO-AVAIL && HIPPI-PH-mode ) {

	jmpf	dfifo_ulp,dst_dma1
	 nop

	;	    	// Pick up info on packet and release
	;	    	// DST-FIFO machine to parse next header.
	;	    	dst_dma_blklen = dfifo_len;
	;	    	dst_dma_ulp = dfifo_ulp;
	;	    	(etc.)
	;	    	dst_out = dst_in;
	;	    	dst_in += dfifo_len;
	;	    	DST-FIFO-State := DST-FIFO-NONE;

	mov	dst_dma_len,dfifo_len
	mov	dst_dma_blklen,dfifo_len
	mov	dst_dma_ulp,dfifo_ulp
	mov	dst_dma_hdrlen,dfifo_hdrlen
	mov	dst_dma_ifield,dfifo_ifield
	mov	dst_dma_flags,dfifo_flags
	mov	dst_dma_rdys,dfifo_rdys
	mov	dst_out,dst_in
	add	dst_in,dst_in,dfifo_len
	const	dst_dma_stk,HIP_STACK_RAW
	const	dfifo_state,DFIFOST_NONE

	;		if ( begin-of-packet ) {
	; 			Queue-B2H( B2H_IN, s=0, l=dst_dma_len );
	; 			B2H-Push-Flag := TRUE;
	;			Host-Int-Flag := TRUE;

	tbit	t0,dfifo_flags,DFIFOFL_NBOP
	jmpt	t0,dst_dma_need_rd

	 constx	v0,(B2H_IN|(HIP_STACK_RAW<<16))
	mov	raddr2,raddr
	call	raddr,queue_b2h
	 mov	v1,dst_dma_len
	mov	raddr,raddr2

	;		}
	; 		DST-DMA-State := DST-WAIT-RD;
	;		(goto case DST-WAIT-RD; XXX)

	jmp	dst_dma_need_rd
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

	;	}

dst_dma1:
	; 	else if ( DST-FIFO-State == DST-FIFO-AVAIL && ulp-valid &&
	;		hdr-buf-avail && begin-of-packet) {

	;				// drop-packet if invalid-ulp

	constx	t0,(ulptostk/4)				; look up stk from ulp
	add	t0,t0,dfifo_ulp
	sll	t0,t0,2
	load	0,WORD,dst_dma_stk,t0
	cple	t0,dst_dma_stk,15			; valid if <= 15. XXX
	jmpt	t0,dst_dma1a
	
	 tbit	t0,dfifo_flags,DFIFOFL_NBOP
	jmpt	t0,dst_dma_drop		; only count the first one
	 nop

	INCSTAT	hst_d_badulps

	; //
	; // dst_dma_drop: destined for invalid ULP, so drop,
	; // reprogram ready's.
	; //

dst_dma_drop:
	add	dst_in,dst_in,dfifo_len
	cpge	t0,dst_in,dst_dram_end			; wrap dst_in?
	jmpf	t0,dst_dma1aa
	 nop
	sub	dst_in,dst_in,dst_dram_size
dst_dma1aa:
	mov	dst_out,dst_in

	mov	raddr2,raddr
	call	raddr,dst_ready				; reprogram ready's
	 mov	v0,dfifo_rdys
	mov	raddr,raddr2

	mov	dst_dma_flags,dfifo_flags

	jmp	dst_dma2
	 const	dfifo_state,DFIFOST_NONE

dst_dma1a:
	tbit	t0,dfifo_flags,DFIFOFL_NBOP
	jmpt	t0,dst_dma1nbop

		; <hdr-buf-avail>?
	
	 sll	t3,dst_dma_stk,3
	constx	t1,hdrbufs
	add	t3,t3,t1				; t3 = &hdrbufs[stk]
	load	0,WORD,t1,t3
	add	t3,t3,4
	load	0,WORD,t2,t3				; <t1,t2> = hdrbuf[stk]
	sub	t3,t3,4
	or	t0,t1,t2
	cpeq	t0,t0,0					; null?
	jmpt	t0,dst_dma2				; forget it...
	 nop


dst_dma1b:
	;	    	// Pick up info on packet and release
	;	    	// DST-FIFO machine to parse next header.
	;	    	dst_dma_blklen = dfifo_len;
	;	    	dst_dma_ulp = dfifo_ulp;
	;	    	(etc.)
	;	    	dst_out = dst_in;
	;	    	dst_in += dfifo_len;
	;	    	DST-FIFO-State := DST-FIFO-NONE;

	mov	dst_dma_len,dfifo_len
	mov	dst_dma_blklen,dfifo_len
	mov	dst_dma_ulp,dfifo_ulp
	mov	dst_dma_hdrlen,dfifo_hdrlen
	mov	dst_dma_ifield,dfifo_ifield
	mov	dst_dma_rdys,dfifo_rdys
	mov	dst_dma_flags,dfifo_flags
	mov	dst_out,dst_in
	add	dst_in,dst_in,dfifo_len
	const	dfifo_state,DFIFOST_NONE

	;		if ( dst_dma_hdrlen > dst_dma_len )
	;			dst_dma_hdrlen = dst_dma_len
	; 		XFER-CYCLE-TOHOST();
	cpgt	t0,dst_dma_hdrlen,dst_dma_len
	jmpf	t0,dst_dma1ba
	 mov	raddr2,raddr
	mov	dst_dma_hdrlen,dst_dma_len
dst_dma1ba:
	constx	v0,OFFS_XFER
	call	raddr,dst_read_xfer
	 add	v0,v0,dst_out
	mov	raddr,raddr2


	; 		DMA-Client := DST;
	; 		Schedule-header-To-Host();	
	; 		DST-DMA-State := DST-DMA-HD;

	const	dma_client,DMA_CLIENT_DST
	const	ddma_state,DDMAST_DMA_HD

	store	0,WORD,zero,t3				; zero out hdrbuf[stk]
	add	t3,t3,4
	store	0,WORD,zero,t3

	cpeq	t0,dst_dma_hdrlen,0			; zero len? pretend
	jmpt	t0,dst_dma1c				; to dma...

	 and	t0,t1,255
	store	0,WORD,t0,p_dma_host_hi			; t1,t2 are hdr buf
	store	0,WORD,t2,p_dma_host_lo			; from above
	add	t0,dst_dma_hdrlen,4			; round up hdr len
	consth	t0,(DMAC_GO|DMAC_WR|DMAC_CLCK|DMAC_OPSUM)
	store	0,WORD,t0,p_dma_host_ctl		; go!

	add	dst_out,dst_out,dst_dma_hdrlen
	cpge	t0,dst_out,dst_dram_end			; wrap dst_out?
	jmpf	t0,dst_dma1c
	 sub	dst_dma_blklen,dst_dma_blklen,dst_dma_hdrlen

	sub	dst_out,dst_out,dst_dram_size

dst_dma1c:

	; 		return 1;

	jmpi	raddr
	 cpeq	v0,v0,v0
	
	;	}
	;	else if ( DST-FIFO-State == DST-FIFO-AVAIL && ulp-valid &&
	;		  not-begin-of-packet ) {
dst_dma1nbop:

	;		<grab-data-from-FIFO>

	mov	dst_dma_len,dfifo_len
	mov	dst_dma_blklen,dfifo_len
	mov	dst_dma_ulp,dfifo_ulp
	mov	dst_dma_hdrlen,dfifo_hdrlen
	mov	dst_dma_ifield,dfifo_ifield
	mov	dst_dma_rdys,dfifo_rdys
	mov	dst_dma_flags,dfifo_flags
	mov	dst_out,dst_in
	add	dst_in,dst_in,dfifo_len

	;		DST-DMA-State := DST-WAIT-RD;

	jmp	dst_dma_need_rd
	 const	dfifo_state,DFIFOST_NONE

	;	}
dst_dma2:
	; 	else if ( DST-Polling-Flag && c2b_avail == 0 ) {

	tbit	t0,state,ST_DST_POLL
	jmpf	t0,dst_dma3b
	 cpeq	t0,c2b_avail,0
	jmpf	t0,dst_dma3b
	 nop

	; 		DST-DMA-State := DST-C2B;
	const	ddma_state,DDMAST_C2B

dst_dma_sched_c2b:

	; 		Schedule-C2B-Read();

	.if DEBUG
	cpeq	t0,c2b_avail,0			; ASSERT we aren't already
	jmpt	t0,.+16				; DMA'ing C2B's
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	constx	t0,(c2b+OFFS_XFER)
	constx	t1,XFER_MWTB			; do XFER cycle
	store	0,WORD,t1,t0			; XFER, rdy to read from host

	 constx	t0,hostp_c2b
	load	0,WORD,t1,t0
	add	t0,t0,4
	load	0,WORD,t2,t0
	add	t2,t2,c2b_hnxt
	store	0,WORD,t2,p_dma_host_lo
	addc	t1,t1,0				; <t1,t2> = host addr to do c2b
	store	0,WORD,t1,p_dma_host_hi

	and	c2b_fetch,t2,(NBPCL-1)		; how much to DMA?
	subr	c2b_fetch,c2b_fetch,NBPCL	; n = 128 - ( addr & 127 )
	cplt	t0,c2b_fetch,64			; if ( n < 64 ) n += 128;
	jmpf	t0,dst_dma3a
	 nop
	add	c2b_fetch,c2b_fetch,NBPCL
dst_dma3a:
	mov	t0,c2b_fetch
	consth	t0,(DMAC_GO|DMAC_NPREF|DMAC_RD)
	store	0,WORD,t0,p_dma_host_ctl	; GO!

	; 		DST-Polling-Flag := FALSE;
	andn	state,state,ST_DST_POLL

	;		DMA-Client := DST;
	const	dma_client,DMA_CLIENT_DST

	; 		return 1;
	jmpi	raddr
	 cpeq	v0,v0,v0
	;	}

dst_dma3b:
	; 	return 0;
	jmpi	raddr
	 const	v0,0


dst_dma_le:

	; case DST-DMA-LE:

	; 	if ( running low on buffers && c2b_avail == 0 ) {

	cpeq	t0,c2b_avail,0
	jmpf	t0,dst_dma4c
	 sub	t0,ifhip_big_in,ifhip_big_out
	jmpf	t0,dst_dma4a
	 sub	t1,ifhip_sml_in,ifhip_sml_out
	constx	t2,(ifhip_big_end-ifhip_big)
	add	t0,t0,t2
dst_dma4a:
	jmpf	t1,dst_dma4b
	 constx	t2,(ifhip_sml_end-ifhip_sml)
	add	t1,t1,t2
dst_dma4b:
	constx	t2,(MAX_BIG*8/2)
	cpge	t0,t0,t2
	constx	t2,(MAX_SML*8/2)
	cpge	t1,t1,t2
	and	t0,t0,t1
	jmpf	t0,dst_dma_sched_c2b
	 const	ddma_state,DDMAST_C2B

	; 		Schedule-C2B-Read();
	; 		DST-DMA-State := DST-C2B;
	; 		DST-Polling-Flag := FALSE;
	; 		return 1;
	; 	}

dst_dma4c:
	; 	DST-DMA-State := DST-IDLE;
	; 	goto DST-IDLE;	// XXX: kludgey yes but allows fast reschedule

	jmp	dst_dma_idle
	 const	ddma_state,DDMAST_IDLE

dst_dma_hd:
dst_dma_need_rd:
dst_dma_c2b_fp:
	; case DST-DMA-HD:
	; case DST-WAIT-RD:
	; case DST-C2B-FP;

	; 	if ( c2b-read-avail ) {

	constx	t0,c2b_read_tbl				; get c2b_read_tbl[stk]
	sll	t3,dst_dma_stk,3
	add	t3,t3,t0
	load	0,WORD,t1,t3
	cpeq	t0,t1,0
	jmpt	t0,dst_dma5a
	 add	t3,t3,4

	load	0,WORD,t2,t3				; <t1,t2> = physaddr
	sub	t3,t3,4					; t3=&c2b_read_tbl[stk]

	store	0,WORD,zero,t3
	add	t3,t3,4
	store	0,WORD,zero,t3				; zero out c2b-readlist

	const	dst_b2h_2,0

	; 			DMA-Client := DST;
	const	dma_client,DMA_CLIENT_DST

	; 			Schedule-RDLIST-DMA( c2b-read-addr );

	.if ( c2b_read & BLKADDR_QSF )
	constx	t3,(c2b_read+OFFS_XFER)
	.else
	constx	t3,(c2b_read+OFFS_XFER-TPDRAM_BLKSIZE)
	.endif
	constx	t0,XFER_MWTB
	store	0,WORD,t0,t3				; XFER-CYCLE-FROMHOST

	srl	dst_dma_rdlst_len,t1,16			; length of read list
	and	t1,t1,255
	store	0,WORD,t1,p_dma_host_hi
	add	t0,dst_dma_rdlst_len,4			; round up
	store	0,WORD,t2,p_dma_host_lo
	consth	t0,(DMAC_GO|DMAC_RD)
	store	0,WORD,t0,p_dma_host_ctl		; go!

	; 			DST-DMA-State := DST-DMA-RDLIST;
	; 			return 1;

	const	ddma_state,DDMAST_DMA_RDLST
	jmpi	raddr
	 cpeq	v0,v0,v0

	; 		}

dst_dma5a:
	; 	else if ( DST-Polling-Flag && c2b_avail == 0 ) {

	tbit	t0,state,ST_DST_POLL
	jmpf	t0,dst_dma5b
	 cpeq	t0,c2b_avail,0
	jmpf	t0,dst_dma5b
	 nop

	; 		Schedule-C2B-Read();
	; 		DST-DMA-State := DST-C2B-FP;
	; 		DST-Polling-Flag := FALSE;
	; 		return 1;

	jmp	dst_dma_sched_c2b
	 const	ddma_state,DDMAST_C2B_FP

	; 	}
	;	else if ( ULP-Deassigned ) {
dst_dma5b:

	jmpf	dst_dma_ulp,dst_dma5ba
	 tbit	t0,state2,ST2_HIPPI_PH
	jmpt	t0,dst_dma5c
	 nop
	jmp	dst_dma5bb
	 nop

dst_dma5ba:
	constx	t0,(ulptostk/4)				; look up stk from ulp
	add	t0,t0,dst_dma_ulp
	sll	t0,t0,2
	load	0,WORD,t0,t0
	cple	t0,t0,15				; valid if <= 15. XXX
	jmpt	t0,dst_dma5c
	 nop
dst_dma5bb:
	INCSTAT	hst_d_badulps

	;		Discard-rest-of-packet();

	add	dst_out,dst_out,dst_dma_blklen
	cpge	t0,dst_out,dst_dram_end
	jmpf	t0,dst_dma5bc
	 nop
	sub	dst_out,dst_out,dst_dram_size
dst_dma5bc:
	mov	raddr2,raddr
	call	raddr,dst_ready
	 mov	v0,dst_dma_rdys

	;		DST-DMA-State := IDLE;
	const	ddma_state,DDMAST_IDLE

	;		return 0;
	jmpi	raddr2
	 const	v0,0
	;	}

dst_dma5c:
	; 	DST-DMA-State := DST-WAIT-RD;
	const	ddma_state,DDMAST_NEED_RD

	; 	return 0;
	jmpi	raddr
	 const	v0,0


dst_dma_rdlst:

	constx	dst_dma_rdlst_p,c2b_read

dst_dma_sched_fp:

	; DST-DMA-State := DST-DMA-FP;
	const	ddma_state,DDMAST_DMA_FP

	; DMA-Client := DST;
	const	dma_client,DMA_CLIENT_DST

	cpeq	t0,dst_dma_len,0
	jmpt	t0,dst_dma6g

	; XFER-CYCLE-TO-HOST()
	 mov	raddr2,raddr
	constx	v0,OFFS_XFER
	call	raddr,dst_read_xfer
	 add	v0,v0,dst_out
	mov	raddr,raddr2

	; Schedule-FP-DMA( c2b-readlist );

		; schedule first chunk
	load	0,WORD,t1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	load	0,WORD,t2,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	srl	t3,t1,16				; t3 = chunk length
	and	t0,t1,255
	store	0,WORD,t0,p_dma_host_hi
	sub	dst_dma_rdlst_len,dst_dma_rdlst_len,8
	cpgt	t0,t3,dst_dma_blklen
	jmpf	t0,dst_dma6a
	 store	0,WORD,t2,p_dma_host_lo
	
	mov	t3,dst_dma_blklen			; chunk > blklen

	sub	dst_dma_rdlst_p,dst_dma_rdlst_p,8	; fix up so we can
	add	dst_dma_rdlst_len,dst_dma_rdlst_len,8	; continue where we
							; left off in rdlist

	add	t2,t2,t3				; addr += t3
	addc	t1,t1,0
	sll	t0,t3,16
	sub	t1,t1,t0				; len -= t3
	store	0,WORD,t1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	store	0,WORD,t2,dst_dma_rdlst_p
	sub	dst_dma_rdlst_p,dst_dma_rdlst_p,4

dst_dma6a:
	sub	dst_dma_blklen,dst_dma_blklen,t3
	add	dst_out,dst_out,t3
	add	dst_b2h_2,dst_b2h_2,t3
	add	t0,t3,4				; round up in case last & odd
	consth	t0,(DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl		; go!

	cpge	t0,dst_out,dst_dram_end			; wrap dst_out?
	jmpf	t0,dst_dma6b
	 nop
	sub	dst_out,dst_out,dst_dram_size
dst_dma6b:

		; if more, schedule block 2
	
	cpgt	t0,dst_dma_blklen,0
	cpgt	t1,dst_dma_rdlst_len,0
	and	t0,t0,t1
	jmpf	t0,dst_dma6g
	 nop
	
	; spin for command buffer ready
dst_dma6c:
	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_DCBF
	jmpt	t0,dst_dma6c
	 nop

		; schedule second chunk
	load	0,WORD,t1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	load	0,WORD,t2,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	srl	t3,t1,16				; t3 = chunk length
	and	t0,t1,255
	store	0,WORD,t0,p_dma_host_hi
	sub	dst_dma_rdlst_len,dst_dma_rdlst_len,8
	cpgt	t0,t3,dst_dma_blklen
	jmpf	t0,dst_dma6d
	 store	0,WORD,t2,p_dma_host_lo
	
	mov	t3,dst_dma_blklen			; chunk > blklen

	sub	dst_dma_rdlst_p,dst_dma_rdlst_p,8	; fix up so we can
	add	dst_dma_rdlst_len,dst_dma_rdlst_len,8	; continue where we
							; left off in rdlist

	add	t2,t2,t3				; addr += t3
	addc	t1,t1,0
	sll	t0,t3,16
	sub	t1,t1,t0				; len -= t3
	store	0,WORD,t1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	store	0,WORD,t2,dst_dma_rdlst_p
	sub	dst_dma_rdlst_p,dst_dma_rdlst_p,4

dst_dma6d:
	sub	dst_dma_blklen,dst_dma_blklen,t3
	add	dst_out,dst_out,t3
	add	dst_b2h_2,dst_b2h_2,t3
	add	t0,t3,4			; round up in case last & odd
	consth	t0,(DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl		; go!

	cpge	t0,dst_out,dst_dram_end			; wrap dst_out?
	jmpf	t0,dst_dma6f
	 nop
	sub	dst_out,dst_out,dst_dram_size
dst_dma6f:
		; if more, schedule interrupt to handle rest
	cpgt	t0,dst_dma_blklen,0
	cpgt	t1,dst_dma_rdlst_len,0
	and	t0,t0,t1
	jmpf	t0,dst_dma6g
	 nop

	or	state,state,ST_DMA_DNINT
	or	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab

dst_dma6g:
	; return 1;
	jmpi	raddr
	 cpeq	v0,v0,v0

dst_dma_incmpl_rd:
	; case DST-INCOMPLETE-RD:
	; 	if ( DST-FIFO-State == DST-FIFO-AVAIL ) {

	cpeq	t0,dfifo_state,DFIFOST_AVAIL
	jmpf	t0,dst_dma7c
	 nop

	; 		// Pick up info on block and release
	; 		// DST-FIFO machine

	; 		dst_dma_blklen = dfifo_len;
	; 		dst_dma_ulp = dfifo_ulp;
	; 		dst_out = dst_in;
	; 		dst_in += dfifo_len;
	; 		DST-FIFO-State := DST-FIFO-NONE;

	mov	dst_dma_len,dfifo_len
	mov	dst_dma_blklen,dfifo_len
	mov	dst_dma_rdys,dfifo_rdys
	mov	dst_dma_flags,dfifo_flags
	mov	dst_out,dst_in
	add	dst_in,dst_in,dfifo_len
	const	dfifo_state,DFIFOST_NONE

	; 		// previous read not done, continue where we
	; 		// left off.

	; 		DST-DMA-State := DST-DMA-FP;
	; 		DMA-Client := DST;
	; 		Schedule-FP-DMA( rdlst_p );
	; 		return 1;

	jmp	dst_dma_sched_fp
	 nop

	; 	}
dst_dma7c:
	;	else if ( DST-FIFO-State == DST-FIFO-ERR ) {

	cpeq	t0,dfifo_state,DFIFOST_ERR
	jmpf	t0,dst_dma7d
	 nop
	
	; 		Queue-B2H( B2H_IN_DONE, l=-1 );

	constx	v0,B2H_IN_DONE
	sll	t0,dst_dma_stk,16
	or	v0,v0,t0
	constn	v1,-256
	mov	raddr2,raddr
	call	raddr,queue_b2h
	 or	v1,v1,dfifo_errs

	; 		B2H-Push-Flag := TRUE;
	;		Host-Int-Flag := TRUE;

	or	state,state,(ST_B2H_PUSH|ST_HINT)

	; 		DST-DMA-State := IDLE;

	add	dst_in,dst_in,dfifo_len
	mov	dst_out,dst_in
	const	dfifo_state,DFIFOST_NONE

	const	ddma_state,DDMAST_IDLE

	;		Re-program-Readys();

	call	raddr,dst_ready
	 mov	v0,dfifo_rdys

	;		return 0;

	jmpi	raddr2
	 const	v0,0

dst_dma_bp_drop_dfl:			
	BPINCSTAT	hst_d_bp_dfl_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_no_bufx:			
	BPINCSTAT	hst_d_bp_no_pgs_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_port:			
	BPINCSTAT	hst_d_bp_port_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_auth:	
	BPINCSTAT	hst_d_bp_auth_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_job:	
	BPINCSTAT	hst_d_bp_job_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_off:	
	BPINCSTAT	hst_d_bp_off_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_op:		
	BPINCSTAT	hst_d_bp_opcode_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_vers_num:		
	BPINCSTAT	hst_d_bp_vers_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	

dst_dma_bp_drop_bad_seq:		
	BPINCSTAT	hst_d_bp_seq_err
	jmp	dst_dma_bp_drop
	 nop
	; // Drop packet	


dst_dma_bp_drop:		
	BPINCSTAT	hst_d_bp_packets
	BP_RESET_DMA_STATUS	; tell driver that no dma is in progress
	jmp	dst_dma_drop
	 nop
	; // Drop packet	


	;	}
dst_dma7d:
	; 	return 0;
	jmpi	raddr
	 const	v0,0


dst_dma_fp:
	;	should never reach here...we're going to do the state
	;	transitions in dst_dma_dn_fp.
	call	raddr,fatal
	 nop



dst_dma_dn_table:
	.word	dst_dma_dn_idle
	.word	dst_dma_dn_c2b
	.word	dst_dma_dn_le
	.word	dst_dma_dn_hd
	.word	dst_dma_dn_need_rd
	.word	dst_dma_dn_c2b_fp
	.word	dst_dma_dn_rdlst
	.word	dst_dma_dn_fp
	.word	dst_dma_dn_incmpl_rd
	.word	dst_dma_dn_bp

	; ----------------------------
	; DST-DMA-Done()
	; ---------------------------

dst_dma_dn:
	; switch ( DST-DMA-State ) {

	constx	t0,dst_dma_dn_table
	add	t0,t0,ddma_state
	load	0,WORD,t0,t0
	jmpi	t0
	 nop

dst_dma_dn_bp:

;		if(bp_state == PUSH_DESC) {
	tbit	t0, bp_state, BP_PORT_PUSH_DESC
	jmpf	t0, dst_dma_dn_bp_past_desc_dma

;		    bp_state = DISABLE_PUSH_DESC;
	  constx	t0, BP_PORT_PUSH_DESC
	andn	bp_state, bp_state, t0
;		    if (job_enabled) {
	sll	t0, bp_state, dst_bp_job
	jmpf	t0, dst_dma_dn_bp_job_err
;		    	store_dma_status(ENABLE_DESC, dst_dma_dn_port, dst_dma_dn_job);
	;; tell the driver that dest_descriptor is being dma-ed, w/ job & port number
        constx  t0, DMA_ACTIVE_DESC_DEST
	;; do this early
	  store	0,WORD,dst_bp_dq_tail_lo, p_dma_host_lo
        sll     t1, dst_bp_job, DMA_STATUS_JOB_SHIFT
        or      t0, t0, t1
        or      t0, t0, dst_bp_port
        ; store it in the dma_status word
        add     t1, bp_config, dma_status
        store   0, WORD, t0, t1

;		    	src = desc_ptr;
;		    	des = dst_dq_tail;
;			XFER-CYCLE-TOHOST();
	constx	v0, dst_bp_desc
	;; XFER-CYCLE
	mov	raddr2,raddr
	constx	t0,OFFS_XFER
	call	raddr,dst_read_xfer
	 add	v0,v0, t0
	mov	raddr,raddr2
;		    	start_desc_dma(src, des, DESTINATION, desc_len);
	;; store host dma write address and let 'er rip!
	store	0,WORD,dst_bp_dq_tail_hi, p_dma_host_hi

	constx	t0, BP_DESC_SIZE
	consth	t0, (DMAC_GO|DMAC_WR)
	store	0,WORD,t0,p_dma_host_ctl	; GO!
	
;		    	wait-for-dma-complete();
dst_dma_dn_bp_dma_wait:
	load	0,WORD,t0,p_dma_host_ctl
	tbit	t0,t0,DMAC_NOTDN
	jmpt	t0,dst_dma_dn_bp_dma_wait
	 nop
; 			inc_status(dest_desc);
	BPINCSTAT	hst_d_bp_descs
	jmp	dst_dma_dn_bp_past_desc_dma
	 nop

;		    }

dst_dma_dn_bp_job_err:	
;		    else {
;			inc_status(drop_job_err);
	BPINCSTAT	hst_d_bp_job_err
;		    }
;		}

dst_dma_dn_bp_past_desc_dma:	
;		inc_status(bp_dst_pkts);
	BPINCSTAT	hst_d_bp_packets
;                add_status(dst_pkt_len) -> bp_dst_bytes ;
	BPINC_LW  hst_d_bp_bytes, curr_dst_bp_pkt_len
;		store_dma_status(DISABLE);
	BP_RESET_DMA_STATUS
; 		break;
	

	mov	raddr2,raddr
	call	raddr,dst_ready				; reprogram ready's
	  mov	v0,dst_dma_rdys
	mov	raddr,raddr2

	jmpi	raddr
	 const	ddma_state,DDMAST_IDLE

	
dst_dma_dn_c2b:
dst_dma_dn_c2b_fp:

	; case DST-C2B:
	; case DST-C2B-FP:
	; 	XFER-CYCLE-C2B();
	; 	break;

	constx	t0,c2b+OFFS_XFER
	add	t0,t0,c2b_fetch
	constx	t1,XFER_MWTB
	store	0,WORD,t1,t0

	jmpi	raddr
	 mov	c2b_avail,c2b_fetch

dst_dma_dn_le:
	; case DST-DMA-LE:

	.if DEBUG
	cpeq	t0,dst_dma_blklen,0			; ASSERT( blklen == 0 )
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	;	Reprogram-Ready-Counter();

	mov	raddr2,raddr

	call	raddr,dst_ready
	 mov	v0,dst_dma_rdys
	
	;	Get Checksum

	constx	t2,0x1FFFF
	HWREGP	t3,XSUM_LOW
	load	0,WORD,t0,t3
	and	t0,t0,t2
	HWREGP	t3,XSUM_HIGH
	load	0,WORD,t1,t3
	and	t1,t1,t2

	const	t2,0xFFFF
	add	t1,t1,t0			; merge the two

	and	t0,dst_dma_cksum_slop,t2	; roll out dma overshoot
	xor	t0,t0,t2
	add	t1,t1,t0
	srl	t0,dst_dma_cksum_slop,16
	xor	t0,t0,t2
	add	t1,t1,t0

	srl	t0,t1,16			; fold it all
	and	t1,t1,t2
	add	t1,t0,t1
	srl	t0,t1,16
	and	t1,t1,t2
	add	t1,t0,t1

	; 	Queue-B2H( B2H_IN_DONE, len, cksum, hdrlen, pages );

	mov	v0,dst_b2h_1
	consth	v0,B2H_IN_DONE
	sll	v1,dst_dma_len,14
	call	raddr,queue_b2h_di
	 or	v1,v1,t1

	mov	raddr,raddr2

	;	if ( Asleep ) {

	tbit	t0,state,ST_SLEEP
	jmpf	t0,dst_dma_dn1b

	;		SLEEP-FLAG := False;
	 andn	state,state,ST_SLEEP
	
	;		Set-Poll-Timer();
	constx	t0,SPOLL_TM
	SET_TIMER	spoll_timer,t0
	constx	t0,DPOLL_TM
	SET_TIMER	dpoll_timer,t0

	;	}
dst_dma_dn1b:
	;	Set-Sleep-Timer();
	constx	t0,SLEEP_TM
	SET_TIMER	sleep_timer,t0

	; 	B2H-Push-Flag := TRUE;
	;	Host-Int-Flag : = TRUE;
	; 	break;

	jmpi	raddr
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

dst_dma_dn_fp:

	; case DST-DMA-FP:

	mov	raddr2,raddr

	; 	if ( dst_dma_blklen > 0 ) {

	cpgt	t0,dst_dma_blklen,0
	jmpf	t0,dst_dma_dn_fp1

	; 		ASSERT( rdlist == 0 );

	.if DEBUG
	 cpeq	t0,dst_dma_rdlst_len,0
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	; 		Queue-B2H( B2H_IN_DONE, <MORE-FLAG>, l = dst_b2h_2 );

	 or	v0,dst_dma_stk,(B2H_IN_DONE>>16)
	sll	v0,v0,16
	const	t0,B2H_ISTAT_MORE
	or	v0,v0,t0
	call	raddr,queue_b2h
	 mov	v1,dst_b2h_2
	
	; 		B2H-Push-Flag := TRUE;
	;		Host-Int-FLag := TRUE;
	or	state,state,(ST_B2H_PUSH|ST_HINT)

	; 		rdlst_len = 0;
	const	dst_dma_rdlst_len,0

	; 		DST-DMA-State := DST-WAIT-RD;

	jmpi	raddr2
	 const	ddma_state,DDMAST_NEED_RD

	; 	}
dst_dma_dn_fp1:
	; 	else if ( end-of-packet || rdlist == 0 ) {

	tbit	t0,dst_dma_flags,DWOKI_EOP
	jmpt	t0,dst_dma_dn_fp1a
	 cpeq	t0,dst_dma_rdlst_len,0
	jmpf	t0,dst_dma_dn_fp2
	 nop

dst_dma_dn_fp1a:
	;	Reprogram-Ready-Counter();
	call	raddr,dst_ready
	 mov	v0,dst_dma_rdys

	; 		Queue-B2H( B2H_IN_DONE, !EOP ? MORE-FLAG:0 );

	or	v0,dst_dma_stk,(B2H_IN_DONE>>16)
	tbit	t0,dst_dma_flags,DWOKI_EOP
	jmpt	t0,dst_dma_dn_fp1b
	 sll	v0,v0,16
	const	t0,B2H_ISTAT_MORE
	or	v0,v0,t0
dst_dma_dn_fp1b:
	call	raddr,queue_b2h
	 mov	v1,dst_b2h_2

	; 		B2H-Push-Flag := TRUE;
	;		Host-Int-Flag := TRUE;
	or	state,state,(ST_B2H_PUSH|ST_HINT)

	; 		rdlst_len = 0;
	const	dst_dma_rdlst_len,0

	; 		DST-DMA-State := DST-IDLE;

	jmpi	raddr2
	 const	ddma_state,DDMAST_IDLE

	; 	}
dst_dma_dn_fp2:
	; 	else {

	;		ASSERT( rdlist > 0 );

	.if DEBUG
	cpgt	t0,dst_dma_rdlst_len,0
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	;	Reprogram-Ready-Counter();
	call	raddr,dst_ready
	 mov	v0,dst_dma_rdys


	; 		DST-DMA-State := DST-INCMPLT-RD;
	jmpi	raddr2
	 const	ddma_state,DDMAST_INCMPL_RD

	; 	}
dst_dma_dn_hd:
	; case DST-DMA-HD:
	; 	Queue-B2H( B2H_IN );

	mov	v0,dst_dma_hdrlen
	consth	v0,B2H_IN
	sll	t0,dst_dma_stk,16
	or	v0,v0,t0
	mov	raddr2,raddr
	call	raddr,queue_b2h
	 sub	v1,dst_dma_len,dst_dma_hdrlen

	;	if ( no-D2-area ) {
	cpeq	t0,v1,0
	jmpf	t0,dst_dma_dn_hd1
	 mov	v0,dst_dma_rdys

	;		DST-DMA-State := DST-IDLE;
	;		Rerogram-Ready-Counter();

	call	raddr,dst_ready
	 const	ddma_state,DDMAST_IDLE

	;	}

dst_dma_dn_hd1:

	; 	B2H-Push-Flag := TRUE;
	;	Host-Int-Flag := TRUE;
	; 	break;

	jmpi	raddr2
	 or	state,state,(ST_B2H_PUSH|ST_HINT)

dst_dma_dn_rdlst:
	; case DST-DMA-RDLIST:
	;	XFER-CYCLE-RDLIST();
	;	break;

	constx	t0,c2b_read+OFFS_XFER
	add	t0,t0,dst_dma_rdlst_len
	constx	t1,XFER_MWTB
	jmpi	raddr
	 store	0,WORD,t1,t0

dst_dma_dn_idle:
dst_dma_dn_need_rd:
dst_dma_dn_incmpl_rd:
	call	raddr,fatal
	 nop


; //----------------------------
; // SRC-DMA()
; //----------------------------
src_dma:

	mov	raddr2,raddr			; not a leaf


	; switch ( SRC-DMA-State ) {

	; case SRC-D2B:
	; case SRC-ACTIVE:
	
	cpeq	t0,sdma_state,SDMAST_BP_ACTIVE
	jmpt	t0, src_dma_bp_active
	cpeq	t0,sdma_state,SDMAST_ACTIVE
	jmpt	t0,src_dma1
	 cpeq	t0,sdma_state,SDMAST_D2B
	jmpf	t0,src_dma2

src_dma1:

	; 	if ( d2b_chunks == 0 &&
	;	     (d2b_valid == d2b_nxt) || (d2b_nxt->hd.flags&BAD) ) {

	 cpeq	t0,d2b_chunks,0
	jmpf	t0,src_dma1c
	 cpeq	t0,d2b_nxt,d2b_valid
	jmpt	t0,src_dma1a
	 nop
	load	0,WORD,t1,d2b_nxt			; t1 = *d2b_nxt
	 tbit	t0,t1,D2B_BAD
	jmpf	t0,src_dma1c

	.if DEBUG
	tbit	t2,t1,D2B_RDY
	jmpt	t2,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

src_dma1a:
	; 		d2b_valid = d2b_nxt;
	; 		if ( DMA-State == SRC-ACTIVE ) {

	 cpeq	t0,sdma_state,SDMAST_ACTIVE
	jmpf	t0,src_dma1b
	 mov	d2b_valid,d2b_nxt

;       if(!last_swoki.SWOKI_KEEPCON)  {
 	tbit    t0,last_swoki,SWOKI_KEEPCON
 	jmpt	t0, past_look_for_bypass_job


	; // if bypass work available, do it
	;  for (i = 0; i < MAX_BP_JOBS; i++) {

	  constx	t6, BP_MAX_JOBS-2

src_dma1a_find_bp_job:
	sll	t1, bp_state, bp_cur_sjob
	jmpf	t1, src_dma1a_find_bp_job_resume
	  nop

	;; test for "is there room in WOKI" can be done in the actual place: 
	;; WOKI might get freed up by then
	

        ;     bp_cj_sdq_head = job[bp_cur_sjob].sdq_head

	;; get head of descriptor queue
	  sll	t1, bp_cur_sjob, BP_JOB_SIZE_POW2
	add	t0, bp_job_structs, t1
	add	t5, t0, (bp_cj_sdq_head-bp_job_reg_base)*4
	load	0,WORD,bp_cj_sdq_head, t5		; get head of descriptor queue

	;; bp_cj_sdq_head points at the fourth word, which (according to the
	;; application) is the opcode word!
	
	load    0, WORD, bp_s_desc_third, bp_cj_sdq_head

        ;     if( bp_cj_sdq_head->opcode == VALID) {
	constn	t0, HIP_BP_DQ_INV
	cpeq	t0, bp_s_desc_third, t0
	jmpt	t0, src_dma1a_find_bp_job_resume
	  nop

        ;             SRC-DMA_State := SRC-IDLE;
        ;             go to "case IDLE";
        ;         }

	;; we've found a bp job that has a valid opcode
	;; transition to idle state, and let processing in that state
	;; take care of the bp pkt
	constx        sdma_state,SDMAST_IDLE
	jmp	src_dma2
	  nop


src_dma1a_find_bp_job_resume:	
        ;     bp_cur_sjob++;
	add	bp_cur_sjob, bp_cur_sjob, 1

;                                         if(bp_cur_sjob == MAX_BP_JOBS)
;                                                 bp_cur_sjob = 0;
	cpeq	t2, bp_cur_sjob, BP_MAX_JOBS
	jmpf	t2, src_dma1a_past_roll
	  nop
	mov	bp_cur_sjob, zero

src_dma1a_past_roll:	
	jmpfdec	t6, src_dma1a_find_bp_job
	  nop

;	}  // end for loop
;     } // end if(!last_swoki.SWOKI_KEEPCON)
	

past_look_for_bypass_job:
	;			// no bypass work was available; try TCP/IP
	; 			Schedule-D2B-Read( 128 bytes ); // poll for RDY
	 const	v0,128
	call	raddr,sched_d2b_rd

	; 			SRC-DMA-state = SRC-D2B;
	 const	sdma_state,SDMAST_D2B
	; 			SRC-Poll-Flag := FALSE;
	andn	state,state,ST_SRC_POLL
	; 			return 1;
	jmp	src_dma9
	 cpeq	v0,v0,v0
	; 		}
	; 		else {
src_dma1b:
	; 			SRC-DMA-State := IDLE;
	const	sdma_state,SDMAST_IDLE
	; 			return 0;
	jmp	src_dma9
	 const	v0,0
	; 		}
	; 	}
	; 	else {
src_dma1c:

	;		d2bs_needed = d2b_chunks==0 ?
	;			MIN( BLKSIZE, d2b_nxt->hd.chunks )+1 :
	;			MIN( BLKSIZE, d2b_chunks ) ;

		; compute d2bs_needed in t2

	cpeq	t0,d2b_chunks,0		; t2 = (d2b_chunks==0) ? d2b_nxt->hd.chunks :
	jmpf	t0,src_dma1dd		;			 d2b_chunks;
	 mov	t2,d2b_chunks
	
	load	0,WORD,t1,d2b_nxt
	srl	t2,t1,D2B_CHUNKS_SHIFT

src_dma1dd:				; t2 = MIN( src_d2b_blksz, t2 );
	cpgt	t0,t2,src_d2b_blksz
	jmpf	t0,src_dma1de
	 sub	t0,d2b_valid,d2b_nxt

	mov	t2,src_d2b_blksz

src_dma1de:
	cpeq	t1,d2b_chunks,0		; if ( d2b_chunks==0 ) t2++
	srl	t1,t1,31
	add	t2,t2,t1

src_dma1e:
	; 		if ( d2b_valid - d2b_nxt < d2bs_needed ) {
	jmpf	t0,src_dma1ee			; modulo subtract
	 sll	t2,t2,3				; d2b_needed (t2) into bytes
	constx	t1,d2b
	sub	t1,d2b_lim,t1
	add	t0,t0,t1
src_dma1ee:
	cplt	t0,t0,t2
	jmpf	t0,src_dma1f

	; 			Schedule-D2B-Read(d2bs_needed rounded up 256);
	; 			SRC-DMA-state = SRC-D2B;
	 add	v0,t2,255
	call	raddr,sched_d2b_rd
	 andn	v0,v0,255
	const	sdma_state,SDMAST_D2B
	; 			SRC-Poll-Flag := FALSE;
	andn	state,state,ST_SRC_POLL
	; 			return 1;
	jmp	src_dma9
	 cpeq	v0,v0,v0
	; 		}
src_dma1f:

	; 		else if ( ! there-is-room ) {

	; XXX:  ( ( ADJUSTED_SRC_BLK_ADDR-src_in ) MOD SRC_BUF_SIZE ) < 128K

	HWREGP	t2,SRC_BLK_ADD
	load	0,WORD,t2,t2			; t2 = <SRC_BLK_ADDR>
	constx	t0,BLKADDR_MASK
	and	t2,t2,t0			; mask valid bits
	constx	t0,TPDRAM_BLKSIZE
	sub	t2,t2,t0			; t2 -= TPDRAM_BLKSIZE
	constx	t0,SRC_DRAM
	add	t2,t2,t0			; t2 += SRC_DRAM
	constx	t0,SRC_LOWADDR
	cplt	t0,t2,t0			; t2 < SRC_LOWADDR
	jmpf	t0,src_dma1fb
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma1fb:
	sub	t2,t2,src_in			; t2 = ( t2 - src_in )
	cple	t0,t2,0
	jmpf	t0,src_dma1fa			; (<src_blk_addr> - src_in) <= 0 ?
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma1fa:
	constx	t0,(128*1024+8192)		; XXX: 128K plus some slop
	cplt	t0,t2,t0
	jmpf	t0,src_dma1g
	 nop
	
	; 			SRC-DMA-State = SRC-FULL;
	const	sdma_state,SDMAST_FULL
	; 			return 0;
	jmp	src_dma9
	 const	v0,0
	; 		}
	; 		else {
src_dma1g:
	; 			// a continuation OR a new packet
	; 			Schedule-Next-SRC-Packet();
	call	raddr,sched_nxt_src
	; 			SRC-DMA-state = SRC-Active;
	 const	sdma_state,SDMAST_ACTIVE

	;			// push out sleep...
	;			Set-Sleep-Timer();

	constx		t0,SLEEP_TM
	SET_TIMER	sleep_timer,t0

	;			// push out delayed interrupts
	;			Set-HINT-Timer();

	constx	t0,HINT_TM
	SET_TIMER	hint_timer,t0

	; 			return 1;
	jmp	src_dma9
	 cpeq	v0,v0,v0
	; 		}
	; 	}
	; 

;         case SRC-BP-ACTIVE:
src_dma_bp_active:


;                 // if valid d2Bs available, go to src-d2b state
;               if(d2b_valid != d2b_nxt ) && (! (d2b_nxt->hd.flags&BAD))  {
	cpeq   t0, d2b_nxt, d2b_valid
	jmpt    t0, go_idle
	  nop			; if equal, it has bad parity!!!
	load    0,WORD,t1,d2b_nxt                       ; t1 = *d2b_nxt
	tbit   t0, t1, D2B_BAD
	jmpt    t0, go_idle

;                   SRC-DMA-state := SRC_d2B;
	  constx sdma_state, SDMAST_D2B

;                   return 0;
;                   }
	jmp	src_dma9
	  const	v0,0


go_idle:
;               else  {  // no valid d2Bs available, go to idle state
;                   SRC-DMA-state := IDLE;
	constx	old_src_dma_state, SDMAST_BP_ACTIVE
	constx	sdma_state,SDMAST_IDLE
;                   // save the main loop time, and just go to the IDLE state
;                   // processing instead of return 0;
;                   goto idle_state_processing;
;                   }

	;; falls through into idle_state_processing code


	
src_dma2:
; idle_state_processing:
;       case IDLE:

;               if(old_src_dma_state != BP_ACTIVE || src_poll_flag != TRUE)) {
	cpeq	t0,sdma_state,SDMAST_IDLE
	jmpf	t0,src_dma3
	;; if did a bp pkt last time thru' loop, AND src-poll-flag is true, 
	;; don't go for a bypass pkt: go for d2B instead
	cpeq	t0, old_src_dma_state, SDMAST_BP_ACTIVE
	jmpf	t0,  bp_WOKI_room_ck
	  tbit	t0, state, ST_SRC_POLL
	jmpt	t0, src_dma2_idle
	

;                   if(! enough_room_in_WOKI_queue) {
bp_WOKI_room_ck:
	  HWREGP  t0, SRC_RT
	load    0, WORD, t0, t0
	tbit    t0, t0, SRC_RT_WOKI_ROOM
	jmpt	t0, found_room_in_WOKI	
	  nop

;                       SRC-DMA-state = IDLE;
	constx 	sdma_state, SDMAST_IDLE

;                       return 0;
;                   }
	jmp   src_dma9
	  const v0,0


found_room_in_WOKI:
;                   else {  // there was room in WOKI_queue, and WOKI was empty
;                       old_src_dma_state = IDLE;
	constx	old_src_dma_state, SDMAST_IDLE

;                       for (i = 0; i < MAX_BP_JOBS; i++) {
	constx  t6, BP_MAX_JOBS-2
src_dma2_tjob_find_job:
	sll	t1, bp_state, bp_cur_sjob
	jmpf	t1, src_dma2_tjob_find_job_resume
	  nop

;                               Job->bp_sdqp = job[bp_cur_sjob].sdq_headp
	  sll	t1, bp_cur_sjob, BP_JOB_SIZE_POW2
	add	t0, bp_job_structs, t1
	add	t5, t0, (bp_cj_sdq_head-bp_job_reg_base)*4
	load	0,WORD,bp_cj_sdq_head, t5		; get head of descriptor queue

	;; ---------------------------------------
	;; | t5 = pointer to base of job structure
	;; ---------------------------------------

;                               load_last_word_of_desc(Job->bp_sdqp) -> bp_desc[3];
	load    0, WORD, bp_s_desc_third, bp_cj_sdq_head

;                               if( bp_desc[3]->opcode == VALID) {
	constn	t0, HIP_BP_DQ_INV
	cpeq	t0, bp_s_desc_third, t0
	jmpt	t0, src_dma2_tjob_find_job_resume
	  nop

;                                       if(no_room_for_one_bufx) {
	; XXX:  ( ( ADJUSTED_SRC_BLK_ADDR-src_in ) MOD SRC_BUF_SIZE ) < 16K
	HWREGP	t2,SRC_BLK_ADD
	load	0,WORD,t2,t2			; t2 = <SRC_BLK_ADDR>
	constx	t0,BLKADDR_MASK
	and	t2,t2,t0			; mask valid bits
	constx	t0,TPDRAM_BLKSIZE
	sub	t2,t2,t0			; t2 -= TPDRAM_BLKSIZE
	constx	t0,SRC_DRAM
	add	t2,t2,t0			; t2 += SRC_DRAM
	constx	t0,SRC_LOWADDR
	cplt	t0,t2,t0			; t2 < SRC_LOWADDR
	jmpf	t0,src_dma2fb
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma2fb:
	sub	t2,t2,src_in			; t2 = ( t2 - src_in )
	cple	t0,t2,0
	jmpf	t0,src_dma2fa			; (<src_blk_addr> - src_in) <= 0 ?
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma2fa:
	constx	t0,(16*1024)			; enough room for max length packet?
	cplt	t0,t2,t0
	jmpf	t0,src_dma2g
	 nop

;                                               SRC-DMA-state = IDLE;
	  const	sdma_state, SDMAST_IDLE

;                                               return 0;
	jmp	src_dma9
	 const	v0,0
;                                       }       

;                                       else  {  // there was room for another bufx
src_dma2g:
;                                               load_rest_desc(Job->bp_sdqp) ->bp_desc[0-2];
	sub	t0, bp_cj_sdq_head, FIRST_WORD_OFFSET 
	 mtsrim  CR, 3-1
	loadm	0, WORD, bp_s_desc_zeroth, t0

	;; bp_s_desc_third contains the opcode and length
	;; bp_s_desc_second get_job, slot, host and port
	;; bp_s_desc_first contains src bufx/off
	;; bp_s_desc_zeroth contains dst bufx/off


;                                               Schedule-SRC-BP-Packet();
	call	raddr, sched_bp_rd
	  nop

;                                               bp_cur_sjob++;
	add	bp_cur_sjob, bp_cur_sjob, 1

;                                               if(bp_cur_sjob == MAX_BP_JOBS)
;                                                       bp_cur_sjob = 0;
	cpeq	t2, bp_cur_sjob, BP_MAX_JOBS
	jmpf	t2, src_dma2g_past_job_wrap
	  nop
	mov	bp_cur_sjob, zero

;                                               return 1;
;                               }
src_dma2g_past_job_wrap:	
	jmp	src_dma9
	  cpeq	v0,v0,v0


src_dma2_tjob_find_job_resume:	
;                               bp_cur_sjob++;
	add	bp_cur_sjob, bp_cur_sjob, 1

;                               if(bp_cur_sjob == MAX_BP_JOBS)
;                                       bp_cur_sjob = 0;

	cpeq	t2, bp_cur_sjob, BP_MAX_JOBS
	jmpf	t2, src_dma2_tjob_past_roll
	  nop
	mov	bp_cur_sjob, zero

src_dma2_tjob_past_roll:	
;                       }  // end for loop for all jobs
	jmpfdec	t6, src_dma2_tjob_find_job
	  nop
	
;                   }	// end if room in WOKI_Q
;               }  // end if old-state non-bypass, and no src-poll-flag
	


;               else   { // old state was "bypass", and poll flag is true
src_dma2_idle:		
;                       old_src_dma_state == SDMAST_D2B;
	constx	old_src_dma_state, SDMAST_D2B

;                       if(src_poll_flag == TRUE)  { 
;                           // need check, because idle-code falls thru' if no bypass jobs

	tbit	t0,state,ST_SRC_POLL
	jmpf	t0,src_dma2z

;                           Schedule-D2B-Read( 128 bytes ); // poll for RDY
	  const	v0,128
	call	raddr,sched_d2b_rd

;                           SRC-DMA-state = SRC-D2B;
	  const	sdma_state,SDMAST_D2B

;                           SRC-Poll-Flag := FALSE;
	andn	state,state,ST_SRC_POLL

;                           return 1;
	jmp	src_dma9
	 cpeq	v0,v0,v0
;                       }  // if src-poll was true


src_dma2z:
;               SRC-DMA-state = IDLE;
	constx  sdma_state, SDMAST_IDLE

;               old_src_dma_state = IDLE;
	constx  old_src_dma_state, SDMAST_IDLE

;               return 0;
	jmp	src_dma9
	 const	v0,0


src_dma3:
	; case SRC-FULL:


	.if DEBUG
	cpeq	t0,sdma_state,SDMAST_FULL
	jmpt	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	; 	if ( there-is-room ) {

	; XXX:  ( (BLK_ADDR - BLKSZ - src_in) MOD DRAM_SIZE ) >= 128K

	HWREGP	t2,SRC_BLK_ADD
	load	0,WORD,t2,t2			; t2 = <SRC_BLK_ADDR>
	constx	t0,BLKADDR_MASK
	and	t2,t2,t0			; mask valid bits
	constx	t0,TPDRAM_BLKSIZE
	sub	t2,t2,t0			; t2 -= TPDRAM_BLKSIZE
	constx	t0,SRC_DRAM
	add	t2,t2,t0			; t2 += SRC_DRAM
	constx	t0,SRC_LOWADDR
	cplt	t0,t2,t0			; t2 < SRC_LOWADDR ?
	jmpf	t0,src_dma3aa
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma3aa:
	sub	t2,t2,src_in
	cple	t0,t2,0				; ( t2-src_in ) <= 0
	jmpf	t0,src_dma3a
	 constx	t0,SRC_BUF_SIZE
	add	t2,t2,t0			; yes, t2 += SRC_BUF_SIZE
src_dma3a:
	constx	t0,(128*1024+8192)		; XXX: 128K plus some slop
	cpge	t0,t2,t0			; t2 > 128K?
	jmpf	t0,src_dma9
	 const	v0,0			; return 0;
	
	; 		Schedule-Next-SRC-Packet();
	call	raddr,sched_nxt_src
	; 		SRC-DMA-state := SRC-Active;
	 const	sdma_state,SDMAST_ACTIVE
	; 		return 1;
	jmp	src_dma9
	 cpeq	v0,v0,v0

src_dma9:
	jmpi	raddr2
	 nop

	; }






; //-------------------
; //  SRC-DMA-Done()
; //-------------------

src_dma_dn:

	;	switch ( SRC-DMA-state ) {

	cpeq	t0, sdma_state,SDMAST_BP_ACTIVE
	jmpt	t0, src_dma_dn2
	
	;	case SRC-D2B:
	 cpeq	t0,sdma_state,SDMAST_D2B
	jmpf	t0,src_dma_dn1
	 nop

	;		XFER-CYCLE-D2B();

	constx	t0,OFFS_XFER
	add	t0,t0,d2b_valid
	constx	t1,XFER_MWTB
	store	0,WORD,t1,t0

	cpge	t0,d2b_valid,d2b_lim
	jmpf	t0,src_dma_dn0
	 nop
	
	constx	d2b_valid,d2b

src_dma_dn0:
	jmpi	raddr
	 nop

	;		break;

	;	case SRC-Active:

src_dma_dn1:
	cpeq	t0,sdma_state,SDMAST_ACTIVE
	jmpf	t0,src_dma_dn9
	 nop

	;		// Source DMA done

	add	src_in,src_in,3
	andn	src_in,src_in,3			; round up to a 32-bit word

	mov	send_blk,src_in			; save before rounding up again

	and	t0,src_in,4			; round up src_in
	add	src_in,src_in,t0
	sll	t0,t0,(D2B_BN64ALIGN_BT-2)
	or	d2b_flags,d2b_flags,t0		; set D2B_BN64ALIGN if needed

	constx	t2,SRC_DRAM+DRAM_SIZE		; wrap src_in/send_blk?
	constx	t3,SRC_BUF_SIZE
	cpge	t0,src_in,t2
	jmpf	t0,src_dma_dn1b
	 cpge	t0,send_blk,t2
	sub	src_in,src_in,t3
src_dma_dn1b:
	jmpf	t0,src_dma_dn1c
	 nop
	sub	send_blk,send_blk,t3
src_dma_dn1c:

	;	XFER-CYCLE-SOURCE();	// get out of SAM into 3port

	constx	t0,OFFS_XFER
	add	t0,t0,src_in
	constx	t1,XFER_MWTB
	store	0,WORD,t1,t0

	store	0,WORD,zero,send_blk		; fix parity in last word
						; in case SRC machine sees
						; it


	;		if ( d2b->stk == HIP_STACK_LE ) {

	and	t0,d2b_flags,0x0F
	cpneq	send_cksum_offs,t0,HIP_STACK_LE		; clr send_cksum_offs
	jmpt	send_cksum_offs,src_dma_dn1e		; if not LE

	;			if ( D2-area-not-64-bit-multiple ) {


	; what we want to know is if this TCP/IP packet
	; has a d2_area that isn't a 64 bit multiple.
	; or, ( send_blk - ( <DMA-start> + ( D2B_FN64ALIGN ? 4 : 0 ) ) )
	; is a multiple of four.  All TCP/IP packets start with an I-field
	: so the packet should be padded if the entire DMA should 
	; is a multiple of four.

	; but, since we know the DMA starts on
	; a 64 bit boundary, we simply compute:
	; ( send_blk ^ ( D2B_FN64ALIGN ? 4 : 0 ) ) & 4

	CK ( D2B_FN64ALIGN_BT >= 2 )

	 srl	t0,d2b_flags,(D2B_FN64ALIGN_BT-2)
	xor	t0,t0,send_blk
	and	t0,t0,4
	cpeq	t0,t0,0
	jmpf	t0,src_dma_dn1d
	 nop
	
	;				Pad-HIPPI-LE-packet

	add	send_blk,send_blk,4

	constx	t0,D2B_BN64ALIGN		; reset D2B_BN64ALIGN
	andn	d2b_flags,d2b_flags,t0

	and	t0,send_blk,4			; round up src_in again!
	add	src_in,send_blk,t0
	sll	t0,t0,(D2B_BN64ALIGN_BT-2)
	or	d2b_flags,d2b_flags,t0		; set D2B_BN64ALIGN if needed

	constx	t2,SRC_DRAM+DRAM_SIZE	; wrap send_blk/src_in ?
	constx	t3,SRC_BUF_SIZE
	cpge	t0,send_blk,t2
	jmpf	t0,src_dma_dn1de
	 cpge	t0,src_in,t2
	sub	send_blk,send_blk,t3
src_dma_dn1de:
	jmpf	t0,src_dma_dn1df
	 store	0,WORD,zero,send_blk	; fix parity in last word
					; in case SRC machine sees
					; it
	sub	src_in,src_in,t3
src_dma_dn1df:

	;			}
src_dma_dn1d:




	;			Get-and-Save-XUMS-regs();

	;	get 'em now...process them later

	HWREGP	t0,XSUM_LOW
	sll	send_cksum_offs,d2b_word1,16
	jmpt	send_cksum_offs,src_dma_dn1e
	 HWREGP	t1,XSUM_HIGH

	load	0,WORD,send_cksum_lo,t0
	and	send_cksum_offs,d2b_word1,0xFF		; XXX: should be 0xFFFF
							; in case some network
							; header is > 256 bytes
	load	0,WORD,send_cksum_hi,t1

	;		}

src_dma_dn1e:

	;		send_fburst = d2b.fburst;
	;		send_blk = src_in;	// flags to tell source
	;		send_chunks = d2b_chunks // stuff this woki

	srl	send_fburst,d2b_word1,18
	const	d2b_word1,0xFFFF			; use params only once

	cpgt	t0,d2b_chunks,0
	jmpi	raddr
	 or	send_flags,t0,d2b_flags

	;		break;


;       case SRC-BP-ACTIVE:
src_dma_dn2:	
;               // Source transfer of bypass packet is done

	;; sets:
	;; src_in -- then wrapped if necessary
	;; send_blk = src_in, but wrapped if needed
	;; send_flags = D2B_FN64ALIGN | D2B_BP_PKT
	;; send_fburst = 0
	;; send_cksum_off = 0x8000 0000
	;; also does XFER-CYCLE-SOURCE to clean out DRAM


;               if(src_in > data_buff_end_addr)
;                       src_in -= SRC_DATA_BUF_SIZE;
	constx	t2,SRC_DRAM+DRAM_SIZE		; wrap src_in/send_blk?
	constx	t3,SRC_BUF_SIZE
	cpge	t0,src_in,t2
	jmpf	t0,src_dma_dn2c
	  nop
	sub	src_in,src_in,t3

src_dma_dn2c:
	;	XFER-CYCLE-SOURCE();	// get out of SAM into 3port
	constx	t0,OFFS_XFER
	add	t0,t0,src_in
	constx	t1,XFER_MWTB
	store	0,WORD,t1,t0

;               *src_in = 0;    // fix parity on last word
;                               // in case sfifo machine looks at it.
	store	0,WORD,zero,src_in		

;               store_dma_status(DISABLE);
	BP_RESET_DMA_STATUS

;               add_status(cur_src_bp_pkt_len) -> bp_src_bytes;
	BPINC_LW  hst_s_bp_bytes, curr_src_bp_pkt_len

;       send_blk = src_in;
	mov	send_blk,src_in			

;               // set up state to hand packet to source fifo machine
;               send_chunks = 1
;               send_flags = FN64
	constx	send_cksum_offs,0x80000000
	constx	send_flags, D2B_FN64ALIGN | D2B_BP_PKT

;               break;
	jmpi	raddr

;               send_fburst = 0;
	  mov	send_fburst, zero


src_dma_dn9:
	call	raddr,fatal
	 nop








;-----------------------------------------------------------
; Recover Source
;	called when the source has gone into stop because of
;	an error.  It's this routine's job to clean things up.
;
;	t1 = <SOURCE Real-Time Register>;
;	t2 = <WOKI-at-top>
;-------------------------------------------------------------

recover_source:
	constx	t0,(SRC_RT_STOP_SEQ_ERR|SRC_RT_STOP_DSIC|SRC_RT_PAR_ERR|SRC_RT_CONN_LOST|SRC_RT_REJECT|SRC_RT_STOP_COMMAND)
	and	t0,t0,t1
	cpeq	t0,t0,0
	jmpf	t0,rsrc01

	tbit	t0,t1,SRC_RT_STOP_BIT_EXE	; spurious stop-bit
	jmpt	t0,src5	
	 nop

	call	raddr,fatal			; fatal: spurious stop
	 nop
rsrc01:

	 HWREGP	t0,SRC_RT_CLR				; clear the bits
	load	0,WORD,t0,t0

	tbit	t0,t1,SRC_RT_STOP_DSIC
	jmpf	t0,rsrc0a
	 nop
	
	tbit	t0,state2,ST2_SRC_NODSIC
	jmpt	t0,rsrc02
	 nop
	
	const	t0,SRC_MODE_WAIT
	HWREGP	t3,SRC_MODE_SEL
	store	0,WORD,t0,t3

	or	state2,state2,ST2_SRC_NODSIC

rsrc02:

	INCSTAT	hst_s_dsic_lost
	
	jmp	rsrc1
	 const	src_err,B2H_OSTAT_DSIC

rsrc0a:
	tbit	t0,t1,SRC_RT_STOP_SEQ_ERR
	jmpf	t0,rsrc0b


	tbit	t0,t1,SRC_RT_2020_SEQERR
	jmpt	t0,rsrc0aa
	 const	src_err,B2H_OSTAT_SEQ

	INCSTAT	hst_s_dm_seqerrs

	jmp	rsrc1
	 nop

rsrc0aa:

	tbit	t0,t1,SRC_RT_2020_SRNDS
	jmpt	t0,rsrc0ab
	 nop

	INCSTAT	hst_s_cd_seqerrs

	jmp	rsrc0ac
	 nop

rsrc0ab:

	INCSTAT	hst_s_cs_seqerrs

rsrc0ac:
	; Connection machine sequence error, need to
	; reset S2020!

	const	t0,SRC_MODE_RST
	HWREGP	t3,SRC_MODE_SEL
	store	0,WORD,t0,t3

	const	t0,SRC_MODE_WAIT
	store	0,WORD,t0,t3

	or	state2,state2,ST2_SRC_NODSIC	; recovered???

	jmp	rsrc1
	 nop

rsrc0b:
	tbit	t0,t1,SRC_RT_PAR_ERR
	jmpf	t0,rsrc0c
	 nop
	
	INCSTAT	hst_s_par_err

	jmp	rsrc1
	 const	src_err,B2H_OSTAT_SPAR

rsrc0c:
	tbit	t0,t1,SRC_RT_CONN_LOST
	jmpf	t0,rsrc0d
	 nop

	INCSTAT	hst_s_connls
	
	jmp	rsrc1
	 const	src_err,B2H_OSTAT_CONNLS

rsrc0d:							; SRC_RT_REJECT
	tbit	t0,t1,SRC_RT_STOP_COMMAND
	jmpf	t0,rsrc0e
	 nop

	INCSTAT	hst_s_timeout

	jmp	rsrc1
	 const	src_err,B2H_OSTAT_TIMEO

rsrc0e:
	const	src_err,B2H_OSTAT_REJ

	INCSTAT	hst_s_rejects


rsrc1:
	constx	t0,(SWOKI_KEEPCON|SWOKI_KEEPPKT)
	andn	last_swoki,last_swoki,t0

	; // Find end-of-connection or end-of-FIFO
rsrc1a:
	; while ( t1.WOKI_FIFO_NE && (t2.flags & NEOC) ) {

	tbit	t0,t1,SRC_RT_WOKI_NE
	jmpf	t0,rsrc2
	 tbit	t0,t2,SWOKI_KEEPCON
	jmpf	t0,rsrc2

	;	if ( t2.flags & STOP )
	;		state &= ~ST_SSTOP_PEND;

	 tbit	t0,t2,SWOKI_STOP
	jmpf	t0,rsrc1b
	 nop

	andn	state,state,ST_SSTOP_PEND

rsrc1b:

	;	pop-woki();

	HWREGP	t0,SRC_WOKI_SHIFT
	store	0,WORD,t0,t0

	;	t1 = <SOURCE Real-Time Register>;

	HWREGP	t0,SRC_RT
	load	0,WORD,t1,t0

	;	t2 =  <SRC-WOKI-AT-TOP>;

	HWREGP	t0,SRC_WOKI_RD
	jmp	rsrc1a
	 load	0,WORD,t2,t0

	; }

rsrc2:

	; // One failed woki remains at front of FIFO

	; if ( (t2.flags & NEOC) ) {

	tbit	t0,t2,SWOKI_KEEPCON
	jmpf	t0,rsrc2c

	;	// In this case, the FIFO has been drained but we
	;	// still haven't found the end-of-connection.  We
	;	// go into drain mode.

	; 	XFER-CYCLE-SRC( src_send );

	 constx	t0,OFFS_XFER
	call	raddr,src_read_xfer
	 add	v0,t0,src_send
	
	;	SOURCE-DRAIN := TRUE;
	or	state2,state2,ST2_SRC_DRAIN

	;	WHILE ( d2b_out != d2b_send ) {
rsrc2b:
	cpeq	t0,d2b_out,d2b_send
	jmpt	t0,rsrc3
	 nop

	;		Queue-B2H( ODONE, d2b_out->stk, error = ostatus );
	;		d2b_out += d2b_out->chunks+1;

	call	raddr,rsrc_odn
	 nop
	jmp	rsrc2b
	 nop
	;	}
	; }
rsrc2c:
	; else {
	;	// Get rid of rest of source epoch

	;	while ( t1.WOKI_FIFO_NE ) {

	;		t1 = <SRC-Status-Register>
	HWREGP	t0,SRC_RT
	load	0,WORD,t1,t0
	tbit	t0,t1,SRC_RT_WOKI_NE
	jmpf	t0,rsrc2ca

	;	t2_prev = t2;
	 mov	t4,t2

	 tbit	t0,t2,SWOKI_STOP
	jmpf	t0,rsrc2cb
	 nop

	andn	state,state,ST_SSTOP_PEND
rsrc2cb:
	; 		pop_woki();
	HWREGP	t0,SRC_WOKI_SHIFT
	store	0,WORD,t0,t0

	;		t2 = <SRC-WOKI-AT-TOP>;

	HWREGP	t0,SRC_WOKI_RD
	load	0,WORD,t2,t0


	;		if ( t2.tag != t2_prev.tag )
	;			break;

	xor	t4,t4,t2
	srl	t4,t4,SWOKI_TAGSHFT
	cpeq	t0,t4,0
	jmpt	t0,rsrc2c
	 nop

	;	}
rsrc2ca:
	;	if ( ! t1.WOKI_FIFO_NE ) {

	tbit	t0,t1,SRC_RT_WOKI_NE
	jmpt	t0,rsrc2d

	;		// There is nothing BEHIND the erroneous front WOKI
	;		// We've emptied (or it stopped empty) the FIFO.
	;		// Set the shift flag.

	;		XFER-CYCLE( src_send );
	 constx	t0,OFFS_XFER
	call	raddr,src_read_xfer
	 add	v0,t0,src_send

	;		Shift-Flag := TRUE;
	or	state2,state2,ST2_SRC_SHFT

	;		WHILE ( d2b_out != d2b_send ) {
	;			Queue-B2H( ODONE, d2b_out->stk, error=ostatus);
	;			d2b_out += d2b_out->chunks+1;
	;		}
	jmp	rsrc2b ; does same thing.
	 nop
	;	}

rsrc2d:

	;	// Whatever's at the top now is what we'll
	;	// restart.  ODONE all the previous (error) packets.

	;	WHILE ( d2b_out->tag <MOD t2.tag ) {
rsrc2e:
	add	d2b_out,d2b_out,4
	load	0,WORD,t0,d2b_out
	sub	d2b_out,d2b_out,4
	xor	t0,t0,t2
	srl	t0,t0,SWOKI_TAGSHFT
	cpeq	t0,t0,0
	jmpt	t0,rsrc2f
	 nop
	
	;		Queue-B2H( ODONE( 1, d2b_out->stk, error=ostatus );
	;		d2b_out += d2b_out->chunks+1;
	call	raddr,rsrc_odn
	 nop

	;	}

	jmp	rsrc2e
	 nop

	; }
rsrc2f:

	; 	XFER-CYCLE-SRC( d2b_out->??.addr );

	add	d2b_out,d2b_out,4
	load	0,WORD,t0,d2b_out
	sub	d2b_out,d2b_out,4
	sll	t0,t0,(31-DRAM_SIZE_BT)
	srl	t0,t0,(31-DRAM_SIZE_BT)
	constx	t1,(SRC_DRAM+OFFS_XFER)
	call	raddr,src_read_xfer
	 add	v0,t0,t1
	
	; }

rsrc3:
	; if ( ! SOURCE-DRAIN && ! DSIC-Lost ) {

	tbit	t0,state2,ST2_SRC_DRAIN
	jmpt	t0,src5
	 tbit	t0,state2,ST2_SRC_NODSIC
	jmpt	t0,src5
	 nop

	;	SRC-CONTROL := < PRIME | GO | <SHIFT-FLAG?> >;

	tbit	t0,state2,ST2_SRC_SHFT
	jmpf	t0,rsrc3a
	 const	t0,(SRC_CTL_PRIME|SRC_CTL_GO)
	
	or	t0,t0,SRC_CTL_SHIFT

	HWREGP	t1,SRC_WOKI_WR		; get SCZ bit set
	constx	t2,SWOKI_FLUSH
	store	0,WORD,t2,t1

	;	SHIFT-FLAG := 0;

	andn	state2,state2,ST2_SRC_SHFT

rsrc3a:
	HWREGP	t1,SRC_CONTROL
	jmp	src5
	 store	0,WORD,t0,t1
	; }



rsrc_odn:
	mov	raddr2,raddr
	load	0,WORD,t4,d2b_out			; t4 = *d2b_out

	tbit	t0,t4,D2B_NACK
	jmpt	t0,rsrc_odn0

	;		Queue-B2H( ODONE( 1, d2b_out->stk, error=ostatus );

	 and	t0,t4,0x0F				; get d2b_out->stk
	or	t0,t0,(B2H_ODONE>>16)
	sll	t0,t0,16
	or	t0,t0,1
	call	raddr,queue_b2h
	 or	v0,t0,src_err
	
	or	state,state,(ST_B2H_PUSH|ST_HINT)

rsrc_odn0:

	;		d2b_out += 8*(d2b_out->chunks + 1);
	
	srl	t4,t4,D2B_CHUNKS_SHIFT
	add	t4,t4,1
	sll	t4,t4,3
	add	d2b_out,d2b_out,t4

	cpge	t0,d2b_out,d2b_lim			; wrap d2b_out?
	jmpf	t0,rsrc_odn1

	 constx	t0,d2b
	sub	t0,d2b_lim,t0
	sub	d2b_out,d2b_out,t0
rsrc_odn1:
	;	return;
	jmpi	raddr2
	 nop


;-----------------------------------------------
;  Src-Read-XFER()
;	v0: hippi-src address
;-----------------------------------------------
src_read_xfer:
	constx	t0,XFER_RDA
	store	0,WORD,t0,v0

	tbit	t0,v0,BLKADDR_QSF
	jmpf	t0,src_read_xfer9
	
	 constx	t0,(SRC_DRAM+OFFS_XFER+DRAM_SIZE-TPDRAM_HBLKSIZE)
	cpge	t0,v0,t0
	jmpf	t0,src_read_xfer1
	
	 constx	t0,XFER_SPLRDA
	constx	v0,(SRC_LOWADDR+OFFS_XFER)
	jmpi	raddr
	 store	0,WORD,t0,v0

src_read_xfer1:
	constx	t0,TPDRAM_HBLKSIZE
	add	v0,v0,t0
	constx	t0,TAP_MASK
	andn	v0,v0,t0
	constx	t0,XFER_SPLRDA
	jmpi	raddr
	 store	0,WORD,t0,v0

src_read_xfer9:
	jmpi	raddr
	 nop

;-----------------------------------------------
;  Dst-Read-XFER()
;	v0: to-host address
;	
;	destorys: v0,t0
;-----------------------------------------------
dst_read_xfer:
	constx	t0,XFER_RDA
	store	0,WORD,t0,v0

	tbit	t0,v0,BLKADDR_QSF
	jmpf	t0,dst_read_xfer9
	
	 constx	t0,(DST_DRAM+OFFS_XFER+DRAM_SIZE-TPDRAM_HBLKSIZE)
	cpge	t0,v0,t0
	jmpf	t0,dst_read_xfer1
	
	 constx	t0,XFER_SPLRDA
	constx	v0,(DST_LOWADDR+OFFS_XFER)
	jmpi	raddr
	 store	0,WORD,t0,v0

dst_read_xfer1:
	constx	t0,TPDRAM_HBLKSIZE
	add	v0,v0,t0
	constx	t0,TAP_MASK
	andn	v0,v0,t0
	constx	t0,XFER_SPLRDA
	jmpi	raddr
	 store	0,WORD,t0,v0

dst_read_xfer9:
	jmpi	raddr
	 nop


;-----------------------------------
; dst_reset_rdy: reset ready counters.
;-----------------------------------

dst_reset_rdy:
	HWREGP	t1,DST_RESET_RDY
	store	0,WORD,t0,t1			; reset all READY stuff
	constx	t0,DST_OP_MAX_RDYS
	HWREGP	t1,DST_MAX_RDY
	store	0,WORD,t0,t1			; set max ready's

	constx	t2,DST_OP_RDY_BURSTS
	HWREGP	t1,DST_RDY_PULSER

dst_reset_rdy1:
	load	0,WORD,t0,t1			; wait for RDY pulser to zero
	and	t0,t0,255
	cpeq	t0,t0,0
	jmpf	t0,dst_reset_rdy1

	 cpgt	t0,t2,255
	jmpf	t0,dst_reset_rdy2
	 const	t0,255
	sub	t2,t2,255
	jmp	dst_reset_rdy1
	 store	0,WORD,t0,t1

dst_reset_rdy2:
	jmpi	raddr
	 store	0,WORD,t2,t1


;-----------------------------------
; dst_ready: increment ready counters.
;-----------------------------------

dst_ready:
	HWREGP	t1,DST_RDY_PULSER
	load	0,WORD,t0,t1
	and	t0,t0,255
	cpeq	t0,t0,0
	jmpf	t0,dst_ready
	
	 const	t0,1023
	add	v0,v0,t0
	srl	v0,v0,10

	.if DEBUG
	cpgt	t0,v0,255		; ASSERT( RDY < 256 );
	jmpf	t0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif ; DEBUG

	jmpi	raddr
	 store	0,WORD,v0,t1		; RDYs += ( (len + 1023) >> 10 )





;----------------------------------------------------------------------
; Interrupts and traps--  All run in Freeze mode
;----------------------------------------------------------------------



;--------------------------------------------
; Source STOP Interrupt
;	do source transfer cycle.
;--------------------------------------------

	.align	16
src_stop_int:
	; Stop due to stop bit execute

	constx	it0,XFER_RDA
	store	0,WORD,it0,src_stop_addr

	tbit	it0,src_stop_addr,BLKADDR_QSF
	jmpf	it0,src_stop_int2
	
	 constx	it0,(SRC_DRAM+OFFS_XFER+DRAM_SIZE-TPDRAM_HBLKSIZE)
	cpge	it0,src_stop_addr,it0
	jmpf	it0,src_stop_int1
	
	 constx	it0,XFER_SPLRDA
	constx	src_stop_addr,(SRC_LOWADDR+OFFS_XFER)
	jmp	src_stop_int2
	 store	0,WORD,it0,src_stop_addr

src_stop_int1:
	constx	it0,TPDRAM_HBLKSIZE
	add	src_stop_addr,src_stop_addr,it0
	constx	it0,TAP_MASK
	andn	src_stop_addr,src_stop_addr,it0
	constx	it0,XFER_SPLRDA
	store	0,WORD,it0,src_stop_addr

src_stop_int2:

		; start up source again...
	constx	it0,(SRC_CTL_PRIME|SRC_CTL_SHIFT|SRC_CTL_GO)
	HWREGP	it1,SRC_CONTROL
	store	0,WORD,it0,it1

	jmp	t_iret
	 andn	state,state,ST_SSTOP_PEND





;--------------------------------------------
; DMA Complete/Command-Buffer-Empty INTERRUPT
;
; This is actually the interrupt that says the
; first <addr,len> pair is done and the pipelined
; pair has started.  We are safe to write the
; next pair if it is a DCBE interrupt. Data Command Buffer
; Empty -- there is no more instructions for the DMA engine to
; process.

;uses the following registers to setup the next transfer
;dma_client -- SRC or DEST
;
;FOR SOURCE SIDE:	
;	d2b_nxt -- initially points to a new d2b. Routine loads the d2b, then increments
;		d2b_next to next d2b. If wrap occurs, it is set to base of d2b region.
;	
;	src_in -- initially points to tail of d2b which just finished being
;		DMAed. It is set to what will be the end after the dma is complete.
;	d2b_blkchunks -- chunks left to process before a woki word is stuffed.
;		blkchunks count the number of d2b's in the packet. blkchunks
;		try to get a full 
;
;registers that are used but not altered
;	p_dma_host_lo
;	p_dma_host_hi
;	p_dma_host_ctl -- points to dma control register
;	d2b_lim -- max absoulte address of d2b buffer
;	d2b -- base address of d2b buffer
;
;	inte_shadow
;	*p_int_enab
;
;FOR DESTINATION SIDE:		
		
;--------------------------------------------

	.align	16
dma_dn_int:

	; if ( DMA-client == SRC ) {

	cpeq	it0,dma_client,DMA_CLIENT_SRC
	jmpf	it0,dma_dn_int1
	 nop

	;	// ASSERT( d2b_blkchunks>0 );

	;	Pre-Schedule-Next-Chunk();

		; get chunk descriptor in <it1,it2>

	load	0,WORD,it1,d2b_nxt
	add	d2b_nxt,d2b_nxt,4
	load	0,WORD,it2,d2b_nxt
	add	d2b_nxt,d2b_nxt,4

;	.if DEBUG
;	sub	it0,d2b_nxt,8			; DEBUG? erase d2b
;	store	0,WORD,zero,it0
;	add	it0,it0,4
;	store	0,WORD,zero,it0
;	.endif

	
;;; load dma engine with address, length (long words) and tell it to go
;;; also set src_in to tail
	and	it3,it1,255	                ; mask to just hi addr bits
	store	0,WORD,it3,p_dma_host_hi
	srl	it1,it1,16			; it1 = d2b_nxt->length
	add	src_in,src_in,it1		; src_in += length
	store	0,WORD,it2,p_dma_host_lo
	add	it1,it1,7			; round up.
	consth  it1,(DMAC_OPSUM|DMAC_GO|DMAC_RD)
	store	0,WORD,it1,p_dma_host_ctl

	cpge	it0,d2b_nxt,d2b_lim		; check for d2b_nxt wrap-around
	jmpf	it0,dma_dn_int0a
	 nop
	constx	d2b_nxt,d2b			; wrap d2b_nxt
dma_dn_int0a:
	;	if ( --d2b_blkchunks == 0 )
	;		Disable-DMA-Done-Interrupt;

	cpgt	it0,d2b_blkchunks,1
	jmpt	it0,dma_dn_int0b		; more chunks.
	 sub	d2b_blkchunks,d2b_blkchunks,1

	andn	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab

	jmp	t_iret
	 andn	state,state,ST_DMA_DNINT	; no more done interrupts

dma_dn_int0b:
	jmp	t_iret
	 nop

	; }
dma_dn_int1:
	; else if ( DMA-Client == DST && DST-DMA-State == DST-DMA-LE ) {

	cpeq	it0,ddma_state,DDMAST_DMA_LE
	jmpf	it0,dma_dn_int3

	;    if ( DMA-Complete-Int-Enabled ) {

	 tbit	it0,inte_shadow,INT_DMA_DONE
	jmpf	it0,dma_dn_int2

	;	Clear-DMA-Complete-int-Enab;

	 andn	inte_shadow,inte_shadow,INT_DMA_DONE
	store	0,WORD,inte_shadow,p_int_enab

	;	This word is extra slop in the checksum

	load	0,WORD,dst_dma_cksum_slop,dst_dma_xfer_addr

	;	XFER-CYCLE-TOHOST();

	constx	it0,OFFS_XFER
	add	dst_dma_xfer_addr,dst_dma_xfer_addr,it0
	constx	it0,XFER_RDA
	store	0,WORD,it0,dst_dma_xfer_addr

	tbit	it0,dst_dma_xfer_addr,BLKADDR_QSF
	jmpf	it0,dma_dn_int1b
	
	 constx	it0,(DST_DRAM+OFFS_XFER+DRAM_SIZE-TPDRAM_HBLKSIZE)
	cpge	it0,dst_dma_xfer_addr,it0
	jmpf	it0,dma_dn_int1a
	 const	it0,XFER_SPLRDA

	constx	dst_dma_xfer_addr,(DST_LOWADDR+OFFS_XFER)
	jmp	dma_dn_int1b
	 store	0,WORD,it0,dst_dma_xfer_addr

dma_dn_int1a:
	constx	it1,TPDRAM_HBLKSIZE
	add	dst_dma_xfer_addr,dst_dma_xfer_addr,it1
	constx	it1,TAP_MASK
	andn	dst_dma_xfer_addr,dst_dma_xfer_addr,it1
	store	0,WORD,it0,dst_dma_xfer_addr

dma_dn_int1b:

	;	Program-Next-Chunk();

	load	0,WORD,it1,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	load	0,WORD,it2,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	constx	it0,ifhip_big_end
	cpge	it0,ifhip_big_out,it0
	jmpf	it0,dma_dn_int1c
	 mov	it0,nbpp
	constx	ifhip_big_out,ifhip_big
dma_dn_int1c:
	and	it1,it1,255
	store	0,WORD,it1,p_dma_host_hi
	cpgt	it1,dst_dma_blklen,it0
	jmpt	it1,dma_dn_int1d
	 store	0,WORD,it2,p_dma_host_lo
	mov	it0,dst_dma_blklen
dma_dn_int1d:
	sub	dst_dma_blklen,dst_dma_blklen,it0
	add	it0,it0,4
	consth	it0,(DMAC_OPSUM|DMAC_GO|DMAC_WR)
	store	0,WORD,it0,p_dma_host_ctl	; GO!

	const	it0,0x100
	add	dst_b2h_1,dst_b2h_1,it0		; increment pages field

	;	if ( none-left )

	cpgt	it0,dst_dma_blklen,0
	jmpt	it0,dma_dn_int1e
	 nop

	;		RETURN; /* make sure state bit cleared */
	jmp	t_iret
	 andn	state,state,ST_DMA_DNINT

dma_dn_int1e:
	;	Set-DMA-Cmd-Buf-Int;

	or	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab

dma_dn_int1f:
	load	0,WORD,it0,p_dma_host_ctl		; wait for command
	tbit	it0,it0,DMAC_DCBF			; buffer to empty
	jmpt	it0,dma_dn_int1f
	 nop

	;    }
dma_dn_int2:

	;	Program-Next-Chunk();

	load	0,WORD,it1,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	load	0,WORD,it2,ifhip_big_out
	add	ifhip_big_out,ifhip_big_out,4
	constx	it0,ifhip_big_end
	cpge	it0,ifhip_big_out,it0
	jmpf	it0,dma_dn_int2a
	 mov	it0,nbpp
	constx	ifhip_big_out,ifhip_big
dma_dn_int2a:
	and	it1,it1,255
	store	0,WORD,it1,p_dma_host_hi
	cpgt	it1,dst_dma_blklen,it0
	jmpt	it1,dma_dn_int2b
	 store	0,WORD,it2,p_dma_host_lo
	mov	it0,dst_dma_blklen
dma_dn_int2b:
	sub	dst_dma_blklen,dst_dma_blklen,it0
	add	it0,it0,4
	consth	it0,(DMAC_OPSUM|DMAC_GO|DMAC_WR)
	store	0,WORD,it0,p_dma_host_ctl	; GO!

	const	it0,0x100
	add	dst_b2h_1,dst_b2h_1,it0		; increment pages field

	;	if ( none-left )

	cpgt	it0,dst_dma_blklen,0
	jmpt	it0,t_iret
	 nop

	;		Disable-DMA-Cmd-Buf-Int;

	andn	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab
	jmp	t_iret
	 andn	state,state,ST_DMA_DNINT

	; RETURNINT;

	; }
dma_dn_int3:
	; else /* Must be DMA FP */ {

	.if DEBUG
	cpeq	it0,ddma_state,DDMAST_DMA_FP
	jmpt	it0,.+12
	 nop
	call	raddr,fatal
	 nop
	.endif

	;	Program-Next-Chunk();

	load	0,WORD,it1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	load	0,WORD,it2,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	srl	it3,it1,16			; it3 = chunk length
	and	it0,it1,255
	store	0,WORD,it0,p_dma_host_hi
	sub	dst_dma_rdlst_len,dst_dma_rdlst_len,8
	store	0,WORD,it2,p_dma_host_lo

	cpgt	it0,it3,dst_dma_blklen
	jmpf	it0,dma_dn_int3a
	 nop

	mov	it3,dst_dma_blklen		; chunk > blklen

		; get the DMA engine going ASAP then fiddle with table...
	add	it0,it3,4			; round up (might be last)
	consth	it0,(DMAC_GO|DMAC_WR)
	store	0,WORD,it0,p_dma_host_ctl	; go!

	sub	dst_dma_rdlst_p,dst_dma_rdlst_p,8	; fix up so we can
	add	dst_dma_rdlst_len,dst_dma_rdlst_len,8	; continue where we
							; left off in rdlst

	add	it2,it2,it3				; addr += it3
	; addc	it1,it1,0	XXXX: we can't wrap across this boundary in memory
	sll	it0,it3,16
	sub	it1,it1,it0				; len -= it3
	store	0,WORD,it1,dst_dma_rdlst_p
	add	dst_dma_rdlst_p,dst_dma_rdlst_p,4
	store	0,WORD,it2,dst_dma_rdlst_p
	jmp	dma_dn_int3b
	 sub	dst_dma_rdlst_p,dst_dma_rdlst_p,4

dma_dn_int3a:
	add	it0,it3,4			; round up (might be last)
	consth	it0,(DMAC_GO|DMAC_WR)
	store	0,WORD,it0,p_dma_host_ctl	; go!

dma_dn_int3b:
	sub	dst_dma_blklen,dst_dma_blklen,it3
	add	dst_out,dst_out,it3
	add	dst_b2h_2,dst_b2h_2,it3

	cpge	it0,dst_out,dst_dram_end	; wrap dst_out?
	jmpf	it0,dma_dn_int3c
	 nop
	sub	dst_out,dst_out,dst_dram_size
dma_dn_int3c:

	;	if ( none-left )

	cpgt	it0,dst_dma_blklen,0
	cpgt	it1,dst_dma_rdlst_len,0
	and	it0,it0,it1
	jmpt	it0,dma_dn_int3d
	 nop

	;		Disable-DMA-Cmd-Buf-Int;

	andn	inte_shadow,inte_shadow,INT_DCBE
	store	0,WORD,inte_shadow,p_int_enab
	andn	state,state,ST_DMA_DNINT

dma_dn_int3d:
	jmp	t_iret
	 nop
	; }


;----------------------------------------
; st_flags
;	update statistic flags from HW registers
;	and software flags.
;----------------------------------------

st_flags:
	const	t2,0

	tbit	t0,state2,ST2_SRC_NODSIC
	jmpt	t0,st_fl1
	 nop
	or	t2,t2,HST_FLAG_DSIC
st_fl1:
	HWREGP	t1,DST_REAL_TIME
	load	0,WORD,t1,t1
	tbit	t0,t1,DST_RT_SRCAV
	jmpf	t0,st_fl2
	 nop
	or	t2,t2,HST_FLAG_SDIC

st_fl2:	tbit	t0,op_flags,OPF_ACCEPT
	jmpf	t0,st_fl3
	 nop
	or	t2,t2,HST_FLAG_DST_ACCEPT

st_fl3:	tbit	t0,t1,DST_RT_PKOUT
	jmpf	t0,st_fl4
	 nop
	or	t2,t2,HST_FLAG_DST_PKTIN

st_fl4:	tbit	t0,t1,DST_RT_CONRQ
	jmpf	t0,st_fl5
	 nop
	or	t2,t2,HST_FLAG_DST_REQIN

st_fl5:	HWREGP	t1,SRC_RT
	load	0,WORD,t1,t1
	tbit	t0,t1,SRC_RT_2020_CNREQ
	jmpf	t0,st_fl6
	 const	t0,HST_FLAG_SRC_REQOUT
	or	t2,t2,t0
st_fl6:	tbit	t0,t1,SRC_RT_2020_CNOUT
	jmpf	t0,st_fl7
	 const	t0,HST_FLAG_SRC_CONIN
	or	t2,t2,t0
st_fl7:
       	constx	t0,(host_stat_buf+hst_flags)
	jmpi	raddr
	 store	0,WORD,t2,t0


;----------------------------------------
; Timer interrupt
;----------------------------------------

	.align	16
timerint:
	constx	it2,TIME_INTVL | TMR_IE
	mtsr	TMR,it2

	constx	it1,INT_33			; wink green LED
	load	0,WORD,it2,it1
	xor	it2,it2,INT33_LED_GREEN
	store	0,WORD,it2,it1

	cpgt	it0,src_wdog_cdown,0
	srl	it0,it0,31
	jmp	t_iret
	 sub	src_wdog_cdown,src_wdog_cdown,it0


;Work around Erratum #6 and the "Unresolved #1" that suggests aligning
;   iret instructions on quadword boundaries.
	.equ	UR_K, 1
	.rep	(8-UR_K-((./4)%4)) % 4	;align the iret
	 nop
	.endr
t_iret:
	mtsrim	CDR,0			;invalid tag
	mfsr	it0, PC0
	const	it1, 0xff0
	and	it0,it0,it1		;extra line number
	consth	it0, CIR_FSEL_TAG + CIR_RW
	CK	(. % (4*4)) == 0
	mtsr	CIR, it0		;clear column 0
	const	it1, 0x1000
	mtsrim	CDR,0
	add	it0,it0,it1
	CK	(. % (4*4)) == 0
	mtsr	CIR, it0		;clear column 1
	nop
	nop
	nop

t_iret9:
	CK	((t_iret9 - t_iret) % (4*4)) == (UR_K*4)
	CK	(. % (4*4)) == 0
	iret
	nop

;---------------------------------------
; Trap1: error conditions
;	INT_TH_PAR		(fatal)
;	INT_ATE_OVRRUN		(fatal)
;--------------------------------------

trap1:
	HWREGP	v4,INT_33
	load	0,WORD,v4,v4

	jmp	fatal2
	 const	diagfail, DIAGFAIL_TRAP1


;----------------------------------------
; Bad ints, fatal errors...
;----------------------------------------


badint:
	jmp	fatal2
	 const	diagfail, DIAGFAIL_BADINT

trap0:
	jmp	fatal2
	 const	diagfail, DIAGFAIL_DEBUG

buserr:
	mfsr	v4, CHA				; good info in event of
	mfsr	v5, CHD				; buserr.
	mfsr	v6, CHC

	jmp	fatal2
	 const	diagfail, DIAGFAIL_BUSERR

;---------------------------------------
; fatal:  software fatal error
;---------------------------------------

fatal:
	.ifdef HALT_ON_ERR
	jmp	.
	 halt
	.endif ; HALT_ON_ERR

	const	diagfail, DIAGFAIL_SOFT




fatal2:
	.ifdef HALT_ON_ERR
	jmp	.
	 halt
	.endif ; HALT_ON_ERR

	constx	it2,INT_33		; immediately turn yellow LED on.
	load	0,WORD,it1,it2		; so you can trigger O-scope with LED
	or	it1,it1,INT33_LED_YELLOW
	andn	it1,it1,INT33_LED_GREEN
	store	0,WORD,it1,it2

	mfsr	diagpc0, PC0
	mfsr	diagpc1, PC1
	mfsr	diagpc2, PC2

	mtsrim	CPS, PS_INTSOFF		; ints/traps off, but out of freeze

	mtsrim	CR, 64-1
	constx	it0, vectbl		; overwrite vectbl with registers
	storem	0,WORD,gr64,it0
	mtsrim	CR, 128-1
	constx	it0, vectbl+256
	storem	0,WORD,lr0,it0

fatal2a:				; blink out code on yellow LED
	const	it0,0
fatal2b:
	const	it1,0
fatal2c:
	add	it1,it1,1
	sll	it2,it1,10
	jmpf	it2,fatal2c
	 nop

	constx	it2,INT_33
	load	0,WORD,it1,it2
	xor	it1,it1,INT33_LED_YELLOW
	andn	it1,it1,INT33_LED_GREEN
	store	0,WORD,it1,it2
	tbit	it1,it1,INT33_LED_YELLOW
	jmpt	it1,fatal2b
	 nop
	
	add	it0,it0,1
	cplt	it1,it0,diagfail
	jmpt	it1,fatal2b
	 nop

	const	it1,0
fatal2d:
	add	it1,it1,1
	sll	it2,it1,9
	jmpf	it2,fatal2d
	 nop
	
	jmp	fatal2a
	 nop

	
; coff2firm assumes the last word is the version number
	.word	VERS

; make sure room for checksum
	CK	. < FIRM_TEXT + FIRM_SIZE - 4

	.end

