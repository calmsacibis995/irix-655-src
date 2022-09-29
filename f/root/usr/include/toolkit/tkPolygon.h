#pragma once

// $Revision: 1.7 $
#include "tkRenderItem.h"

#define MAXPOLYPOINTS	20

class tkPolygon : public tkRenderItem {
protected:
	int	pointCount;
	float	pointArray[MAXPOLYPOINTS][2];
	float	leftCoord, rightCoord, bottomCoord, topCoord;
	short	closed;				// should this be Bool???
	tkPolygon() { }

public:
	tkPolygon(Box2 const& a, tkPen const& p);
	tkPolygon(FILE* strm);

	void	addPoint(float	x, float  y);
	/* this one does some mapping for us */
	void	addPoint(int x, int y);
	void	addPoint(Point2 const& p)
		{
		    addPoint(int( p.x ), int( p.y ));
		}
	void	addPoint(Point3 const& p)
		{
		    addPoint(int( p.x ), int( p.y ));
		}
	void	setLocalCoord(double l, double r, double b, double t)
		{
		    leftCoord = l; rightCoord = r;
		    bottomCoord = b; topCoord = t;
		}
	int	getPointCount()
		{ return pointCount; }
	void	setClosure(short c)
		{ closed = c; }
	short	getClosure()
		{ return closed; }
	virtual void draw();
	void	translatePoints(float delx, float dely);
	void	translatePoint(int index, float delx, float dely);
	void	scalePoints(float mulx, float muly);
	virtual void changeArea( Box2 const& a );
	virtual void getBoundingBox(Box2& result);

	virtual const char* className();
};
