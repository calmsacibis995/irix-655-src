/**************************************************************************
 *									  *
 *		 Copyright (C) 1995 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs	 contain  *
 *  unpublished	 proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may	not be disclosed  *
 *  to	third  parties	or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*
 * Warning: This file was machine generated and should not be edited.
 */
static unsigned char binom0[] = {1 };
static unsigned char binom1[] = {1, 1 };
static unsigned char binom2[] = {1, 2, 1 };
static unsigned char binom3[] = {1, 3, 3, 1 };
static unsigned char binom4[] = {1, 4, 6, 4, 1 };
static unsigned char binom5[] = {1, 5, 10, 10, 5, 1 };
static unsigned char binom6[] = {1, 6, 15, 20, 15, 6, 1 };
static unsigned char binom7[] = {1, 7, 21, 35, 35, 21, 7, 1 };
static unsigned char *binoms[] = {binom0, binom1, binom2, binom3, binom4, binom5, binom6, binom7 };
static unsigned char imbed0[] = {
	0
	};
static unsigned char imbed1[] = {
/* Number of 0 cubes in a 1 cube = 1 */
	 0,
/* Number of 1 cubes in a 1 cube = 1 */
	 0,
	};
static unsigned char imbed2[] = {
/* Number of 0 cubes in a 2 cube = 1 */
	 0, 1,
/* Number of 1 cubes in a 2 cube = 2 */
	 0, 1,
	 1, 0,
/* Number of 2 cubes in a 2 cube = 1 */
	 0, 1,
	};
static unsigned char imbed3[] = {
/* Number of 0 cubes in a 3 cube = 1 */
	 0, 1, 2,
/* Number of 1 cubes in a 3 cube = 3 */
	 0, 1, 2,
	 1, 0, 2,
	 2, 0, 1,
/* Number of 2 cubes in a 3 cube = 3 */
	 0, 1, 2,
	 0, 2, 1,
	 1, 2, 0,
/* Number of 3 cubes in a 3 cube = 1 */
	 0, 1, 2,
	};
static unsigned char imbed4[] = {
/* Number of 0 cubes in a 4 cube = 1 */
	 0, 1, 2, 3,
/* Number of 1 cubes in a 4 cube = 4 */
	 0, 1, 2, 3,
	 1, 0, 2, 3,
	 2, 0, 1, 3,
	 3, 0, 1, 2,
/* Number of 2 cubes in a 4 cube = 6 */
	 0, 1, 2, 3,
	 0, 2, 1, 3,
	 0, 3, 1, 2,
	 1, 2, 0, 3,
	 1, 3, 0, 2,
	 2, 3, 0, 1,
/* Number of 3 cubes in a 4 cube = 4 */
	 0, 1, 2, 3,
	 0, 1, 3, 2,
	 0, 2, 3, 1,
	 1, 2, 3, 0,
/* Number of 4 cubes in a 4 cube = 1 */
	 0, 1, 2, 3,
	};
static unsigned char imbed5[] = {
/* Number of 0 cubes in a 5 cube = 1 */
	 0, 1, 2, 3, 4,
/* Number of 1 cubes in a 5 cube = 5 */
	 0, 1, 2, 3, 4,
	 1, 0, 2, 3, 4,
	 2, 0, 1, 3, 4,
	 3, 0, 1, 2, 4,
	 4, 0, 1, 2, 3,
/* Number of 2 cubes in a 5 cube = 10 */
	 0, 1, 2, 3, 4,
	 0, 2, 1, 3, 4,
	 0, 3, 1, 2, 4,
	 0, 4, 1, 2, 3,
	 1, 2, 0, 3, 4,
	 1, 3, 0, 2, 4,
	 1, 4, 0, 2, 3,
	 2, 3, 0, 1, 4,
	 2, 4, 0, 1, 3,
	 3, 4, 0, 1, 2,
/* Number of 3 cubes in a 5 cube = 10 */
	 0, 1, 2, 3, 4,
	 0, 1, 3, 2, 4,
	 0, 1, 4, 2, 3,
	 0, 2, 3, 1, 4,
	 0, 2, 4, 1, 3,
	 0, 3, 4, 1, 2,
	 1, 2, 3, 0, 4,
	 1, 2, 4, 0, 3,
	 1, 3, 4, 0, 2,
	 2, 3, 4, 0, 1,
/* Number of 4 cubes in a 5 cube = 5 */
	 0, 1, 2, 3, 4,
	 0, 1, 2, 4, 3,
	 0, 1, 3, 4, 2,
	 0, 2, 3, 4, 1,
	 1, 2, 3, 4, 0,
/* Number of 5 cubes in a 5 cube = 1 */
	 0, 1, 2, 3, 4,
	};
