#ifndef __LIBEXC_H__
#define __LIBEXC_H__

#include <sgidefs.h>

/* User-level stack unwinding functionality */
/* API prototypes */

#ifdef __cplusplus
extern "C" {
#endif

int trace_back_stack(int, __uint64_t *, char **, int, int);
int trace_back_stack_(int *, __uint64_t *, char **, int *, int *);
int trace_back_stack_and_print(void);
int trace_back_stack_and_print_(void);
void exc_dladdr(__uint64_t, char *, int, __uint64_t *);
void exc_dladdr_(__uint64_t *, char *, int *, __uint64_t *);

#ifdef __cplusplus
}
#endif

#endif
