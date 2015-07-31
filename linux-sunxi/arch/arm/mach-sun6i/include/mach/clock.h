/*
 * arch/arm/mach-sun6i/include/mach/clock.h
 *
 * Copyright 2012 (c) Allwinner.
 * kevin.z.m (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __AW_CLOCK_H__
#define __AW_CLOCK_H__

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#define CCU_LOCK_LIUGANG_20120930 	/* add ccu lock, liugang, 2012-9-30 */

#ifdef CCU_LOCK_LIUGANG_20120930
#define CCU_LOCK_INIT(lock)	spin_lock_init(lock)
#define CCU_LOCK_DEINIT(lock)	do{} while(0)
#define CCU_LOCK(lock, flags)	spin_lock_irqsave((lock), (flags))
#define CCU_UNLOCK(lock, flags)	spin_unlock_irqrestore((lock), (flags))
#define DEFINE_FLAGS(x)		unsigned long x
#else
#define CCU_LOCK_INIT(lock)	do{} while(0)
#define CCU_LOCK_DEINIT(lock)	do{} while(0)
#define CCU_LOCK(lock, flags)	do{} while(0)
#define CCU_UNLOCK(lock, flags)	do{} while(0)
#define DEFINE_FLAGS(flags)	do{} while(0)
#endif /* CCU_LOCK_LIUGANG_20120930 */

/* define clock error type      */
typedef enum __AW_CCU_ERR
{
    AW_CCU_ERR_NONE     =  0,
    AW_CCU_ERR_PARA_NUL = -1,
    AW_CCU_ERR_PARA_INV = -2,
} __aw_ccu_err_e;


typedef enum __AW_CCU_CLK_ONOFF
{
    AW_CCU_CLK_OFF          = 0,
    AW_CCU_CLK_ON           = 1,
} __aw_ccu_clk_onff_e;


typedef enum __AW_CCU_CLK_RESET
{
    AW_CCU_CLK_RESET        = 0,
    AW_CCU_CLK_NRESET       = 1,
} __aw_ccu_clk_reset_e;


/* define system clock name */
#define CLK_SYS_LOSC        "sys_losc"
#define CLK_SYS_HOSC        "sys_hosc"
#define CLK_SYS_PLL1        "sys_pll1"
#define CLK_SYS_PLL2        "sys_pll2"
#define CLK_SYS_PLL3        "sys_pll3"
#define CLK_SYS_PLL4        "sys_pll4"
#define CLK_SYS_PLL5        "sys_pll5"
#define CLK_SYS_PLL6        "sys_pll6"
#define CLK_SYS_PLL7        "sys_pll7"
#define CLK_SYS_PLL8        "sys_pll8"
#define CLK_SYS_PLL9        "sys_pll9"
#define CLK_SYS_PLL10       "sys_pll10"
#define CLK_SYS_PLL2X8      "sys_pll2X8"
#define CLK_SYS_PLL3X2      "sys_pll3X2"
#define CLK_SYS_PLL6X2      "sys_pll6X2"
#define CLK_SYS_PLL7X2      "sys_pll7X2"
#define CLK_SYS_MIPI_PLL    "sys_mipi_pll"
#define CLK_SYS_AC327       "sys_ac327"
#define CLK_SYS_AR100       "sys_ar100"
#define CLK_SYS_AXI         "sys_axi"
#define CLK_SYS_AHB0        "sys_ahb0"
#define CLK_SYS_AHB1        "sys_ahb1"
#define CLK_SYS_APB0        "sys_apb0"
#define CLK_SYS_APB1        "sys_apb1"
#define CLK_SYS_APB2        "sys_apb2"

