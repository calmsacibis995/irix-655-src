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

#ifndef VKAPP_H
#define VKAPP_H

//////////////////////////////////////////////////////////////////////////////////////////
// VkApp.h

#include <X11/Xlib.h>
#include <Vk/VkComponent.h>
#include <Vk/VkComponentList.h>

class VkApp;
class VkSimpleWindow;
class VkBusyDialog;
class VkDialogManager;
class VkCursorList;

extern VkApp             *theApplication;
extern unsigned int       VkDebugLevel;

/*******************************************************
CLASS
    VkApp

OVERVIEW





******************************************************/

class VkApp : public VkComponent {

    friend class VkSimpleWindow;
    friend class VkQuickHelp;

  public:

  // Normal constructor -- most apps should use this one
    VkApp(char             *appClassName,
	  int              *arg_c, 
	  char            **arg_v,
	  XrmOptionDescRec *optionList       = NULL,
	  int               sizeOfOptionList = 0);

  // This constructor is rarely needed.  The pricipal use is to allow setting
  // resources when creating the first shell.
  //	* Any resource that can be passed on the arglist, such as default
  //	  font list, should be passed in that way.
  //
  //	* Visual information cannot be known until after the display is
  //	  opened.  preRealizeFunction(w) will be called with the
  //	  baseWidget (i.e the shell).  This is after widget creation, but
  //	  before it is realized.
  //
  //	  At that time, the application can find the visual information
  //	  it needs, and do a setValues call.  The manual not withstanding,
  //	  since the widget has not yet been realized, this is OK -- provided
  //	  that the call sets *all* visual resources consistently.
  //
  // The motivation for all of this is that without it you could not have
  // a non-default-visual, single-visual, application.

    VkApp(char             *appClassName,
	  int              *arg_c, 
	  char            **arg_v,
	  ArgList	    argList,
	  Cardinal	    argCount,
	  void		  (*preRealizeFunction)(Widget w),
	  XrmOptionDescRec *optionList,
	  int               sizeOfOptionList);

  // Only for certain internal testing.
    VkApp(Widget w);

     static const int ViewKitMajorRelease;
     static const int ViewKitMinorRelease;
     static const char ViewKitReleaseString[];

     void   setVersionString(const char *str);
     const char  *versionString() { return _versionString;}

     virtual      ~VkApp();

    ////////////
    // Callback invoked when a new window registers itself
    // with the application object
    static const char *const newWindowCallback;

    ////////////
    // Callback invoked when a window unregisters itself
    // with the application object, normally when the
    // window object has been deleted
    static const char *const windowDestroyedCallback;

    ////////////
    // An application should not need to override run().  If there really
    // is required extra event handling, it is better to use
    // 		run(Boolean (*appEventHandler)(XEvent &))
    // See "man VkApp" for further information.
    void run_first();				// Stuff before the event loop
    virtual void run();				// Binary compatibility
    void run(Boolean (*appEventHandler)(XEvent &));	// App extra events
    void runOneEvent(Boolean (*appEventHandler)(XEvent &) = NULL);

    virtual void terminate(int status = 0);

    ////////////
    // An application should not need to override handlePendingEvents().  If
    // there really is required extra event handling, it is better to use
    // 		handlePendingEvents(Boolean (*appEventHandler)(XEvent &))
    // See "man VkApp" for further information.
    virtual void handlePendingEvents();
	    void handlePendingEvents (Boolean (*appEventHandler)(XEvent &));
    XtInputMask handleOnePendingEvent (Boolean (*appEventHandler)(XEvent &) = NULL);

    ////////////
    // This sets a flag, determining whether subsequent VkSimpleWindow's
    // will be in the overlay planes or not.  This only works if it is
    // called before the application's VKApp is constructed.
    static void useOverlayApps(const Boolean flag = TRUE);

    virtual void quitYourself();
            void handleRawHelpEvent(XEvent *event);
    virtual void handleRawEvent(XEvent *event);
    static  void useSchemes(const char*);

  
    void         setMainWindow(VkSimpleWindow *);  // Let the app know that a particular "window" is the main one

    // Operations on all windows

    virtual void raise();
    virtual void lower();
    virtual void iconify();
    virtual void open();
    virtual void show();
    virtual void hide();

    void    startupIconified(const Boolean flag) { _startupIconified = flag; }

    virtual Cursor  busyCursor();
    virtual Cursor  normalCursor();
    void    setNormalCursor(const Cursor);
    void    setBusyCursor(const Cursor);
    void    setBusyCursor(VkCursorList *);
    void    showCursor(const Cursor);
    void    setAboutDialog(VkDialogManager *d) { _aboutDialog = d; }
    VkDialogManager*  aboutDialog() { return (_aboutDialog); }

    void    setStartupDialog(VkDialogManager *d) { _startupDialog = d; }
    VkDialogManager*  startupDialog() { return (_startupDialog); }

    virtual void         busy(const char *msg = NULL,     VkSimpleWindow *parent = NULL);
    virtual void         veryBusy(const char *msg = NULL, VkSimpleWindow *parent = NULL);
    virtual void         notBusy();

    virtual void         progressing(const char *msg = NULL);

    void  setBusyDialog(VkBusyDialog *);

    XtAppContext        appContext() const { return _appContext;}
    char               *name() const;
    Display            *display() const { return _dpy;}
    char              **argv() const { return _argv; }
    char               *argv(int index);
    int                 argc() const { return _argc;}
    char               *applicationClassName() const { return _applicationClassName;}
    VkSimpleWindow     *mainWindow() const;
    char               *shellGeometry() const;  // The size of the hidden shell
    Boolean             startupIconified() const { return _startupIconified; }
    Boolean             isBusy() { return (_busyCounter > 0); }
    virtual const char *className();

