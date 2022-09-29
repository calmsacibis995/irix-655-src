#pragma once

// $Revision: 1.18 $
// $Date: 1992/10/30 09:26:10 $
#include "tkValue.h"
#include "Box2.h"

class tkEvent;
typedef void(*Fcnptr)( tkEvent* );

class tkEvent : public tkObject {

/* tkEvent
 * tkEvents are generated in two ways. First, a converter translates
 * outside events into tkEvents. Second, toolkit objects generate tkEvents.
 * tkEvents are type-coded, with a number that uniquely defines the
 * type of tkEvent. The name space for tkEvent types is in events.h.
 * By default, tkEventManager handles and distributes tkEvents.
 */

protected:
	tkEvent*	nextEvent;		// used by event manager
	long		eventName;		/* event name; see events.h */
	tkObject	*clientName;		/* client of this interest */
	tkObject 	*senderName;		/* the sender of this event */
	friend class 	tkEventManager;

public:
	tkEvent(long na = 0, tkObject* cl = 0, tkObject* s = 0)
	    { eventName = na; clientName = cl; senderName = s; }
	~tkEvent() {};

	long name() { return eventName; }
	tkObject* client() { return clientName; }
	tkObject* sender() { return senderName; }
	void setName(long en) { eventName = en; }
	void setClient(tkObject *cl) { clientName = cl; }
	void setSender( tkObject* s ) { senderName = s; }


	/*
	 * Examine an event, and see if our client wants to receieve it.
	 * The implication here is that "this" object is an expression of
	 * some clients interest in an event.  This method is responsibile
	 * for performing the necessary discrimination of event, passing
	 * on to its client those events that are deemed interesting.
	 */
	virtual void triggerEvent(tkEvent* event);
	virtual const char* className();
};

/*
 * Window event.  Window events are focused to a particular window, and
 * remain within the scope of the window.
 */
class tkWindowEvent : public tkEvent {
protected:
	tkGID	gid;			/* window number */

public:
	tkWindowEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID);
	~tkWindowEvent() {};

	tkGID windowNumber() { return gid; }
	void setWindowNumber(tkGID wn) { gid = wn; }

	virtual void triggerEvent(tkEvent* event);
};

/*
 * Window event.  Window events are focused to a particular window, and
 * remain within the scope of the window.
 */
class tkNumberEvent : public tkEvent {
protected:
	int	number;			/* unique number */

public:
	tkNumberEvent(long na = 0, tkObject* cl = 0, int n = -1);
	~tkNumberEvent() {};

	int getNumber() { return number; }
	void setNumber(int n) { number = n; }

	virtual void triggerEvent(tkEvent* event);
};

/*
 *  FcnCall Event, like the regular events, but they carry a function
 *  pointer in them.  The function gets called when they are triggered.
 */
class tkFcnCallEvent : public tkEvent {
protected:
    Fcnptr	fcn;
public:
    tkFcnCallEvent( long na = 0, tkObject* cl = 0, Fcnptr f = 0 )
		  : tkEvent (na, cl)
	{ fcn = f; }
    ~tkFcnCallEvent() {};
    
    void(*getFcn())( tkEvent* ) { return fcn; } // yeee-ha!
    void setFcn( Fcnptr f )
	{ fcn = f; }
    virtual void triggerEvent( tkEvent* );

    virtual const char* className();
};


/*
 *  Value Event, like the regular events, but there is a tkValue
 *  associated with them.
 */

class tkValueEvent : public tkWindowEvent {
protected:
    tkValue	val;
public:
    tkValueEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID, tkValue* v = 0);
    ~tkValueEvent();

    void getValue( tkValue& v ) { v = val; }
    void getValue( tkValue* v ) { *v = val; }
    void getValue( Bool* b ) { val.getValue( b ); }
    void getValue( int* i )     { val.getValue( i ); }
    void getValue( double* d )  { val.getValue( d ); }
    void getValue( CharString* s ) { val.getValue( s ); }
    void getValue( char* s) { val.getValue( s ); }

    void setValue( const tkValue& v )	{ val = v; }
    void setValue( tkValue* v )	{ val = *v; }
    void setValue( int i )	{ val = i; }
    void setValue( double d )	{ val = d; }
    void setValue( CharString& s )	{ val = s; }
    void setValue( CharString* s )	{ val = s; }
    void setValue( char* s )		{ val = s; }

    virtual const char* className();
};

