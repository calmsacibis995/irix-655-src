
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


#ifndef VKCOMPONENT_H
#define VKCOMPONENT_H

#include <Xm/Xm.h>
#include <Vk/VkCallbackObject.h>

class VkNameList;

#if _VK_MAJOR > 1
class VkScreen;
class VkGlobalData;
#endif

/*******************************************************
CLASS
    VkComponent

    A base class for all classes that support a user interface

OVERVIEW

   VkComponent is a subclass of VkCallbackObject that supports an
   encapsulated widget hierarchy. The VkComponent class is the basis
   of nearly all classes in the ViewKit.  This abstract class defines
   the basic protocol followed by all components in the ViewKit,
   as well as those created by application developers.  The concept of
   a user interface component is the fundamental underlying idea on
   which the ViewKit is based. A component is simply a C++ class that
   has some semantics and a presentation. Nearly all classes in the
   ViewKit are components, including VkApp, VkSimpleWindow, and so
   on. The ViewKit provides many component classes, and encourages an
   approach to building applications based on building
   application-specific components.  Developers generally write
   applications by writing and connecting new components.

GUIDLINES FOR SUBCLASSES

   The following are a set of guidelines for writing components based
   on the VkComponent class.

   All classes derived from VkComponent (known as components) support
   one or more widgets.  Widgets encapsulated by a component should
   form a subtree below a single root widget.

   The root of the widget subtree created by a component is referred
   to as the base widget of the object. The base widget must be
   created by the derived class, and assigned to the _baseWidget
   member inherited from the VkComponent class.

   Components should usually create the base widget and all other
   widgets in the class constructor. The constructor should manage all
   widgets except the base widget, which should be left unmanaged. The
   entire subtree represented by a component can be managed or
   unmanaged using the member functions supported by VkComponent.

   All constructors should take at least two arguments, a widget to be
   used as the parent of the component's base widget, and a string to
   be used as the name of the base widget. The name argument should be
   passed on to the VkComponent constructor, which makes a copy of the
   string. All references to a component's name should use the
   _name member inherited from VkComponent, or the name()
   access function.

   All component classes should override the virtual \fIclassName\fP()
   member function, which is expected to return a string that
   identifies the name of the class.  Components should define any Xt
   callbacks required by the class as static member functions.  These
   functions are normally declared in the private section of the
   class, because they are seldom useful to derived classes.

   All Xt callback functions installed for Motif widgets should be
   passed the this pointer as client data.  Callback functions
   are expected to retrieve this pointer, cast it to the expected
   object type and call a corresponding member function. By
   convention, static member functions used as callbacks have the same
   name as the member function they call, with the word Callback
   appended. For example, the static member function
   startCallback() calls the member function \fIstart\fP().
   Member functions called by static member functions are often
   private, but may also be part of the public or subclass protocol of
   the class. Occasionally it is useful to declare one of these
   functions to be virtual, allowing derived classes to change the
   function ultimately called as a result of a callback.

   Derived classes should call installDestroyHandler()
   immediately after creating a component's base widget. This sets up
   callbacks that handle certain unpleasant problems that can occur
   with regard to widget destruction.

   Derived classes that need to specify default resources to function
   correctly should call the function setDefaultResources() with
   an appropriate resource list before creating the component's base
   widget.

   Derived classes that wish to initialize data members from values in
   the resource database should define an appropriate resource
   specification and call the function getResources()
   immediately after the installDestroyHandler() function.

******************************************************/

class VkComponent : public VkCallbackObject {

  friend class VkCmdFactory;

 public:

    ////////////
    // Destructor
    virtual ~VkComponent();


    ////////////
    // This member function should be called to display the widgets in
    // a component. The show() member function must be called initially
    // to display the widgets, and may be called after hide() has been
    // called to redisplay the component. In the simplest case, show()
    // is analogous to calling XtManageChild() on the base Widget of
    // the component.
    virtual void show();

    //////////// 
    // This member function causes a component to
    // disappear from the screen.  Like show(), hide() is
    // analogous to XtUnmanageChild(_baseWidget). 
    virtual void hide();

    ////////////
    // Shouldn't be used in normal circumstances
    virtual void realize(); 

