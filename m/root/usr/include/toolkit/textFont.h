#ifndef	_textFont_
#define	_textFont_

// $Revision: 1.11 $
// $Date: 1992/10/30 09:25:38 $
#include "fmclient.h"
#include "gl/bitmap.h"

class textFont {
protected:
	fmfonthandle font;
	fmfontinfo fi;
	int references;
	short* widths;
	Bitmap* bitmaps;

public:
	textFont();
	~textFont();

	// make the font imaging (via imageSpan) very fast
	void speedy();

	int setFont(char* name);
	int setFont(char* name, float size);

	textFont* dup() { references++; return this; }

	fmfonthandle getFontHandle() { return font; }

	// image len characters, returning the width, in pixels, of
	// the imaged data.  The imaging is done relative to x and
	// y, where y is assumed to be the baseline.
	int imageSpan(char* data, int len, int x, int y);

	Bitmap* getBitmaps() { return bitmaps; }
	int getNumberOfGlyphs() { return fi.nglyphs; }
	int getWidth(char* buf);
	int getWidth(char* buf, int buflen);
	int getWidth(char c) { return widths[c]; }

	// return the inked height of the character
	int getHeight(char c);

	// return the ascent of the character
	int getAscent(char c);

	// return the descent of the character
	int getDescent(char c);

	int getMaxHeight() { return fi.height; }
	int getMaxWidth() { return fi.width; }
	int getMaxDescender() { return fi.yorig; }
};

// get pointer to the toolkit default font
extern textFont*	tkGetDefaultFont();

// change the default font
extern void		tkSetDefaultFont(char* name, float size);
extern void		tkSetDefaultFont(char* name);

// get pointer to a textFont for a particular font
extern textFont*	tkGetFont(char* name, float size);
extern textFont*	tkGetFont(char* name);
#endif