/*
 *  NumberValue Event, like the regular number event, but there is a tkValue
 *  associated with them.
 */

class tkValueNumberEvent : public tkNumberEvent {
protected:
    tkValue	val;
public:
    tkValueNumberEvent(long na = 0, tkObject* cl = 0, int n = -1, tkValue* v = 0);
    ~tkValueNumberEvent();

    void getValue( tkValue& v ) { v = val; }
    void getValue( tkValue* v ) { *v = val; }
    void getValue( Bool* b ) { val.getValue( b ); }
    void getValue( int* i )     { val.getValue( i ); }
    void getValue( double* d )  { val.getValue( d ); }
    void getValue( CharString* s ) { val.getValue( s ); }
    void getValue( char* s) { val.getValue( s ); }

    void setValue( const tkValue& v )	{ val = v; }
    void setValue( tkValue* v )	{ val = *v; }
    void setValue( int i )	{ val = i; }
    void setValue( double d )	{ val = d; }
    void setValue( CharString& s )	{ val = s; }
    void setValue( CharString* s )	{ val = s; }
    void setValue( char* s )		{ val = s; }

    virtual const char* className();
};

/*
 * Motion event.
 */
class tkMotionEvent : public tkWindowEvent {
protected:
	short	xCoord;
	short	yCoord;
	short	butState;

public:
	tkMotionEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID)
		: tkWindowEvent (na,cl,wn) { xCoord = yCoord = butState = 0; }
	~tkMotionEvent(){};
	long getX() { return xCoord; }
	long getY() { return yCoord; }
	void getXY(Point2& result)
	    { result.x = xCoord; result.y = yCoord; }
	short getButState() { return butState; }

	void setX(long x) { xCoord = (short) x; }
	void setY(long y) { yCoord = (short) y; }
	void setXY(long x, long y) { xCoord = (short) x; yCoord = (short) y; }

	void setButState( short s ) { butState = s; }
};

/*
 * Button event.
 */
class tkButtonEvent : public tkMotionEvent {
protected:
	short	device;
	short	state;

public:
	tkButtonEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID);
	~tkButtonEvent();
	short getDevice() { return device; }
	short getState() { return state; }

	void setDevice(short d) { device = d; }
	void setState(short s) { state = s; }

	virtual void triggerEvent(tkEvent* event);
};

enum tkKeyboardEventType {
    tkKeyboardKeyEvent,
    tkKeyboardExtendedKeyEvent,
    tkKeyboardStringEvent,
    tkKeyboardBreakEvent,
};

class tkKeyboardEvent : public tkWindowEvent {
protected:
	char*	buf;
	short	c;
	short	buflen;
	tkKeyboardEventType kind;

public:
	tkKeyboardEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID);
	~tkKeyboardEvent();

	void setSingleCharacter(unsigned char ch)
	    { kind = tkKeyboardKeyEvent; c = ch; }
	void setExtendedCharacter(short ch)
	    { kind = tkKeyboardExtendedKeyEvent; c = ch; }
	void setBreakEvent()
	    { kind = tkKeyboardBreakEvent; }
	void setMultipleCharacter(char const* data, int datalen);

	tkKeyboardEventType getTypeOfEvent()
	    { return kind; }

	unsigned char getSingleCharacter() { return c; }
	short getExtendedCharacter() { return c; }
	char* getBuffer() { return buf; }
	int getBufferLength() { return buflen; }
};

class tkTextViewEvent : public tkValueEvent {
protected:
	char terminator;
public:
    tkTextViewEvent(long na = 0, tkObject* cl = 0, tkGID wn = tkInvalidGID, 
		 tkValue* v = 0, char term = 0)
	: tkValueEvent (na,cl,wn,v)
	{ terminator = term; }
    ~tkTextViewEvent() {};

    char getterminator() { return terminator; }
    void setterminator(char val = 0) { terminator = val; }
    virtual const char* className();
};
