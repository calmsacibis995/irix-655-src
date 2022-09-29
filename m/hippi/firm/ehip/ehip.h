;
; ehip.h
;
; Copyright (C) 1993 Silicon Graphics, Inc.
;
; Firmware constants, definitions.  Distinct from ehiphw.h in that ehiphw.h
; contains things designated by hardware.
;

; firmware will execute in HIPPI destination memory.

	.equ	FIRM_TEXT,	(DST_DRAM)
	.equ	FIRM_SIZE,	(0x5000)	; 20K (was 18K)
	.equ	FIRM_SBSS,	(SRC_DRAM)
	.equ	FIRM_DBSS,	(DST_DRAM+FIRM_SIZE)

	.equ	SBSS_SIZE,	(0xa0000)	; 655K
	.equ	DBSS_SIZE,	0x2000		; 8 KB (was 4)

	.equ	SRC_BUF_SIZE,	(DRAM_SIZE - SBSS_SIZE)
	.equ	DST_BUF_SIZE,	(DRAM_SIZE - DBSS_SIZE - FIRM_SIZE)

; low address of (DRAM) buffer space

	.equ	SRC_LOWADDR,	(SRC_DRAM+DRAM_SIZE-SRC_BUF_SIZE)
	.equ	DST_LOWADDR,	(DST_DRAM+DRAM_SIZE-DST_BUF_SIZE)


; sizes of local d2b, c2b, b2h copies

	.equ	B2H_B_SIZE,	1024
	.equ	C2B_B_SIZE,	256
	.equ	D2B_B_SIZE,	32768


	
	
;---------------------------------
;	Bypass Job Structures ....
;  d = destination (if first letter) 
;    = descriptor  (if second letter)
;  f = free
;  l = list
;  m = map
;  q = queue
;  j = job, d = port or destination
; ex: dfl = destination freelist
; ex: dfm = destination freemap
; ex: sdq = source descriptor queue
; ex: bp_d_job = bypass port job index -- the job index associated with a 
;              particular destination buffer index (port).
;---------------------------------
	

	.equ	BP_VERSION,	00
		
	.equ	BP_LW_TO_BYTES, 3
	.equ	BP_BYTES_TO_LW, 3
	.equ	LW_ALIGN_CHECK, 28

	.equ	BP_W_TO_BYTES, 2
	.equ	BP_BYTES_TO_W, 2
	.equ	W_ALIGN_CHECK, 29

	.equ	BP_JOB_SIZE,	64
	.equ	BP_JOB_SIZE_POW2, 6
	.equ	BP_PORT_SIZE,	64
	.equ	BP_PORT_SIZE_POW2, 6


; ByPass buffer sizes and constants

        .equ    BP_DESC_SIZE, 16               ; descriptor length, bytes

	.equ    BP_FREE_IN_SIZE,  4               ; freelist index size
        .equ    BP_SDQ_SIZE,      16384           ; one page for descriptor qeueue
	.equ	BP_SDQ_SIZE_POW2, 14

        .equ    BP_SFM_ENTRIES,   4096             ; source free map size (each entry is phys addr)
        .equ    BP_SFM_SIZE,      BP_SFM_ENTRIES*BP_FREE_IN_SIZE
	.equ	BP_SFM_SIZE_POW2, 14

; Destination freemap. 
        .equ    BP_DFM_ENTRIES,   4096      ; destination freemap number of entries
        .equ    BP_DFM_SIZE,      BP_DFM_ENTRIES*BP_FREE_IN_SIZE
	.equ	BP_DFM_SIZE_POW2, 14
	
        .equ    BP_DFL_ENTRIES,   1      ; destination freelist number of entries
        .equ    BP_DFL_SIZE,      BP_DFL_ENTRIES*BP_FREE_IN_SIZE
	.equ	BP_DFL_SIZE_POW2, 14
	.equ	BP_DFL_INVALID,	  -1


        .equ    BP_MAX_JOBS,     8               ; max number of jobs
        .equ    BP_MAX_PORTS,     BP_MAX_JOBS*128 ; max bypass file descriptors

	;; number of bypass long message slots supported for incoming packets
	.equ	BP_MAX_SLOTS,	32

	;; for each slot, struct for verifying sequence num and calculating bufx
	;; <max seq number, expected sequence number)
	;; thus need 4*2 = 8 bytes
	;; make 16 for future growth (memory is cheap!)
	.equ	BP_SLOT_TABLE_ENTRY_SIZE,	16  ; to have pow 2
	.equ	BP_SLOT_TABLE_ENTRY_SIZE_POW2,   4
	.equ	BP_SLOT_TABLE_SIZE,	BP_MAX_SLOTS*BP_SLOT_TABLE_ENTRY_SIZE
	.equ	BP_SLOT_TABLE_SIZE_POW2,   9	; log2(32*16)

; Hostx map
        .equ    BP_MAX_HOSTX,  128             ; max size of ifield index map
        .equ    BP_MAX_HOSTX_POW2, 7           ; 


; Bypass status region
	.equ	BP_STATS_BUF,	32             ; number of bypass status words

;;; HCMD constants
	.equ	BP_PORT_OPCODE_SHIFT,     28    ; shift for port hcmd
	.equ	HIP_BP_PORT_DISABLE,       0
	.equ	HIP_BP_PORT_ENABLE,         1

;;; Bypass Descriptor OPCODES
	.equ	HIP_BP_INVALID_OPCODE,          0; 
	.equ    HIP_BP_DQ_INV,          -1 ; actually is 15, but this make shift unnecessary
	

