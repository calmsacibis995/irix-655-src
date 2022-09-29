/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#include <sys/miser_public.h>
#include <sys/sysmp.h>
#include <stdio.h>
#include <errno.h>
#include "libmiser.h"

miser_data_t miser_data;


int
main(int argc, char ** argv)
{
	time_t time_quantum;

	if (argc != 3 || argv[1][0] != '-' && argv[1][1] != 'f') {
		merror("usage: miser_reset -f filename ");
		exit(1);
	}	
	miser_data.md_request_type = MISER_ADMIN_QUEUE_MODIFY_RESET;

	if (strlen(argv[2]) >=  MISER_REQUEST_BUFFER_SIZE) {
		merror("file name and path must be less than %d characters long",
			MISER_REQUEST_BUFFER_SIZE);
		exit(1);
	}
	memset(miser_data.md_data, 0, MISER_REQUEST_BUFFER_SIZE);
	strcpy(miser_data.md_data, argv[2]);
	if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
		merror("Could not contact miser: %s", strerror(errno));
		exit(1);
	}

	if (miser_data.md_error) {
		merror("miser reset failed: %s",
				 miser_error(miser_data.md_error));
		return 1;
	}
	return 0;
}
