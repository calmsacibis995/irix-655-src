/* Copyright Mark */
/*
 * Don't remove the "Copyright Mark".  It is needed on the first line
 * as a mark used by Makefile to add the copyright to the file it
 * creates called obj_ext.h.
 */
/* $Copyright: |
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991, 1990 MIPS Computer Systems, Inc.      |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 252.227-7013.  |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Avenue                               |
 * |         Sunnyvale, California 94088-3650, USA             |
 * |-----------------------------------------------------------|
 * $ */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/obj.h,v 7.28 1994/08/23 22:31:19 ajp Exp $ */

#ifndef __OBJ_H__
#define __OBJ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <a.out.h>
#include <elf.h>
#include <sys/elf.h>
#include <msym.h>
#include <sex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "obj_type.h"
#include "obj_list.h"
#include "obj_ext.h"
#ifdef _DELTA_C_PLUS_PLUS
#include "elf_delta.h"
#endif

#define global
#define OBJ_CONTINUE	-1
#define OBJ_FAIL	-1

#define OT_NONE		1
#define OT_MIPS_COFF	2
#define OT_MIPS_ELF	3

#define OM_READ		1
#define OM_EXECUTE	2
#define OM_WRITE	3

typedef struct {
	unsigned long	ipd;
	unsigned long	adr;
} dbx_proctbl;

