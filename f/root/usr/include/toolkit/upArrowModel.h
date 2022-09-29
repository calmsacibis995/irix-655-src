#ifndef _upArrowModel_
#define _upArrowModel_

#ifndef _tkRenderItem_
#include "tkRenderItem.h"
#endif

#ifndef _tkPolygon_
#include "tkPolygon.h"
#endif

class upArrowModel : public tkRenderItem {
protected:
    int		displayState;

    tkPolygon	*outline;
    tkPolygon	*innershadow;
    tkPolygon	*outtershadow;
    tkPolygon	*fill;

public:
    upArrowModel(int w, int h, int state);
    virtual ~upArrowModel();
    virtual void draw();
    virtual void getBoundingBox(Box2&);
};

#endif

