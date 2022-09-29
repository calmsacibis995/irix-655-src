/**************************************************************************
 *									  *
 *		Copyright ( C ) 1990, 1991, Silicon Graphics, Inc.	  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ifndef IDEV_H
#define	IDEV_H

	/*
	 * This header file includes some documentation for application
	 * writers and people supporting input devices.
	 */

/***====================================================================***/
/***			DEFINES TO INTERPRET SHMIQ EVENTS		***/
/***====================================================================***/

#define	QE_PTR_EVENT	0
#define QE_VAL_EVENT	1
#define QE_BTN_EVENT	2
#define	QE_PROX_EVENT	3

	/* bits in flags for pointer events (QE_RESPONSE, QE_GEN_PTR_X,
	 * and QE_GEN_PTR_Y are also used).
	 */
#define	QE_X_CLAMPED	0x04
#define	QE_Y_CLAMPED	0x08

	/* bits in flags for valuator events */
#define QE_MORE_EVENTS	0x01	/* set if more val changes from this device are
				   coming. */
#define QE_RESPONSE	0x02	/* set if this event is the result of a 
				   set or change valuator call. */
#define QE_CLAMPED	0x04	/* set if the valuator is clamped to */
				/* its minimum or maximum value */

	/* bits in flags for button events */
#define QE_BTN_DOWN	0x01	/* set in flags if button was pressed */

	/* bits in flags for proximity events */
#define QE_PROX_IN	0x01	/* set if sensor moved into proximity */



#define	IDEVGETDEVICEDESC	_IOWR('i',0,idevDesc)
#define	IDEVGETVALUATORDESC	_IOWR('i',1,idevGetSetValDesc)
#define	IDEVGETKEYMAPDESC	_IOWR('i',2,idevKeymapDesc)
#define	IDEVGETSTRDPYDESC	_IOWR('i',3,idevStrDpyDesc)
#define	IDEVGETINTDPYDESC	_IOWR('i',4,idevIntDpyDesc)

#define	IDEVGETBUTTONS		_IOR('i',5,IDEV_MASK_SIZE)
#define	IDEVGETVALUATORS	_IOR('i',6,idevValuatorState)
#define	IDEVGETLEDS		_IOR('i',7,IDEV_MASK_SIZE)
#define	IDEVGETSTRDPY		_IOR('i',8,idevStrDpyState)
#define	IDEVGETINTDPYS		_IOR('i',9,idevIntDpyState)

#define	IDEVENABLEBUTTONS	_IOW('i',10,idevBitVals)

#define	IDEVENABLEVALUATORS	_IOW('i',11,idevBitVals)
#define	IDEVSETVALUATORS	_IOW('i',12,idevValuatorState)
#define	IDEVCHANGEVALUATORS	_IOW('i',13,idevValuatorState)
#define	IDEVSETVALUATORDESC	_IOW('i',14,idevGetSetValDesc)

#define	IDEVSETLEDS		_IOW('i',15,idevBitVals)
#define	IDEVSETSTRDPY 		_IOW('i',16,idevStrDpyState)
#define	IDEVSETINTDPYS		_IOW('i',17,idevIntDpyState)
#define	IDEVRINGBELL		_IOW('i',18,idevBellSettings)

#define	IDEVKEYBDCONTROL	_IOWR('i',30,idevKeybdControl)
#define	IDEVPTRCONTROL		_IOWR('i',31,idevPtrControl)
#define	IDEVOTHERCONTROL	_IOWR('i',32,idevOtherControl)
#define	IDEVSETPTRMODE		_IOW('i',33,idevPtrMode)
#define	IDEVOTHERQUERY		_IOWR('i',34,idevOtherQuery)
#define	IDEVSETPTRBOUNDS	_IOWR('i',35,idevPtrBounds)
#define	IDEVSETPTR		_IOWR('i',36,idevPtrVals)
#define	IDEVSETTRANSFORM	_IOWR('i',37,idevGetSetTransform)
#define	IDEVGETTRANSFORM	_IOWR('i',38,idevGetSetTransform)

#define IDEVINITDEVICE          _IOW('i',51,unsigned int)


#define	IDEV_MAX_BUTTONS	256
#define	IDEV_MAX_VALUATORS	256
#define	IDEV_MAX_LEDS		256
#define	IDEV_MAX_STR_DPYS	256
#define	IDEV_MAX_INT_DPYS	256

/***====================================================================***/
/***                       GENERAL STRUCTURES                           ***/
/***====================================================================***/

	/*
	 *  WARNING:  Make sure that IDEV_MASK_SIZE is large
	 *	enough for the class with the most elements.
	 */
