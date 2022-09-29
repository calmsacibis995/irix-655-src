#ifndef	_tkTypes_
#define	_tkTypes_

// $Revision: 1.5 $
// $Date: 1992/10/30 09:27:12 $

// Boolean type
typedef unsigned char Bool;
#undef	TRUE
#define	TRUE	((Bool)1)
#undef	FALSE
#define	FALSE	((Bool)0)

// minimum of two numbers
inline long
min(long a, long b)
{
	return (a < b) ? a : b;
}

inline float
min(float a, float b)
{
	return (a < b) ? a : b;
}

// maximum of two numbers
inline long
max(long a, long b)
{
	return (a > b) ? a : b;
}

inline float
max(float a, float b)
{
	return (a > b) ? a : b;
}

// Absolute value
inline long
abs(long a)
{
	return (a < 0) ? -a : a;
}

inline float
abs(float a)
{
	return (a < 0.0) ? -a : a;
}
#endif
