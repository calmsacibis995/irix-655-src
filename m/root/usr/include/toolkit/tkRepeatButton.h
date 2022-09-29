#ifndef	_tkRepeatButton_
#define	_tkRepeatButton_

// $Revision: 1.6 $
// $Date: 1990/07/18 14:46:54 $
#ifndef	_tkButton_
#include "tkButton.h"
#endif
#ifndef	_tkTimer_
#include "tkTimer.h"
#endif

#define	tkREPEAT_DURATION	0.05		// need better place for this

// Derive an autorepeat button class.  After transition, wait 
// Double-click time, then start triggering them at the autorepeat rate.
class tkRepeatButton : public tkButton {
protected:
	tkEvent	*timerInterest;			// event for timer to trigger
	float duration;
	unsigned short repeatEnabled:1;

public:
	// Create a default button.  Has 2 states, latches when selected.
	tkRepeatButton(int states = 2);
	~tkRepeatButton();

	// enable/disable repeating, and set repeat duration
	void enableRepeat() { repeatEnabled = 1; }
	void disableRepeat() { repeatEnabled = 0; }
	void setRepeatTime( float d ) { duration = d; }

	virtual void beginSelect( Point& p );
	virtual void endSelect( Point& p );

	virtual void rcvEvent( tkEvent *e );

	virtual const char* className();
};
#endif
