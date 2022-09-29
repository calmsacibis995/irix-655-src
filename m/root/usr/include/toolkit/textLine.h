#ifndef	_textLine_
#define	_textLine_

// $Revision: 1.4 $
// $Date: 1990/07/18 14:36:35 $
#ifndef	_textSpan_
#include "textSpan.h"
#endif
#ifndef	_NFILE
#include <stdio.h>
#endif

// maximum length of a textLine
const int textLineMaxLength = 0x7fffffff;

class textLine : public linkItem {
protected:
	linkList spans;
	unsigned int messedUp:1;

	textSpan* findSpan(int& column);
	void compact();
	void invalidateCache();
	void maskStyle(textStyle*, textStyle*, textStyle*, textStyle*);

public:
	// XXX temporary location for per-viewer cache
	unsigned int painted:1;
	short maxDescender;
	short maxHeight;
	short width;

	// create a new textLine, with an empty span using style
	textLine(textStyle* style);

	~textLine();
	void dump();

	// point-mark operations
	// these calls return the number of characters effected
	int cut(int column, int len);
	int copy(int column, int len, char* buf, int buflen);
	int paste(int column, textStyle* s, char* buf, int buflen);
	int replace(int column, textStyle* s, char* buf, int buflen);
	int change(int column, int len, textStyle* s, textStyle* mask);
	int writeFile(int column, int len, FILE* out);

	// return the character at column, or -1 if none
	int charAt(int column);

	// list is the linkList that this textLine is on
	textLine* split(linkList* list, int column, textStyle* s);
	int join(linkList* list);

	// Return the number of characters in this line
	int length();

	int isDirty() { return messedUp; }
	void dirty() { messedUp = 1; }
	void dirtyAll();
	void clean() { messedUp = 0; }

	textSpan* first() { return (textSpan*) spans.head; }
	textLine* next() { return (textLine*) linkItem::next; }
	textLine* prev() { return (textLine*) linkItem::prev; }
};

#endif