;;; Bit definitions for third word of bypass descriptor
	;; bits for v4.1 opcode
	.set	NEXT_BT,24
	NXTBT	BP_OP_Z		; MBZ bit (unused)
	NXTBT	BP_OP_d		; dynamic buffers
	NXTBT	BP_OP_D		; Descriptors sent up to host
	NXTBT	BP_OP_I		; Interrupt bit asserted
	NXTBT	BP_OP_F		; "First ublock" Bit Asserted
	NXTBT	BP_OP_S		; Single Packet Mode
	NXTBT	BP_OP_X		; use physical addresses: unused in type 1 desc
	NXTBT	BP_OP_G		; "get" bit

	.equ	BP_DESC_LENGTH_MASK, 0xffffff
	.equ	BP_DESC_LEAST_BLOCKSIZE, 0x1   ; len/blksz in LSB in v4.1

;;; Bit definitions for second word of bypass descriptor
	.equ	BP_DESC_GET_JOB_MASK, 0xf8000000
	.equ	BP_DESC_SLOT_MASK,    0x07c00000
	.equ	BP_DESC_HOSTX_MASK,   0x003f0000
	.equ	BP_DESC_PORT_MASK,    0x0000ffff
	.equ	BP_DESC_HOST_PORT_MASK, 0x003fffff

	.equ	BP_DESC_SLOT_SHIFT,   22
	.equ	BP_DESC_HOSTX_SHIFT,  16


;;; Bit definitions for first word of bypass descriptor
	.equ	BP_DESC_SRC_BUFX_MASK, 0xffff0000 
	.equ	BP_DESC_SRC_OFF_MASK,  0x0000ffff 
	.equ	BP_DESC_SRC_BUFX_INCREMENT, 0x00010000

	.equ	BP_DESC_SRC_BUFX_SHIFT,  16


;;; Bit definitions for zeroth word of bypass descriptor
	.equ	BP_DESC_DST_BUFX_MASK, 0xffff0000 
	.equ	BP_DESC_DST_OFF_MASK,  0x0000ffff 
	.equ	BP_DESC_DST_BUFX_INCREMENT, 0x00010000

	.equ	BP_DESC_DST_BUFX_SHIFT,  16



;;; temporary storage area for long-bulk descriptor
        .equ    TMP_DESC_SZ_PER_SLOT_POW2,      3
        .equ    TMP_DESC_SZ_PER_SLOT,           8
        .equ    TMP_DESC_SZ_PER_JOB_POW2,       7
        .equ    TMP_DESC_SZ_PER_JOB,            128



; host system parameters

	.equ	NBPCL,		128		; cache line size in bytes

; polling timer values

	.equ	SPOLL_TM,	6000	; 200us
	.equ	DPOLL_TM,	30000	; 1ms
	.equ	SLEEP_TM,	150000	; 5ms
	.equ	HINT_TM,	60000	; 2ms

; default source timeout

	.equ	SRC_WDOG_INIT,20	; roughly 5 seconds
	.equ	DST_WDOG_INIT,20	; roughly 5 seconds (XXX:unimplemented)

; other firmware parameters

;	BE CAREFUL MODIFYING THESE!

	.equ	DST_OP_THRESHOLD,	128	; 128 bursts is our threshold
						; to start pushing data to host
						; ! if you change this, also
						; change HIPPI_DST_THRESH
						; in sys/hippidev.h !

	.equ	DST_OP_MAX_RDYS,	128	; maximum ready's outstanding
	.equ	DST_OP_RDY_BURSTS,	980	; init burst counter to this
	.equ	DST_WOKI_PAF,		276	; set woki FIFO PAF to this

	.equ	B2H_THRESHOLD,64		; we'll push B2H's after we
						; get this many (bytes)

	CK	B2H_THRESHOLD < B2H_B_SIZE

