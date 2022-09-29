/*************************************************************************
*                                                                        *
*               Copyright (C) 1992, Silicon Graphics, Inc.       	 *
*                                                                        *
*  These coded instructions, statements, and computer programs  contain  *
*  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
*  are protected by Federal copyright law.  They  may  not be disclosed  *
*  to  third  parties  or copied or duplicated in any form, in whole or  *
*  in part, without the prior written consent of Silicon Graphics, Inc.  *
*                                                                        *
**************************************************************************/
#ident  "$Revision: 1.6 $ $Author: kostadis $"

#include <sgidefs.h>	/* for _MIPS_SIM defs */
#ifdef __STDC__
        #pragma weak aio_write = _aio_write
#if (_MIPS_SIM == _MIPS_SIM_ABI64 || _MIPS_SIM == _MIPS_SIM_NABI32)
        #pragma weak aio_write64 = _aio_write
        #pragma weak _aio_write64 = _aio_write
#endif
#endif
#include "synonyms.h"
#include <aio.h>
#include "local.h"

/*
 * Posix 1003.1b-1993
 */
int
aio_write(struct aiocb *aio)
{
#ifdef __AIO_52_COMPAT__
	extern int old_aio_write(aiocb_t *);
	if (aio->aio_sigevent.sigev_notify < 128)
		return (old_aio_write(aio));
#endif /*  __AIO_52_COMPAT__ */
	/* Intercept if kaio */
	if (KAIO_IS_ACTIVE) {
	     struct kaio_ret kaio_ret;
	     libdba_kaio_write(aio, &kaio_ret);
	     if ((kaio_ret.back2user) == 1) {
		  return(kaio_ret.rv);
	     }
	}
	return(_aqueue(aio, LIO_WRITE, NULL));
}
