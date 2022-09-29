/*
 * eoe/cmd/miser/Debug.h
 *	Miser debug prototypes and inline functions.
 */

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ifndef __DEBUG__HEADER__
#define __DEBUG__HEADER__
#ifdef DEBUG

#include "iostream.h"


/* Print a debug string and file name and line number */
#define STRING_PRINT(string) \
	cerr << "DBG: " << string << " (" << __FILE__ << ":" << __LINE__ \
	<< ")" << endl;


/* Print a debug symbol and value pair, and the file name and line number */
#define SYMBOL_PRINT(symbol) \
	cerr << "DBG: " << #symbol << " = " << symbol << " (" << __FILE__ \
	<< ":" << __LINE__ << ")" << endl; 


/* Print a debug function trace message, and the file name and line number */
#define TRACE(func) \
	cerr << "Call: " << #func <<  " (" << __FILE__ << ":" << __LINE__ \
	<< ")" << endl; func;

#else


#define STRING_PRINT(string) ;
#define SYMBOL_PRINT(symbol) ;
#define TRACE(func) ;


#endif

#endif /* DEBUG_HEADER */
