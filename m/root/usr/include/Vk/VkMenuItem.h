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


// Classes for menu items in a VkMenu object.
    
#ifndef VKMENUITEM_H
#define VKMENUITEM_H
    
#include <Vk/VkComponent.h>

class VkComponentList;
class VkCmdList;
class VkCmdManager;

#define MAXWIDGETS 25


enum  VkMenuItemType {
    END,                     // Used to mark the end of a static menu structure definition
	ACTION,              // A "normal" menu item - uses a pushbutton gadget
	ACTIONWIDGET,        // Same as an action, but forces a widget to be used
	SUBMENU,             // A cascading submenu
	RADIOSUBMENU,        // Same as above, but forced to act as a radio-style pane
	SEPARATOR,           // A separator gadget
	LABEL,               // A label gadget
	TOGGLE,              // A two-state toggle button gadget
	OPTION,              // An XmOption Menu
	POPUP,               // a popup pane
	BAR,                 // a menu bar
        CONFIRMFIRSTACTION,  // An action that will not be executed unless user confirms first
	OBJECT,              // A user-defined subclass of VkMenuActionObject
	STUB,                // Used internally to support undo of arbirtrary callbacks
        COMMAND,
        TOGGLE_COMMAND
	};


/*******************************************************
CLASS
    VkMenuItem

OVERVIEW




******************************************************/

class VkMenuItem : public VkComponent {
    
    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

 public:


    ////////////
    //
    //
    ~VkMenuItem();
    
    ////////////
    //
    //
    virtual void setLabel(const char *);

    ////////////
    //
    //
    char *getLabel();
    

    ////////////
    //
    //
    void setPosition(int );

    ////////////
    //
    //
    void activate();

    ////////////
    //
    //
    void deactivate();

    ////////////
    //
    //
    int remove();

    ////////////
    //
    //
    void show();

    ////////////
    //
    //
    void show(Widget);

    ////////////
    //
    //
    void hide();


    ////////////
    //
    //
    virtual const char* className();

    ////////////
    //
    //
    virtual Boolean isContainer();

    ////////////
    //
    //
    virtual  VkMenuItemType menuType () = 0;

    ////////////
    //
    //
    Widget baseWidget() const;


 protected:

    int                _position;
    Boolean            _isBuilt;
    int                _sensitive;
    class VkMenu      *_parentMenu;
    char              *_label;

    Boolean         _isHidden;


    ////////////
    //
    //
    VkMenuItem();

    ////////////
    //
    //
    VkMenuItem(const char * );

    static  Widget  _unmanagedWidgets[MAXWIDGETS];
    static  int     _numUnmanagedWidgets;
    static  void    manageAll();

    static VkComponentList *_workProcList;


    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuItem(const VkMenuItem&);
    VkMenuItem &operator =(const VkMenuItem&);
};

/*******************************************************
CLASS
    VkMenuAction

OVERVIEW




******************************************************/

class VkMenuAction : public VkMenuItem {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuAction(const char *name, XtCallbackProc func = NULL, XtPointer clientData = NULL);

    ////////////
    //
    //
    VkMenuAction(const char *, XtCallbackProc, XtCallbackProc, XtPointer clientData = NULL);

    ////////////
    //
    //
    ~VkMenuAction();


    ////////////
    //
    //
    virtual void undo();

    ////////////
    //
    //
    Boolean hasUndo() { return (_undoCallback != NULL); }

    ////////////
    //
    //
    virtual VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:

    XtCallbackProc  _undoCallback;
    XtCallbackProc  _func;       
    void           *_data;           


    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuAction(const VkMenuAction&);
    VkMenuAction &operator =(const VkMenuAction&);
};


/*******************************************************
CLASS
    VkMenuCmdAction

OVERVIEW




******************************************************/

class VkMenuCmdAction : public VkMenuAction {

    friend class VkMenu;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;

  public:

    ////////////
    // Constructor
    //    
    VkMenuCmdAction(const char *name, XtCallbackProc func = NULL,
		    XtPointer clientData = NULL);

    void setCmdManager(VkCmdManager *um) { _cmdManager = um; }
    void setData(void *d) { _cmddata = d;}

