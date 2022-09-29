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

#ifndef VKERRORDIALOG_H
#define VKERRORDIALOG_H

/*******************************************************
CLASS
    VkErrorDialog

OVERVIEW





AUTHOR
   Doug Young
******************************************************/


#include <Vk/VkDialogManager.h>

class VkErrorDialog : public VkDialogManager {

  public:


    ////////////
    //
    //    
    VkErrorDialog(const char* name) : VkDialogManager(name) { _showCancel = FALSE;
							      _showApply = FALSE; }

    ////////////
    //
    //
    ~VkErrorDialog();

  protected:


    ////////////
    //
    //
    virtual Widget createDialog(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkErrorDialog(const VkErrorDialog&);
    VkErrorDialog &operator= (const VkErrorDialog&);
};

extern VkErrorDialog *getTheErrorDialog();

#if _VK_MAJOR > 1
extern VkErrorDialog *getAppErrorDialog(VkComponent *obj);

// WARNING... [ this macro is only valid for single screen apps ]
// 
#define theErrorDialog getAppErrorDialog(NULL)

#else

#define theErrorDialog getTheErrorDialog()
#endif /* _VK_MAJOR */

#endif