; ----------------------------------------------
; 	register definitions
; ----------------------------------------------

	.reg	it0,	u0			; trap handler temps
	.reg	it1,	u1
	.reg	it2,	u2
	.reg	it3,	u3

	.reg	zero,	gr1			; stack pointer is always zero

	lreg	dummyraddr			; do first. XXX (or
						; else raddr gets aliased)
	lreg	raddr2

	lreg	state				; software states bits:
	.set	NEXT_BT,0

	NXTBT	ST_SLEEP			; board is asleep
	NXTBT	ST_HINT				; host needs to be interrupted
	NXTBT	ST_DHINT			; host int timer is set
	NXTBT	ST_B2H_PUSH			; push out B2H's
	NXTBT	ST_SSTOP_PEND			; SRC stop pending
	NXTBT	ST_DMA_DNINT			; DMA done interrupt enabled
	NXTBT	ST_SRC_POLL	                ; enable polling of host d2b
	NXTBT	ST_DST_POLL

	lreg	state2
	.set	NEXT_BT,0
	NXTBT	ST2_SRC_DRAIN			; drain source until new conn
	NXTBT	ST2_SRC_NODSIC			; src has lost DSIC-- recover
	NXTBT	ST2_SRC_SHFT			; do src shift when restart
	NXTBT	ST2_DST_NBOP			; dst fifo is continuation
	NXTBT	ST2_HIPPI_PH			; don't try to parse headers
	NXTBT	ST2_DST_DRAIN			; drain and reset READYs.
	NXTBT	ST2_DST_FEOP			; dest "find end-of-pkt"

	lreg	op_flags
	.set	NEXT_BT,0
	NXTBT	OPF_ACCEPT			; board is accepting conn's
	NXTBT	OPF_ENB_LE			; board is accepting HIPPI-LE
	NXTBT	OPF_NODISC			; no disconnect on parity/LLRC
	NXTBT	OPF_SRC_NEOC			; status of not-end-of-connection
						; added to support bypass new state

	lreg	p_dma_host_hi			; pointers to common registers
	lreg	p_dma_host_lo
	lreg	p_dma_host_ctl
	lreg	p_int_enab

	lreg	p_regs_0ws			; base pointers to reg spaces
	lreg	p_regs_1ws			; (see HWREGP macro in ehip.h)
	lreg	p_regs_hs

	lreg	inte_shadow			; a shadow of INT_ENAB register
	lreg	host_o_cmdid			; last command ID

	lreg	sdma_state
	.equ	SDMAST_IDLE,	0
	.equ	SDMAST_D2B,	1
	.equ	SDMAST_ACTIVE,	2
	.equ	SDMAST_FULL,	3
	.equ	SDMAST_BP_ACTIVE, 4

	lreg	ddma_state			; WARNING: if you change
	.equ	DDMAST_IDLE,	0		; these values, be sure
	.equ	DDMAST_C2B,	4		; to update jump table in
	.equ	DDMAST_DMA_LE,	8
	.equ	DDMAST_DMA_HD,	12
	.equ	DDMAST_NEED_RD,	16
	.equ	DDMAST_C2B_FP,	20
	.equ	DDMAST_DMA_RDLST,24
	.equ	DDMAST_DMA_FP,	28
	.equ	DDMAST_INCMPL_RD,32
	.equ	DDMAST_DMA_BP,	36              ; this is not on dst_dma jump table
						; because it is exited immediately
						; in dst_dma_dn

	lreg	dma_client
	.equ	DMA_CLIENT_NONE,0
	.equ	DMA_CLIENT_SRC,	1
	.equ	DMA_CLIENT_DST,	2

	greg	t5				; XXX: I need more temps
	greg	t6

	greg	spoll_timer
	greg	dpoll_timer
	greg	sleep_timer
	greg	hint_timer			; to-host interrupt timer

	.equ	DEFAULT_NBPP, 16384		; default bytes per page
	greg	nbpp				; NBPP (page size) of host
	greg	mlen				; MLEN (mbuf size) of host


; ----------------   source ---------------------

;	There's a large D2B area in physical memory on the host. (32K)
;	We have a similarly sized D2B area on board.  This keeps it
;	simple in that we can quickly figure out host addresses
;	from local addresses.
;
; d2b_nxt:
;	Current head or chunk DMA engine is working on.  This is the
;	address in the d2b area on-board.
;
; d2b_valid:
;	Just past the last d2b entry that has been DMA'ed from the host.
;	This is an address in the on-board d2b area.
;
; d2b_lim:
;	One entry past the last location of the on-board d2b area.
;	(Corresponds to the end of the d2b area in the host.)
;
; d2b_flags:
; d2b_word1:
;	Current D2B head we're working on.
;
; d2b_chunks:
;	Chunks left in entire D2B we're working on.
;	
; d2b_blkchunks:
;	Chunks left before we'll stuff a woki (we send out 128K blocks)
;
; d2b_send:
;	Current head we're working on.  Incremented when last WOKI for
;	packet is stuffed.
;
; d2b_out:
;	Head of earliest D2B that hasn't been completely transmitted
;	yet.

	lreg	d2b_nxt
	lreg	d2b_send
	lreg	d2b_out
	lreg	d2b_valid
	lreg	d2b_lim

	lreg	d2b_flags
	lreg	d2b_word1	; cksum/fburst
	lreg	d2b_chunks
	lreg	d2b_blkchunks

	.equ	SRC_D2B_BLK,131072		; 128K sent at a time

	; How many D2B's before we'll stuff a WOKI?  (SRC_D2B_BLK / nbpp)

	lreg	src_d2b_blksz

	lreg	src_in				; after last word DMA'ed in
	lreg	src_dmaed			; first word of current d2b DMA
	lreg	src_send			; after last word snd attempted

	lreg	src_err				; last send error

	lreg	src_tag_in			; tag we'll use for D2B
	lreg	src_stop_addr			; Stop WOKI will force xfer
						; at this address.
	lreg	last_swoki

	lreg	send_blk			; info handed to source machine
	lreg	send_flags			; after src DMA is complete.
	lreg	send_fburst
	lreg	send_cksum_offs	                ; if msb=1, then don't put chksum in packet
	lreg	send_cksum_lo
	lreg	send_cksum_hi

	lreg	b2h_sn
	lreg	b2h_queued			; bytes of b2h queued
	lreg	b2h_h_offs			; offset into hosts b2h area
	lreg	b2h_h_lim			; offset past last b2h entry
						; before wrapping

	greg	src_wdog_blktag			; last blk/tag of src FIFO front
	greg	src_wdog_cdown			; countdown timer to dump
	greg	src_wdog_tval			; timeout value
	greg	src_wdog_bp_pkts		; counter to show bypass is
						; making progress
	greg	src_wdog_old_bp_pkts		; previous bp pkt count
	
