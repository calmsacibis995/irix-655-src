#ifndef	_textSpan_
#define	_textSpan_

// $Revision: 1.5 $
// $Date: 1990/07/18 14:36:48 $
#ifndef	_linkList_
#include "linkList.h"
#endif
#ifndef	_textStyle_
#include "textStyle.h"
#endif

class textLine;
class textView;

class textSpan : public linkItem {
protected:
	friend class textLine;

	textStyle style;		// style of text in this span
	unsigned int len:16,		// number of chars in span
		     space:15,		// space reserverd for chars
		     messedUp:1;	// viewer attributes are wrong

	void clip(int& off, int& amount);
public:
	unsigned short hasTab:1,	// a \t is in the text
		       painted:1,	// its on the display
		       width:14;	// width, in pixels, of image
protected:
	char cdata[1];			// pointer to characters

	friend textSpan* getNewSpan(int);
	textSpan(textStyle* s, int spaceNeeded);

public:
	textSpan(textStyle* s, char* buf, int buflen);
	~textSpan();
	void dump();

	// these return the number of characters actually affected
	int cut(int offset, int len);
	int copy(int offset, int len, char* buf, int buflen);

	// list is the linkList that this span is on
	textSpan* paste(linkList* list, int offset, char* buf, int buflen);
	textSpan* split(linkList* list, int offset, textStyle* newStyle);
	textSpan* join(linkList* list);

	int length() { return len; }

	int isDirty() { return messedUp; }
	void dirty() { messedUp = 1; }
	void clean() { messedUp = 0; }

	textSpan* next() { return (textSpan*) linkItem::next; }
	textSpan* prev() { return (textSpan*) linkItem::prev; }
	char* data() { return cdata; }

	textStyle& getStyle() { return style; }
	void setStyle(textStyle& s) { style = s; }

	textFont* getFont() { return style.font(); }
};

#endif