#define	IDEV_MASK_SIZE		((IDEV_MAX_BUTTONS+7)/8)
#define	idevOffset(n)		((n)/8)
#define	idevMask(n)		(1<<((n)&0x7))
#define	idevSize(n)		(((n)+7)/8)
#define	idevNth(im,n)		((im)[(n)/8]&(1<<((n)&0x7)))
#define	idevSetNth(im,n)	((im)[(n)/8]|= (1<<((n)&0x7)))
#define	idevClearNth(im,n)	((im)[(n)/8]&=~(1<<((n)&0x7)))
#define	idevClearAll(im)	bzero((im),sizeof (im))
#define	idevCopyAll(s,d)	bcopy((s),(d),IDEV_MASK_SIZE)

	/*
	 * An idevBitVals structure contains a mask bit and a
	 * value bit per element of any input class for an input
	 * device.   Use an idevBitVals to selectively change or
	 * query bit values.
	 */
typedef struct _idev_bit_vals {
	unsigned char	mask[IDEV_MASK_SIZE];
	unsigned char	value[IDEV_MASK_SIZE];
} idevBitVals;

/***====================================================================***/
/***               IOCTLS TO GET A DESCRIPTION OF A DEVICE              ***/
/***====================================================================***/

	/*
	 * The IDEVGETDEVICEDESC ioctl accepts and fills in an idevDesc, 
	 *      which describes the device as a whole.
	 *
	 * A device consists of several input and feedback classes 
	 *      each of which has 0 or more elements.  An idevDesc
	 *	contains a symbolic name for the type of the device
	 *	(usually one of the device classes defined by the X input 
	 *	extension) a symbolic name for the device, and a count of the 
	 *	number of elements of each input class on the device.
	 *
	 *      Legal input classes are:
	 *        Buttons
	 *        Valuators 
	 *      Legal feedback classes are:
	 *        LEDs
	 *        String Displays
	 *        Integer Displays
	 *        Bell
	 *
	 * In addition, an device has an optional name for the
	 *      keymap associated with its buttons.    If the
	 *      device has a named keymap, the hasKeymapField is
	 *	non-zero.
	 */
#define	IDEV_MAX_NAME_LEN	15
#define	IDEV_MAX_TYPE_LEN	15

typedef struct _idev_desc {
	char		devName[IDEV_MAX_NAME_LEN+1];
	char		devType[IDEV_MAX_TYPE_LEN+1];
	unsigned short	nButtons;
	unsigned short	nValuators;
	unsigned short	nLEDs;
	unsigned short	nStrDpys;
	unsigned short	nIntDpys;
	unsigned char	nBells;
	unsigned char	flags;
} idevDesc;
#define	IDEV_HAS_KEYMAP		0x01
#define IDEV_HAS_PROXIMITY	0x02
#define IDEV_HAS_PCKBD		0x04

	/*
	 * QUERY DEVICE COMPONENT DESCRIPTIONS:
	 * Description of the specified member of an input class.   If 
	 *    all elements in the class have the same characteristics,
	 *    the IDEV_SAME bit is turned on in the flags field of the
	 *    description.
	 * If the IDEV_SET_ALL is set when changing the description of
	 * a valuator, all valuators are changed.
	 */
#define	IDEV_SAME		(0x8000)
#define	IDEV_SET_ALL		(0x8000)

	/* 
	 * Use the IDEVGETVALUATORDESC ioctl to get the description of
	 *    the valuator specified by dialNum.   IDEVGETVALUATORDESC 
	 *    accepts and fills in an idevGetSetValDesc.  An axis must
	 *    report absolute values. The resolution field returns the 
	 *    resolution of the specified valuator in 'ticks per inch'
	 *    (whatever that means).   The minVal and maxVal fields contain 
	 *    the minimum and maximum values the device will return.   
	 * possibleModes can be one of
	 *    IDEV_ABSOLUTE, IDEV_RELATIVE, IDEV_EITHER
	 * mode is the current mode and can be one of:
	 *    IDEV_ABSOLUTE, IDEV_RELATIVE
	 */
typedef struct _idev_valuator_desc {
	unsigned	hwMinRes;
	unsigned	hwMaxRes;
	int		hwMinVal;
	int		hwMaxVal;
	unsigned char	possibleModes;
	unsigned char	mode;
	unsigned short  resolution;
	int		minVal;
	int		maxVal;
} idevValuatorDesc;

