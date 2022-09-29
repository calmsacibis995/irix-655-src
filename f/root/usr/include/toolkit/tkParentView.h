#ifndef	_tkParentView_
#define	_tkParentView_

// $Revision: 1.11 $
// $Date: 1990/08/23 17:51:55 $
#ifndef	_tkViewCltn_
#include "tkViewCltn.h"
#endif
#ifndef	_tkModel_
#include "tkModel.h"
#endif

// tkParentView is a container for other views.  It provides a local
// coordinate space for its kids, as well as a background model for imaging.
// Finally, it offers two choices for locate/select handling.  If the
// tkParentView has an area, that is, its region.getExtentX() &&
// region.getExtentY() are non-zero, then it will clip locate/select messages
// before passing them to its kids.  If either of the extents are zero, then
// no clipping is done.  Clipping is used for containers that want to force
// the kids to be fully contained.
class tkParentView : public tkView {
protected:
	tkModelItem*	bg;
	tkView*		lastTarget;
	Bool		selected;
	Bool		insideLastTarget;


	// add/remove a view.  for derived class use only
	virtual void addAView(tkView&);
	virtual void removeAView(tkView&);


public:
	tkViewCltn	vc;   // CCC changed from private to public due to WS
	tkParentView();
	tkParentView(FILE* strm);
	~tkParentView();

	/*XXX cheat, for now */
	tkView*& operator[](int i)
	    { return vc[i]; }

	// like inside, but for kids only
	virtual tkView* insideKids(Point const& p);

	// set/get the background model
	void setBackgroundModel(tkModelItem* nbg) { bg = nbg; }
	void setBackgroundModel(tkModelItem& nbg) { bg = &nbg; }
	tkModelItem* getBackgroundModel() { return bg; }

	virtual int size(); 

	virtual tkView* inside( Point const& );

	// contstrain these events to fall within our area, or our kids area
	// if we have no area.
	virtual void locate( Point& p );
	virtual void moveLocate( Point& p );
	virtual void delocate( Point& p );
	virtual void beginSelect( Point& p );
	virtual void endSelect( Point& p );
	virtual void continueSelect( Point& p );
	virtual void enterSelect( Point& p );
	virtual void exitSelect( Point& p );
	virtual void doubleClick( Point& p );
	virtual void menuSelect( Point& p );

	// actions
	virtual void open();				// initialize
	virtual void close();				// put away for later

	virtual void paint();
	virtual void rcvEvent(tkEvent*);

	virtual const char* className();
};
#endif
