#ident  "$Header: "

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mbuf.h>
#include <klib/klib.h>
#include "icrash.h"

/* Global variables
 */
struct mbufconst mbufconst;		/* Sizes of mbuf constants                   */
vtab_t *vtab;					/* Pointer to table of eval variable info	 */
int klp_hndl;                   /* handne of the current klib_s struct */

/* Global variables containing information about the dump
 */
int nfstype;                    /* number of vfs's in vfssw[]                */
int mbpages = -1;				/* Number of mbuf pages in use			     */

/* Addresses of key kernel symbols (variables) are obtained from the 
 * appropriate syment struct. Note that if a symbol is actually a pointer
 * type variable, we don't allocate space for the symbol address because
 * it is not referenced anywhere in the code.  Generally speaking, the 
 * variable name is the name of the symbol with an "S_" prefix.
 */
kaddr_t S_mbpages;        		/* Number of cluster pages in use            */
kaddr_t S_strdzone;             /* Streams zone memory 		                 */

/* The following global variables contain kernel pointers to various 
 * structures, lists, tables, buffers, etc. Getting such a pointer is
 * a two-step process. The syment struct for the symbol is located
 * first. Then, the address of the symbol is used to access the actual
 * pointer value. Generally speaking, they variable name is the name 
 * of the symbol with an "_" prefix.
 */
kaddr_t _hbuf;                  /* buf hash table                            */

/* Pointers to various memory blocks allocated during icrash startup.
 * These blocks have been allocated using the K_PERM flag, which makes
 * them permanent. Throughout the run of the program.
 */
k_ptr_t strstp = 0;				/* Streams statas struct                     */
k_ptr_t tlbdumptbl; 			/* tlb dump table                            */
k_ptr_t _vfssw;                 /* vfs switch table                          */

/* Macros for geting addresses of symbols and pointer values
 */
#define GET_SYM(symp, s) \
	symp = kl_get_sym(s, K_TEMP); \
	if (KL_ERROR) { \
		fprintf(KL_ERRORFP, "icrash: %s not found in symbol table\n", s); \
		exit(1); \
	}

#define GET_SYM_ADDR(k, s) \
	k = kl_sym_addr(s); \
	if (KL_ERROR) { \
		fprintf(KL_ERRORFP, "icrash: %s not found in symbol table\n", s); \
		exit(1); \
	}

#define INIT_UPDATE(fp, s) \
	if (klib_debug) { \
		fprintf (fp, "%s\n", s); \
	} \
	else if (!report_flag && !suppress_flag && (prog != FRU_PROGRAM)) { \
		fprintf(fp, "."); \
		fflush(fp); \
	}

/*
 * fatal()
 */
void
fatal(char *x)
{
	if (x != (char *)NULL) {
		fprintf(KL_ERRORFP, "%s", x);
	}
	exit(1);
}

/*
 * init_global_vars()
 */
void
init_global_vars(FILE *ofp)
{
	k_ptr_t tbp;			/* temp block pointer  */
	struct syment *symp;	/* temp syment pointer */

	/* Note that in the past, icrash would fail if there was any problem
	 * with the initialization of any of the global variables in this
	 * function. Since all critical global variables have been moved
	 * to KLIB, it no longer makes sense to cause icrash to fail if
	 * a problem occurs. Note, however, that ignoring a failure here
	 * does not eliminate the possibility of a problem occurring in
	 * another part of the program.
	 */ 

	if (!(S_mbpages = kl_sym_addr("mbpages"))) {
		fprintf(KL_ERRORFP, "\nicrash: mbpages not found in symbol table\n");
	}

	if (symp = kl_get_sym("_mbufconst", K_TEMP)) {
		kl_get_struct(symp->n_value, sizeof(struct mbufconst), 
				&mbufconst, "mbufconst");
		kl_free_sym(symp);
	}
	else {
		fprintf(KL_ERRORFP, "\nicrash: _mbufconst not found in symbol table\n");
	}

	if (symp = kl_get_sym("nfstype", K_TEMP)) {

		kl_get_block(symp->n_value, 4, &nfstype, "nfstype");
		kl_free_sym(symp);

		if (symp = kl_get_sym("vfssw", K_TEMP)) {
			if (!(_vfssw = kl_alloc_block((nfstype * 
					kl_struct_len("vfssw")), K_PERM))) {
				fprintf(KL_ERRORFP, 
					"NOTICE: could not allocate space for vfssw[]!\n");
			}
			else {
				kl_get_block(symp->n_value, 
					(nfstype * kl_struct_len("vfssw")), _vfssw, "vfssw");
			}
			kl_free_sym(symp);
		}
		else {
			fprintf(KL_ERRORFP, "\nicrash: vfssw not found in symbol table\n");
		}
	}
	else {
		fprintf(KL_ERRORFP, "\nicrash: nfstype not found in symbol table\n");
	}

	if (!(S_strdzone = kl_sym_addr("strdzone"))) {
		if (DEBUG(DC_GLOBAL, 1)) {
			fprintf(KL_ERRORFP, 
				"\nicrash: strdzone not found in symbol table\n");
		}
	}

}

/*
 * init() -- Initialize everything.
 */
