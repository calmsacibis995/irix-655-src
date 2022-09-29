/*
	Copyright 1995, Silicon Graphics, Inc.
*/

/*
	kona_diag.h	$Revision: 1.14 $	$Date: 1996/08/03 00:10:54 $

	Kona-specific diagnostic interface data structures.  These
	are parameters passed to the device driver from a user-level
	diagnostic thread through the kona-private section of
	gfxioctl() .

	A thread must first register itself as a diagnostic one
	by making a gfxioctl() call with GFX_SET_DIAG_FLAG .

	There are two functions provided by this interface:  memory
	mapping to allow DMA testing and flow control selection that,
	when activated, instructs the device driver to handle pipe flow
	control on behalf of the diagnostics test program.
*/
#ifndef _KONA_DIAG_H
#define _KONA_DIAG_H

#include <sys/types.h>

/*
   Diag ioctl numbers.  These are described in more detail
   below.  KONA_DIAG_IOCTL_BASE is defined in sys/kona.h
   but we don't put these actual ioctl numbers there so as to
   avoid cluttering operational code with diags stuff
*/   
#define KONA_DIAG_MAP			(KONA_DIAG_IOCTL_BASE + 0)
#define KONA_DIAG_FLOW			(KONA_DIAG_IOCTL_BASE + 1)
#define KONA_DIAG_CREDITS		(KONA_DIAG_IOCTL_BASE + 2)


/*
	KONA_DIAG_MAP -- provide virtual to physical/io address translation / management

	This ioctl provides the kernel management necessary to support
	pipe DMA tests.  It takes as its argument a pointer to a
	kona_diag_map struct.

	The user should pass in a page-aligned virtual address (addr) of
	a pre-allocated memory area, its length (length), and
	the desired operation (flags).

	In flags, the user must specify KONA_DIAG_MAP_CREATE xor
	KONA_DIAG_MAP_DESTROY and KONA_DIAG_MAP_PHYSICAL xor
	KONA_DIAG_MAP_IOVIRTUAL.  I.e. the user may request to

	Create a physical mapping
	Destroy a physical mapping
	Create an I/O virtual mapping
	Destroy an I/O virtual mapping

	For CREATE, the device driver will perform the necessary
	mapping operation and lock the area into physical memory.  The
	resultant mapped physical or I/O address will be returned in
	paddr or ioaddr as appropriate.  The diagnostic user should
	then program the HIP DMA input or output controller with this value.

	For IOVIRTUAL, the kernel, in addition to the above, will program
	the mapping tables of the virtual to physical mapping into the
	I/O mapping hardware (e.g. the F chip).

	For PHYSICAL, the physical address is simply returned.  N.B.
	that only one page may be mapped via this call because no
	scatter/gather-type operation is performed.

	DESTROY undoes the CREATE freeing the resources.  WARNING:
	the device driver keeps no record of the allocated resource
	so care must be taken to re-supply all parameters (addr,
	length, PHYSICAL or IOVIRTUAL) that was given to the
	original CREATE call.  This is bad for integrity, but the the
	diagnostic program already has enough privilege to trash the
	system since we are allowing him to directly program DMA.

	The ioctl returns -1 and sets errno if it could not perform
	the requested operation.  XXX enumerate error return values.

*/


typedef struct kona_diag_map_args {
	caddr_t	addr;	/* IN:  page aligned user memory address to be mapped 		    */
	uint_t	length;	/* IN:  byte length of memory to ma; if physical must be <= 1 page */
	uint_t	pnum;	/* OUT: page number to program in the PNUM register		    */
	uint_t	flags;	/* IN:  command flags as follows:				    */
	uint_t	poff;	/* OUT: page offset and other memory attributes			    */
} kona_diag_map_args_t;

/* Set of permissible values for kona_diag_map.flags	*/
#define KONA_DIAG_MAP_CREATE	1	/* Create & lock the mapping 	*/
#define KONA_DIAG_MAP_DESTROY	2	/* Destroy & unlock the mapping	*/
#define KONA_DIAG_MAP_PHYSICAL	4	/* return physical address 	*/
#define KONA_DIAG_MAP_IOVIRTUAL	8	/* return an I/O virtual addr	*/

