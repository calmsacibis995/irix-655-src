#ifndef __LIBELF_H__
#define __LIBELF_H__

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

#include <sys/types.h>
#include <elf.h>

/*
  Most programs can ignore _LIBELF_XTND_64 with no loss of
  functionality.  Compilers are the main exception.

  Compilers which are themselves 32-bit programs desiring to build
  64-bit elf object files in which a section must be bigger than
  what can be spanned by a 32-bit signed integer should be compiled
  with _LIBELF_XTND_64 set and link against -lelf_xtnd instead of
  -lelf.

  #defining _LIBELF_XTND_64 results in using a definition of
  struct Elf_Data which is *not* identical to the structure
  defined in standard SVR4 documentation, and which means that
  the opaque struct Elf is not identical either (because Elf_Data
  is not identical).

  A compiler which is itself a 64-bit object can also call the
  extended version if _LIBELF_XTND_64 is #defined when the
  compiler is compiled, though it adds no capability to do this 
  (and when linking 64-bit, -lelf_xtnd is just a link to -lelf).

  Tools which read 64-bit (or which read or write 32-bit) objects
  but do not create large 64-bit objects need not bother with 
  _LIBELF_XTND_64 or -lelf_xtnd.

  Mixing #define _LIBELF_XTND_64 with -lelf will result in
  link-time errors. Mixing #undef _LIBELF_XTND_64 with -lelf_xtnd
  will result in link-time errors.

  See the declaration of elf64_fsize(),
  elf_rawfile(), and elf_strptr() and struct Elf_Data
  to see the differences.
*/

#undef _LIBELF_XTND_EXPANDED_DATA /* libelf internal #define */
#ifdef _LIBELF_XTND_64            /* public documented #define */
#if  (_MIPS_SZLONG == 32)         
/* When building for a 32bit abi, use extended definition if requested */
#     define     _LIBELF_XTND_EXPANDED_DATA  1
#endif
#endif


#ifdef __STDC__
	typedef void		Elf_Void;
#else
	typedef char		Elf_Void;
#endif


/*	commands
 */

typedef enum {
	ELF_C_NULL = 0,	/* must be first, 0 */
	ELF_C_READ,
	ELF_C_WRITE,
	ELF_C_CLR,
	ELF_C_SET,
	ELF_C_FDDONE,
	ELF_C_FDREAD,
	ELF_C_RDWR,
	ELF_C_READ_MMAP,   /* sgi addition */
	ELF_C_WRITE_FAST,  /* sgi addition */
	ELF_C_NUM	/* must be last */
} Elf_Cmd;


/*	flags
 */

#define ELF_F_DIRTY	0x1
#define ELF_F_LAYOUT	0x4


/*	file types
 */

typedef enum {
	ELF_K_NONE = 0,	/* must be first, 0 */
	ELF_K_AR,
	ELF_K_COFF,
	ELF_K_ELF,
	ELF_K_NUM	/* must be last */
} Elf_Kind;


/*	translation types
 */

typedef enum {
	ELF_T_BYTE = 0,	/* must be first, 0 */
	ELF_T_ADDR,
	ELF_T_DYN,
	ELF_T_EHDR,
	ELF_T_HALF,
	ELF_T_OFF,
	ELF_T_PHDR,
	ELF_T_RELA,
	ELF_T_REL,
	ELF_T_SHDR,
	ELF_T_SWORD,
	ELF_T_SXWORD,
	ELF_T_SYM,
	ELF_T_WORD,
	ELF_T_XWORD,
	ELF_T_NUM	/* must be last */
} Elf_Type;


typedef struct Elf	Elf;
typedef struct Elf_Scn	Elf_Scn;


/*	archive member header
 */

typedef struct {
	char		*ar_name;
	time_t		ar_date;
	uid_t	 	ar_uid;
	gid_t 		ar_gid;
	mode_t		ar_mode;
	off_t		ar_size;
	char 		*ar_rawname;
} Elf_Arhdr;


