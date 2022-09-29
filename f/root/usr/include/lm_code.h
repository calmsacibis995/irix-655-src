/******************************************************************************

	    COPYRIGHT (c) 1990, 1996 by Globetrotter Software Inc.
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
 *	Module:	lm_code.h v3.44.0.0
 *
 *	Description: 	Encryption codes to be used in a VENDORCODE macro 
 *			for FLEXlm daemons, create_license, lm_init(),
 *			and lm_checkout() call - modify these values 
 *			for your own use.  (The VENDOR_KEYx values
 *			are assigned by Globetrotter Software).
 *
 *	example LM_CODE() macro:
 *
 *		LM_CODE(var_name, ENCRYPTION_SEED1, ENCRYPTION_SEED2,
 *					VENDOR_KEY1, VENDOR_KEY2, 
 *					VENDOR_KEY3, VENDOR_KEY4, VENDOR_KEY5);
 *	Last changed:  2/13/98
 *
 */

/*
 *	ENCRYPTION_SEED1 and 2
 *	VENDOR's private encryption seed
 *		These are 0x87654321 and 0x12345678 by default.
 *		Each vendor must ensure that you replace these with
 *		numbers which are unique to your company, and keep these
 *		numbers secret.  Only someone with access to these
 *		numbers can generate license files that will work with
 *		your application.
 *		MAKE SURE the numbers are not left to the defaults.
 */
#ifndef LM_CODE_H
#define LM_CODE_H

#define ENCRYPTION_SEED1 0x5fb36a92
#define ENCRYPTION_SEED2 0xa375cd18

/*
 *	FLEXlm vendor keys.
 *	Changing these keys has NO impact on license files (unlike 
 *	the ENCRYPTION_SEEDs).
 */

 
#define VENDOR_KEY1 0x6532ab3b
#define VENDOR_KEY2 0xd1b022ed
#define VENDOR_KEY3 0x37bc8e6a
#define VENDOR_KEY4 0x67c2582a
#define VENDOR_KEY5 0xbd846e7c

/*
 *	FLEXlm vendor name
 */

#define VENDOR_NAME "sgifd"

/*
 *	Older customers with newer versions should set 
 *	behavior defaults to previous version.  New customers
 *	Should use the current defaults, as set below
 *	Valid settings include:
 *		LM_BEHAVIOR_V2, _V3, _V4, _V5, and _V5_1
 *	See machind/RELEASE_NOTES for details.
 */

#define LM_VER_BEHAVIOR LM_BEHAVIOR_V5

#endif /* LM_CODE_H */
