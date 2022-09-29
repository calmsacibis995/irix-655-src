;
; ehiphw.h
;
; Copyright (C) 1993 Silicon Graphics, Inc.
;
; Everest HIPPI card.
;
; Hardware constants, definitions
;
; $ Revision: $
;

; ---------------
; Memory Regions:
; ---------------
	.equ	EEPROM,		0x00000000	; flash-mem, packed (read only)
	.equ	FLASH_RW,	0x01000000	; flash-mem, unpacked (r/w)
	.equ	REGS_0WS,	0x02000000	; zero-wait-state registers
	.equ	REGS_1WS,	0x03000000	; one-wait-state registers
	.equ	REGS_HS,	0x03800000	; hippi-synchronous
	.equ	SRC_DRAM,	0x04000000	; src triple-port DRAM begin
	.equ	DST_DRAM,	0x06000000	; dst triple-port DRAM begin


;----------------
;EEPROM
;----------------
	.equ	EEPROM_SIZE,	0x20000		; 128K of flash


; -------------------------
; TPDRAM (Triple-port DRAM)
; -------------------------
;
; regions of TPDRAM:
;
;	NORMAL:		normal dram r/w access
;	NPARITY:	read: read w/o parity check,
;			write:  with arbitrary parity (from WT_WRONG_PAR reg)
;	XFER:		do transfer cycle
;	PXFER:		do pseudo-transfer only loading ATE block address

	.equ	DRAM_SIZE,	(0x100000)		; 1MB each
	.equ	DRAM_SIZE_BT,	20			; 20 bits of DRAM addr

; offsets to special regions of SRC/DST_DRAM

	.equ	OFFS_NPARITY,	0x00800000
	.equ	OFFS_XFER,	0x01000000		; dram transfers
	.equ	OFFS_DXFER,	0x01800000		; diag-transfer

	.equ	SDRAM_NPARITY,	(SRC_DRAM+OFFS_NPARITY)
	.equ	SDRAM_XFER,	(SRC_DRAM+OFFS_XFER)	; src dram transfers
	.equ	SDRAM_DXFER,	(SRC_DRAM+OFFS_DXFER)	; src pseudo-transfer

	.equ	DDRAM_NPARITY,	(DST_DRAM+OFFS_NPARITY)
	.equ	DDRAM_XFER,	(DST_DRAM+OFFS_XFER)	; dst dram transfers
	.equ	DDRAM_DXFER,	(DST_DRAM+OFFS_DXFER)	; dst pseudo-transfer

; TPDRAM transfer bits.  These bits are bits written to TPDRAM XFER locations.
; The hardware uses them to control the type of transfer cycle performed.
; Refer to the Micron data sheets for more information.

	.equ	XB_SAMA,	0	; transfer to/from SAMa (src/to-host)
	DEFBT	XB_SAMB,	6	; transfer to/from SAMb (dst/from-host)
	.equ	XB_RTRANS,	0
	DEFBT	XB_WTRANS,	5	; write (vs. read) transfer bit
	DEFBT	XB_TRM,		4	; transfer mask operation
	DEFBT	XB_DSF1,	3	; device special function 1
	DEFBT	XB_DSF2,	2	; device special function 2
	DEFBT	XB_SE,		1	; serial enable
	DEFBT	XB_MKD,		0	; mask data

; TPDRAM transfer bits.  These are more useful definitions than above
; bit definitions.  These are transfer cycles we'll likely be using in
; the firmware

	.equ	XFER_RDA,	(XB_SAMA|XB_SE|XB_RTRANS|XB_MKD)
	.equ	XFER_SPLRDA,	(XB_SAMA|XB_SE|XB_RTRANS|XB_DSF1|XB_MKD)
	.equ	XFER_WTB,	(XB_SAMB|XB_SE|XB_WTRANS)
	.equ	XFER_MWTB,	(XB_SAMB|XB_SE|XB_TRM|XB_DSF2|XB_WTRANS|XB_MKD)
	.equ	XFER_CLR_BMR,	(XB_SE|XB_RTRANS|XB_DSF1|XB_DSF2|XB_MKD)

