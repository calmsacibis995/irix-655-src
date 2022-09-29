#pragma once

// $Revision: 1.14 $
// $Date: 1992/12/11 21:13:25 $
#include "tkEvent.h"
#include "tkVector.h"
#include "tkWindow.h"

// This is a small class to bundle event templates with windows.
class appWindow {
public :
    tkWindow		*w;
    tkWindowEvent	*quitEvent;
};

Vector(appWindowCltn,appWindow*);

// Generic application, that understands basic actions,
// like open, close...
class tkApp : public tkObject {
protected:
    appWindowCltn	wincltn;
    tkApp		*parentApp;
    tkWindow		*currentHog;		// any window currently hogging
    virtual void zot(tkWindow*, Bool freeit);
    virtual void sendClose();

public:
    tkApp();
    ~tkApp();

    virtual void appInit( tkApp*, int, char*[], char*[] ); // argc, argv, envp
    virtual void appAlive( tkApp* );
    virtual void appClose();
    virtual void rcvEvent( tkEvent* );
    virtual void addWindow( tkWindow* );
    virtual void inputHog( tkWindow* );
    void removeWindow( tkWindow* w) { zot(w, FALSE); }
    void destroyWindow( tkWindow* w) { zot(w, TRUE); }

    tkWindow* overWin( Point2 const&, tkGID );

    virtual const char* className();

    friend void tkAddApp( tkApp* );
    friend void	tkRemoveApp( tkApp* );

    float DesiredPositionX;	// This is the position the parent desires
    float DesiredPositionY;	// this app to come up at.
    void setDesiredPosition(float x, float y)
		{ DesiredPositionX = x, DesiredPositionY = y; }
};