/*	archive symbol table
 */

typedef struct {
	char		*as_name;
	size_t		as_off;
	unsigned long	as_hash;	/* as computed by elf_hash() */
} Elf_Arsym;


/*	data descriptor
 */

typedef struct {
	Elf_Void	*d_buf;
	Elf_Type	d_type;
#ifdef  _LIBELF_XTND_EXPANDED_DATA
	Elf64_Xword	d_size;
	Elf64_Sxword	d_off;		/* offset into section */
	Elf64_Xword	d_align;	/* alignment in section */
#else
	size_t		d_size;
	off_t		d_off;		/* offset into section */
	size_t		d_align;	/* alignment in section */
#endif
	unsigned	d_version;	/* elf version */
} Elf_Data;


/*	function declarations
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _LIBELF_XTND_EXPANDED_DATA
#define elf_begin 	elf_begin_xtnd
#define elf_cntl 	elf_cntl_xtnd
#define elf_end 	elf_end_xtnd
#define elf_errmsg 	elf_errmsg_xtnd
#define elf_errno 	elf_errno_xtnd
#define elf_fill 	elf_fill_xtnd
#define elf_flagdata 	elf_flagdata_xtnd
#define elf_flagehdr 	elf_flagehdr_xtnd
#define elf_flagelf 	elf_flagelf_xtnd
#define elf_flagphdr 	elf_flagphdr_xtnd
#define elf_flagscn 	elf_flagscn_xtnd
#define elf_flagshdr 	elf_flagshdr_xtnd
#define elf32_fsize 	elf32_fsize_xtnd
#define elf64_fsize 	elf64_fsize_xtnd
#define elf_getarhdr 	elf_getarhdr_xtnd
#define elf_getarsym 	elf_getarsym_xtnd
#define elf_getbase 	elf_getbase_xtnd
#define elf_getdata 	elf_getdata_xtnd
#define elf32_getehdr 	elf32_getehdr_xtnd
#define elf64_getehdr 	elf64_getehdr_xtnd
#define elf_getident 	elf_getident_xtnd
#define elf32_getphdr 	elf32_getphdr_xtnd
#define elf64_getphdr 	elf64_getphdr_xtnd
#define elf_getscn 	elf_getscn_xtnd
#define elf32_getshdr 	elf32_getshdr_xtnd
#define elf64_getshdr 	elf64_getshdr_xtnd
#define elf_hash 	elf_hash_xtnd
#define elf_kind 	elf_kind_xtnd
#define elf_ndxscn 	elf_ndxscn_xtnd
#define elf_newdata 	elf_newdata_xtnd
#define elf32_newehdr 	elf32_newehdr_xtnd
#define elf64_newehdr 	elf64_newehdr_xtnd
#define elf32_newphdr 	elf32_newphdr_xtnd
#define elf64_newphdr 	elf64_newphdr_xtnd
#define elf_newscn 	elf_newscn_xtnd
#define elf_nextscn 	elf_nextscn_xtnd
#define elf_next 	elf_next_xtnd
#define elf_rand 	elf_rand_xtnd
#define elf_rawdata    	elf_rawdata_xtnd
#define elf_rawfile    	elf_rawfile_xtnd
#define elf_strptr     	elf_strptr_xtnd
#define elf_update     	elf_update_xtnd
#define elf_version    	elf_version_xtnd
#define elf32_xlatetof 	elf32_xlatetof_xtnd
#define elf64_xlatetof 	elf64_xlatetof_xtnd
#define elf32_xlatetom 	elf32_xlatetom_xtnd
#define elf64_xlatetom	elf64_xlatetom_xtnd
#endif

Elf		*elf_begin	(int, Elf_Cmd, Elf *);
int		elf_cntl	(Elf *, Elf_Cmd);
int		elf_end		(Elf *);
const char	*elf_errmsg	(int);
int		elf_errno	(void);
void		elf_fill	(int);
unsigned	elf_flagdata	(Elf_Data *, Elf_Cmd, unsigned);
unsigned	elf_flagehdr	(Elf *, Elf_Cmd,  unsigned);
unsigned	elf_flagelf	(Elf *, Elf_Cmd, unsigned);
unsigned	elf_flagphdr	(Elf *, Elf_Cmd, unsigned);
unsigned	elf_flagscn	(Elf_Scn *, Elf_Cmd, unsigned);
unsigned	elf_flagshdr	(Elf_Scn *, Elf_Cmd, unsigned);
size_t		elf32_fsize	(Elf_Type, size_t, unsigned);
#ifdef _LIBELF_XTND_EXPANDED_DATA
Elf64_Xword	elf64_fsize	(Elf_Type, Elf64_Xword, unsigned);
#else
size_t		elf64_fsize	(Elf_Type, size_t, unsigned);
#endif
Elf_Arhdr	*elf_getarhdr	(Elf *);
Elf_Arsym	*elf_getarsym	(Elf *, size_t *);
#ifdef _LIBELF_XTND_EXPANDED_DATA
Elf64_Xword	elf_getbase	(Elf *);
#else
off_t		elf_getbase	(Elf *);
#endif
Elf_Data	*elf_getdata	(Elf_Scn *, Elf_Data *);
Elf32_Ehdr	*elf32_getehdr	(Elf *);
Elf64_Ehdr	*elf64_getehdr	(Elf *);
char		*elf_getident	(Elf *, size_t *);
Elf32_Phdr	*elf32_getphdr	(Elf *);
Elf64_Phdr	*elf64_getphdr	(Elf *);
Elf_Scn		*elf_getscn	(Elf *elf, size_t);
Elf32_Shdr	*elf32_getshdr	(Elf_Scn *);
Elf64_Shdr	*elf64_getshdr	(Elf_Scn *);
unsigned long	elf_hash	(const char *);
Elf_Kind	elf_kind	(Elf *);
size_t		elf_ndxscn	(Elf_Scn *);
Elf_Data	*elf_newdata	(Elf_Scn *);
Elf32_Ehdr	*elf32_newehdr	(Elf *);
Elf64_Ehdr	*elf64_newehdr	(Elf *);
Elf32_Phdr	*elf32_newphdr	(Elf *, size_t);
Elf64_Phdr	*elf64_newphdr	(Elf *, size_t);
Elf_Scn		*elf_newscn	(Elf *);
Elf_Scn		*elf_nextscn	(Elf *, Elf_Scn *);
Elf_Cmd		elf_next	(Elf *);
size_t		elf_rand	(Elf *, size_t);
Elf_Data	*elf_rawdata	(Elf_Scn *, Elf_Data *);
#ifdef _LIBELF_XTND_EXPANDED_DATA
char          *elf_rawfile    (Elf *, Elf64_Xword *);
char          *elf_strptr     (Elf *, Elf64_Xword, Elf64_Xword);
#else
char		*elf_rawfile	(Elf *, size_t *);
char		*elf_strptr	(Elf *, size_t, size_t);
#endif
off_t		elf_update	(Elf *, Elf_Cmd);
unsigned	elf_version	(unsigned);
Elf_Data	*elf32_xlatetof	(Elf_Data *, const Elf_Data *, unsigned);
Elf_Data	*elf64_xlatetof	(Elf_Data *, const Elf_Data *, unsigned);
Elf_Data	*elf32_xlatetom	(Elf_Data *, const Elf_Data *, unsigned);
Elf_Data	*elf64_xlatetom	(Elf_Data *, const Elf_Data *, unsigned);

#ifdef __cplusplus
}
#endif

#endif /* _LANGUAGE_C || _LANGUAGE_C_PLUS_PLUS */

#endif /* __LIBELF_H__ */
