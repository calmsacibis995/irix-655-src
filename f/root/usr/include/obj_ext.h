#ifndef __OBJ_EXT_H
#define __OBJ_EXT_H
#include <elf.h>
#include <sys/elf.h>
#include <msym.h>
extern unsigned long
foreach_section(
	struct obj	*obj,
	unsigned long	(*routine)(
			    struct obj		*obj, 
#ifdef __sgi
                            void *      handle,
#else
			    struct scnhdr	*handle,
#endif
			    unsigned long	data),
	unsigned long	data);
extern SCNHDR	 *
address_to_section(
    struct obj		*obj,
    unsigned long	find_addr);
extern unsigned long
section_type(
	struct obj	*obj,
	struct scnhdr	*scnhdr);
extern char *
section_raw(
    struct obj		*obj,
    struct scnhdr	*section);
extern unsigned long
section_nrel(
    struct obj		*obj,
    struct scnhdr	*section);
extern struct reloc *
section_rel(
    struct obj		*obj,
    struct scnhdr	*section);
extern unsigned long
find_section_byname(
		 struct obj	*obj,
		 struct scnhdr 	*handle,
		 unsigned long	data);
extern unsigned long
find_section_bytype(
		 struct obj	*obj,
		 struct scnhdr	*handle,
		 unsigned long	data);
extern unsigned long
obj_section_type(
		struct obj_list *obj_list,
		unsigned long addr);
extern unsigned long
aux_isym(
	struct obj	*obj,
	unsigned long	iaux);
extern unsigned long
file_aux_isym(
	struct obj	*obj,
	unsigned long	file,
	unsigned long	iaux);
extern long
get_aux(
	struct obj 	*obj,
	long 		typeindex);
extern void
get_range(
	  struct obj    *obj,
	  long          typeindex,
	  struct obj_type *type);
extern unsigned long *
objList_add(
    struct obj_list	**head,			/* points to head of list */
    unsigned long	element,		/* new data item */
    char		*msg,			/* for error message */
    unsigned long	where			/* beginning or end */
    );
extern unsigned long *
objList_change(
    struct obj_list	**head,			/* points to head of list */
    unsigned long	list_obj,		/* list obj where new item will
						 * be inserted before or after
						 */
    unsigned long	element,		/* new data item */
    char 		*msg,
    unsigned long	where			/* before or after */
    );
extern unsigned long
foreach_obj (
    struct obj_list	*head,
    unsigned long	(*routine)(unsigned long x, unsigned long y, 
				unsigned long *p),
    unsigned long	data
    );
extern unsigned long
forall_previous_objs (
    struct obj_list	*head,
    struct obj		*obj,
    unsigned long	(*routine)(struct obj	*cur, struct obj	*orig,
				unsigned long d),
    unsigned long	data
    );
extern unsigned long
list_last(
    struct obj_list		*head);
extern unsigned long
foreach_sublist (
    struct obj_list	*head,
    unsigned long	(*routine)(struct obj_list *head, unsigned long data),
    unsigned long	data
    );
extern unsigned long
symbol_to_file(
	struct obj	*obj,
	unsigned long	symbol);
extern unsigned long
type_to_file(
	struct obj	*obj,
	unsigned long	type);
extern unsigned long
procedure_to_file(
	struct obj	*obj,
	unsigned long	procedure);
