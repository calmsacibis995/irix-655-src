////////////////////////////////////////////////////////////////////////////////
///////   Copyright 1995, Silicon Graphics, Inc.  All Rights Reserved.   ///////
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


#ifndef VKBASE_H
#define VKBASE_H

// IMPORTANT:  include these common defs... [ version info ]
//
#include <Vk/VkCommonDefs.h>

// VkBase is an abstract class.
//
// The purpose is so that all ViewKit classes have a standard opaque
// pointer that can be used to extend the class without breaking binary
// compatibility.
//
//	* The first time we can break binary compatibility, all ViewKit
//	  classes will be descendants of it.
//
//	* In the meantime, all new ViewKit classes are subclassed from it.
//
//
// This pointer is for use only by classes that come with the ViewKit product.
// Unfortunately we cannot make it available to derived classes, because
// then it, too, would become a binary compatibility item.  Derived
// libraries may wish to employ a similar strategy themselves.
//
// The constructor is included in the class so that it will get inlined in the
// constructor of each class that directly inherits from VkBase.  Thus, there
// is no run-time penalty, such as there would be if a function call to the
// constructor were required.

class VkBase {

  protected:
#if _VK_MAJOR > 1
    // NOTE:  Used internal to ViewKit to support extensions without 
    //        breaking compatibility...
    //
    void *_extension;
#else
    void *__extension_record;
#endif

    VkBase();
    virtual ~VkBase();

};

#endif /* VKBASE_H */