/* define module clock name */
#define CLK_MOD_NAND0       "mod_nand0"
#define CLK_MOD_NAND1       "mod_nand1"
#define CLK_MOD_SDC0        "mod_sdc0"
#define CLK_MOD_SDC1        "mod_sdc1"
#define CLK_MOD_SDC2        "mod_sdc2"
#define CLK_MOD_SDC3        "mod_sdc3"
#define CLK_MOD_TS          "mod_ts"
#define CLK_MOD_SS          "mod_ss"
#define CLK_MOD_SPI0        "mod_spi0"
#define CLK_MOD_SPI1        "mod_spi1"
#define CLK_MOD_SPI2        "mod_spi2"
#define CLK_MOD_SPI3        "mod_spi3"
#define CLK_MOD_I2S0        "mod_i2s0"
#define CLK_MOD_I2S1        "mod_i2s1"
#define CLK_MOD_SPDIF       "mod_spdif"
#define CLK_MOD_USBPHY0     "mod_usbphy0"
#define CLK_MOD_USBPHY1     "mod_usbphy1"
#define CLK_MOD_USBPHY2     "mod_usbphy2"
#define CLK_MOD_USBEHCI0    "mod_usbehci0"
#define CLK_MOD_USBEHCI1    "mod_usbehci1"
#define CLK_MOD_USBOHCI0    "mod_usbohci0"
#define CLK_MOD_USBOHCI1    "mod_usbohci1"
#define CLK_MOD_USBOHCI2    "mod_usbohci2"
#define CLK_MOD_USBOTG      "mod_usbotg"
#define CLK_MOD_MDFS        "mod_mdfs"
#define CLK_MOD_DEBE0       "mod_debe0"
#define CLK_MOD_DEBE1       "mod_debe1"
#define CLK_MOD_DEFE0       "mod_defe0"
#define CLK_MOD_DEFE1       "mod_defe1"
#define CLK_MOD_DEMP        "mod_demp"
#define CLK_MOD_LCD0CH0     "mod_lcd0ch0"
#define CLK_MOD_LCD0CH1     "mod_lcd0ch1"
#define CLK_MOD_LCD1CH0     "mod_lcd1ch0"
#define CLK_MOD_LCD1CH1     "mod_lcd1ch1"
#define CLK_MOD_CSI0S       "mod_csi0s"
#define CLK_MOD_CSI0M       "mod_csi0m"
#define CLK_MOD_CSI1S       "mod_csi1s"
#define CLK_MOD_CSI1M       "mod_csi1m"
#define CLK_MOD_VE          "mod_ve"
#define CLK_MOD_ADDA        "mod_adda"
#define CLK_MOD_AVS         "mod_avs"
#define CLK_MOD_DMIC        "mod_dmic"
#define CLK_MOD_HDMI        "mod_hdmi"
#define CLK_MOD_HDMI_DDC    "mod_hdmi_ddc"
#define CLK_MOD_PS          "mod_ps"
#define CLK_MOD_MTCACC      "mod_mtcacc"
#define CLK_MOD_MBUS0       "mod_mbus0"
#define CLK_MOD_MBUS1       "mod_mbus1"
#define CLK_MOD_DRAM        "mod_dram"
#define CLK_MOD_MIPIDSIS    "mod_mipidsis"
#define CLK_MOD_MIPIDSIP    "mod_mipidsip"
#define CLK_MOD_MIPICSIS    "mod_mipicsis"
#define CLK_MOD_MIPICSIP    "mod_mipicsip"
#define CLK_MOD_IEPDRC0     "mod_iepdrc0"
#define CLK_MOD_IEPDRC1     "mod_iepdrc1"
#define CLK_MOD_IEPDEU0     "mod_iepdeu0"
#define CLK_MOD_IEPDEU1     "mod_iepdeu1"
#define CLK_MOD_GPUCORE     "mod_gpucore"
#define CLK_MOD_GPUMEM      "mod_gpumem"
#define CLK_MOD_GPUHYD      "mod_gpuhyd"
#define CLK_MOD_TWI0        "mod_twi0"
#define CLK_MOD_TWI1        "mod_twi1"
#define CLK_MOD_TWI2        "mod_twi2"
#define CLK_MOD_TWI3        "mod_twi3"
#define CLK_MOD_UART0       "mod_uart0"
#define CLK_MOD_UART1       "mod_uart1"
#define CLK_MOD_UART2       "mod_uart2"
#define CLK_MOD_UART3       "mod_uart3"
#define CLK_MOD_UART4       "mod_uart4"
#define CLK_MOD_UART5       "mod_uart5"
#define CLK_MOD_GMAC        "mod_gmac"
#define CLK_MOD_DMA         "mod_dma"
#define CLK_MOD_HSTMR       "mod_hstmr"
#define CLK_MOD_MSGBOX      "mod_msgbox"
#define CLK_MOD_SPINLOCK    "mod_spinlock"
#define CLK_MOD_LVDS        "mod_lvds"
#define CLK_SMP_TWD         "smp_twd"
#define CLK_MOD_R_TWI       "mod_r_twi"
#define CLK_MOD_R_1WIRE     "mod_r_1wire"
#define CLK_MOD_R_UART      "mod_r_uart"
#define CLK_MOD_R_P2WI      "mod_r_p2wi"
#define CLK_MOD_R_TMR       "mod_r_tmr"
#define CLK_MOD_R_CIR       "mod_r_cir"
#define CLK_MOD_R_PIO       "mod_r_pio"

