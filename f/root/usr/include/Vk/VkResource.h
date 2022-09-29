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
#ifndef VKRESOURCE_H
#define VKRESOURCE_H

#include <Xm/Xm.h>


extern char * VkGetResource(const char * name, 
			    const  char * className);


extern XtPointer VkGetResource(Widget w, 
			       const char *names, 
			       const char *classNames, 
			       const char *desiredType, 
			       const char *defaultValue);


///////////////////////////////////////////////////
// Obsolete interfaces
///////////////////////////////////////////////////


extern char * VkGetResource(Widget w, 
			    const char * name, 
			    const char * className);
extern XtPointer VkGetResource(Widget w, 
			       const char * name, 
			       const char * className, 
			       const char * defaultStr);

extern XtPointer VkGetResource(Widget w, 
			     const char * name, 
			     const char * className, 
			     const char * desiredType, 
			     int size, 
			     void * defaultValue);

extern XtPointer VkGetResource(Widget w, 
			       const char * name, 
			       const char * className, 
			       const char * desiredType, 
			       int size, 
			       const char * defaultValue);

#endif



