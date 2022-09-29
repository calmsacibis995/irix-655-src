#pragma once

// $Revision: 1.33 $
// $Date: 1991/12/16 16:37:11 $
#include "fourBits.h"

// design constants for standard dialogues, menus and buttons

	// a negative color index is used to indicate a pair of dither indices
	// for use in double-buffered color index mode on an 8-bit-plane system.
	// Given a pair of 4-bit indices (row and column of the 16x16 matrix
	// of possibile 2-spot dithered colors), ditherIndex encodes them
	// as a single negative color index, suitable for use by the
	// withStdColor() dithering utility.
#ifndef NO_DITHER
#define ditherIndex(row,col) (-(((row)<<4)+(col)))
#else
#define ditherIndex(row,col) (row)
#endif

    // must match slots set in stdColors.c++
#define BASE_COLOR 16
#define SLOT0_RGBCOLOR		  0,   0,   0
#define SLOT1_RGBCOLOR		255,   0,   0
#define SLOT2_RGBCOLOR		  0, 255,   0
#define SLOT3_RGBCOLOR		255, 255,   0
#define SLOT4_RGBCOLOR		  0,   0, 255
#define SLOT5_RGBCOLOR		255,   0, 255
#define SLOT6_RGBCOLOR		  0, 255, 255
#define SLOT7_RGBCOLOR		255, 255, 255
#define SLOT8_RGBCOLOR		 85,  85,  85
#define SLOT9_RGBCOLOR		198, 113, 113
#define SLOT10_RGBCOLOR		113, 198, 113
#define SLOT11_RGBCOLOR		142, 142,  56
#define SLOT12_RGBCOLOR		113, 113, 198
#define SLOT13_RGBCOLOR		142,  56, 142
#define SLOT14_RGBCOLOR		 56, 142, 142
#define SLOT15_RGBCOLOR		170, 170, 170

#define BG_COLOR		15	// Dialogue background is Light Gray
#define BG_ICOLOR		ditherIndex(7,15)
#define BG_RGBCOLOR		213,213,213	// dither indices 7,15
#define BG_EDGE_ICOLOR		ditherIndex(0,0)
#define BG_EDGE_RGBCOLOR	0,0,0		// dither indices 7,15

#define QUIET_COLOR	10     		// Colors used in a Visual state
#define QUIET_ICOLOR	ditherIndex(12,14)
#define QUIET_RGBCOLOR	85,128,170	// dither indices 12,14
#define QTXT_COLOR	0
//#define QTXT_ICOLOR	ditherIndex(0,0)
//  for compatibility with textView
#define QTXT_ICOLOR	0
#define QTXT_RGBCOLOR	0,0,0		// index 0
#define SELECT_COLOR	0
#define SELECT_ICOLOR	ditherIndex(13,13)
#define SELECT_RGBCOLOR	142,56,142	// index 13
#define STXT_COLOR	7
//#define STXT_ICOLOR	ditherIndex(7,7)
//  for compatibility with textView
#define STXT_ICOLOR	7
#define STXT_RGBCOLOR	255,255,255	// index 7
#define LOCATE_COLOR	14
#define LOCATE_ICOLOR	ditherIndex(12,12)
#define LOCATE_RGBCOLOR	113,113,198	// index 12
#define LTXT_COLOR	0
//#define LTXT_ICOLOR	ditherIndex(0,0)
//  for compatibility with textView
#define LTXT_ICOLOR	0
#define LTXT_RGBCOLOR	0,0,0		// index 0

#define MENU_BTN_HT	float(35.0)	// Menu Buttons should "look" different
#define MENU_BTN_WD	float(75.0)	// than radio or checklist buttons
#define MENU_BTN_CRV	float(1.0)
#define MENU_BTN_OFF	float(7.0)
			/* Spacing between buttons */
#define MENU_BTN_SPACE	float(MENU_BTN_OFF*4.0)
			/* Spacing between groups of buttons */

