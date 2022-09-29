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


/*
 * rdbg_brkpt.c
 *
 * $Revision: 1.2 $
 *
 */

#include <sys/types.h>
#ifdef R4650
#include "r4650.h"
#else
#include <sys/sbd.h>
#endif
#include <sys/inst.h>

#include "eframe.h"
#include "rdbg.h"
#include "serial.h"
#include "lincutil.h"

int brkpt_cont;


#define BPTYPE_NONE	0
#define BPTYPE_SUSP	1
#define BPTYPE_CONT	2
#define BPTYPE_TEMP	3
#define BPTYPE_PERM	4

#define MAXBPTS		16

#define BPINST		0x0000000d

static struct {
	int	bp_type;
	inst_t	*bp_addr;	/* address of breakpoint */
	inst_t	bp_inst;	/* instruction replaced by brkpt */
} bptab[ MAXBPTS ];

void
initbp()
{
	int	i;

	for (i=0; i<MAXBPTS; i++)
		bptab[i].bp_type = BPTYPE_NONE;
	
	brkpt_cont = 0;
}


static int
putinstruction( inst_t *addr, inst_t instr, inst_t *prev_inst )
{

#ifdef RDBGDEBUG
	printf1( "putinstr: 0x%x --> 0x%x\n", (long)instr, (long)addr );
#endif

	if ( read_mem_word( addr, prev_inst ) ) {
#ifdef RDBGDEBUG
		printf1( "putinstr: couldn't read instruction.\n" );
#endif
		return -1;
	}
	if ( write_mem_word( addr, instr ) ) {
#ifdef RDBGDEBUG
		printf1( "putinstr: couldn't write new instruction.\n" );
#endif
		return -1;
	}

	wbinval_dcache( addr, sizeof(inst_t) );
	invalidate_icache( addr, sizeof(inst_t) );

	return 0;
}

int
addbrkpt( inst_t *addr, int type )
{
	int	i, open_entry = -1;
	inst_t	prev_inst;

#ifdef RDBGDEBUG
	printf1( "addbrkpt: addr = 0x%x  type = %d\n", (long)addr, type );
#endif

	for (i=0; i<MAXBPTS; i++) {
		if ( bptab[i].bp_type == BPTYPE_NONE )
			open_entry = i;

		if ( bptab[i].bp_type != BPTYPE_NONE &&
		     bptab[i].bp_addr == addr ) {
		
			if ( bptab[i].bp_type == BPTYPE_TEMP &&
			     type == BPTYPE_PERM )
				bptab[i].bp_type = BPTYPE_PERM;
			
			return 0;
		}
	}
	if ( open_entry < 0 )
		return -1;
	
	if ( putinstruction( addr, BPINST, &prev_inst ) < 0 )
		return -2;
	
	bptab[ open_entry ].bp_type = type;
	bptab[ open_entry ].bp_inst = prev_inst;
	bptab[ open_entry ].bp_addr = addr;

	return 0;
}

/* Remove all temprorary breakpoints, put back all suspended breakpoints.
 */
void
fixup_brkpts()
{
	int	i;
	inst_t	temp_inst;

	for (i=0; i<MAXBPTS; i++)
		if ( bptab[i].bp_type == BPTYPE_TEMP ) {
			(void) putinstruction( bptab[i].bp_addr,
				bptab[i].bp_inst, &temp_inst );
			bptab[i].bp_type = BPTYPE_NONE;
		}
		else if ( bptab[i].bp_type == BPTYPE_SUSP ) {
			(void) putinstruction( bptab[i].bp_addr,
				BPINST, & temp_inst );
			bptab[i].bp_type = BPTYPE_PERM;
		}
}


/*******************************************************************
 *
 * The following routines were stolen from symmon/brkpt.c
 *
 *******************************************************************/

/*
 * is_branch -- determine if instruction can branch
 */
static int
is_branch(inst_t a0)
{
	union mips_instruction i;
	i.word = a0;

	switch (i.j_format.opcode) {
	case spec_op:
		switch (i.r_format.func) {
		case jr_op:
		case jalr_op:
			return(1);
		}
		return(0);

	case j_op:
	case jal_op:

	case bcond_op:	/* bltzl, bgezl, bltzal, bgezal + likely counterparts */
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
	case beql_op:
	case bnel_op:
	case blezl_op:
	case bgtzl_op:
		return(1);

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop3_op:
		switch (i.r_format.rs) {
		case bc_op:
			return(1);
		}
		return(0);
	}
	return(0);
}

