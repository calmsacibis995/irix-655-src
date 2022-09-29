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

#ifndef VKWARNINGDIALOG_H
#define VKWARNINGDIALOG_H

#include <Vk/VkDialogManager.h>

class VkWarningDialog : public VkDialogManager {

  public:
    
    VkWarningDialog(const char* name) : VkDialogManager(name) { _showCancel = FALSE; }
    ~VkWarningDialog();

  protected:

    virtual Widget createDialog(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkWarningDialog(const VkWarningDialog&);
    VkWarningDialog &operator= (const VkWarningDialog&);
};

extern VkWarningDialog *getTheWarningDialog();

#if _VK_MAJOR > 1
extern VkWarningDialog *getAppWarningDialog(VkComponent *comp);
    
// WARNING... [ this macro is only valid for single screen apps ]
//
#define theWarningDialog getAppWarningDialog(NULL)

#else
    
#define theWarningDialog getTheWarningDialog()
#endif /* _VK_MAJOR > 1 */

#endif
