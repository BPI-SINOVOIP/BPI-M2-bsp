/*
 *  arch/arm/mach-sun6i/clock/ccm.c
 *
 * Copyright (c) 2012 Allwinner.
 * kevin.z.m (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <mach/platform.h>
#include <mach/clock.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include "ccm_i.h"

#define make_aw_clk_inf(clk_id, clk_name)   {.id = clk_id, .name = clk_name}
__ccmu_reg_list_t       *aw_ccu_reg;
__ccmu_reg_cpu0_list_t  *aw_cpus_reg;

__aw_ccu_clk_t aw_ccu_clk_tbl[] =
{
    make_aw_clk_inf(AW_SYS_CLK_NONE,        "sys_none"          ),
    make_aw_clk_inf(AW_SYS_CLK_LOSC,        CLK_SYS_LOSC        ),
    make_aw_clk_inf(AW_SYS_CLK_HOSC,        CLK_SYS_HOSC        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL1,        CLK_SYS_PLL1        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2,        CLK_SYS_PLL2        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3,        CLK_SYS_PLL3        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL4,        CLK_SYS_PLL4        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL5,        CLK_SYS_PLL5        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6,        CLK_SYS_PLL6        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7,        CLK_SYS_PLL7        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL8,        CLK_SYS_PLL8        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL9,        CLK_SYS_PLL9        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL10,       CLK_SYS_PLL10       ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2X8,      CLK_SYS_PLL2X8      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3X2,      CLK_SYS_PLL3X2      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6x2,      CLK_SYS_PLL6X2      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7X2,      CLK_SYS_PLL7X2      ),
    make_aw_clk_inf(AW_SYS_CLK_MIPIPLL,     CLK_SYS_MIPI_PLL    ),
    make_aw_clk_inf(AW_SYS_CLK_AC327,       CLK_SYS_AC327       ),
    make_aw_clk_inf(AW_SYS_CLK_AR100,       CLK_SYS_AR100       ),
    make_aw_clk_inf(AW_SYS_CLK_AXI,         CLK_SYS_AXI         ),
    make_aw_clk_inf(AW_SYS_CLK_AHB0,        CLK_SYS_AHB0        ),
    make_aw_clk_inf(AW_SYS_CLK_AHB1,        CLK_SYS_AHB1        ),
    make_aw_clk_inf(AW_SYS_CLK_APB0,        CLK_SYS_APB0        ),
    make_aw_clk_inf(AW_SYS_CLK_APB1,        CLK_SYS_APB1        ),
    make_aw_clk_inf(AW_SYS_CLK_APB2,        CLK_SYS_APB2        ),
    make_aw_clk_inf(AW_CCU_CLK_NULL,        "null"              ),
    make_aw_clk_inf(AW_MOD_CLK_NAND0,       CLK_MOD_NAND0       ),
    make_aw_clk_inf(AW_MOD_CLK_NAND1,       CLK_MOD_NAND1       ),
    make_aw_clk_inf(AW_MOD_CLK_SDC0,        CLK_MOD_SDC0        ),
    make_aw_clk_inf(AW_MOD_CLK_SDC1,        CLK_MOD_SDC1        ),
    make_aw_clk_inf(AW_MOD_CLK_SDC2,        CLK_MOD_SDC2        ),
    make_aw_clk_inf(AW_MOD_CLK_SDC3,        CLK_MOD_SDC3        ),
    make_aw_clk_inf(AW_MOD_CLK_TS,          CLK_MOD_TS          ),
    make_aw_clk_inf(AW_MOD_CLK_SS,          CLK_MOD_SS          ),
    make_aw_clk_inf(AW_MOD_CLK_SPI0,        CLK_MOD_SPI0        ),
    make_aw_clk_inf(AW_MOD_CLK_SPI1,        CLK_MOD_SPI1        ),
    make_aw_clk_inf(AW_MOD_CLK_SPI2,        CLK_MOD_SPI2        ),
    make_aw_clk_inf(AW_MOD_CLK_SPI3,        CLK_MOD_SPI3        ),
    make_aw_clk_inf(AW_MOD_CLK_I2S0,        CLK_MOD_I2S0        ),
    make_aw_clk_inf(AW_MOD_CLK_I2S1,        CLK_MOD_I2S1        ),
    make_aw_clk_inf(AW_MOD_CLK_SPDIF,       CLK_MOD_SPDIF       ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY0,     CLK_MOD_USBPHY0     ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY1,     CLK_MOD_USBPHY1     ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY2,     CLK_MOD_USBPHY2     ),
    make_aw_clk_inf(AW_MOD_CLK_USBEHCI0,    CLK_MOD_USBEHCI0    ),
    make_aw_clk_inf(AW_MOD_CLK_USBEHCI1,    CLK_MOD_USBEHCI1    ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI0,    CLK_MOD_USBOHCI0    ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI1,    CLK_MOD_USBOHCI1    ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI2,    CLK_MOD_USBOHCI2    ),
    make_aw_clk_inf(AW_MOD_CLK_USBOTG,      CLK_MOD_USBOTG      ),
    make_aw_clk_inf(AW_MOD_CLK_MDFS,        CLK_MOD_MDFS        ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE0,       CLK_MOD_DEBE0       ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE1,       CLK_MOD_DEBE1       ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE0,       CLK_MOD_DEFE0       ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE1,       CLK_MOD_DEFE1       ),
    make_aw_clk_inf(AW_MOD_CLK_DEMIX,       CLK_MOD_DEMP        ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH0,     CLK_MOD_LCD0CH0     ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH1,     CLK_MOD_LCD0CH1     ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH0,     CLK_MOD_LCD1CH0     ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH1,     CLK_MOD_LCD1CH1     ),
    make_aw_clk_inf(AW_MOD_CLK_CSI0S,       CLK_MOD_CSI0S       ),
    make_aw_clk_inf(AW_MOD_CLK_CSI0M,       CLK_MOD_CSI0M       ),
    make_aw_clk_inf(AW_MOD_CLK_CSI1S,       CLK_MOD_CSI1S       ),
    make_aw_clk_inf(AW_MOD_CLK_CSI1M,       CLK_MOD_CSI1M       ),
    make_aw_clk_inf(AW_MOD_CLK_VE,          CLK_MOD_VE          ),
    make_aw_clk_inf(AW_MOD_CLK_ADDA,        CLK_MOD_ADDA        ),
    make_aw_clk_inf(AW_MOD_CLK_AVS,         CLK_MOD_AVS         ),
    make_aw_clk_inf(AW_MOD_CLK_DMIC,        CLK_MOD_DMIC        ),
    make_aw_clk_inf(AW_MOD_CLK_HDMI,        CLK_MOD_HDMI        ),
    make_aw_clk_inf(AW_MOD_CLK_HDMI_DDC,    CLK_MOD_HDMI_DDC    ),
    make_aw_clk_inf(AW_MOD_CLK_PS,          CLK_MOD_PS          ),
    make_aw_clk_inf(AW_MOD_CLK_MTCACC,      CLK_MOD_MTCACC      ),
    make_aw_clk_inf(AW_MOD_CLK_MBUS0,       CLK_MOD_MBUS0       ),
    make_aw_clk_inf(AW_MOD_CLK_MBUS1,       CLK_MOD_MBUS1       ),
    make_aw_clk_inf(AW_MOD_CLK_DRAM,        CLK_MOD_DRAM        ),
    make_aw_clk_inf(AW_MOD_CLK_MIPIDSIS,    CLK_MOD_MIPIDSIS    ),
    make_aw_clk_inf(AW_MOD_CLK_MIPIDSIP,    CLK_MOD_MIPIDSIP    ),
    make_aw_clk_inf(AW_MOD_CLK_MIPICSIS,    CLK_MOD_MIPICSIS    ),
    make_aw_clk_inf(AW_MOD_CLK_MIPICSIP,    CLK_MOD_MIPICSIP    ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDRC0,     CLK_MOD_IEPDRC0     ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDRC1,     CLK_MOD_IEPDRC1     ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDEU0,     CLK_MOD_IEPDEU0     ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDEU1,     CLK_MOD_IEPDEU1     ),
    make_aw_clk_inf(AW_MOD_CLK_GPUCORE,     CLK_MOD_GPUCORE     ),
    make_aw_clk_inf(AW_MOD_CLK_GPUMEM,      CLK_MOD_GPUMEM      ),
    make_aw_clk_inf(AW_MOD_CLK_GPUHYD,      CLK_MOD_GPUHYD      ),
    make_aw_clk_inf(AW_MOD_CLK_TWI0,        CLK_MOD_TWI0        ),
    make_aw_clk_inf(AW_MOD_CLK_TWI1,        CLK_MOD_TWI1        ),
    make_aw_clk_inf(AW_MOD_CLK_TWI2,        CLK_MOD_TWI2        ),
    make_aw_clk_inf(AW_MOD_CLK_TWI3,        CLK_MOD_TWI3        ),
    make_aw_clk_inf(AW_MOD_CLK_UART0,       CLK_MOD_UART0       ),
    make_aw_clk_inf(AW_MOD_CLK_UART1,       CLK_MOD_UART1       ),
    make_aw_clk_inf(AW_MOD_CLK_UART2,       CLK_MOD_UART2       ),
    make_aw_clk_inf(AW_MOD_CLK_UART3,       CLK_MOD_UART3       ),
    make_aw_clk_inf(AW_MOD_CLK_UART4,       CLK_MOD_UART4       ),
    make_aw_clk_inf(AW_MOD_CLK_UART5,       CLK_MOD_UART5       ),
    make_aw_clk_inf(AW_MOD_CLK_GMAC,        CLK_MOD_GMAC        ),
    make_aw_clk_inf(AW_MOD_CLK_DMA,         CLK_MOD_DMA         ),
    make_aw_clk_inf(AW_MOD_CLK_HSTMR,       CLK_MOD_HSTMR       ),
    make_aw_clk_inf(AW_MOD_CLK_MSGBOX,      CLK_MOD_MSGBOX      ),
    make_aw_clk_inf(AW_MOD_CLK_SPINLOCK,    CLK_MOD_SPINLOCK    ),
    make_aw_clk_inf(AW_MOD_CLK_LVDS,        CLK_MOD_LVDS        ),
    make_aw_clk_inf(AW_MOD_CLK_SMPTWD,      CLK_SMP_TWD         ),
    make_aw_clk_inf(AW_MOD_CLK_R_TWI,       CLK_MOD_R_TWI       ),
    make_aw_clk_inf(AW_MOD_CLK_R_1WIRE,     CLK_MOD_R_1WIRE     ),
    make_aw_clk_inf(AW_MOD_CLK_R_UART,      CLK_MOD_R_UART      ),
    make_aw_clk_inf(AW_MOD_CLK_R_P2WI,      CLK_MOD_R_P2WI      ),
    make_aw_clk_inf(AW_MOD_CLK_R_TMR,       CLK_MOD_R_TMR       ),
    make_aw_clk_inf(AW_MOD_CLK_R_CIR,       CLK_MOD_R_CIR       ),
    make_aw_clk_inf(AW_MOD_CLK_R_PIO,       CLK_MOD_R_PIO       ),
    make_aw_clk_inf(AW_AXI_CLK_DRAM,        "axi_dram"          ),
    make_aw_clk_inf(AW_AHB_CLK_MIPICSI,     CLK_AHB_MIPICSI     ),
    make_aw_clk_inf(AW_AHB_CLK_MIPIDSI,     CLK_AHB_MIPIDSI     ),
    make_aw_clk_inf(AW_AHB_CLK_SS,          CLK_AHB_SS          ),
    make_aw_clk_inf(AW_AHB_CLK_DMA,         CLK_AHB_DMA         ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC0,      CLK_AHB_SDMMC0      ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC1,      CLK_AHB_SDMMC1      ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC2,      CLK_AHB_SDMMC2      ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC3,      CLK_AHB_SDMMC3      ),
    make_aw_clk_inf(AW_AHB_CLK_NAND1,       CLK_AHB_NAND1       ),
    make_aw_clk_inf(AW_AHB_CLK_NAND0,       CLK_AHB_NAND0       ),
    make_aw_clk_inf(AW_AHB_CLK_SDRAM,       CLK_AHB_SDRAM       ),
    make_aw_clk_inf(AW_AHB_CLK_GMAC,        CLK_AHB_GMAC        ),
    make_aw_clk_inf(AW_AHB_CLK_TS,          CLK_AHB_TS          ),
    make_aw_clk_inf(AW_AHB_CLK_HSTMR,       CLK_AHB_HSTMR       ),
    make_aw_clk_inf(AW_AHB_CLK_SPI0,        CLK_AHB_SPI0        ),
    make_aw_clk_inf(AW_AHB_CLK_SPI1,        CLK_AHB_SPI1        ),
    make_aw_clk_inf(AW_AHB_CLK_SPI2,        CLK_AHB_SPI2        ),
    make_aw_clk_inf(AW_AHB_CLK_SPI3,        CLK_AHB_SPI3        ),
    make_aw_clk_inf(AW_AHB_CLK_OTG,         CLK_AHB_OTG         ),
    make_aw_clk_inf(AW_AHB_CLK_EHCI0,       CLK_AHB_EHCI0       ),
    make_aw_clk_inf(AW_AHB_CLK_EHCI1,       CLK_AHB_EHCI1       ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI0,       CLK_AHB_OHCI0       ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI1,       CLK_AHB_OHCI1       ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI2,       CLK_AHB_OHCI2       ),
    make_aw_clk_inf(AW_AHB_CLK_VE,          CLK_AHB_VE          ),
    make_aw_clk_inf(AW_AHB_CLK_LCD0,        CLK_AHB_LCD0        ),
    make_aw_clk_inf(AW_AHB_CLK_LCD1,        CLK_AHB_LCD1        ),
    make_aw_clk_inf(AW_AHB_CLK_CSI0,        CLK_AHB_CSI0        ),
    make_aw_clk_inf(AW_AHB_CLK_CSI1,        CLK_AHB_CSI1        ),
    make_aw_clk_inf(AW_AHB_CLK_HDMI ,       CLK_AHB_HDMI        ),
    make_aw_clk_inf(AW_AHB_CLK_DEBE0,       CLK_AHB_DEBE0       ),
    make_aw_clk_inf(AW_AHB_CLK_DEBE1,       CLK_AHB_DEBE1       ),
    make_aw_clk_inf(AW_AHB_CLK_DEFE0,       CLK_AHB_DEFE0       ),
    make_aw_clk_inf(AW_AHB_CLK_DEFE1,       CLK_AHB_DEFE1       ),
    make_aw_clk_inf(AW_AHB_CLK_MP,          CLK_AHB_MP          ),
    make_aw_clk_inf(AW_AHB_CLK_GPU,         CLK_AHB_GPU         ),
    make_aw_clk_inf(AW_AHB_CLK_MSGBOX,      CLK_AHB_MSGBOX      ),
    make_aw_clk_inf(AW_AHB_CLK_SPINLOCK,    CLK_AHB_SPINLOCK    ),
    make_aw_clk_inf(AW_AHB_CLK_DEU0,        CLK_AHB_DEU0        ),
    make_aw_clk_inf(AW_AHB_CLK_DEU1,        CLK_AHB_DEU1        ),
    make_aw_clk_inf(AW_AHB_CLK_DRC0,        CLK_AHB_DRC0        ),
    make_aw_clk_inf(AW_AHB_CLK_DRC1,        CLK_AHB_DRC1        ),
    make_aw_clk_inf(AW_AHB_CLK_MTCACC,      CLK_AHB_MTCACC      ),
    make_aw_clk_inf(AW_APB_CLK_ADDA,        CLK_APB_ADDA        ),
    make_aw_clk_inf(AW_APB_CLK_SPDIF,       CLK_APB_SPDIF       ),
    make_aw_clk_inf(AW_APB_CLK_PIO,         CLK_APB_PIO         ),
    make_aw_clk_inf(AW_APB_CLK_I2S0,        CLK_APB_I2S0        ),
    make_aw_clk_inf(AW_APB_CLK_I2S1,        CLK_APB_I2S1        ),
    make_aw_clk_inf(AW_APB_CLK_TWI0,        CLK_APB_TWI0        ),
    make_aw_clk_inf(AW_APB_CLK_TWI1,        CLK_APB_TWI1        ),
    make_aw_clk_inf(AW_APB_CLK_TWI2,        CLK_APB_TWI2        ),
    make_aw_clk_inf(AW_APB_CLK_TWI3,        CLK_APB_TWI3        ),
    make_aw_clk_inf(AW_APB_CLK_UART0,       CLK_APB_UART0       ),
    make_aw_clk_inf(AW_APB_CLK_UART1,       CLK_APB_UART1       ),
    make_aw_clk_inf(AW_APB_CLK_UART2,       CLK_APB_UART2       ),
    make_aw_clk_inf(AW_APB_CLK_UART3,       CLK_APB_UART3       ),
    make_aw_clk_inf(AW_APB_CLK_UART4,       CLK_APB_UART4       ),
    make_aw_clk_inf(AW_APB_CLK_UART5,       CLK_APB_UART5       ),
    make_aw_clk_inf(AW_DRAM_CLK_VE,         CLK_DRAM_VE         ),
    make_aw_clk_inf(AW_DRAM_CLK_CSI_ISP,    CLK_DRAM_CSI_ISP    ),
    make_aw_clk_inf(AW_DRAM_CLK_TS,         CLK_DRAM_TS         ),
    make_aw_clk_inf(AW_DRAM_CLK_DRC0,       CLK_DRAM_DRC0       ),
    make_aw_clk_inf(AW_DRAM_CLK_DRC1,       CLK_DRAM_DRC1       ),
    make_aw_clk_inf(AW_DRAM_CLK_DEU0,       CLK_DRAM_DEU0       ),
    make_aw_clk_inf(AW_DRAM_CLK_DEU1,       CLK_DRAM_DEU1       ),
    make_aw_clk_inf(AW_DRAM_CLK_DEFE0,      CLK_DRAM_DEFE0      ),
    make_aw_clk_inf(AW_DRAM_CLK_DEFE1,      CLK_DRAM_DEFE1      ),
    make_aw_clk_inf(AW_DRAM_CLK_DEBE0,      CLK_DRAM_DEBE0      ),
    make_aw_clk_inf(AW_DRAM_CLK_DEBE1,      CLK_DRAM_DEBE1      ),
    make_aw_clk_inf(AW_DRAM_CLK_MP,         CLK_DRAM_MP         ),
    make_aw_clk_inf(AW_CCU_CLK_CNT,         "count"             ),
};


/*
*********************************************************************************************************
*                           aw_ccu_init
*
*Description: initialise clock mangement unit;
*
*Arguments  : none
*
*Return     : result,
*               AW_CCMU_OK,     initialise ccu successed;
*               AW_CCMU_FAIL,   initialise ccu failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_init(void)
{
    CCU_DBG("%s\n", __func__);

    /* initialise the CCU io base */
    aw_ccu_reg = (__ccmu_reg_list_t *)IO_ADDRESS(AW_CCM_BASE);
    aw_cpus_reg = (__ccmu_reg_cpu0_list_t *)IO_ADDRESS(AW_R_PRCM_BASE);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_exit
