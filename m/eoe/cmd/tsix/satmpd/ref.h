#ifndef REF_HEADER
#define REF_HEADER

#ident "$Revision: 1.1 $"

#include <sys/pmo.h>
#include <fetchop.h>

struct ref {
	atomic_var_t refcnt;
	void (*delfunc) (void *);
};

void incref (void *);
void decref (void *);
void iniref (void *, void (*) (void *));

#endif /* REF_HEADER */