#define	IDEV_ABSOLUTE		0x0
#define	IDEV_RELATIVE		0x1
#define	IDEV_EITHER		0x2

#define	IDEV_SET_RESOLUTION	(0x01)
#define	IDEV_SET_MIN		(0x02)
#define	IDEV_SET_MAX		(0x04)
#define	IDEV_SET_MODE		(0x08)

typedef struct _idev_valuator_query {
	short			valNum;
	unsigned short		flags;
	idevValuatorDesc	desc;
} idevGetSetValDesc;


	/* 
	 * Use the IDEVGETKEYMAPDESC ioctl to get the symbolic name of the
	 *	default keymap for the specified device.  
	 *	IDEVGETKEYMAPDESC accepts and fills in an idevKeymapDesc.  
	 *	If the device does not have a keymap, name is "" and the
	 *      ioctl fails.   Otherwise, name returns the name
	 *	of the keymap.   The only keymap name currently defined
	 *	is "SGI Standard" which uses the default key mapping compiled
	 *	into the X server.   The X server uses any other name to
	 *	look up a file in /usr/lib/X11/input (e.g. "swedish" would
	 *	use the keymap specified in /usr/lib/X11/input/swedish.xkm).
	 */
#define IDEV_KEYMAP_NAME_LEN	15
typedef struct _idev_keymap_desc {
	char	name[IDEV_KEYMAP_NAME_LEN+1];
} idevKeymapDesc;

	/*
	 * Use the IDEVGETSTRDPYDESC ioctl to get a description of
	 *    the specified string display.   IDEVGETSTRDPYDESC accepts
	 *    and fills in an idevStrDpyDesc.   The maxLength field 
	 *    returns the maximum length of a displayed string in symbols.  
	 *    The symbolSet field returns a symbolic name for the set of 
	 *    symbols the string display can show, but no names for 
	 *    symbol sets are currently defined.   If all string displays 
	 *    in the device have the same length and symbol set,
	 *    the allSame field is non-zero;
	 */
#define	IDEV_STR_DPY_SYMBOLS_NAME_LEN	15
typedef	struct _idev_str_dpy_desc {
	unsigned short	dpyNum;
	unsigned short	maxLength;
	char		allSame;
	char	 	symbolSet[IDEV_STR_DPY_SYMBOLS_NAME_LEN+1];
} idevStrDpyDesc;

	/*
	 * Use the IDEVGETINTDPYDESC ioctl to get a description of
	 *    the specified integer display. IDEVGETINTDPYDESC accepts
	 *    and fills in an idevIntDpyDesc.   The allSame field is
	 *    non-zero if all integer displays on the device have the 
	 *    same characteristics.
	 *
	 * The minValue and maxValue fields specify the minimum and
	 *    maximum values the specified display can show.
	 * The resolution field specifies the number of digits the 
	 *    feedback can display.
	 */	
typedef struct _idev_int_dpy_desc {
	unsigned short	dpyNum;
	unsigned char	allSame;
	unsigned char	pad;
	int	minValue;
	int	maxValue;
	int	resolution;
} idevIntDpyDesc;

/***====================================================================***/
/***                   IOCTLS TO QUERY DEVICE STATE                     ***/
/***====================================================================***/

	/*
	 * Use the IDEVGETBUTTONS ioctl to query the state of the device 
	 *     buttons.   IDEVGETBUTTONS returns an idevMask.  Each 
	 *     bit in the bits field describes a key.  If the bit is
	 *     set, the key is down.  Use the idevNth macro
	 *     to check the state of a button.
	 *
	 * Use the IDEVGETVALUATORS ioctl to query the state of one or
	 * more device valutors.   IDEVGETVALUATORS accepts and fills
	 * in an idevValuatorState.   The "value" array returns the current 
	 * values for "nValuators" (max IDEV_VALUATOR_STATE_MAX) device 
	 * valuators, starting at firstValuator.
	 */

#define	IDEV_VALUATOR_STATE_MAX	8
typedef struct _idev_valuator_state {
	unsigned char	firstValuator;
	unsigned char	nValuators;
	unsigned short	pad;
	int		value[IDEV_VALUATOR_STATE_MAX];
} idevValuatorState;
#define	idevValInState(ps,v) (((ps)->firstValuator<=(v))&&\
				(((ps)->firstValuator+(ps)->nValuators)>(v)))

	/*
	 * Use the IDEVGETLEDS ioctl to query the state of the device LEDs.
	 * IDEVGETLEDS returns an idevMask structure.   Each bit in the
	 * bits field corresponds to a device LED; a '1' bit indicates that
	 * the LED is lit.   Use the idevNth macro to check the state of
	 * an LED
	 *
	 * Use the IDEVGETSTRDPY ioctl to query the state of a single
	 * device string display.   IDEVGETSTRDPY accepts and
	 * fills in an idevStrDpyState.   The nDpy field specifies
	 * the display to be read, and the string field returns the
	 * contents of the display.
	 * 
	 * The 'on' field is non-zero if the corresponding display is
	 * showing anything at all (on), or zero if the display is clear
	 * (off).
	 */