; TPDRAM control registers
	.equ	WT_WRONG_PAR,	(REGS_1WS+0x010)
	.equ	MEM_ERR,	(REGS_1WS+0x014)	; memory err status
	.equ	MEM_STATIC_BITS,(REGS_1WS+0x018)
	.equ	DQ_MASK_DATA,	(REGS_1WS+0x01C)



; --------
; Host DMA
; --------
;
; Host DMA is a simple DMA engine hooked to the serial inputs/outputs of
; the TPDRAM.  Local address is controlled by doing transfer cycles on
; the TPDRAM thus setting the block and tap pointers.  The host addresses
; are then plugged into the DMA_HOST_HI, DMA_HOST_LO registers (64-bit
; although Everest only supports the low 40 bits).  Writing the DMA_CNTL
; (count/control) register starts the DMA.
;
; There is also a DMA "command buffer" to facilitate DMA pipelining.  As
; long as the direction and VAM bits don't change, you can write a DMA
; command before the previous completes by just waiting for the DCBF bit
; to go off or waiting for a DCBE interrupt.

	.equ	DMA_HOST_HI,	(REGS_0WS+0x000)
	.equ	DMA_HOST_LO,	(REGS_0WS+0x008)
	.equ	DMA_HOST_CTL,	(REGS_0WS+0x00C)

; bits in DMA_CNTL register:
;
	DEFBT	DMAC_MAPD,29	; using FCI virtual addresses
	DEFBT	DMAC_NPREF,28	; disable prefetching
	DEFBT	DMAC_DCBF,23	; DMA command buffer full
	DEFBT	DMAC_NOTDN,22	; DMA not complete
	DEFBT	DMAC_CLCK,21	; clear checksummer this DMA
	DEFBT	DMAC_OPSUM,20	; checksummer sums (end-around carry)
	DEFBT	DMAC_OPXOR,19	; checksummer will XOR
	DEFBT	DMAC_GO,18	; GO!
	DEFBT	DMAC_LOOP,17	; loopback from dst TPDRAM to src TPDRAM
	DEFBT	DMAC_WR,16	; write: (write to host memory)
	.equ	DMAC_RD,0	; read: (read from host memory)
; bits 15-3 are the double word DMA count.  (shifted three bits so it
; looks like a byte count.)

; checksummer:
	.equ	XSUM_LOW,	(REGS_0WS+0x030)
	.equ	XSUM_HIGH,	(REGS_0WS+0x034)
	DEFBT	XSUM_C,16	; carry bit
	DEFBT	XSUM_Z,17	; zero bit
	DEFBT	XSUM_OVF,18	; 2s complement ovflow bit




; -----------------
; HIPPI Destination
; -----------------
	.equ	DST_FILL_FLUSH,	(REGS_HS+0x40)
	.equ	DST_STAT_BLOCK,	(REGS_HS+0x44)
	.equ	DST_MODE_SEL,	(REGS_HS+0x50)
	.equ	DST_CONTROL,	(REGS_HS+0x54)
	.equ	DST_RESET_RDY,	(REGS_HS+0x58)
	.equ	DST_THRESH,	(REGS_HS+0xB0)
	.equ	DST_BIE,	(REGS_HS+0xB4)
	.equ	DST_WIB,	(REGS_HS+0xB8)
	.equ	DST_WIE_EN,	(REGS_HS+0xBC)
	.equ	DST_CONNECT,	(REGS_HS+0xC4)
	.equ	DST_REAL_TIME,	(REGS_HS+0xC8)
	.equ	DST_RT_CLR,	(REGS_HS+0xCC)
	.equ	DST_RDY_PULSER,	(REGS_HS+0xD0)
	.equ	DST_FREE_BST,	(REGS_HS+0xD4)
	.equ	DST_RDY_OUTST,	(REGS_HS+0xD8)
	.equ	DST_MAX_RDY,	(REGS_HS+0xDC)
	.equ	DST_WOKI_FLAGS,	(REGS_HS+0xE0)

; DST_FILL_FLUSH bits:
	DEFBT	DST_FF_SOT,	21
	DEFBT	DST_FF_AOT,	20
; DST_MODE_SEL (S2021) values:
	.equ	DST_MODE_RESET,	0
	.equ	DST_MODE_DIAG,	4
	.equ	DST_MODE_RUN,	5