int
init(FILE *ofp)
{
	int flags = 0;
	syment_t *symp;	/* temp syment pointer */

	/* Initialize the klib memory allocator. Note that if 
	 * kl_init_mempool() is not called, malloc() and free() 
	 * will be used when allocating or freeing blocks of 
	 * memory. Or, another set of functions can be registered 
	 * instead.
	 */
	INIT_UPDATE(ofp, "Initializing memory pool");
	kl_init_mempool(0, 0, 0);

	/* Set up the error handler right away, just in case there is 
	 * a KLIB error during startup.
	 */
	INIT_UPDATE(ofp, "Setting up klib error");
	kl_setup_error(stderr, program, NULL);

	/* Initialize the klib_s struct that will provide a single 
	 * point of contact for various bits of KLIB data.
	 */
	INIT_UPDATE(ofp, "Initializing klib_s struct");
	if ((klp_hndl = init_klib()) == -1) {
		fprintf(stderr, "Could not initialize klib_s struct!\n");
		return(1);
	}

	if (ignoreos) {
		flags = K_IGNORE_FLAG;
	}
	if (icrashdef_flag) {
		flags |= K_ICRASHDEF_FLAG;
	}

	/* Set any KLIB global behavior flags (e.g., K_IGNORE_FLAG). 
	 */
	INIT_UPDATE(ofp, "Setting klib flags");
	set_klib_flags(flags);

	/* Setup info relating to the corefile...
	 */
	INIT_UPDATE(ofp, "Setting up info relating to namelist/corefile image");
	if (kl_setup_meminfo(namelist, corefile, O_RDWR) || KL_ERROR) {
		if (KL_ERROR == KLE_SHORT_DUMP) {
			/* In this case, we need to save the availmon report, and
			 * at the very least dump out the "dump header" for the caller
			 * of 'icrash'.
			 */
			if (report_flag && (availmon)) {
				availmon_report(ofp);
				exit(dump_header_report(ofp));
			}
			else {
				fprintf(KL_ERRORFP,
					"\nWARNING: Compressed vmcore has no real data pages.  "
					"Use the command\n"
					"         '/etc/uncompvm -h %s' to print out any "
					"available\n"
					"         vmcore information.\n", corefile);
			}
			exit(1);
		}
		fprintf(stderr, "Could not setup meminfo!\n");
		return(1);
	}

	INIT_UPDATE(ofp, "Initializing symbol information");
	if (kl_setup_symtab(namelist)) {
		fprintf(KL_ERRORFP, "\nCould not initialize libsym!\n");
		return(1);
	}

	if (icrashdef_flag) {
		INIT_UPDATE(ofp, "Setting icrashdef filename");
		if (kl_set_icrashdef(icrashdef)) {
			fprintf(KL_ERRORFP, "\nCould not set icrashdef filename!\n");
			fprintf(KL_ERRORFP, "Continuing...\n");
		}
	}

	INIT_UPDATE(ofp, "Initializing libkern");
	if (libkern_init()) {
		fprintf(KL_ERRORFP, "\nCould not initialize libkern!\n");
		return(1);
	}

	/* Check to make sure that we are trying to analyzing a 6.5
	 * system or a dump from a 6.5 system (unless the undocumented
	 * -i flag is used and then all bets are off).
	 */
	if (K_IRIX_REV != IRIX6_5) {
		if (!ignoreos || (K_IRIX_REV != IRIX_SPECIAL)) {
			fprintf(KL_ERRORFP, "\nNOTICE: This version of icrash will only "
				"work on a live system running\n");
			fprintf(KL_ERRORFP, "        IRIX 6.5 or with a system dump "
				"from a 6.5 system.\n");
			return(1);
		}
	}

	/* Check to see if system is active
	 */
	if (ACTIVE) {
		if (prog == FRU_PROGRAM) {
			fatal("fru cannot be run against active systems!\n");
		}
		else if (report_flag) {
			fatal("cannot generate a report against a live system!\n");
		}
	}

	INIT_UPDATE(ofp, "Initializing common kernel variables");
	init_global_vars(ofp);

	if (K_TLBDUMPSIZE) {
		GET_SYM(symp, "tlbdumptbl");
		tlbdumptbl = kl_alloc_block(K_MAXCPUS * K_TLBDUMPSIZE, K_PERM); 
		kl_get_block(kl_kaddr_to_ptr(symp->n_value), 
			(K_MAXCPUS * K_TLBDUMPSIZE), tlbdumptbl, "tlbdumptbl");
		kl_free_sym(symp);
	}

	if (prog == FRU_PROGRAM) {
		if (K_IP < 0) {
			fatal("could not determine IP type!\n");
		} 
		else if ((K_IP != 19) && (K_IP != 21) && (K_IP != 25)) {
			fatal("fru only applies to IP19, IP21, and IP25 systems!\n");
		}
	}

	vtab = (vtab_t*)kl_alloc_block(sizeof(vtab_t), K_PERM);
	init_variables(vtab);

	/* Adjust the size for long, signed long, and unsigned long
	 * entries in the base_type[] array if this is a 32-bit system.
	 */
	if (K_NBPW == 4) {

		int i = 0;
		extern base_t base_type[];

		while(base_type[i].name) {
			if (!strcmp(base_type[i].name, "long") ||
						!strcmp(base_type[i].name, "signed long") ||
						!strcmp(base_type[i].name, "unsigned long")) {
				base_type[i].byte_size = 4;
			}
			i++;
		}
	}

	/* Initialize the kernel struct table (used by the walk command).
	 */
	(void)init_struct_table();

	switch (prog) {

		case ICRASH_PROGRAM:

			/* Set up history table 
			 */
			using_history();
			stifle_history(MAX_HISTORY);
			break;

		case FRU_PROGRAM:
			break;
	}
	return(0);
}
