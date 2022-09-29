/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*
 * errors.h
 *
 *	Header file for error reporting for HIPPI Serial Card for Origin.
 *      Defines errors written by the firmware to the hcmd sign register 
 *	(which is visible to driver) and error led's values.
 * 	must be a separate file because assembly, firmware, and driver 
 *	all need to use it.
 */

#ifndef __HIPPI_FIRM_ERRORS_H
#define __HIPPI_FIRM_ERRORS_H

/*************************************************************
  Boot Prom (lincprom) codes
*************************************************************/

/* Progress codes used by boot firmware.  These values are placed on the LEDs while
 * the lincprom goes through its paces.  Note that seeing this value on
 * the LEDs means that it has begun this procedure, not that it has
 * finished it!  So, this gives you an idea of where the lincprom got
 * stuck if it hangs.
 *
 * led<3> is always on to show it is a lincprom status code.
 * led<2:0> is progress code. Note that if error occurs and an error 
 * wink is being displayed, it uses led<2>, so the progress led<2> 
 * is lost. This doesn't matter in most cases, since if SDRAM
 * tests pass there isn't much code executed (and no new hardware
 * functionality tested) between MEMTEST_DONE and either dieing
 * because no firmware or jumping into linc fw error codes.
 */
#define PROGRESS_RESET		0x8	/* started executing at reset vector */
#define PROGRESS_INIT_SDRAM	0x9	/* configuring SDRAM */
#define PROGRESS_INIT_CACHES	0xa	/* initializing the processor caches */
#define PROGRESS_TEST_SDRAM	0xb	/* testing SDRAM */
#define PROGRESS_MEMTEST_DONE	0xc	/* mem tests done, copying firmware */


/* DIE codes.  These are failure codes that we'll convey using
 * a winking LED if things go wrong during power-up. 
 */
#define DIE_SDRAMBAD	2	/* SDRAM failed memory test */
#define DIE_CACHEEXC	3	/* cache exception was called. */
#define DIE_CKSUM	4	/* PROM'ed code had bad checksum */
#define DIE_NMI		5	/* PROM got unexpected NMI */
#define	DIE_BADEXCEPT	6	/* unexpected exception */
#define DIE_NOFIRM	7	/* no firmware is present. spinning. */



/*************************************************************
  Linc fw status codes
*************************************************************/

    /* Boot status codes */
#define HIP_SIGN_INIT		0x0bead000 /* init value after SDRAM tests */
#define HIP_SIGN_C_INIT		0x0bead100 /* init value after C code starts */
#define HIP_SIGN_RR_VEND	0x0bead301 /* testing rr vendor/dev id*/
#define HIP_SIGN_RR_BIST	0x0bead302 /* testing rr BIST */
#define HIP_SIGN_RR_WIN		0x0bead303 /* testing rr window size */
#define HIP_SIGN_RR_MEM		0x0bead304 /* testing rr memory */
#define HIP_SIGN_RR_BOOT	0x0bead400 /* loading/booting roadrunner fw */
#define HIP_SIGN		0x0AceFace /* all is okay - boot success! */

    /* crash signatures */
#define HIP_SIGN_MODE_SHIFT	24         /* num bits major mode is shifted */
#define HIP_SIGN_ASSFAIL	0x0a	   /* fw hit an assertion failure */
#define HIP_SIGN_CDIE		0x0d	   /* fw hit a die code failure */
#define HIP_SIGN_BOOT_DEATH	0x0b	   /* boot prom died 
					    * see firm/linc/include/lincprom.h
					    * for codes.
					    */

#define HIP_SIGN_ASSFAIL_FILE_MASK  0x00ff0000 /* mask to get the file number */
#define HIP_SIGN_ASSFAIL_LINE_MASK  0x0000ffff /* mask to get the line number */
#define HIP_SIGN_ASSFAIL_FILE_SHIFT  16 /* mask to get the file number */

#define HIP_SIGN_CDIE_MAJ_MASK	0x00ff0000 /* mask to get major number */
#define HIP_SIGN_CDIE_MIN_MASK	0x0000ffff /* mask to get minor number */
#define HIP_SIGN_CDIE_MAJ_SHIFT	16         

#define HIP_SIGN_BOOT_MAJ_SHIFT 16 /* failure number for boot death */

/*************************************************************
  Linc fw DIE codes.
************************************************************ */

#define CDIE_RR_CFG_ERR	1	/* roadrunner conig space init failed */
#define CDIE_RR_ERR	2	/* roadrunner mem space/SSRAM init failed */
#define CDIE_CISR_ERR	3	/* LINC has given us an error interupt.
				 * minor code is MSBit that is asserted.
				 * fw tries to catagorize it by what bus 
				 * saw error, but if it can't it lumps the 
				 * error as a CDIE_AUX_CISR_MISC */
#define CDIE_OPP_DEAD   4 	/* opposite side's firmware is dead - this
				 * is mapped for the LED's to both yellow
				 * LED's on (no blinking)*/
#define CDIE_ASSFAIL	5	/* not seen in sign register */

/* Auxilary Data supplied with DIE codes */

#define CDIE_AUX_RR_CFG_VEND	1 /* rr device/vendor code doesn't match */
#define CDIE_AUX_RR_CFG_BIST	2 /* rr BIST test failed */
#define CDIE_AUX_RR_CFG_WIN	3 /* rr failed cfg space window size test */
#define CDIE_AUX_RR_CFG_HALT	4 /* rr wan't halted at boot time */

#define CDIE_AUX_RR_SSRAM_DATA1	1 /* failed verify on first data pattern */
#define CDIE_AUX_RR_SSRAM_DATA2	2 /* failed verify on 2nd data pattern */
#define CDIE_AUX_RR_SSRAM_ADDR	3 /* failed verify on address lines test */
#define CDIE_AUX_RR_FW_BOOT	4 /* roadrunner firmware didn't boot */
#define CDIE_AUX_RR_FW_DEAD	5 /* rr firmware didn't clear watchdog bit */

#define CDIE_AUX_CISR_PPCI_ERR	1 /* some error occurred on PPCI */
#define CDIE_AUX_CISR_CPCI_ERR	2 /* some error occurred on CPCI */
#define CDIE_AUX_CISR_SDRAM_ERR	3 /* some error occurred on Linc SDRAM */
#define CDIE_AUX_CISR_BBUS_ERR	4 /* some error occurred on Byte Bus */
#define CDIE_AUX_CISR_SYSAD_ERR	5 /* some error occured on SYSAD bus */
#define CDIE_AUX_CISR_DMA0_ERR	6
#define CDIE_AUX_CISR_DMA1_ERR	7
#define CDIE_AUX_CISR_MISC	8 /* uncategorized error */

  /* translation from ascii file names to a file number */
#define ASSFAIL_FILENO_SFW	1
#define ASSFAIL_FILENO_COMMON	2
#define ASSFAIL_FILENO_DFW	3
#define ASSFAIL_FILENO_DQUEUE	4
#define ASSFAIL_FILENO_INTR	5
#define ASSFAIL_FILENO_SBYPASS	6
#define ASSFAIL_FILENO_SQUEUE	7
#define ASSFAIL_FILENO_DBYPASS	8
#define ASSFAIL_FILENO_DMA	9
#define ASSFAIL_FILENO_ERRORS	10
#define ASSFAIL_FILENO_MAIN	11

#endif