#define	IDEV_STR_DPY_MAX_LEN	62
typedef struct _idev_str_dpy_state {
	unsigned char	nDpy;
	unsigned char	on;
	unsigned short	str_len;
	unsigned short	string[IDEV_STR_DPY_MAX_LEN];
} idevStrDpyState;

	/*
	 * Use the IDEVGETINTDPYS ioctl to query the state of one or more
	 * device integer displays.   IDEVGETINTDPYS accepts and
	 * fills in an idevIntDpyState.   The "value" array returns 
	 * the currently displayed values for "nDpys" 
	 * (max IDEV_INTDPY_STATE_MAX) device integer displays, starting 
	 * at firstDpy.
	 * 
	 * The 'on' field contains a bit for each display whose state
	 * is described which indicates whether or not the corresponding 
	 * display is showing any value at all.   The least significant
	 * bit of 'on' describes firstDpy, bit 1 describes firstDpy+1 and
	 * so on.
	 */
#define	IDEV_INT_DPY_MAX	8
typedef struct _idev_int_dpy_state {
	unsigned char	firstDpy;
	unsigned char	nDpys;
	unsigned short	on;
	short		value[IDEV_INT_DPY_MAX];
} idevIntDpyState;


/***====================================================================***/
/***                   IOCTLS TO CHANGE DEVICE STATE                    ***/
/***====================================================================***/

	/*
	 * Use the IDEVENABLEBUTTONS ioctl to start/stop reporting 
	 * events from device buttons.  Use the IDEVENABLEVALUATORS 
	 * to start/stop reporting events from device valuators.
	 * All ioctls accept an idevBitVals structure.
	 *
	 * A '1' bit in the mask.bit field of the idevBitVals enables
	 * the corresponding valuator or button if the value.bit field
	 * is '1' or disables it if the value.bit field is '0'
	 * The enabled state of buttons or valuators whose value mask.bit 
	 * is '0' is not changed.
	 *
	 * Use the IDEVSETVALUATORS and IDEVCHANGEVALUATORS ioctls
	 * to change the current value of one or more device valuators.
	 * Both ioctls accept and fill in an idevValuator struct.
	 * The IDEVSETVALUATORS ioctl changes the current value of
	 * a valuator to the corresponding value in the values array.
	 * The IDEVCHANGEVALUATORS ioctl adds the members of the
	 * values array to the current location of the corresponding
	 * dials.
	 *
	 * Use the IDEVSETVALUATORDESC ioctl to change the minimum
	 * and maximum possible values or the valuator resolution.
	 * IDEVSETVALUATORDESC accepts an idevGetSetValDesc struct.
	 * If the IDEV_SET_MIN bit is set in the flags field of the
	 * idevValuatorDesc, the minimum bound of the nthVal valuator
	 * is changed.  IDEV_SET_MAX and IDEV_SET_MODE control whether
	 * or not the maximum bound or event mode are changed.
	 *
	 * Use the IDEVSETLEDS ioctl to turn LEDs on or off.   Both 
	 * ioctls accept an idevBitVals struct, which is declared above.   
	 * A '1' bit in the mask.bits field of the idevBitVals turns the 
	 * corresponding LED on (corresponding bit in value.bits is '1')
	 * off (corresponding bit in the value.bits field is '0').   A '0' 
	 * bit in the mask.bits field has no effect on the corresponding 
	 * LED.
	 * Use the idevClearAll, idevSetNth and idevClearNth macros 
	 * to turn bits on or off.   
	 *
	 * Use the IDEVSETSTRDPY ioctl to change the contents of a
	 * string display.   IDEVSETSTRDPY accepts an idevStrDpyState
	 * structure (described above).  If the 'on' field is zero,
	 * the display is cleared *even if 'string' contains text*.
	 *
	 * Use the IDEVSETINTDPYS ioctl to change the contents of one
	 * or more integer displays.   If a bit in the 'on' field
	 * which corresponds to a display is zero, that display is
	 * cleared.
	 *
	 * Use the IDEVRINGBELL ioctl to ring a device bell.  You
	 * can specify volume (1-100), pitch, and duration in
	 * milliseconds.   You have no guarantee that the device
	 * will comply, but it should do the best that it can.
	 */
