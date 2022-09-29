#pragma once

// $Revision: 1.38 $
// $Date: 1990/08/09 14:39:08 $
#include "tkEvent.h"
#include "tkView.h"
#include "tkPen.h"
#include "stddefs.h"
#include "tkBorderModel.h"

class textEdit;
class textFont;
class tkRegExp;

class tkTextView : public tkView {
protected :
	textEdit*	editor;

	tkPen*		pen;
	char*		textBuf;
	long		textBufLen;
	tkRegExp*	regExp; 
	textFont*	font;
	char*		translationTable;
	tkValueEvent*	menuInterest;
	long		menu;
	long		menuEventName;
	int		errorColor;
	int		selectColor;
	tkBorderModel*	borderModel;

	unsigned int	userIsAllowedToType:1,
			matchRE:1,
			autoCenter:1,
			doTranslation:1,
			useMenu:1,
			errorHighLight:1,
			unsetErrorOnModify:1,
			:1,
			terminator:8,
			maxlen:16;

	void fixMaps();
	void sendValue(Bool gotterminator);
	void updateHighLight();
	tkObject* myClient;

public :
	// note: set select and error color to an ICOLOR if necessary.
	tkTextView(char* initial, int lengthLimit, Box2 const& bbox, tkPen* pen,
			 int selColor = TVIEW_SEL_COLOR,
			 int errorHighLightColor = ERROR_COLOR);
	~tkTextView();

	virtual void setBounds( Box2 const& );

	void setContentLimit(int cl) { maxlen = cl; }
	int getContentLimit() { return maxlen; }

	void init();			// put it together and wire it up
	void setTerminator( char c ) { terminator = c; }
	void setFont(char* name, float size);
	void selectAll();

	void setBorderModel(tkBorderModel* m) { borderModel = m; }

	void setAutoCentering();
	void clearAutoCentering();

	// These are only here for backwards compatability -- do not use.
	void setFancyBorder();
	void clearFancyBorder();

	// disable the point.  caller must call paintInContext if
	// the display needs to show this.
	void hidePoint();

	// enable the point.  caller must call paintInContext if
	// the display needs to show this.
	void showPoint();

	// Manage error highlighting.  Modifies text background to use the
	// errorHighLightColor (on) or pen background color (off).  Does
	// not update the display (use paintInContext).
	void errorOn();
	void errorOff();
	void setRemoveErrorHighLightOnModify();
	void clearRemoveErrorHighLightOnModify();

	void allowControlCharacters(int f);

	void setText(char* newText);
	char *gettext();

	int setRegExp(char *re);
	char *getRegExp();

	// array of 256 characters that is used for key mapping
	void setTranslationTable(char* table);

	// disable translation of characters
	void disableTranslation();

	// enable translation, assuming a setTranslationTable has already
	// been done.  Quick way to re-enable translation, without the
	// overhead of setTranslationTable.
	void enableTranslation();

	// enable edit menu (this is the default)
	void enableMenu();

	// disable the edit menu
	void disableMenu();

	int getWidth();
	void setPen(tkPen* p) { pen = p; }
	tkPen* getPen() { return pen; }
	void setClient( tkObject* );

	virtual void locate( Point& p );
	virtual void delocate( Point& p );
	virtual void beginSelect( Point& p );
	virtual void endSelect( Point& p );
	virtual void exitSelect( Point& p );
	virtual void enterSelect( Point& p );
	virtual void continueSelect( Point& p );
	virtual void menuSelect( Point& p );
	virtual void doubleClick( Point& p );

	virtual	void keyboardEvent( tkKeyboardEvent* );
	virtual	void gainKeyboardFocus();
	virtual	void loseKeyboardFocus();

	virtual tkView* inside( Point const& p );

	virtual void paint();
	virtual void rcvEvent(tkEvent* e);
	virtual const char* className();
};
