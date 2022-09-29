/*
 * Derive some DMA register access macros
 */

/*
 * 2 channels, c == [12]
 */
#define	CBASE(c) (c == 1 ? VICEDMA_CTL_CH1 : VICEDMA_CTL_CH2)
#define	CTL(c) (CBASE(c))
#define	STAT(c) (CBASE(c) + VICEDMA_STAT_CH1 - VICEDMA_CTL_CH1)
#define	DATA(c) (CBASE(c) + VICEDMA_DATA_CH1 - VICEDMA_CTL_CH1)
#define	MEM_PT(c) (CBASE(c) + VICEDMA_MEM_PT_CH1 - VICEDMA_CTL_CH1)
#define	VICE_PT(c) (CBASE(c) + VICEDMA_VICE_PT_CH1 - VICEDMA_CTL_CH1)
#define	COUNT(c) (CBASE(c) + VICEDMA_COUNT_CH1 - VICEDMA_CTL_CH1)

/*
 * 4 descriptors per channel, d == [1234]
 */
#define DSIZE   (VICEDMA_CTL_CH1_D2 - VICEDMA_CTL_CH1_D1)
#define DBASE(c,d) (VICEDMA_CTL_CH1_D1 + DSIZE * 4 * (c - 1) + DSIZE * (d - 1))
#define DCTL(c,d) (DBASE(c,d))
#define SMEM_HI(c,d) (DBASE(c,d) + VICEDMA_SMEM_HI_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define SMEM_LO(c,d) (DBASE(c,d) + VICEDMA_SMEM_LO_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define WIDTH(c,d) (DBASE(c,d) + VICEDMA_WIDTH_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define STRIDE(c,d) (DBASE(c,d) + VICEDMA_STRIDE_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define LINES(c,d) (DBASE(c,d) + VICEDMA_LINES_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define VMEM_Y(c,d) (DBASE(c,d) + VICEDMA_VMEM_Y_CH1_D1 - VICEDMA_CTL_CH1_D1)
#define VMEM_C(c,d) (DBASE(c,d) + VICEDMA_VMEM_C_CH1_D1 - VICEDMA_CTL_CH1_D1)
