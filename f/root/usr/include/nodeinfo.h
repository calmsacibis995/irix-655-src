#ifndef __NODEINFO_H__
#define __NODEINFO_H__

/*
 * nodeinfo.h
 *
 *	Structures that describe the configuration of a machine.
 *
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */

#ident "$Revision: 1.3 $"

/*****************************************************************************

	nodeinfo.h

	This file contains declarations and definitions supporting the 
	'nodeinfo' data structure, a high-level, user-friendly, summary
	of host information, including hardware inventory, network addresses,
	and file system structure.

	The nodeinfo structure contains the following information:

	[[[ PE Information ]]]

		num pes
		array of pes
			pe
				board
					type
					speed
				cpu
					type
					rev
				fpu
					type
					rev

	[[[ Memory Information ]]]

		memory size
		memory interleave

	[[[ IP Information ]]]

		hostname
		ip address

		num configured interfaces
		array of configured interfaces
			interface
				device name
				network number
				ip address
				ip name
				up

	[[[ Graphics Information ]]]

		num graphics
		array of graphics
			graphics
				type

	7/28/95, Brian Totty

 *****************************************************************************/

#if defined(_LANGUAGE_C_PLUS_PLUS)
extern "C" {
#endif

#include <sys/types.h>
#include <invent.h>
#include <netinet/in.h>
#include <sys/param.h>

/*---------------------------------------------------------------------------*

  	General Information

 *---------------------------------------------------------------------------*/

typedef struct
{
	int major;
	int minor;
} nodeinfo_revision;

/*---------------------------------------------------------------------------*

  	PE Information

 *---------------------------------------------------------------------------*/

typedef struct
{
	int type;			/* cpuboard state in <sys/invent.h> */
	int speed;			/* Speed in MHz */
} nodeinfo_pe_board;

typedef struct
{
	int type;			/* Implementation ID */
	nodeinfo_revision rev;		/* Revision */
} nodeinfo_pe_cpu;

typedef struct
{
	int type;			/* Implementation ID */
	nodeinfo_revision rev;		/* Revision */
} nodeinfo_pe_fpu;

typedef struct
{
	nodeinfo_pe_board *board;	/* Overall board info */
	nodeinfo_pe_cpu   *cpu;		/* CPU info */
	nodeinfo_pe_fpu   *fpu;		/* FPU info */
} nodeinfo_pe;

typedef struct
{
	int num_pes;			/* # of PE entries */
	nodeinfo_pe **pes;		/* List of ptrs to entries */
} nodeinfo_pe_info;

/*---------------------------------------------------------------------------*

  	Memory Information

 *---------------------------------------------------------------------------*/

typedef struct
{
	int mbytes;			/* Size of main memory in MBytes */
	int interleave;			/* Main memory interleave */
} nodeinfo_mem_info;

/*---------------------------------------------------------------------------*

  	IP Information

 *---------------------------------------------------------------------------*/

typedef struct
{
	int  flags;			/* Status flags */
	char *device;			/* Device name */
	struct in_addr network;		/* Network address */
	struct in_addr address;		/* Interface address */
} nodeinfo_ip_interface;

#define NI_IPF_UP	0x80000000	/* Interface is UP */


typedef struct
{
	long hostid;			/* Hostid of machine */
	char *ip_name;			/* Hostname of primary interface */

	int num_ip_interfaces;		/* Number of interfaces in list */
	nodeinfo_ip_interface **ips;	/* List of ptrs to interface info */
} nodeinfo_ip_info;

/*---------------------------------------------------------------------------*

  	Graphics Information

 *---------------------------------------------------------------------------*/

typedef struct
{
	int type;			/* gfx type from <sys/invent.h> */
	inventory_t info;		/* entire gfx inventory entry */
} nodeinfo_gfx_interface;

typedef struct
{
	int num_gfx_interfaces;		/* # of entries in list */
	nodeinfo_gfx_interface **gfxs;	/* List of ptrs to gfx entries */
} nodeinfo_gfx_info;

/*---------------------------------------------------------------------------*

  	Array Machine Information (only valid with asgetnodeinfo_array)

 *---------------------------------------------------------------------------*/

typedef struct
{
	const char *logicalname;	/* Logical name of machine */
	int	   portnum;		/* Port number of array services */
	int	   num_attrs;		/* # of attributes */
	const char **attrs;		/* List of ptrs to attribute strings */
} nodeinfo_am_info;


/*---------------------------------------------------------------------------*

  	nodeinfo

 *---------------------------------------------------------------------------*/

typedef struct
{
	char *ident;			/* Identifying string */
	nodeinfo_revision version;	/* nodeinfo version */
	nodeinfo_pe_info  *pe_info;	/* Processor Element info */
	nodeinfo_ip_info  *ip_info;	/* IP (network) info */
	nodeinfo_gfx_info *gfx_info;	/* Graphics info */
	nodeinfo_mem_info *mem_info;	/* Memory info */
	nodeinfo_am_info  *am_info;	/* Array machine info */
} nodeinfo;

#define NODEINFO_VERSION_MAJOR	1	/* Major version number */
#define NODEINFO_VERSION_MINOR  2	/* Minor version number */

#if defined(_LANGUAGE_C_PLUS_PLUS)
}
#endif

#endif   /* _NODEINFO_H_ */
