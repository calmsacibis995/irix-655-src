/*
 * lmsgi.h
 *
 * This is the public interface to the flexlm library that SGI uses.
 * It greatly simplifies the number of flexlm calls needed.
 *
 *
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */

#include <sys/types.h>
#include <lmclient.h>
#include <lm_code.h>
#include <lm_attr.h>


/* Constants used to support apps written for FLEX 4.0 */

#define ENCRYPTION_CODE_1 ENCRYPTION_SEED1
#define ENCRYPTION_CODE_2 ENCRYPTION_SEED2



/* additional attributes for license_set_attr() */

#define LMSGI_NO_SUCH_FEATURE	10001	/* PTR to func returning int */
				/* function to print out error message and */
				/* other information if no such feature */
				/* is available in the current license file */

#define	LMSGI_30_DAY_WARNING	10002 /* PTR to func returning int */
				/* function to print out warning message */
				/* that the current license expires within */
				/* 30 days */
#define	LMSGI_60_DAY_WARNING	10003 /* PTR to func returning int */
				/* function to print out warning message */
				/* that the current license expires within */
				/* 60 days */
#define	LMSGI_90_DAY_WARNING	10004 /* PTR to func returning int */
				/* function to print out warning message */
				/* that the current license expires within */
				/* 90 days */

#ifdef __cplusplus
extern "C" {
#endif

extern int    license_init(VENDORCODE*, char *, boolean_t);
extern int    license_chk_out(VENDORCODE *, char *, char *);
extern int    license_chk_in(char *, int);
extern void   license_timer(void);
extern int    license_set_attr(int, LM_A_VAL_TYPE);
extern time_t license_expdate(char *);
extern char*  license_errstr(void);
extern int    license_status(char *);
extern void   license_free(void);

extern LM_HANDLE *get_job(void);

#ifdef __cplusplus
}
#endif