/* define ahb module gatine clock */
#define CLK_AHB_MIPICSI     "ahb_mipicsi"
#define CLK_AHB_MIPIDSI     "ahb_mipidsi"
#define CLK_AHB_SS          "ahb_ss"
#define CLK_AHB_DMA         "ahb_dma"
#define CLK_AHB_SDMMC0      "ahb_sdmmc0"
#define CLK_AHB_SDMMC1      "ahb_sdmmc1"
#define CLK_AHB_SDMMC2      "ahb_sdmmc2"
#define CLK_AHB_SDMMC3      "ahb_sdmmc3"
#define CLK_AHB_NAND1       "ahb_nand1"
#define CLK_AHB_NAND0       "ahb_nand0"
#define CLK_AHB_SDRAM       "ahb_sdram"
#define CLK_AHB_GMAC        "ahb_gmac"
#define CLK_AHB_TS          "ahb_ts"
#define CLK_AHB_HSTMR       "ahb_hstmr"
#define CLK_AHB_SPI0        "ahb_spi0"
#define CLK_AHB_SPI1        "ahb_spi1"
#define CLK_AHB_SPI2        "ahb_spi2"
#define CLK_AHB_SPI3        "ahb_spi3"
#define CLK_AHB_OTG         "ahb_otg"
#define CLK_AHB_EHCI0       "ahb_ehci0"
#define CLK_AHB_EHCI1       "ahb_ehci1"
#define CLK_AHB_OHCI0       "ahb_ohci0"
#define CLK_AHB_OHCI1       "ahb_ohci1"
#define CLK_AHB_OHCI2       "ahb_ohci2"
#define CLK_AHB_VE          "ahb_ve"
#define CLK_AHB_LCD0        "ahb_lcd0"
#define CLK_AHB_LCD1        "ahb_lcd1"
#define CLK_AHB_CSI0        "ahb_csi0"
#define CLK_AHB_CSI1        "ahb_csi1"
#define CLK_AHB_HDMI        "ahb_hdmid"
#define CLK_AHB_DEBE0       "ahb_debe0"
#define CLK_AHB_DEBE1       "ahb_debe1"
#define CLK_AHB_DEFE0       "ahb_defe0"
#define CLK_AHB_DEFE1       "ahb_defe1"
#define CLK_AHB_MP          "ahb_mp"
#define CLK_AHB_GPU         "ahb_gpu"
#define CLK_AHB_MSGBOX      "ahb_msgbox"
#define CLK_AHB_SPINLOCK    "ahb_spinlock"
#define CLK_AHB_DEU0        "ahb_deu0"
#define CLK_AHB_DEU1        "ahb_deu1"
#define CLK_AHB_DRC0        "ahb_drc0"
#define CLK_AHB_DRC1        "ahb_drc1"
#define CLK_AHB_MTCACC      "ahb_mtcacc"

/* define apb module gatine clock */
#define CLK_APB_ADDA        "apb_adda"
#define CLK_APB_SPDIF       "apb_spdif"
#define CLK_APB_PIO         "apb_pio"
#define CLK_APB_I2S0        "apb_i2s0"
#define CLK_APB_I2S1        "apb_i2s1"
#define CLK_APB_TWI0        "apb_twi0"
#define CLK_APB_TWI1        "apb_twi1"
#define CLK_APB_TWI2        "apb_twi2"
#define CLK_APB_TWI3        "apb_twi3"
#define CLK_APB_UART0       "apb_uart0"
#define CLK_APB_UART1       "apb_uart1"
#define CLK_APB_UART2       "apb_uart2"
#define CLK_APB_UART3       "apb_uart3"
#define CLK_APB_UART4       "apb_uart4"
#define CLK_APB_UART5       "apb_uart5"

/* define dram module gating clock */
#define CLK_DRAM_VE         "dram_ve"
#define CLK_DRAM_CSI_ISP    "dram_csi_isp"
#define CLK_DRAM_TS         "dram_ts"
#define CLK_DRAM_DRC0       "dram_drc0"
#define CLK_DRAM_DRC1       "dram_drc1"
#define CLK_DRAM_DEU0       "dram_deu0"
#define CLK_DRAM_DEU1       "dram_deu1"
#define CLK_DRAM_DEFE0      "dram_defe0"
#define CLK_DRAM_DEFE1      "dram_defe1"
#define CLK_DRAM_DEBE0      "dram_debe0"
#define CLK_DRAM_DEBE1      "dram_debe1"
#define CLK_DRAM_MP         "dram_mp"




struct __AW_CCU_CLK;
struct clk_ops;

typedef struct clk
{
    struct __AW_CCU_CLK *aw_clk;    /* clock handle from ccu csp                            */
    struct clk_ops      *ops;       /* clock operation handle                               */
    int             enable;     /* enable count, when it down to 0, it will be disalbe  */
#ifdef CCU_LOCK_LIUGANG_20120930
    spinlock_t      lock;	/* to synchronize the clock setting */
#endif /* CCU_LOCK_LIUGANG_20120930 */

} __ccu_clk_t;


extern int clk_reset(struct clk *clk, __aw_ccu_clk_reset_e reset);

#endif  /* #ifndef __AW_CLOCK_H__ */

