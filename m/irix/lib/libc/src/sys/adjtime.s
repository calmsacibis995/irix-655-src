/*
 * Copyright 1985 by Silicon Grapics Incorporated
 */

#ident	"$Header: /proj/irix6.5m/isms/irix/lib/libc/src/sys/RCS/adjtime.s,v 1.2 1987/10/24 11:42:09 jmb Exp $"

#include <sys/regdef.h>
#include <sys/asm.h>
#include <sys.s>
#include "sys/syscall.h"

SYSCALL(adjtime)
	RET(adjtime)