typedef struct obj {
	char		o_target_swapped:1;
	char            o_arch;
	struct stat	o_statb;
	unsigned long	o_type;		/* COFF, whatever */
	FILHDR 		*o_pfilehdr;    /* COFF headers */
	AOUTHDR 	*o_paouthdr;
	SCNHDR		*o_pscnhdr;
	pHDRR		o_phdrr;	/* symbol table header */
	pFDR		o_pfdr;		/* file descriptors */
	pPDR		o_ppdr;		/* proc descriptors */
	pSYMR		o_psymr;	/* local symbols */
	pEXTR		o_pextr;	/* external symbols */
	char		*o_pssext; 	/* external string table */
	char		*o_pss;		/* local string table */
	char		*o_pline;	/* compress line numbers */
	unsigned long 	*o_prfd;	/* relative file descriptors */
	pAUXU		o_pauxu;	/* auxiliaries */
	char		*o_praw;	/* raw data */
	unsigned long	o_type_base; 	/* objlist symbol counts */
	unsigned long	o_symbol_base;
	unsigned long	o_file_base;
	unsigned long	o_procedure_base;
	unsigned long	o_external_base;
	char		*o_name; 	/* name of object (from open or liblist) */
	Elf32_Ehdr	*o_pelfhdr; 	/* ELF headers */
	Elf32_Phdr	*o_pproghdr;
	Elf32_Shdr	*o_psecthdr;
	/* Fields for rld */
	char		*o_path;	/* full path to object */
	char		*o_soname;	/* name of object (from dynamic) */
	int		o_fd;		/* fd from EXECFD */
	unsigned long	o_base_address; /* start of first segment */
	unsigned long	o_text_start;	/* text start */
	unsigned long	o_text_size;    /* size of text in bytes */
	unsigned long	o_data_start;	/* data start */
	unsigned long	o_bss_start;	/* bss start */
	unsigned long	o_bss_size;	/* size of bss in bytes */
	Elf32_Addr	o_entry;	/* entry point */

	Elf32_Addr	*o_base;	/* pointer to base of text */
	Elf32_Addr	*o_hash;	/* pointer to hash table */
	char		*o_str;	        /* pointer to the string table */
	Elf32_Sym	*o_dsym;	/* pointer to the dynsym table */
	Elf32_Msym	*o_msym;	/* pointer to the msym table */
	Elf32_Got	*o_got;	        /* pointer to the local got */
	Elf32_Got	*o_extgot;	/* pointer to the external got */
	Elf32_Rel	*o_rel; 	/* pointer to the reloc table */
	Elf32_Lib	*o_libl;	/* pointer to the liblist table */
	Elf32_Conflict	*o_conf;	/* pointer to the conflict table */
	Elf32_Word	o_dyflags;	/* flags from dynamic table */
	Elf32_Word	o_locgotno;	/* number of local got entries */
	char		*o_rpath;	/* directory path */

	Elf32_Word 	o_rldver;	/* rld version */
	Elf32_Word	o_tstamp;	/* time stamp */
	Elf32_Word	o_ichksum;	/* interface checksum */
	Elf32_Word	o_iversion;	/* interface version */

	Elf32_Sword	o_symcount;	/* symbol count */
	Elf32_Word	o_syent;	/* symbol table entry size */
	Elf32_Word	o_stsize;	/* string table size */
	Elf32_Word	o_rlsize;	/* relocation table size */
	Elf32_Word	o_rlent;	/* relocation table entry size */
	Elf32_Sword	o_llcount;	/* liblist count */
	Elf32_Word	o_htsize;	/* hash table size */
	Elf32_Sword	o_cfcount;	/* conflict table size */

	Elf32_Word	o_rldflag;	/* flags for rld */
	Elf32_Word	o_flag; 	/* flags for libmld */
	Elf32_Word	o_mode;		/* see OM_ */
	Elf32_Addr      o_init;	        /* address of .init section */
        Elf32_Addr      o_fini;         /* address of .fini section */
	Elf32_Word	o_unrefextno;	/* index of first unreferenced ext sym */
	Elf32_Word	o_gotsym;	/* index of first sym that has a GOT entry */
	unsigned long   o_rld_map;      /* contains the address where
                                         * pObj_Head from RLD should be
                                         * written into
                                         */
        dbx_proctbl 	*o_dbx_ptbl;	/* side proc table sorted for b-search*/
	Elf32_Byte	o_init_done;	/* True if init code for this
					 * object has finished execution.
					 */
#ifdef _DELTA_C_PLUS_PLUS

	Delta32_ClassInfo	*o_delta_class;	/* pointer to start of Delta C++ class definition table */
	Elf32_Sword	o_dccount;	/* number of entries for o_delta_class */
	Delta32_ClassInstance	*o_delta_instance;/* pointer to start of Delta C++ class instance table */
	Elf32_Sword	o_dicount;	/* number of entries for o_delta_instance */
	Elf32_Rel	*o_delta_reloc;  /* pointer to start of Delta C++ class relocation table */
	Elf32_Sword	o_drcount;	/* number of entries for o_delta_reloc */
	Delta32_Sym	*o_delta_sym;    /* pointer to start of Delta C++ symbol table that Delta relocation points to */
	Elf32_Sword	o_dscount;	/* number of entries for o_delta_sym */
	Delta32_Sym	*o_delta_classsym;/* pointer to start of Delta C++ symbol table for holding class declaration */
	Elf32_Sword	o_dcscount;	/* number of entries for o_delta_classsym */
	Elf32_Word	o_cxx_flag;	/* Flags indicating information about C++ flavor */
#endif /* _DELTA_C_PLUS_PLUS */
 	Elf32_Addr	o_proc_table;	/* where the runtime procedure table is at runtime */
 	Elf32_Addr      o_pixie_init;   /* address of pixie's .init section */
	Elf32_Addr	o_symlib;	/* address of symbol library section */
	Elf32_Addr	o_rld_text_resolve_addr;    /* recorded address of rld's text resolve function */
} OBJ, *pOBJ;
#define cbOBJ sizeof(OBJ)

#ifdef _OBJ_USE_MACRO
#define obj_otype(obj) ((obj)->o_type)
#define obj_pfilehdr(obj) ((obj)->o_pfilehdr)
#define obj_paouthdr(obj) ((obj)->o_paouthdr)
#define obj_pscnhdr(obj) ((obj)->o_pscnhdr)

#define obj_phdrr(obj) ((obj)->o_phdrr)
#define obj_psymr(obj) ((obj)->o_psymr)
#define obj_pextr(obj) ((obj)->o_pextr)

/* Dbx set/get macros */
#define obj_symbol_base(obj)	((obj)->o_symbol_base)
#define obj_file_base(obj)	((obj)->o_file_base)
#define obj_procedure_base(obj)	((obj)->o_file_base)
#define obj_type_base(obj)	((obj)->o_type_base)

