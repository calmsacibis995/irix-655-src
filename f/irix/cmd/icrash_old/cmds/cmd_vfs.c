#ident  "$Header: /proj/irix6.5f/isms/irix/cmd/icrash_old/cmds/RCS/cmd_vfs.c,v 1.1 1999/05/25 19:50:14 tjm Exp $"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>
#include "icrash.h"
#include "extern.h"

/*
 * vfs_cmd() -- Perform a 'vfs' lookup on some virtual file system pointer
 */
int
vfs_cmd(command_t cmd)
{
	int i, vfs_cnt = 0;
	kaddr_t value;
	k_ptr_t vfsbuf, vfsp;

	vfsbuf = alloc_block(STRUCT("vfs"), B_TEMP);
	if (!(cmd.flags & C_FULL)) {
		vfs_banner(cmd.ofp, BANNER|SMAJOR);
	}
	for (i = 0; i < cmd.nargs; i++) {
		if (cmd.flags & C_FULL) {
			vfs_banner(cmd.ofp, BANNER|SMAJOR);
		}
		GET_VALUE(cmd.args[i], &value);
		if (KL_ERROR) {
			KL_ERROR |= KLE_BAD_VFS;
			kl_print_error(K);
		}
		else {
			vfsp = get_vfs(value, vfsbuf, cmd.flags);
			if (KL_ERROR) {
				kl_print_error(K);
			}
			else {
				vfs_cnt += print_vfs(value, vfsbuf, cmd.flags, cmd.ofp);
			}
		}
	}
	vfs_banner(cmd.ofp, SMAJOR);
	PLURAL("vfs struct", vfs_cnt, cmd.ofp);
	free_block(vfsbuf);
	return(0);
}

#define _VFS_USAGE "[-f] [-w outfile] vfs_list"

/*
 * vfs_usage() -- Print the usage string for the 'vfs' command.
 */
void
vfs_usage(command_t cmd)
{
	CMD_USAGE(cmd, _VFS_USAGE);
}

/*
 * vfs_help() -- Print the help information for the 'vfs' command.
 */
void
vfs_help(command_t cmd)
{
	CMD_HELP(cmd, _VFS_USAGE,
		"Display the vfs structure for each virtual address included "
		"in vfs_list.");
}

/*
 * vfs_parse() -- Parse the command line arguments for 'vfs'.
 */
int
vfs_parse(command_t cmd)
{
	return (C_TRUE|C_WRITE|C_FULL);
}
