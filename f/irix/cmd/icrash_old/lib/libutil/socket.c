#ident "$Header: /proj/irix6.5f/isms/irix/cmd/icrash_old/lib/libutil/RCS/socket.c,v 1.1 1999/05/25 19:50:14 tjm Exp $"

#include <stdio.h>
#include <sys/types.h>
#include "icrash.h"
#include "extern.h"

/*
 * get_tcpaddr() -- Print and format an ipaddr into a tcpaddr.
 */
void
get_tcpaddr(unsigned ipaddr, short port, char *str)
{
	int i;
	char s[16];

	if (DEBUG(DC_GLOBAL, 3)) {
		fprintf(stderr, "get_tcpaddr: ipaddr=0x%x, port=0x%x, str=%s\n",
			ipaddr, port, str);
	}

	str[0] = 0;
	for (i = 24; i >= 0; i -= 8) {
		sprintf(s, "%d.", (ipaddr >> i) &0xff);
		strcat(str, s);
	}
	sprintf(s, "%d", port & 0xffff);
	strcat(str, s);
}
