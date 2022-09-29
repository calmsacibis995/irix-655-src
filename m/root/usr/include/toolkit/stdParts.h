#pragma once

// $Revision: 1.16 $
// $Date: 1990/08/09 14:34:32 $
#include "tkModel.h"
#include "tkPen.h"
#include "tkScrollBar.h"
#include "tkButton.h"
#include "stddefs.h"
#include "upArrowModel.h"
#include "leftArrowModel.h"
#include "rightArrowModel.h"
#include "downArrowModel.h"
#include "tkTextView.h"
#include "tkBorderModel.h"
#include "BorderPntView.h"
#include "ScrollList.h"

// Create a model which has a tkLabel and a tkRoundRectangle in it.
// The label is centered within the width & height dimensions.
// The pen is used as follows:
//	- foreground color is the color of the tkLabel's text
//	- background color is the color of the tkRoundRectangle
//	- edge color is the edge color of the tkRoundRectangle
//	- line width is the line width of the tkRoundRectangle
tkModel*	makeCenteredLabel(char* text, tkPen const& pen, float width,
					float height);

tkButton*	makeStandardCheckBox(tkPen const& p, float width, float height);

// Construct an arrow, inset one pixel inside the box defined by
// w and h.  state is a tkBUTTON state.
// The arrow has a transparent background
// (watch for memory leak if you are assuming that you have a tkModelItem --
//  leak caused by lack of virtual distructors -- maybe should add close)
upArrowModel*	makeStandardUpArrow(int w, int h, int state);
downArrowModel*	makeStandardDownArrow(int w, int h, int state);
leftArrowModel*	makeStandardLeftArrow(int w, int h, int state);
rightArrowModel* makeStandardRightArrow(int w, int h, int state);


// Create a scrollbar/slider complete with models.
// The pen is used as follows:
//	- foreground color is the color of the valuator thumb
//	- background color is the color of the scrollbar background
//	- edge color is the edge color of the thumb
//	- line width is the width of the thumb outline

//  NOTE:   The pen is IGNORED in the current code!

tkValuator*	makeStandardSlider(Bool horizontal, Box2 const& b, tkPen const& p);
tkScrollBar*	makeStandardScrollBar(Bool horizontal, Box2 const& b, tkPen const& p);
void		resizeStandardSlider(tkValuator* val, Box2 const& b);
void		resizeStandardScrollBar(tkScrollBar* sb, Box2 const& b);

// Create a model which has a tkLabel and a tkRectangle in it.
// the specs are the same as makeCenteredLabel
tkModel* 	makeRoundCenteredLabel(char*, tkPen const&, Box2 const& box);
tkModel* 	makeRectenteredLabel(char*, tkPen const&, float, float);

// Construct a two state button: (use as radio button)
//      lbl - is the text centered on a tkRectangle.
//      val - is the tkValue assigned to state 1.  State 0 is assigned (0).
//	w, h - specify the dimensions (width, height)
tkButton* makePickButton(char* lbl, tkValue const& val, float w, float h);

// Construct a toggle button:
//	Similar to a pickButton, but designed to look either pressed in
//	or popped out -- and to be toggled on/off as a single switch.
//	(not a radio button).
tkButton* makeToggleButton(char *lbl, tkValue const& val, float w, float h);

// Construct a momentary button:
//      lbl - is the text centered on a tkRectangle.
//	w, h - specify the dimensions (width, height)
tkButton* makeMenuButton(char*, float, float);

//  Same as above (menuButton), but styled for auto-accept.
tkButton* makeAcceptButton(char*, float, float);

//  Make a standard tkTextView, complete with fancy border.
tkTextView* makeFancyTextView(char*, int, Box2 const&, tkPen*);

//  Given a ScrollList, wrap it up in a fancy border with a scroll bar
//  in its own BorderPntView.  Now BorderPntView owns the ScrollList.
BorderPntView* makeScrollList(ScrollList*, Box2 const&);