typedef struct _idev_bell_settings {
	int		which;
	unsigned short	volume;
	unsigned short	pitch;
	unsigned int	duration;
} idevBellSettings;

	/*
	 * Use the IDEVKEYBDCONTROL ioctl to control click volume
	 * and key autorepeat (on/off, rate, which keys repeat).
	 * Only those values specified in the 'which' field are
	 * changed.
	 *
	 * Use the IDEVPTRCONTROL ioctl to control valuator acceleration,
	 * threshold, etc. for *all* valuators on the device.
	 * Only those values specified in the 'which' field are changed.
	 */
#define	IDEV_KC_SET_CLICK_VOL	0x01
#define	IDEV_KC_SET_REPEAT	0x02
#define	IDEV_KC_SET_REP_DELAY	0x04
#define	IDEV_KC_SET_REP_RATE	0x08
#define	IDEV_KC_SET_LEDS	0x10
#define	IDEV_KC_SET_KEY_REPEAT	0x20
#define	IDEV_KC_SET_KBD_LEDS	0x40
#define	IDEV_KC_SET_ALL		0x3f
#define	IDEV_KC_SET_ALL_X	0x33

#define	IDEV_KC_NUM_LOCK	0x01
#define	IDEV_KC_CAPS_LOCK	0x02
#define	IDEV_KC_SCROLL_LOCK	0x04
typedef	struct _idev_kbd_ctrl {
	unsigned short	which;
	unsigned char	clickVolume;
	unsigned char	repeatOn;
	unsigned char	repeatDelay;
	unsigned char	repeatRate;
	unsigned char	kbdleds_mask;
	unsigned char   kbdleds_value;
	unsigned	leds;
	idevBitVals	keyRepeat;
} idevKeybdControl;

#define	IDEV_PC_SET_ACCEL_N		0x01
#define	IDEV_PC_SET_ACCEL_D		0x02
#define	IDEV_PC_SET_ACCEL		0x03
#define	IDEV_PC_SET_THRESHOLD		0x04
#define	IDEV_PC_SET_MULTIPLIER		0x08
#define	IDEV_PC_SET_CURVE_FACTOR	0x10
#define	IDEV_PC_SET_CURVE_INFLECTION	0x20
#define	IDEV_PC_SET_ALL_X		0x07
#define	IDEV_PC_SET_ALL			0x3f

typedef struct _idev_ptr_ctrl {
	unsigned char	valNum;
	unsigned char	which;
	short		accelNumerator;
	short		accelDenominator;
	short		threshold;
	short		mult;
	short		curveFactor;
	short		curveInflection;
} idevPtrControl;

	/*
	 * Use the IDEVOTHERCONTROL ioctl to control device parameters
	 * specific to some device.   Examples of "other" controls might 
	 * include spaceball translation modes or tablet resolution controls.
	 * The IDEVOTHERCONTROL ioctl accepts an idevOtherControl structure.   
	 * The "name" field is a string (maximum length 15) which identifies 
	 * the type of control.   The contents of the "data" field 
	 * depend on the type of control, but must be null-terminated and
	 * at most 23 bytes.
	 *
	 * Use the IDEVOTHERQUERY ioctl to query device parameters.
	 * The IDEVOTHERQUERY ioctl accepts an idevOtherQuery structure.
	 * The "name" field is a string (maximum length 15) which 
	 * specifies the control to query.  On return, the data field
	 * contains the value requested.
	 */

#define	IDEV_CTRL_NAME_LEN	15
#define	IDEV_CTRL_DATA_LEN	23
typedef struct _idev_other_control {
	char	name[IDEV_CTRL_NAME_LEN+1];
	char	data[IDEV_CTRL_DATA_LEN+1];
} idevOtherControl,idevOtherQuery;


#define	IDEV_GEN_NON_PTR_EVENTS	0x2
#define	IDEV_GEN_PTR_X		0x10
#define	IDEV_GEN_PTR_Y		0x20
#define	IDEV_GEN_PTR_EVENTS	(IDEV_GEN_PTR_X|IDEV_GEN_PTR_Y)
#define	IDEV_GEN_ALL_EVENTS	(IDEV_GEN_PTR_EVENTS|IDEV_GEN_NON_PTR_EVENTS)
#define	IDEV_EXCLUSIVE		0x40
#define	IDEV_SILENT		0x80000
typedef struct _idev_ptr_mode {
	unsigned char	mode;
	unsigned char	pad;
	short	xAxis;
	short	yAxis;
} idevPtrMode;

