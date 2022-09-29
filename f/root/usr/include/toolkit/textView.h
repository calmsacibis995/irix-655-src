#ifndef	_textView_
#define	_textView_

// $Revision: 1.8 $
// $Date: 1990/07/18 14:37:04 $
#ifndef	_textFrame_
#include "textFrame.h"
#endif

// This value is returned by both findLineColumn and findXY when the
// given coordinates are fully in bound
#define	TV_IN_BOUNDS		0x00	// coordinates are fully in frame

// These values can be returned by findXY
#define	TV_COLUMN_OUTSIDE	0x0C	// column falls out of line
#define	TV_COLUMN_TOO_LARGE	0x08	// column is past end of line
#define	TV_COLUMN_TOO_SMALL	0x04	// column is before start of line
#define	TV_LINE_OUTSIDE		0x03	// line falls out of frame
#define	TV_LINE_TOO_LARGE	0x02	// line is past end of frame
#define	TV_LINE_TOO_SMALL	0x01	// line is before start of frame

// These values can be returned by findLineColumn
#define	TV_X_OUTSIDE		0xC0	// x pixel falls outside of line
#define	TV_X_TOO_LARGE		0x80	// x pixel is past end of line
#define	TV_X_TOO_SMALL		0x40	// x pixel is before start of line
#define	TV_Y_OUTSIDE		0x30	// y pixel falls out of frame
#define	TV_Y_TOO_LARGE		0x20	// y pixel is past end of frame
#define	TV_Y_TOO_SMALL		0x10	// y pixel is before start of frame

class textView {
protected:
	textFrame* tf;
	int* stops;
	short backgroundColor;
	textStyle selectionStyle;	// style info for selection's
	union {
	    short tabStops;
	    short tabWidth;
	};
	long topPixel;			// absolute (frame coord) top pixel
	short topLine;			// absolute frame line #
	short xOffset;			// x offset for imaging lines
	short height;
	short width;

	unsigned short fixedTabs:1;
	unsigned short messedUp:1;
	unsigned short scrolled:1;
	unsigned short extendSelection:1;
	unsigned short topPixelOffset:12;	// pixel offset into top line

	int next(int x, int sw);
	void paintSpan(textSpan* s, int& x, int ybase, int ydesc, int yh);
	void updateSpanCache(textSpan* s, int x);
	void updateLineCache(textLine* l);
	void updateFrameCache();

public:
	textView(textFrame* tfp);
	~textView();

	void setSize(int x, int y) { width = x; height = y; }
	void getSize(int* x, int* y) { *x = width; *y = height; }
	int getWidth() { return width; }
	int getHeight() { return height; }
	void setXOffset(int x) { xOffset = x; }
	int getXOffset() { return xOffset; }
	void setTabSpacing(int pixels);
	void setTabStops(int tabStops, int* stops);

	// set/get the top imaging position by pixel/percentage/line
	void setTopPixel(int topPixel);
	void setTopPosition(float pct);
	void setTopLine(int topLine);
	int getTopPixel();
	float getTopPosition();
	int getTopLine() { return topLine; }

	// clear/get the view dirty bit
	void clean() { messedUp = 0; }
	void dirty() { messedUp = 1; }
	int isDirty() { return messedUp; }

	void setBackgroundColor(int bgc) { backgroundColor = bgc; }
	int getBackgroundColor() { return backgroundColor; }
	void setSelectionStyle(textStyle& s) { selectionStyle = s; }
	void setSelectionStyle(textStyle* s) { selectionStyle = *s; }
	textStyle& getSelectionStyle() { return selectionStyle; }

	void validate() { if (tf->isDirty()) updateFrameCache(); }
	void setScrolled() { scrolled = 1; }
	void clearScrolled() { scrolled = 0; }
	int isScrolled() { return scrolled; }

	void setExtendSelection() { extendSelection = 1; }
	void clearExtendSelection() { extendSelection = 0; }

	void paint();

	// Return an x,y cartesian coordinate for the given line&column.
	// The x,y coordinates are frame relative, with y==0 being at the
	// top of the frame, and y increasing for each line in the frame.
	// x==0 means the left most column of any line in the frame.
	int findXY(int line, int column, int* x, int* y);

	// Return a line,column coordinate for the given x&y.
	// The x,y coordinates are frame relative, with y==0 being at the
	// top of the frame, and y increasing for each line in the frame.
	// x==0 means the left most column of any line in the frame.
	int findLineColumn(int x, int y, int* line, int* column);

	// Return the visual height of the given line in the view.
	// If the line doesn't exist, return -1.
	int lineHeight(int line);

	// Return the visual width of the given line in the view.
	// If the line doesn't exist, return -1.
	int lineWidth(int line);

	// Convert a viewer Y coordinate (cartesian coordinate) into a frame
	// coordinate.
	int viewYToFrameY(int viewY);

	// Convert a frame Y coordinate into a viewer coordinate
	int frameYToViewY(int frameY);
};
#endif