/* Rld set/get macros */
#define obj_base_address(obj) ((obj)->o_base_address)
#define obj_set_base_address(obj,x) (obj_base_address(obj) = (unsigned long)(x))
#define obj_map_address(obj) ((unsigned long)(obj)->o_praw)
#define obj_set_map_address(obj,x) ((obj)->o_praw = (char *)(x))
#define obj_map_diff(obj) ((unsigned long)(obj_base_address(obj) - obj_map_address(obj)))
#define obj_map_diff_dbx(obj) (obj->o_mode != OM_EXECUTE ? 0 : (unsigned long)(obj_base_address(obj) - obj_map_address(obj)))
#define obj_hash(obj) ((obj)->o_hash)
#define obj_set_hash_address(obj,x) (obj_hash(obj) = (Elf32_Addr *)(x))
#define obj_dynstr(obj) ((obj)->o_str)
#define obj_set_dynstr_address(obj,x) (obj_dynstr(obj) = (char *)(x))
#define obj_dynsym(obj) ((obj)->o_dsym)
#define obj_set_dynsym_address(obj,x) (obj_dynsym(obj) = (Elf32_Sym *)(x))
#define obj_msym(obj) ((obj)->o_msym)
#define obj_set_msym_address(obj,x) (obj_msym(obj) = (Elf32_Msym *)(x))
#define obj_got(obj)  ((obj)->o_got)
#define obj_set_got_address(obj,x) (obj_got(obj) = (Elf32_Got *)(x))
#define obj_dynrel(obj) ((obj)->o_rel)
#define obj_set_dynrel_address(obj,x) ((obj_dynrel(obj)) = (Elf32_Rel *)(x))
#define obj_liblist(obj) ((obj)->o_libl)
#define obj_set_liblist_address(obj,x) (obj_liblist(obj) = (Elf32_Lib *)(x))
#define obj_conflict(obj) ((obj)->o_conf)
#define obj_set_conflict_address(obj,x) (obj_conflict(obj) = (Elf32_Conflict *)(x))
#define obj_locgotcount(obj) ((obj)->o_locgotno)
#define obj_set_locgotcount(obj,x) (obj_locgotcount(obj) = (x))
#define obj_unrefextno(obj) ((obj)->o_unrefextno)
#define obj_set_unrefextno(obj,x) (obj_unrefextno(obj) = (x))
#define obj_gotsym(obj) ((obj)->o_gotsym)
#define obj_set_gotsym(obj,x) (obj_gotsym(obj) = (x))
#define obj_timestamp(obj) ((obj)->o_tstamp)
#define obj_set_timestamp(obj,x) (obj_timestamp(obj) = (Elf32_Word)(x))
#define obj_ichecksum(obj) ((obj)->o_ichksum)
#define obj_set_ichecksum(obj,x) (obj_ichecksum(obj) = (Elf32_Word)(x))
#define obj_iversion(obj) ((obj)->o_iversion)
#define obj_set_iversion(obj,x) (obj_iversion(obj) = (Elf32_Word)(x))
#define obj_dynflags(obj) ((obj)->o_dyflags)
#define obj_set_dynflags(obj,x) (obj_dynflags(obj) = (Elf32_Word)(x))
#define obj_dynrelsz(obj) ((obj)->o_rlsize)
#define obj_set_dynrelsz(obj,x) (obj_dynrelsz(obj) = (Elf32_Word)(x))
#define obj_dynrelent(obj) ((obj)->o_rlent)
#define obj_set_dynrelent(obj,x) (obj_dynrelent(obj) = (Elf32_Word)(x))
#define obj_dynsymcount(obj) ((obj)->o_symcount)
#define obj_set_dynsymcount(obj,x) (obj_dynsymcount(obj) = (Elf32_Sword)(x))
#define obj_dynsyment(obj) ((obj)->o_syent)
#define obj_set_dynsyment(obj,x) (obj_dynsyment(obj) = (Elf32_Word)(x))
#define obj_dynstrsz(obj) ((obj)->o_stsize)
#define obj_set_dynstrsz(obj,x) (obj_dynstrsz(obj) = (Elf32_Word)(x))
#define obj_liblistcount(obj) ((obj)->o_llcount)
#define obj_set_liblistcount(obj,x) (obj_liblistcount(obj) = (Elf32_Sword)(x))
#define obj_conflictcount(obj) ((obj)->o_cfcount)
#define obj_set_conflictcount(obj,x) (obj_conflictcount(obj) = (Elf32_Sword)(x))
#define obj_extgot(obj) ((obj)->o_extgot)
#define obj_set_extgot(obj,x) (obj_extgot(obj) = (Elf32_Got *)(x))
#define obj_rpath(obj) ((obj)->o_rpath)
#define obj_set_rpath(obj,x) (obj_rpath(obj) = (char *)(x))
#define obj_soname(obj) ((obj)->o_soname)
#define obj_set_soname(obj,x) (obj_soname(obj) = (char *)(x))
#define obj_rldversion(obj) ((obj)->o_rldver)
#define obj_set_rldversion(obj,x) (obj_rldversion(obj) = (x))