    ////////////
    // An alias for show() 
    // For compatibility with components based on C++/Motif book
    void manage()   { show(); } 


    ////////////
    // An alias for hide() 
    // For compatibility with components based on C++/Motif book
    void unmanage() { hide(); } 

    ////////////
    // Make this component sensitive to input
    void activate();    

    ////////////
    // Make this component insensitive to input
    void deactivate();

    ////////////
    // True if the component is active
    //
    Boolean isActive();

    ////////////
    // Return the name of an instance
    const char * name() const { return _name; }

    ////////////
    // Return the class name of any instance. Derived classes
    // should override
    virtual const char *className();

    ////////////
    // Return the root of the widget hierarchy
    Widget baseWidget() const;

    ////////////
    // Associate some data with this component. VkComponent does
    // not use or interpret this data
    void setUserData(void *);

    ////////////
    // Retrieve the data stored with this component
    void* getUserData();

    ////////////
    // This function can be used to support "safe quit" mechanisms. In
    // general, this method is only used by VkSimpleWindow and subclasses
    // (See VkAppand VkSimpleWindow). 
    virtual Boolean okToQuit();

    ////////////
    // A heuristic test that returns TRUE is a pointer is to a
    // VkComponent or derived class. Returns FALSE is not a component, 
    // or if the component has been deleted.
    //
    // This function never did modify the parameter, so the interface
    // has been changed to be explicitly const.  It needs to cast away
    // const and call the old one to preserve binary compatibility.
    static Boolean isComponent(const VkComponent *obj)
	{ return (isComponent((VkComponent *)obj)); }

    // This is the older one.  It must remain public for source compatibility.
    // If it becomes private, the compiler will complain it is inaccessible.
    static Boolean isComponent(VkComponent *);

    ////////////
    // Callback Called when a VkComponent is deleted
    static const char * const deleteCallback;

    ////////////
    // Return the base widget when component is cast to (Widget)
    virtual operator Widget () const;

    ////////////
    // Not used, reserved for future use
    virtual void setAttribute(const char *, void *); 

    ////////////
    // Not used, reserved for future use
    virtual char **attributeList(); 

    ////////////
    // Not used, reserved for future use
    virtual void getAttribute(const char *, void **);

    ////////////
    // loadObject following member function supports dynamic loading
    // of objects, as supported by rapidapp. Objects must be
    // set up properly for this to work. See man VkComponent
    // and man VkCallbackObject or the rapidapp documentation for details.
    static VkComponent *loadObject(const char *name,
				   Widget parent,
				   const char *className,
				   const char *filename);
#if _VK_MAJOR > 1
    ////////////
    // WARNING... [ be very careful not to override these methods ]
    //
    virtual VkScreen      *getScreen();
    virtual VkGlobalData  *getGlobalData();
#endif    

  protected:


    ////////////
    // This function should be called by derived classes immediately
    // after the component's base widget is created. It registers an
    // XmNdestroyCallback function for the base widget that helps
    // ensure that the widget is not deleted out from under the
    // object. When linking with the debugging version of the ViewKit
    // library, a warning will be issued about any class that does not
    // install a destroyHandler.
    void installDestroyHandler(); 

    ////////////
    //This function removes the destroy callback installed by
    // installDestroyHandler(). Occasionally, it may be necessary
    // to disable the destroy callback. The VkComponent class removes
    // the callback in the destructor before destroying the widget, to
    // prevent referencing an object after it has been deleted.
    void removeDestroyHandler(); 

    ////////////
    // This virtual function is called when a component's base widget
    // is destroyed. The default VkComponent member function simply
    // NULL's the _baseWidget member. Derived classes may override this
    // function is additional tasks need to be performed in the event
    // of widget destruction.  However, they should always call their
    // base class's method as well.
    virtual void widgetDestroyed(); 


    ////////////
    // Suport for doing things after realize time In spite of the
    // name, this function is actually called when this component is
    // mapped
    virtual void afterRealizeHook();

    ////////////
    // This member function can be called to store a collection of
    // resources in the application's resource database. This is
    // usually done to associate a set of resources with all instances
    // of a class automatically. Resources are loaded with the lowest
    // precedence, so that these resources are true defaults. They can
    // be overridden easily in any resource file.
    void setDefaultResources ( const Widget , const String * );

