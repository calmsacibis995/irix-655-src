/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.2 88/02/08 
 */


/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */

#ifdef __STDC__
	#pragma weak svc_run = _svc_run
#endif
#include "synonyms.h"

#include <rpc/rpc.h>
#include <rpc/errorhandler.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>		/* prototype for select() */
#include <string.h>		/* prototype for strerror() */

void
svc_run(void)
{
	fd_set readfds;

	for (;;) {
		readfds = svc_fdset;
		switch (select(FD_SETSIZE, &readfds, (fd_set *)0, (fd_set *)0,
			       (struct timeval *)0)) {
		case -1:
			if (errno == EINTR) {
				continue;
			}
			_rpc_errorhandler(LOG_ERR, "svc_run: - select failed: %s", strerror(errno));
			return;
		case 0:
			continue;
		default:
			svc_getreqset(&readfds);
		}
	}
}
