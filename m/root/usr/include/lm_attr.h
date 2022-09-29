/******************************************************************************

	    COPYRIGHT (c) 1990, 1998 by Globetrotter Software Inc.
	This software has been provided pursuant to a License Agreement
	containing restrictions on its use.  This software contains
	valuable trade secrets and proprietary information of 
	Globetrotter Software Inc and is protected by law.  It may 
	not be copied or distributed in any form or medium, disclosed 
	to third parties, reverse engineered or used in any manner not 
	provided for in said License Agreement except with the prior 
	written authorization from Globetrotter Software Inc.

 *****************************************************************************/
/*	
 *	Module:	lm_attr.h v3.51.0.0
 *
 *	Description: 	Attribute tags for FLEXlm setup parameters.
 *
 *	M. Christiano
 *	5/3/90
 *
 *	Last changed:  5/27/98
 *
 */

#define LM_A_DECRYPT_FLAG	1	/* (short) */
#define LM_A_DISABLE_ENV	2	/* (short) */
#define LM_A_LICENSE_FILE	3	/* (char *) -- obsolete in v5 */
#define LM_A_CRYPT_CASE_SENSITIVE 4	/* (short) */
#define LM_A_GOT_LICENSE_FILE	5	/* (short) */
#define LM_A_CHECK_INTERVAL	6	/* (int) */
#define LM_A_RETRY_INTERVAL	7	/* (int) */
#define LM_A_TIMER_TYPE		8	/* (int) */
#define LM_A_RETRY_COUNT	9	/* (int) */
#define	LM_A_CONN_TIMEOUT	10	/* (int) */
#define	LM_A_NORMAL_HOSTID	11	/* (short) */
#define LM_A_USER_EXITCALL	12	/* PTR to func returning int */
#define	LM_A_USER_RECONNECT	13	/* PTR to func returning int */
#define LM_A_USER_RECONNECT_DONE 14	/* PTR to func returning int */
#define LM_A_USER_CRYPT		15	/* PTR to func returning (char *) */
#define	LM_A_USER_OVERRIDE	16	/* (char *) */
#define LM_A_HOST_OVERRIDE	17	/* (char *) */
#define LM_A_PERIODIC_CALL	18	/* PTR to func returning int */
#define LM_A_PERIODIC_COUNT	19	/* (int) */
#define LM_A_NO_TRAFFIC_ENCRYPT	21	/* (short) */
#define LM_A_USE_START_DATE	22	/* (short) */
#define LM_A_MAX_TIMEDIFF	23	/* (int) */
#define LM_A_DISPLAY_OVERRIDE	24	/* (char *) */
#define LM_A_ETHERNET_BOARDS	25	/* (char **) */
#define LM_A_LINGER		27	/* (long) */
/* 
 *	#28 RESERVED -- it used to be LM_A_CUR_JOB, and if a 
 *	user uses that number, we want it to error out
 *	#define LM_A_CUR_JOB		28	 (LM_HANDLE *) 
 */

#define LM_A_SETITIMER		29	/* PTR to func returning void, eg PFV */
#define LM_A_SIGNAL		30	/* PTR to func returning PTR to */
					/*    function returning void, eg:
					      PFV (*foo)(); 	*/
#define LM_A_TRY_COMM		31	/* (short) Try old comm versions */
#define LM_A_VERSION		32	/* (short) FLEXlm version */
#define LM_A_REVISION		33	/* (short) FLEXlm revision */
#define LM_A_COMM_TRANSPORT	34	/* (short) Communications transport */
					/*	  to use (LM_TCP/LM_UDP) */
#define LM_A_CHECKOUT_DATA	35	/* (char *) Vendor-defined checkout  */
					/*				data */
#define LM_A_PROCESS_UPGRADE	36	/* (short) Process UPGRADE lines? */
#define LM_A_DIAGS_ENABLED	37	/* (short) Allow FLEXlm diag output */
#define LM_A_REDIRECT_VERIFY	38	/* Verification routine for hostid */
					/* redirect (NULL -> NO redirection) */
					/* PTR to func returning (char *) */
#define LM_A_HOSTID_PARSE	39	/* Vendor-defined hostid parsing */
					/* routine: PTR to func returning int */
#define LM_A_VENDOR_GETHOSTID	40	/* Return Vendor-defined hostid */
					/* PTR to func returning (HOSTID *) */
#define LM_A_VENDOR_PRINTHOSTID	41	/* Print Vendor-defined hostid */
					/* PTR to func returning (char *) */
#define LM_A_VENDOR_CHECKID	42	/* Compare 2 Vendor-defined hostids */
					/* PTR to func returning int */
#define LM_A_UDP_TIMEOUT	43	/* (int) If UDP client doesn't send */
					/* heartbeart, then it is timed out */
					/* default = 3 minutes, although */
					/* actual timeout can take two minutes*/
					/* longer */
