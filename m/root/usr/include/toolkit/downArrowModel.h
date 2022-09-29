#ifndef _downArrowModel_
#define _downArrowModel_

#ifndef _tkRenderItem_
#include "tkRenderItem.h"
#endif

#ifndef _tkPolygon_
#include "tkPolygon.h"
#endif

class downArrowModel : public tkRenderItem {
protected:
    int		displayState;

    tkPolygon	*outline;
    tkPolygon	*innershadow;
    tkPolygon	*outtershadow;
    tkPolygon	*fill;

public:
    downArrowModel(int w, int h, int state);
    virtual ~downArrowModel();
    virtual void draw();
    virtual void getBoundingBox(Box2&);
};

#endif

