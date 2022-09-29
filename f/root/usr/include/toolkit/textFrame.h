#ifndef	_textFrame_
#define	_textFrame_

// $Revision: 1.7 $
// $Date: 1990/07/18 14:36:21 $
#ifndef	_textLine_
#include "textLine.h"
#endif

class textFrame {
protected:
	linkList lines;			// linked list of textLine's
	textStyle currentStyle;		// current text writing style
	textStyle styleMask;		// mask for style during change
	textLine* point;		// cached line that point is on
	textLine* mark;			// cached line that mark is on
	textLine* last;			// cache of last findLine
	short pointLine;		// line that point is on
	short pointColumn;		// column that point is on
	short markLine;			// line that mark is on
	short markColumn;		// column that mark is on
	short lastLine;			// line that last findLine found

	unsigned short messedUp:1,	// viewer cache is out of date
		       len:15;		// number of lines in frame

	// sort the point and mark
	int sortPointAndMark();
	void checkLast();

public:
	// XXX temporary per viewer cache.  MOVE these to textView
	long height;
	long width;

	textLine* findLine(int line, int add);

	textFrame();
	~textFrame();
	void dump();

	void setPoint(int line, int column);
	void getPoint(int* line, int* col)
	    { *line = pointLine; *col = pointColumn; }
	void setMark(int line, int column);
	void getMark(int* line, int* col)
	    { *line = markLine; *col = markColumn; }

	int cut();
	int copy(char* data, int len);
	int paste(char* data, int len);
	int replace(char* data, int len);
	int change();
	int count();
	int writeFile(FILE* out);

	// return the character at line,col or -1 if none
	int charAt(int line, int col);

	// insure that the given line exists in the frame
	void makeLine(int line);

	void split();
	void join();

	void setStyle(textStyle& s) { currentStyle = s; }
	void setStyle(textStyle* s) { currentStyle = *s; }
	textStyle& getStyle() { return currentStyle; }

	void setStyleMask(textStyle& sm) { styleMask = sm; }
	void setStyleMask(textStyle* sm) { styleMask = *sm; }
	textStyle& getStyleMask() { return styleMask; }

	int length() { return len; }
	int length(int line);

	int isDirty() { return messedUp; }
	void dirty() { messedUp = 1; }
	void dirty(int line);
	void dirtyAll();
	void clean() { messedUp = 0; }

	textLine* first() { return (textLine*) lines.head; }
};

#endif
