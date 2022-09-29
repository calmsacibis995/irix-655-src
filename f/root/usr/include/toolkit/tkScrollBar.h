#pragma once

// $Revision: 1.9 $
// $Date: 1990/08/09 14:38:49 $
#include "tkValuator.h"
#include "tkRepeatButton.h"
#include "tkParentView.h"

// tkScrollBar is a collection of a valuator and some buttons
class tkScrollBar : public tkParentView {
protected:
	tkValuator*	val;
	tkRepeatButton*	inc;
	tkRepeatButton*	dec;
	tkObject*	client;
	float		increment;
	float		decrement;
	float		pageIncrement;
	float		pageDecrement;

	// send updated value to client
	virtual void valueChanged(tkValue* v);

public:
	tkScrollBar();
	~tkScrollBar();

	virtual void setBounds( Box2 const& );

	// set/get the valuator && button components
	void setValuator(tkValuator* v);
	void setIncButton(tkRepeatButton* b);
	void setDecButton(tkRepeatButton* b);
	tkValuator* getValuator() { return val; }
	tkRepeatButton* getDecButton() { return dec; }
	tkRepeatButton* getIncButton() { return inc; }

	// set/get the increment/decrement values
	void setIncValue(float ni) { increment = ni; }
	void setDecValue(float nd) { decrement = nd; }
	float getIncValue() { return increment; }
	float getDecValue() { return decrement; }

	// set/get the increment/decrement values
	void setPageIncValue(float ni) { pageIncrement = ni; }
	void setPageDecValue(float nd) { pageDecrement = nd; }
	float getPageIncValue() { return pageIncrement; }
	float getPageDecValue() { return pageDecrement; }

	// set/get client information
	void setClient(tkObject* c) { client = c; }
	tkObject* getClient() { return client; }

	// enable/disable the scrollbar
	void enable();
	void disable();
	Bool isEnabled() { return val->isEnabled(); }

	// update display after an enable/disable/setPercentage, etc.
	void updateDisplay();

	virtual void rcvEvent(tkEvent* e);
};
