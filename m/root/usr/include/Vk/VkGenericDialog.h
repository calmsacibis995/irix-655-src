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

#ifndef VKGENERICDIALOG_H
#define VKGENERICDIALOG_H

/*******************************************************
CLASS
    VkGenericDialog

OVERVIEW





AUTHOR
   Doug Young
******************************************************/

#include <Vk/VkDialogManager.h>

class VkGenericDialog : public VkDialogManager {

  public:


    ////////////
    //
    //
    ~VkGenericDialog();

  protected:


    ////////////
    //
    //
    virtual Widget createDialog(Widget);
    

    ////////////
    //
    //
    VkGenericDialog(const char* name) : VkDialogManager(name) { }

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkGenericDialog(const VkGenericDialog&);
    VkGenericDialog &operator= (const VkGenericDialog&);
};

#endif
