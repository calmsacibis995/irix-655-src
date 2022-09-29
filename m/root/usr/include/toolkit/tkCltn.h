#ifndef	_tkCltn_
#define	_tkCltn_

// $Revision: 1.5 $
// $Date: 1990/08/23 17:50:45 $
#ifndef	_tkObject_
#include "tkObject.h"
#endif

class tkStream;
class tkCltn : public tkObject {

/* tkCltn
 * Abstract class defining common collection notions.
 */

public:
	virtual Bool isEmpty();
	virtual int size();
	virtual const char* className();
};
#endif
