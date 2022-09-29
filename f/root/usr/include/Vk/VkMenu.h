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

#ifndef VKMENU_H
#define VKMENU_H

#include <Vk/VkMenuItem.h>
#include <Vk/VkSimpleWindow.h>
#if _VK_MAJOR > 1
#include <Vk/VkScreen.h>
#endif

class VkWindow;
class VkMenuAction;
class VkMenuActionWidget;
class VkMenuToggle;
class VkSubMenu;
class VkRadioSubMenu;
class VkMenuUndoAction;
class VkMenuRedoAction;
class VkCmdFactory;
class VkUndoCmdFactory;
class VkRedoCmdFactory;
class VkToggleCmdFactory;
class VkCmdManager;

struct VkMenuDesc {
    VkMenuItemType       menuType;
    char		*name;    
    XtCallbackProc       callback;
    VkMenuDesc          *submenu;
    XtPointer            clientData;
    XtCallbackProc       undoCallback;
};

/*******************************************************
CLASS
    VkMenu

    Base class for all ViewKit menus

OVERVIEW

      



******************************************************/


class VkMenu : public VkMenuItem {

    friend   class VkMenuItem;
    friend   class VkMenuBar;
    friend   class VkOptionMenu;
    friend   class VkSubMenu;
    friend   class VkMenuUndoRedoCmdAction;

  public:

    ////////////
    // Destructor
    virtual ~VkMenu();

    ////////////
    // Create the widget for this menu
    virtual void build(Widget);

    ////////////
    // Create a normal "action" entry, i.e. a button to the
    // menu.
    VkMenuAction       *addAction(const char     *name, 
				  XtCallbackProc  func = NULL, 
				  XtPointer       data = NULL, 
				  int             pos  = -1);

    ////////////
    // Add an action to the menu, forcing the UI representation to be
    // a widget rather than a gadget (the default)
    VkMenuActionWidget *addActionWidget(const char     *name, 
					XtCallbackProc func = NULL, 
					XtPointer      data = NULL, 
					int            pos = -1);

    ////////////
    // Add an action that is only executed if the user responds to 
    // a confirming dialog
    VkMenuConfirmFirstAction  *addConfirmFirstAction(const char     *name, 
						     XtCallbackProc  func = NULL, 
						     XtPointer       data = NULL, 
						     int             pos = -1);

    ////////////
    // Add a VkCmd or subclass to the menu. A VkMenuAction item will
    // be created and the command will automatically be connected to
    // the button.  The cmd object will be registered with the
    // specified cmdmanager when it is executed
    VkMenuAction  *addCmd(const char   *name, 
			  VkCmdFactory *cmd, 
			  VkCmdManager *cmdManager = NULL, 
			  void         *data = NULL, 
			  int            pos = -1);


    ////////////
    // Add an undo cmd item to the menu. The item automatically tracks
    // availability of undo, label changes, etc.
    VkMenuAction  *addCmd(const char   *name, 
			  VkUndoCmdFactory *cf, 
			  VkCmdManager *cmdManager = NULL,
			  int            pos = -1);
			  


    ////////////
    // Add a redo cmd item to the menu
    VkMenuAction  *addCmd(const char   *name, 
			  VkRedoCmdFactory *cf, 
			  VkCmdManager *cmdManager = NULL,
			  int            pos = -1);

    
    ////////////
    // Add a cmd by name. This assumes that the Cmd has been registered
    // in the VkCmdRegistry. The optional third argument determines
    // whether this  item is a toggle or a button
    VkMenuItem  *addCmd(const char    *name, 
			const char    *cmdToken,
			Boolean        toggle = FALSE,
			VkCmdManager  *cmdManager = NULL, 
			void          *data = NULL,
			int            pos = -1);


    ////////////
    // Add a VkToggleCmd or subclass to the menu. A VkMenuToggle
    // item will be created and the command will automatically
    // be connected to the toggle. If an undo manager object is 
    // specified, the object will be added to this undo list
    // when it is executed
    VkMenuToggle  *addCmd(const char         *name,
			  VkToggleCmdFactory *cmdf, 
			  VkCmdManager       *cmdManager = NULL,
			  int                 pos = -1);

    ////////////
    //  Add a separator to the menu. The name will be "sep".
    VkMenuSeparator    *addSeparator(int pos = -1);

    ////////////
    // Add a named separator to the menu
    VkMenuSeparator    *addSeparator(const char *name, int pos = -1);    


    ////////////
    // Adds a non-selectable label to the menu
    VkMenuLabel        *addLabel(const char *name, int pos = -1);


    ////////////
    // Add a toggle item to the menu
    VkMenuToggle       *addToggle(const char     *name, 
				  XtCallbackProc  func = NULL, 
				  XtPointer       data = NULL, 
				  int             state = -1, 
				  int             pos = -1);

    ////////////
    //
    //
    void                add(VkMenuItem *item, int pos = -1);

    ////////////
    //
    //
    VkSubMenu *addSubmenu(VkSubMenu *submenu, int pos = -1);

