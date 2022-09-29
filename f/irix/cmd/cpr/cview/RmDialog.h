
//////////////////////////////////////////////////////////////
//
// Header file for RmDialog
//
//    This file is generated by RapidApp 1.2
//
//    This class is a ViewKit VkDialogManager subclass
//    See the VkDialogManager man page for info on the API
//
//    Restrict changes to those sections between
//    the "//--- Start/End editable code block" markers
//    This will allow RapidApp to integrate changes more easily
//
//    This class is a ViewKit user interface "component".
//    For more information on how ViewKit dialogs are used, see the
//    "ViewKit Programmers' Manual"
//
//////////////////////////////////////////////////////////////
#ifndef RMDIALOG_H
#define RMDIALOG_H
#include <Vk/VkGenericDialog.h>

//---- Start editable code block: headers and declarations


//---- End editable code block: headers and declarations



//---- RmDialog class declaration

class RmDialog: public VkGenericDialog { 

  public:

    RmDialog ( Widget w, const char * name );
    RmDialog ( const char * name );
    ~RmDialog();
    const char *className();
    //---- Start editable code block:  public


    //---- End editable code block:  public

  protected:

    virtual Widget createDialog ( Widget );
    Widget prepost ( const char      *msg,
                     XtCallbackProc   okCB,
                     XtCallbackProc   cancelCB,
                     XtCallbackProc   applyCB,
                     XtPointer        clientData,
                     const char      *helpString,
                     VkSimpleWindow  *parentWindow);


    // Classes created by this class

    class RmBD *_rmBD;


    // Widgets created by this class




    // Member functions called from callbacks

    virtual void apply ( Widget, XtPointer );
    virtual void cancel ( Widget, XtPointer );
    virtual void ok ( Widget, XtPointer );

    //---- Start editable code block:  protected


    //---- End editable code block:  protected


  private:

    // Callbacks to interface with Motif


    //---- Start editable code block:  private


    //---- End editable code block:  private

};

//---- Start editable code block: End of generated code


//---- End editable code block: End of generated code

#endif