    ////////////
    // This member function can be used in conjunction with an
    // XtResource list to initialize members of a specific object from
    // values retrieved from the resource database. It must be called
    // after the base widget has been created.
    void getResources ( const XtResourceList, const int );

    ////////////
    // Similar to the above method, but this version initializes
    // an arbitrary structure, passed as a void *
    void getResources ( void *ptr, const XtResourceList, const int );

    ////////////
    // The name of this component
    char   *_name;

    ////////////
    // The root of the widget hierarchy encapsulated by this component
    Widget  _baseWidget;    

    ////////////
    // A reference to _baseWidget, for compatibility with components
    // based on C++/Motif book.  Initialized by the constructors.
    Widget& _w;  

    ////////////
    // The VkComponent constructor initializes the baseWidget to NULL
    // and initializes the _name member of the object. If a string is
    // given as an argument to the constructor, this name is
    // copied. Otherwise, the component is given the temporary name
    // "component". In any case, a dynamically allocated string is
    // assigned to the _name member. The VkComponent constructor is
    // declared to be protected and can only be called from derived
    // classes.
    VkComponent( const char *name ); 

    ////////////
    //Default constructor should never be used directly, but may be
    //called from subclass default constructors.
    VkComponent(); 

    ////////////
    // Copy constructor. Copies the name and NULLs the _baseWidget member.
    //
    // The only reason that this constructor is provided is so that a
    // subclass can have its own copy constructor if it chooses to do so.
    // Raw VkComponent's should not be copied -- doing so introduces
    // VkComponents with NULL baseWidget's.
    //
    // Using this copy constructor is not recommended.  If you do, you
    // are blazing a new trail.  It is debatable whether anyone can tell
    // me in plain English what (in general) copying a Vkcomponent even means.
    //
    // It is, of course, OK for any subclass that wants to have a public
    // copy constructor to do so.
    VkComponent ( const VkComponent& );

    ////////////
    // Operator equal. Copies the name and NULLs the _baseWidget member.
    //
    // The only reason that this constructor is provided is so that a
    // subclass can have its own operator equal if it chooses to do so.
    // Raw VkComponent's should not be copied -- doing so introduces
    // VkComponents with NULL baseWidget's.
    //
    // Using operator=() is not recommended.  If you do, you are blazing
    // a new trail.  It is debatable whether anyone can tell me in plain
    // English what (in general) operator=() of a Vkcomponent even means.
    // If our wisdom in this area expands, these calls may in time become
    // better -- not necessarily compatibly.
    // 
    // VkCallbackObjectOperatorEqual does the real operator=() work.  Without
    // this, subclasses could not have operator=(), because they'd have no
    // way to get the VkCallbackObject opeator=() work done.
    //
    VkComponent &operator=(const VkComponent&);
    VkComponent &VkComponentOperatorEqual(const VkComponent&);
	
#if _VK_MAJOR > 1
    // NOTE:  these are the Vk 1.5.2 extension records...
    //
    Boolean	_noCallbackYet;
    Boolean     _active;    // Indicates sensitivity of base widget
    void       *_userData;  // Place to store data
#else	
    ////////////
    // Used internal to ViewKit to support extensions with breaking
    // compatibility
    void *_extension;
#endif
    
  private:


    ////////////
    // convenience function that can be registered with  VkCmdFactory
    // objects to be notified if the command becomes active or
    // inactive. 
    void changeActiveCallback(VkCallbackObject *, void *, void *);

   // Support for sanity check of object validity

    VkComponent *_self;  

    void destroyVkComponent();	// Used by operator=() and the destructor

    static void widgetDestroyedCallback ( Widget, 
					  XtPointer, 
					  XtPointer );

    static void afterRealizeEventHandler ( Widget, 
					   XtPointer, 
					   XEvent *,
					   Boolean *);


};


// A convenience for applications that need to pass both an object and
// other client data This should almost NEVER be needed

typedef struct {
    void *client_data;
    void *obj;
} VkCallbackStruct;

#endif