; ----------------   destination FIFO state machine variables ---------------------

	lreg	c2b_hnxt			; offset into host of next c2b
	lreg	c2b_fetch			; how much c2b DMAing
	lreg	c2b_avail			; how much c2b available

	lreg	ifhip_sml_in
	lreg	ifhip_sml_out
	lreg	ifhip_big_in
	lreg	ifhip_big_out

	lreg	dfifo_state
	.equ	DFIFOST_NONE,		0
	.equ	DFIFOST_FLUSH,		1
	.equ	DFIFOST_AVAIL,		2
	.equ	DFIFOST_ERR,		3

	lreg	dfifo_len	        ; number of bytes in packet
	lreg	dfifo_flags		; DWOKI_IFIELD,DWOKI_EOP,DWOKI_THRESH,
	DEFBT	DFIFOFL_NBOP,		28
	lreg	dfifo_ulp
	lreg	dfifo_d2_size
	lreg	dfifo_ifield
	lreg	dfifo_hdrlen
	lreg	dfifo_rdys
	lreg	dfifo_residue

	lreg	dfifo_errs

	lreg	dst_in
	lreg	dst_out

	lreg	dst_dma_len
	lreg	dst_dma_blklen
	lreg	dst_dma_ulp
	lreg	dst_dma_hdrlen
	lreg	dst_dma_ifield
	lreg	dst_dma_flags
	lreg	dst_dma_stk
	lreg	dst_dma_rdys

	lreg	dst_dma_xfer_addr
	lreg	dst_dma_cksum_slop

	lreg	dst_dma_rdlst_len
	lreg	dst_dma_rdlst_p

	lreg	dst_dram_end
	lreg	dst_dram_size

	lreg	dst_b2h_1
	lreg	dst_b2h_2

	lreg	dst_wdog_cdown			; countdown timer to dump
	lreg	dst_wdog_tval			; timeout value

;;; Bypass registers for destination pipeline
;;; These are used to hand parameters from dst_dma FSM to dst_dma_dn routine

	lreg	dst_bp_port
	lreg	dst_bp_job
	lreg	dst_bp_dq_tail_lo
	lreg	dst_bp_dq_tail_hi
	

	greg	n_tohost_ints
	greg	dst_seqeword_addr	; debug
	greg	dst_statblock		; debug
	greg	dst_weirdwokis		; debug
	greg	dst_drained_wokis	; debug

; -------- useful macros ---------------------------------------

;interval for 29K timer
	.equ	TIME_INTVL, 0x7FFFFF	; 254ms
	.equ	TIME_INTVL_BT, 22
	CK	TIME_INTVL>0x10000
	CK	TIME_INTVL<(1<<24)

	.equ	TIME_USEC,33		; ticks per usec (proc speed in Mhz)

;timer macros.  these timers only work if polled at least every 1/2 of
; TIME_INTVL.  intervals cannot be any longer than 1/2 of TIME_INTVL.
;
 .macro	SET_TIMER, timer, intvlreg
	mfsr	timer,TMC
	sub	timer,timer,intvlreg
 .endm

 .macro CHECK_TIMER, result, timer
	mfsr	result,TMC
	sub	result,result,timer
	sll	result,result,(31-TIME_INTVL_BT) ; shift sign into bit 31
 .endm


; HWREGP- macro to produce one instruction that generates a
; a hardware register address.  Three base registers are kept
; around and offset is merely added to one of them.

 .macro	HWREGP, pointr,hwreg
	.if ( (hwreg) >= REGS_0WS && (hwreg) <= REGS_0WS+255 )
		add	pointr,p_regs_0ws,(hwreg - REGS_0WS)
	.else
	 .if ( (hwreg) >= REGS_1WS && (hwreg) <= REGS_1WS+255 )
		add	pointr,p_regs_1ws,(hwreg - REGS_1WS)
	 .else
	  .if ( (hwreg) >= REGS_HS && (hwreg) <= REGS_HS+255 )
		add	pointr,p_regs_hs,(hwreg - REGS_HS)
	  .else
		.print "*** Can't reach register address @hwreg@."
		.err
	  .endif
	 .endif
	.endif
 .endm



; ----------------- HIPPI-FP and other protocol stuff -------------

	.equ	HIPPI_FP_ULP_SHIFT,	24
	DEFBT	HIPPI_FP_FLAGS_P,	23
	DEFBT	HIPPI_FP_FLAGS_B,	22
	.equ	HIPPI_FP_D1SIZE_MASK,	0x7F8

	.equ	HIPPI_ULP_MI,	2
	.equ	HIPPI_ULP_LE,	4
	.equ	HIPPI_ULP_IPIM,	6
	.equ	HIPPI_ULP_IPIS,	7

	.equ	IP_PROTO_TCP,	6
	.equ	IP_PROTO_UDP,	17



;------------------- Board to Host Interface ------------------------

;	needs to be kept in sync with sys/hippihw.h (driver)



;shape of the host-communications area

    ;host-written/board-read portion of the communications area
	.dsect
hc_cmd:		.block  4			;one of the hip_cmd enum

        .equ	HCMD_NOP,		0
        .equ	HCMD_INIT,		1
        .equ	HCMD_EXEC,		2
        .equ	HCMD_FLAGS,		3
        .equ	HCMD_WAKEUP,		4
        .equ	HCMD_ASGN_ULP,		5
        .equ	HCMD_DSGN_ULP,		6
        .equ	HCMD_STATUS,		7
        .equ	HCMD_JOB,		8
        .equ	HCMD_PORT,		9
        .equ	HCMD_BP_CONFIG,		10

hc_cmd_id:	.block  4			;ID used by board to ack cmd