#define LM_A_ALLOW_SET_TRANSPORT 44 /* (int) If this is true, users */
					/* can reset COMM_TRANSPORT. */
					/* default = true: With default, */
					/* users can reset COMM_TRANSPORT via */
					/* COMM_TRANSPORT line in license */
					/* file or FLEXLM_COMM_TRANSPORT */
					/* environment variable. */
#define LM_A_CHECKOUTFILTER	45	/* Vendor-defined checkout filter */
					/* PTR to func returning int */
#define LM_A_LICENSE_FILE_PTR	46	/* (char *) - for lm_get_attr use  */
					/* char *s; lm_get_attr(..,&s);  */
					/* -- obsolete in v5 */
#define LM_A_ALT_ENCRYPTION	47	/* (VENDORCODE *) - for alternate */
					/* encryption codes */
#define LM_A_VD_GENERIC_INFO	48	/* PTR to LM_VD_GENERIC_INFO struct */
					/* struct type set to: */
					/* LM_VD_GENINFO_HANDLE_TYPE, */
					/* struct feat set to (CONFIG *) */
					/* returned from lm_next_conf() */
					/* after lh_get_attr, values returned */
					/* in struct (see lm_client.h) */
#define LM_A_VD_FEATURE_INFO	49	/* PTR to LM_VD_FEATURE_INFO struct */
					/* struct type set to: */
					/* LM_VD_FEATINFO_HANDLE_TYPE, */
					/* struct feat set to (CONFIG *) */
					/* returned from lm_next_conf() */
					/* after lh_get_attr, values returned */
					/* in struct (see lm_client.h) */
#if 0 /* discontinued in v5 */
#define LM_A_MASTER		50	/* (char *),lm_get_attr only, returns */
					/* name of master node for red. servs */
#endif
#define LM_A_LF_LIST		51	/* (char ***) -- returns null- */
					/* terminated list of filenames */
#define	LM_A_PLATFORM_OVERRIDE	52	/* (char *) */
#ifdef LM_INTERNAL
#define LM_A_PORT_HOST_PLUS   53        /* Internal Use Only */
#endif
#define LM_A_LICENSE_DEFAULT	56	/* (char *) -- use this instead of */
					/* LM_A_LICENSE_FILE or  */
					/* LM_A_LICENSE_FILE_PTR */
#define LM_A_CAPACITY		57	/* (short) */
#define LM_A_VENDOR_ID_DECLARE  58	/* set: PTR to LM_VENDOR_HOSTID struct*/
					/* get: PTR to LM_VENDOR_HOSTID_PTR */
#define LM_A_GENERIC_SERVER_OK	59	/* defaults to NOT-OK (0) */
#define LM_A_USING_GENERIC_SERVER 60	/* true if flexlmd is server 
					   get_attr ONLY */
#define LM_A_RETRY_CHECKOUT 	61	/* (int) default: 0 (false). 
					   Retry checkout if failure is due 
					   to communications error */
#define LM_A_SUPPORT_HP_IDMODULE 62	/* (int) default: 0 (false)
					   Not recommended, since it can
					   impact performance, and requires
					   use of SIGALRM */
#define LM_A_TCP_TIMEOUT	63	/* (int) If TCP client doesn't send */
					/* heartbeart, then it is timed out */
					/* default = 2 hours */
#define LM_A_CHECK_BADDATE	64	/* default = false */
#define LM_A_BEHAVIOR_VER	65	/* (char *) LM_BEHAVIOR_V6, etc. 
					   (ptr to char *) in lc_get_attr */
#define LM_A_LKEY_START_DATE	66	/* (int) default false-sdate in lkey */
#define LM_A_LKEY_LONG		67	/* (int) default false - 64-bit lkey*/
#define LM_A_LONG_ERRMSG	68	/* (int) error messages fmt */
#define LM_A_LICENSE_CASE_SENSITIVE 69	/* (int) default false */
#define LM_A_LICENSE_FMT_VER	70	/* (char *) LM_BEHAVIOR_V6, etc. 
					 * default LM_BEHAVIOR_CURRENT
					 */

#define LM_A_PC_PROMPT_FOR_FILE	71	/* (int) default true */
#define LM_A_PROMPT_FOR_FILE	LM_A_PC_PROMPT_FOR_FILE	 /* alias */
#define LM_A_MAX_LICENSE_LEN	72	/* (int) default true */
#define LM_A_INTERNAL1		73	/* for internal use only */
#define LM_A_USER_CRYPT_FILTER	74	/* PTR to func taking job, int, int, 
								int */
#define LM_A_USER_CRYPT_FILTER_GEN 75	/* PTR to func taking job, int, int */

#ifdef VMS
#define LM_A_EF_1		1001	/* (int) */
#define LM_A_EF_2		1002	/* (int) */
#define LM_A_EF_3		1003	/* (int) */
#define LM_A_EF_4		1004	/* (int) */
#define LM_A_EF_5		1005	/* (int) */
#define LM_A_VMS_MAX_LINKS	1006	/* (int) */
#endif
