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

#ifndef VKOPTIONMENU_H
#define VKOPTIONMENU_H

#include <Vk/VkMenu.h>

class VkOptionMenu : public VkMenu {

  public:

    VkOptionMenu(VkMenuDesc *desc, XtPointer defaultClientData = NULL);
    VkOptionMenu(const char *name = "optionMenu", VkMenuDesc *desc = NULL, XtPointer defaultClientData = NULL);

    VkOptionMenu(Widget, VkMenuDesc *desc, XtPointer defaultClientData = NULL);
    VkOptionMenu(Widget, const char *name = "optionMenu", VkMenuDesc *desc = NULL, XtPointer defaultClientData = NULL);


    ~VkOptionMenu();

    virtual VkMenuItemType menuType ();

    void set(char*);
    void set(int);
    void set(VkMenuItem *);

    int getIndex();
    VkMenuItem *getItem();

    void forceWidth(int);          // Set the size of all existing menu entries
    virtual void build(Widget);
    virtual const char* className();

  protected:

    Widget _pulldown;
    Widget menuParent();
    virtual void widgetDestroyed();

  private:

    VkMenuItem *_currentItem;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkOptionMenu(const VkOptionMenu&);
    VkOptionMenu &operator= (const VkOptionMenu&);
    
};

#endif