hc_cmd_data:
hc_cmd_exec:
		.block  64

	.equ	HIP_DWN_LEN,8

    ;bits in init.flags
	DEFBT	HIP_FLAG_ACCEPT,0	;accept hippi connections
	DEFBT	HIP_FLAG_IF_UP,	1	;TCP/IP networking is ON
	DEFBT	HIP_FLAG_NODISC,2	;don't disconnect on parity/LLRC err

    ;host-read/board-written part of the communications area

hc_sign:	.block  4			;board signature
	.equ	HIP_SIGN,	0xBeadFace	;all is well
hc_vers:	.block  4			;board/eprom version
	DEFBT	CKSUM_VERS,31			;bad checksum

hc_inum:	.block  4			;++ on board-to-host interrupt

hc_cmd_ack:	.block  4			;ID of last completed command

hc_cmd_res:	.block  64			;minimize future version
						;problems

    ;command read and write lengths
	.equ    HC_CMD_RSIZE, (hc_sign - hc_cmd)
	.equ	HC_CMD_WSIZE, (. - hc_sign)
	.equ	HC_CMD_SIZE,  (. - hc_cmd)

	.use	*


;Host signature to board
	.equ	HOST_SIGN,	0xEE4400EE


;ifhip parameter definitions

	.equ	MIN_BIG,	16
	.equ	MAX_BIG,	128
	.equ	MIN_SML,	2
	.equ	MAX_SML,	64

	.equ	MAX_LE_D2SIZE,	(65535+8)	; max D2_size in HIPPI-LE

; HIPPI stack numbers

	.equ	 HIP_STACK_LE,		0       ; ifnet TCP/IP interface
	.equ	 HIP_STACK_IPI3,	1       ; IPI-3 command set driver
	.equ	 HIP_STACK_RAW,		2       ; raw device interface
	.equ	 HIP_STACK_FP,		3       ; HIPPI-FP ULP device
	.equ	 HIP_N_STACKS,		16




;
; host-to-board control requests (C2B)
;
; XXX: there's a jump table in the firmware--
;	be sure it stays correct

	.equ	C2B_OPMASK,	0xF0	; mask for op
	.equ	C2B_STMASK,	0x0F	; mask for dma "stack"
	.equ	C2B_OP_SHIFT,	8
	.equ	C2B_EMPTY,	(0*16)
	.equ	C2B_SML,	(1*16)	;add little mbuf to pool
	.equ	C2B_BIG,	(2*16)	;add little mbuf to pool
	.equ	C2B_WRAP,	(3*16)	;back to start of buffer
	.equ	C2B_READ,	(4*16)	;post a read to a ULP

	.equ	MAX_DMA_ELEM,	19	;maximum chunks in 1 host to hippi DMA

	.equ	MAX_OUTQ,	25	;XXX: longer?


;
; board-to-host requests (B2H)
;
	.equ	B2H_SLEEP,	1*0x100000	;board has run out of work
	.equ	B2H_ODONE,	2*0x100000	;output DMA commands finished
	.equ	B2H_IN,		3*0x100000	;hdr DMA'ed, input available
	.equ	B2H_IN_DONE,	4*0x100000	;input DMA finished
	.equ	B2H_PORTINT,	5*0x100000	;bypass destination port interrupt

	.equ	B2H_OPMASK,	  0xF00000	;op mask
	.equ	B2H_STMASK,	  0x0F0000	;stack associated with b2h
	.equ	B2H_SN_SHIFT,	24		; bits left to shift sn

	.equ	B2H_OSTAT_SHIFT,8

	.equ	B2H_OSTAT_GOOD,	0*0x100
	.equ	B2H_OSTAT_SEQ,	1*0x100		; HIPPI sequence error
	.equ	B2H_OSTAT_DSIC,	2*0x100		; DSIC lost
	.equ	B2H_OSTAT_TIMEO,3*0x100		; connection timed-out
	.equ	B2H_OSTAT_CONNLS,4*0x100	; connection lost
	.equ	B2H_OSTAT_REJ,	5*0x100		; connection rejected
	.equ	B2H_OSTAT_SHUT,	6*0x100		; interface shut down before sent
	.equ	B2H_OSTAT_SPAR,	7*0x100		; SRC detected parity err (bad)

	.equ	B2H_ISTAT_I,	0x8000		; B2H_IN: I-field in front
	.equ	B2H_ISTAT_MORE,	0x4000		; B2H_IN_DONE: not end-of-pkt

	DEFBT	B2H_IERR_PARITY,	0	; DST parity error
	DEFBT	B2H_IERR_LLRC,		1	; DST LLRC error
	DEFBT	B2H_IERR_SEQ,		2	; DST sequence error
	DEFBT	B2H_IERR_SYNC,		3	; DST sync error
	DEFBT	B2H_IERR_ILBURST,	4	; DST illegal burst
	DEFBT	B2H_IERR_SDIC,		5	; DST SDIC lost


;
; data-to-board requests (D2B)
;
	.equ	D2B_CHUNKS_SHIFT,16
	DEFBT	D2B_RDY,	15
	DEFBT	D2B_BAD,	14
	DEFBT	D2B_IFLD,	13
	DEFBT	D2B_NEOC,	12
	DEFBT	D2B_NEOP,	11
	DEFBT	D2B_NACK,	10		; don't bother acking output
	DEFBT	D2B_FN64ALIGN,	9		; a pseudo flag set internally
						;;  dummy word at front
	DEFBT	D2B_BN64ALIGN,	8		; a pseudo flag set internally
						;;  dummy word at end

	DEFBT	D2B_BP_PKT,	7               ; pseudo flag just used when
						; internally to tell source
						; engine pkt is a bypass pkt.
	DEFBT	D2B_UNKNOWN,	0


