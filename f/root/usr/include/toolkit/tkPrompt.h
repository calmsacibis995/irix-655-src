#pragma once

// $Revision: 1.11 $
// $Date: 1990/08/09 14:37:51 $
#include "tkNotifier.h"
#include "tkTextView.h"

class tkPrompt : public tkNotifier {
protected:
	tkTextView*		data;
	tkTextViewEvent*	actionInterest;
	int			acceptButton;

public:
	tkPrompt();
	tkPrompt(int cnt, char *lbls[]);
	~tkPrompt();

	virtual void open();
	virtual void close();
	virtual void rcvEvent( tkEvent *);

	char *getData();

	tkTextView* getDataField() { return data; }

	void initDataField(char *, int, Box2 const&, tkPen* );
	void allowControlCharacters(int f);
	void setEditText(char*);
	void setEditFont(char* name, float size);
	void selectAll();

	// setup a button to pretend was pressed when the user
	// types carriage return
	void setAcceptButton(int, char*, tkValue const&, float w = MENU_BTN_WD,
				    float h = MENU_BTN_HT);

	virtual void setButtonatts(int, char*, tkValue const&, 
	    float mw = MENU_BTN_WD, float mh = MENU_BTN_HT, Bool accept=FALSE);
};
