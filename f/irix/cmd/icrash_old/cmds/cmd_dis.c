#ident  "$Header: /proj/irix6.5f/isms/irix/cmd/icrash_old/cmds/RCS/cmd_dis.c,v 1.1 1999/05/25 19:50:14 tjm Exp $"
#include <sys/types.h>
#include <sys/param.h>
#include <disassembler.h>
#include <stdio.h>
#include <errno.h>
#include "icrash.h"
#include "extern.h"

/* Register names for the N32/N64 bit world.
 */
static char *REGNAMES[32] = {
		"zero", "at",   "v0",   "v1",   "a0",   "a1",   "a2",   "a3",
		"a4",   "a5",   "a6",   "a7",   "t0",   "t1",   "t2",   "t3",
		"s0",   "s1",   "s2",   "s3",   "s4",   "s5",   "s6",   "s7",
		"t8",   "t9",   "k0",   "k1",   "gp",   "sp",   "s8",   "ra"
};

/*
 * In IRIX 6.0, things have changed.  You can use the function called
 * __disasm32() or __disasm64().  Depending on whether what you are
 * reading in is 64 bit or not, you'll get different results.
 */

#define _DIS_USAGE "[-f] [-w outfile] dis_addr [count]"

/*
 * dis_usage() -- Print the usage string for the 'dis' command.
 */
void
dis_usage(command_t cmd)
{
	CMD_USAGE(cmd, _DIS_USAGE);
}

/*
 * dis_help() -- Print the help information for the 'dis' command.
 */
void
dis_help(command_t cmd)
{
	CMD_HELP(cmd, _DIS_USAGE,
		"Display the disassembled code from address for count "
		"instructions.  The default count is 1.");
}

/*
 * dis_parse() -- Parse the command line arguments for the 'dis' command.
 */
int
dis_parse(command_t cmd)
{
	return (C_MAYBE|C_FULL|C_WRITE);
}

static Elf64_Addr value;
static k_uint_t lines;

/*
 * dis_cmd() -- Run the 'dis' command.
 */
int
dis_cmd(command_t cmd)
{
	char *get_funcname(), *strpbrk();
	char *c = (char *)NULL, *ptr = (char *)NULL;
	char linebuf[128], disbuf[128];
	Elf64_Addr symbol64;
	Elf32_Addr result, regmask, regnum, symbol32;
	symaddr_t *symfunc, *find_sym();
	struct syment *sp, *get_sym();
	int i, lineno, status, flagged = FALSE;

	if (cmd.nargs > 2) {
		dis_usage(cmd);
		return (-1);
	}

	/* Set the disassembler's location and the number of lines to
	 * dump out.  Also make sure our lines value is correct.
	 */
	if (cmd.nargs) {
		if (c = strpbrk(cmd.args[0], "+-/*")) {
			GET_VALUE(cmd.args[0], &value);
			if (KL_ERROR) {
				kl_print_error(K);
				return (1);
			}
		}
		else {
			if (sp = get_sym(cmd.args[0],B_TEMP)) {
				value = (Elf64_Addr)sp->n_value;
				free_sym(sp);
			}
			else {
				GET_VALUE(cmd.args[0], &value);
				if (KL_ERROR) {
					kl_print_error(K);
					return (1);
				}
			}
		}

		if (cmd.nargs > 1) {
			GET_VALUE(cmd.args[1], &lines);
			if (KL_ERROR) {
				kl_print_error(K);
				return (1);
			}

			if (lines <= 0) {
				KL_SET_ERROR_NVAL(KLE_BAD_LINENO, lines, 0);
				kl_print_error(K);
				dis_usage(cmd);
				return (1);
			}
		}
		else {
			lines = 1;
		}
	}
	else {
		/* Make sure that there is a value...
		 */
		if (!value) {
			dis_usage(cmd);
			return (1);
		}
	}

	/* Make sure that address is valid
	 */
	kl_is_valid_kaddr(K, value, (k_ptr_t)NULL, 0);
	if (KL_ERROR) {
		KL_SET_ERROR_NVAL(KLE_BAD_PC, value, 2);
		kl_print_error(K);
		return(1);
	}

	if (value % 4) {

		/* Don't print these addresses -- They are not properly aligned
		 * code addresses
		 */
		KL_SET_ERROR_NVAL(KLE_INVALID_VADDR_ALIGN, value, 2);
		kl_print_error(K);
		return (1);
	}

	/* Initialize properly.
	 */
	if (PTRSZ64(K)) {
		__dis_init64("", "", REGNAMES, 1);
	}
	else {
		__dis_init32("", "", REGNAMES, 1);
	}

	/* Read in a 32 bit address set at a time and dump it out.
	 */
	for (i = 0; i < lines; i++) {

		/* Start the line off.
		 */
		lineno = get_lineno(value);
		ptr = get_funcname(value);
		if (lineno < 0) {
			continue;
		}

		if (!flagged) {
			flagged = TRUE;
			if (!(cmd.flags & C_FULL)) {
				fprintf(cmd.ofp, "======================================="
					"========================================\n");
			}
		}

		if (ptr) {
			sprintf(linebuf, "%s%s[%s:%d, 0x%llx]\t",
				((cmd.flags & C_FULL) ? "" : " "),
				(((lines == 1) && (cmd.flags & C_FULL)) ? "*" : " "),
				ptr, lineno, value);
		}
		else {
			sprintf(linebuf, "%s%s[noname:%d, 0x%llx]\t",
				((cmd.flags & C_FULL) ? "" : " "),
				(((lines == 1) && (cmd.flags & C_FULL)) ? "*" : " "),
				lineno, value);
		}

		/* Get the value to disassemble.
		 */
		kl_get_block(K, value, sizeof(Elf32_Addr), &result, "disvalue");

		/*
		 * Disassemble depending on the address size.
		 */
		if (PTRSZ64(K)) {
			status = __disasm64(disbuf, value, result,
						&regmask, &symbol64, &regnum);
		}
		else {
			status = __disasm32(disbuf, (Elf32_Addr)value, result,
						&regmask, &symbol32, &regnum);
		}
		strcat(linebuf, disbuf);
		if (PTRSZ64(K)) {
			if ((status > 0) && (symbol64) && (ptr = get_funcname(symbol64))) {
				fprintf(cmd.ofp, "%s (%s)\n", linebuf, ptr);
			}
			else {
				fprintf(cmd.ofp, "%s\n", linebuf);
			}
		}
		else {
			if ((status > 0) && (symbol32) &&
				(ptr = get_funcname((kaddr_t)symbol32))) {
					fprintf(cmd.ofp, "%s (%s)\n", linebuf, ptr);
			}
			else {
				fprintf(cmd.ofp, "%s\n", linebuf);
			}
		}
		value += sizeof(Elf32_Addr);
	}

	if ((flagged) && (!(cmd.flags & C_FULL))) {
		fprintf(cmd.ofp, "======================================="
			"========================================\n");
	}
	return(0);
}
