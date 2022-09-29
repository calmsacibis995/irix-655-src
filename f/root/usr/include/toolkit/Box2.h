#pragma once

#include "Point.h"
#include "tkGID.h"

class Box2 : public tkObject {
public:
	float	xorg, yorg;
	float	xlen, ylen;

	Box2()
		{ /* contents are garbage */ }
	Box2( double llx, double lly, double width, double height )
		{ xorg = llx; yorg = lly; xlen = width; ylen = height; }
	Box2( int llx, int lly, int width, int height )
		{ xorg = (float)llx; yorg = (float)lly; 
		  xlen = (float)width; ylen = (float)height; }
	Box2( Box2 const& b )
	    { xorg = b.xorg; yorg = b.yorg;
	      xlen = b.xlen; ylen = b.ylen; }



	void setBounds( float norgx, float norgy, float nlenx, float nleny );

	/* return upper right corner */
	float getCornerX() const;
	float getCornerY() const;
	int getiCornerX() const;
	int getiCornerY() const;


	float getOriginX() const	{ return xorg; }
	float getOriginY() const	{ return yorg; }
	float getExtentX() const	{ return xlen; }
	float getExtentY() const	{ return ylen; }

	int getiOriginX() const	{ return (int)xorg; }
	int getiOriginY() const	{ return (int)yorg; }
	int getiExtentX() const	{ return (int)xlen; }
	int getiExtentY() const	{ return (int)ylen; }

	/* find things out */
	void center(Point2& cent)
		{ cent.x = xorg + xlen / 2.0; cent.y = yorg + ylen / 2.0; }

	/* return TRUE if the given thing is inside this box */
	Bool inside( float x, float y ) const;
	Bool inside(Point const& point) const;

	/* return the intersection of this box and the other one */
	Box2& intersect(Box2& result, Box2 const& other) const;
	Box2* intersect(Box2* result, Box2 const* other) const;

	/* image a box */
	void fill() const
		{ rectf(xorg, yorg, xorg + xlen - 1, yorg + ylen - 1); }
	void outline() const
		{ rect(xorg, yorg, xorg + xlen - 1, yorg + ylen - 1); }

	void operator=( Box2 const& );
};