#define PICK_BTN_HT	float(29.0)	//  Works better with an odd size.
#define PICK_BTN_WD	float(75.0)

#define BTN_BORDER_COL		0	
#define BTN_BORDER_ICOLOR	ditherIndex(0,0)
#define BTN_BORDER_RGBCOLOR	0,0,0	// index 0

#define BTN_BORDER_LW	3	

#define BUTTON_FONT	"Helvetica"
#define BUTTON_FSIZE    11.0

#define HEADING_COLOR		0   /* specs for displaying static text	*/
#define HEADING_ICOLOR		ditherIndex(0,0)
#define HEADING_RGBCOLOR	0,0,0	// index 0

#define HEADING_FONT	"Helvetica-Bold"
#define HEADING_FSIZE   11.0
#define HEADING_HT	22.0

#define BOUND_TILE	float(4.0) /* Offsets used when tiling generic buttons*/
#define UNBOUND_TILE	float(12.0)

#define MENU_HOFF	float(20.0) /* Offsets used when tiling menu buttons */
#define MENU_YOFF	float(20.0)


// design constants for the workspace and dirview

#define EM_PICK_BTN_HT	float(31.0)
#define EM_PICK_BTN_WD	float(61.0)

#define LABEL_FONT	"Helvetica"
#define LABEL_FSIZE	11.0
#define	COPYRIGHT_FSIZE	9.0

#define W_BG_COLOR	10
#define W_BG_ICOLOR	ditherIndex(12,14)
#define W_BG_RGBCOLOR	85,128,170	// dither indices 12,14
#define D_BG_COLOR	8
#define D_BG_ICOLOR	ditherIndex(10,12)
#define D_BG_RGBCOLOR	113,156,156	// index 10,12
#define L_BG_COLOR	15
#define L_BG_ICOLOR	ditherIndex(7,15)
#define L_BG_RGBCOLOR	213,213,213	// dither indices 7,15

#define ICONWIDTH		float(55.0)
#define ICONHEIGHT		float(55.0)

#define LIST_ICONWIDTH		float(25.0)
#define LIST_ICONHEIGHT		float(25.0)

#define ICON_OUTLINE_COLOR		0
#define ICON_OUTLINE_ICOLOR		ditherIndex(0,0)
#define ICON_OUTLINE_RGBCOLOR		0,0,0		// index 0

#define ICON_BACK_COLOR			11
#define ICON_BACK_ICOLOR		ditherIndex(8,8)
#define ICON_BACK_RGBCOLOR		85,85,85	// index 8

#define ICON_QUIET_COLOR		15
#define ICON_QUIET_ICOLOR		ditherIndex(7,15)
#define ICON_QUIET_RGBCOLOR		213,213,213	// dither indices 7,15

#define ICON_LOCATE_COLOR		7
#define ICON_LOCATE_ICOLOR		ditherIndex(7,7)
#define ICON_LOCATE_RGBCOLOR		255,255,255	// index 7

#define ICON_LOCATE_SELECT_COLOR	3
#define ICON_LOCATE_SELECT_ICOLOR	ditherIndex(3,3)
#define ICON_LOCATE_SELECT_RGBCOLOR	255,255,0	// index 3

#define ICON_SELECT_COLOR		3
#define ICON_SELECT_ICOLOR		ditherIndex(3,3)
#define ICON_SELECT_RGBCOLOR		255,255,0	// index 3

#define ICON_DIM_SELECT_COLOR		12
#define ICON_DIM_SELECT_ICOLOR		ditherIndex(3,15)
#define ICON_DIM_SELECT_RGBCOLOR	213,213,85	// dither indices 3,15

#define ICON_DRAG_FRONT			3
#define ICON_DRAG_FRONT_ICOLOR		ditherIndex(3,3) /* use in overlays */
#define ICON_DRAG_FRONT_RGBCOLOR	ICON_SELECT_RGBCOLOR
#define ICON_DRAG_BACK			2
#define ICON_DRAG_BACK_ICOLOR		ditherIndex(2,2) /* use in overlays */
#define ICON_DRAG_BACK_RGBCOLOR		ICON_BACK_RGBCOLOR

