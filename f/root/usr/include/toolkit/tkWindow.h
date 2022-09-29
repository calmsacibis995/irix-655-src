#pragma once

// $Revision: 1.38 $
// $Date: 1992/12/11 21:14:26 $
#include "Box2.h"
#include "tkParentView.h"
#include "tkPen.h"
#include "X11/cursorfont.h"

class tkApp;

// tkWindow is a kind of view that implements an interface between
// the windowing system (IWS) and the toolkit. It provides a root
// for clipping, and a focus for window system events.
class tkWindow : public tkParentView {
protected:
	tkGID		gid;			/* gl window id */
	tkGID		ov_gid;			/* gl overlay window id */
	Bool		hasOverlay;
	tkCONTEXT	cx;
	Bool		doubleBuffered;
	int		opaque;
	char*		title;
	char*		name;
	long		flags;
	Box2		screenArea;
	Point2		prefSize;
	Point2		prefPos;
	Point2		minSize;
	Point2		maxSize;
	Point2		fudgeFactor;
	Point2		stepUnit;
	Point2		aspect;
	Point2		mySize;
	Point2		myOrigin;

	// event interests
	tkWindowEvent*	redrawEvent;
	tkWindowEvent*  redrawChangeEvent;
	tkWindowEvent*	stowEvent;
	tkWindowEvent*	unstowEvent;
	tkWindowEvent*	gainFocus;
	tkWindowEvent*	loseFocus;
	//tkWindowEvent*	quitEvent;
	tkWindowEvent*	shutEvent;
	tkMotionEvent*	motionEvent;
	tkValueEvent*	keybdEvent;
	tkMotionEvent*	fullScreenMotionEvent;
	tkButtonEvent*	selectButton;
	tkButtonEvent*	functionButton;
	tkButtonEvent*	menuButton;
	tkButtonEvent*	dclickEvent;

	tkView*		lastTarget;
	tkApp*		containingApp;
	unsigned int	insideLastTarget : 1;
	unsigned int	winSelFocus : 1;
	unsigned int	redrawOverride : 1;
	unsigned int	dismissalMode : 1;
	unsigned int	haveFocus : 1;
	unsigned int	handleOwnClose : 1;
	unsigned int	startUpNoStow : 1;
	unsigned int	startUpNoClose : 1;
	unsigned int	startUpNoQuit : 1;
	unsigned int	startUpIconic : 1;
	unsigned int	noBorder : 1;
	unsigned int	noResize : 1;
	tkWindow*	youraHog;
	tkView*		keybdFocus;
	tkView*		keyboardOwner;

	// to handle the many sizes of screen we now deal with
	long 	xmaxscreen;
	long 	ymaxscreen;

	// X flags
	long normalFlags;

	//  If this is FALSE, then FREEZE, SHUT, QUIT and THAW are not queued.
	Bool		queueBorderActions;

	void allocateWindow();
	void handleMotion(tkMotionEvent* e);
	void handleSelectButton(tkButtonEvent* e, Bool isDoubleClick);

public:
	tkWindow();
	tkWindow(tkPen const& pen);
	~tkWindow();

	void initWindow();

	void closeDescendents( Bool d = 0 );

	void addView(tkView& v) { addAView(v); }
	void removeView(tkView& v);

	void setApp(tkApp* app) { containingApp = app; }

	virtual void open();
	virtual void close();
	virtual void shutdown();

	void setTitle(char* title);
	void setIconTitle(char* title);
	void setName(char* name);

	/* set constraints */
	void prefsize(float, float);
	void prefsize(Point2& p)
	    { prefsize(p.x, p.y); }

	void prefposition(float, float, float, float);
	void prefposition(Point2& ll, Point2& ur)
	    { prefposition(ll.x, ur.x, ll.y, ur.y); }
	void prefposition(Box2& r)
	    { prefposition(r.xorg, r.xorg + r.xlen - 1,
			   r.yorg, r.yorg + r.ylen - 1);
	    }
	void prefpositionOnScreen(float, float, float, float);

	void doublebuffer();

	void minsize(float, float);
	void minsize(Point2& p)
	    { minsize(p.x, p.y); }

	void maxsize(float, float);
	void maxsize(Point2& p)
	    { maxsize(p.x, p.y); }

	void getsize( float* , float* );
	void getsize(Point2& p)
	    { getsize(&(p.x), &(p.y)); }

	void getorigin( float* , float* );
	void getorigin(Point2& p)
	    { getorigin(&(p.x), &(p.y)); }

	void fudge(float, float);
	void fudge(Point2& p)
	    { fudge(p.x, p.y); }

	void stepunit(float, float);
	void stepunit(Point2& p)
	    { stepunit(p.x, p.y); }

	void keepaspect(float, float);
	void keepaspect(Point2& p)
	    { keepaspect(p.x, p.y); }

	// window control
	void winpop();
	void winpush();
	void winPosition(int, int, int, int);
	void winMove(long x, long y);
	void setWinPosition(float xorg, float yorg, float xlen, float ylen);
	void setRootWindowColormap();
	void BlockUntilExposeEvent(Display *, Window , Bool);
	Cursor defineFontCursor(int);
	void setCursor(Cursor);

	void setHog( tkWindow *w ) { youraHog = w; }
	void setKeyboardOwner( tkView* );
	void setDismissalMode();
	void clearDismissalMode();
	void getKeyboardFocus( tkView* );
	virtual tkWindow* getParentWindow();
	tkView* getFocusView();

	void setopaque();
	void resetopaque();
	void setcolor( Colorindex );
	void clearWindow();
	virtual void redrawWindow();
	virtual void stowWindow();
	virtual void unstowWindow();
	virtual void updateScreenArea();

	void setRedrawOverride() { redrawOverride = 1; }
	void setHandleClose( Bool val )	 { handleOwnClose = val; }

	void fullScreenListen( Bool );

	Colorindex getcolor();

	virtual void paint();
	virtual void rcvEvent(tkEvent* e);
	virtual tkGID getgid();
	virtual const char* className();
	tkCONTEXT getContext()			{ return cx; }
	void setStartUpNoStow(Bool f)		{ startUpNoStow = f; }
	void setStartUpNoClose(Bool f)		{ startUpNoClose = f; }
	void setStartUpNoQuit(Bool f)		{ startUpNoQuit = f; }
	void setStartUpIconic(Bool f)		{ startUpIconic = f; }
	void setNoBorder(Bool f)		{ noBorder = f; }
	void setNoResize(Bool f)		{ noResize = f; }
	void clearOverlay();
	void setOverlayTrue()			{ hasOverlay = TRUE; }

	static void getCurrentMousePosition(float* xpos, float* ypos);
	static int getButton(int glDev);

	static tkCONTEXT pushContext(tkCONTEXT newCX);
	static void popContext(tkCONTEXT oldCX);
	Bool IsDoublebuffered() { return doubleBuffered; }
	tkGID getOverlayGID();
};
