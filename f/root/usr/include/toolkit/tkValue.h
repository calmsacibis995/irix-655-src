#pragma once

// $Revision: 1.9 $
// $Date: 1990/08/09 14:39:25 $
#include "CharString.h"

enum tkValueType {
	IntegerValue, LongValue, FloatValue, StringValue
};

// tkValue class.  tkValues hold a value for somebody.

class tkValue : public tkObject {
protected:
	tkValueType type;		// type of value we are holding
	union {
		int i;
		long l;
		float f;
		CharString *s;
	} datum;			// current value

public:
	tkValue()		{ datum.i = 0; type = IntegerValue; }
	~tkValue();
	tkValue( tkValue const& v ) { datum = v.datum; type = v.type; }
	tkValue( tkValue* v )	{ datum = v->datum; type = v->type; }
	tkValue(int i)		{ datum.i = i; type = IntegerValue; }
	tkValue(long l)		{ datum.l = l; type = LongValue; }
	tkValue(float f)	{ datum.f = f; type = FloatValue; }
	tkValue(CharString& s)	{ datum.s = &s; type = StringValue; }
	tkValue(CharString* s ) { datum.s = s; type = StringValue; }
	tkValue( char* s );

	// Set the value
	    void setValue(int i)
		{ datum.i = i; type = IntegerValue; }
	    void setValue(long l)
		{ datum.l = l; type = LongValue; }
	    void setValue(float f)
		{ datum.f = f; type = FloatValue; }
	    void setValue(CharString& s)
		{ datum.s = &s; type = StringValue; }
	    void setValue( CharString* s )
		{ datum.s = s; type = StringValue; }
	    void setValue( char* s );

	// Return the boolean value of this value.
	//	value is Boolean:
	//		return value directly
	//	value is integer:
	//		return TRUE if value is non-zero, FALSE otherwise
	//	value is float:
	//		return TRUE if value is non-zero, FALSE otherwise
	//	value is string:
	//		return TRUE if the string is non-null, FALSE otherwise
	    void getValue( Bool *b ) const;

	// Return the integer value of this value.
	//	value is Bool:
	//		return 1 if TRUE, 0 if FALSE.
	//	value is integer:
	//		return value directly.
	//	value is float:
	//		return the nearest integer directly.
	//	value is string:
	//		return the atoi() of the string
	    void getValue( int *i ) const;
	    void getValue( long *l ) const { getValue( (int *)l ); }

	// Return the float value of this value.
	//	value is Bool:
	//		return 1.0 if TRUE, 0.0 if FALSE.
	//	value is integer:
	//		return (float) value
	//	value is float:
	//		return value directly.
	//	value is string:
	//		return the atod() of the string
	    void getValue( float *f ) const;
	    void getValue( double *d ) const;

	// Return the string value of this value.
	//	value is Bool:
	//		return "" if TRUE, (char *)0 if FALSE.
	//	value is integer:
	//		return "%d" of value
	//	value is float:
	//		return "%g" of value
	//	value is string:
	//		return the value directly
	    void getValue( CharString *s ) const;
	    void getValue( char* ) const;

	// Return the current type of the value.
	    tkValueType getType() const
		{ return type; }

	// Assign one value to another.
	    void operator=(tkValue const& src);
	    void operator=( int i )
		{ datum.i = i; type = IntegerValue; }
	    void operator=( long l )
		{ datum.l = l; type = LongValue; }
	    void operator=( double d )			// can't identify float
		{ datum.f = (float)d; type = FloatValue; }
	    void operator=( CharString& s )
		{ *(datum.s) = s; type = StringValue; }
	    void operator=( CharString* s )
		{ datum.s = s; type = StringValue; }
	    void operator=( char* s );

	// Do comparison between values
	    Bool operator==( tkValue const& v ) const;

	virtual const char* className();
};
