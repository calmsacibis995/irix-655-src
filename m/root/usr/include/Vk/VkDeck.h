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
#ifndef VKDECK_H
#define VKDECK_H

#include <Vk/VkComponent.h>

class VkWidgetList;

class VkDeck : public VkComponent {

  public:

    enum TransitionStyle { POP, SLIDELEFT, SLIDERIGHT, SLIDEUP, SLIDEDOWN, DISOLVE };

    VkDeck(const char * name, Widget);
   ~VkDeck();

    void addView(Widget);
    void addView(VkComponent *);

    void pop(TransitionStyle style = POP);
    void pop(Widget, TransitionStyle style = POP);
    void pop(VkComponent*, TransitionStyle style = POP);
    void push(TransitionStyle style = POP);
    void push(Widget, TransitionStyle style = POP);
    void push(VkComponent*, TransitionStyle style = POP);

    Widget topWidget();
    VkComponent *topComponent();    
    
    virtual const char* className();

  protected:

    VkWidgetList *_children;
    int _current;

    void pushWidget(Widget w);
    void popWidget(Widget w);

    void afterRealizeHook();

private:

    GC     _gc;
    Pixmap _cover;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkDeck(const VkDeck&);
    VkDeck &operator= (const VkDeck&);
}; 

#endif
