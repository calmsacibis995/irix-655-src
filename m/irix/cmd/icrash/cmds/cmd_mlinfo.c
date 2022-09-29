#ident  "$Header: /proj/irix6.5m/isms/irix/cmd/icrash/cmds/RCS/cmd_mlinfo.c,v 1.4 1999/05/25 19:21:38 tjm Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <klib/klib.h>
#include "icrash.h"

/*
 * mlinfo_cmd() -- Display ml_info information 
 */
int
mlinfo_cmd(command_t cmd)
{
	int i, ml_info_cnt = 0, first_time = TRUE;
	kaddr_t value, ml;
	k_ptr_t mlp;

	mlp = kl_alloc_block(ML_INFO_SIZE, K_TEMP);
	ml_info_banner(cmd.ofp, BANNER|SMAJOR);
	for (i = 0; i < cmd.nargs; i++) {
		GET_VALUE(cmd.args[i], &value);
		if (KL_ERROR) {
			KL_ERROR |= KLE_BAD_ML_INFO; 
			kl_print_error();
			continue;
		}
		kl_get_ml_info(value, 2, mlp);
		if (KL_ERROR) {
			kl_print_error();
		}
		else {
			if (first_time) {
				first_time = FALSE;
			}
			else if (cmd.flags & (C_FULL|C_NEXT)) {
				ml_info_banner(cmd.ofp, BANNER|SMAJOR);
			}
			print_ml_info(value, mlp, cmd.flags, cmd.ofp);
			ml_info_cnt++;
		}

		if ((DEBUG(DC_GLOBAL, 1) || mlp) && (cmd.flags & C_FULL)) {
			fprintf(cmd.ofp, "\n");
		}
	}
	if (!cmd.nargs || (cmd.flags & C_ALL)) {
		ml = K_MLINFOLIST;
		while (ml) {
			kl_get_ml_info(ml, 2, mlp);
			if (KL_ERROR) {
				kl_print_error();
				break;
			}
			else {
				if (first_time) {
					first_time = FALSE;
				}
				else if (cmd.flags & (C_FULL|C_NEXT)) {
					ml_info_banner(cmd.ofp, BANNER|SMAJOR);
				}
				print_ml_info(ml, mlp, cmd.flags, cmd.ofp);
				ml_info_cnt++;
			}

			if ((DEBUG(DC_GLOBAL, 1) || mlp) && (cmd.flags & C_FULL)) {
				fprintf(cmd.ofp, "\n");
			}
			ml = kl_kaddr(mlp, "ml_info", "ml_next");
		}
	}
	ml_info_banner(cmd.ofp, SMAJOR);
	PLURAL("ml_info struct", ml_info_cnt, cmd.ofp);
	kl_free_block(mlp);
	return(0);
}

#define _MLINFO_USAGE "[-a] [-f] [-w outfile] mlinfo_list"

/*
 * mlinfo_usage() -- Print the usage string for the 'mlinfo' command.
 */
void
mlinfo_usage(command_t cmd)
{
	CMD_USAGE(cmd, _MLINFO_USAGE);
}

/*
 * mlinfo_help() -- Print the help information for the 'mlinfo' command.
 */
void
mlinfo_help(command_t cmd)
{
	CMD_HELP(cmd, _MLINFO_USAGE,
		"Display information from the ml_info struct for each virtual "
		"address included in mlinfo_list.");
}

/*
 * mlinfo_parse() -- Parse the command line arguments for 'mlinfo'.
 */
int
mlinfo_parse(command_t cmd)
{
	return (C_MAYBE|C_WRITE|C_FULL|C_ALL);
}
