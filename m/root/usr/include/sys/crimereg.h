#ifndef __GL_CRIMEREG_H__
#define __GL_CRIMEREG_H__
/*
**
** crimereg.h - CRIME chip interface registers
**
*/

#include "crimedef.h"

#if !defined(_KERNEL) && !defined(X11R6) && !defined(_STANDALONE)
#define CRMREG_BITFIELDS
#endif

/*****************************************/
/* Some useful types */

#ifdef EASY_TO_READ
typedef struct {
    u_long r:8;
    u_long g:8;
    u_long b:8;
    u_long a:8;
} ColorType;
#endif

typedef u_long ColorType;

#ifdef CRMREG_BITFIELDS
typedef struct {
    int x:16;
    int y:16;
} CrmFbAddrType; 
#else
typedef u_long CrmFbAddrType;
#endif

#ifdef CRMREG_BITFIELDS
typedef union {
    CrmFbAddrType fb;
    u_long linear;
} CrmVaddrType;
#else
typedef u_long CrmVaddrType;
#endif


/*****************************************/
/* Programming interface registers */
/*****************************************/


/***********************************************/
/* TLB register typedefs */
/*****************************************/

typedef union {
    u_short	taddr[4];
    u_long	laddr[2];
    long long	dw;
} CrmTlbType;
 
typedef struct { 
    volatile CrmTlbType	fbA[64]; 
    volatile CrmTlbType	fbB[64]; 
    volatile CrmTlbType	fbC[64];
    volatile CrmTlbType	tex[28];
    volatile CrmTlbType	cid[4];
    volatile CrmTlbType	linearA[16];
    volatile CrmTlbType	linearB[16];
} CrmTlbReg;


/***********************************************/
/* Interface buffer register typedefs */
/*****************************************/

typedef struct {
    volatile u_long w[2];
} CrmIntfBufData;

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved1:18;
    u_long start:1;
    u_long wmask:2;
    u_long pageId:3;
    u_long offset:8;
    u_long reserved2;
} CrmIntfBufAddr;

typedef struct {
    u_long reserved:4;
    u_long fullLevel:7;
    u_long emptyLevel:7;
    u_long stallLevel:7;
    u_long stallCount:7;
} CrmIntfBufCtlReg;

#else
typedef unsigned long long CrmIntfBufAddr;

#define CRMIBADDR_OFFSET(s) (((s) >> 32) & 0xff)
#define CRMIBADDR_PAGE(s) (((s) >> 40) & 7)
#define CRMIBADDR_WMASK(s) (((s) >> 43) & 3)
#define CRMIBADDR_START(s) (((s) >> 45) & 1)
#define CRMIBADDR_TO_PHYS(s) \
	(CRMIBADDR_PAGE(s) << 12 | CRMIBADDR_OFFSET(s) << 3)

typedef unsigned long CrmIntfBufCtlReg;

#define CRMIBCTL_FULL_MASK  ((1<<7)-1)
#define CRMIBCTL_FULL_SHIFT 21
#define CRMIBCTL_FULL(s) \
	(((s) >> CRMIBCTL_FULL_SHIFT) & CRMIBCTL_FULL_MASK)

#define CRMIBCTL_EMPTY_MASK  ((1<<7)-1)
#define CRMIBCTL_EMPTY_SHIFT 14
#define CRMIBCTL_EMPTY(s) \
	(((s) >> CRMIBCTL_EMPTY_SHIFT) & CRMIBCTL_EMPTY_MASK)

#define CRMIBCTL_STALL_LEVEL_MASK  ((1<<7)-1)
#define CRMIBCTL_STALL_LEVEL_SHIFT 7
#define CRMIBCTL_STALL_LEVEL(s) \
	(((s) >> CRMIBCTL_STALL_LEVEL_SHIFT) & CRMIBCTL_STALL_LEVEL_MASK)

#define CRMIBCTL_STALL_COUNT_MASK  ((1<<7)-1)
#define CRMIBCTL_STALL_COUNT_SHIFT 0
#define CRMIBCTL_STALL_COUNT(s) \
	(((s) >> CRMIBCTL_STALL_COUNT_SHIFT) & CRMIBCTL_STALL_COUNT_MASK)