#define	IDEV_PB_SET_MIN_X	0x01
#define	IDEV_PB_SET_MAX_X	0x02
#define	IDEV_PB_SET_MIN_Y	0x04
#define	IDEV_PB_SET_MAX_Y	0x08
typedef	struct _idev_ptr_bounds {
	unsigned	which;
	short		minX,maxX;
	short		minY,maxY;
} idevPtrBounds;

#define	IDEV_PTR_SET_X		0x10
#define	IDEV_PTR_SET_Y		0x20
#define	IDEV_PTR_SET		(IDEV_PTR_SET_X|IDEV_PTR_SET_Y)
#define	IDEV_PTR_SET_X_VAL	0x40
#define	IDEV_PTR_SET_Y_VAL	0x80
#define	IDEV_PTR_SET_VALS	(IDEV_PTR_SET_X_VAL|IDEV_PTR_SET_Y_VAL)
#define	IDEV_PTR_SET_ALL	(IDEV_PTR_SET|IDEV_PTR_SET_VALS)
typedef struct _idev_ptr_vals {
	unsigned	which;
	short		x;
	short		y;
} idevPtrVals;

	 /*
	 * The IDEVINITDEVICE ioctl initializes the input device.
	 * You should initialize whenever you open the device.
	 */

	/*
	 * possible values for 'possible' and 'which'
	 *    IDEV_ACCEL -- Accelerate (scale delta by numerator/denominator 
	 *		    above threshold).
	 *    IDEV_SCALE -- scale absolute value by numerator/denominator
	 *    IDEV_CURVE -- apply curve to delta before accelerating
	 *
	 */
#define	IDEV_ACCEL		1
#define	IDEV_SCALE		2
#define	IDEV_CURVE		4

	/*
	 * flags
	 *    IDEV_INVERT -- invert delta or subtract absolute value from
	 *	max.
	 */
#define	IDEV_INVERT		(1<<0)
typedef struct idev_transform {
	unsigned char	possible;
	unsigned char	which;
	unsigned short	flags;
	short		numerator;
	short		denominator;
	unsigned short	m1;
	unsigned short	cM;
	unsigned short	inflection;
	unsigned short	threshold;
} idevTransform;

#define	IDEV_VT_SET_NUMERATOR		0x0001
#define	IDEV_VT_SET_DENOMINATOR		0x0002
#define	IDEV_VT_SET_THRESHOLD		0x0004
#define	IDEV_VT_SET_MULTIPLIER		0x0008
#define	IDEV_VT_SET_CURVE_FACTOR	0x0010
#define	IDEV_VT_SET_CURVE_INFLECTION	0x0020
#define	IDEV_VT_SET_ALL_X		0x0007
#define	IDEV_VT_SET_ALL			0x003f

#define	IDEV_VT_SET_TRANSFORM		0x8000
#define	IDEV_VT_SET_FLAGS		0x4000
#define	IDEV_VT_PTR_X			0x0100
#define	IDEV_VT_PTR_Y			0x0200
#define	IDEV_VT_SET_PTR			0x0300
#define	IDEV_VT_SET_PTR_AND_VAL		0x0400
typedef struct _idev_get_set_transform {
	unsigned short	which;
	unsigned short	valNum;
	idevTransform	transform;
} idevGetSetTransform;

#ifdef _KERNEL
	/*
	 * Helper functions and structures so that device 
	 * streams modules don't have to have tons of almost 
	 * identical functions.
	 */

typedef struct idev_info idevInfo;

	/*
	 * An idevShmiqInfo structure contains everything a streams
	 * fuction needs to know about a shmiq device.
	 */
typedef struct idev_shmiq_info {
	struct queue		*rq;
	struct queue		*wq;
	struct shmiqlinkid	 shmiqid;
	void	(*readData)(idevInfo *info, char *str, int  len);
	int	(*writeIoctl)(idevInfo *info, int cmd,
				int size, char *data, int *found);
} idevShmiqInfo;

	/*
	 * An idevValInfo stucture contains everything a streams function
	 * needs to know about the valuators of a device.  Each array
	 * in an idevValInfo has 'nVal' elements.
	 */

typedef struct idev_dev_val_info {
	int	 		 nVal;
	int			*sysValue;
	idevValuatorDesc	*desc;
	idevTransform		*transform;
	unsigned char		*active;

	unsigned char 		 mode;
} idevValInfo;

	/*
	 * An idevPtrInfo stucture contains everything a streams function
	 * needs to know to turn valuator data into pointer events.
	 */
