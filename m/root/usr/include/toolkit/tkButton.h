#pragma once

// $Revision: 1.11 $
// $Date: 1990/08/09 14:35:33 $
#include "tkView.h"
#include "tkModelItem.h"
#include "tkValue.h"

// Possible button visual states
#define	tkBUTTON_QUIET		0	// nothing happening
#define	tkBUTTON_DISABLED	1	// button is inactive
#define	tkBUTTON_LOC		2	// locator inside
#define	tkBUTTON_SEL		3	// button has been picked; locator out
#define	tkBUTTON_LOCSEL		4	// button has been picked; locator in
#define	tkBUTTON_VISUAL_STATES	5

// Complete button class.  Implements a multi-state toggle.  Derived classes
// implement variations (like momentary buttons).
class tkButton : public tkView {
protected:
	short		states;			// number of states button has
	short		currentState;		// current state button is in
	tkValue*	values;			// one value per state
	tkModelItem**	models;			// models for each state
	tkObject*	client;
	short		direction;		// how to get to next state
	unsigned char	currentVisualState;	// how button looks right now
	unsigned int	doitOnUp:1;		// notify client on up stroke
	unsigned int	shiftClicks:1;		// do shift clicking

	// pick next state
	virtual void performStateTransition();

	// inform client that value changed.  used by derived classes
	virtual void valueChanged(tkValue* v);

public:
	// Create a default button.  Has 2 states, latches when selected.
	tkButton(int states = 2);
	~tkButton();

	// set/get flag controlling when button performs its action
	void setTransitionOnUp() { doitOnUp = TRUE; }
	void setTransitionOnDown() { doitOnUp = FALSE; }
	Bool getTransitionPoint() { return doitOnUp; }
	void enableShiftClicks() { shiftClicks = TRUE; }
	void disableShiftClicks() { shiftClicks = FALSE; }
	Bool getShiftClicks() { return shiftClicks; }

	// set/get number of states
	void setStates(int newStates);
	int getStates() { return states; }

	// set/get client information
	void setClient(tkObject* c) { client = c; }
	tkObject* getClient() { return client; }

	// set the model(s) for a given state
	void setModel(int state, int vs, tkModelItem* model);
	void setModels(int state, tkModelItem* quiet, tkModelItem* dis,
			   tkModelItem* loc, tkModelItem* sel,
			   tkModelItem* locsel);

	// set/get the value for a particular state number
	void setValue(int state, tkValue const & nv);
	void setValues(tkValue* nv);		// assume one per state
	tkValue& getValue(int state);
	tkValue& getCurrentValue() { return getValue(currentState); }

	// set/get the current state
	void setCurrentState(int newState);
	int getCurrentState() { return currentState; }

	// set/get the current visual state
	// these do not update the image, so paintInContext must be
	// called if that is desired.
	void setCurrentVisualState(int ns)
	    { currentVisualState = ns; }
	int getCurrentVisualState()
	    { return currentVisualState; }

	// enable/disable the button
	void enable();
	void disable();

	tkModelItem* getModel( int i )
            { return models[i]; }

	int elements() { return tkBUTTON_VISUAL_STATES * states; }

	virtual void locate( Point& p );
	virtual void delocate( Point& p );
	virtual void beginSelect( Point& p );
	virtual void endSelect( Point& p );
	virtual void enterSelect( Point& p );
	virtual void exitSelect( Point& p );

	virtual void paint();

	virtual const char* className();
};
