#pragma once

// $Revision: 1.23 $
// $Date: 1992/05/07 10:33:09 $
#include "textView.h"
#include "tkScrollBar.h"

class textEdit : public tkView {
protected:
	textFrame	tf;
	textView	tv;
	tkEvent*	blinkInterest;
	tkEvent*	dragInterest;
	short		dragX;			// record of drag location
	short		dragY;

	tkScrollBar*	vbar;			// vertical scroll bar
	tkScrollBar*	hbar;			// horizontal scroll bar
	short		markLine;
	short		markColumn;
	short		selectionDirection;

	Colorindex	pointColor;
	short		pointLine;
	short		pointColumn;
	short		pointX;
	short		pointY;
	short		pointHeight;
	short		textLine;		// stash of last click
	short		textColumn;
	short		cursorColumn;

	short		fixedContent;		// fixed length of content
						// for fieldEditor

	unsigned int	pointLocationIsValid:1;
	unsigned int	pointIsVisible:1;
	unsigned int	haveSelection:1;
	unsigned int	useTimer:1;
	unsigned int	fileIsModified:1;
	unsigned int	askForKeybd:1;
	unsigned int	hidePoint:1;
	unsigned int	fieldEditor:1;
	unsigned int	selecting:1;
	unsigned int	implicitCopy:1;
	unsigned int	allowCntlChr:1;
	unsigned int	hScrollOkay:1;
	unsigned int	scrollDuringSelectionOkay:1;

	// undo buffer handling
	enum undoOpType {
	    tkTextEdit_insert,			// text inserted
	    tkTextEdit_delete,			// text deleted
	    tkTextEdit_replace,			// selection replaced
	};
	undoOpType	undoOp;
	short		undoLine;
	short		undoColumn;
	char*		undoBuf;
	long		undoBufLen;		// count of data in buffer
	long		undoBufSize;		// size of buffer

	long		menu;
	long		menuEventName;

	// compute the x,y pixel locations of the point for imaging it
	void validatePointLocation();

	// process keyboard data
	void handleKeyboardData(unsigned char ascii);
	void handleSpecialKeyboardData(unsigned char ascii);

	// manage editing
	void copyToUndoBuffer(int fromLine, int fromColumn,
			      int toLine, int toColumn);
	int fixColumn(int l, int c);
	int getLineColumn(Point& p, int* line, int* column);
	void adjustBufferSize(char** buf, long* size, long newSize);
	void cutCurrentSelection(Bool copyToCutBuffer);

	// manage user selection
	void changeSelection(Bool erase, int l1, int c1, int l2, int c2);
	void select(int l1, int c1, int l2, int c2)
	    { changeSelection(FALSE, l1, c1, l2, c2); }
	void deselect(int l1, int c1, int l2, int c2)
	    { changeSelection(TRUE, l1, c1, l2, c2); }
	void extendSelection(int l, int c, Bool updateDisplay);
	void hideSelection();
	void revealSelection();

public:
	textEdit();
	~textEdit();

	// programatic access to editing functions
	void undo();
	void cut();
	void copy();
	void paste();

	// initialize object; connect up wiring to other objects
	void init();
	void noKeybd() { askForKeybd = 0; }
	void noBlink() { useTimer = 0; }

	void noPoint();
	void showPoint();

	textFrame* getTextFrame() { return &tf; }
	textView* getTextView() { return &tv; }

	// set/get the scroll bar
	void setHScrollBar(tkScrollBar* s) { hbar = s; hbar->setClient(this); }
	tkScrollBar* getHScrollBar() { return hbar; }
	void setVScrollBar(tkScrollBar* s) { vbar = s; vbar->setClient(this); }
	tkScrollBar* getVScrollBar() { return vbar; }

	// set/get the point color
	void setPointColor(Colorindex c) { pointColor = c; }
	Colorindex getPointColor() { return pointColor; }

	// put some ascii data into the frame
	void putAscii(char* buf, int len);

	// read a file into the editor at the given line & column
	void readFile(FILE* in, int line, int column);

	// write the buffer out to a file
	void writeFile(FILE* out);

	// inform editor that its size has changed so that it can
	// fix up the scrollbar
	void sizeChanged();

	void setText(char* text);
	void setMenu(long menuTag, long men)
	    { menu = menuTag; menuEventName = men; }
	int modified() { return fileIsModified; }
	void unmodify() { fileIsModified = 0; }
	void setFieldEditor() { fieldEditor = 1; }
	void clearFieldEditor() { fieldEditor = 0; }
	void setFixedContent(int fc) { fixedContent = fc; }
	int getFixedContent() { return fixedContent; }
	void allowControlCharacters(int acc) { allowCntlChr = acc; }

	void setPoint(int l, int c);
	void getPoint(int* l, int* c);
	void setMark(int l, int c);
	void getMark(int* l, int* c);
	void loseSelection();
	void makeSelection();
	void checkForScroll(Bool forceScrolled);

	// mark contents as entirely selected
	void selectAll();

	virtual void beginSelect( Point& p );
	virtual void continueSelect( Point& p );
	virtual void endSelect( Point& p );
	virtual void enterSelect( Point& p );
	virtual void exitSelect( Point& p );

	virtual void doubleClick( Point& p );
	virtual void menuSelect( Point& p );

	virtual	void keyboardEvent( tkKeyboardEvent* );
	virtual	void gainKeyboardFocus();
	virtual	void loseKeyboardFocus();

	virtual void setBounds(Box2 const&);
	virtual void paint();
	virtual void redrawInContext();
	virtual void rcvEvent(tkEvent*);
};