#define obj_text_start(obj) ((obj)->o_text_start)
#define obj_set_text_start(obj,x) (obj_text_start(obj) = (Elf32_Addr)(x))
#define	obj_text_size(o)	((o)->o_text_size)
#define obj_set_text_size(o,x)	(obj_text_size(o) = (Elf32_Word) (x))
#define obj_data_start(obj) ((obj)->o_data_start)
#define obj_set_data_start(obj,x) (obj_data_start(obj) = (Elf32_Addr)(x))
#define	obj_data_size(obj) (obj_bss_start(obj) - obj_data_start(obj))
#define obj_bss_start(obj) ((obj)->o_bss_start)
#define obj_set_bss_start(obj,x) (obj_bss_start(obj) = (Elf32_Addr)(x))
#define	obj_bss_size(o)	((o)->o_bss_size)
#define	obj_set_bss_size(o,x)	(obj_bss_size(o) = (x))
#define obj_name(obj) ((obj)->o_name)
#define obj_set_name(obj,x) (obj_name(obj) = (char *)(x))
#define obj_path(obj) ((obj)->o_path)
#define obj_set_path(obj,x) (obj_path(obj) = (char *)(x))
#define obj_fd(obj) ((obj)->o_fd)
#define obj_set_fd(obj,x) (obj_fd(obj) = (x))

#define obj_init_address(obj) ((obj)->o_init)
#define obj_set_init_address(obj,x) (obj_init_address(obj) = (Elf32_Addr)(x))

#define obj_fini_address(obj) ((obj)->o_fini)
#define obj_set_fini_address(obj,x) (obj_fini_address(obj) = (Elf32_Addr)(x))

#define obj_entry_address(obj) ((obj)->o_entry)
#define obj_set_entry_address(obj,x) (obj_entry_address(obj) = (Elf32_Addr) (x))

#define obj_rld_map(obj) ((obj)->o_rld_map)
#define obj_set_rld_map(obj,x) (obj_rld_map(obj) = (unsigned long) (x))

#define obj_rld_text_resolve_addr(obj) ((obj)->o_rld_text_resolve_addr)
#define obj_set_rld_text_resolve_addr(obj,x) (obj_rld_text_resolve_addr(obj) = (Elf32_Addr) (x))

#endif
/* o_rldflag field values, they are powers of two */
#define	OF_NONE		0x0000		/* object is in the the list */
#define	OF_MAPPED	0x0001		/* object is mapped flag */
#define	OF_MODIFIED	0x0002		/* object has been modified */
#define OF_TSTMPCHG	0x0004		/* object's timestamp has been modified */
#define OF_CHKSUMCHG	0x0008		/* object's checksum has been modified */
#define OF_MOVED	0x0010		/* object has been moved */
#define OF_POSTTST	0x0020		/* object follows a timestamp changed obj */
#define OF_POSTCSUM	0x0040		/* object follows a checksum changed obj */
#define OF_POSTMOVED	0x0080		/* object follows a moved obj */
#define OF_FIXEDUP      0x0100          /* the obj has run fixed_all_defines already */
#define OF_SYMBOLIC     0x0200          /* the obj has DT_SYMBOLIC set -- means
                                         * starts searching itself for symbol
                                         * resolution first rather than
                                         * the executable as in default case
                                         */
#define OF_HIDDEN       0x0400          /* the obj is hidden from the rest of the obj list, thus no objects should be resolved to it */
#define OF_INITIAL      0x0800          /* the obj is an initial obj mapped in at program start-up time */

#define OF_START_INIT	0x1000		/* The obj has had its init
					 * code started.
					 */
#define OF_INIT_DONE	0x2000		/* The obj has hd its int
					 * code completed.
					 */
#define OF_TEMP_UNHIDDEN 0x4000		/* this obj is temporarily open
					 * for symbol resolution
					 */
#define OF_COMMITTED	0x8000		/* this obj is not subject
					 * to deletion during
					 * cleanup after an error
					 * dlopen.
					 */
