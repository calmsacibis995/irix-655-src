#ifndef	_tkValuator_
#define	_tkValuator_

// $Revision: 1.10 $
// $Date: 1990/07/18 14:49:20 $
#ifndef	_tkView_
#include "tkView.h"
#endif
#ifndef	_tkModelItem_
#include "tkModelItem.h"
#endif

// Valuator class.  tkValuator's manage a continuous value for
// a client.  Whenever the value is changed, the client is informed.
// The client sets a lower bound and an upper bound for the valuator to
// operate within.
class tkValuator : public tkView {
protected:
	//  I am making models friends to get at the scaling info. -- jice
	friend class tkDivotModel;
	friend class tkThumbModel;

	float lowerBound;
	float upperBound;
	float currentValue;
	int thumbOrigin;
	int thumbOffset;
	int thumbExtent;
	int divotOrigin;
	tkObject* client;
	tkModelItem* thumbQuiet;
	tkModelItem* thumbDrag;
	tkModelItem* thumbErase;
	tkModelItem* thumbLocate;
	tkModelItem* divot;
	tkModelItem* bg;
	tkModelItem* image;
	float percentageShown;
	unsigned int dragging:1;
	unsigned int updateBackground:1;
	unsigned int disabled:1;

	// send value change event to client
	virtual void valueChanged(tkValue* v);

	// compute a new value
	virtual float computeNewValue(long ivalue, long irange);

	// Return TRUE if the valuator is a horizontal valuator, FALSE if
	// it is a vertical valuator.  XXX this is lazy
	Bool horizontal() { return region.xlen > region.ylen; }

	// Reposition the thumb in the valuator
	void moveThumb();

	// Update thumb's painted position
	void eraseThumb();
	void drawThumb();

public:
	tkValuator();
	~tkValuator();

	// set/get bounds info
	void setLowerBound(float d);
	float getLowerBound() { return lowerBound; }
	void setUpperBound(float d);
	float getUpperBound() { return upperBound; }

	// set/get current value
	void changeCurrentValue(float d, Bool updateClientAndDisplay,
			        Bool forceNewValue = 0);
	void setCurrentValue(float d) { changeCurrentValue(d, FALSE); }
	void setPercentage(float d);
	float getCurrentValue() { return currentValue; }
	float getPercentage();

	// set/get the percentage of view shown
	void setPercentageShown(float ps);
	void setPercentageShown(float viewRange, float contentRange);
	float getPercentageShown() { return percentageShown; }

	// set/get client information
	void setClient(tkObject* c) { client = c; }
	tkObject* getClient() { return client; }

	// set/get the thumb/bg models
	void setThumbModels(tkModelItem* quiet, tkModelItem* drag,
			    tkModelItem* erase, tkModelItem* locate);
	void setDivot(tkModelItem* d) { divot = d; }
	void setBackgroundModel(tkModelItem* bgm) { bg = bgm; }
	tkModelItem* getBackgroundModel() { return bg; }

	// enable/disable the valuator
	void enable();
	void disable();
	Bool isEnabled() { return !disabled; }

	virtual void locate( Point& );
	virtual void moveLocate( Point& );
	virtual void delocate( Point& );

	virtual void beginSelect( Point& p );
	virtual void continueSelect( Point& p );
	virtual void endSelect( Point& p );

	virtual void paint();
};
#endif