; DST_CONTROL bits:
	DEFBT	DST_CTL_LLRC,	26	; data enable: all LLRC words
	DEFBT	DST_CTL_LLRCERR,25	; data enable: LLRC words in error
	DEFBT	DST_CTL_STATBL,	24	; go bit for status block (ro)
	DEFBT	DST_CTL_GENOP,	23	; data enable: Gen_Op_Status words
	DEFBT	DST_CTL_SEQERR,	22	; data enable: sequence errors
	DEFBT	DST_CTL_FF,	21	; go bit for fill and flush (ro)
	DEFBT	DST_CTL_IDATA,	20	; enable: data and I-field

	DEFBT	DST_CTL_DIS_SDIC_LOST,	19 ; auto disconnect: lose DSIC
	DEFBT	DST_CTL_DIS_THRESHOLD,	18 ; auto disconnect: reach threshold
	DEFBT	DST_CTL_DIS_SEQ_ERR,	17 ; auto disconnect: sequence error
	DEFBT	DST_CTL_DIS_PARITY,	16 ; auto disconnect: parity error
	DEFBT	DST_CTL_DIS_LLRC,	15 ; auto disconnect: LLRC error
	DEFBT	DST_CTL_DIS_SYNC,	14 ; auto disconnect: sync error
	DEFBT	DST_CTL_DIS_ILLBURST,	13 ; auto disconnect: illegal burst

	DEFBT	DST_CTL_ANY,	12	; arm: end of every burst
	DEFBT	DST_CTL_NON1ST_SHT,11	; arm: non-first short burst
	DEFBT	DST_CTL_1ST_SHT,10	; arm: first short burst
	DEFBT	DST_CTL_1ST,	9	; arm: first burst
	DEFBT	DST_CTL_I_FIELD,8	; arm: I-field
	DEFBT	DST_CTL_EOC,	7	; arm: end of connection
	DEFBT	DST_CTL_THRESH,	6	; arm: threshold reached
	DEFBT	DST_CTL_EOP,	5	; arm: end of packet
	DEFBT	DST_CTL_EONC,	4	; arm: empty connection
	DEFBT	DST_CTL_SDIC_LOST,3	; arm: SDIC lost
; DST_REAL_TIME bits:
	DEFBT	DST_RT_SRCAV,	9	; source available
	DEFBT	DST_RT_BROUT,	8	; burst out
	DEFBT	DST_RT_CONRQ,	7	; connection request
	DEFBT	DST_RT_PKOUT,	6	; packet out
	DEFBT	DST_RT_MIC,	5	; "many in connection"
	DEFBT	DST_RT_AIE,	4	; "any in epoch"
	DEFBT	DST_RT_SDIC_LOST,3	; SDIC lost
	DEFBT	DST_RT_SYNC_ERR,2	; sync error
	DEFBT	DST_RT_REQ_LOST,1	; request lost
; DST_CONNECT bits:
	DEFBT	DST_CONN_MAN,	3	; manual mode
	DEFBT	DST_ACD,	2	; auto clear disable
	DEFBT	DST_CONN_CONNIN,1	; CONIN pin on S2021
	DEFBT	DST_CONN_ACCEPT,0	; Accept=1/Reject=0 ACCRJ pin on S2021
; 


; to read 36-bit WOKIs, you do a 29K burst read into two regs from
; the DST_WOKI_LO address.  DST_WOKI_LO shifts the fifo (into the
; fifo register) and then you can read DST_WOKI_HI.

	.equ	DST_WOKI_LO,	(REGS_1WS+0xA4)		; status bits
	.equ	DST_WOKI_HI,	(REGS_1WS+0xA8)		; count in epoch

