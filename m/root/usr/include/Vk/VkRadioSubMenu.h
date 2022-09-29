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

#ifndef RADIOSUBMENU_H
#define RADIOSUBMENU_H

#include <Vk/VkSubMenu.h>

class VkRadioSubMenu : public VkSubMenu {

   public:
    
    VkRadioSubMenu(const char *, 
		   VkMenuDesc * desc = NULL, 
		   XtPointer defaultClientData = 0);

    VkRadioSubMenu(Widget, 
		   const char *, 
		   VkMenuDesc * desc = NULL, 
		   XtPointer defaultClientData = 0);

    ~VkRadioSubMenu();


    virtual const char* className();
    VkMenuItemType menuType ();

   protected:

    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkRadioSubMenu(const VkRadioSubMenu&);
    VkRadioSubMenu &operator= (const VkRadioSubMenu&);
};

#endif

