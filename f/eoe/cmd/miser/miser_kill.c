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
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syssgi.h>

#define PIDDIGITS 10

miser_data_t miser_data;

int
main(int argc, char ** argv)
{
	time_t time_quantum;
	int c;
	pid_t bid = -1, pid;
	uid_t uid;
	char *sbid;
	char *path;
	struct sched_param sparam;
	int fd;
	void *p;
	prpsinfo_t j;

	while ((c = getopt(argc, argv, "j:")) != -1) {
		switch (c) {
		case 'j':
			bid = atoi(optarg);
			break;
		default:
			merror("usage: miser_kill -j bid");
			exit(1);
			break;
		}
	}
	
	if(bid == -1) {
		merror("usage: miser_kill -j bid");
		exit(1);
	}
	/* get the first proc in the proc group.
	 * the proc group leader (bid) may not exist, but there are other 
	 * members in the group.
	 */
	if (syssgi(SGI_GETGRPPID, bid, &pid, sizeof(pid_t)) <= 0) {
		merror("No process in bid %d\n", bid);
		exit(1);
	}

	/* open one of the proc in the group to make sure it is
	 * a batch job.
	 */
	path = malloc(strlen("/proc/pinfo/") + 1 + PIDDIGITS);
       	memset(path, 0, strlen("/proc/pinfo/") + 1 + PIDDIGITS);
       	sprintf(path, "/proc/pinfo/%010d", pid);
	/* printf("%s\n", path); */

	fd = open(path, O_RDONLY);
	if ((fd == -1) || (ioctl(fd, PIOCPSINFO, &j) == -1)){
		merror("Invalid bid");
		exit(1);
	}
	/* printf("%s\n",j.pr_clname); */

	if((strcmp(j.pr_clname, "B")) && (strcmp(j.pr_clname, "BC"))){
		printf("Pid %d not a batch bid\n", bid);
		exit(1);
	}

	uid = getuid();
	setuid(uid);

	if (kill (-bid, SIGKILL) != 0) {
		if (errno == EPERM) {
			merror("Do not have persmission to kill bid %d", bid);
		} else if (errno == ESRCH) {
			merror("No process can be found corresponding to %d",
								bid);
		} 
		exit(1);
	}

	if (strcmp(j.pr_clname, "BC"))
		return 0;

	miser_data.md_request_type = MISER_USER_JOB_KILL;
	memset(miser_data.md_data, 0, MISER_REQUEST_BUFFER_SIZE);
	memcpy(miser_data.md_data, &bid, sizeof(bid));

	/* Must be set before passing it in */
	miser_data.md_error = MISER_ERR_OK;

	if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
		merror("Could not contact miser: %s", strerror(errno));
		exit(1);
	}

	if (miser_data.md_error) {
		merror("miser kill failed: %s",
				 miser_error(miser_data.md_error));
		return 1;
	}
	return 0;
}
