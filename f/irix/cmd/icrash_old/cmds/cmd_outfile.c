#ident  "$Header: /proj/irix6.5f/isms/irix/cmd/icrash_old/cmds/RCS/cmd_outfile.c,v 1.1 1999/05/25 19:50:14 tjm Exp $"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>
#include "icrash.h"
#include "extern.h"

/*
 * outfile_cmd() -- Sets the default output file
 */
int
outfile_cmd(command_t cmd)
{
	FILE *nfp;

	if (cmd.nargs == 0) {
		fprintf(cmd.ofp, "Current output file is : \"%s\"\n", outfile);
	} 
	else if (!strcmp(cmd.args[0], "stdout")) {
		cmd.fp = stdout;
		sprintf(outfile, "stdout");
		fprintf(cmd.ofp, "Output now directed to standard output (stdout).\n");
	} 
	else if (nfp = fopen(cmd.args[0], "a")) {
		cmd.fp = nfp;
		sprintf(outfile, "%s", cmd.args[0]);
		fprintf(cmd.ofp, "New output file is : \"%s\"\n", outfile);
	} 
	else {
		fprintf(cmd.ofp, "Cannot open file \"%s\"\n", cmd.args[0]);
		fprintf(cmd.ofp, "Current output file is : \"%s\"\n", outfile);
	}
	return(0);
}

#define _OUTFILE_USAGE "[outfile]"

/*
 * outfile_usage() -- Print the usage string for the 'outfile' command.
 */
void
outfile_usage(command_t cmd)
{
	CMD_USAGE(cmd, _OUTFILE_USAGE);
}

/*
 * outfile_help() -- Print the help information for the 'outfile' command.
 */
void
outfile_help(command_t cmd)
{
	CMD_HELP(cmd, _OUTFILE_USAGE,
		"Set outfile (the file where all command output is sent) if "
		"outfile is indicated.  Otherwise, display the current value of "
		"outfile.  If [outfile] is \"stdout\", the output file will be "
		"set to the current standard output, not to a file called "
		"\"stdout\".");
}

/*
 * outfile_parse() -- Parse the command line arguments for 'outfile'.
 */
int
outfile_parse(command_t cmd)
{
	return (C_TRUE);
}