#define OF_FINI_ORDER_SET     0x10000 /* this obj's fini order is set */
#define OF_PIXIE_INIT_DONE    0x20000 /* The obj has had its pixie init
                                       * code completed.
                                       */
#define OF_INIT_ENCOUNTERED   0x40000 /* this obj has been processed by run_init_code */

#define FOREIGN_OBJ	-1

#ifdef _DELTA_C_PLUS_PLUS
#define	CXX_FLAG_DELTA	    0x1
#define	CXX_DELTA_FIXUP	    0x2		/* object delta relocations have been done */
#define	CXX_DELTA_RDEFNS    0x4		/* object class definitions have been read */
#define	CXX_DELTA_VERSION_MASK   0xf0000000	/* mask for version number of delta sections */
#endif /* _DELTA_C_PLUS_PLUS */

#ifdef _OBJ_USE_MACRO
#define obj_rldflags(o) ((o)->o_rldflag)
#define obj_set_rldflag(o,x) (obj_rldflags(o) |= (x))
#define obj_unset_rldflag(o,x) (obj_rldflags(o) &= ~(x))
#define obj_is_mapped(o) (obj_rldflags(o)&OF_MAPPED)

#define obj_was_modified(o) (obj_rldflags(o)&OF_MODIFIED)

#define obj_chksum_changed(o) (obj_rldflags(o)&OF_CHKSUMCHG)

#define obj_was_moved(o) (obj_rldflags(o)&OF_MOVED)

#define obj_followed_csc(o) (obj_rldflags(o)&OF_POSTCSUM)

#define obj_fixedup_moved(o) (obj_rldflags(o)&OF_FIXEDUP)

#define obj_is_hidden(o) (obj_rldflags(o)&OF_HIDDEN)

#ifdef _DELTA_C_PLUS_PLUS
#define obj_cxx_flags(o) ((o)->o_cxx_flag)
#define obj_is_delta_object(o) (obj_cxx_flags(o)&CXX_FLAG_DELTA)
#define obj_delta_fixup(o) (obj_cxx_flags(o)&OF_DELTA_FIXUP)
#define obj_delta_read_defns(o) (obj_cxx_flags(o)&OF_DELTA_RDEFNS)
#define obj_get_delta_version(o) (((o) & CXX_DELTA_VERSION_MASK) >> 28)
#endif  /* _DELTA_C_PLUS_PLUS */

/* Dynamic string indices -> char * */
#define obj_dynstrtab(o)   ((o)->o_str)
#define obj_dynstring(o,i) ((char *)(obj_dynstrtab(o)+i))

/* Dynamic symbol manipulation */
/* caps leftover from rld */
#define obj_conflict_foreign(o)	(obj_conflictcount(o) == FOREIGN_OBJ)
#define obj_liblist_foreign(o)	(obj_liblistcount(o) == FOREIGN_OBJ)

#define	obj_dynsym_value(o,i)	((o)->o_dsym[(i)].st_value)
#define	obj_dynsym_size(o,i)	((o)->o_dsym[(i)].st_size)
#define	obj_sym_shndx(o,i)	((o)->o_dsym[(i)].st_shndx)
#define	obj_dynsym_name(o,i)	(obj_dynstring(o,(o)->o_dsym[(i)].st_name))
#define	obj_sym_info(o,i)	((o)->o_dsym[(i)].st_info)
#endif


#define NOMSYM	((Elf32_Msym *)0)


#ifdef _OBJ_USE_MACRO
#define obj_msym_exists(o)	(((o)->o_msym != NOMSYM))
#define obj_msym_not_exists(o)	(((o)->o_msym == NOMSYM))
#define	obj_dynsym_hash_value(o,i)   ((obj_msym_exists(o) && ((o)->o_msym[i].ms_hash_value)) ? (o)->o_msym[i].ms_hash_value : get_dynsym_hash_value((o),i))
#define	obj_nbucket(o)	((o)->o_hash[0])
#define	obj_nchain(o)	((o)->o_hash[1])
#define	obj_hash_bucket(o,i)	((o)->o_hash[(i+2)])
#define	obj_hash_chain(o,i)	((o)->o_hash[(i)+obj_nbucket(o)+2])
#define	obj_dynsym_got(o,i)	((o)->o_extgot[(i-obj_gotsym(o))].g_index)
#define obj_set_dynsym_got(o,i,x) (obj_dynsym_got(o,i) = (x))
#define	obj_locgot(o,i)	((o)->o_got[(i)].g_index)

