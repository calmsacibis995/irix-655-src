#pragma once

// $Revision: 1.10 $
// $Date: 1991/04/01 13:15:00 $
#include "tkParentView.h"
#include "tkButton.h"

// A tkRadioButton is a collection of two state tkButton's.  It provides
// an interaction wrapper.  This code will ignore a button that is
// addButton'd that doesn't have exactly two states.  See tkParentView.h
// for controlling the interaction of the contained buttons.  tkRadioButton
// will pass on to its client, the value generated whenever a contained button
// goes to state #1 (not state #0).  Accordingly, users of tkRadioButton
// should tag each of the contained buttons with a different value so that
// they can be distinguished when an event is generated.
class tkRadioButton : public tkParentView {
protected:
    tkObject*		client;
    tkButton*		current;
    tkButton*		old;

    // inform client that value changed.  used by derived classes
    virtual void valueChanged(tkValue* v);

    // switch to a new current button, changing its visual state, and
    // optionally updating the image
    void switchButtons(tkButton* winner, int winnerVisualState, Bool draw);

    // change the current buttons visual state
    void switchVisualState(int visualState);

public:
    tkRadioButton();

    void addButton(tkButton* b);
    void addButton(tkButton& b) { addButton(&b); }
    void removeButton(tkButton& b);

    // set/get the current selected button
    void changeCurrentButton(tkButton* b, Bool updateDisplay)
	{ switchButtons(b, tkBUTTON_QUIET, updateDisplay); }
    void setCurrentButton(tkButton* b)
	{ switchButtons(b, tkBUTTON_QUIET, FALSE); }
    tkButton* getCurrentButton()
	{ return current; }

    // set/get client information
    void setClient(tkObject* c) { client = c; }
    tkObject* getClient() { return client; }

    virtual void setBounds( Box2 const& );
    virtual void beginSelect( Point& p );
    virtual void endSelect( Point& p );
    virtual void continueSelect( Point& p );
    virtual void rcvEvent( tkEvent* );
    virtual const char* className();
};
