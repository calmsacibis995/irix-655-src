#pragma once

// $Revision: 1.7 $
// $Date: 1993/10/18 19:57:00 $
#include "tkObject.h"

/*
 *  An Point can be either a 2D or 3D Point, depending on the interested
 *  party.
 */
struct Point2;
struct Point3;
struct Point3h;

struct Point : public tkObject {
	Point() { /* contents are garbage */ }

	virtual void set( Point& );
	virtual void set( Point* );
	virtual void set( float, float );
	virtual void set( float, float, float );
	virtual void set( float, float, float, float );

	virtual float getX() const;
	virtual float getY() const;
	virtual float getZ() const;
	virtual float getW() const;

	virtual int getiX() const;
	virtual int getiY() const;
	virtual int getiZ() const;
	virtual int getiW() const;

	virtual void operator=( Point& src );
	virtual Bool operator==( Point& p );

	virtual const char* className();
};


struct Point2 : public Point {
	float x, y;

	Point2() { /* junk */ };
	Point2( Point& p )
	    { x = p.getX(); y = p.getY(); }
	Point2( Point* p )
	    { x = p->getX(); y = p->getY(); }
	Point2(float nx, float ny)
	    { x = nx; y = ny; }
	Point2( float nx, float ny, float nz )
	    { x = nx;  y = ny; /* XXX help! */nz = nz; }
	Point2( float nx, float ny, float nz, float nw )
	    { x = nx;  y = ny; /* XXX aarggh */ nz = nz; nw = nw; }

	/* set the Point */
	void set(Point& p);
	void set(Point* p);
	void set( float nx, float ny );
	void set( float nx, float ny, float nz );
	void set( float xx, float yy, float zz, float ww );
	
	/* get the Point */
	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;
	
	int getiX() const;
	int getiY() const;
	int getiZ() const;
	int getiW() const;
	
	void operator=( Point& src );
	Bool operator==( Point& p );
};


struct iPoint2 : public Point {
	int x, y;

	iPoint2() { /* junk */ };
	iPoint2( Point& p )
	    { x = p.getiX(); y = p.getiY(); }
	iPoint2( Point* p )
	    { x = p->getiX(); y = p->getiY(); }
	iPoint2(int nx, int ny)
	    { x = nx; y = ny; }
	iPoint2( int nx, int ny, int nz )
	    { x = nx;  y = ny; /* XXX help! */nz = nz; }
	iPoint2( int nx, int ny, int nz, int nw )
	    { x = nx;  y = ny; /* XXX aarggh */ nz = nz; nw = nw; }
	iPoint2(float nx, float ny)
	    { x = (int)nx; y = (int)ny; }
	iPoint2( float nx, float ny, float nz )
	    { x = (int)nx;  y = (int)ny; /* XXX help! */nz = (int)nz; }
	iPoint2( float nx, float ny, float nz, float nw )
	    { x = (int)nx;  y = (int)ny; 
	      /* XXX aarggh */ nz = (int)nz; nw = (int)nw; }

	/* set the Point */
	void set(Point& p);
	void set(Point* p);
	void set( int nx, int ny );
	void set( int nx, int ny, int nz );
	void set( int xx, int yy, int zz, int ww );
	void set( float nx, float ny );
	void set( float nx, float ny, float nz );
	void set( float xx, float yy, float zz, float ww );
	
	/* get the Point */
	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;
	
	int getiX() const;
	int getiY() const;
	int getiZ() const;
	int getiW() const;
	
	void operator=( Point& src );
	Bool operator==( Point& p );
};


struct Point3 : public Point {
	float x, y, z;

	Point3() { /* junk */ }
	Point3( Point& p )
	    { x = p.getX(); y = p.getY(); z = p.getZ(); }
	Point3( Point* p )
	    { x = p->getX(); y = p->getY(); z = p->getZ(); }
	Point3(float nx, float ny)
	    { x = nx; y = ny; z = 0.0; }
	Point3( float nx, float ny, float nz )
	    { x = nx;  y = ny;  z = nz; }
	Point3( float nx, float ny, float nz, float nw )
	    { x = nx;  y = ny;  z = nz; /*XXX*/ nw = nw; }

	/* set the Point */
	void set(Point& p);
	void set(Point* p);
	void set( float nx, float ny );
	void set( float nx, float ny, float nz );
	void set( float xx, float yy, float zz, float ww );
	
	/* get the Point */
	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;

	int getiX() const;
	int getiY() const;
	int getiZ() const;
	int getiW() const;

	void operator=( Point& src );
	Bool operator==( Point& p );
};

struct Point3h : public Point {
	float x, y, z, w;

	Point3h() { /* junk */ }
	Point3h( Point& p )
	    { x = p.getX(); y = p.getY(); z = p.getZ(); w = p.getW(); }
	Point3h( Point* p )
	    { x = p->getX(); y = p->getY(); z = p->getZ(); w = p->getW(); }
	Point3h(float nx, float ny)
	    { x = nx; y = ny; z = 0.0; w = 1.0; }
	Point3h( float nx, float ny, float nz )
	    { x = nx;  y = ny;  z = nz; w = 1.0; }
	Point3h( float nx, float ny, float nz, float nw )
	    { x = nx;  y = ny;  z = nz;  w = nw; }

	/* set the Point */
	void set(Point& p);
	void set(Point* p);
	void set( float nx, float ny );
	void set( float nx, float ny, float nz );
	void set( float xx, float yy, float zz, float ww );
	
	/* get the Point */
	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;

	int getiX() const;
	int getiY() const;
	int getiZ() const;
	int getiW() const;

	void operator=( Point& src );
	Bool operator==( Point& p );
};