; status and error bits in DST_WOKI_LO:

	DEFBT	DWOKI_DE_ALL_LLRC,	26		; -- data enable bits -
	DEFBT	DWOKI_DE_LLRC_ERR,	25
	DEFBT	DWOKI_DE_STATUS,	24
	DEFBT	DWOKI_DE_GENOP,		23
	DEFBT	DWOKI_DE_SEQERR,	22
	DEFBT	DWOKI_DE_GUNK,		21		; "fill and flush" words
	DEFBT	DWOKI_DE_DATAI,		20		; data or I-field

	DEFBT	DWOKI_AD_SDIC_LOST,	19		; --- auto disconn bits
	DEFBT	DWOKI_AD_THRESH,	18
	DEFBT	DWOKI_AD_SEQ_ERR,	17
	DEFBT	DWOKI_AD_PARITY,	16
	DEFBT	DWOKI_AD_LLRC_ERR,	15
	DEFBT	DWOKI_AD_SYNC,		14
	DEFBT	DWOKI_AD_ILBRST,	13

							; --- ARM bits ---
	DEFBT	DWOKI_ANYBRST,		12		; any burst
	DEFBT	DWOKI_N1STSHT,		11		; non-first short
	DEFBT	DWOKI_1STSHT,		10		; first short
	DEFBT	DWOKI_1STBURST,		9		; 1st burst
	DEFBT	DWOKI_IFIELD,		8		; I-field
	DEFBT	DWOKI_EOC,		7		; end-of-connection
	DEFBT	DWOKI_THRESH,		6		; hit threshold
	DEFBT	DWOKI_EOP,		5		; end-of-packet
	DEFBT	DWOKI_EONC,		4		; end-of-null-conn
	DEFBT	DWOKI_SDIC_LOSS,	3		; SDIC Lost

		; each epoch has a type:
	.equ	DWOKI_DTYPE_MASK,	0x07		; epoch data type
	.equ	DWOKI_DTYPE_FF,		0	; fill-and-flush (garbage)
	.equ	DWOKI_DTYPE_LLRC_ERR,	1
	.equ	DWOKI_DTYPE_LLRC,	2
	.equ	DWOKI_DTYPE_GENOP,	3
	.equ	DWOKI_DTYPE_SEQERR,	4
	.equ	DWOKI_DTYPE_STATUS,	5
	.equ	DWOKI_DTYPE_DATA,	7	; or I-field

; valid bits in DST_WOKI_HI
	.equ	DWOKI_COUNT_BITS,	0xFFFFC



; ------------
; HIPPI Source
; ------------
	.equ	SRC_MODE_SEL,	(REGS_HS+0x60)
	.equ	SRC_CONTROL,	(REGS_HS+0x64)
	.equ	SRC_WOKI_RD,	(REGS_HS+ 0x68)
	.equ	SRC_WOKI_WR,	(REGS_1WS+0x68)	; REGS_0WS???
	.equ	SRC_WOKI_SHIFT,	(REGS_HS+0x6C)
	.equ	SRC_BYPASS,	(REGS_HS+0x70)
	.equ	SRC_COUNTER,	(REGS_HS+0x74)
	.equ	SRC_RT,		(REGS_HS+0x78)
	.equ	SRC_RT_CLR,	(REGS_HS+0x7C)

; SRC_MODE_SEL (S2020 mode) values:
	.equ	SRC_MODE_RST,	0
	.equ	SRC_MODE_TEST,	1
	.equ	SRC_MODE_WAIT,	2		; S2020 interlock mode
	.equ	SRC_MODE_RUN,	3		; S2020 run mode
; SRC_CONTROL bits:
	DEFBT	SRC_CTL_PRIME,	4		; prime data pipe
	DEFBT	SRC_CTL_SHIFT,	3		; shift WOKI FIFO
	DEFBT	SRC_CTL_GO,	2		; start src state machine
	DEFBT	SRC_CTL_STOP,	1		; STOP and Disconnect
	DEFBT	SRC_CTL_SINGLE,	0		; single step
; SRC_WOKI_{RD,WR} bits:
	.equ	SWOKI_TAGSHFT,	24		; 8-bit tag
	.equ	SWOKI_BPSHFT,	23              ; set if packet was a bypass packet
	.equ	SWOKI_TAG_BITS,	8
	.equ	SWOKI_TAG_MASK,	(0xFF<<24)
	DEFBT	SWOKI_BP,	23
	DEFBT	SWOKI_FLUSH,	22		; don't send this data
	DEFBT	SWOKI_KEEPCON,	21		; don't drop connect
	DEFBT	SWOKI_KEEPPKT,	20		; don't drop packet
	DEFBT	SWOKI_STOP,	19		; stop after this WOKI
	.equ	SWOKI_MAX_LEN,	0x3FFFF		; length field is in WORDS!
	.equ	SWOKI_LEN_BITS,	18