static unsigned char imbed6[] = {
/* Number of 0 cubes in a 6 cube = 1 */
	 0, 1, 2, 3, 4, 5,
/* Number of 1 cubes in a 6 cube = 6 */
	 0, 1, 2, 3, 4, 5,
	 1, 0, 2, 3, 4, 5,
	 2, 0, 1, 3, 4, 5,
	 3, 0, 1, 2, 4, 5,
	 4, 0, 1, 2, 3, 5,
	 5, 0, 1, 2, 3, 4,
/* Number of 2 cubes in a 6 cube = 15 */
	 0, 1, 2, 3, 4, 5,
	 0, 2, 1, 3, 4, 5,
	 0, 3, 1, 2, 4, 5,
	 0, 4, 1, 2, 3, 5,
	 0, 5, 1, 2, 3, 4,
	 1, 2, 0, 3, 4, 5,
	 1, 3, 0, 2, 4, 5,
	 1, 4, 0, 2, 3, 5,
	 1, 5, 0, 2, 3, 4,
	 2, 3, 0, 1, 4, 5,
	 2, 4, 0, 1, 3, 5,
	 2, 5, 0, 1, 3, 4,
	 3, 4, 0, 1, 2, 5,
	 3, 5, 0, 1, 2, 4,
	 4, 5, 0, 1, 2, 3,
/* Number of 3 cubes in a 6 cube = 20 */
	 0, 1, 2, 3, 4, 5,
	 0, 1, 3, 2, 4, 5,
	 0, 1, 4, 2, 3, 5,
	 0, 1, 5, 2, 3, 4,
	 0, 2, 3, 1, 4, 5,
	 0, 2, 4, 1, 3, 5,
	 0, 2, 5, 1, 3, 4,
	 0, 3, 4, 1, 2, 5,
	 0, 3, 5, 1, 2, 4,
	 0, 4, 5, 1, 2, 3,
	 1, 2, 3, 0, 4, 5,
	 1, 2, 4, 0, 3, 5,
	 1, 2, 5, 0, 3, 4,
	 1, 3, 4, 0, 2, 5,
	 1, 3, 5, 0, 2, 4,
	 1, 4, 5, 0, 2, 3,
	 2, 3, 4, 0, 1, 5,
	 2, 3, 5, 0, 1, 4,
	 2, 4, 5, 0, 1, 3,
	 3, 4, 5, 0, 1, 2,
/* Number of 4 cubes in a 6 cube = 15 */
	 0, 1, 2, 3, 4, 5,
	 0, 1, 2, 4, 3, 5,
	 0, 1, 2, 5, 3, 4,
	 0, 1, 3, 4, 2, 5,
	 0, 1, 3, 5, 2, 4,
	 0, 1, 4, 5, 2, 3,
	 0, 2, 3, 4, 1, 5,
	 0, 2, 3, 5, 1, 4,
	 0, 2, 4, 5, 1, 3,
	 0, 3, 4, 5, 1, 2,
	 1, 2, 3, 4, 0, 5,
	 1, 2, 3, 5, 0, 4,
	 1, 2, 4, 5, 0, 3,
	 1, 3, 4, 5, 0, 2,
	 2, 3, 4, 5, 0, 1,
/* Number of 5 cubes in a 6 cube = 6 */
	 0, 1, 2, 3, 4, 5,
	 0, 1, 2, 3, 5, 4,
	 0, 1, 2, 4, 5, 3,
	 0, 1, 3, 4, 5, 2,
	 0, 2, 3, 4, 5, 1,
	 1, 2, 3, 4, 5, 0,
/* Number of 6 cubes in a 6 cube = 1 */
	 0, 1, 2, 3, 4, 5,
	};
