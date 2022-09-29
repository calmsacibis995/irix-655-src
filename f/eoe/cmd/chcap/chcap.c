/*
 * COPYRIGHT NOTICE
 * Copyright 1995, Silicon Graphics, Inc. All Rights Reserved.
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or 
 * duplicated in any form, in whole or in part, without the prior written 
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions 
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or 
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished - 
 * rights reserved under the Copyright Laws of the United States.
 *
 */

/*
 * chcap - set the requested capability set on files.
 *
 */

#ident	"$Revision: 1.1 $"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/capability.h>

int
main (int argc, char *argv[])
{
	cap_t capset;		/* capabilities on file */
	char *new_string;
	char *old_string;
	size_t new_len;
	size_t old_len;
	int i;

	/*
	 * Check number of arguments
	 */
	if (argc <= 1) {
		fprintf (stderr, "%s: No capabilities specified\n", argv[0]);
		exit (1);
	}
	if (argc == 2) {
		fprintf (stderr, "%s: No files specified\n", argv[0]);
		exit (1);
	}

	/*
	 * The second argument is the delta to the old capability state
	 */
	if ((capset = cap_from_text(argv[1])) == NULL) {
		fprintf (stderr, "%s: Invalid capability specification\n",
		    argv[0]);
		exit (1);
	}
	cap_free(capset);
	new_len = strlen(argv[1]);

	/*
	 * Set file capabilities. Report errors for each file.
	 */
	for (i = 2; i < argc; i++) {
		if ((capset = cap_get_file(argv[i])) == NULL) {
			perror (argv[i]);
			continue;
		}
		old_string = cap_to_text(capset, &old_len);
		cap_free(capset);
		
		if (old_string == NULL) {
			perror (argv[i]);
			continue;
		}

		new_string = malloc(old_len + new_len + 2);
		sprintf(new_string, "%s %s", old_string, argv[1]);
		cap_free(old_string);

		capset = cap_from_text(new_string);
		cap_free(new_string);

		if (capset == NULL) {
			perror (argv[i]);
			continue;
		}

		if (cap_set_file(argv[i], capset) == -1)
			perror (argv[i]);

		cap_free(capset);
	}

	return (0);
}
