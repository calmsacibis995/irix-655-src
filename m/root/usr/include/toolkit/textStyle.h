#ifndef	_textStyle_
#define	_textStyle_

// $Revision: 1.4 $
// $Date: 1990/07/18 14:36:56 $
#ifndef	_textFont_
#include "textFont.h"
#endif

extern int tkColorMap[256];
extern textFont* tkFontMap[256];

// textStyle is an encapsulation of the various style choices for text.
class textStyle {
protected:
    int ExtraSpaceForCompiler; // CCC compiler bombs if not here
	union {
	    struct {
		unsigned int
		    underline:1,	// text should be underlined
		    transparent:1,	// background should be transparent
		    selected:1,		// text should be selected
		    :5,
		    font:8,		// font to use
		    textColor:8,	// text color
		    pageColor:8;	// page color
	    } s;
	    long l;
	};

public:
	textStyle(int fontn = 0, int text = 0, int page = 7, int under = 0, int transp = 0, int select = 0) { // l = 0; }
	    l = 0;
	    s.underline = under;
	    s.transparent = transp;
	    s.selected = select;
	    s.font = fontn;
	    s.textColor = text;
	    s.pageColor = page;
	}
	textStyle(textStyle* other) { l = other->l; }

	int operator==(textStyle& other) { return l == other.l; }
	operator int() { return l; }

	// get style attributes
	int getFont() { return s.font; }
	textFont* font() { return tkFontMap[s.font]; }
	int textColor() { return tkColorMap[s.textColor]; }
	int rawTextColor() { return s.textColor; }
	int pageColor() { return tkColorMap[s.pageColor]; }
	int rawPageColor() { return s.pageColor; }
	int transparent() { return s.transparent; }
	int underline() { return s.underline; }
	int selected() { return s.selected; }

	// set style attributes
	void setFont(int n) { s.font = n; }
	void setTextColor(int n) { s.textColor = n; }
	void setPageColor(int n) { s.pageColor = n; }
	void setTransparent(int n) { s.transparent = n; }
	void setUnderline(int n) { s.underline = n; }
	void setSelected(int n) { s.selected = n; }
};

#endif