static unsigned char imbed7[] = {
/* Number of 0 cubes in a 7 cube = 1 */
	 0, 1, 2, 3, 4, 5, 6,
/* Number of 1 cubes in a 7 cube = 7 */
	 0, 1, 2, 3, 4, 5, 6,
	 1, 0, 2, 3, 4, 5, 6,
	 2, 0, 1, 3, 4, 5, 6,
	 3, 0, 1, 2, 4, 5, 6,
	 4, 0, 1, 2, 3, 5, 6,
	 5, 0, 1, 2, 3, 4, 6,
	 6, 0, 1, 2, 3, 4, 5,
/* Number of 2 cubes in a 7 cube = 21 */
	 0, 1, 2, 3, 4, 5, 6,
	 0, 2, 1, 3, 4, 5, 6,
	 0, 3, 1, 2, 4, 5, 6,
	 0, 4, 1, 2, 3, 5, 6,
	 0, 5, 1, 2, 3, 4, 6,
	 0, 6, 1, 2, 3, 4, 5,
	 1, 2, 0, 3, 4, 5, 6,
	 1, 3, 0, 2, 4, 5, 6,
	 1, 4, 0, 2, 3, 5, 6,
	 1, 5, 0, 2, 3, 4, 6,
	 1, 6, 0, 2, 3, 4, 5,
	 2, 3, 0, 1, 4, 5, 6,
	 2, 4, 0, 1, 3, 5, 6,
	 2, 5, 0, 1, 3, 4, 6,
	 2, 6, 0, 1, 3, 4, 5,
	 3, 4, 0, 1, 2, 5, 6,
	 3, 5, 0, 1, 2, 4, 6,
	 3, 6, 0, 1, 2, 4, 5,
	 4, 5, 0, 1, 2, 3, 6,
	 4, 6, 0, 1, 2, 3, 5,
	 5, 6, 0, 1, 2, 3, 4,
/* Number of 3 cubes in a 7 cube = 35 */
	 0, 1, 2, 3, 4, 5, 6,
	 0, 1, 3, 2, 4, 5, 6,
	 0, 1, 4, 2, 3, 5, 6,
	 0, 1, 5, 2, 3, 4, 6,
	 0, 1, 6, 2, 3, 4, 5,
	 0, 2, 3, 1, 4, 5, 6,
	 0, 2, 4, 1, 3, 5, 6,
	 0, 2, 5, 1, 3, 4, 6,
	 0, 2, 6, 1, 3, 4, 5,
	 0, 3, 4, 1, 2, 5, 6,
	 0, 3, 5, 1, 2, 4, 6,
	 0, 3, 6, 1, 2, 4, 5,
	 0, 4, 5, 1, 2, 3, 6,
	 0, 4, 6, 1, 2, 3, 5,
	 0, 5, 6, 1, 2, 3, 4,
	 1, 2, 3, 0, 4, 5, 6,
	 1, 2, 4, 0, 3, 5, 6,
	 1, 2, 5, 0, 3, 4, 6,
	 1, 2, 6, 0, 3, 4, 5,
	 1, 3, 4, 0, 2, 5, 6,
	 1, 3, 5, 0, 2, 4, 6,
	 1, 3, 6, 0, 2, 4, 5,
	 1, 4, 5, 0, 2, 3, 6,
	 1, 4, 6, 0, 2, 3, 5,
	 1, 5, 6, 0, 2, 3, 4,
	 2, 3, 4, 0, 1, 5, 6,
	 2, 3, 5, 0, 1, 4, 6,
	 2, 3, 6, 0, 1, 4, 5,
	 2, 4, 5, 0, 1, 3, 6,
	 2, 4, 6, 0, 1, 3, 5,
	 2, 5, 6, 0, 1, 3, 4,
	 3, 4, 5, 0, 1, 2, 6,
	 3, 4, 6, 0, 1, 2, 5,
	 3, 5, 6, 0, 1, 2, 4,
	 4, 5, 6, 0, 1, 2, 3,
/* Number of 4 cubes in a 7 cube = 35 */
	 0, 1, 2, 3, 4, 5, 6,
	 0, 1, 2, 4, 3, 5, 6,
	 0, 1, 2, 5, 3, 4, 6,
	 0, 1, 2, 6, 3, 4, 5,
	 0, 1, 3, 4, 2, 5, 6,
	 0, 1, 3, 5, 2, 4, 6,
	 0, 1, 3, 6, 2, 4, 5,
	 0, 1, 4, 5, 2, 3, 6,
	 0, 1, 4, 6, 2, 3, 5,
	 0, 1, 5, 6, 2, 3, 4,
	 0, 2, 3, 4, 1, 5, 6,
	 0, 2, 3, 5, 1, 4, 6,
	 0, 2, 3, 6, 1, 4, 5,
	 0, 2, 4, 5, 1, 3, 6,
	 0, 2, 4, 6, 1, 3, 5,
	 0, 2, 5, 6, 1, 3, 4,
	 0, 3, 4, 5, 1, 2, 6,
	 0, 3, 4, 6, 1, 2, 5,
	 0, 3, 5, 6, 1, 2, 4,
	 0, 4, 5, 6, 1, 2, 3,
	 1, 2, 3, 4, 0, 5, 6,
	 1, 2, 3, 5, 0, 4, 6,
	 1, 2, 3, 6, 0, 4, 5,
	 1, 2, 4, 5, 0, 3, 6,
	 1, 2, 4, 6, 0, 3, 5,
	 1, 2, 5, 6, 0, 3, 4,
	 1, 3, 4, 5, 0, 2, 6,
	 1, 3, 4, 6, 0, 2, 5,
	 1, 3, 5, 6, 0, 2, 4,
	 1, 4, 5, 6, 0, 2, 3,
	 2, 3, 4, 5, 0, 1, 6,
	 2, 3, 4, 6, 0, 1, 5,
	 2, 3, 5, 6, 0, 1, 4,
	 2, 4, 5, 6, 0, 1, 3,
	 3, 4, 5, 6, 0, 1, 2,
/* Number of 5 cubes in a 7 cube = 21 */
	 0, 1, 2, 3, 4, 5, 6,
	 0, 1, 2, 3, 5, 4, 6,
	 0, 1, 2, 3, 6, 4, 5,
	 0, 1, 2, 4, 5, 3, 6,
	 0, 1, 2, 4, 6, 3, 5,
	 0, 1, 2, 5, 6, 3, 4,
	 0, 1, 3, 4, 5, 2, 6,
	 0, 1, 3, 4, 6, 2, 5,
	 0, 1, 3, 5, 6, 2, 4,
	 0, 1, 4, 5, 6, 2, 3,
	 0, 2, 3, 4, 5, 1, 6,
	 0, 2, 3, 4, 6, 1, 5,
	 0, 2, 3, 5, 6, 1, 4,
	 0, 2, 4, 5, 6, 1, 3,
	 0, 3, 4, 5, 6, 1, 2,
	 1, 2, 3, 4, 5, 0, 6,
	 1, 2, 3, 4, 6, 0, 5,
	 1, 2, 3, 5, 6, 0, 4,
	 1, 2, 4, 5, 6, 0, 3,
	 1, 3, 4, 5, 6, 0, 2,
	 2, 3, 4, 5, 6, 0, 1,
/* Number of 6 cubes in a 7 cube = 7 */
	 0, 1, 2, 3, 4, 5, 6,
	 0, 1, 2, 3, 4, 6, 5,
	 0, 1, 2, 3, 5, 6, 4,
	 0, 1, 2, 4, 5, 6, 3,
	 0, 1, 3, 4, 5, 6, 2,
	 0, 2, 3, 4, 5, 6, 1,
	 1, 2, 3, 4, 5, 6, 0,
/* Number of 7 cubes in a 7 cube = 1 */
	 0, 1, 2, 3, 4, 5, 6,
	};