; SRC_COUNTER bits:
	DEFBT	SRC_CNT_SCZ,	18
	DEFBT	SRC_CNT_CLEO,	19
; SRC_RT, SRC_RT_CLR bits:
	DEFBT	SRC_RT_2020_DTREQ,	20
	DEFBT	SRC_RT_STOPPED,		19
	DEFBT	SRC_RT_2020_SRNDS,	18
	DEFBT	SRC_RT_2020_ACCRJ,	17
	DEFBT	SRC_RT_2020_CNOUT,	16
	DEFBT	SRC_RT_2020_PKTAV,	15
	DEFBT	SRC_RT_2020_SEQERR,	14
	DEFBT	SRC_RT_2020_CNREQ,	13
	DEFBT	SRC_RT_SCZERO,		12
	DEFBT	SRC_RT_DTREQ_HAPPENED,	11
	DEFBT	SRC_RT_SRC_END_ERR,	10
	DEFBT	SRC_RT_STOP_SEQ_ERR,	9
	DEFBT	SRC_RT_STOP_DSIC,	8
	DEFBT	SRC_RT_PAR_ERR,		7
	DEFBT	SRC_RT_CONN_LOST,	6
	DEFBT	SRC_RT_REJECT,		5
	DEFBT	SRC_RT_STEP_EXE,	4
	DEFBT	SRC_RT_STOP_BIT_EXE,	3
	DEFBT	SRC_RT_STOP_COMMAND,	2
	DEFBT	SRC_RT_WOKI_NE,		1
	DEFBT	SRC_RT_WOKI_ROOM,	0


; -------------------------
; ATE (Automatic Transfer Engines)
; -------------------------
;
; There are four ATEs, each associated with each serial port of the two
; TPDRAMS.  Thus, the four ATEs are: HIPPI source, from-host,
; to-host, HIPPI dest.  The ATEs generate transfer cycles to move
; data to/from DRAM to/from the TPDRAMs serial registers.
;
;		---------------------------------
; --> from-host	|*B*	 source TPDRAM	     *A*|   HIPPI src  ---->
;		---------------------------------
;		---------------------------------
; <--  to-host	|*A*	 dest TPDRAM	     *B*|   HIPPI dst  <----
;		---------------------------------

; These read-only registers allow you to read current ATE block addresses.
; For diagnostics, you can set these addresses by doing the appropriate
; pseudo-transfers to the TPDRAM.

	.equ	SRC_BLK_ADD,	(REGS_1WS+0x080)	; source block addr
	.equ	FH_BLK_ADD,	(REGS_1WS+0x084)	; from-host block addr
	.equ	TH_BLK_ADD,	(REGS_1WS+0x088)	; to-host block addr
	.equ	DST_BLK_ADD,	(REGS_1WS+0x08C)	; dst block addr

	.equ	BLKADDR_MASK,	0xFF800
	DEFBT	BLKADDR_QSF,	10
	.equ	TAP_MASK,	0x003FF
	.equ	TPDRAM_BLKSIZE,	2048
	.equ	TPDRAM_HBLKSIZE,(TPDRAM_BLKSIZE/2)

; wrap-around points for TPDRAM (low address)

	.equ	FHSRC_START,	(REGS_1WS+0x090)	; from-host/src
	.equ	THDST_START,	(REGS_1WS+0x098)	; to-host/dst

; for clearing error conditions:
	.equ	ATE_OVERRUN,	(REGS_1WS+0x020)
	.equ	TH_PAR,		(REGS_1WS+0x038)


