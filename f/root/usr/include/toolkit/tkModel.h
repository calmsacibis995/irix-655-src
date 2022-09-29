#pragma once

// $Revision: 1.13 $
// $Date: 1991/01/16 18:36:03 $
#include "tkModelItem.h"
#include "tkModelCltn.h"
#ifndef	GD_YPMAX
#include "tkGID.h"
#endif

class tkModel : public tkModelItem {
protected:
	friend class tkModelClass;
	tkModelCltn mc;
	Matrix xform;

public:
	tkModel(int startingSize = 0); // makes xform = I
	tkModel(FILE* strm);

	~tkModel();

	// replicate tkVector subclass interface
	tkModelItem*& operator[](int i)
	    { return mc[i]; }
	int cltnIndex(tkModelItem* item)
	    { return mc.cltnIndex(item); }
	Bool inCltn(tkModelItem* item)
	    { return mc.inCltn(item); }
	void addToCltn(tkModelItem* item)
	    { mc.addToCltn(item); }
	void removeFromCltn(tkModelItem* item)
	    { mc.removeFromCltn(item); }
	void insertInCltn(tkModelItem* item, int index)
	    { mc.insertInCltn(item, index); }
	void push(tkModelItem* item)
	    { mc.push(item); }
	void pop(tkModelItem* item)
	    { mc.pop(item); }
	int size()
	    { return mc.elements(); }

	void localTransform() { ::multmatrix( xform ); }
	void setMatrix( Matrix const & m );
	void translate( Point const & );
	void scale( Point const & );
	void scale( float );
	//void rotate( float, char );

	virtual void draw();
	virtual void getBoundingBox(Box2& result);

	virtual const char* className();
};