  protected:

    VkCmdManager  *_cmdManager;
    void          *_cmddata;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuCmdAction(const VkMenuCmdAction&);
    VkMenuCmdAction &operator =(const VkMenuCmdAction&);
};

/*******************************************************
CLASS
    VkMenuUndoRedoCmdAction

    Private class, used internally


AUTHOR
   Doug Young
******************************************************/

class VkMenuUndoRedoCmdAction : public VkMenuCmdAction {

    friend class VkMenu;
    
public:
    
    ////////////
    // Constructor
    //    
    VkMenuUndoRedoCmdAction(const char *name, XtCallbackProc func = NULL, XtPointer clientData = NULL);
    
  protected:

    ////////////
    // Update the label on the command when the undo/redo cmd changes
    //
    virtual void update(VkCallbackObject *, void *, void*);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuUndoRedoCmdAction(const VkMenuUndoRedoCmdAction&);
    VkMenuUndoRedoCmdAction &operator =(const VkMenuUndoRedoCmdAction&);
};



/*******************************************************
CLASS
    VkMenuConfirmFirstAction

OVERVIEW





******************************************************/


class VkMenuConfirmFirstAction : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuConfirmFirstAction(const char *name, XtCallbackProc func = NULL, XtPointer clientData = NULL);

    ////////////
    //
    //
    ~VkMenuConfirmFirstAction();

    ////////////
    //
    //
    virtual VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:


    ////////////
    //
    //
    virtual void build(Widget);


  private:

     static void actionCallback(Widget, XtPointer, XtPointer);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuConfirmFirstAction(const VkMenuConfirmFirstAction&);
    VkMenuConfirmFirstAction &operator =(const VkMenuConfirmFirstAction&);
};

/*******************************************************
CLASS
    VkMenuActionObject

OVERVIEW




******************************************************/

class VkMenuActionObject : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuActionObject(const char *, void *clientData = NULL);

    ////////////
    //
    //
    ~VkMenuActionObject();


    ////////////
    //
    //
    virtual VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:


    ////////////
    //
    //
    virtual void undoit(void *) = 0;

    ////////////
    //
    //
    virtual void doit(void *) = 0;
    void *_clientData;

  private:

    static void undoCallback(Widget, XtPointer, XtPointer);
    static void doitCallback(Widget, XtPointer, XtPointer);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuActionObject(const VkMenuActionObject&);
    VkMenuActionObject &operator =(const VkMenuActionObject&);
};


/*******************************************************
CLASS
    VkMenuActionStub

OVERVIEW




******************************************************/

class VkMenuActionStub : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuActionStub(const char *name, XtCallbackProc func = NULL, XtPointer clientData = NULL);

    ////////////
    //
    //
    ~VkMenuActionStub();


    ////////////
    //
    //
    virtual VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:

     virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuActionStub(const VkMenuActionStub&);
    VkMenuActionStub &operator =(const VkMenuActionStub&);
};

/*******************************************************
CLASS
    VkMenuActionWidget

OVERVIEW





******************************************************/


class VkMenuActionWidget : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:
    

    ////////////
    //
    //
    VkMenuActionWidget(const char *name, 
		       XtCallbackProc func = NULL, 
		       XtPointer clientData = NULL);


    ////////////
    //
    //
    VkMenuActionWidget(const char *, 
		       XtCallbackProc, 
		       XtCallbackProc func, 
		       XtPointer clientData = NULL);


    ////////////
    //
    //
    ~VkMenuActionWidget();


    ////////////
    //
    //
    virtual  VkMenuItemType menuType ();


    ////////////
    //
    //
    virtual const char* className();

  private:


    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuActionWidget(const VkMenuActionWidget&);
    VkMenuActionWidget &operator =(const VkMenuActionWidget&);
};



/*******************************************************
CLASS
    VkMenuLabel

OVERVIEW




******************************************************/

class VkMenuLabel : public VkMenuItem {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuLabel(const char *);

    ////////////
    //
    //
    ~VkMenuLabel();

    ////////////
    //
    //
    virtual  VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:


    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuLabel(const VkMenuLabel&);
    VkMenuLabel &operator =(const VkMenuLabel&);
};


