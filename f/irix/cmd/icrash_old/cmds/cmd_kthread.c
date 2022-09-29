#ident "$Header: /proj/irix6.5f/isms/irix/cmd/icrash_old/cmds/RCS/cmd_kthread.c,v 1.1 1999/05/25 19:50:14 tjm Exp $"
#define _KERNEL  1
#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>
#include "icrash.h"
#include "extern.h"

/*
 * kthread_cmd() -- Dump out kthread struct information 
 */
int
kthread_cmd(command_t cmd)
{
	int i, mode, cnt, first_time = TRUE, kthread_cnt = 0;
	k_ptr_t ktp; 
	kaddr_t value, kthread;

	if (cmd.nargs == 0) {
#ifdef ANON_ITHREADS
		fprintf(cmd.ofp, "ACTIVE ITHREADS:\n\n");
		cnt = list_active_ithreads((cmd.flags|C_KTHREAD), cmd.ofp);
		kthread_banner(cmd.ofp, SMAJOR);
		fprintf(cmd.ofp, "%d active ithreads found\n", cnt);
		fprintf(cmd.ofp, "\n");
#endif

		fprintf(cmd.ofp, "ACTIVE STHREADS:\n\n");
		cnt = list_active_sthreads((cmd.flags|C_KTHREAD), cmd.ofp);
		kthread_banner(cmd.ofp, SMAJOR);
		fprintf(cmd.ofp, "%d active sthreads found\n", cnt);
		fprintf(cmd.ofp, "\n");

		fprintf(cmd.ofp, "ACTIVE UTHREADS:\n\n");
		cnt = list_active_uthreads((cmd.flags|C_KTHREAD), cmd.ofp);
		kthread_banner(cmd.ofp, SMAJOR);
		fprintf(cmd.ofp, "%d active uthreads found\n", cnt);
		fprintf(cmd.ofp, "\n");

		fprintf(cmd.ofp, "ACTIVE XTHREADS:\n\n");
		cnt = list_active_xthreads((cmd.flags|C_KTHREAD), cmd.ofp);
		kthread_banner(cmd.ofp, SMAJOR);
		fprintf(cmd.ofp, "%d active xthreads found\n", cnt);
		fprintf(cmd.ofp, "\n");
		return(0);
	} 
	else {
		for (i = 0; i < cmd.nargs; i++) {

			if ((first_time == TRUE) || (cmd.flags & C_FULL)) {
				kthread_banner(cmd.ofp, BANNER|SMAJOR);
				first_time = FALSE;
			}

			get_value(cmd.args[i], &mode, K_NPROCS(K), &value);
			if (KL_ERROR) {
				KL_ERROR |= KLE_BAD_KTHREAD;
				kl_print_error(K);
				continue;
			}

			if (mode != 2) {
				KL_SET_ERROR_NVAL(KLE_BAD_KTHREAD, value, mode);
				kl_print_error(K);
			}
			else {
				kthread = value;
				ktp = kl_get_kthread(K, kthread, (cmd.flags|C_ALL));
				if (KL_ERROR) {
					/* Make sure the error value reflects the input
					 * value, not the kthread address (in case it was
					 * entered as a PID)
					 */
					KL_SET_ERROR_NVAL(KL_ERROR, value, mode);
					kl_print_error(K);
				}
				else {
					print_kthread(kthread, ktp, cmd.flags, cmd.ofp);
					free_block(ktp);
					kthread_cnt++;
				}
			}
		}
	}
	kthread_banner(cmd.ofp, SMAJOR);
	PLURAL("kthread struct", kthread_cnt, cmd.ofp);
	return(0);
}

#define _KTHREAD_USAGE "[-f] [-n] [-w outfile] [kthread_list]"

/*
 * kthread_usage() -- Print the usage string for the 'kthread' command.
 */
void
kthread_usage(command_t cmd)
{
	CMD_USAGE(cmd, _KTHREAD_USAGE);
}

/*
 * kthread_help() -- Print the help information for the 'kthread' command.
 */
void
kthread_help(command_t cmd)
{
	CMD_HELP(cmd, _KTHREAD_USAGE,
		"Display relevant information for each entry in kthread_list. If "
		"no entries are specified, display information for all active "
		"kthreads. Entries in kthread_list can take the form of a "
		" virtual address.");
}

/*
 * kthread_parse() -- Parse the command line arguments for 'kthread'.
 */
int
kthread_parse(command_t cmd)
{
	return (C_MAYBE|C_FULL|C_NEXT|C_WRITE);
}
