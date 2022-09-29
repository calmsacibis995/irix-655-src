#pragma once

// $Revision: 1.7 $
// $Date: 1990/08/09 14:38:27 $
#include "tkModelItem.h"
#include "Box2.h"
#include "tkPen.h"

class tkRenderItem : public tkModelItem {
protected:
	Box2	area;
	tkPen	pen;

public:
	tkRenderItem();
	~tkRenderItem();
	virtual const char* className();

	virtual void changeArea(Box2 const& a);
	virtual void changePen(tkPen const& p);
	virtual Box2* getArea();
	virtual tkPen* getPen();
};
