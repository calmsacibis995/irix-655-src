#pragma once

// $Revision: 1.13 $
// $Date: 1992/12/11 21:13:45 $
#include "tkApp.h"
#include "tkVector.h"
#include "tkEventManager.h"
#include "sys/param.h"

#ifndef NOFILES_MAX
#define NOFILES_MAX 200
#endif

class tkWindow;
class tkEvent;

struct timeval;
typedef	void	(*tkTranslator)();

// A small class to hold tkApps and their CLOSED events.

class ExecApp {
public:
    tkApp*	app;
    tkEvent*	closed_event;
};

Vector( AppCltn, ExecApp* );

class tkExec : public tkObject {

/* tkExec
 * Executive for a UNIX process.  Has the event manager as its friend,
 * and controls stuff like the select() loop in execLoop().
 */

protected:
	tkTranslator	xlate[NOFILES_MAX];
	int		minfd;
	int		maxfd;
	fd_set		rd;
	tkEventManager	em;

public:
	tkExec();

	virtual void rcvEvent( tkEvent* );

	virtual const char* className();

	friend void		tkAddTranslator(int fd, tkTranslator func);
	friend void		tkRemoveTranslator(int fd);
	friend tkEventManager*	tkGetEventManager();
	friend void		tkEventLoop();
	friend void		tkEventPoll(timeval *tv = 0);
	friend AppCltn*		ExecAppCltn();

	friend void		tkSendQuitOrClose(Window);
	static Bool		leftShiftIsDown, rightShiftIsDown;
	static Bool		leftCtrlIsDown, rightCtrlIsDown;
	static Bool		leftAltIsDown, rightAltIsDown;
	static Bool		midMouseIsDown;
	static Bool		leftMouseIsDown;
};

/* other exec functions */
extern	void	execEnableGL(Bool = TRUE);
extern	void	execAddWindow(tkWindow*);
extern	void	execRemoveWindow(tkWindow*);
