#ifndef _CharString_
#define _CharString_

// $Revision: 1.6 $
// $Date: 1992/10/30 09:24:09 $
#ifndef	_tkObject_
#include "tkObject.h"
#endif

class CharString : public tkObject {
protected:
    char	*p;
    int		len;
public:

    CharString()
	{ p = 0; len = 0; }
    CharString( char*, int l = 0 );
    CharString( CharString&, int l = 0 );
    CharString( CharString*, int l = 0 );
    ~CharString()	{ delete p; }

    int size()		{ return len; }
    char* string()	{ return p ? p : ""; }

    CharString& operator=( int );
    CharString& operator=( long );
    CharString& operator=( double );
    CharString& operator=( CharString& );
    CharString& operator=( CharString* );
    CharString& operator=( char* );

    Bool operator==( CharString& str );
    Bool operator==( char* s );
};

#endif
