
//////////////////////////////////////////////////////////////
//
// Header file for RmBD
//
//    This file is generated by RapidApp 1.2
//
//    This class is derived from RmBDUI which 
//    implements the user interface created in 
//    RapidApp. This class contains virtual
//    functions that are called from the user interface.
//
//    When you modify this header file, limit your changes to those
//    areas between the "//---- Start/End editable code block" markers
//
//    This will allow RapidApp to integrate changes more easily
//
//    This class is a ViewKit user interface "component".
//    For more information on how components are used, see the
//    "ViewKit Programmers' Manual", and the RapidApp
//    User's Guide.
//////////////////////////////////////////////////////////////
#ifndef RMBD_H
#define RMBD_H
#include "RmBDUI.h"
//---- Start editable code block: headers and declarations

//---- End editable code block: headers and declarations

//---- RmBD class declaration

class RmBD : public RmBDUI
{

  public:

    RmBD ( const char *, Widget );
    RmBD ( const char * );
    ~RmBD();
    const char *  className();
    virtual void apply(Widget, XtPointer);
    virtual void cancel(Widget, XtPointer);
    virtual void ok(Widget, XtPointer);

    static VkComponent *CreateRmBD( const char *name, Widget parent ); 

    //---- Start editable code block: RmBD public

    //---- End editable code block: RmBD public

  protected:

    //---- Start editable code block: RmBD protected

    //---- End editable code block: RmBD protected

  private:

    static void* RegisterRmBDInterface();

    //---- Start editable code block: RmBD private

    //---- End editable code block: RmBD private

};
//---- Start editable code block: End of generated code

//---- End editable code block: End of generated code

#endif