;--------------------------------

	lreg	diagfail			; failure code
	lreg	diagpc0				; attempt to get PC of failure
	lreg	diagpc1
	lreg	diagpc2

	.equ	DIAGFAIL_SOFT,	1		; software failure
	.equ	DIAGFAIL_BADINT,2		; unknown trap/interrupt
	.equ	DIAGFAIL_SRC,	3		; source memory test failed
	.equ	DIAGFAIL_DST,	4		; destination memory test fail
	.equ	DIAGFAIL_EEPROM,5		; flash eeprom cksum error
	.equ	DIAGFAIL_DEBUG, 6		; got debug interrupt
	.equ	DIAGFAIL_TRAP1,	7		; got a TRAP1
	.equ	DIAGFAIL_BUSERR,8		; 29K got BERR


; -------- bypass registers -----------------------------------

        ; enable on a per job basis (job 0 in bit 0, etc)
	;; bypass state is of form
	;; 
	;;  31,30,29			15		7	0
	;; -----------------------------------------------------
	;; |j0|j1|j2|....	j14|j15	|.... flags     |ulp	|
	;; *----------------------------------------------------*
	;;
	;; where j0 is job enable for job 0, ulp is the ulp number for the bypass
	;; 

	.equ	BP_PREFETCH_THRESHOLD, 64
	lreg    bp_state
        .equ    BP_ULP_MASK,         0x00ff ; mask for ulp
	.equ	BP_JOB_MASK,	     0xff000000; leave room for op flags
	.set	NEXT_BT,8
	NXTBT	BP_PORT_PUSH_DESC     ; push desc to host when data dma done

	lreg	bp_cur_sjob	; used to loop thru' all jobs in bypass

	; bypass source descriptor words
	lreg	bp_s_desc_zeroth 
	lreg	bp_s_desc_first		
	lreg	bp_s_desc_second
	lreg	bp_s_desc_third

	;; the above is going to be the order of regs. in 
	;; sched_bp_rd; however, because of the way application
	;; uses the bypass (which, in turn, is an artifact of the 
	;; DMA engine always going from low to high addr) it have
	;; the opcode when 

	.equ	FIRST_WORD_OFFSET,	12
	.equ	THIRD_WORD_OFFSET,	4

	lreg	bp_job_structs
	lreg	bp_port_structs


;;current job state -- this dictates the order of the structure in memory

	.equ	bp_job_reg_base, NEXT_LREG+128
	.equ	HCMD_JOB_ENABLE, 1
	lreg	bp_cj_sdq_head
        lreg    bp_cj_sdq_base
        lreg    bp_cj_sdq_end

        ;; breakup of bp_cj_src_hostport::
        ;; 15 :  0 :: ack_port
        ;; 23 : 16 :: don't care
        ;; 31 : 24 :: ack_host
        lreg    bp_cj_ack_hostport
        .equ    BP_ACK_PORT_MASK,       0xffff
        .equ    BP_ACK_HOST_SHIFT,      24
        .equ    BP_ACK_HOST_MASK,       0xff

	lreg	bp_cj_bufx_entry_size	; page size in bytes 

        lreg    bp_cj_auth0
        lreg    bp_cj_auth1
        lreg    bp_cj_auth2

	lreg	bp_cj_job	; used to get the job idx in enable/disable from driver


	.equ	bp_job_reg_end, NEXT_LREG+128
	.equ	BP_TOTAL_JOB_REGS, bp_job_reg_end - bp_job_reg_base
	

;; current port state

	.equ	bp_port_reg_base, NEXT_LREG+128
	lreg	bp_cp_state
	.set	NEXT_BT,0
	NXTBT	BP_PORT_VAL	; is it a valid port
	NXTBT	BP_PORT_PGX	; do we have a valid page for short msg
	NXTBT	BP_PORT_INT_PENDING ; any pending interrupts to port

	lreg	bp_cp_job	      ; job port is attached to

	lreg	bp_cp_d_data_tail     ; ptr to next location to put data
	lreg	bp_cp_data_bufx	      ; page index of current data page

	lreg	bp_cp_dq_base_lo      ; desc queue base addr
	lreg	bp_cp_dq_base_hi
	lreg	bp_cp_dq_size	      ; desc queue size
	lreg	bp_cp_dq_tail	      ; ptr to next location to put desc
	lreg	bp_cp_int_cnt	      ; interrupt count

	lreg	bp_cp_port	      ; current port number

		
	.equ	bp_port_reg_end, NEXT_LREG+128
	.equ	BP_TOTAL_PORT_REGS, bp_port_reg_end - bp_port_reg_base

	lreg	bp_cp_d_data_base_hi  ; data pages
	lreg	bp_cp_d_data_base_lo  
	.equ	BP_FM_SHIFT_LO, 14
	.equ	BP_FM_SHIFT_HI, 18

	;; for double buffering dma etc.
	greg	bp_dbl_dma_d_data_base_hi  
	greg	bp_dbl_dma_d_data_base_lo  

        ;; for holding the slot num (used in dfl determination)
	greg	bp_glob_slot

        ;; for holding slot specific info
	greg	bp_slot_max_seqnum
	greg	bp_slot_exp_seqnum


;---------------------------------
;	ByPass Packet Format...
;---------------------------------

	.dsect
	
