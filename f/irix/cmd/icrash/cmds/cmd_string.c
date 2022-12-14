#ident  "$Header: /proj/irix6.5f/isms/irix/cmd/icrash/cmds/RCS/cmd_string.c,v 1.12 1999/05/25 19:52:33 tjm Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <klib/klib.h>
#include "icrash.h"

/*
 * string_cmd() -- Run the 'string' command.
 */
int
string_cmd(command_t cmd)
{
	int c;
	k_uint_t count;
	kaddr_t addr;
	struct syment *sp;

	sp = kl_get_sym(cmd.args[0], K_TEMP);
	if (!KL_ERROR) {
		addr = sp->n_value;
		kl_free_sym(sp);
	} 
	else {
		GET_VALUE(cmd.args[0], &addr);
		if (KL_ERROR) {
			kl_print_error();
			return(1);
		}
	}

	if (cmd.nargs > 1) {
		GET_VALUE(cmd.args[1], &count);
		if (KL_ERROR) {
			kl_print_error();
			return(1);
		}
	} 
	else {
		count = 1;
	}

	if (!addr) {
		KL_SET_ERROR_CVAL(KLE_BAD_SYMNAME, cmd.args[0]);
		kl_print_error();
		return(1);
	}

	kl_is_valid_kaddr(addr, (k_ptr_t)NULL, 0);
	if (KL_ERROR) {
		kl_print_error();
		return(1);
	}

	c = 0;
	fprintf(cmd.ofp, "0x%llx = \"", addr);
	while (c++ < count) {
		addr = dump_string(addr, cmd.flags, cmd.ofp);
		if (KL_ERROR) {
			kl_print_error();
			break;
		}
	}
	fprintf(cmd.ofp, "\"\n");
	return(0);
}

#define _STRING_USAGE "[-w outfile] start_address | symbol [count]"

/*
 * string_usage() -- Print the usage string for the 'string' command.
 */
void
string_usage(command_t cmd)
{
	CMD_USAGE(cmd, _STRING_USAGE);
}

/*
 * string_help() -- Print the help information for the 'string' command.
 */
void
string_help(command_t cmd)
{
	CMD_HELP(cmd, _STRING_USAGE,
		"Display count strings of ASCII characters starting at "
		"start_address (or address for symbol).");
}

/*
 * string_parse() -- Parse the command line arguments for 'string'.
 */
int
string_parse(command_t cmd)
{
	return (C_TRUE|C_WRITE);
}