typedef struct idev_dev_ptr_info {
	short			 xAxis,yAxis;
	short			 minX,maxX;
	short			 minY,maxY;
	short			 x,y;
	idevTransform		 yTransform;
	idevTransform		 xTransform;
} idevPtrInfo;

	/*
	 * An idevBtnInfo stucture contains everything a streams function
	 * needs to know about the buttons of a device.  Each array
	 * in an idevBtnInfo has 'nBtn' elements.
	 */
typedef struct idev_dev_btn_info {
	int		 nBtn;
	unsigned char	*state;
	unsigned char	*active;
} idevBtnInfo;

	/*
	 *  An idevInfo structure contains fields that are common
	 *  to all "idev" devices.
	 */
struct idev_info {
	idevShmiqInfo	sInfo;
	idevBtnInfo	bInfo;
	idevValInfo	vInfo;
	idevPtrInfo	pInfo;
	struct termio	*settings;
};

	/* 
	 * Given a queue, information a device and an idevValuatorState, 
	 * idevGenValEvents formats and generates shmiq events.  
	 *
	 * If the valuator mode is IDEV_GEN_PTR_EVENTS, idevGenValEvents
	 * will generate events of type IDEV_PTR_EVENT only. IDEV_PTR_EVENT
	 * events contain a button mask with up to five buttons, an X
	 * coordinate and a Y coordinate.
	 *
	 * If the valuator mode is IDEV_GEN_NON_PTR_EVENTS, idevGenValEvents
	 * generates events of type IDEV_VAL_EVENT only.   If the valuator
	 * mode is IDEV_GEN_ALL_EVENTS, idevGenValEvents generates events of
	 * type IDEV_PTR_EVENT for the axes which control the cursor and
	 * IDEV_VAL_EVENT events for all axes.   If the IDEV_EXCLUSIVE bit
	 * of the mode is set, idevGenValEvents does not generate valuator
	 * events for the axes which control the cursor.
	 *
	 * All shmiq valuator events use absolute coordinates.  The
	 * sysValues field in vInfo should contain the current absolute
	 * values for each axis.   The idevGenValEvents routine updates
	 * vInfo->sysValues with the information from vNew.
	 *
	 * The idevGenValEvents routine clamps returned valuator positions 
	 * to the min and max values specified in vInfo->desc.  The 
	 * idevGenValEvents routine sets the QE_CLAMPED bit in the shmiq 
	 * events for any axes that it clamps.
	 *
	 * 'flags' can contain any combination of QE_RESPONSE, QE_MORE_EVENTS,
	 * IDEV_VALS_ABSOLUTE, and IDEV_GENERATE_ALL.   
	 * If the QE_RESPONSE bit is set in 'flags', idevGenValEvents sets
	 * QE_RESPONSE in the flags field of all generated events.   
	 * 
	 * If IDEV_VALS_ABSOLUTE is set in 'flags' idevGenValEvents uses
	 * the values in vNew as absolute valuator positions, 
	 * otherwise they are added to the previous valuator position.   
	 *
	 * Normally, idevGenValEvents generates events only for axes that 
	 * change.   If IDEV_GENERATE_ALL is set, idevGenValEvents generates 
	 * events for all axes that are specified in vNew and active 
	 * in vInfo->active.
	 *
	 * Normally, idevGenValEvents sets the QE_MORE_EVENTS for all
	 * generated valuator events save the last.   If the QE_MORE_EVENTS
	 * bit is set in 'flags,' idevGenValEvents sets QE_MORE_EVENTS
	 * in the last field too.
	 *
	 * idevGenValEvents returns zero if it fails, non-zero if it
	 * succeeds.  If any of the values were clamped, it sets the 
	 * QE_CLAMPED bit in the return value.
	 *
	 * idevGenValEvent is similar to idevGenValEvents, but it
	 * generates a single event from an axis and value that are
	 * passed in.
	 */
#define	IDEV_VALS_ABSOLUTE	((unsigned)0x0100)
#define	IDEV_GENERATE_ALL	((unsigned)0x0200)
#define	IDEV_NO_TRANSFORM	((unsigned)0x0400)
extern	int	idevGenValEvents(
			idevInfo		*pInfo,
			idevValuatorState	*vNew,
			unsigned		 flags);