BP_PKT_DUMMY:	.block 4
BP_PKT_IFIELD:	.block 4
BP_PKT_FP_HI:	.block 4
	.equ	HIP_BP_FP_BASE,    0x00800030  
	; P=1, B=0, D1_size = 6lw, D2_off=0 :: see page 56 in HiPPI manual
;       // pkt->fp.ulp = fs->ulp;
;       // pkt->fp.p = 1;
;       // pkt->fp.b = 0;
;       // pkt->fp.reserved = 0;
;       // pkt->fp.d1_size = D1_SIZE_LONGS;
;       // pkt->fp.d2_offset = 0;


BP_PKT_FP_LO:	.block 4

BP_PKT_D1_START:	
BP_PKT_FIRST_WORD:	.block 4
	;; bits for v4.1 opcode are defined in the desc. words defn.
	.equ	BP_PKT_OPCODE_MASK,	0xff000000
	.equ	BP_PKT_SRC_HOSTX_MASK,	0x003f0000
	.equ    BP_PKT_SRC_PORT_MASK,    0x0000ffff

	.equ	BP_PKT_SRC_HOSTX_SHIFT,  16
	

BP_PKT_SECOND_WORD:	.block 4
	.equ	BP_PKT_GET_JOB_MASK, 0xf8000000
	.equ	BP_PKT_SLOT_MASK,    0x07c00000
	.equ	BP_PKT_DST_HOSTX_MASK,   0x003f0000
	.equ	BP_PKT_DST_PORT_MASK,    0x0000ffff
	.equ	BP_PKT_VERSION_MASK, 0x00c00000

	.equ	BP_PKT_SLOT_SHIFT,   22
	.equ	BP_PKT_DST_HOSTX_SHIFT,  16
	.equ	BP_PKT_VERSION_SHIFT,  22


BP_PKT_THIRD_WORD:	.block 4	; offset in bytes for destination
	.equ	BP_PKT_SEQNUM_MASK,    0x00ffffff
	
	;; breakup is 16 bits of don't care and 16 bits of dest offset
	;; can be replaced directly from 3rd word of src descriptor
BP_PKT_SRC_BUFX:	.block 4
BP_PKT_SRC_OFF:		.block 4

BP_PKT_DST_BUFX:	.block 4
BP_PKT_DST_OFF:		.block 4

BP_PKT_AUTH0:	.block 4
BP_PKT_AUTH1:	.block 4
BP_PKT_AUTH2:	.block 4

BP_PKT_CHKSUM:	.block 4
BP_PKT_PADDING:	.block 4

BP_PKT_D1_END:	
	.equ	BP_PKT_HEADER_LEN, BP_PKT_D1_END
	.equ	BP_D1_LEN, BP_PKT_HEADER_LEN - BP_PKT_D1_START

        .use    *


;---------------------------------
;	Statistics Counters...
;---------------------------------

	.dsect
hst_flags:	.block 4
	DEFBT	HST_FLAG_DSIC,		0
	DEFBT	HST_FLAG_SDIC,		1
	DEFBT	HST_FLAG_DST_ACCEPT,	4
	DEFBT	HST_FLAG_DST_PKTIN,	5
	DEFBT	HST_FLAG_DST_REQIN,	6
	DEFBT	HST_FLAG_SRC_REQOUT,	8
	DEFBT	HST_FLAG_SRC_CONIN,	9

hst_s_conns:	.block 4	
hst_s_packets:	.block 4
hst_s_rejects:	.block 4
hst_s_dm_seqerrs:.block 4
hst_s_cd_seqerrs:.block 4
hst_s_cs_seqerrs:.block 4
hst_s_dsic_lost:.block 4
hst_s_timeout:	.block 4
hst_s_connls:	.block 4
hst_s_par_err:	.block 4
hst_s_resvd:	.block 6*4

hst_d_conns:	.block 4
hst_d_packets:	.block 4
hst_d_badulps:	.block 4
hst_d_ledrop:	.block 4
hst_d_llrc:	.block 4
hst_d_par_err:	.block 4
hst_d_seq_err:	.block 4
hst_d_sync:	.block 4
hst_d_illbrst:	.block 4
hst_d_sdic_lost:.block 4
hst_d_nullconn:	.block 4
hst_d_resvd:	.block 5*4

	.equ	HST_STATS_SIZE,.-hst_flags

        .use    *


;---------------------------------
;	Bypass Statistics Counters...
;---------------------------------

	.dsect

hst_bp_job_vect:		.block 4
hst_bp_ulp:			.block 4
hst_s_bp_descs:			.block 4
hst_s_bp_packets:		.block 4
;;; number of bytes sent out by source
hst_s_bp_bytes:			.block	4*2

;; SOURCE errors

;; hostx was out of bounds	
hst_s_bp_desc_ifield_err:	.block 4

;;; page index was out of bounds
hst_s_bp_desc_bufx_err:		.block 4

;;; invalid opcode
hst_s_bp_desc_opcode_err:	.block 4
	
;;; packet offset + length crosses a page boundary
hst_s_bp_desc_addr_err:		.block 4	

hst_s_bp_dummy:			.block 4*6

		
hst_d_bp_descs:		.block 4
hst_d_bp_packets:	.block 4

;;; number of bytes received out by dest
hst_d_bp_bytes:		.block	4*2


;;; DESTINATION errors
;;; destination buffer is not in bounds or port not enabled
hst_d_bp_port_err:	.block 4

;;; job not enabled
hst_d_bp_job_err:	.block 4

;;; no pages available on destination freelist 
hst_d_bp_no_pgs_err:	.block 4

