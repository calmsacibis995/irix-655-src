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
#ifndef VKTICKMARKS_H
#define VKTICKMARKS_H

#include <Vk/VkComponent.h>

class VkTickMarks : public VkComponent {

  public:

    VkTickMarks(const char *name, Widget parent, Boolean labelsToLeft = True,
		Boolean noLabels = False, Boolean centerLabels = False);
    ~VkTickMarks();
    
    virtual const char* className();
    
    void setScale(int min, int max, int majorInterval, int minorInterval);
    void setMargin(int marginTop, int marginBottom);
    
    int labelSpacing() { return _labelSpacing; }
    void addLabel(int value);
    static VkComponent *CreateVkTickMarks(const char *name, Widget parent);
    
  protected:

    void adjustWidth();
    void draw();
    
    static void expose_stub(Widget w, XtPointer clientData, XtPointer callData);
    static void resize_stub(Widget w, XtPointer clientData, XtPointer callData);
    
    Pixel _fg, _bg;
    XmFontList _fontList;
    GC _gc;
    int _min, _max, _majorInt, _minorInt;
    int _majorSize, _minorSize, _labelSpacing;
    int _marginTop, _marginBottom, _lineThickness;
    Boolean _labelsToLeft, _connectTicks, _noLabels, _centerLabels;
    Boolean _signedLabels;
    XFontStruct *_fs;
    int *_labels;
    int _numLabels, _sizeLabels;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkTickMarks(const VkTickMarks&);
    VkTickMarks &operator= (const VkTickMarks&);
}; 

#endif
