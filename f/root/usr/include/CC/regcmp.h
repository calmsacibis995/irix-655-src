/*ident	"@(#)ctrans:incl-master/proto-headers/regcmp.h	1.2" */

#ifndef __REGCMP_H
#define __REGCMP_H

extern "C" {

char *logname(void);
char *regcmp(const char *, ...); 
char *regex(const char *, const char *, ...);

}

#endif
