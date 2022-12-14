/*
 * Copyright 1985 by Silicon Grapics Incorporated
 */

#ident	"$Header: /proj/irix6.5m/isms/irix/lib/libc/src/sys/RCS/sendto.s,v 1.3 1997/05/12 14:15:48 jph Exp $"

#include <sys/regdef.h>
#include <sys/asm.h>
#include <sys.s>
#include "sys/syscall.h"

#if defined(_LIBC_ABI) || defined(_LIBC_NOMP)
SYSCALL(sendto)
	RET(sendto)
#else
#define SYS__sendto SYS_sendto
SYSCALL(_sendto)
	RET(_sendto)
#endif
