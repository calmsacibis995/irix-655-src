#pragma once

// $Revision: 1.10 $
// $Date: 1992/10/20 10:29:50 $
#include "tkCltn.h"

typedef int (*tkCompare)( char*, char* );

class tkVector : public tkCltn {

// Abstract vector class.  Defines common components to all vector type's.
// Vectors are single dimensional arrays that use contiguous memory for
// holding their contents.  These vectors are dynamically expandable.
protected:
	void	*base;
	int	_space_;	/* number of elements in vector */
	int	n;	/* number of used elements in vecotr */

public:
	tkVector(int startingSize = 0);
	~tkVector();

	/* Return the size of an element */
	virtual long elementSize();

	/* Expand the vector to hold more elements */
	virtual void expand(int newSize);

	/*
	 * Compress the array to hold exactly as many elements as it
	 * already has.
	 */
	virtual int  vectorIndex(char* item);
	virtual Bool inVector(char* item);
	virtual void addToVector(char* item);
	virtual void insertInVector(char* item, int newIndex);
	virtual void removeFromVector(char* item);
	virtual void compress();
	virtual void relocateItem(int oldIndex, int newIndex);

	virtual int elements();
	virtual Bool isEmpty();
	virtual int size();
	virtual void sort( tkCompare, int siz = 0 );
	virtual const char* className();
};

/*
 * Macro magic to define a vector type (subclass of tkVector)
 * for a particular data type. Note that pushing puts an
 * item at 0 and poping puts it at the end of the list.
 */
#define	Vector(NAME,type)					\
class NAME : public tkVector {					\
public:								\
	NAME(int startingSize = 0) : tkVector(startingSize) { }	\
	type& operator[](int index)				\
	    { expand(index+1); return ((type *)base)[index]; }	\
	int  cltnIndex(type item)				\
	    { return tkVector::vectorIndex((char *)item); }	\
	Bool inCltn(type item)					\
	    { return tkVector::inVector((char *)item); }	\
	void addToCltn(type item)				\
	    { tkVector::addToVector((char *)item); }		\
	void removeFromCltn(type item)				\
	    { tkVector::removeFromVector((char *)item); }	\
	void insertInCltn(type item, int index)			\
	    { tkVector::insertInVector((char *)item, index); }	\
	void push(type item)					\
	    { tkVector::removeFromVector((char *)item);		\
	      tkVector::insertInVector((char *)item, 0); }	\
	void pop(type item)					\
	    { tkVector::removeFromVector((char *)item);		\
	      tkVector::addToVector((char *)item); }		\
	virtual long elementSize();				\
	virtual const char* className();			\
};

#define	VectorImplementation(NAME,type)				\
long NAME::elementSize()						\
{								\
	return sizeof(type);					\
}								\
const char* NAME::className()					\
{								\
	return "NAME";						\
}