#define obj_set_local_got(o,i,x) (obj_locgot(o,i) = (x))
#define	obj_dynsym_rel_index(o,i)	ELF32_MS_REL_INDEX((o)->o_msym[(i)].ms_info)
#define	obj_msym_ms_info(o,i)	((o)->o_msym[(i)].ms_info)
#define	obj_set_msym_ms_info(o,i,x)	((o)->o_msym[(i)].ms_info = (x))
#define	obj_msym_ms_hash_value(o,i)	((o)->o_msym[(i)].ms_hash_value)
#define	obj_set_msym_ms_hash_value(o,i,x)	((o)->o_msym[(i)].ms_hash_value = (x))
#endif

#ifdef _OBJ_USE_MACRO
#define obj_rel_off(o,i)	((o)->o_rel[(i)].r_offset)
#define	obj_rel_info(o,i)	((o)->o_rel[(i)].r_info)
#define	obj_conflict_symbol(o,i)	((o)->o_conf[(i)].c_index)
#define obj_liblist_name(o,i)   (obj_dynstring(o,obj_liblist(o)[(i)].l_name))
#define	obj_liblist_tstamp(o,i)	((o)->o_libl[(i)].l_time_stamp)
#define	obj_liblist_csum(o,i)	((o)->o_libl[(i)].l_checksum)
#define	obj_liblist_version_str(o,i)	((o)->o_str+(o)->o_libl[(i)].l_version)
#define	obj_liblist_version(o,i)	((o)->o_libl[(i)].l_version)
#define	obj_liblist_flags(o,i)	((o)->o_libl[(i)].l_flags)
#define	obj_interface_version(o)	((o)->o_str+(o)->o_iversion)

#define obj_interface_not_match(comp,obj,i) \
                      (strcmp(obj_liblist_version_str(obj,i), \
			      obj_interface_version(comp)))

#define obj_checksum_not_match(comp,obj,i) \
                      (obj_liblist_csum(obj,i) != comp->o_ichksum)
#define obj_name_not_match(comp,obj,i) \
                      (strcmp(obj_liblist_name(obj,i), comp->o_soname))

#define obj_tstamp_not_match(comp,obj,i) \
                      (obj_liblist_tstamp(obj,i) != comp->o_tstamp)


#define obj_different_name(oa,ob)	(strcmp(oa->o_soname, ob->o_soname))

/* Elf fields */
#define obj_pelfhdr(obj) ((obj)->o_pelfhdr)
#define obj_pproghdr(obj) ((obj)->o_pproghdr)
#define obj_psecthdr(obj) ((obj)->o_psecthdr)

#define obj_section(obj,x) (obj_psecthdr(obj)[x])
#define obj_shstrndx(obj)  (obj_pelfhdr(obj)->e_shstrndx)
#define obj_section_index_name(obj,x) \
                   ((char *) (obj_shstrndx(obj) ? \
			      (obj)->o_praw + \
			      obj_section(obj,obj_shstrndx(obj)).sh_offset + \
			      obj_section(obj,x).sh_name : "N/A"))
#define obj_section_name(obj,section) \
                   ((char *)((obj)->o_praw + \
			     obj_section(obj,obj_shstrndx(obj)).sh_offset + \
			     (section)->sh_name))
#define obj_section_bits(obj,section) \
    ((char *)(&((obj)->o_praw[(section)->sh_offset])))
#define obj_section_index_bits(obj,x) \
    ((char *)(&((obj)->o_praw[obj_section(obj,x).sh_offset])))
#endif

#ifndef _ELF
#define _RHEADER	".rheader"


#ifdef _OBJ_USE_MACRO
#define hdr_symptr(pobj) (pobj->o_pfilehdr->f_symptr)

#define obj_section_vaddr(obj, psection) (psection->s_vaddr)
#define obj_section_size(obj, psection) (psection->s_size)
#define obj_section_name(obj, psection) (psection->s_name)
#define procedure_lnlow(obj, procedure) (obj->o_ppdr[procedure-obj->o_procedure_base].lnLow)
#define procedure_lnhigh(obj, procedure) (obj->o_ppdr[procedure-obj->o_procedure_base].lnHigh)

#endif
#endif /* _ELF */

#ifdef __cplusplus
}
#endif

#endif 
