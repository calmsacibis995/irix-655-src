#ifndef	_tkEventManager_
#define	_tkEventManager_

// $Revision: 1.9 $
// $Date: 1990/07/18 14:39:44 $
#ifndef	_tkEvent_
#include "tkEvent.h"
#endif

class tkInterestBucket;

// Event manager object.  Receives events and dispatches them using the
// tkEvent's discrimination functions.
class tkEventManager : public tkObject {
protected:
	tkEvent*	eventListHead;
	tkEvent*	eventListTail;

	void processEvent(tkEvent* e);

public:
	tkEventManager();
	~tkEventManager();

	// Give an event to the event manager.  If immediateDispatch is
	// true, the event is handed off directly.  Otherwise, the
	// event is queued
	void postEvent(tkEvent*, Bool immediateDispatch);

	// Return TRUE if any events are pending processing
	Bool eventsArePending();

	// Express interest in a particular event
	void expressInterest(tkEvent*);
	void loseInterest(tkEvent*);

	// Process all pending events on the event queue
	void process();
};
#endif
