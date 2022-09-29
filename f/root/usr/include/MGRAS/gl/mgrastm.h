#ifndef __glmgrastm_h_
#define __glmgrastm_h_

/*
** Copyright 1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** $Header: /proj/irix6.5f/isms/gfx/include/MGRAS/gl/RCS/mgrastm.h,v 1.58 1997/08/05 01:50:08 jerian Exp $
*/

#include <sys/mgras.h>

#define MGRAS_TEXMAN_DMA_POOL_PAGE_SHIFT	12	/* to match size */
#define MGRAS_TEXMAN_DMA_POOL_PAGE_SIZE		0x1000

#define MG_TM_VIDTEX_MAXLOD	9	/* Max LOD for MIPMAP video tex */

#define MMAN 0
#define TMAN 1
#define LUM1 2
#define LUMAL 3
#define LUM2 4

/*
** texture name/target structure
*/
typedef struct __GLMGTexNameTargRec
{
    GLuint	texName;
    GLuint	texTarg;
} __GLMGTexNameTarg;

/*
** save/restore info structure
*/
typedef struct __TexDesc{
    struct __TexDesc *next, *n[2], *prev, *p[2]; /* pointers for list structures */
    struct __TexTransParm *tp;		/* pointer back to level structure */
    GLfloat *priority;			/* Pointer to priority value */
    GLshort start[2], num[2], end[2];	/* offset, start, #pages, end */
    GLushort HQPagesPer;
    GLushort tram_usage;	
    }TexDesc;

typedef struct __TexTransParm{
    TexDesc *desc;			/* pointer to image data structure */
    struct __GLMGTexLevInfoRec *lev;	/* pointer back to level structure */
    struct __TexTransParm *next[2], *prev[2]; /* Pointers for list structures */
    struct __TexTransParm *used[2], *lru[2];  /* TM used and least-recently-used list */
    struct __GLMGcontextRec *hwcx;	/* context responsible for use */
    mgras_dma_flush_defn *flush; 	/* structure for flush definition */
    mgras_dma_replace_defn *replace; 	/* structure for replace definition */
    void *impage_starts;	/* pointers to page starts */
    unsigned int *impage_nums;	/* pointers to impage numbers */
    GLubyte *data;		/* Pointer to data */
    GLubyte *freedata;		/* Original pointer to malloced data */
    GLushort linebytes;		/* bytes per line */
    GLushort pagelines;		/* lines per page */
    GLshort start[2];		/* TRAM and memory page location */
    GLshort num[2]; 		/* total # pages for level */
    GLubyte vx2;		/* Vertical x2 mode */
    GLubyte tr1;		/* Special 1-TRAM mode */
    GLubyte numcomp;		/* # components 0-3 */
    GLubyte comptype;		/* 0=>1byte/1=>2byte */
    GLubyte select;		/* indicates TRAM subsetting (amongst TRAMs) */
    GLubyte saved;		/* level image data saved */
    GLubyte swap[2];		/* swap area location */
    GLubyte resid[2];		/* hardware residency or memory locked for complete level */
    GLubyte tram_usage;		/* Size of TRAM subset used */
    }TexTransParm;

#define MG_TM_1D 	0
#define MG_TM_PROXY_1D	1
#define MG_TM_2D	2
#define MG_TM_PROXY_2D	3
#define MG_TM_3D	4
#define MG_TM_PROXY_3D	5
#define MG_TM_4D	6
#define MG_TM_PROXY_4D	7
#define MG_TM_DET	8

#define TM_LUMINANCE	0
#define TM_ALPHA	1
#define TM_INTENSITY	2
/*
** Macro to restore the curr pointer of a TexDimInfo after pull-model
** traversal has nulled it out
*/
#define REFRESH_DIM(dim) { \
	if ((dim) && ((dim)->curr == NULL)) \
		mgrTM_RefreshDim(dim); \
	}

/*
** detail texture lut descriptor
*/
typedef struct __GLMGTexDetFuncRec {
    GLint requestedNumber;	/* Number of points requested by user */
    GLfloat *requestedPoints;	/* Points requested by user */
    GLint lut[32];		/* Detail texture lookup table */
    GLint detail_c;		/* Detail cutoff value */
    struct __GLMGTexDetFuncRec *next; 	/* next in linked list */
    } __GLMGTexDetFunc;

/*
** texture level identification
*/
typedef struct __GLMGTexLevIDRec {
    GLsizei width, height, depth, extent; /* sizes declared for level */
    GLint brd_data;		/* border data declared by app */
    GLenum requestedformat;	/* internal format requested by the user */
    } __GLMGTexLevID;

#define MG_TM_TEXID_SIZE 34
#define MG_TM_COMPARE_FORMAT 28
#define MG_TM_COMPARE_SIZE 17

