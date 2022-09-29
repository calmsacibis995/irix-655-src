////////////////////////////////////////////////////////////////////////////////
///////   Copyright 1992, Silicon Graphics, Inc.  All Rights Reserved.   ///////
//                                                                            //
// This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;     //
// the contents of this file may not be disclosed to third parties, copied    //
// or duplicated in any form, in whole or in part, without the prior written  //
// permission of Silicon Graphics, Inc.                                       //
//                                                                            //
// RESTRICTED RIGHTS LEGEND:                                                  //
// Use,duplication or disclosure by the Government is subject to restrictions //
// as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data     //
// and Computer Software clause at DFARS 252.227-7013, and/or in similar or   //
// successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -    //
// rights reserved under the Copyright Laws of the United States.             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
#ifndef VKVUMETER_H
#define VKVUMETER_H

#include <Vk/VkComponent.h>

class VkVuMeter : public VkComponent {

  public:

    VkVuMeter(const char *name, Widget parent);
    ~VkVuMeter();
    
    void setValue(int value, int peak);
    
    virtual const char* className();

    static VkComponent *CreateVkVuMeter(const char *name, Widget parent);
  
  protected:

    void expose();
    void resize();

    static void expose_stub(Widget w, XtPointer clientData, XtPointer callData);
    static void resize_stub(Widget w, XtPointer clientData, XtPointer callData);
    
    int _granularity;
    int _spacing;
    int _size;
    int _max;
    int _interval;
    int _lastValue, _lastPeak;
    int _numReds, _numYellows, _numGreens;
    Dimension _width, _height, _segmentSize, _segmentSpacing, _topMargin;
    GC _gcRed, _gcGreen, _gcYellow;
    Boolean _drawSides, _peakBlock;
    int _sideThickness, _sideSpacing, _peakHeight;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkVuMeter(const VkVuMeter&);
    VkVuMeter &operator= (const VkVuMeter&);
}; 

#endif