    ////////////
    //
    //
    VkSubMenu *addSubmenu(const char *name, int pos = -1);

    ////////////
    //
    //
    VkSubMenu *addSubmenu(const char *name, 
			  VkMenuDesc*, 
			  XtPointer defaultClientData = NULL, 
			  int pos = -1);

    ////////////
    //
    //
    VkRadioSubMenu *addRadioSubmenu(VkRadioSubMenu *submenu, int pos = -1);

    ////////////
    //
    //
    VkRadioSubMenu *addRadioSubmenu(const char *name, int pos = -1);

    ////////////
    //
    //
    VkRadioSubMenu *addRadioSubmenu(const char *name, VkMenuDesc*, XtPointer defaultClientData = NULL);


    ////////////
    // enter a submenu into the system, but don't show or attach it
    // This allows it to be found using findNamedItem
    VkMenuItem *registerSubmenu(const char * name, 
				XtCallbackProc func, 
				XtPointer data, 
				VkMenuItem * submenu);
    

    ////////////
    // manipulation of existing menu items, many more to be added
    VkMenuItem *findNamedItem(const char * , Boolean caseless = FALSE);


    ////////////
    //
    VkMenuItem *removeItem(const char *);

    ////////////
    //
    //
    VkMenuItem *activateItem(const char * );

    ////////////
    //
    //
    VkMenuItem *deactivateItem(const char * );

    ////////////
    //
    //
    VkMenuItem *replace(const char * , VkMenuItem * );


    ////////////
    //
    //
    static  void useWorkProcs(Boolean flag);

    ////////////
    //
    //
    static  void useOverlayMenus(Boolean flag = TRUE);


    ////////////
    //
    //
    virtual VkMenuItemType menuType () = 0;

    ////////////
    //
    //
    virtual const char* className();

    ////////////
    //
    //
    Boolean isContainer();

    ////////////
    //
    //
    int getItemPosition(VkMenuItem*);

    ////////////
    //
    //
    int getItemPosition(char *name);

    ////////////
    //
    //
    int getItemPosition(Widget);

    ////////////
    //
    //
    VkMenuItem * operator[] (int index) const;

    ////////////
    //
    //
    int numItems() const;

    ////////////
    // Deprecated.
    VkMenuAction   *addAction(const char     *name,
			      XtCallbackProc  func, 
			      XtCallbackProc  undoCallback, 
			      XtPointer       data, 
			      int             pos  = -1);


    ////////////
    // Deprecated
    VkMenuActionWidget *addActionWidget(const char      *name, 
					XtCallbackProc   func,
					XtCallbackProc   undoCallback, 
					XtPointer        data, 
					int              pos = -1);

    ////////////
    // Deprecated
    VkMenuToggle       *addToggle(const char     *name, 
				  XtCallbackProc  func, 
				  XtCallbackProc  undoCallback, 
				  XtPointer       data, 
				  int             state = -1, 
				  int             pos = -1);

#if _VK_MAJOR > 1
    virtual  VkScreen  *getScreen() { return _vkScreen; }
#endif

  protected:

    VkMenuItem     **_contents;
    int              _nItems;
    int              _maxItems;
	
#if _VK_MAJOR > 1
	VkScreen		*_vkScreen;
#endif

    ////////////
    //
    //
    VkMenu();

    ////////////
    //
    //
    VkMenu(const char *name);

    ////////////
    //
    //
    VkMenu(VkMenuDesc *, XtPointer defaultClientData=NULL);

    ////////////
    //
    //
    VkMenu(const char *name, VkMenuDesc *, XtPointer defaultClientData=NULL);


    ////////////
    //
    //
    VkMenu *findParent(VkMenuItem *);

    ////////////
    //
    //
    VkSimpleWindow *getWindow();

    ////////////
    //
    //
    virtual Widget menuParent() = 0;

    ////////////
    //
    // Called before creating a menu.  This sets the visual attributes
    // correctly -- either by a copy from the proper visual parent, or else
    // setting a consistent set of overlay resources.
    //
    void argList
		( Widget parent, Arg* args, int *cnt );

    ////////////
    //
    // Called to set up the callback to install the colormap.
    void setColormap(Widget w);

#if _VK_MAJOR > 1
    Boolean usingOverlayMenus(Widget w);
#endif	

  private:

    static Boolean _useWorkProcs;
    static Boolean _useOverlayMenus;
#if _VK_MAJOR > 1
    // NOTE:  these static variables are no longer valid in ViewKit 2.1
    //
#else
    static Colormap _overlayColormap;
#endif
    void handleMenuDescriptor(VkMenuDesc *, XtPointer);
    void addMenuEntry(VkMenuItem *, int pos = -1);
    int  removeMenuEntry(VkMenuItem *);

    static void executeCmdCallback(Widget, XtPointer, XtPointer);
    static void executeToggleCmdCallback(Widget, XtPointer, XtPointer);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenu(const VkMenu&);
    VkMenu &operator= (const VkMenu&);
};

#endif