#endif

#define CRIME_FIFO_DEPTH 64
typedef struct {
    volatile CrmIntfBufData 	data[CRIME_FIFO_DEPTH];
    volatile CrmIntfBufAddr 	addr[CRIME_FIFO_DEPTH];
    volatile CrmIntfBufCtlReg 	ctl;
    volatile u_long              reserved1;
    volatile u_long              reset;
    volatile u_long              reserved2;
} CrmIntfBufReg;

/***********************************************/
/* Pixel pipe register typedefs */
/*****************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved:19;
    u_long bufType:3;
    u_long bufDepth:2;
    u_long pixType:4;
    u_long pixDepth:2;
    u_long doublePix:1;
    u_long doublePixSel:1;
} CrmBufModeType;
#else
typedef u_long CrmBufModeType;
#endif
typedef struct {
    volatile CrmBufModeType src;
    volatile int align1;
    volatile CrmBufModeType dst;
    volatile int align2;
} CrmBufModeReg;


/*****************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved:20;
    u_long enCid:1;
    u_long cidMapSel:1;
    u_long enScrMask:5;
    u_long scrMaskMode:5;
} CrmClipModeReg;

#else
typedef u_long CrmClipModeReg;
#endif

/*****************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved:8;
    u_long enNoConflict:1;
    u_long enGL:1;
    u_long enPixelXfer:1;
    u_long enScissorTest:1;
    u_long enLineStipple:1;
    u_long enPolyStipple:1;
    u_long enOpaqStipple:1;
    u_long enShade:1;
    u_long enTexture:1;
    u_long enFog:1;
    u_long enCoverage:1;
    u_long enAntialiasLine:1;
    u_long enAlphaTest:1;
    u_long enBlend:1;
    u_long enLogicOp:1;
    u_long enDither:1;
    u_long enColorMask:1;
    u_long enColorByteMask:4;
    u_long enDepthTest:1;
    u_long enDepthMask:1;
    u_long enStencilTest:1;
} CrmDrawModeReg;

#else
typedef u_long CrmDrawModeReg;
#endif

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    volatile CrmFbAddrType min;
    volatile CrmFbAddrType max;
} CrmClipRectReg;
#else
typedef unsigned long long CrmClipRectReg;
#endif
/*****************************************/

typedef struct {
    volatile CrmFbAddrType src;
    volatile int align1;
    volatile CrmFbAddrType dst;
    volatile int align2;
} CrmWinOffsetReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long opCode:8;
    u_long reserved:5;
    u_long lineSkipLastEP:1;
    u_long edgeType:2;
    u_long lineWidth:16;
} CrmPrimitiveReg;

#else
typedef u_long CrmPrimitiveReg;
#endif

/*****************************************/

typedef CrmFbAddrType CrmVertexXReg;

typedef struct {
    volatile u_long x;
    volatile u_long y;
} CrmVertexGLReg;

typedef struct {
    volatile CrmVertexXReg  X[3];
    volatile int align;
    volatile CrmVertexGLReg GL[3];
} CrmVertexReg;
/*****************************************/

typedef u_long CrmStartSetupReg;

/*****************************************/

typedef struct {
    volatile CrmVaddrType addr;
    volatile int align1;
    volatile int xStep;
    volatile int yStep;
} CrmPixelXferSrcReg;

typedef struct {
    volatile int linAddr;
    volatile int linStride;
} CrmPixelXferDstReg;

typedef struct {
    volatile CrmPixelXferSrcReg src;
    volatile CrmPixelXferDstReg dst;
} CrmPixelXferReg;


/*****************************************/

#ifdef EASY_TO_READ
typedef struct {
    u_long index:8;
    u_long maxIndex:8;
    u_long repeatCnt:8;
    u_long maxRepeat:8;
} CrmStippleModeReg;
#endif
typedef u_long CrmStippleModeReg;

typedef struct {
    volatile CrmStippleModeReg mode;
    volatile u_long pattern;
} CrmStippleReg;

/*****************************************/

