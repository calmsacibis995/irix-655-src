#ident "$Revision: 1.1 $"

#include <stdlib.h>
#include "ref.h"

void
incref (void *p)
{
	struct ref *r = (struct ref *) p;

	if (r != NULL)
		(void) atomic_fetch_and_increment (&r->refcnt);
}

void
decref (void *p)
{
	struct ref *r = (struct ref *) p;

	if (r != NULL && atomic_fetch_and_decrement (&r->refcnt) == 1)
		(*r->delfunc) (p);
}

void
iniref (void *p, void (*delfunc) (void *))
{
	struct ref *r = (struct ref *) p;

	if (r == NULL || delfunc == NULL)
		return;
	r->refcnt = 1;
	r->delfunc = delfunc;
}
