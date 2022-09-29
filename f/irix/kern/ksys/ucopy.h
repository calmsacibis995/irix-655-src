#if !defined(_KSYS_UCOPY_H)
#define _KSYS_UCOPY_H

/*
 * Prototypes for cell-safe kernel-to-kernel copy functions
 */

extern int kcopyin(caddr_t, caddr_t, size_t);
extern int kcopyout(caddr_t, caddr_t, size_t);

#endif	/* _KSYS_UCOPY_H */
