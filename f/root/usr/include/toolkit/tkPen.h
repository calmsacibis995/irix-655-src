#pragma once

// $Revision: 1.6 $
// $Date: 1991/01/11 18:49:37 $
#include "tkObject.h"
#ifndef	GD_YPMAX
#include "tkGID.h"
#endif

/* tkPen
 * tkPen defines the notion of a drawing pen. It contains
 * drawing attributes, like color, linewidth and font.
 * Other toolkit objects are drawn using the attributes
 * of an associated pen.
 */
class tkPen : public tkObject {
protected:
	short	fgcolor;
	short	bgcolor;
	short	edgecolor;
	short	linewidth;

public:
	tkPen(short fg = WHITE, short bg = BLACK, short ec = -1, short lw = 0)
	    { fgcolor = fg; bgcolor = bg; edgecolor = ec; linewidth = lw; }
	tkPen(FILE* strm);

	void setForegroundColor( short c ) { fgcolor = c; }
	void setBackgroundColor( short c ) { bgcolor = c; }
	void setEdgeColor( short c ) { edgecolor = c; }
	void setLineWidth( short c ) { linewidth = c; }

	short getForegroundColor() const { return fgcolor; }
	short getBackgroundColor() const { return bgcolor; }
	short getEdgeColor() const { return edgecolor; }
	short getLineWidth() const { return linewidth; }

	virtual const char* className();
};
