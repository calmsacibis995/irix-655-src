/*
 * Copyright 1985 by Silicon Grapics Incorporated
 */

#ident	"$Header: /proj/irix6.5f/isms/irix/lib/libc/src/sys/RCS/getsocknm.s,v 1.2 1987/10/24 11:42:27 jmb Exp $"

#include <sys/regdef.h>
#include <sys/asm.h>
#include <sys.s>
#include "sys/syscall.h"

SYSCALL(getsockname)
	RET(getsockname)