/*
 * is_conditional -- determine if instruction is conditional branch
 */
static int
is_conditional(unsigned a0)
{
	union mips_instruction i;
	i.word = a0;

	switch (i.j_format.opcode) {
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
	case beql_op:
	case bnel_op:
	case blezl_op:
	case bgtzl_op:
		return(1);
	case bcond_op:
		switch (i.i_format.rt) {
		case bltz_op:
		case bgez_op:
		case bltzl_op:
		case bgezl_op:
		case bltzal_op:
		case bgezal_op:
		case bltzall_op:
		case bgezall_op:
			return(1);
		}

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop3_op:
		switch (i.r_format.rs) {
		case bc_op:
			return(1);
		}
		return(0);
	}
	return(0);
}

/*
 * branch_target -- calculate branch target
 */
static inst_t *
branch_target(eframe_t *ep, unsigned a0, inst_t *pc)
{
	union mips_instruction i;
	register short simmediate;

	i.word = a0;

	switch (i.j_format.opcode) {
	case spec_op:
		switch (i.r_format.func) {
		case jr_op:
		case jalr_op:
			return((inst_t *)(ep->regs[ R_R0+i.r_format.rs]));
		}
		break;

	case j_op:
	case jal_op:
		return( (inst_t *)((((unsigned long)pc+4)&~((1<<28)-1)) | (i.j_format.target<<2)));

	case bcond_op:
		switch (i.i_format.rt) {
		case bltz_op:
		case bgez_op:
		case bltzl_op:
		case bgezl_op:
		case bltzal_op:
		case bgezal_op:
		case bltzall_op:
		case bgezall_op:
			/*
			 * assign to temp since compiler currently
			 * doesn't handle signed bit fields
			 */
			simmediate = i.i_format.simmediate;
			return(pc+1+simmediate);
		}
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
	case beql_op:
	case bnel_op:
	case blezl_op:
	case bgtzl_op:
		/*
		 * assign to temp since compiler currently
		 * doesn't handle signed bit fields
		 */
		simmediate = i.i_format.simmediate;
		return(pc+1+simmediate);

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop3_op:
		switch (i.r_format.rs) {
		case bc_op:
			/*
			 * kludge around compiler deficiency
			 */
			simmediate = i.i_format.simmediate;
			return(pc+1+simmediate);
		}
		break;
	}
	/* _fatal_error("branch_target"); */
	/*NOTREACHED*/
}

/*
 * end of code stolen from symmon/brkpt.c
 *
 *
 **************************************************************************/


int
step1( eframe_t *ep )
{
	inst_t	*pc, inst;

	pc = (inst_t *) ep->regs[ R_EPC ];

	if ( read_mem_word( pc, &inst ) ) {
#ifdef RDBGDEBUG
		printf1( "step1: couldn't get instruction at pc: 0x%x\n",
			(long)pc );
#endif
		return -1;
	}

#ifdef RDBGDEBUG
	printf1( "step1: pc = 0x%x  inst=0x%x\n", (long)pc, (long)inst );
#endif
	
	if ( is_branch(inst) ) {
		if ( addbrkpt( branch_target(ep,inst,pc), BPTYPE_TEMP ) )
			return -1;
		if ( is_conditional(inst) )
			if ( addbrkpt( pc+2, BPTYPE_TEMP ) )
				return -1;
	}
	else
		if ( addbrkpt( pc+1, BPTYPE_TEMP ) )
			return -1;
	
	return 0;
}


int
cont( eframe_t *ep )
{
	int	i;
	inst_t	temp_instr, *pc;

	pc = (inst_t *) ep->regs[ R_EPC ];

	for (i=0; i<MAXBPTS; i++)
		if ( bptab[i].bp_type == BPTYPE_PERM &&
		     bptab[i].bp_addr == pc ) {
		
			/* Suspend breakpoint at PC.
			 */
			(void) putinstruction( pc, bptab[i].bp_inst,
				&temp_instr );
			bptab[i].bp_type = BPTYPE_SUSP;

			/* Single step so that suspended instruction
			 * gets executed.
			 */
			if ( step1( ep ) )
				return -1;

			brkpt_cont = 1;	/* Must do continue in two steps! */

			break;
		}

	return 0;
}