#if _MIPS_SIM == _ABI64 &&  defined(_KERNEL)
typedef struct abi32_kona_diag_map_args {	/* 32 bit version of the above structure */
	app32_ptr_t	addr;
	uint_t		length;
	uint_t		pnum;
	uint_t		flags;
	uint_t		poff;
} abi32_kona_diag_map_args_t;
#endif


/*
	KONA_DIAG_FLOW -- enable / disable kernel support of pipe flow control.

	For diagnostics, flow control of the pipe may be operated in two ways.

	First and by default, the kernel does nothing with respect to flow
	control so that if the diagnostic process exceeds the capacity of the
	graphics fifo, additional data will simply be discarded.

	Second, if the diagnostic thread enables flow control by passing
	KONA_DIAG_FLOW_ON as the parameter to the KONA_DIAG_FLOW ioctl,
	the kernel will set up and handle fifo flow control on behalf
	of the thread so that it is free to write to the pipe without
	fear of overflowing the hardware.  Kernel flow control may
	again be deactivated by issuing the KONA_DIAG_FLOW_OFF command
	to the KONA_DIAG_FLOW ioctl.

	KONAinitialize (aka the GFX_INITIALIZE ioctl) must have been called
	before making this call.

	Possible errors:
		EACCES	-- pipe not alive (KONAinitialize not called).
		EINVAL	-- parameter not KONA_DIAG_FLOW_ON or KONA_DIAG_FLOW_OFF

	WARNING:  When flow control is requested, the kernel will

	o  enable the flow control interrupts by setting
	   INTR_BIT_FIFO_HI, INTR_BIT_FIFO_LO, and INTR_BIT_FIFO_OV in HH_INTR_ENABLE,

	o  set the global interrupt INTR_GLOBAL_EN in HH_INTR_INFO,

	o  set GINTR_VECTOR_MASK and GINTR_CPUID_MASK of HH_GINTR_INFO,

	o  set HH_GFIFO_HI and GFIFO_LO, and

	o  use HH_INTR_STATUS and HH_INTR_ACK.

	XXX is that all?

	If the diagnostic program changes these values while KONA_DIAG_FLOW is ON,
	flow control will fail.
*/

/* Set of permissible values for the KONA_DIAG_FLOW parameter (uint_t flow_control)	*/
#define KONA_DIAG_FLOW_OFF		0
#define KONA_DIAG_FLOW_ON		1

/*
	KONA_DIAG_CREDITS -- get gfifo credit counts 

	Diagnostics may wish to test the flow of gfx credits on systems
	that use credit-based flow control.

	If the current system does not use credit-based flow control,
	this IOCTL returns zero for the total number of credit bytes.
	All other fields are undefined (though typically zeroed).

	Otherwise, the total credit bytes equals the size of the gfx
	fifo in the adaptor between the host and the HIP.  This value
	does not change.  

	The bias credit bytes is equal to the amount of gfx skid
	in the current CPU.  Any time the number of host-side credits
	drops below this bias value, the host does not allow the current
	CPU to send more gfx data aside from the skid.  This bias value
	does not change.

	The current credit bytes is equal to the total number of
	host-side credits available to the current CPU.  Diagnostics
	are particularly interested in this value.

	The intr_delay_nsec is the number of nanoseconds that the 
	host will allow the current credit bytes to be below the
	bias value before it interrupts the CPU.

	KONAinitialize (aka the GFX_INITIALIZE ioctl) must have been called
	before making this call.

	Possible errors:
		None.
*/

typedef struct kona_diag_credits_args {
	uint_t  total;  	 /* OUT: total credits; 0 == credit-based flow control not used */
	uint_t  bias;   	 /* OUT: bias credits (equals CPU skid on current platform)     */
	uint_t  current;	 /* OUT: current number of host-side credits		        */
	uint_t  intr_delay_nsec; /* OUT: nsecs current can be < bias before CPU interrupted     */
} kona_diag_credits_args_t;

#endif /* _KONA_DIAG_H */