#define ICON_FONT		"Helvetica"
#define ICON_FSIZE		11.0

#define PANELHEIGHT		float(76.0)
#define WINDOW_X_LEN		716
#define WINDOW_Y_LEN		800
#define CON_LINE_WIDTH		1


// design constants for scrollbar/slider/arrow buttons

#define	SBAR_BG_COLOR		14
#define	SBAR_BG_ICOLOR		ditherIndex(15,15)
#define	SBAR_BG_RGBCOLOR	170,170,170	// index 15

#define	SBAR_EDGE_COLOR		0
#define	SBAR_EDGE_ICOLOR	ditherIndex(0,0)
#define	SBAR_EDGE_RGBCOLOR	0,0,0		// index 0

#define	SBAR_EDGE_WIDTH		1

#define	SBAR_QUIET_COLOR	15
#define	SBAR_QUIET_ICOLOR	ditherIndex(7,15)
#define	SBAR_QUIET_RGBCOLOR	213,213,213	// dither indices 7,15

#define	SBAR_LOCATED_COLOR	7
#define	SBAR_LOCATED_ICOLOR	ditherIndex(7,7)
#define	SBAR_LOCATED_RGBCOLOR	255,255,255	// index 7

#define	SBAR_SELECTED_COLOR	0
#define	SBAR_SELECTED_ICOLOR	ditherIndex(7,7)
#define	SBAR_SELECTED_RGBCOLOR	255,255,255	// index 7

#define	SBAR_DISABLED_COLOR	14
#define	SBAR_DISABLED_ICOLOR	ditherIndex(15,15)
#define	SBAR_DISABLED_RGBCOLOR	SBAR_BG_RGBCOLOR

#define	SBAR_DIVOT_COLOR	1
#define	SBAR_DIVOT_ICOLOR	ditherIndex(8,8)
#define	SBAR_DIVOT_RGBCOLOR	85,85,85	// index 8

// generic error color

#define	ERROR_COLOR	3
#define	ERROR_ICOLOR	ditherIndex(3,3)
#define	ERROR_RGBCOLOR	255,255,0	// index 3

//  sizes for scrollBars (moved from tkScrollView.h)

#define SCROLLWIDTH	float(22.0)
#define SCROLLWIDTH_H	float(22.0)

// design constants for text fields

#define TVIEW_BORDER_LW		0
#define TVIEW_HEIGHT		float(33.0)

#define TVIEW_BG_COLOR		164
#define TVIEW_BG_ICOLOR		ditherIndex(9,15)
#define TVIEW_BG_RGBCOLOR	184,141,141	// dither indices 9, 15

#define TVIEW_EDGE_COLOR	0
#define	TVIEW_EDGE_ICOLOR	ditherIndex(0,0)
#define TVIEW_EDGE_RGBCOLOR	0,0,0

//#define	TVIEW_TEXT_ICOLOR	ditherIndex(0,0)
//  for compatability with textView
#define TVIEW_TEXT_COLOR	0
#define TVIEW_TEXT_ICOLOR	0
#define	TVIEW_TEXT_RGBCOLOR	0,0,0

#define TVIEW_SEL_COLOR		52
#define	TVIEW_SEL_ICOLOR	ditherIndex(7,15)
#define	TVIEW_SEL_RGBCOLOR	212,212,212	// dither indices 7, 15

//  Constants used to define stipple patterns in stdParts, etc.

#define ONLS1	100 //	unique number
#define ONLS0	101
#ifndef NO_DITHER
#define ONLS1_PAT 0x5555	    // on-off pattern that starts on.
#define ONLS0_PAT 0xAAAA
#else
#define ONLS1_PAT 0xFFFF	    // Don't dither--write every pixel.
#define ONLS0_PAT 0xFFFF
#endif
#define PATNUM0	100
#define PATNUM1	101