*
*Description: exit clock managment unit;
*
*Arguments  : none
*
*Return     : result,
*               AW_CCMU_OK,     exit ccu successed;
*               AW_CCMU_FAIL,   exit ccu failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_exit(void)
{
    CCU_DBG("%s\n", __func__);
    aw_ccu_reg = NULL;
    aw_cpus_reg = NULL;

    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_get__clk
*
*Description: get clock information by clock id.
*
*Arguments  : id    clock id;
*
*Return     : clock handle, return NULL if get clock information failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_get_clk(__aw_ccu_clk_id_e id, __ccu_clk_t *clk)
{
    __aw_ccu_clk_t  *tmp_clk;

    if(clk && (id < AW_CCU_CLK_NULL)) {
        tmp_clk = &aw_ccu_clk_tbl[id];

        /* set clock operation handle */
        clk->ops = &sys_clk_ops;
        clk->aw_clk = tmp_clk;

        /* query system clock information from hardware */
        tmp_clk->parent = sys_clk_ops.get_parent(id);
        tmp_clk->onoff  = sys_clk_ops.get_status(id);
        tmp_clk->rate   = sys_clk_ops.get_rate(id);
        tmp_clk->hash   = ccu_clk_calc_hash(tmp_clk->name);
    }
    else if(clk && (id < AW_CCU_CLK_CNT)) {
        tmp_clk = &aw_ccu_clk_tbl[id];

        /* set clock operation handle */
        clk->ops = &mod_clk_ops;
        clk->aw_clk = tmp_clk;

        /* query system clock information from hardware */
        tmp_clk->parent = mod_clk_ops.get_parent(id);
        tmp_clk->onoff  = mod_clk_ops.get_status(id);
        tmp_clk->reset  = mod_clk_ops.get_reset(id);
        tmp_clk->rate   = mod_clk_ops.get_rate(id);
        tmp_clk->hash   = ccu_clk_calc_hash(tmp_clk->name);
    }
    else {
        CCU_ERR("clock id is invalid when get clk info!\n");
        return -1;
    }

    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_switch_ahb_2_pll6
*
*Description: switch ahb to pll6
*
*Arguments  : void
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_switch_ahb_2_pll6(void)
{
    aw_ccu_reg->Ahb1Div.Ahb1PreDiv = 3;
    aw_ccu_reg->Ahb1Div.Ahb1Div = 3;
    __delay(2000);
    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = 3;
    __delay(5000);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_switch_apb_2_pll6
*
*Description: switch ahb to pll6
*
*Arguments  : void
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_switch_apb_2_pll6(void)
{
    aw_ccu_reg->Apb2Div.DivN = 3;
    aw_ccu_reg->Apb2Div.DivM = 4;
    __delay(2000);
    aw_ccu_reg->Apb2Div.ClkSrc = 2;
    __delay(5000);

    return 0;
}