typedef struct {
    volatile ColorType fgColor;
    volatile int align1;

    volatile ColorType bgColor;
    volatile int align2;

    volatile int r0;
    volatile int g0;
    volatile int b0;
    volatile int a0;
    volatile int drdx;
    volatile int dgdx;
    volatile int drdy;
    volatile int dgdy;
    volatile int dbdx;
    volatile int dadx;
    volatile int dbdy;
    volatile int dady;
} CrmShadeReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long reserved:7;
    u_long tiled:1;
    u_long texelType:4;
    u_long texelDepth:2;
    u_long mapLevel:4;
    u_long maxLevel:4;
    u_long minFilter:3;
    u_long magFilter:1;
    u_long wrapS:2;
    u_long wrapT:2;
    u_long func:2;
} CrmTextureModeReg;
#else
typedef u_long CrmTextureModeReg;
#endif

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long reserved:16;
    u_long uShift:4;
    u_long vShift:4;
    u_long uWrapShift:4;
    u_long vWrapShift:4;
    u_long uIndexMask:16;
    u_long vIndexMask:16;
} CrmTextureFormatReg;
#else
typedef unsigned long long CrmTextureFormatReg;
#endif

typedef struct {
    volatile CrmTextureModeReg 	mode;
    volatile int align1;

    volatile CrmTextureFormatReg format;

    volatile long long 	sq0;
    volatile long long 	tq0;
    volatile int 	q0;
    volatile int 	stShift;
    volatile long long 	dsqdx;
    volatile long long 	dsqdy;
    volatile long long 	dtqdx;
    volatile long long 	dtqdy;
    volatile int 	dqdx;
    volatile int 	dqdy;

    volatile ColorType borderColor;
    volatile int align2;

    volatile ColorType envColor;
    volatile int align3;
} CrmTextureReg;

/*****************************************/

typedef struct {
    volatile ColorType color;
    volatile int align1;

    volatile int f0;
    volatile int align2;
    volatile int dfdx;
    volatile int align3;
    volatile int dfdy;
    volatile int align4;
} CrmFogReg;

/*****************************************/

typedef struct {
    int slope:16;
    int ideal:16;
} CrmAntialiasLineReg;

typedef struct {
    u_long reserved:16;
    u_long start:8;
    u_long end:8;
} CrmAntialiasCovReg;

typedef struct {
    volatile CrmAntialiasLineReg line;
    volatile CrmAntialiasCovReg  cov;
} CrmAntialiasReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long reserved:20;
    u_long func:4;
    u_long ref:8;
} CrmAlphaTestReg;
#else
typedef u_long CrmAlphaTestReg;
#endif

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long reserved:20;
    u_long op:4;
    u_long src:4;
    u_long dst:4;
} CrmBlendFuncReg;
#else
typedef u_long CrmBlendFuncReg;
#endif

typedef struct {
    volatile ColorType constColor;
    volatile int align1;
    volatile CrmBlendFuncReg func;
    volatile int align2;
} CrmBlendReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long reserved:28;
    u_long op:4;
} CrmLogicOpReg;
#else
typedef u_long CrmLogicOpReg;
#endif

/*****************************************/

typedef u_long CrmColorMaskReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct { 
    u_long reserved:4;
    u_long func:3;
    u_long enTagClear:1;
    u_long clear:24;
} CrmDepthMode;
#else
typedef u_long CrmDepthMode;
#endif

typedef struct {
    volatile CrmDepthMode mode;
    volatile int align1;

    volatile long long z0;
    volatile long long dzdx;
    volatile long long dzdy;
} CrmDepthReg;

/*****************************************/

#ifdef CRMREG_BITFIELDS
typedef struct {
    u_long ref:8;
    u_long mask:8;
    u_long func:4;
    u_long sfail:4;
    u_long dpfail:4;
    u_long dppass:4;
} CrmStencilModeReg;
#else
typedef u_long CrmStencilModeReg;
#endif

typedef struct { 
    volatile CrmStencilModeReg mode;
    volatile int align;
    volatile u_long mask;
} CrmStencilReg;

/*****************************************/

typedef u_long CrmPixPipeNullReg;
typedef u_long CrmPixPipeFlushReg;