/*
** texture level descriptor
*/
typedef struct __GLMGTexLevInfoRec {
    struct __GLMGTextureInfoRec *tex;	/* Pointer back to texture info structure */
    __GLMGTexLevID id; 			/* identification data */
    TexTransParm *tp;		/* transfer parameters */
    GLshort xoffset, yoffset, zoffset, eoffset; /* offsets declared for load */
    GLshort loadx, loady, loadz, loade; /* sizes declared for load */
    GLushort num;		/* total # tram pages for texture */
    GLushort mmpage, logmmpage;  /* #mipmap pages and log */
    GLushort mmpage_div_z;	/* EMR - 3D texture object fix */
    GLshort brd_page;		/* offset to border page(s) (-1=>none) */
    GLubyte parity;		/* level parity (1==left half in GrpI) */
    GLubyte logssize, logtsize, logrsize, logqsize; /* log of sizes */
    GLubyte lod;		/* LOD level number */
    GLubyte prohibited;		/* prohibited level */
    } __GLMGTexLevInfo;

/*
** texture descriptor
*/
typedef struct __GLMGTextureInfoRec {
    GLint refcount;   	/* reference count: create with 1; delete when 0 */
			/* refcount MUST be first in this structure */
    __GLTexobjFreeFn free;      /* MUST be 2nd; function called to free tex object */
    struct __GLMGDLTexObjRec *dl_tex_obj;
				/* pointer to pull-model DL data for texture */
    struct __GLMGTextureInfoRec *db_partner; /* ptr to double-buffering partner */
    TexTransParm *packed;	/* common representation for packed levels */
    __GLMGTexLevInfo *lev[14];   /* 14 possible levels */
    __GLMGTexLevInfo *somevalid; /* pointer to largest valid level */
    __GLMGTexDetFunc *det_func; /* Detail texture function */
    GLuint name;		/* texture id */
    mddma_cntrlu mddma_cntrl;	/* texture state for MDDMA_CNTRL register */
    txlodu txlod;		/* texture state for TXLOD register */
    txsizeu txsize;		/* texture state for TXSIZE register */
    texmode1u texmode1;		/* texture state for TEXMODE1 register */
    texmode2u texmode2;		/* texture state for TEXMODE2 register */
    GLfloat priority, requestedpriority;	/* priority extension */
    GLfloat requested_det_level; /* detail level requested by user */
    GLfloat vidFieldAdj;	/* value to adjust video fields in ucode */
    GLint qfunc;		/* pixel texture mode */
    GLfloat minlod_c_set;	/* min lod clamping value */
    GLfloat maxlod_c_set;	/* max lod clamping value */
    GLuint baselevel_set; 	/* Support for texture_lod extension */
    GLuint toplevel_set; 	/* Support for texture_lod extension */
    GLenum bcol_rg, bcol_ba;	/* border color */
    GLenum internalformat;	/* internal format assigned by TM */
    GLushort swap[2];		/* swap area base */
    GLushort num;		/* total # tram pages for texture */
    GLshort vidFieldNum;	/* field number of video field loaded */
    GLubyte minlod, maxlod;	/* min and max lod settings */
    GLubyte numcomp;		/* #components in internal format [1:4]=>[0:3] */
    GLubyte compdepth;		/* component depth (bits) in internal format [4/8/12]=>[0/1/2] */
    GLubyte externalbytes;	/* # of bytes per texel in the external format */
    GLubyte hx2;		/* Horizontal/vertical x2 mode */
    GLubyte db_possible;	/* is texture-double-buffering possible */
    GLubyte mm_blend;		/* is mipmap blending enable? */
    GLubyte sacp, tacp, racp;	/* texture wrap mode */
    GLubyte clamp_gl5;		/* old clamping mode */
    GLubyte min, mag;		/* min and mag filter modes */
    GLubyte mipgen;		/* Flag for automatic mipmap generation */
    GLubyte dim;		/* texture dimension [1D:4D]=>[0:3] */
    GLubyte vx2;		/* Horizontal/vertical x2 mode */
    GLubyte mm_enab; 		/* enable mipmapping */
    GLubyte chopped;		/* system borders "chopped" for lack of space */
    GLubyte db_enab; 		/* enable texture double-buffering (within TRAM) */
    GLubyte tram_buffer;	/* 0->lobyte/1=>hibyte (for double buffered) */
    GLubyte select_enab;	/* enable TRAM subsetting (amongst TRAMs) */
    GLubyte tram_select;	/* selects TRAM subset */
    GLubyte compsel_enab;	/* component select: enable */
    GLubyte dual_select;	/* component select: select 1 of 2 textures */
    GLubyte quad_select;	/* component select: select 1 of 4 textures */
    GLubyte detail, det_level;	/* detail flag and scale code */
    GLubyte valid;		/* Is this texture valid for drawing? */
    GLubyte resid;		/* hardware residency for entire texture */
    GLubyte isVidTexture;	/* flag => is this a video texture */
    GLubyte onecomptype;	/* 0=>luminance, 1=>alpha, 2=>intensity */
    } __GLMGTextureInfo;