static unsigned char *imbeds0[] = {imbed0 + 0*0 };
static unsigned char *imbeds1[] = {imbed1 + 1*0, imbed1 + 1*1 };
static unsigned char *imbeds2[] = {imbed2 + 2*0, imbed2 + 2*1, imbed2 + 2*3 };
static unsigned char *imbeds3[] = {imbed3 + 3*0, imbed3 + 3*1, imbed3 + 3*4, imbed3 + 3*7 };
static unsigned char *imbeds4[] = {imbed4 + 4*0, imbed4 + 4*1, imbed4 + 4*5, imbed4 + 4*11, imbed4 + 4*15 };
static unsigned char *imbeds5[] = {imbed5 + 5*0, imbed5 + 5*1, imbed5 + 5*6, imbed5 + 5*16, imbed5 + 5*26, imbed5 + 5*31 };
static unsigned char *imbeds6[] = {imbed6 + 6*0, imbed6 + 6*1, imbed6 + 6*7, imbed6 + 6*22, imbed6 + 6*42, imbed6 + 6*57, imbed6 + 6*63 };
static unsigned char *imbeds7[] = {imbed7 + 7*0, imbed7 + 7*1, imbed7 + 7*8, imbed7 + 7*29, imbed7 + 7*64, imbed7 + 7*99, imbed7 + 7*120, imbed7 + 7*127 };
static unsigned char **imbeds[] = {imbeds0, imbeds1, imbeds2, imbeds3, imbeds4, imbeds5, imbeds6, imbeds7 };
static unsigned char **imbed,*binom;