/*******************************************************
CLASS
    VkMenuSeparator

OVERVIEW





******************************************************/


class VkMenuSeparator : public VkMenuLabel {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    //
    //    
    VkMenuSeparator();

    ////////////
    //
    //
    VkMenuSeparator(const char *name);    

    ////////////
    //
    //
    ~VkMenuSeparator();

    ////////////
    //
    //
    virtual VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

  protected:

    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuSeparator(const VkMenuSeparator&);
    VkMenuSeparator &operator =(const VkMenuSeparator&);
};


/*******************************************************
CLASS
    VkMenuToggle

OVERVIEW





******************************************************/

class VkMenuToggle : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:


    ////////////
    // Constructor
    VkMenuToggle(const char *, 
		 XtCallbackProc func = NULL, 
		 XtPointer clientData = NULL);

    ////////////
    // Constructor
    VkMenuToggle(const char *, 
		 XtCallbackProc, XtCallbackProc, 
		 XtPointer clientData = NULL);

    ////////////
    // Destructor
    ~VkMenuToggle();


    ////////////
    // return the type of this menu item
    virtual  VkMenuItemType menuType ();

    ////////////
    //
    //
    virtual const char* className();

    ////////////
    //
    //
    void setVisualState(Boolean state);

    ////////////
    //
    //
    void setStateAndNotify(Boolean);

    ////////////
    //
    //
    Boolean getState();

    //////////////////////////////////////
    // For use with a VkToggleCmd object
    // the callData is expected to be a Boolean
    // that indicates the current state
    void update(VkCallbackObject*, void *, void *);

  protected:

    int             _state;


    ////////////
    //
    //
    virtual void build(Widget);

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuToggle(const VkMenuToggle&);
    VkMenuToggle &operator =(const VkMenuToggle&);
};


/*******************************************************
CLASS
    VkMenuCmdToggle

OVERVIEW





AUTHOR
   Doug Young
******************************************************/

class VkMenuCmdToggle : public VkMenuToggle {

    friend class VkMenu;
  public:

    ////////////
    // Constructor
    //    
    VkMenuCmdToggle(const char *name, XtCallbackProc func = NULL,
		    XtPointer clientData = NULL);

    void setCmdManager(VkCmdManager *cm) { _cmdManager = cm; }
    void setData(void *d) { _cmddata = d;}

    const char* className() { return "VkMenuCmdToggle"; };

    VkMenuItemType menuType() { return TOGGLE_COMMAND; };

  protected:

    VkCmdManager  *_cmdManager;
    void          *_cmddata;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuCmdToggle(const VkMenuCmdToggle&);
    VkMenuCmdToggle &operator =(const VkMenuCmdToggle&);
};


///////////////////////////////////////////////////////////////////
// Undo support - DEPRECATED - Use VkUndoManager instead
//
///////////////////////////////////////////////////////////////////

class VkComponentList;
class VkAction;

class VkMenuUndoManager : public VkMenuAction {

    friend  class VkMenu;
    friend  class VkMenuBar;
    friend  class VkPopupMenu;
    friend  class VkMarkingMenu;
    friend  class VkOptionMenu;
    friend  class VkSubMenu;

  public:

    VkMenuUndoManager(const char *);
    ~VkMenuUndoManager();

     void  reset();
     void  add(VkMenuAction *);    
     void  add(const char *name, XtCallbackProc undoCallback, XtPointer clientData);
     void  multiLevel (Boolean flag) { _multiLevel = flag; }
     VkComponentList *historyList() { return _commands; }
     virtual const char* className();

  protected:

     VkComponentList *_commands;
     virtual void undo();
     void setUndoLabel();
     virtual void build(Widget);

 
  private:
    
    static void undoCallback(Widget, XtPointer, XtPointer);
    XmString _labelString;
    Boolean  _multiLevel;
    Boolean  _undoMode;

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkMenuUndoManager(const VkMenuUndoManager&);
    VkMenuUndoManager &operator =(const VkMenuUndoManager&);
};

extern VkMenuUndoManager *getTheUndoManager();
    
#define theUndoManager getTheUndoManager()


#endif