/*
** texture dimension (1D,2D,etc) descriptor
*/
typedef struct {
    __GLMGTextureInfo tex; 	/* default texture zero */
    __GLMGTextureInfo *curr; 	/* current texture for dimension */
    GLuint dimension;		/* actual dimension value (needed to refresh curr ptr) */
    } __GLMGTexDimInfo;

/*
** texture environment descriptor
*/
typedef struct {
    GLenum mode;		/* Env mode 0:MOD 1:DECAL 2:BLEND 3:ALPHA */
    GLenum ecol_rg, ecol_b;	/* environment color */
    } __GLMGTexEnvInfo;

typedef struct _TM_Alloc {
	TexDesc *desc;
	GLfloat *priority;
	struct _SaveNode *next, *seq;
} TM_Alloc;

/*
** Texture manager region list
*/
typedef struct  _TM_MapRegionList{
struct _TM_MapRegionList * next;
void * region;
size_t  length;
} TM_MapRegionList;

/*
** Memory Arena for amalloc/afree
*/

typedef struct  _TM_Memory_Arena{
    void *  save_arena;        /*arena for mmaped memory*/
    void *  save_arena_alloc;  /* pointer for free      */
    TM_MapRegionList *  mmapregList;  /*addrs alloced*/
    TM_MapRegionList *  mmapaddrList; /*addrs to try*/
} TM_Memory_Arena;



typedef struct _TM_hw {
   TexDesc resid[5]; 		/* Residence representation */ 
   TexDesc open[5]; 		/* List of open areas */
    __GLMGTextureInfo *db_ready; /* ptr to double-buffered available partner */
    __GLMGTextureInfo *draw_setup; /* texture currently setup for drawing */
    __GLMGTexLevInfo *load_setup; /* texture level currently setup for loading */
    TexTransParm unused[2], used1[2], used2[2], *curr_used[2]; /* used lists for dynamic load balancing */
    TexTransParm ovfl;		/* overflow data */
    TexDesc *dstore, **dchunks;	/* allocation store for management structures */
    GLint dfree, numdchunk;	/* counters for allocation store */
    GLint pixel_texture;	/* flag indicating pixel texture enabled */
    GLenum tram_load_subset;	/* load to TRAM subset */
    GLenum tram_load_buffer;	/* load to hi/lo buffer */
    GLenum tram_draw_subset;	/* draw from TRAM subset */
    GLenum tram_draw_buffer;	/* draw from hi/lo buffer */
    __GLnamesArray *namesArray; /* binary tree for texture object storage */
    long long share_id;		/* share-group ID from global tex mgr */
    GLfloat priority[2];	/* priority values */
    GLint opportunity[2];	/* signal for possible open space for texture placement */
    GLint quiescent[2];		/* signal for swapping textures to stop looking for new placement */
    GLint thrashing[2];		/* signal that texture looked for new placement and failed */
    GLint download;		/* signal that download has occurred */
    GLuint TLBversion;		/* global comparison value to trigger TLB size update */
    GLushort end[2];		/* end of space */
    GLushort saveall;		/* indication to save all data immediately */
    GLushort numdownloaded;	/* statistics */
    TM_Memory_Arena mem_arena;  /* arena for memory allocation*/
} TM_hw;

/*
** texture manager descriptor
*/
typedef struct {
    __GLMGTexDimInfo *enab; 	/* ptr to current dimension */
    __GLMGTexDimInfo dim[9]; 	/* 1D, 2D, 3D, 4D, DET, and PROXY structures */
    __GLMGTexEnvInfo env; 	/* texture environment structures */
    __GLMGTexDetFunc det_func, *curr_det_func;	/* Detail texture func descriptors */
    TM_hw *hw;			/* Local TM (perhaps shared) hardware description */
    GLboolean basereg_is_set;	/* true if this context's tex base register is initialized */
    mgras_dma_edit_release_args release_args;
    mgras_dma_edit_append_args append_args;
    mgras_dma_edit_replace_args dmaReplaceArgs;
    GLint hq_ge_valid;		/* flag indicating HQ3/GE11 valid for texturing */
    GLenum num_tram;		/* # of TRAM's in system [124]=>[012] */
    GLint pixel_tex_areplace;	/* pixel texture alpha replace */
    GLint pixel_tex_aselect;	/* pixel texture alpha select */
    GLuint TLBversion;		/* local comparison value to trigger TLB size update */
    } __GLMGTexManInfo;

#endif /* __glmgrastm_h_ */
