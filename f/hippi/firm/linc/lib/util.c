/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/


/*
 * util.c
 *
 * $Revision: 1.4 $
 */

#include <sys/types.h>

#include "lincutil.h"
#include "hip_errors.h"
#include "sys/PCI/linc.h"

/* Very crude sprintf(). */
void
vasprintf( char *s, char *fmt, long *arg0p )
{
	long	*argp = arg0p, val, temp;
	char	*s2;

	while ( *fmt != '\0' )
	   if ( *fmt != '%' )
		*s++ = *fmt++;
	   else
		switch ( *++fmt ) {
		case '%':
			*s++ = *fmt++;
			break;
		case 'c':
			fmt++;
			*s++ = (char) *( argp++ );
			break;
		case 's':
			fmt++;
			s2 = (char *) *( argp++ );
			while ( *s2 )
				*s++ = *s2++;
			break;
		case 'd':
			fmt++;
			val = *( argp++ );
			temp = val;
			if ( val == 0 ) {
				*s++ = '0';
				break;
			}
			else if ( val < 0 ) {
				*s++ = '-';
				val = -val;
			}
			while ( val > 0 ) {
				s++;
				val /= 10;
			}

			s2=s-1;

			val = temp;
			if ( val < 0 )
				val = -val;
			while ( val > 0 ) {
				*s2-- = '0' + val % 10;
				val /= 10;
			}
			break;
		case 'x':
			fmt++;
			val = *( argp++ );
			for (temp=0; temp<sizeof(long)*2; temp++) {
				int	digit =
					 (u_long)val>>(sizeof(long)*8-4);
				if ( digit > 9 )
					*s++ = 'A'+digit-10;
				else
					*s++ = '0'+digit;
				val <<= 4;
			}
			break;
		default:
			*s++ = '%';
			*s++ = *fmt++;
			*s++ = '?';
			break;
		}


	*s = '\0';
}

void
sprintf( char *s, char *fmt, ... )
{
	vasprintf( s, fmt, (long *) (&fmt + 1) );
}

int
strcmp( const char *s1, const char *s2 )
{
	while ( *s1 && *s2 && *s1 == *s2 )
		s1++,s2++;
	return ( ! (*s1 == '\0' && *s2 == '\0') );
}

char*
strcpy( char *s1, const char *s2 )
{
	char *s = s1;
	while ( *s1++ = *s2++ )
		;
	return s;
}

void
delayncycles(int n)
{
	__uint32_t	x;

	x = get_r4k_count();
	while ( (int)(get_r4k_count()-x) < n )
		;
}

void
bzero( void *dst, int length )
{
	if ( ((u_long)dst & 3) == 0 && (length & 3) == 0 ) {
		while ( length > 0 ) {
			*(u_int *)dst = 0;
			dst = (void *)( (u_long)dst + 4 );
			length -= 4;
		}
	}
	else {
		while ( length > 0 ) {
			*(u_char *)dst = 0;
			dst = (void *)( (u_long)dst + 1 );
			length--;
		}
	}
}

void
bcopy( const void *src, void *dst, int length )
{
	if ( ((u_long)dst & 3) == 0 && ((u_long)src & 3) == 0 &&
	     		(length & 3) == 0 ) {
		while ( length > 0 ) {
			*(u_int *)dst = *(u_int *)src;
			dst = (void *)( (u_long)dst + 4 );
			src = (void *)( (u_long)src + 4 );
			length -= 4;
		}
	}
	else {
		while ( length > 0 ) {
			*(u_char *)dst = *(u_char *)src;
			dst = (void *)( (u_long)dst + 1 );
			src = (void *)( (u_long)src + 1 );
			length--;
		}
	}
}

/************************************************************
  wait_usec
************************************************************/

void
wait_usec(int i)  {
    int timer;
    /* wait i usec */
    timer_set(&timer);
    while ( !timer_expired(timer, i))
        ;
}


void
blink_error_leds(int mode, int major, int minor) {
    u_int mode_led;
    int j;

    if (mode == HIP_SIGN_ASSFAIL) {
        mode = HIP_SIGN_CDIE;
	major = CDIE_ASSFAIL;
    }

    if (mode == HIP_SIGN_CDIE)
        mode_led = 0;
    else
        mode_led = 8;

    /* reset leds, clear leds for a long time, set all error leds for a 
     * long time, clear them, and then start winking error sequence.
     */

    LINC_WRITEREG(LINC_LED,  ~0x0); /* off */
    wait_usec(2*ERROR_BLINK_TIMER); /* 1 sec */

    LINC_WRITEREG(LINC_LED,  ~0xf); /* on */
    wait_usec(4*ERROR_BLINK_TIMER); /* 2 sec */
	  
    LINC_WRITEREG(LINC_LED,  ~0x0); /* off */
    wait_usec(2*ERROR_BLINK_TIMER); /* 1 sec */

    /* start with mode by itself */
    LINC_WRITEREG(LINC_LED, ~mode_led);
    wait_usec(ERROR_BLINK_TIMER); /* 1/2 sec */


    /* blink LEDS with error code */
    /* wait a min on/off time */
    for (j = 0; j < 16; j++) {
        u_int error_led1 = ((major - j) > 0) ? 1 : 0;
	u_int error_led0 = (((0xf & minor) - j) > 0) ? 1 : 0;
      
	LINC_WRITEREG(LINC_LED, 
		      ~(mode_led |(error_led1<<2) | error_led0));
	wait_usec(ERROR_BLINK_TIMER);

	LINC_WRITEREG(LINC_LED, ~mode_led);
	wait_usec(ERROR_BLINK_TIMER);

	if (error_led1 == 0 && error_led0 == 0)
	    break; 

	
    }
}
