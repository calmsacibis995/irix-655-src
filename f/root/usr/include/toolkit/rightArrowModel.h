#ifndef _rightArrowModel_
#define _rightArrowModel_

#ifndef _tkRenderItem_
#include "tkRenderItem.h"
#endif

#ifndef _tkPolygon_
#include "tkPolygon.h"
#endif

class rightArrowModel : public tkRenderItem {
protected:
    int		displayState;

    tkPolygon	*outline;
    tkPolygon	*innershadowTop;
    tkPolygon	*outtershadowTop;
    tkPolygon	*outtershadowBot;
    tkPolygon	*fill;

public:
    rightArrowModel(int w, int h, int state);
    virtual ~rightArrowModel();
    virtual void draw();
    virtual void getBoundingBox(Box2&);
};

#endif

