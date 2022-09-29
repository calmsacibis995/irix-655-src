#ifndef	_events_
#define	_events_

// $Revision: 1.25 $
// $Date: 1991/04/29 17:49:41 $

// Constants for event names
#define	tkEVENT_REDRAW			1
#define	tkEVENT_GAIN_FOCUS		2
#define	tkEVENT_LOSE_FOCUS		3
#define tkEVENT_REDRAWICONIC		4
#define tkEVENT_PIECECHANGE		5
#define tkEVENT_DEPTHCHANGE		6
#define tkEVENT_QUIT			7
#define tkEVENT_CALLBACK		8

#define	tkEVENT_MOTION			9
#define tkEVENT_KEYBD			10
#define	tkEVENT_SELECT_BUTTON		11
#define	tkEVENT_FUNCTION_BUTTON		12
#define	tkEVENT_MENU_BUTTON		13
#define tkEVENT_DOUBLE_CLICK		14

#define tkEVENT_SELECTED		15
#define tkEVENT_DESELECTED		16
#define tkEVENT_ACTION			17
#define tkEVENT_STILLSELECTED		18
#define tkEVENT_VALUECHANGE		19
#define tkEVENT_OPENED			20
#define tkEVENT_CLOSED			21

#define	tkEVENT_FILECHANGE		22
#define	tkEVENT_DIRCHANGE		23
#define	tkEVENT_STOW			24
#define	tkEVENT_UNSTOW			25

#define	tkEVENT_TIMER_EXPIRED		26

#define	tkEVENT_SAVE			27

#define tkEVENT_SHUT			28
#define	tkEVENT_WINDOW_GONE		29

// toolkit bus events

// This event is sent when the cut buffers contents change
// there is no value
#define	tkEVENT_CUT_BUFFER_CHANGED	30

// This event is sent when a new double click value has been
// picked.  The value is the new double click rate
#define	tkEVENT_DOUBLE_CLICK_CHANGED	31

// This event is sent when keyboard clicking is changed via the control
// panel. The value is either 0 for off, or non-zero for on.
#define	tkEVENT_KEY_CLICK_CHANGED	32

// This event is sent when the blank time is changed via the control
// panel. The value is the new blank time, in seconds
#define	tkEVENT_BLANK_TIME_CHANGED	33

// This event is sent when the mouse warp is changed via the control
// panel.  The value is not used.
#define	tkEVENT_MOUSE_WARP_CHANGED	34

// This event is sent when the system date/time is changed via the control
// panel. The value is a unix time_t (see manual page time(2))
#define	tkEVENT_SYSTEM_TIME_CHANGED	35

// This event is sent when the gamma correction is changed via the control
// panel. The value is a floating point value.
#define	tkEVENT_GAMMA_CHANGED		36

// This event is used to notify the parentApp when a tkNotifier or tkPrompt has
// recieved a response.
#define NOTIFY_CLOSE			37

// This event should be used by any app wishing to delete the notifier
#define FLUSH_NOTIFY			38

// This event is used to notify the parentApp when a VhelpApp has recieved
// a close event.
#define HELP_CLOSE			39

#define tkEVENT_MID_BUTTON		40

#define tkEVENT_MID_DOUBLE_CLICK	41

// define user event from this point on, as in:
//
//	#define USER_EVENT_1		(tkUSER_EVENTBASE + 1)
#define tkUSER_EVENTBASE		20000
#define INFO_CLOSE			tkUSER_EVENTBASE + 2
#define tkEVENT_OPENINPLACE		tkUSER_EVENTBASE + 3
#define tkEVENT_CHILDCLOSE		tkUSER_EVENTBASE + 4
#define tkEVENT_REFRESH			tkUSER_EVENTBASE + 5
#define NEW_WORKSPACE			tkUSER_EVENTBASE + 6
#define NEW_DIRVIEW			tkUSER_EVENTBASE + 7
#define LONG_INFO_CLOSE			tkUSER_EVENTBASE + 8
#define SET_TRANSLATE			tkUSER_EVENTBASE + 9
#define SHORT_START			tkUSER_EVENTBASE + 10
#define LONG_START			tkUSER_EVENTBASE + 11
#define CHANGE_DEVICE			tkUSER_EVENTBASE + 12
#define UPDATE_INFO			tkUSER_EVENTBASE + 13
#define ALTOPEN				tkUSER_EVENTBASE + 14
#define LONG_INFO_NAME_EVENT    	tkUSER_EVENTBASE + 15
#define TRANSFER_EVENT			tkUSER_EVENTBASE + 16
#define CURS_EVENT			tkUSER_EVENTBASE + 17
#define MORE_FILES			tkUSER_EVENTBASE + 18
#define PREF_CLOSE			tkUSER_EVENTBASE + 19
#define APP_CLOSE			tkUSER_EVENTBASE + 20
#define FAM_DEATH			tkUSER_EVENTBASE + 21
#define FETCH				tkUSER_EVENTBASE + 22
#define FETCH_START			tkUSER_EVENTBASE + 23
#define FETCH_END			tkUSER_EVENTBASE + 24
#define tkEVENT_FAM_REFRESH		tkUSER_EVENTBASE + 25
#define tkEVENT_REDRAW_CHANGE		tkUSER_EVENTBASE + 26

#define POST_PROMPT                     tkUSER_EVENTBASE + 27

#define PRN_ADD_PRINTER                 tkUSER_EVENTBASE + 28
#define PRN_DELETE_PRINTER              tkUSER_EVENTBASE + 29
#define PRN_PRINT_QUEUE_BTN             tkUSER_EVENTBASE + 30
#define PRN_ACCEPT_REQ_BTN              tkUSER_EVENTBASE + 31
#define PRN_ADD_PRINTER_CONFIRMED       tkUSER_EVENTBASE + 32
#define PRN_DELETE_PRINTER_CONFIRMED    tkUSER_EVENTBASE + 33
#define PRN_PRINT_QUEUE_BTN_CONFIRMED   tkUSER_EVENTBASE + 34
#define PRN_PRINT_QUEUE_BTN_DENIED      tkUSER_EVENTBASE + 35
#define PRN_ACCEPT_REQ_BTN_CONFIRMED    tkUSER_EVENTBASE + 36
#define PRN_ACCEPT_REQ_BTN_DENIED       tkUSER_EVENTBASE + 37

#define EXTENDED_KEYBD_BIT		0x0100
#endif