extern struct fdr *
file_pfd(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_symbol(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_lineindex(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_cline(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_cbLineOffset(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_lang(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_glevel(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_symbol_count(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
file_type_base(
	struct obj	*obj,
	unsigned long	file);
extern char *
file_string_base(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
foreach_rfd(
	struct obj	*obj,
	unsigned long	file,
	unsigned long	(*routine)(
			    struct obj		*obj,
			    unsigned long	file,
			    char		*data1,
			    long		data2,
			    long		data3),
	unsigned long	data1,
	unsigned long	data2,
	unsigned long	data3);
extern char *
st_file_name(
	struct obj	*obj,
	unsigned long	file);
extern unsigned long
address_to_file(
	struct obj	*obj,
	unsigned long	pc);
extern unsigned long _create_rt_proc_table(
	unsigned long	obj_as_data,
	unsigned long	*result,
	unsigned long	*unused);
extern void
change_fortran_names(struct obj *obj);
extern struct obj *
obj_open(struct obj	*obj,
	char		*objname,
	int		mode);
extern struct obj *
dbx_obj_open(struct obj	*obj,
	char		*objname);
extern struct obj * 
obj_init(struct obj *obj, int mode);
extern struct obj *
add_obj(
	struct obj_list	**obj_list,
	char		*name);
extern struct obj *
dbx_add_obj(
	struct obj_list	**obj_list,
	char		*name);
extern struct obj *
dbx_insert_obj(
	struct obj_list	**obj_list,
        struct obj	*obj);
extern struct obj *
symbol_to_obj(
	struct obj_list	*obj_list,
	unsigned long	symbol);
extern struct obj *
procedure_to_obj(
	struct obj_list	*obj_list,
	unsigned long	procedure);
extern struct obj *
file_to_obj(
	struct obj_list	*obj_list,
	unsigned long	file);
extern struct obj *
address_to_obj(
	struct obj_list	*obj_list,
	unsigned long	addr);
extern struct obj *
obj_read_from_fd(struct obj *obj,
		 int fd,
		 unsigned long offset,
		 unsigned long size);
extern void
    obj_close(struct obj *obj);
extern void
    dbx_obj_close(struct obj *obj);
extern int
    obj_nsections(struct obj *obj);
extern Elf32_Dyn *obj_dynamic(struct obj *obj);
extern int
obj_get_dynamic_info(Elf32_Dyn *dy, 
		     struct obj *o, 
		     unsigned long text_offset,
		     unsigned long data_offset);
extern unsigned elfhash(char *pname);
extern int obj_shared(struct obj *obj);
extern int obj_call_shared(struct obj *obj);
extern char *
elf_get_pt_interp(struct obj *obj);
extern Elf32_Addr
obj_find_symbol_value(unsigned long o, char *name, int diff);
extern unsigned long
obj_find_procedure_table(
	unsigned long	obj_as_data,
	unsigned long	addr,
	unsigned long	*unused);
extern unsigned long
_create_rt_proc_table(
	unsigned long	obj_as_data,
	unsigned long	*result,
	unsigned long	*unused);
extern unsigned long
procedure_symbol(
	struct obj	*obj,
	long		procedure);
extern struct pdr *procedure_ppd(
	struct obj     *obj,
	long 		procedure);
extern unsigned long
procedure_address(
	struct obj	*obj,
	long		procedure);
extern long
procedure_iline(
	struct obj	*obj,
	long		procedure);
extern long
procedure_lnLow(
	struct obj	*obj,
	long		procedure);
extern long
procedure_lnHigh(
	struct obj	*obj,
	long		procedure);
extern long
procedure_cbLineOffset(
	struct obj	*obj,
	long		procedure);
extern unsigned long
address_to_procedure(
	struct obj	*obj,
	unsigned long	pc);
extern int newprocedure(
	unsigned long addr,
        long sym,
        long iline,
        long lnLow,
	long lnHigh);
extern unsigned long
symbol_class(
	struct obj	*obj,
	long		symbol);
extern unsigned long
symbol_type(
	struct obj	*obj,
	long		symbol);
extern unsigned long
symbol_value(
	struct obj	*obj,
	long		symbol);
extern long
symbol_iaux(
	struct obj	*obj,
	long		symbol);
extern long
file_symbol_iaux(
	struct obj	*obj,
	unsigned long	file,
	long		symbol);
extern long
symbol_isym(
	struct obj	*obj,
	long		symbol);
extern long
file_symbol_isym(
	struct obj	*obj,
	unsigned long	file,
	long		symbol);
extern long
symbol_value_isym(
	struct obj 	*obj,
	long		symbol);
extern long
symbol_value_iaux(
	struct obj 	*obj,
	long		symbol);
extern unsigned long
end_symbol(
	struct obj	*obj,
	long		symbol);
extern unsigned long
file_end_symbol(
	struct obj	*obj,
	unsigned long	file,
	long		symbol);
extern unsigned long
procedure_end_symbol(
	struct obj	*obj,
	long		symbol);
extern unsigned long
file_procedure_end_symbol(
	struct obj	*obj,
	unsigned long	file,
	long		symbol);
extern unsigned long
address_to_symbol(
	struct obj_list	*obj_list,
	unsigned long	pc);
extern unsigned long
symbol_to_outer_scope_symbol(
	struct obj	*obj,
	unsigned long	symbol);
extern char *
symbol_name(
	struct obj	*obj,
	long	        symbol);
extern struct symr *user_symbol_alloc();
extern int user_symbol_free();
extern union auxu *user_type_alloc();
extern long newsymbol(
char * name,
int st,
int sc,
struct obj_type *type,
int val);
extern int
modify_user_symbol(
long sym,
int st,
int sc,
struct obj_type *type,
int val);
extern long
find_user_symbol(
char *name,
int casesense);
extern long
scope_name_search(
	struct obj	*obj,
	unsigned long	symbol,
	char		*name,
	long		casesense,
	long 		tag_only);
extern unsigned long
file_scope_name_search(
	struct obj	*obj,
	unsigned long	file,
	char		*name,
	long		casesense,
	long		tag_only);
extern long
search_cobol_main(
	struct obj	*obj);
extern long
search_procedures(
	struct obj	*obj,
	char		*name,
	int		casesense);
extern unsigned long
search_files(
	struct obj	*obj,
	char		*name,
	int		extension,
	int		casesense,
	int		lastcomp);
extern long
st_find_symbol(
	struct obj	*obj,
	long		symbol,
	char		*name,
	int		casesense,
	int		tag_only);
extern int 
case_strcmp(
char *s1,
char *s2);
extern int
case_streq(
char *s1,
char *s2,
int casesense);
extern long
search_externals(
	struct obj	*obj,
	char		*name,
	int 		casesense);
extern unsigned long
access_lines(
	struct obj	*obj,
	unsigned long	file,
	unsigned long	procedure,
	unsigned long	find_address,
	unsigned long	*find_line,
	unsigned long	exact);
extern unsigned long
file_line_to_address(
	struct obj	*obj,
	unsigned long	file,
	unsigned long	*find_line,
	unsigned long	exact);
extern unsigned long
address_to_line(
	struct obj	*obj,
	unsigned long	address);
extern long iline_to_line(
	     struct obj       *obj,
	     struct fdr       *pfd,
	     struct pdr       *ppd,
             long             iline);
extern int set_line(
		struct obj     *obj,
		long           file,
                long           procedure,
                long           proc_iline
);
extern long next_line();
extern int
elf_spoof_coffhdrs(struct obj *obj,
		   struct filehdr *filhdr,
		   struct aouthdr *aouthdr,
		   struct scnhdr *scnhdr);
extern int
elf_nreloc(struct obj *obj, int type);
extern char *
obj_raw_bits(struct obj *obj, char *sectname);
extern int
obj_section_reloc_type(struct obj *obj,
		       int i);
extern int
get_section_num(struct obj *obj);
extern struct obj *
obj_sym_open(struct obj *obj,
	     char *objname);
extern void
obj_sym_close(struct obj *obj);
extern void obj_update_dynamic(struct obj *obj, Elf32_Dyn *dyn);
extern unsigned long obj_vtofo(struct obj *obj,
			      unsigned long vaddr);
extern unsigned long obj_vtop(struct obj *obj,
			  unsigned long vaddr);
extern struct obj *obj_rewrite(struct obj *newobj,
			       struct obj *oldobj,
			       char *newobjfilename);
extern struct obj *obj_write(struct obj *obj);
extern void obj_extend_text_segment(struct obj *obj, int incr);
extern void *
obj_extend_bss_segment(struct obj *obj, int incr);
extern void obj_set_section_size(struct obj *obj, char *sname,int size);
extern void obj_map_dynamic(struct obj *obj,
			    unsigned long tstart, unsigned long tsize,
			    unsigned long *textmap,
			    unsigned long dstart, unsigned dsize,
			    unsigned long *datamap);
extern unsigned long obj_gpvalue(struct obj *obj);
extern void obj_add_global_abs_symbol(struct obj *obj,
				      char *symname,
				      unsigned long address);
extern unsigned long obj_end(struct obj *obj);
extern unsigned long
obj_otype(struct obj *obj);
extern FILHDR *
obj_pfilehdr(struct obj *obj);
extern AOUTHDR *
obj_paouthdr(struct obj *obj);
extern SCNHDR *
obj_pscnhdr(struct obj *obj);
extern pHDRR 
obj_phdrr(struct obj *obj);
extern pSYMR
obj_psymr(struct obj *obj);
extern pEXTR
obj_pextr(struct obj *obj);
extern unsigned long
obj_symbol_base(struct obj *obj);
extern unsigned long
obj_file_base(struct obj *obj);
extern unsigned long
obj_procedure_base(struct obj *obj);
extern unsigned long
obj_type_base(struct obj *obj);
extern unsigned long
obj_base_address(struct obj *obj);
extern void
obj_set_base_address(struct obj *obj, unsigned long x);
extern char *
obj_map_address(struct obj *obj);
extern void
obj_set_map_address(struct obj *obj, char *x);
extern unsigned long
obj_map_diff(struct obj *obj);
extern unsigned long
obj_map_diff_dbx(struct obj *obj);
extern Elf32_Addr *
obj_hash(struct obj *obj);
extern void
obj_set_hash_address(struct obj *obj, Elf32_Addr *x);
extern char *
obj_dynstr(struct obj *obj);
extern void
obj_set_dynstr_address(struct obj *obj,char *x);
extern Elf32_Sym *
obj_dynsym(struct obj *obj);
extern void
obj_set_dynsym_address(struct obj *obj, Elf32_Sym *x);
extern Elf32_Msym *
obj_msym(struct obj *obj);
extern void
obj_set_msym_address(struct obj *obj, Elf32_Msym *x);
extern Elf32_Got *
obj_got(struct obj *obj);
extern void
obj_set_got_address(struct obj *obj, Elf32_Got *x);
extern Elf32_Rel *
obj_dynrel(struct obj *obj);
extern void
obj_set_dynrel_address(struct obj *obj, Elf32_Rel *x);
extern Elf32_Lib *
obj_liblist(struct obj *obj);
extern Elf32_Lib *
obj_create_liblist_entry(Elf32_Word name, Elf32_Word time_stamp, Elf32_Word checksum, Elf32_Word version, Elf32_Word flags);
extern void
obj_set_liblist_address(struct obj *obj, Elf32_Lib *x);
extern Elf32_Conflict *
obj_conflict(struct obj *obj);
extern void
obj_set_conflict_address(struct obj *obj, Elf32_Conflict *x);
extern Elf32_Sword
obj_locgotcount(struct obj *obj);
extern void
obj_set_locgotcount(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_unrefextno(struct obj *obj);
extern void
obj_set_unrefextno(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_gotsym(struct obj *obj);
extern void
obj_set_gotsym(struct obj *obj, Elf32_Word x);
extern Elf32_Word 
obj_timestamp(struct obj *obj);
extern void
obj_set_timestamp(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_ichecksum(struct obj *obj);
extern void
obj_set_ichecksum(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_iversion(struct obj *obj);
extern void
obj_set_iversion(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_dynflags(struct obj *obj);
extern void
obj_set_dynflags(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_dynrelsz(struct obj *obj);
extern void
obj_set_dynrelsz(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_dynrelent(struct obj *obj);
extern void
obj_set_dynrelent(struct obj *obj, Elf32_Word x);
extern Elf32_Sword
obj_dynsymcount(struct obj *obj);
extern void
obj_set_dynsymcount(struct obj *obj, Elf32_Sword x);
extern Elf32_Word
obj_dynsyment(struct obj *obj);
extern void
obj_set_dynsyment(struct obj *obj, Elf32_Word x);
extern Elf32_Word
obj_dynstrsz(struct obj *obj);
extern void
obj_set_dynstrsz(struct obj *obj, Elf32_Word x);
extern Elf32_Sword
obj_liblistcount(struct obj *obj);
extern void
obj_set_liblistcount(struct obj *obj, Elf32_Sword x);
extern Elf32_Sword
obj_conflictcount(struct obj *obj);
extern void
obj_set_conflictcount(struct obj *obj, Elf32_Sword x);
extern Elf32_Got *
obj_extgot(struct obj *obj);
extern void
obj_set_extgot(struct obj *obj, Elf32_Got *x);
extern char *
obj_rpath(struct obj *obj);
extern void
obj_set_rpath(struct obj *obj, char *x);
extern char *
obj_soname(struct obj *obj);
extern void
obj_set_soname(struct obj *obj, char *x);
extern Elf32_Word
obj_rldversion(struct obj *obj);
extern char *
obj_rldversion_string(struct obj *obj);
extern void
obj_set_rldversion(struct obj *obj, Elf32_Word x);
extern unsigned long
obj_text_start(struct obj *obj);
extern void
obj_set_text_start(struct obj *obj, Elf32_Addr x);
extern unsigned long
obj_text_size(struct obj *o);
extern void
obj_set_text_size(struct obj *o, Elf32_Word x);
extern unsigned long
obj_data_start(struct obj *obj);
extern void
obj_set_data_start(struct obj *obj, Elf32_Addr x);
extern unsigned long 
obj_bss_start(struct obj *obj);
extern void
obj_set_bss_start(struct obj *obj, Elf32_Addr x);
extern unsigned long
obj_data_size(struct obj *obj);
extern unsigned long
obj_bss_size(struct obj *o);
extern void
obj_set_bss_size(struct obj *o, Elf32_Word x);
extern char *
obj_name(struct obj *obj);
extern void
obj_set_name(struct obj *obj, char *x);
extern char *
obj_path(struct obj *obj);
extern void
obj_set_path(struct obj *obj, char *x);
extern Elf32_Addr
obj_init_address(struct obj *obj);
extern void
obj_set_init_address(struct obj *obj, Elf32_Addr x);
extern Elf32_Addr
obj_pixie_init_address(struct obj *obj);
extern void
obj_set_pixie_init_address(struct obj *obj, Elf32_Addr x);
extern Elf32_Addr
obj_fini_address(struct obj *obj);
extern void
obj_set_fini_address(struct obj *obj, Elf32_Addr x);
extern Elf32_Addr
obj_entry_address(struct obj *obj);
extern void
obj_set_entry_address(struct obj *obj, Elf32_Addr x);
extern Elf32_Word
obj_rldflags(struct obj *o);
extern void
obj_set_rldflag(struct obj *o, Elf32_Word x);
extern void
obj_unset_rldflag(struct obj *o, Elf32_Word x);
extern int
obj_is_mapped(struct obj *o);
extern int
obj_was_modified(struct obj *o);
extern int
obj_chksum_changed(struct obj *o);
extern int
obj_was_moved(struct obj *o);
extern int
obj_followed_csc(struct obj *o);
extern int
obj_is_symbolic(struct obj *o);
extern int
obj_committed(struct obj *o);
extern void
obj_set_cxx_flags(struct obj *o, Elf32_Word x);
extern int
obj_cxx_flags(struct obj *o);
extern int
obj_is_delta_object(struct obj *o);
extern int
obj_delta_fixup(struct obj *o);
extern int
obj_delta_read_defns(struct obj *o);
extern int
obj_get_delta_version(struct obj *o);
extern int
obj_start_init(struct obj *o);
extern int
obj_init_done(struct obj *o);
extern int
obj_pixie_init_done(struct obj *o);
extern void
obj_set_pixie_init_done(struct obj *o);
extern void
obj_set_start_init(struct obj *o);
extern void
obj_set_init_done(struct obj *o);
extern char *
obj_dynstrtab(struct obj *o);
extern char *
obj_dynstring(struct obj *o, int i);
extern void
obj_set_dynstrtab(struct obj *o, char *str);
extern int
obj_conflict_foreign(struct obj *o);
extern int
obj_liblist_foreign(struct obj *o);
extern Elf32_Addr
obj_dynsym_value(struct obj *o, int i);
extern void
obj_set_dynsym_value(struct obj *o, int i, Elf32_Addr x);
extern Elf32_Byte
obj_sym_other(struct obj *o, int i);
extern Elf32_Word
obj_dynsym_size(struct obj *o, int i);
extern void
obj_set_dynsym_size(struct obj *o, int i, Elf32_Addr x);
extern Elf32_Half
obj_sym_shndx(struct obj *o, int i);
extern Elf32_Word
obj_dynsym_name(struct obj *o, int i);
extern unsigned char
obj_sym_info(struct obj *o, int i);
extern int
obj_msym_exists(struct obj *o);
extern int
obj_msym_not_exists(struct obj *o);
extern Elf32_Word
obj_dynsym_hash_value(struct obj *o, int i);
extern Elf32_Addr
obj_nbucket(struct obj *o);
extern Elf32_Addr
obj_nchain(struct obj *o);
extern Elf32_Addr
obj_hash_bucket(struct obj *o, int i);
extern Elf32_Addr
obj_hash_chain(struct obj *o, int i);
extern Elf32_Addr
obj_dynsym_got(struct obj *o, int i);
extern void
obj_set_dynsym_got(struct obj *o, int i, Elf32_Addr x);
extern Elf32_Addr
obj_locgot(struct obj *o, int i);
extern void
obj_set_local_got(struct obj *o, int i, Elf32_Addr x);
extern Elf32_Word
obj_dynsym_rel_index(struct obj *o, int i);
extern Elf32_Word
obj_msym_ms_info(struct obj *o, int i);
extern void
obj_set_msym_ms_info(struct obj *o, int i, Elf32_Word x);
extern Elf32_Word
obj_msym_ms_hash_value(struct obj *o, int i);
extern void
obj_set_msym_ms_hash_value(struct obj *o, int i, Elf32_Word x);
extern Elf32_Addr
obj_rel_off(struct obj *o, int i);
extern Elf32_Word
obj_rel_info(struct obj *o, int i);
extern Elf32_Addr
obj_conflict_symbol(struct obj *o, int i);
extern char *
obj_liblist_name(struct obj *o, int i);
extern Elf32_Word
obj_liblist_tstamp(struct obj *o, int i);
extern Elf32_Word
obj_liblist_csum(struct obj *o, int i);
extern char *
obj_liblist_version_str(struct obj *o, int i);
extern Elf32_Word
obj_liblist_version(struct obj *o, int i);
extern Elf32_Word
obj_liblist_flags(struct obj *o, int i);
extern char *
obj_interface_version(struct obj *o);
extern int
obj_interface_not_match(struct obj *comp, struct obj *obj, int i);
extern int
obj_checksum_not_match(struct obj *comp, struct obj *obj, int i);
extern int
obj_name_not_match(struct obj *comp, struct obj *obj, int i);
extern int 
obj_tstamp_not_match(struct obj *comp, struct obj *obj, int i);
extern int
obj_different_name(struct obj *oa, struct obj *ob);
extern Elf32_Ehdr *
obj_pelfhdr(struct obj *obj);
extern Elf32_Phdr *
obj_pproghdr(struct obj *obj);
extern Elf32_Shdr *
obj_psecthdr(struct obj *obj);
extern Elf32_Shdr
obj_section(struct obj *obj, int x);
extern Elf32_Half
obj_shstrndx(struct obj *obj);
extern char *
obj_section_index_name(struct obj *obj, int x);
extern char *
obj_section_bits(struct obj *obj, Elf32_Shdr *section);
extern char *
obj_section_index_bits(struct obj *obj, int x);
extern unsigned long
obj_section_vaddr(
	struct obj *obj,
	struct scnhdr *section);
extern unsigned long
obj_section_size(
        struct obj *obj,
        struct scnhdr *section);
extern unsigned long
obj_section_offset(
        struct obj *obj,
        struct scnhdr *section);
extern char *
obj_section_name(
        struct obj *obj,
        struct scnhdr *section);
extern unsigned long
obj_rld_map(struct obj *obj);
extern void
obj_set_rld_map(struct obj *obj, unsigned long x);
extern int
obj_fd(struct obj *obj);
extern void
obj_set_fd(struct obj *obj, int x);
extern long
hdr_symptr(struct obj *obj);
extern long
procedure_lnlow(struct obj *obj, long procedure);
extern long
procedure_lnhigh(struct obj *obj, long procedure);
extern char
obj_arch(struct obj *obj);
extern Elf32_Addr
obj_delta_class(struct obj *obj);
extern void
obj_set_delta_class(struct obj *obj, Elf32_Addr x);
extern Elf32_Sword
obj_delta_class_no(struct obj *obj);
extern void
obj_set_delta_class_no(struct obj *obj, Elf32_Sword x);
extern Elf32_Addr
obj_delta_instance(struct obj *obj);
extern void
obj_set_delta_instance(struct obj *obj, Elf32_Addr x);
extern Elf32_Sword
obj_delta_instance_no(struct obj *obj);
extern void
obj_set_delta_instance_no(struct obj *obj, Elf32_Sword x);
extern Elf32_Addr
obj_delta_reloc(struct obj *obj);
extern void
obj_set_delta_reloc(struct obj *obj, Elf32_Addr x);
extern Elf32_Sword
obj_delta_reloc_no(struct obj *obj);
extern void
obj_set_delta_reloc_no(struct obj *obj, Elf32_Sword x);
extern Elf32_Addr
obj_delta_rel_off(struct obj *o, int i);
extern Elf32_Word
obj_delta_rel_info(struct obj *o, int i);
extern Elf32_Addr
obj_delta_sym(struct obj *obj);
extern void
obj_set_delta_sym(struct obj *obj, Elf32_Addr x);
extern Elf32_Sword
obj_delta_sym_no(struct obj *obj);
extern void
obj_set_delta_sym_no(struct obj *obj, Elf32_Sword x);
extern Elf32_Addr
obj_delta_classsym(struct obj *obj);
extern void
obj_set_delta_classsym(struct obj *obj, Elf32_Addr x);
extern Elf32_Sword
obj_delta_classsym_no(struct obj *obj);
extern void
obj_set_delta_classsym_no(struct obj *obj, Elf32_Sword x);
extern void
obj_set_symlib(struct obj *obj, Elf32_Addr x);
extern Elf32_Word
obj_symlib_num(struct obj *obj, int i);
extern Elf32_Addr
obj_rld_text_resolve_addr(struct obj *obj);
extern void
obj_set_rld_text_resolve_addr(struct obj *obj, Elf32_Addr x);
#endif /* __OBJ_EXT_H */
