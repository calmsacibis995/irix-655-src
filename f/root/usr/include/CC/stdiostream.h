/*ident	"@(#)ctrans:incl-master/const-headers/stdiostream.h	1.4" */
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1991 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#ifndef STDSTREAMH
#define STDSTREAMH

#include <iostream.h>
#include <stdio.h>

class stdiobuf : public streambuf {
	/*** stdiobuf is obsolete, should be avoided ***/
public: // Virtuals
	virtual int	overflow(int=EOF);
	virtual int	underflow();
	virtual int	sync() ;
	virtual streampos
#ifdef __cplusplus2
			seekoff(streamoff,seek_dir,int) ;
#else
			seekoff(streamoff,ios::seek_dir,int) ;
#endif
	virtual int	pbackfail(int c);
public:
			stdiobuf(FILE* f) ;
	FILE*		stdiofile() { return fp ; }
	virtual		~stdiobuf() ;
private:
	FILE*		fp ;			
	int		last_op ;
	char		buf[2];
};

class stdiostream : public ios {
public:
			stdiostream(FILE*) ;
			~stdiostream() ;
	stdiobuf*	rdbuf() ;
private:
	stdiobuf	buf ;
};

#endif

