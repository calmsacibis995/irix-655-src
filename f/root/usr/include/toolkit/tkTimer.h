#pragma once

// $Revision: 1.8 $
// $Date: 1990/09/07 13:53:51 $
#include "tkObject.h"
#include "tkExec.h"

class tkTimer : public tkObject {
public:
    tkTimer(Bool autoReload = FALSE);
    virtual ~tkTimer();

    void setTimerDuration(float d)		{ duration = d; }
    float getTimerDuration()			{ return duration; }

    void startTimer();
    void startTimer(float d)
	{ setTimerDuration(d); startTimer(); }

    void stopTimer();

    float getDelta()				{ return delta; }

protected:
    float		duration;		// Duration timer is set to
    float		delta;			// relative time left
    Bool		ticking;		// TRUE if timer is armed
    Bool		reload;			// TRUE if timer should rearm
    tkEvent		*event;
    tkTimer*		next;			// link in active list

    virtual void tick();

    void removeFromActiveList();
    void addToActiveList();
    void setDelta(float d)			{ delta = d; }

    static void alarm(float timeThatElapsed);

    static tkTimer* activeTimers;

    friend void tkEventPoll(struct timeval*);
};

extern tkTimer tkInteractionTimer;
extern tkTimer tkBlinkTimer;
