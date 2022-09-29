/*ident	"@(#)ctrans:incl-master/proto-headers/rand48.h	1.3" */

#ifndef __RAND48_H
#define __RAND48_H

extern "C" {

double drand48(void);
double erand48(unsigned short [3]);
long jrand48(unsigned short [3]);
long lrand48(void);
long mrand48(void);
long nrand48(unsigned short [3]);
void srand48(long);
void lcong48(unsigned short [7]);
unsigned short *seed48(unsigned short [3]);

}

#endif