;-----------------------------------
; Interrupts
;-----------------------------------
;
;
; Interrupt Sources
; -----------------
; 
; 		Source			Cleared by		     Enable bit
; 		---------------		-------------------------	----
; Trap_0:	Debug_Trap *		Set INT_33[6]			16
; 
; Trap_1:	Any_ATE_Overrun		Write ATE_OVERRUN		15
; 		TH_Par_Err		Write TH_PAR			14
;   		Src_Stop_Seq_Err	Set SRC_CONTROL[Src_GO]		13
;   		Src_Stop_DSIC_Lost	Reset the 2021			12
;   		Src_Stop_Par_Err	Set SRC_CONTROL[Src_GO]		11
;   		Src_Stop_CON_Lost	Set SRC_CONTROL[Src_GO]		10
;   		Src_Stop_Reject		Set SRC_CONTROL[Src_GO]		 9
;   		Src_Single_Step_Ex	Set SRC_CONTROL[Src_GO]		 8
; 		Dst_SDIC_Lost		Read DST_RT_CLR			 7
; 		Dst_Sync_Err		Read DST_RT_CLR			 6
; 		Dst_REQ_Lost		Read DST_RT_CLR			 5
; 
; Int_0:	Src_Stop_Bit_Exe	Set SRC_CONTROL[Src_GO]		 4
; 
; Int_1:	DMA_Complete		Set DMA_CONTROL[GO]		 3
; 		DMA_Cmd_Buffer_Empty	Set DMA_CONTROL[GO]		 2
; 
; Int_2:	Dst_WOKI_Not_Empty	Read DST_WOKI_LO		 1
; 
; Int_3:	Host_Int		Clear BD_CTL[Host_To_29K]	 0 
; 		---------------		-------------------------	---

	.equ	INT_ENAB,	(REGS_1WS+0x24)
	.equ	INT_33,		(REGS_1WS+0x28)
	.equ	BD_CTL,		(REGS_1WS+0x2C)

; -------------
;   INT_ENAB:
; -------------
	; Trap0:
	DEFBT	INT_DEBUG,		16	; Debug_Trap Interrupt Enable
	; Trap1:
	DEFBT	INT_TH_PAR,		15	; To_Host_parity_error int
						; enable
	DEFBT	INT_ATE_OVRRUN,		14	; ATE Overrun interrupt enable
	DEFBT	INT_SRC_SEQ_ERR,	13	; Source sequence error
						; interrupt enble
	DEFBT	INT_SRC_DSIC_LST,	12	; source DSIC lost interrupt
	DEFBT	INT_SRC_PAR_ERR,	11	; Source Parity Error
	DEFBT	INT_SRC_CON_LOST,	10	; Source connection lost
	DEFBT	INT_SRC_REJECT,		9	; Source reject
	DEFBT	INT_SNGL_STEP,		8	; Single_Step_Executed int
	DEFBT	INT_DST_SDIC_LOST,	7	; Dest SDIC lost interrupt
	DEFBT	INT_DST_SYNC_ERR,	6	; Dest Sync Err
	DEFBT	INT_DST_REQ_LOST,	5	; Dest request lost
	; Int0:
	DEFBT	INT_SRC_STOP_EXE,	4	; SRC_STOP_BIT_EXECUTED int
	; Int1:
	DEFBT	INT_DMA_DONE,		3	; DMA_Complete int enable
	DEFBT	INT_DCBE,		2	; DMA_Command_Bufer_Empty int
	; Int2:
	DEFBT	INT_DST_WOKI_NE,	1	; Destination WOKI non-empty
	; Int3:
	DEFBT	INT_HOST_INT,		0	; Host interrupt

; ------------
;   INT_33:
; ------------
	DEFBT	INT33_LED_YELLOW,	7	; Yellow LED
	DEFBT	INT33_LED_GREEN,	6	; Green LED
	DEFBT	INT33_ANY_ATE,		5	; Any_ATE_Overrun
	DEFBT	INT33_TH_PAR_ERR,	4	; To_Host Parity Error
	DEFBT	INT33_REF_INH,		3	; TPDRAM Refresh inhibit
	DEFBT	INT33_DQ_MASK_DAT,	2	; Always set to 1!
	DEFBT	INT33_DST_WOKI_NE,	1	; Destination WOKI non-empty
	DEFBT	INT33_DEBUG_TRAP,	0	; Debug Trap

;-------------
; BD_CTL:
;-------------
	DEFBT	BDCTL_RESET_29K,	2	; Reset 29K
	DEFBT	BDCTL_29K_INT,		1	; From-Host-To-29K interrupt
	DEFBT	BDCTL_HOST_INT,		0	; From-29K-To-Host interrupt

