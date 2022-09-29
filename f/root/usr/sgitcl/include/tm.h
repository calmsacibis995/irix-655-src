
#ifndef _TM_H
#define _TM_H

#include <stdio.h>
#include <stdlib.h>
#include <tcl.h>
#include <Xm/Xm.h>

#if USE_UIL
#include <Mrm/MrmPublic.h>
#endif

#if XmVersion == 1001
#  define MOTIF11
#endif

#ifdef MOTIF11
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#define XmFONTLIST_DEFAULT_TAG XmSTRING_DEFAULT_CHARSET
#endif

#ifdef DEBUG_MALLOC
#include <dbmalloc.h>
#endif

#define TM_MAXARGS 100
#define TM_NUM_PARAMS 10

/* global vbl names for stuf used in Text verify callbacks */
#define TM_TEXT_DOIT "_Tm_Text_Doit"
#define TM_TEXT_STARTPOS "_Tm_Text_StartPos"
#define TM_TEXT_ENDPOS "_Tm_Text_EndPos"
#define TM_TEXT_PTR "_Tm_Text_Ptr"
#define TM_TEXT_LENGTH "_Tm_Text_Length"

/* global vbl names for result capturing */
#define TM_RESULT_BUF "_Tm_Result_Buf"
#define TM_SAVE_RESULT "_Tm_Save_Result"

/* global vbl names for stuff used in convertProc in D&D */
#define TM_CONVERT_TYPE "_Tm_Convert_Type"
#define TM_CONVERT_VALUE "Tm_Convert_Value"

/* forward def */
struct Tm_Widget;

/*
 * This contains info that is common to all widgets
 * created under one display
 */
typedef struct Tm_Display {
    Display	*display;
    Widget	toplevel;
    Widget	commWidget;	/* used for send command */
    Atom 	registryProperty;
    Atom	commProperty;
    int		numshellwidgets;
    struct Tm_Widget  **shellwidgets;
    struct Tm_Display *next;
#if USE_UIL
    MrmHierarchy hierarchy;
#endif
} Tm_Display;

/*
 * each widget created by tcl has one of these
 */
typedef struct Tm_Widget {
    Widget	widget;		/* Xt widget */
    char	*pathName;	/* full path from `.' */
    Tcl_Interp  *interp;	/* interp for this widget */
    char 	*parent;	/* parent path name from `.' */
    char	*dropProc;	/* D&D proc */
    char	*transferProc;	/* D&D proc */
    char 	*convertProc;	/* D&D proc */
    Tm_Display	*displayInfo;	/* info shared by all in this interp */
}   Tm_Widget;

/* This structure is used as the client data field in callback functions */
typedef struct Tm_ClientData {
    char	*callback_func;
    Tm_Widget   *widget_info;
}   Tm_ClientData;

/* This structure is used for Input Handlers */
typedef struct Tm_InputData {
    Tcl_Interp *interp;
    char *command;	/* command to be executed by input handler */
} Tm_InputData;

/* This structure is used for Timer Handlers */
typedef struct Tm_TimerData {
    Tcl_Interp *interp;
    char *command;	/* command to be executed by input handler */
} Tm_TimerData;

typedef struct {
    char *fileName;
}   Tm_ResourceType, *Tm_ResourceTypePtr;

typedef int (*Tm_WidgetCmdProc)_ANSI_ARGS_((ClientData clientData, Tcl_Interp *interp,
            int argc, char **argv));

/* used in drag and drop */
typedef struct Tm_TransferStruct {
    char *value;
    char *closure;
} Tm_TransferStruct;

/* some resource values need to be reclaimed after use in Set/GetValues
   this data structure is used for a list of them
 */

typedef void (*Tm_FreeProc) _ANSI_ARGS_((char *));

typedef struct Tm_FreeResourceType {
    char *data;
    Tm_FreeProc free;
} Tm_FreeResourceType;


/*
 * The following structure defines all of the commands supported by
 * Tm, and the C procedures that execute them.
 */