/*****************************************/
#ifdef EASY_TO_READ
typedef struct { 
    CrmBufModeReg   	BufMode;
    CrmClipModeReg     	ClipMode;
    CrmDrawModeReg     	DrawMode;
    CrmClipRectReg      ScrMask[5];
    CrmClipRectReg      Scissor;
    CrmWinOffsetReg     WinOffset;
    CrmPrimitiveReg     Primitive;
    CrmVertexReg     	Vertex;
    CrmPixelXferReg     PixelXfer;
    CrmStippleReg      	Stipple;
    CrmShadeReg        	Shade;
    CrmTextureReg      	Texture;
    CrmFogReg          	Fog;
    CrmAntialiasReg     Antialias;
    CrmAlphaTestReg    	AlphaTest;
    CrmBlendReg        	Blend;
    CrmLogicOpReg      	LogicOp;
    CrmColorMaskReg    	ColorMask;
    CrmDepthReg        	Depth;
    CrmStencilReg      	Stencil;
} CrmDrawReg;
#endif

/*****************************************/
/* MTE register typedefs */
/*****************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved:20;
    u_long opCode:1;
    u_long enStipple:1;
    u_long pixDepth:2;
    u_long srcBufType:3;
    u_long dstBufType:3;
    u_long srcECC:1;
    u_long dstECC:1;
} CrmMteModeReg;

#else
typedef u_long CrmMteModeReg;
#endif

typedef struct {
    volatile CrmMteModeReg       mode;
    volatile int align1;
    volatile u_long		byteMask;
    volatile int align2;
    volatile u_long		stippleMask;
    volatile int align3;
    volatile u_long		fgValue;
    volatile int align4;
    volatile CrmVaddrType        src0;
    volatile int align5;
    volatile CrmVaddrType        src1;
    volatile int align6;
    volatile CrmVaddrType        dst0;
    volatile int align7;
    volatile CrmVaddrType        dst1;
    volatile int align8;
    volatile int        		srcYStep;
    volatile int align9;
    volatile int        		dstYStep;
    volatile int align10[9];
    volatile int			null;
    volatile int align11;
    volatile int			flush;
} CrmMteReg;
/***********************************************/
/* Status registers */
/***********************************************/

#ifdef CRMREG_BITFIELDS

typedef struct {
    u_long reserved:3;
    u_long reIdle:1;
    u_long setupIdle:1;
    u_long pixPipeIdle:1;
    u_long mteIdle:1;
    u_long intfBufLevel:7;
    u_long intfBufRdPtr:6;
    u_long intfBufWrPtr:6;
    u_long intfBufStartPtr:6;
} CrmStatusReg;

#else

typedef unsigned long CrmStatusReg;

#define CRMSTAT_RE_IDLE		(1 << 28)

#define CRMSTAT_IB_LEVEL_SHIFT  18
#define CRMSTAT_IB_LEVEL_MASK	0x7f
#define CRMSTAT_IB_LEVEL(s)	(((s) >> CRMSTAT_IB_LEVEL_SHIFT) &\
					 CRMSTAT_IB_LEVEL_MASK)
#define CRMSTAT_IB_RDPTR_SHIFT  12
#define CRMSTAT_IB_RDPTR_MASK	0x3f
#define CRMSTAT_IB_RDPTR(s)	(((s) >> CRMSTAT_IB_RDPTR_SHIFT) &\
					 CRMSTAT_IB_RDPTR_MASK)
#define CRMSTAT_IB_WRPTR_SHIFT  6
#define CRMSTAT_IB_WRPTR_MASK	0x3f
#define CRMSTAT_IB_WRPTR(s)	(((s) >> CRMSTAT_IB_WRPTR_SHIFT) &\
					 CRMSTAT_IB_WRPTR_MASK)
#define CRMSTAT_IB_STPTR_SHIFT  0
#define CRMSTAT_IB_STPTR_MASK	0x3f
#define CRMSTAT_IB_STPTR(s)	(((s) >> CRMSTAT_IB_STPTR_SHIFT) &\
					 CRMSTAT_IB_STPTR_MASK)
#endif

typedef unsigned long CrmSetStartPtrReg;

#endif /* __GL_CRIMEREG_H__ */
