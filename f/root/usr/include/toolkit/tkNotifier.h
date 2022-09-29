#pragma once

// $Revision: 1.20 $
// $Date: 1990/08/09 14:36:58 $
#include "tkApp.h"
#include "tkWindow.h"
#include "tkModel.h"
#include "tkViewCltn.h"
#include "tkExec.h"
#include "tkEvent.h"
#include "events.h"
#include "stddefs.h"

#define NOTIFY_BTN1	1
#define NOTIFY_BTN2	2
#define NOTIFY_BTN3	3

class tkNotifier : public tkWindow {
protected:
	int		num_btns;
	float		width;
	float		height;
	tkModel*	bg;
	tkViewCltn	btns;
	int		shutButton;
	int		defaultButton;
	Bool		closeonpost;
	Bool		prefposFlag;
	Bool		usingInForeground;

	tkValueEvent*	notify;
	tkValueEvent*	done;
	tkWindowEvent*	wquit;

public:
	tkNotifier();
	tkNotifier(int cnt, char *lbls[]);

	virtual void open();
	virtual void close();
	virtual void shutdown();
	virtual void cleanNotifier();
	virtual void rcvEvent( tkEvent *);

	void initWindow(int, char *[]);
	virtual void postNotification( int );
	
	virtual void setWindowposition(float, float, float, float);
	virtual void setWindowSize(float, float);

	void setButtonCount( int );
	virtual void setButtonatts(int, char *, tkValue const&, 
		float mw = MENU_BTN_WD, float mh = MENU_BTN_HT, Bool = FALSE);
	virtual void setButton(int, char *, tkValue const& );

	//  call before open.
	void setForeground() { usingInForeground = TRUE; foreground(); }

	// set which button to pretend was pressed when the user bops
	// the window shut button
	void setShutButton(int);

	void setDefaultButton(int);

	// Specify if you want the window to go away when posting reply
	void closeOnpost(Bool);
};
