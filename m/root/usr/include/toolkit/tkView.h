#pragma once

// $Revision: 1.20 $
// $Date: 1992/10/20 10:29:52 $
#include "tkEvent.h"
#include "Box2.h"
#include "matrix.h"
#include "tkGID.h"

// tkView is the foundation class for interaction behaviour; it
// embodies a single interaction behaviour (SmallTalk View +
// Controller model).  A tkView responds to input events and produces
// visible output to the user.  tkView often uses an associated tkModel to
// produce specific behaviour.
class tkWindow;
class tkView : public tkObject {
protected:
	Box2		region;
	Point3		position;
	tkView*		parent;	// immediate ancestor

public:
	tkView();		// makes mapMatrix = I, clipRegion = parent.

	/* set the new clip region for this view. */
	void setRegion( Box2 const& newRegion );
	virtual void setBounds( Box2 const& );
	Box2& getBounds() { return region; }
	float getOriginX() { return region.xorg; }
	float getOriginY() { return region.yorg; }
	float getExtentX() { return region.xlen; }
	float getExtentY() { return region.ylen; }
	int getiOriginX() { return (int)(region.xorg); }
	int getiOriginY() { return (int)(region.yorg); }
	int getiExtentX() { return (int)(region.xlen); }
	int getiExtentY() { return (int)(region.ylen); }

	tkView* getParent() { return parent; }
	void setParent(tkView* p) { parent = p; }  // CCC

	/*
	 *  Methods for setting the mapping matrix.  This matrix
	 *  maps the contents of this view into its parent.
	 *  The parent can be 2D or 3D, and this view can be 2D or 3D;
	 *  this view can contain any number of 2D or 3D views.
	 */
	void translate( Point const& );
	virtual void localTransform() ;
	virtual void globalTransform();

	/* return view that the given coordinate is inside */
	virtual tkView* inside( Point const& p );

	// return the intersection of all containing bounding boxes,
	// defining the clipping region for this view.  Also return
	// the window relative origin for this view
	virtual void getClipBox(Box2* result, int* xOrigin, int* yOrigin);

	// respond to the locator waving over the view
	virtual void locate( Point& p );		// enter target
	virtual void moveLocate( Point& p );		// wiggle in target
	virtual void delocate( Point& p );		// exit target

	// respond to the selector waving over the view
	virtual void beginSelect( Point& p );		// bug down
	virtual void endSelect( Point& p );		// bug up
	virtual void continueSelect( Point& p );	// wiggle while down
	virtual void enterSelect( Point& p );		// enter target
	virtual void exitSelect( Point& p );		// exit target

	// handle double click or menu selecting
	virtual void doubleClick( Point& p );
	virtual void menuSelect( Point& p );

	// handle keyboard events and focus
	virtual void keyboardEvent( tkKeyboardEvent* );
	virtual void gainKeyboardFocus();
	virtual void loseKeyboardFocus();

	// actions
	virtual void open();				// initialize
	virtual void close();				// put away for later

	// return this tkView's window id
	virtual tkGID getgid();
	virtual void rcvEvent( tkEvent* );

	// return this tkView's parent window pointer
	virtual tkWindow *getParentWindow();


	/* ask the view to paint itself */
	virtual void paint();	        // used internally, assumes xform set
	virtual void paintInContext(); // used externally, sets xform absolutely

	virtual const char* className();
};
