#ifndef __OSFCN_H
#define __OSFCN_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#ifdef _SYSTYPE_SVR3
extern "C" {
const char* crypt(const char*, const char*);
void encrypt(char*, int);
int crypt_close(int[]);
char *des_crypt(char *, char *);
void des_encrypt(char *, int);
int run_crypt(long, char *, unsigned int, int *);
}
#else
#include <ulimit.h>
#include <crypt.h>
#endif
#include <sys/wait.h>
#include <sys/lock.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <math.h>
#include <bstring.h>

#endif





