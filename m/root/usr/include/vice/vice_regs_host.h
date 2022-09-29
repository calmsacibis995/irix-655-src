/*
 * VICE register #defines specific for host access.
 */
/*
 * Automatically generated from viceregs.t:
 * $Revision: 1.4 $ $Date: 1997/07/02 20:15:30 $
 */
#define VICEMSP_BASE                   0x0000
#define VICEMSP_IRAM                   0x2000
#define VICEBSP_IRAM                   0x4000
#define VICEBSP_TABLE_BASE             0x5000
#define VICEBSP_OUT_FIFO               0x7000
#define VICEBSP_IN_FIFO                0x7800
#define VICEMSP_DRAM                   0x8000
#define VICEMSP_DRAM_A                 0x8000
#define VICEMSP_DRAM_B                 0x8800
#define VICEMSP_DRAM_C                 0x9000
#define VICEMSP_TLB_BASE               0xf000
#define VICEMSP_SYS_BASE               0x800000
#define VICEMSP_SYS_GRPH_BASE          0x10800000
#define VICEMSP_SYS_BYTES              0x400000
#define VICEMSP_DRAM_SIZE              0x1800
#define VICEMSP_IRAM_SIZE              0x1000
#define VICEBSP_IRAM_SIZE              0x0800
#define VICEBSP_IN_BYTES               0x0040
#define VICEBSP_OUT_BYTES              0x0040
#define VICEBSP_NTABLE_ENTRIES         0x0500
#define VICE_NTLBENTRIES               0x0080
#define VICE_ID                        0x0008
#define HST_BSP_IN_BOX                 0x0028
#define HST_BSP_OUT_BOX                0x0030
#define MSP_CTL_STAT                   0x0040
#define MSP_ExcpFlag                   0x0048
#define MSP_PC                         0x0050
#define MSP_BadAddr                    0x0058
#define MSP_WatchPoint                 0x0060
#define MSP_EPC                        0x0068
#define MSP_CAUSE                      0x0070
#define BSP_RPAGE                      0x0078
#define BSP_SW_INT                     0x0080
#define VICE_CFG                       0xe000
#define VICE_INT_RESET                 0xe008
#define VICE_INT_EN                    0xe010
#define VICEMSPCMD_HALT                0x0001
#define VICEMSPCMD_RESET               0x0002
#define VICEMSPCMD_BUSY                0x0004
#define VICEMSPCMD_1STEP               0x0008
#define MSP_CAUSE_AdEL                 0x0000
#define MSP_CAUSE_AdES                 0x0001
#define MSP_CAUSE_BP                   0x0002
#define MSP_CAUSE_WP                   0x0003
#define MSP_CAUSE_SuRI                 0x0004
#define MSP_CAUSE_VuRI                 0x0005
#define MSP_CAUSE_Con                  0x0006
#define MSP_CAUSE_AdEI                 0x0007
#define MSP_ExcpFlag_AdEL              0x0001
#define MSP_ExcpFlag_AdES              0x0002
#define MSP_ExcpFlag_BP                0x0004
#define MSP_ExcpFlag_WP                0x0008
#define MSP_ExcpFlag_SuRI              0x0010
#define MSP_ExcpFlag_VuRI              0x0020
#define MSP_ExcpFlag_Con               0x0040
#define MSP_ExcpFlag_AdEI              0x0080
#define MSP_D_RAM                      0x0100
#define VICE_COUNT                     0x0108
#define BSP_CTL_STAT                   0x0110
#define BSP_WatchPoint                 0x0118
#define BSP_IN_COUNT                   0x0120
#define BSP_OUT_COUNT                  0x0128
#define BSP_PC                         0x0140
#define BSP_EPC                        0x0148
#define BSP_HALT_RESET                 0x0150
#define BSP_CAUSE                      0x0158
#define VICE_INT                       0x0160
#define BSP_FIFO_CTL_STAT              0x0168
#define BSP_AVALID_BITS                0x0170
#define BSP_FVALID_BITS                0x0178
#define VICEDMA_CTL_CH1                0x0180
#define VICEDMA_STAT_CH1               0x0188
#define VICEDMA_DATA_CH1               0x0190
#define VICEDMA_MEM_PT_CH1             0x0198
#define VICEDMA_VICE_PT_CH1            0x01a0
#define VICEDMA_COUNT_CH1              0x01a8
#define MSP_SW_INT                     0x01b8
#define VICEDMA_CTL_CH2                0x01c0
#define VICEDMA_STAT_CH2               0x01c8
#define VICEDMA_DATA_CH2               0x01d0
#define VICEDMA_MEM_PT_CH2             0x01d8
#define VICEDMA_VICE_PT_CH2            0x01e0
#define VICEDMA_COUNT_CH2              0x01e8
#define BSP_IN_BOX                     0x01f0
#define BSP_OUT_BOX                    0x01f8
#define VICEDMA_CTL_CH1_D1             0x1000
#define VICEDMA_SMEM_HI_CH1_D1         0x1008
#define VICEDMA_SMEM_LO_CH1_D1         0x1010
#define VICEDMA_WIDTH_CH1_D1           0x1018
#define VICEDMA_STRIDE_CH1_D1          0x1020
#define VICEDMA_LINES_CH1_D1           0x1028
#define VICEDMA_VMEM_Y_CH1_D1          0x1030
#define VICEDMA_VMEM_C_CH1_D1          0x1038
#define VICEDMA_CTL_CH1_D2             0x1040
#define VICEDMA_SMEM_HI_CH1_D2         0x1048
#define VICEDMA_SMEM_LO_CH1_D2         0x1050
#define VICEDMA_WIDTH_CH1_D2           0x1058
#define VICEDMA_STRIDE_CH1_D2          0x1060
#define VICEDMA_LINES_CH1_D2           0x1068
#define VICEDMA_VMEM_Y_CH1_D2          0x1070
#define VICEDMA_VMEM_C_CH1_D2          0x1078
#define VICEDMA_CTL_CH1_D3             0x1080
#define VICEDMA_SMEM_HI_CH1_D3         0x1088
#define VICEDMA_SMEM_LO_CH1_D3         0x1090
#define VICEDMA_WIDTH_CH1_D3           0x1098
#define VICEDMA_STRIDE_CH1_D3          0x10a0
#define VICEDMA_LINES_CH1_D3           0x10a8
#define VICEDMA_VMEM_Y_CH1_D3          0x10b0
#define VICEDMA_VMEM_C_CH1_D3          0x10b8
#define VICEDMA_CTL_CH1_D4             0x10c0
#define VICEDMA_SMEM_HI_CH1_D4         0x10c8
#define VICEDMA_SMEM_LO_CH1_D4         0x10d0
#define VICEDMA_WIDTH_CH1_D4           0x10d8
#define VICEDMA_STRIDE_CH1_D4          0x10e0
#define VICEDMA_LINES_CH1_D4           0x10e8
#define VICEDMA_VMEM_Y_CH1_D4          0x10f0
#define VICEDMA_VMEM_C_CH1_D4          0x10f8
#define VICEDMA_CTL_CH2_D1             0x1100
#define VICEDMA_SMEM_HI_CH2_D1         0x1108
#define VICEDMA_SMEM_LO_CH2_D1         0x1110
#define VICEDMA_WIDTH_CH2_D1           0x1118
#define VICEDMA_STRIDE_CH2_D1          0x1120
#define VICEDMA_LINES_CH2_D1           0x1128
#define VICEDMA_VMEM_Y_CH2_D1          0x1130
#define VICEDMA_VMEM_C_CH2_D1          0x1138
#define VICEDMA_CTL_CH2_D2             0x1140
#define VICEDMA_SMEM_HI_CH2_D2         0x1148
#define VICEDMA_SMEM_LO_CH2_D2         0x1150
#define VICEDMA_WIDTH_CH2_D2           0x1158
#define VICEDMA_STRIDE_CH2_D2          0x1160
#define VICEDMA_LINES_CH2_D2           0x1168
#define VICEDMA_VMEM_Y_CH2_D2          0x1170
#define VICEDMA_VMEM_C_CH2_D2          0x1178
#define VICEDMA_CTL_CH2_D3             0x1180
#define VICEDMA_SMEM_HI_CH2_D3         0x1188
#define VICEDMA_SMEM_LO_CH2_D3         0x1190
#define VICEDMA_WIDTH_CH2_D3           0x1198
#define VICEDMA_STRIDE_CH2_D3          0x11a0
#define VICEDMA_LINES_CH2_D3           0x11a8
#define VICEDMA_VMEM_Y_CH2_D3          0x11b0
#define VICEDMA_VMEM_C_CH2_D3          0x11b8
#define VICEDMA_CTL_CH2_D4             0x11c0
#define VICEDMA_SMEM_HI_CH2_D4         0x11c8
#define VICEDMA_SMEM_LO_CH2_D4         0x11d0
#define VICEDMA_WIDTH_CH2_D4           0x11d8
#define VICEDMA_STRIDE_CH2_D4          0x11e0
#define VICEDMA_LINES_CH2_D4           0x11e8
#define VICEDMA_VMEM_Y_CH2_D4          0x11f0
#define VICEDMA_VMEM_C_CH2_D4          0x11f8
#define BSP_BOX_READY                  0x8000
#define VICEBSPCS_RESET                0x0001
#define VICEBSPCS_HALT                 0x0002
#define VICEBSPCS_HALT_ACK             0x0004
#define VICEBSPFIFO_EMPTY              0x0001
#define VICEBSPFIFO_FULL               0x0002
#define VICEBSPFIFO_RESET              0x0004
#define VICEBSPFIFO_MSPSIG             0x0008
#define VICEBSPFIFO_BSPSIG             0x0010
#define VICE_INT_DMA1_DONE             0x0001
#define VICE_INT_DMA1_ERR              0x0002
#define VICE_INT_MSP_INTR              0x0004
#define VICE_INT_MSP_EXC               0x0008
#define VICE_INT_BSP_INTR              0x0010
#define VICE_INT_BSP_EXC               0x0020
#define VICE_INT_SYSADERR              0x0040
#define VICE_INT_DMA2_DONE             0x0080
#define VICE_INT_DMA2_ERR              0x0100
#define VICEDMA_CTL_GO                 0x0001
#define VICEDMA_CTL_IE                 0x0002
#define VICEDMA_CTL_STOP               0x0004
#define VICEDMA_CTL_RESET              0x0008
#define VICEDMA_CTL_DESCR_PT0          0x0010
#define VICEDMA_CTL_DESCR_PT1          0x0020
#define VICEDMA_CTL_DESCR_PT2          0x0040
#define VICEDMA_CTL_DESCR_PT3          0x0080
#define VICEDMA_CTL_DESCR_MASK         0x00f0
#define VICEDMA_CTL_TLB_BYP            0x0100
#define VICEDMA_CTL_FLUSH              0x0200
#define VICEDMA_CTL_HPEL422            0x0400
#define VICEDMA_STAT_DONE              0x0001
#define VICEDMA_STAT_ERROR             0x0002
#define VICEDMA_STAT_ACTIVE            0x0004
#define VICEDMA_STAT_RW                0x0008
#define VICEDMA_STAT_DESCR_PT0         0x0010
#define VICEDMA_STAT_DESCR_PT1         0x0020
#define VICEDMA_STAT_DESCR_PT2         0x0040
#define VICEDMA_STAT_DESCR_PT3         0x0080
#define VICEDMA_STAT_DESCR_MASK        0x00f0
#define VICEDMA_STAT_CODE              0x0f00
#define VICEDMA_STAT_CODE_IDLE         0x0000
#define VICEDMA_STAT_CODE_STOP_RESET   0x0100
#define VICEDMA_STAT_CODE_HALT         0x0200
#define VICEDMA_STAT_CODE_ADDR         0x0300
#define VICEDMA_STAT_CODE_ADDR_WAIT    0x0400
#define VICEDMA_STAT_CODE_DATA_WAIT    0x0500
#define VICEDMA_STAT_CODE_DATA_MOVE    0x0600
#define VICEDMA_STAT_CODE_TLB_MISS     0x0700
#define VICEDMA_STAT_CODE_TLB_MOD      0x0800
#define VICEDMA_DCTL_CHP               0x0003
#define VICEDMA_DCTL_CHP_FV_FH         0x0000
#define VICEDMA_DCTL_CHP_FV_HH         0x0001
#define VICEDMA_DCTL_CHP_HV_FH         0x0002
#define VICEDMA_DCTL_CHP_HV_HH         0x0003
#define VICEDMA_CHROMA_ONLY            0x0004
#define VICEDMA_DCTL_HPEN              0x0008
#define VICEDMA_DCTL_TOPFIELD          0x0010
#define VICEDMA_DCTL_VERT_MV_NEG       0x0040
#define VICEDMA_DCTL_FIELDMODE         0x0080
#define VICEDMA_DCTL_LHP               0x0300
#define VICEDMA_DCTL_LHP_FV_FH         0x0000
#define VICEDMA_DCTL_LHP_FV_HH         0x0100
#define VICEDMA_DCTL_LHP_HV_FH         0x0200
#define VICEDMA_DCTL_LHP_HV_HH         0x0300
#define VICEDMA_DCTL_LOC               0x0070
#define VICEDMA_DCTL_LOC_DRAM_A        0x0000
#define VICEDMA_DCTL_LOC_DRAM_B        0x0010
#define VICEDMA_DCTL_LOC_DRAM_C        0x0020
#define VICEDMA_DCTL_LOC_IRAM_MSP      0x0030
#define VICEDMA_DCTL_LOC_IRAM_BSP      0x0040
#define VICEDMA_DCTL_LOC_TRAM_BSP      0x0050
#define VICEDMA_DCTL_LOC_BITS_BSP      0x0060
#define VICEDMA_DCTL_LOC_TLB           0x0070
#define VICEDMA_DCTL_YC                0x0c00
#define VICEDMA_DCTL_YC_BLK            0x0000
#define VICEDMA_DCTL_YC_420            0x0400
#define VICEDMA_DCTL_YC_422            0x0800
#define VICEDMA_DCTL_YC_YONLY          0x0c00
#define VICEDMA_DCTL_FILL              0x1000
#define VICEDMA_DCTL_RW                0x2000
#define VICEDMA_DCTL_SKIP              0x4000
#define VICEDMA_DCTL_HALT              0x8000
#define MSP_RESET_INTERVAL             0x0010
#define VICEMSPCMD_STEPPING            0x0040
#define VICEMSPCMD_BREAKPOINT          0x0080
#define VICEMSPCMD_INTRHOST            0x0010
#define VICEMSPCMD_MSPGO               0x0020
#define VICEMSP_VREG_BASE              0x11000
#define VICEMSP_VCC                    0x11200
#define VICEMSP_VCO                    0x11204
#define VICEMSP_VECTOR_WIDTH           0x0008
#define VICESIM_CMD                    0x11fe8
#define VICESIM_CMD2                   0x11fe0
#define TRACE_SU_I                     0x0001
#define TRACE_VU_I                     0x0002
#define TRACE_SU_R                     0x0004
#define TRACE_VU_R                     0x0008
#define TRACE_MSP_ALL                  0x0010
#define TRACE_MSP_ALL_SWITCH           0x0020
#define TRACE_BSP_ALL                  0x0040
#define TRACE_BSP_ALL_SWITCH           0x0080
#define TRACE_BSP_DISPLAY              0x0100
#define TRACE_LOW_ACC                  0x0200
#define VICEBSP_BUSY                   0x11ff0
#define VICEMSP_BREAKADDR              0x11ff8
#define VICEMSP_SREG_BASE              0x12000
#define _MSP_HW_EXC                    0x12f00
#define _BSP_HW_EXC                    0x12f08
#define _DMA1_DONE                     0x12f10
#define _DMA1_ERR                      0x12f18
#define _SYSAD_ERR                     0x12f20
#define _BSP_IN_INDEX                  0x12f28
#define _BSP_IN_TOP                    0x12f30
#define _BSP_OUT_INDEX                 0x12f38
#define _BSP_OUT_TOP                   0x12f40
#define _DMA2_DONE                     0x12f48
#define _DMA2_ERR                      0x12f50
#define VICE_END                       0x13000
