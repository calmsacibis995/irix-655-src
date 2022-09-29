#pragma once

// $Revision: 1.3 $
// $Date: 1990/08/09 14:35:22 $
#include "tkRenderItem.h"

// define the border extents

#define BORDER_LS   3
#define BORDER_RS   3
#define BORDER_TS   4
#define BORDER_BS   5

class tkBorderModel : public tkRenderItem {
protected:
	tkBorderModel() { }

	//  the border colors.
	int light;
	int medium;
	int dark;

public:
	//  area determines the total extent of the border.  When a view is
	//  to be surrounded by a border, it should have a smaller extent.
	//  pen is used to fill the "inside" of the border:
	//    background color is used to fill a region offset from the border
	//    edge color is used to paint an internal border around this region

	//  The rest of the colors are predetermined.

	tkBorderModel(Box2 const& a, tkPen const & p, int lcolor = 7,
		      int mcolor = 15, int dcolor = 8);

	virtual void draw();
};