extern	int	idevGenValEvent(idevInfo	*pInfo,
			int			 axis,
			int			 value,
			unsigned 		 flags);

	/* 
	 * idevSetPtrControl sets pointer acceleration parameters for the
	 * specified device.
	 * idevSetPtr sets ointer acceleration, threshold, multiplier,
	 * curve factor and curve inflection for the specified device.
	 *
	 */
extern	idevTransform idevDfltAccel;
extern	idevTransform idevDfltScale;
extern	int	idevSetPtrCtrl(idevInfo *pInfo,idevPtrControl *ctrl);
extern	int	idevSetPtr(idevInfo *pInfo, idevPtrVals *ctrl);

	/*
	 * idevGetValDesc queries the min value, max value or resolution of the 
	 * specified axis.
	 * idevGetValState queries the current system value for the specified
	 * axes.
	 */
extern	int	idevGetValDesc(idevInfo *pInfo, idevGetSetValDesc *axis);
extern	int	idevGetValState(idevInfo *pInfo, idevValuatorState *vals);

	/*
	 * Changes the min value, max value or resolution of the 
	 * specified axes.
	 *
	 * If the current value of some axis is outside of the
	 * new range, idevSetValDesc clamps the event into the
	 * new range and generates an event with the QE_RESPONSE
	 * bit set.
	 */
extern	int	idevSetValDesc(idevInfo	*pInfo,idevGetSetValDesc *new);

extern	int	idevGetTransform(idevInfo *pInfo,idevGetSetTransform *trans);
extern	int	idevSetTransform(idevInfo *pInfo,idevGetSetTransform *trans);

	/*
	 * Given a queue, information about device buttons, and 
	 * a button to change, idevGenBtnEvent formats and generates
	 * a shmiq events if the button is active.
	 * If the IDEV_BTN_PRESS bit is set in flags, idevGenBtnEvent
	 * generates a down event, otherwise it generates an up event.
	 * 
	 * idevGenBtnEvent checks the current state of the button
	 * before generating an event.   If the new event is a
	 * state change, idevGenBtnEvent generates the event normally.
	 *
	 * If the new event is *not* a state change, other bits
	 * in flags control event generation.
	 * If the IDEV_FAKE_EVENT bit is set, idevGenBtnEvent fakes
	 * an event to ensure that the new event is a state change.
	 * If the IDEV_FORCE_EVENT bit is set, idevGenBtnEvent generates
	 * the new event despite the fact that there is no state change.
	 * If neither bit is set, idevGenBtnEvent generates no event.
	 * Example:   button 1 is down in bInfo->state, and the device
	 *	module calls idevGenBtnEvent for a button 1 press.
	 * If IDEV_FAKE_EVENT is set in flags, idevGenBtnEvent generates
	 *	"button 1 up" followed by "button 1 down."
	 * If IDEV_FORCE_EVENT is set in flags, idevGenBtnEvent generates
	 *	"button 1 down."
	 * If neither bit is set, idevGenBtnEvents doesn't generate
	 *	an event.
	 */
#define	IDEV_BTN_PRESS		0x01
#define	IDEV_FAKE_EVENT		0x80
#define	IDEV_FORCE_EVENT	0x40

extern	int	idevGenBtnEvent(idevInfo	*pInfo,
			unsigned		 btnNum,
			int			 flags);

extern	int	idevGenBtnEvents(idevInfo	*pInfo,
			unsigned char		*newMask,
			unsigned char		*newState);

	/* 
	 * The idevSetPtrMode function changes the pointer
	 * mode and pointer axes of the device.
	 * The idevSetPtrBounds function changes the width and
	 * height (maximum X and Y) for QE_PTR_EVENTs from  a
	 * device.
	 */
extern	int	idevSetPtrMode(idevInfo *pInfo,idevPtrMode *pMode);
extern	int	idevSetPtrBounds(idevInfo *pInfo,idevPtrBounds *pBounds);

	/*
	 * Given a length (in bytes), a bit array to update and and an 
	 * idevBitVals, idevUpdateBitArray changes any bits specified in 
	 * new->mask to the corresponding state specified in old->value.
	 */
extern	int	idevUpdateBitArray(int nBytes,char *old, idevBitVals *new);

	/* 
	 * Given information about a device and a termio with the correct
	 * line settings, idevChangeLineSettings issues an ioctl to 
	 * change the line settings as appropriate.
	 */
extern	int	idevChangeLineSettings(idevInfo *pInfo,struct termio *tio);

extern	int	idev_rput( register queue_t *rq, mblk_t *mp );
extern	int	idev_wput( register queue_t *wq, register mblk_t *mp );

#endif

#endif /* IDEV_H */
