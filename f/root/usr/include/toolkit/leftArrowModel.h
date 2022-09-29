#ifndef _leftArrowModel_
#define _leftArrowModel_

#ifndef _tkRenderItem_
#include "tkRenderItem.h"
#endif

#ifndef _tkPolygon_
#include "tkPolygon.h"
#endif

class leftArrowModel : public tkRenderItem {
protected:
    int		displayState;

    tkPolygon	*outline;
    tkPolygon	*innershadowTop;
    tkPolygon	*outtershadowTop;
    tkPolygon	*outtershadowBot;
    tkPolygon	*fill;

public:
    leftArrowModel(int w, int h, int state);
    virtual ~leftArrowModel();
    virtual void draw();
    virtual void getBoundingBox(Box2&);
};

#endif