    static void setFallbacks(char **);

    VkComponentList *windowList();

    ////////////
    // Support for providing help.
    // 
    // Historically, the way to provide help from a ViewKit
    // application has been to link the application with a library
    // that implemented the C functions SGIHelpInit(), SGIHelpMsg(),
    // and SGIHelpIndexMsg().  ViewKit and the application would call
    // these functions in response to users' requests for help.
    //
    // Starting in IRIX 6.5, help can also be provided by adding
    // callbacks to the "helpInitCallback", "helpMsgCallback", and
    // "helpIndexMsgCallback" lists.  Instead of calling the
    // SGIHelp*() functions, ViewKit and the application call the
    // VkApp methods helpInit(), helpMsg(), and helpIndexMsg().
    //
    // If there are no callbacks installed on the corresponding
    // callback lists, the VkApp help methods call the corresponding
    // SGIHelp*() methods so that old applications continue to work.
    // If callbacks are installed, the callbacks are called instead.
    //
    // An application that provides help using the sgihelp program can
    // call the VkApp method useSGIHelp(), which will install:
    //
    //  sgiHelpInit() on the "helpInitCallback" list
    //  sgiHelpMsg() on the "helpMsgCallback" list
    //  sgiHelpIndexMsg() on the "helpIndexMsgCallback" list
    //
    // sgiHelpInit(), sgiHelpMsg(), and sgiHelpIndexMsg() interact
    // with the sgihelp program to provide the same help that
    // applications used to get by linking with -lhelpmsg.
    //
    // An application wishing to provide its own help mechanism can add
    // its own callbacks to the callback lists.  The following table
    // lists the callback names and the type for the "callData" argument
    // to the callbacks:
    //
    //  Name                 	callData
    //  ----------------------------------
    //  helpInitCallback	HelpInitArgs*
    //  helpMsgCallback		HelpMsgArgs*
    //  helpIndexMsgCallback	HelpIndexMsgArgs*
    //
    // After installing these callbacks, helpInit() will need to be
    // called.
    //

    ////////////
    // API for applications and ViewKit to use to invoke help.
    int helpInit(char *client, char *sep);
    int helpMsg(char *key, char *book, char *userData);
    int helpIndexMsg(char *key, char *book);

    ////////////
    // Callback lists for help.
    static const char *const helpInitCallback;
    static const char *const helpMsgCallback;
    static const char *const helpIndexMsgCallback;

    ////////////
    // "callData" for "helpInitCallback".
    struct HelpInitArgs {
	Display *display;
	char *client;
	char *sep;
	int returnValue;
    };

    ////////////
    // "callData" for "helpMsgCallback".
    struct HelpMsgArgs {
	char *key;
	char *book;
	char *userData;
	int returnValue;
    };

    ////////////
    // "callData" for "helpIndexMsgCallback".
    struct HelpIndexMsgArgs {
	char *key;
	char *book;
	int returnValue;
    };

    ////////////
    // API for applications that want to provide help using the
    // sgihelp program.
    void useSGIHelp();

    ////////////
    // Callbacks which when installed on the help callback lists
    // implement help using the sgihelp program.
    static void sgiHelpInit(VkCallbackObject* caller, void* clientData,
			    void* callData);
    static void sgiHelpMsg(VkCallbackObject* caller, void* clientData,
			   void* callData);
    static void sgiHelpIndexMsg(VkCallbackObject* caller, void* clientData,
				void* callData);

  protected:

    VkComponentList    _winList;
    int parseCommandLine(XrmOptionDescRec  *, Cardinal); 
    virtual void afterRealizeHook();
    Boolean _quitSemaphore;

    VkCursorList  *_busyCursorList;

  // helpEnabled() is called by the QuickHelp consructor.  It could be
  // public, but it is an internal imlementation detail tht we do not want
  // user apps to use.  So we make VkQuickHelp a friend of VkApp.
    void helpEnabled (Boolean);

  private:

    static  XtResource _resSpec[];
    static  String _resources[];

    static Cursor _busyCursor;
    static Cursor _normalCursor;

    VkDialogManager *_aboutDialog;
    VkDialogManager *_startupDialog;

    void        createCursors();
    Boolean     manageShowHide();
  // Constructors' real initialization code
    void	VkAppInitialize(
			char             *appClassName,
			int              *arg_c, 
			char            **arg_v,
			ArgList		  argList,
			Cardinal	  argCount,
			void		(*preRealizeFunction)(Widget w),
			XrmOptionDescRec *optionList,
			int               sizeOfOptionList);

    // Various data needed by other parts of an application


    char              **_argv;
    int                 _argc;
    char               *_shellGeometry;
    VkSimpleWindow     *_mainWindow;
    char               *_versionString;
    Display            *_dpy;
    XtAppContext        _appContext;
    char               *_applicationClassName;
    void                 addWindow ( VkSimpleWindow * );
    void                 removeWindow( VkSimpleWindow * );
    int                 _busyCounter;
    Boolean             _startupIconified;
    Boolean             _usePopupPlanes;
    static const char  *_useSchemes;
    static String      *_fallbacks;
    static Boolean	_useOverlayApps;

  private:

    // There is only one VkApp per application.
    //
    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkApp(const VkApp&);
    VkApp &operator =(const VkApp&);
}; 

#endif /* VKAPP_H */
