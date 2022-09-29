#ifndef	_tkModelItem_
#define	_tkModelItem_

// $Revision: 1.7 $
// $Date: 1990/07/18 14:42:55 $
#ifndef	_tkObject_
#include "tkObject.h"
#endif
#ifndef	_Box_
#include "Box2.h"
#endif

class tkModelItem : public tkObject {

/* tkModelItem
 * A TkModelItem has two properties. First, it supports a geometric model,
 * a visible thing. Second, it supports a default way of viewing the model.
 */

protected:

public:
	tkModelItem();
	~tkModelItem();
	virtual void draw();
	virtual void getBoundingBox(Box2& result);

	virtual const char* className();
};
#endif