typedef struct Tm_Cmd {
    char *name;			/* Name of command. */
    int (*cmdProc) _ANSI_ARGS_((ClientData clientData, Tcl_Interp *interp,
	    int argc, char **argv));
				/* Command procedure. */
    int (*widgetCmdProc) _ANSI_ARGS_((ClientData clientData, Tcl_Interp *interp,
	    int argc, char **argv));
				/* Widget Command procedure. */
} Tm_Cmd;

extern Tm_Cmd Tm_Commands[];       /* the commands for Motif widgets */
extern Tm_Cmd Tm_ExternCommands[]; /* the commands for extension widgets */


/* strings for use in widget create commands */
#define xmArrowButton		"xmArrowButton"
#define xmBulletinBoard		"xmBulletinBoard"
#define xmBulletinBoardDialog	"xmBulletinBoardDialog"
#define xmCascadeButton		"xmCascadeButton"
#define xmCommand		"xmCommand"
#define xmDialogShell		"xmDialogShell"
#define xmDrawingArea		"xmDrawingArea"
#define xmDrawnButton		"xmDrawnButton"
#define xmErrorDialog		"xmErrorDialog"
#define xmFileSelectionBox	"xmFileSelectionBox"
#define xmFileSelectionDialog	"xmFileSelectionDialog"
#define xmForm			"xmForm"
#define xmFormDialog		"xmFormDialog"
#define xmFrame			"xmFrame"
#define xmInformationDialog	"xmInformationDialog"
#define xmLabel			"xmLabel"
#define xmList			"xmList"
#define xmMainWindow		"xmMainWindow"
#define xmMenuBar		"xmMenuBar"
#define xmMessageBox		"xmMessageBox"
#define xmMessageDialog		"xmMessageDialog"
#define xmOptionMenu		"xmOptionMenu"
#define xmPanedWindow		"xmPanedWindow"
#define xmPopupMenu		"xmPopupMenu"
#define xmPromptDialog		"xmPromptDialog"
#define xmPulldownMenu		"xmPulldownMenu"
#define xmPushButton		"xmPushButton"
#define xmQuestionDialog	"xmQuestionDialog"
#define xmRowColumn		"xmRowColumn"
#define xmScale			"xmScale"
#define xmScrollBar		"xmScrollBar"
#define xmScrolledList		"xmScrolledList"
#define xmScrolledText		"xmScrolledText"
#define xmScrolledWindow	"xmScrolledWindow"
#define xmSelectionBox		"xmSelectionBox"
#define xmSelectionDialog	"xmSelectionDialog"
#define xmSeparator		"xmSeparator"
#define xmText			"xmText"
#define xmTextField		"xmTextField"
#define xmToggleButton		"xmToggleButton"
#define xmTopLevelShell		"topLevelShell"
#define xmWarningDialog		"xmWarningDialog"
#define xmWorkingDialog		"xmWorkingDialog"

#if XmVERSION >= 2
#define xmComboBox		"xmComboBox"
#define xmContainer		"xmContainer"
#define xmCSText		"xmCSText"
#define xmDropDownComboBox	"xmDropDownComboBox"
#define xmDropDownList		"xmDropDownList"
#define xmIconGadget		"xmIconGadget"
#define xmNotebook		"xmNotebook"
#define xmSpinBox		"xmSpinBox"
#endif

/* this is used to link the string used in widget creation to the
 * widget class
 */
typedef struct Tm_CommandToClassType {
    char *command;
    WidgetClass *clas;
} Tm_CommandToClassType;

extern Tm_CommandToClassType Tm_CommToClass[];		/* Motif widgets */
extern Tm_CommandToClassType Tm_ExternCommandToClass[];	/* external widgets */

/* mapping between reasons in callback call_data and the strings
   that tclMotif would return in %reason substitutions
 */
typedef struct Tm_ReasonType {
    int reason;
    char *str;
} Tm_ReasonType;

#endif /* _TM_H */
