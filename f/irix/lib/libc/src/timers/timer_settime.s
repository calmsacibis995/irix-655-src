 /*************************************************************************
 #									  *
 # 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 #									  *
 #  These coded instructions, statements, and computer programs  contain  *
 #  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 #  are protected by Federal copyright law.  They  may  not be disclosed  *
 #  to  third  parties  or copied or duplicated in any form, in whole or  *
 #  in part, without the prior written consent of Silicon Graphics, Inc.  *
 #									  *
 #************************************************************************/
#include <sys/regdef.h>
#include <sys/asm.h>
#include <sys.s>
#include "sys/syscall.h"

SYSCALL(timer_settime)
	RET(timer_settime)

