#ifndef	_tkObject_
#define	_tkObject_

// $Revision: 1.5 $
// $Date: 1990/07/18 14:43:39 $
#ifndef	_NFILE
#include <stdio.h>
#endif
#include "tkTypes.h"

class tkEvent;

// Root class of toolkit. tkObject defines a common interface to all objects
class tkObject {
public:
	tkObject();
	~tkObject();
	/* return the name of the given object's class */
	virtual const char *className();

	/* generate an error about sub class responsibility */
	void subClassResponsibility(const char *parent, const char *method);

	virtual void rcvEvent( tkEvent* );
};
#endif
