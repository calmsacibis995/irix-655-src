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

#ifndef VKFILESELECTIONDIALOG_H
#define VKFILESELECTIONDIALOG_H

#include <Vk/VkDialogManager.h>

/*******************************************************
CLASS
    VkFileSelectionDialog

OVERVIEW



******************************************************/


class VkFileSelectionDialog : public VkDialogManager {

  public:


    ////////////
    //
    //
    VkFileSelectionDialog( const char* name ) : 
	VkDialogManager( name ) { _fileName   = NULL; 
	_selection  = NULL;
	_filter     = NULL;
	_directory  = NULL;  
	_showApply  = TRUE;  }


    ////////////
    //
    //
    virtual ~VkFileSelectionDialog();


    ////////////
    //
    //
    const char* className();


    ////////////
    //
    //
    const char* fileName()       const { return _fileName; }


    ////////////
    //
    void setDirectory(const char *);


    ////////////
    //
    //
    void setSelection(const char *);

    ////////////
    //
    //
    void setFilterPattern(const char *);

  protected:


    ////////////
    //
    //
    virtual Widget createDialog( Widget dialogParent );


    ////////////
    //
    //
    virtual void ok(Widget, XtPointer);

    ////////////
    //
    //
    virtual void apply(Widget, XtPointer);

    ////////////
    //
    //
    virtual void cancel(Widget, XtPointer);


    ////////////
    //
    //
    XmFileSelectionBoxCallbackStruct *callData() const;


  private:

    char  *_fileName;
    char *_selection;
    char *_filter;
    char *_directory;

    XmFileSelectionBoxCallbackStruct *_callData;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkFileSelectionDialog(const VkFileSelectionDialog&);
    VkFileSelectionDialog &operator= (const VkFileSelectionDialog&);
};

extern VkFileSelectionDialog *getTheFileSelectionDialog();

#if _VK_MAJOR > 1
extern VkFileSelectionDialog *getAppFileSelectionDialog(VkComponent *comp);
    
// WARNING... [ this macro is only valid for single screen apps ]
//
#define theFileSelectionDialog getAppFileSelectionDialog(NULL)

#else
    
#define theFileSelectionDialog getTheFileSelectionDialog()
#endif // _VK_MAJOR > 1


#endif // VKFILESELECTIONDIALOG_H