;;; page index on freelist is invalid
hst_d_bp_dfl_err:	.block 4

;;; received authentication did not match job authentication
;;; for this destination buffer
hst_d_bp_auth_err:	.block 4
	
;;; FIXED or SUB_16K_LONG opcode -- offset plus packet length would 
;;; cross a page boundary
hst_d_bp_off_err:	.block 4

;;; received opcode was invalid
hst_d_bp_opcode_err:	.block 4

;;; version number mismatch
hst_d_bp_vers_err:	.block 4

;;; unexpected seq. number
hst_d_bp_seq_err:	.block 4

hst_d_bp_dummy:		.block 4*3


	
	.equ	HST_BPSTATS_SIZE,.-hst_bp_job_vect

        .use    *


;;; configuration region for bypass

        greg    bp_config
        .dsect

        ; setup of the bypass config structure
        num_jobs:       .block 4
        num_ports:      .block 4
        hostx_base:     .block 4
        hostx_size:     .block 4
        dfl_base:       .block 4
        dfl_size:       .block 4
        sfm_base:       .block 4
        sfm_size:       .block 4
        dfm_base:       .block 4
        dfm_size:       .block 4
        bpstat_base:    .block 4
        bpstat_size:    .block 4
        sdq_base:       .block 4
        sdq_size:       .block 4
        bpjob_base:     .block 4
        bpjob_size:     .block 4
        dma_status:     .block 4
        reserved:       .block 15*4

;; the definition of the bits of the "dma_status" word:
;; struct dma_status {
;;      __uint32_t      active  : 1;    /* 1 = dma is active */
;;      __uint32_t      data    : 1;    /* 1 = data dma, 0 = descriptor dma */
;;      __uint32_t      dest    : 1;    /* 1 = dest, 0 = src */
;;      __uint32_t      2_pgs   : 1;    /* 1 = dma spans 2 pgs, 0 if 1-pg */
;;      __uint32_t      resrvd  : 4;    /* for future */
;;      __uint32_t      job     : 8;    /* job number */
;;      union {
;;        __uint32_t    bufx     : 16;   /* page index if data dma */
;;        __uint32_t    port    : 16;   /* port if descriptor dma */
;;      } d;
;; }
;; data    dest    job     d
;; 0       0       <job#>  <port#> -- invalid
;; 0       1       <job#>  <port#> -- desc dest dma on job/port
;; 1       0       <job#>  <bufx#>  -- data src  dma on job/bufx
;; 1       1       <job#>  <bufx#>  -- data dest dma on job/bufx



	.equ	DMA_INACTIVE,		0
	;; DMA_ACTIVE_DESC_SRC is invalid
	.equ	DMA_ACTIVE_DATA_SRC,	0xc0000000
	.equ	DMA_ACTIVE_DESC_DEST,	0xa0000000
	.equ	DMA_ACTIVE_DATA_DEST,	0xe0000000
	.equ	DMA_STATUS_2_PG_DMA,    0x10000000
	.equ	DMA_STATUS_JOB_SHIFT,	16
	.equ	DMA_STATUS_JOB_MASK,	0xff
	.equ	DMA_STATUS_PORT_PGX_MASK,  0xffff
	

    ;command read and write lengths
        .equ   BP_CONFIG_REGION, (. - num_jobs)

        .use    *




	
        greg    stat_p                          ; pointer to statistics area
        greg    stat_t0                         ; temp reg
        greg    stat_t1                         ; temp reg
        greg    stat_t2                         ; temp reg


        ; note: I use this macro in brach delay slots so
        ;       the first instruction needs to be pretty innocuous

	.macro	INCSTAT, statname
	        add     stat_t0,stat_p,statname
	        load    0,WORD,stat_t1,stat_t0 
	        add     stat_t1,stat_t1,1
		store   0,WORD,stat_t1,stat_t0
	.endm

	.macro  MOVETOSTAT, regname, statname
		add     stat_t0, stat_p, statname
		store   0,WORD,	%%(regname), stat_t0
	.endm
	
        greg    bpstat_p         ; pointer to bypass statistics area

        greg    old_src_dma_state ; used in the FSM

	greg	curr_src_bp_pkt_len	; used to keep track of num bytes xferred
	greg	curr_dst_bp_pkt_len	; used to keep track of num bytes xferred

	.macro	BPINC_LW, statname, amount
		; loaded as: <stat_t1, stat_t2>, i.e., t1 is the
		; MSBits and t2 the LSbits
	        add     stat_t0, bpstat_p, statname
		mtsrim	CR, 2-1
	        loadm   0, WORD, stat_t1, stat_t0
	        add     stat_t2, stat_t2, amount
		addc	stat_t1, stat_t1, zero
		mtsrim	CR, 2-1
		storem   0,WORD,stat_t1,stat_t0
	.endm

	.macro	BPINCSTAT, statname
	        add     stat_t0,bpstat_p,statname
	        load    0,WORD,stat_t1,stat_t0
	        add     stat_t1,stat_t1,1
		store   0,WORD,stat_t1,stat_t0
	.endm

	.macro  BPMOVETOSTAT, regname, statname
		add     stat_t0, bpstat_p, statname
		store   0,WORD,	%%(regname), stat_t0
	.endm

	.macro	BP_RESET_DMA_STATUS
	        ;;; Store the bufx and job-id to dma_status here.
        	constx  t0, DMA_INACTIVE
        	; store it in the dma_status word
        	add     t1, bp_config, dma_status
        	store   0, WORD, t0, t1
	.endm


