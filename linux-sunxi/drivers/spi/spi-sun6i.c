/*
 * drivers/spi/spi-sun6i.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I SPI Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <mach/hardware.h>
#include <mach/irqs-sun6i.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <mach/spi.h>
#include <mach/clock.h>
#include <mach/system.h>

#define SPI_INF(...)    printk(__VA_ARGS__)
#define SPI_ERR(...)    do {printk(KERN_ERR "%s(Line%d) ", __FILE__, __LINE__); printk(__VA_ARGS__);} while(0)

#if 0
    #define SPI_DBG(...)    printk(__VA_ARGS__)
#else
    #define SPI_DBG(...)
#endif

#define SUN6I_SPI_OK   0
#define SUN6I_SPI_FAIL -1

enum spi_mode_type {
    SINGLE_HALF_DUPLEX_RX,		//single mode, half duplex read
    SINGLE_HALF_DUPLEX_TX,		//single mode, half duplex write
    SINGLE_FULL_DUPLEX_RX_TX,	//single mode, full duplex read and write
	DUAL_HALF_DUPLEX_RX,		//dual mode, half duplex read
	DUAL_HALF_DUPLEX_TX,		//dual mode, half duplex write
    MODE_TYPE_NULL,
};

enum spi_dma_dir {
	SPI_DMA_RDEV,
	SPI_DMA_WDEV,
	SPI_DMA_RWNULL,
};

struct sun6i_spi {
    struct platform_device *pdev;
	struct spi_master *master;/* kzalloc */
	void __iomem *base_addr; /* register */
    enum sw_ic_ver version;

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	struct clk *hclk;  /* ahb spi gating bit */
	struct clk *mclk;  /* ahb spi gating bit */
	unsigned long gpio_hdle;
#endif

	enum spi_dma_dir dma_dir_wdev;
	enum spi_dma_dir dma_dir_rdev;
	dm_hdl_t dma_hdle_tx;
	dm_hdl_t dma_hdle_rx;
	enum spi_mode_type mode_type;

	unsigned int irq; /* irq NO. */
	char irq_name[48];

	int busy;
#define SPI_FREE   (1<<0)
#define SPI_SUSPND (1<<1)
#define SPI_BUSY   (1<<2)

	int result; /* 0: succeed -1:fail */

	struct workqueue_struct *workqueue;
	struct work_struct work;

	struct list_head queue; /* spi messages */
	spinlock_t lock;

	struct completion done;  /* wakup another spi transfer */

	/* keep select during one message */
	void (*cs_control)(struct spi_device *spi, bool on);

/*
 * (1) enable cs1,    cs_bitmap = SPI_CHIP_SELECT_CS1;
 * (2) enable cs0&cs1,cs_bitmap = SPI_CHIP_SELECT_CS0|SPI_CHIP_SELECT_CS1;
 *
 */
#define SPI_CHIP_SELECT_CS0 (0x01)
#define SPI_CHIP_SELECT_CS1 (0x02)

	int cs_bitmap;/* cs0- 0x1; cs1-0x2, cs0&cs1-0x3. */
};

/* config chip select */
static s32 spi_set_cs(u32 chipselect, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);

    if (chipselect < 4) {
        reg_val &= ~SPI_TC_SS_MASK;/* SS-chip select, clear two bits */
        reg_val |= chipselect << SPI_TC_SS_BIT_POS;/* set chip select */
        writel(reg_val, base_addr + SPI_TC_REG);
        return SUN6I_SPI_OK;
    }
    else {
        SPI_ERR("Chip Select set fail! cs = %d\n", chipselect);
        return SUN6I_SPI_FAIL;
    }
}

/* config spi */
static void spi_config_tc(u32 master, u32 config, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);

    /*1. POL */
    if (config & SPI_POL_ACTIVE_)
        reg_val |= SPI_TC_POL;/*default POL = 1 */
    else
        reg_val &= ~SPI_TC_POL;

    /*2. PHA */
    if (config & SPI_PHA_ACTIVE_)
        reg_val |= SPI_TC_PHA;/*default PHA = 1 */
    else
        reg_val &= ~SPI_TC_PHA;

    /*3. SSPOL,chip select signal polarity */
    if (config & SPI_CS_HIGH_ACTIVE_)
        reg_val &= ~SPI_TC_SPOL;
    else
        reg_val |= SPI_TC_SPOL;/*default SSPOL = 1,Low level effective */

    /*4. LMTF--LSB/MSB transfer first select */
    if (config & SPI_LSB_FIRST_ACTIVE_)
        reg_val |= SPI_TC_FBS;
    else
        reg_val &= ~SPI_TC_FBS;/*default LMTF =0, MSB first */

    /*master mode: set DDB,DHB,SMC,SSCTL*/
    if(master == 1) {
        /*5. dummy burst type */
        if (config & SPI_DUMMY_ONE_ACTIVE_)
            reg_val |= SPI_TC_DDB;
        else
            reg_val &= ~SPI_TC_DDB;/*default DDB =0, ZERO */

        /*6.discard hash burst-DHB */
        if (config & SPI_RECEIVE_ALL_ACTIVE_)
            reg_val &= ~SPI_TC_DHB;
        else
            reg_val |= SPI_TC_DHB;/*default DHB =1, discard unused burst */

        /*7. set SMC = 1 , SSCTL = 0 ,TPE = 1 */
        reg_val &= ~SPI_TC_SSCTL;
    }
    else {
        /* tips for slave mode config */
        SPI_INF("slave mode configurate control register.\n");
    }
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set spi clock */
static void spi_set_clk(u32 spi_clk, u32 ahb_clk, void *base_addr)
{
    u32 reg_val = 0;
    u32 div_clk = ahb_clk / (spi_clk << 1);

    SPI_DBG("set spi clock %d, mclk %d\n", spi_clk, ahb_clk);
    reg_val = readl(base_addr + SPI_CLK_CTL_REG);

    /* CDR2 */
    if(div_clk <= SPI_CLK_SCOPE) {
        if (div_clk != 0) {
            div_clk--;
        }
        reg_val &= ~SPI_CLK_CTL_CDR2;
        reg_val |= (div_clk | SPI_CLK_CTL_DRS);
        SPI_DBG("CDR2 - n = %d \n", div_clk);
    }/* CDR1 */
    else {
		div_clk = 0;
		while(ahb_clk > spi_clk){
			div_clk++;
			ahb_clk >>= 1;
		}
        reg_val &= ~(SPI_CLK_CTL_CDR1 | SPI_CLK_CTL_DRS);
        reg_val |= (div_clk << 8);
        SPI_DBG("CDR1 - n = %d \n", div_clk);
    }
    writel(reg_val, base_addr + SPI_CLK_CTL_REG);
}

static void spi_set_sample_delay(struct sun6i_spi *sspi)
{
    u32 reg_ccr = 0;
    u32 reg_tcr = 0;
    u32 cpha;
    u32 ccr1;

    if((sspi->version==MAGIC_VER_A)||
       (sspi->version==MAGIC_VER_B)){
        return;
    }

    reg_ccr = readl(sspi->base_addr + SPI_CLK_CTL_REG);
    if((reg_ccr & SPI_CLK_CTL_DRS)!=0){  // cdr2

        // clear the SPI_TC_SDC bit.
        reg_tcr = readl(sspi->base_addr + SPI_TC_REG);
        cpha = reg_tcr & SPI_TC_PHA;
        reg_tcr &= ~(SPI_TC_SDC);
        writel(reg_tcr, sspi->base_addr + SPI_TC_REG);

        ccr1 = cpha ? 0 : 1;
        reg_ccr &= ~(1<<8);
        reg_ccr |= (ccr1<<8);
        writel(reg_ccr, sspi->base_addr + SPI_CLK_CTL_REG);
    }
}

/* start spi transfer */
static void spi_start_xfer(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);
    reg_val |= SPI_TC_XCH;
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* enable spi bus */
static void spi_enable_bus(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_GC_REG);
    reg_val |= SPI_GC_EN;
    writel(reg_val, base_addr + SPI_GC_REG);
}

/* disbale spi bus */
static void spi_disable_bus(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_GC_REG);
    reg_val &= ~SPI_GC_EN;
    writel(reg_val, base_addr + SPI_GC_REG);
}

/* set master mode */
static void spi_set_master(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_GC_REG);
    reg_val |= SPI_GC_MODE;
    writel(reg_val, base_addr + SPI_GC_REG);
}

/* enable transmit pause */
static void spi_enable_tp(void *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);
    reg_val |= SPI_GC_TP_EN;
    writel(reg_val, base_addr + SPI_GC_REG);
}

/* soft reset spi controller */
static void spi_soft_reset(void *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);
	reg_val |= SPI_GC_SRST;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* enable irq type */
static void spi_enable_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);
    bitmap &= SPI_INTEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, base_addr + SPI_INT_CTL_REG);
}

/* disable irq type */
static void spi_disable_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);
    bitmap &= SPI_INTEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + SPI_INT_CTL_REG);
}

/* enable dma irq */
static void spi_enable_dma_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);
    bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

/* disable dma irq */
static void spi_disable_dma_irq(u32 bitmap, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);
    bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

/* query irq pending */
static u32 spi_qry_irq_pending(void *base_addr)
{
    return (SPI_INT_STA_MASK & readl(base_addr + SPI_INT_STA_REG));
}

/* clear irq pending */
static void spi_clr_irq_pending(u32 pending_bit, void *base_addr)
{
    pending_bit &= SPI_INT_STA_MASK;
    writel(pending_bit, base_addr + SPI_INT_STA_REG);
}

/* query txfifo bytes */
static u32 spi_query_txfifo(void *base_addr)
{
    u32 reg_val = (SPI_FIFO_STA_TX_CNT & readl(base_addr + SPI_FIFO_STA_REG));
    reg_val >>= SPI_TXCNT_BIT_POS;
    return reg_val;
}

/* query rxfifo bytes */
static u32 spi_query_rxfifo(void *base_addr)
{
    u32 reg_val = (SPI_FIFO_STA_RX_CNT & readl(base_addr + SPI_FIFO_STA_REG));
    reg_val >>= SPI_RXCNT_BIT_POS;
    return reg_val;
}

/* reset fifo */
static void spi_reset_fifo(void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);
    reg_val |= (SPI_FIFO_CTL_RX_RST|SPI_FIFO_CTL_TX_RST);
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

/* set transfer total length BC, transfer length TC and single transmit length STC */
static void spi_set_bc_tc_stc(u32 tx_len, u32 rx_len, u32 stc_len, u32 dummy_cnt, void *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_BURST_CNT_REG);
    reg_val &= ~SPI_BC_CNT_MASK;
    reg_val |= (SPI_BC_CNT_MASK & (tx_len + rx_len + dummy_cnt));
    writel(reg_val, base_addr + SPI_BURST_CNT_REG);
    //SPI_DBG("\n-- BC = %d --\n", readl(base_addr + SPI_BURST_CNT_REG));

    reg_val = readl(base_addr + SPI_TRANSMIT_CNT_REG);
    reg_val &= ~SPI_TC_CNT_MASK;
    reg_val |= (SPI_TC_CNT_MASK & tx_len);
    writel(reg_val, base_addr + SPI_TRANSMIT_CNT_REG);
    //SPI_DBG("\n-- TC = %d --\n", readl(base_addr + SPI_TRANSMIT_CNT_REG));

	reg_val = readl(base_addr + SPI_BCC_REG);
    reg_val &= ~SPI_BCC_STC_MASK;
    reg_val |= (SPI_BCC_STC_MASK & stc_len);
    writel(reg_val, base_addr + SPI_BCC_REG);
    //SPI_DBG("\n-- STC = %d --\n", readl(base_addr + SPI_BCC_REG));
}

/* set ss control */
static void spi_ss_ctrl(void *base_addr, u32 on_off)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);
    on_off &= 0x1;
    if(on_off)
        reg_val |= SPI_TC_SS_OWNER;
    else
        reg_val &= ~SPI_TC_SS_OWNER;
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set ss level */
static void spi_ss_level(void *base_addr, u32 hi_lo)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);
    hi_lo &= 0x1;
    if(hi_lo)
        reg_val |= SPI_TC_SS_LEVEL;
    else
        reg_val &= ~SPI_TC_SS_LEVEL;
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set dhb, 1: discard unused spi burst; 0: receiving all spi burst */
static void spi_set_all_burst_received(void *base_addr)
{
    u32 reg_val = readl(base_addr+SPI_TC_REG);
	reg_val &= ~SPI_TC_DHB;
    writel(reg_val, base_addr + SPI_TC_REG);
}

static void spi_set_dual_read(void *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);
	reg_val |= SPI_BCC_DUAL_MOD_RX_EN;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
static int sun6i_spi_get_cfg_csbitmap(int bus_num);
#endif

/* flush d-cache */
static void sun6i_spi_cleanflush_dcache_region(void *addr, __u32 len)
{
	__cpuc_flush_dcache_area(addr, len + (1 << 5) * 2 - 2);
}


/* ------------------------------- dma operation start----------------------------- */
static char *spi_dma_rx[] = {"spi0_rx", "spi1_rx", "spi2_rx", "spi3_rx"};
static char *spi_dma_tx[] = {"spi0_tx", "spi1_tx", "spi2_tx", "spi3_tx"};

/* dma full done callback for spi rx */
u32 sun6i_spi_dma_cb_rx(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	struct sun6i_spi *sspi = (struct sun6i_spi *)parg;

	if(DMA_CB_ABORT == cause)
		return 0;

	spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, sspi->base_addr);
	SPI_DBG("[spi-%d]: dma -read data end!\n", sspi->master->bus_num);

	return 0;
}

/* dma full done callback for spi tx */
u32 sun6i_spi_dma_cb_tx(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	struct sun6i_spi *sspi = (struct sun6i_spi *)parg;

	if(DMA_CB_ABORT == cause)
		return 0;

	spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, sspi->base_addr);
	SPI_DBG("[spi-%d]: dma -write data end!\n", sspi->master->bus_num);

	return 0;
}

/* request dma channel and set callback function */
static int sun6i_spi_prepare_dma(struct sun6i_spi *sspi, enum spi_dma_dir dma_dir)
{
    int ret = 0;
    int bus_num = sspi->master->bus_num;
	struct dma_cb_t done_cb;
    switch(dma_dir)
    {
        case SPI_DMA_RDEV:
            sspi->dma_hdle_rx = sw_dma_request(spi_dma_rx[bus_num], DMA_WORK_MODE_SINGLE);
            if(NULL == sspi->dma_hdle_rx) {
                SPI_ERR("[spi-%d]: request dma rx failed!\n", bus_num);
                return -EINVAL;
            }
			done_cb.func = sun6i_spi_dma_cb_rx;
			done_cb.parg = sspi;
            /* rx callback */
			ret = sw_dma_ctl(sspi->dma_hdle_rx, DMA_OP_SET_QD_CB, (void *)&done_cb);
            break;
        case SPI_DMA_WDEV:
            sspi->dma_hdle_tx = sw_dma_request(spi_dma_tx[bus_num], DMA_WORK_MODE_SINGLE);
            if(NULL == sspi->dma_hdle_tx) {
                SPI_ERR("[spi-%d]: request dma tx failed!\n", bus_num);
                return -EINVAL;
            }
			done_cb.func = sun6i_spi_dma_cb_tx;
			done_cb.parg = sspi;
            /* tx callback */
			ret = sw_dma_ctl(sspi->dma_hdle_tx, DMA_OP_SET_QD_CB, (void *)&done_cb);
            break;
        default:
            return -1;
    }

    return ret;
}

/*
 * config dma src and dst address,
 * io or linear address,
 * drq type,
 * then enqueue
 * but not trigger dma start
 */
static int sun6i_spi_config_dma(struct sun6i_spi *sspi, enum spi_dma_dir dma_dir, void *buf, unsigned int len)
{
    int ret = 0;
    int bus_num = sspi->master->bus_num;
    unsigned char spi_drq_dst[] = {DRQDST_SPI0TX, DRQDST_SPI1TX, DRQDST_SPI2TX, DRQDST_SPI3TX};
	unsigned char spi_drq_src[] = {DRQSRC_SPI0RX, DRQSRC_SPI1RX, DRQSRC_SPI2RX, DRQSRC_SPI3RX};
    unsigned long spi_phyaddr[] = {SPI0_BASE_ADDR_START, SPI1_BASE_ADDR_START, SPI2_BASE_ADDR_START, SPI3_BASE_ADDR_START};/* physical address */
    struct dma_config_t spi_hw_conf = {0};

	switch (dma_dir)
	{
		case SPI_DMA_RDEV:
			spi_hw_conf.src_drq_type = spi_drq_src[bus_num];
			spi_hw_conf.dst_drq_type = DRQDST_SDRAM;
			spi_hw_conf.address_type = DMAADDRT_D_LN_S_IO;
			spi_hw_conf.src_addr = spi_phyaddr[bus_num] + SPI_RXDATA_REG;
			spi_hw_conf.dst_addr = virt_to_phys(buf);
			spi_hw_conf.irq_spt = CHAN_IRQ_QD;

			spi_hw_conf.xfer_type = DMAXFER_D_SBYTE_S_SBYTE;
			spi_hw_conf.byte_cnt = len;
			spi_hw_conf.bconti_mode = false;
			spi_hw_conf.para = 3;

			/* flush d-cache */
			sun6i_spi_cleanflush_dcache_region((void *)buf, len);
			/* set src, dst, drq type, configuration */
			ret = sw_dma_config(sspi->dma_hdle_rx, &spi_hw_conf, ENQUE_PHASE_NORMAL);
			break;
		case SPI_DMA_WDEV:
			spi_hw_conf.src_drq_type = DRQSRC_SDRAM;
			spi_hw_conf.dst_drq_type = spi_drq_dst[bus_num];
			spi_hw_conf.address_type = DMAADDRT_D_IO_S_LN;
			spi_hw_conf.src_addr = virt_to_phys(buf);
			spi_hw_conf.dst_addr = spi_phyaddr[bus_num] + SPI_TXDATA_REG;
			spi_hw_conf.irq_spt = CHAN_IRQ_QD;

			spi_hw_conf.xfer_type = DMAXFER_D_SBYTE_S_SBYTE;
			spi_hw_conf.byte_cnt = len;
			spi_hw_conf.bconti_mode = false;
			spi_hw_conf.para = 3;

			/* flush d-cache */
			sun6i_spi_cleanflush_dcache_region((void *)buf, len);
			/* set src, dst, drq type, configuration */
			ret = sw_dma_config(sspi->dma_hdle_tx, &spi_hw_conf, ENQUE_PHASE_NORMAL);
			break;
		default:
            return -1;
	}

    return ret;
}

/* set dma start flag, if queue, it will auto restart to transfer next queue */
static int sun6i_spi_start_dma(struct sun6i_spi *sspi, enum spi_dma_dir dma_dir)
{
	int ret = 0;

	switch(dma_dir)
    {
        case SPI_DMA_RDEV:
            ret = sw_dma_ctl(sspi->dma_hdle_rx, DMA_OP_START, NULL);
            break;
        case SPI_DMA_WDEV:
            ret = sw_dma_ctl(sspi->dma_hdle_tx, DMA_OP_START, NULL);
            break;
        default:
            return -1;
	}

    return ret;
}

/* release dma channel, and set queue status to idle. */
static int sun6i_spi_release_dma(struct sun6i_spi *sspi, enum spi_dma_dir dma_dir)
{
    int ret = 0;
    unsigned long flags;

    switch(dma_dir)
    {
        case SPI_DMA_RDEV:
            spin_lock_irqsave(&sspi->lock, flags);
			ret = sw_dma_ctl(sspi->dma_hdle_rx, DMA_OP_STOP, NULL); /* first stop */
			ret += sw_dma_release(sspi->dma_hdle_rx);
            sspi->dma_hdle_rx = NULL;
            sspi->dma_dir_rdev = SPI_DMA_RWNULL;
            spin_unlock_irqrestore(&sspi->lock, flags);
            break;
        case SPI_DMA_WDEV:
            spin_lock_irqsave(&sspi->lock, flags);
			ret = sw_dma_ctl(sspi->dma_hdle_tx, DMA_OP_STOP, NULL); /* first stop */
			ret += sw_dma_release(sspi->dma_hdle_tx);
            sspi->dma_hdle_tx = NULL;
            sspi->dma_dir_wdev  = SPI_DMA_RWNULL;
            spin_unlock_irqrestore(&sspi->lock, flags);
            break;
        default:
            return -1;
    }

    return ret;
}
/* ------------------------------dma operation end----------------------------- */

/* check the valid of cs id */
static int sun6i_spi_check_cs(int cs_id, struct sun6i_spi *sspi)
{
    int ret = SUN6I_SPI_FAIL;

    switch(cs_id)
    {
        case 0:
            ret = (sspi->cs_bitmap & SPI_CHIP_SELECT_CS0) ? SUN6I_SPI_OK : SUN6I_SPI_FAIL;
            break;
        case 1:
            ret = (sspi->cs_bitmap & SPI_CHIP_SELECT_CS1) ? SUN6I_SPI_OK : SUN6I_SPI_FAIL;
            break;
        default:
            SPI_ERR("[spi-%d]: chip select not support! cs = %d \n", sspi->master->bus_num, cs_id);
            break;
    }

    return ret;
}

/* spi device on or off control */
static void sun6i_spi_cs_control(struct spi_device *spi, bool on)
{
	struct sun6i_spi *sspi = spi_master_get_devdata(spi->master);
	unsigned int cs = 0;
	if (sspi->cs_control) {
		if(on) {
			/* set active */
			cs = (spi->mode & SPI_CS_HIGH) ? 1 : 0;
		}
		else {
			/* set inactive */
			cs = (spi->mode & SPI_CS_HIGH) ? 0 : 1;
		}
		spi_ss_level(sspi->base_addr, cs);
	}
}

/*
 * change the properties of spi device with spi transfer.
 * every spi transfer must call this interface to update the master to the excute transfer
 * set clock frequecy, bits per word, mode etc...
 * return:  >= 0 : succeed;    < 0: failed.
 */
static int sun6i_spi_xfer_setup(struct spi_device *spi, struct spi_transfer *t)
{
	/* get at the setup function, the properties of spi device */
	struct sun6i_spi *sspi = spi_master_get_devdata(spi->master);
	struct sun6i_spi_config *config = spi->controller_data; //allocate in the setup,and free in the cleanup
    void *__iomem base_addr = sspi->base_addr;

	config->max_speed_hz  = (t && t->speed_hz) ? t->speed_hz : spi->max_speed_hz;
	config->bits_per_word = (t && t->bits_per_word) ? t->bits_per_word : spi->bits_per_word;
	config->bits_per_word = ((config->bits_per_word + 7) / 8) * 8;

	if(config->bits_per_word != 8) {
	    SPI_ERR("[spi-%d]: just support 8bits per word... \n", spi->master->bus_num);
	    return -EINVAL;
	}

	if(spi->chip_select >= spi->master->num_chipselect) {
	    SPI_ERR("[spi-%d]: spi device's chip select = %d exceeds the master supported cs_num[%d] \n",
	                    spi->master->bus_num, spi->chip_select, spi->master->num_chipselect);
	    return -EINVAL;
	}

	/* check again board info */
	if( SUN6I_SPI_OK != sun6i_spi_check_cs(spi->chip_select, sspi) ) {
	    SPI_ERR("sun6i_spi_check_cs failed! spi_device cs =%d ...\n", spi->chip_select);
	    return -EINVAL;
	}

	/* set cs */
	spi_set_cs(spi->chip_select, base_addr);
    /*
     *  master: set spi module clock;
     *  set the default frequency	10MHz
     */
    spi_set_master(base_addr);
   	if(config->max_speed_hz > SPI_MAX_FREQUENCY) {
	    return -EINVAL;
	}
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	spi_set_clk(config->max_speed_hz, clk_get_rate(sspi->mclk), base_addr);
#else
	spi_set_clk(config->max_speed_hz, 24000000, base_addr);
#endif
    /*
     *  master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     *  set bit width-default: 8 bits
     */
    spi_config_tc(1, spi->mode, base_addr);
    spi_set_sample_delay(sspi);
	spi_enable_tp(base_addr);

	return 0;
}


/*
 * < 64 : cpu ;  >= 64 : dma
 * wait for done completion in this function, wakup in the irq hanlder
 */
static int sun6i_spi_xfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct sun6i_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem* base_addr = sspi->base_addr;
	unsigned long flags = 0;
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned rx_len = t->len;	/* number of bytes sent */
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	struct sun6i_dual_mode_dev_data *dual_mode_cfg = (struct sun6i_dual_mode_dev_data *)spi->dev.platform_data;
	int ret = 0;

    SPI_DBG("[spi-%d]: begin transfer, txbuf %p, rxbuf %p, len %d\n", spi->master->bus_num, tx_buf, rx_buf, t->len);
    if ((!t->tx_buf && !t->rx_buf) || !t->len)
		return -EINVAL;

    /* write 1 to clear 0 */
    spi_clr_irq_pending(SPI_INT_STA_MASK, base_addr);
    /* disable all DRQ */
    spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, base_addr);
    /* reset tx/rx fifo */
    spi_reset_fifo(base_addr);

    if(sspi->mode_type != MODE_TYPE_NULL)
        return -EINVAL;

	/* single spi mode */
	if(!dual_mode_cfg || dual_mode_cfg->dual_mode == 0){
		SPI_DBG("in single SPI mode\n");
		/* full duplex */
		if(tx_buf && rx_buf){
			spin_lock_irqsave(&sspi->lock, flags);
			spi_set_all_burst_received(sspi->base_addr);
			spi_set_bc_tc_stc(tx_len, 0, tx_len, 0, base_addr);
			sspi->mode_type = SINGLE_FULL_DUPLEX_RX_TX;
			spin_unlock_irqrestore(&sspi->lock, flags);
		}else{
			/* half duplex transmit(single mode) */
			if(tx_buf){
				spin_lock_irqsave(&sspi->lock, flags);
				spi_set_bc_tc_stc(tx_len, 0, tx_len, 0, base_addr);
				sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
				spin_unlock_irqrestore(&sspi->lock, flags);
			}/* half duplex receive(single mode) */
			else if(rx_buf){
				spin_lock_irqsave(&sspi->lock, flags);
				spi_set_bc_tc_stc(0, rx_len, 0, 0, base_addr);
				sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
				spin_unlock_irqrestore(&sspi->lock, flags);
			}
		}
	}else{
		/* dual spi mode */
		if(dual_mode_cfg->dual_mode == 1){
			SPI_DBG("in dual SPI mode\n");
			if(tx_buf && rx_buf){
				SPI_ERR("full duplex is not support in dual spi mode\n");
				return -1;
			}else{
				/* half duplex transmit(dual mode) */
				if(tx_buf){
					if(dual_mode_cfg->single_cnt >= tx_len){
						SPI_ERR("single tranmit count must be less than total transmit count in dual spi mode\n");
						return -1;
					}
					spin_lock_irqsave(&sspi->lock, flags);
					spi_set_bc_tc_stc(tx_len, 0, dual_mode_cfg->single_cnt, 0, base_addr);
					sspi->mode_type = DUAL_HALF_DUPLEX_TX;
					spin_unlock_irqrestore(&sspi->lock, flags);
				}/* half duplex receive(dual mode) */
				else if(rx_buf){
					spin_lock_irqsave(&sspi->lock, flags);
					spi_set_dual_read(base_addr);
					spi_set_bc_tc_stc(dual_mode_cfg->single_cnt, rx_len, dual_mode_cfg->single_cnt, dual_mode_cfg->dummy_cnt, base_addr);
					sspi->mode_type = DUAL_HALF_DUPLEX_RX;
					spin_unlock_irqrestore(&sspi->lock, flags);
				}
			}
		}else{
			SPI_ERR("the value of dual_mode is error!\n");
			return -1;
		}
	}

    /*
     * 1. Tx/Rx error irq,process in IRQ;
     * 2. Transfer Complete Interrupt Enable
     */
    spi_enable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);

    /* >64 use DMA transfer, or use cpu */
    if(t->len > BULK_DATA_BOUNDARY) {
        switch(sspi->mode_type)
        {
            case SINGLE_HALF_DUPLEX_RX:
            {
                SPI_DBG(" rx -> by dma\n");
                /* rxFIFO reday dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
				sspi->dma_dir_rdev = SPI_DMA_RDEV;
                ret = sun6i_spi_prepare_dma(sspi, sspi->dma_dir_rdev);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sun6i_spi_config_dma(sspi, sspi->dma_dir_rdev, (void *)rx_buf, rx_len);
                sun6i_spi_start_dma(sspi, sspi->dma_dir_rdev);

                spi_start_xfer(base_addr);
                break;
            }
            case SINGLE_HALF_DUPLEX_TX:
            {
                SPI_DBG(" tx -> by dma\n");
                spi_start_xfer(base_addr);
                /* txFIFO empty dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
				sspi->dma_dir_wdev = SPI_DMA_WDEV;
                ret = sun6i_spi_prepare_dma(sspi, sspi->dma_dir_wdev);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sun6i_spi_config_dma(sspi, sspi->dma_dir_wdev, (void *)tx_buf, tx_len);
                sun6i_spi_start_dma(sspi, sspi->dma_dir_wdev);
                break;
            }
            case SINGLE_FULL_DUPLEX_RX_TX:
            {
                SPI_DBG(" rx and tx -> by dma\n");
                /* rxFIFO reday dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
				sspi->dma_dir_rdev = SPI_DMA_RDEV;
                ret = sun6i_spi_prepare_dma(sspi, sspi->dma_dir_rdev);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sun6i_spi_config_dma(sspi, sspi->dma_dir_rdev, (void *)rx_buf, rx_len);
                sun6i_spi_start_dma(sspi, sspi->dma_dir_rdev);

                spi_start_xfer(base_addr);

                /* rxFIFO empty dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
				sspi->dma_dir_wdev = SPI_DMA_WDEV;
                ret = sun6i_spi_prepare_dma(sspi, sspi->dma_dir_wdev);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sun6i_spi_config_dma(sspi, sspi->dma_dir_wdev, (void *)t->tx_buf, tx_len);
                sun6i_spi_start_dma(sspi, sspi->dma_dir_wdev);
                break;
            }
			case DUAL_HALF_DUPLEX_RX:
			{
				SPI_ERR("dual half duplex rx -> by dma (not support now)\n");
				break;
			}
			case DUAL_HALF_DUPLEX_TX:
			{
				SPI_ERR("dual half duplex tx -> by dma (not support now)\n");
				break;
			}
            default:
                return -1;
        }
    }
    else {
        switch(sspi->mode_type)
        {
            case SINGLE_HALF_DUPLEX_RX:
            {
                unsigned int poll_time = 0x7ffff;
                SPI_DBG(" rx -> by ahb\n");
                /* SMC=1,XCH trigger the transfer */
                spi_start_xfer(base_addr);
                while(rx_len && (--poll_time >0)) {
                    /* rxFIFO counter */
                    if(spi_query_rxfifo(base_addr)){
                        *rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);//fetch data
                        --rx_len;
                    }
                }
                if(poll_time <= 0) {
                    SPI_ERR("cpu receive data time out!\n");
					return -1;
                }
                break;
            }
            case SINGLE_HALF_DUPLEX_TX:
            {
                unsigned int poll_time = 0xfffff;
                SPI_DBG(" tx -> by ahb\n");
                spi_start_xfer(base_addr);

                spin_lock_irqsave(&sspi->lock, flags);
                for(; tx_len > 0; --tx_len) {
                    writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
                }
                spin_unlock_irqrestore(&sspi->lock, flags);

                while(spi_query_txfifo(base_addr)&&(--poll_time > 0) );/* txFIFO counter */
                if(poll_time <= 0) {
                    SPI_ERR("cpu transfer data time out!\n");
					return -1;
                }
                break;
            }
            case SINGLE_FULL_DUPLEX_RX_TX:
            {
                unsigned int poll_time_tx = 0xfffff;
                unsigned int poll_time_rx = 0x7ffff;
                SPI_DBG(" rx and tx -> by ahb\n");
                if((rx_len == 0) || (tx_len == 0))
                    return -EINVAL;

                spi_start_xfer(base_addr);

                spin_lock_irqsave(&sspi->lock, flags);
                for(; tx_len > 0; --tx_len) {
                    writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
                }
                spin_unlock_irqrestore(&sspi->lock, flags);

                while(spi_query_txfifo(base_addr)&&(--poll_time_tx > 0) );/* txFIFO counter */
                if(poll_time_tx <= 0) {
                    SPI_ERR("cpu transfer data time out!\n");
					return -1;
                }

                while(rx_len && (--poll_time_rx >0)) {
                    /* rxFIFO counter */
                    if(spi_query_rxfifo(base_addr)){
                        *rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);//fetch data
                        --rx_len;
                    }
                }
                if(poll_time_rx <= 0) {
                    SPI_ERR("cpu receive data time out!\n");
					return -1;
                }
                break;
            }
			case DUAL_HALF_DUPLEX_RX:
			{
				SPI_ERR("dual half duplex rx -> by ahb (not support now)\n");
				break;
			}
			case DUAL_HALF_DUPLEX_TX:
			{
				SPI_ERR("dual half duplex tx -> by ahb (not support now)\n");
				break;
			}
            default:
                return -1;
        }
    }
	/* wait for xfer complete in the isr. */
	wait_for_completion(&sspi->done);
    /* get the isr return code */
    if(sspi->result != 0) {
        SPI_ERR("[spi-%d]: xfer failed... \n", spi->master->bus_num);
        ret = -1;
    }
    /* release dma resource if neccessary */
    if(sspi->dma_dir_rdev != SPI_DMA_RWNULL)
        sun6i_spi_release_dma(sspi, sspi->dma_dir_rdev);

    if(sspi->dma_dir_wdev != SPI_DMA_RWNULL)
        sun6i_spi_release_dma(sspi, sspi->dma_dir_wdev);

    if(sspi->mode_type != MODE_TYPE_NULL)
        sspi->mode_type = MODE_TYPE_NULL;

	return ret;
}

/* spi core xfer process */
static void sun6i_spi_work(struct work_struct *work)
{
	struct sun6i_spi *sspi = container_of(work, struct sun6i_spi, work);
	spin_lock_irq(&sspi->lock);
	sspi->busy = SPI_BUSY;
	/*
     * get from messages queue, and then do with them,
	 * if message queue is empty ,then return and set status to free,
	 * otherwise process them.
	 */
	while (!list_empty(&sspi->queue)) {
		struct spi_message *msg = NULL;
		struct spi_device  *spi = NULL;
		struct spi_transfer *t  = NULL;
		unsigned int cs_change = 0;
		int status;
		/* get message from message queue in sun6i_spi. */
		msg = container_of(sspi->queue.next, struct spi_message, queue);
		/* then delete from the message queue,now it is alone.*/
		list_del_init(&msg->queue);
		spin_unlock_irq(&sspi->lock);
		/* get spi device from this message */
		spi = msg->spi;
		/* set default value,no need to change cs,keep select until spi transfer require to change cs. */
		cs_change = 1;
		/* set message status to succeed. */
		status = 0;
		/* search the spi transfer in this message, deal with it alone. */
		list_for_each_entry (t, &msg->transfers, transfer_list) {
			if (t->bits_per_word || t->speed_hz) { /* if spi transfer is zero,use spi device value. */
				status = sun6i_spi_xfer_setup(spi, t);/* set the value every spi transfer */
				if (status < 0)
					break;/* fail, quit */
				SPI_DBG("[spi-%d]: xfer setup \n", sspi->master->bus_num);
			}
			/* first active the cs */
			if (cs_change)
				sspi->cs_control(spi, 1);
			/* update the new cs value */
			cs_change = t->cs_change;
			/*
             * do transfer
			 * > 64 : dma ;  <= 64 : cpu
			 * wait for done completion in this function, wakup in the irq hanlder
			 */
			status = sun6i_spi_xfer(spi, t);
			if (status)
				break;/* fail quit, zero means succeed */
			/* accmulate the value in the message */
			msg->actual_length += t->len;
			/* may be need to delay */
			if (t->delay_usecs)
				udelay(t->delay_usecs);
			/* if zero ,keep active,otherwise deactived. */
			if (cs_change)
				sspi->cs_control(spi, 0);
		}
		/*
		 * spi message complete,succeed or failed
		 * return value
		 */
		msg->status = status;
		/* wakup the uplayer caller,complete one message */
		msg->complete(msg->context);
		/* fail or need to change cs */
		if (status || !cs_change) {
			sspi->cs_control(spi, 0);
		}
        /* restore default value. */
		sun6i_spi_xfer_setup(spi, NULL);
		spin_lock_irq(&sspi->lock);
	}
	/* set spi to free */
	sspi->busy = SPI_FREE;
	spin_unlock_irq(&sspi->lock);
}

/* wake up the sleep thread, and give the result code */
static irqreturn_t sun6i_spi_handler(int irq, void *dev_id)
{
	struct sun6i_spi *sspi = (struct sun6i_spi *)dev_id;
	void *base_addr = sspi->base_addr;
    unsigned int status = spi_qry_irq_pending(base_addr);
    spi_clr_irq_pending(status, base_addr);//write 1 to clear 0.
    SPI_DBG("[spi-%d]: irq status = %x \n", sspi->master->bus_num, status);

    sspi->result = 0; /* assume succeed */
    /* master mode, Transfer Complete Interrupt */
    if(status & SPI_INT_STA_TC){
        SPI_DBG("[spi-%d]: SPI TC comes\n", sspi->master->bus_num);
        spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);
        /*
         * just check dma+callback receive,skip other condition.
         * dma+callback receive: when TC comes,dma may be still not complete fetch data from rxFIFO.
         * other receive: cpu or dma+poll,just skip this.
         */
        if(sspi->dma_dir_rdev == SPI_DMA_RDEV) {
            unsigned int poll_time = 0xffff;
            /*during poll,dma maybe complete rx,rx_dma_used is 0. then return.*/
            while(spi_query_rxfifo(base_addr)&&(--poll_time > 0));
            if(poll_time <= 0) {
                SPI_ERR("[spi-%d]: dma callback method, rx data time out in irq !\n", sspi->master->bus_num);
                sspi->result = -1;// failed
                complete(&sspi->done);
                return SUN6I_SPI_FAIL;
            }
            else if(poll_time < 0xffff) {
                SPI_DBG("[spi-%d]: rx irq comes first, dma last. wait = 0x%x\n", sspi->master->bus_num, poll_time);
            }
        }
        /*wakup uplayer, by the sem */
        complete(&sspi->done);
        return IRQ_HANDLED;
    }/* master mode:err */
    else if(status & SPI_INT_STA_ERR){
        SPI_ERR("[spi-%d]: SPI ERR comes\n", sspi->master->bus_num);
        /* error process, release dma in the workqueue,should not be here */
        spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);
        //spi_restore_state(1, base_addr);
		spi_soft_reset(base_addr);
        sspi->result = -1;
        complete(&sspi->done);
        SPI_ERR("[spi-%d]: master mode error: txFIFO overflow/rxFIFO underrun or overflow\n", sspi->master->bus_num);
        return IRQ_HANDLED;
    }
    SPI_DBG("[spi-%d]: SPI NONE comes\n", sspi->master->bus_num);
    return IRQ_NONE;
}

/* interface 1 */
static int sun6i_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct sun6i_spi *sspi = spi_master_get_devdata(spi->master);
	unsigned long flags;
	msg->actual_length = 0;
	msg->status = -EINPROGRESS;

	spin_lock_irqsave(&sspi->lock, flags);
    /* add msg to the sun6i_spi queue */
	list_add_tail(&msg->queue, &sspi->queue);
    /* add work to the workqueue,schedule the cpu. */
	queue_work(sspi->workqueue, &sspi->work);
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}

/* interface 2, setup the frequency and default status */
static int sun6i_spi_setup(struct spi_device *spi)
{
	struct sun6i_spi *sspi = spi_master_get_devdata(spi->master);
	struct sun6i_spi_config *config = spi->controller_data;/* general is null. */
	unsigned long flags;

    /* just support 8 bits per word */
	if (spi->bits_per_word != 8)
		return -EINVAL;
    /* first check its valid,then set it as default select,finally set its */
    if(SUN6I_SPI_FAIL == sun6i_spi_check_cs(spi->chip_select, sspi)) {
        SPI_ERR("[spi-%d]: not support cs-%d \n", spi->master->bus_num, spi->chip_select);
        return -EINVAL;
    }
    if(spi->max_speed_hz > SPI_MAX_FREQUENCY)
	    return -EINVAL;
	if (!config) {
		config = kzalloc(sizeof *config, GFP_KERNEL);
		if (!config)
			return -ENOMEM;
		spi->controller_data = config;
	}
    /*
     * set the default vaule with spi device
     * can change by every spi transfer
     */
	config->bits_per_word = spi->bits_per_word;
	config->max_speed_hz  = spi->max_speed_hz;
	config->mode		  = spi->mode;

	spin_lock_irqsave(&sspi->lock, flags);
	/* if spi is free, then deactived the spi device */
	if (sspi->busy & SPI_FREE) {
		/* set chip select number */
	    spi_set_cs(spi->chip_select, sspi->base_addr);
	    /* deactivate chip select */
		sspi->cs_control(spi, 0);
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}

/* interface 3 */
static void sun6i_spi_cleanup(struct spi_device *spi)
{
    if(spi->controller_data) {
        kfree(spi->controller_data);
        spi->controller_data = NULL;
    }
}


#ifdef CONFIG_AW_ASIC_EVB_PLATFORM

static void sun6i_spi_request_gpio(struct sun6i_spi *sspi)
{
	int	cnt, i;
	char spi_para[16] = {0};
	script_item_u used, *list = NULL;
	script_item_value_type_e type;

	sprintf(spi_para, "spi%d_para", sspi->master->bus_num);
	type = script_get_item(spi_para, "spi_used", &used);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		SPI_ERR("[spi-%d] spi_used err!\n", sspi->master->bus_num);
		return;
	}

	if (1 == used.val) {
		/* ªÒ»°gpio list */
		cnt = script_get_pio_list(spi_para, &list);
		if (0 == cnt) {
			SPI_ERR("[spi-%d] get gpio list failed\n", sspi->master->bus_num);
			return;
		}

		/* …Í«Îgpio */
		for (i = 0; i < cnt; i++)
			if (0 != gpio_request(list[i].gpio.gpio, NULL))
				goto end;

		/* ≈‰÷√gpio list */
		if (0 != sw_gpio_setall_range(&list[0].gpio, cnt))
			SPI_ERR("[spi-%d] sw_gpio_setall_range failed\n", sspi->master->bus_num);
	}

	return;

end:
	/*  Õ∑≈gpio */
	while (i--)
		gpio_free(list[i].gpio.gpio);
}

static void sun6i_spi_release_gpio(struct sun6i_spi *sspi)
{
	int gpio_cnt, i;
	script_item_u *list = NULL;
	char spi_para[16] = {0};

	sprintf(spi_para, "spi%d_para", sspi->master->bus_num);
	gpio_cnt = script_get_pio_list(spi_para, &list);
	for(i = 0; i < gpio_cnt; i++)
		gpio_free(list[i].gpio.gpio);
}

static int sun6i_spi_set_mclk(struct sun6i_spi *sspi, u32 mod_clk)
{
    struct clk *source_clock = NULL;
    char* name = NULL;
    u32 source = 1;
    int ret = 0;
	long rate = 0;

    switch (source)
    {
        case 0:
            source_clock = clk_get(NULL, "sys_hosc");
            name = "sys_hosc";
            break;
        case 1:
            source_clock = clk_get(NULL, "sys_pll6");
            name = "sys_pll6";
            break;
        default:
            return -1;
    }

    if (!source_clock || IS_ERR(source_clock)) {
		ret = PTR_ERR(source_clock);
		SPI_ERR("[spi-%d] Unable to get spi source clock resource\n", sspi->master->bus_num);
		return -1;
	}

    if (clk_set_parent(sspi->mclk, source_clock)) {
        SPI_ERR("[spi-%d] spi clk_set_parent failed\n", sspi->master->bus_num);
        ret = -1;
        goto out;
    }

	rate = clk_round_rate(sspi->mclk, mod_clk);
    if (clk_set_rate(sspi->mclk, rate)) {
        SPI_ERR("[spi-%d] spi clk_set_rate failed\n", sspi->master->bus_num);
        ret = -1;
        goto out;
    }

    SPI_INF("[spi-%d] source = %s, src_clk = %u, mclk %u\n", sspi->master->bus_num,
            name, (unsigned)clk_get_rate(source_clock), (unsigned)clk_get_rate(sspi->mclk));

	if (clk_enable(sspi->mclk)) {
		SPI_ERR("[spi-%d] Couldn't enable module clock 'spi'\n", sspi->master->bus_num);
		ret = -EBUSY;
		goto out;
	}
	ret = 0;

out:
    clk_put(source_clock);
	source_clock = NULL;

    return ret;
}

static int sun6i_spi_hw_init(struct sun6i_spi *sspi)
{
	void *base_addr = sspi->base_addr;
	unsigned long sclk_freq = 0;
	char* mclk_name[] = {CLK_MOD_SPI0, CLK_MOD_SPI1, CLK_MOD_SPI2, CLK_MOD_SPI3};

    sspi->mclk = clk_get(&sspi->pdev->dev, mclk_name[sspi->pdev->id]);
	if (!sspi->mclk || IS_ERR(sspi->mclk)) {
		SPI_ERR("[spi-%d] Unable to acquire module clock 'spi'\n", sspi->master->bus_num);
		return -1;
	}

    if (sun6i_spi_set_mclk(sspi, 100000000))
    {
        SPI_ERR("[spi-%d] sun6i_spi_set_mclk 'spi'\n", sspi->master->bus_num);
        clk_put(sspi->mclk);
		sspi->mclk = NULL;
        return -1;
    }

	if (clk_reset(sspi->mclk, AW_CCU_CLK_NRESET)) {
		SPI_ERR("[spi-%d] NRESET module clock failed!\n", sspi->master->bus_num);
		return -1;
	}

	/* 1. enable the spi module */
	spi_enable_bus(base_addr);
	/* 2. set the default chip select */
	if(SUN6I_SPI_OK == sun6i_spi_check_cs(0, sspi)) {
	    spi_set_cs(0, base_addr);
	}
	else{
        spi_set_cs(1, base_addr);
	}
    /*
     * 3. master: set spi module clock;
     * 4. set the default frequency	10MHz
     */
    spi_set_master(base_addr);
    sclk_freq  = clk_get_rate(sspi->mclk);
    spi_set_clk(10000000, sclk_freq, base_addr);
    /*
     * 5. master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     * 6. set bit width-default: 8 bits
     */
    spi_config_tc(1, SPI_MODE_0, base_addr);
    spi_set_sample_delay(sspi);
    spi_enable_tp(base_addr);
	/* 7. manual control the chip select */
	spi_ss_ctrl(base_addr, 1);

	return 0;
}

static int sun6i_spi_hw_exit(struct sun6i_spi *sspi)
{
	/* disable the spi controller */
    spi_disable_bus(sspi->base_addr);

	/* disable module clock */
	if(!sspi->mclk || IS_ERR(sspi->mclk)) {
		printk("[spi-%d] spi mclk handle is invalid, just return!\n", sspi->master->bus_num);
		return -1;
	} else {
		if (clk_reset( sspi->mclk, AW_CCU_CLK_RESET)) {
			printk("[spi-%d] RESET module clock failed!\n", sspi->master->bus_num);
			return -1;
		}
		clk_disable(sspi->mclk);
		clk_put(sspi->mclk);
		sspi->mclk = NULL;
	}

	if(!sspi->hclk || IS_ERR(sspi->hclk)) {
		printk("[spi-%d] spi hclk handle is invalid, just return!\n", sspi->master->bus_num);
		return -1;
	} else {
		clk_disable(sspi->hclk);
		clk_put(sspi->hclk);
		sspi->hclk = NULL;
	}

	sun6i_spi_release_gpio(sspi);

	return 0;
}
#else
static int sun6i_spi_hw_init(struct sun6i_spi *sspi)
{
	void *base_addr = sspi->base_addr;
	/* 1. enable the spi module */
	spi_enable_bus(base_addr);
	/* 2. set the default chip select */
	if(SUN6I_SPI_OK == sun6i_spi_check_cs(0, sspi)){
	    spi_set_cs(0, base_addr);
	}
	else{
        spi_set_cs(1, base_addr);
	}
    /*
     * 3. master: set spi module clock;
     * 4. set the default frequency	10MHz
     */
    spi_set_master(base_addr);
    spi_set_clk(10000000, 24000000, base_addr);
    /*
     * 5. master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     * 6. set bit width-default: 8 bits
     */
    spi_config_tc(1, SPI_MODE_0, base_addr);
    spi_set_sample_delay(sspi);
	/* 7. manual control the chip select */
	spi_ss_ctrl(base_addr, 1);
	spi_enable_tp(base_addr);

	return 0;
}

static int sun6i_spi_hw_exit(struct sun6i_spi *sspi)
{
	/* disable the spi controller */
    spi_disable_bus(sspi->base_addr);

	return 0;
}
#endif


static int __init sun6i_spi_probe(struct platform_device *pdev)
{
	struct resource	*mem_res;
	struct sun6i_spi *sspi;
	struct sun6i_spi_platform_data *pdata;
	struct spi_master *master;
	int ret = 0, err = 0, irq;
	int cs_bitmap = 0;

	if (pdev->id < 0) {
		SPI_ERR("Invalid platform device id-%d\n", pdev->id);
		return -ENODEV;
	}

	if (pdev->dev.platform_data == NULL) {
		SPI_ERR("platform_data missing!\n");
		return -ENODEV;
	}

	pdata = pdev->dev.platform_data;
	if (!pdata->clk_name) {
		SPI_ERR("spi clock name must be initialized!\n");
		return -EINVAL;
	}

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		SPI_ERR("Unable to get spi MEM resource\n");
		return -ENXIO;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		SPI_ERR("No spi IRQ specified\n");
		return -ENXIO;
	}

    /* create spi master */
	master = spi_alloc_master(&pdev->dev, sizeof(struct sun6i_spi));
	if (master == NULL) {
		SPI_ERR("Unable to allocate SPI Master\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);
	sspi = spi_master_get_devdata(master);
    memset(sspi, 0, sizeof(struct sun6i_spi));

	sspi->master          = master;
	sspi->irq             = irq;
	sspi->dma_hdle_rx     = NULL;
	sspi->dma_hdle_tx     = NULL;
	sspi->dma_dir_rdev		= SPI_DMA_RWNULL;
	sspi->dma_dir_wdev		= SPI_DMA_RWNULL;
	sspi->cs_control      = sun6i_spi_cs_control;
	sspi->cs_bitmap       = pdata->cs_bitmap; /* cs0-0x1; cs1-0x2; cs0&cs1-0x3. */
	sspi->busy            = SPI_FREE;
	sspi->mode_type		= MODE_TYPE_NULL;
    sspi->version       = sw_get_ic_ver();

	master->bus_num         = pdev->id;
	master->setup           = sun6i_spi_setup;
	master->cleanup         = sun6i_spi_cleanup;
	master->transfer        = sun6i_spi_transfer;
	master->num_chipselect  = pdata->num_cs;
	/* the spi->mode bits understood by this driver: */
	master->mode_bits       = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH| SPI_LSB_FIRST;
    /* update the cs bitmap */
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
    cs_bitmap = sun6i_spi_get_cfg_csbitmap(pdev->id);
#else
	cs_bitmap = 0x1;
#endif
	if(cs_bitmap & 0x3){
	    sspi->cs_bitmap = cs_bitmap & 0x3;
	    SPI_INF("[spi-%d]: cs bitmap from cfg = 0x%x \n", master->bus_num, cs_bitmap);
	}

	snprintf(sspi->irq_name, sizeof(sspi->irq_name), "sun6i-spi.%u", pdev->id);
	err = request_irq(sspi->irq, sun6i_spi_handler, IRQF_DISABLED, sspi->irq_name, sspi);
	if (err) {
		SPI_ERR("Cannot request IRQ\n");
		goto err0;
	}

	if (request_mem_region(mem_res->start,
			resource_size(mem_res), pdev->name) == NULL) {
		SPI_ERR("Req mem region failed\n");
		ret = -ENXIO;
		goto err1;
	}

	sspi->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (sspi->base_addr == NULL) {
		SPI_ERR("Unable to remap IO\n");
		ret = -ENXIO;
		goto err2;
	}

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	/* Setup clocks */
	sspi->hclk = clk_get(&pdev->dev, pdata->clk_name);
	if (!sspi->hclk || IS_ERR(sspi->hclk)) {
		SPI_ERR("Unable to acquire clock 'ahb spi'\n");
		ret = PTR_ERR(sspi->hclk);
		goto err3;
	}

	if (clk_enable(sspi->hclk)) {
		SPI_ERR("Couldn't enable clock 'ahb spi'\n");
		ret = -EBUSY;
		goto err4;
	}
#endif

	sspi->workqueue = create_singlethread_workqueue(dev_name(master->dev.parent));
	if (sspi->workqueue == NULL) {
		SPI_ERR("Unable to create workqueue\n");
		ret = -ENOMEM;
		goto err5;
	}

    sspi->pdev = pdev;

	/* Setup Deufult Mode */
	if (sun6i_spi_hw_init(sspi)) {
		SPI_ERR("spi hw init failed!\n");
		goto err6;
	}

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	sun6i_spi_request_gpio(sspi);
#endif

	spin_lock_init(&sspi->lock);
	init_completion(&sspi->done);
	INIT_WORK(&sspi->work, sun6i_spi_work);/* banding the process handler */
	INIT_LIST_HEAD(&sspi->queue);

	if (spi_register_master(master)) {
		SPI_ERR("cannot register SPI master\n");
		ret = -EBUSY;
		goto err7;
	}

	/* free spi_1 gpio for 40pin userspace control*/
	if(sspi->master->bus_num == 1)
		sun6i_spi_release_gpio(sspi);

	SPI_INF("allwinners SoC SPI Driver loaded for Bus SPI-%d with %d Slaves at most\n",
            pdev->id, master->num_chipselect);
	SPI_INF("[spi-%d]: driver probe succeed, base %p, irq %d!\n", master->bus_num, sspi->base_addr, sspi->irq);
	return 0;

err7:
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	if (sspi->mclk) {
		clk_disable(sspi->mclk);
		clk_put(sspi->mclk);
		sspi->mclk = NULL;
	}
#endif
err6:
	destroy_workqueue(sspi->workqueue);
err5:
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	if (sspi->hclk)
		clk_disable(sspi->hclk);
err4:
	if (sspi->hclk) {
		clk_put(sspi->hclk);
		sspi->hclk = NULL;
	}
err3:
#endif
	iounmap((void *)sspi->base_addr);
err2:
	release_mem_region(mem_res->start, resource_size(mem_res));
err1:
	free_irq(sspi->irq, sspi);
err0:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return ret;
}

static int sun6i_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sun6i_spi *sspi = spi_master_get_devdata(master);
	struct resource	*mem_res;
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy |= SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);

	while (sspi->busy & SPI_BUSY)
		msleep(10);

	sun6i_spi_hw_exit(sspi);
	spi_unregister_master(master);
	destroy_workqueue(sspi->workqueue);

	iounmap((void *) sspi->base_addr);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res != NULL)
		release_mem_region(mem_res->start, resource_size(mem_res));

	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return 0;
}

#ifdef CONFIG_PM
static int sun6i_spi_suspend(struct device *dev)
{
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sun6i_spi *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy |= SPI_SUSPND;
	spin_unlock_irqrestore(&sspi->lock, flags);

	while (sspi->busy & SPI_BUSY)
		msleep(10);

	/* disable spi bus */
	spi_disable_bus(sspi->base_addr);

	if (!sspi->mclk || IS_ERR(sspi->mclk)) {
		printk("sspi mclk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable(sspi->mclk);
	}

	/* disable clk */
	if (!sspi->hclk || IS_ERR(sspi->hclk)) {
		printk("sspi hclk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable(sspi->hclk);
	}

	SPI_INF("[spi-%d]: suspend okay.. \n", master->bus_num);
#endif

	return 0;
}

static int sun6i_spi_resume(struct device *dev)
{
#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sun6i_spi  *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	/* Enable the clock */
	clk_enable(sspi->hclk);
	sun6i_spi_hw_init(sspi);

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy = SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);
	SPI_INF("[spi-%d]: resume okay.. \n", master->bus_num);
#endif
	return 0;
}

static const struct dev_pm_ops sun6i_spi_dev_pm_ops = {
	.suspend = sun6i_spi_suspend,
	.resume  = sun6i_spi_resume,
};

#define SUN6I_SPI_DEV_PM_OPS (&sun6i_spi_dev_pm_ops)
#else
#define SUN6I_SPI_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static struct platform_driver sun6i_spi_driver = {
	.probe   = sun6i_spi_probe,
	.remove  = sun6i_spi_remove,
	.driver = {
        .name	= "sun6i-spi",
		.owner	= THIS_MODULE,
		.pm		= SUN6I_SPI_DEV_PM_OPS,
	},
};


/* ---------------- spi resouce and platform data start ---------------------- */
struct sun6i_spi_platform_data sun6i_spi0_pdata = {
	.cs_bitmap  = 0x1,
	.num_cs		= 1,
	.clk_name = CLK_AHB_SPI0,
};

static struct resource sun6i_spi0_resources[] = {
	[0] = {
		.start	= SPI0_BASE_ADDR_START,
		.end	= SPI0_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AW_IRQ_SPI0,
		.end	= AW_IRQ_SPI0,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device sun6i_spi0_device = {
	.name		= "sun6i-spi",
	.id			= 0,
	.num_resources	= ARRAY_SIZE(sun6i_spi0_resources),
	.resource	= sun6i_spi0_resources,
	.dev		= {
		.platform_data = &sun6i_spi0_pdata,
	},
};

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
struct sun6i_spi_platform_data sun6i_spi1_pdata = {
	.cs_bitmap	= 0x3,
	.num_cs		= 2,
	.clk_name = CLK_AHB_SPI1,
};

static struct resource sun6i_spi1_resources[] = {
	[0] = {
		.start	= SPI1_BASE_ADDR_START,
		.end	= SPI1_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AW_IRQ_SPI1,
		.end	= AW_IRQ_SPI1,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device sun6i_spi1_device = {
	.name		= "sun6i-spi",
	.id			= 1,
	.num_resources	= ARRAY_SIZE(sun6i_spi1_resources),
	.resource	= sun6i_spi1_resources,
	.dev		= {
		.platform_data = &sun6i_spi1_pdata,
	},
};

static struct resource sun6i_spi2_resources[] = {
	[0] = {
		.start	= SPI2_BASE_ADDR_START,
		.end	= SPI2_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AW_IRQ_SPI2,
		.end	= AW_IRQ_SPI2,
		.flags	= IORESOURCE_IRQ,
	}
};

struct sun6i_spi_platform_data sun6i_spi2_pdata = {
	.cs_bitmap	= 0x1,
	.num_cs		= 1,
	.clk_name = CLK_AHB_SPI2,
};

static struct platform_device sun6i_spi2_device = {
	.name		= "sun6i-spi",
	.id			= 2,
	.num_resources	= ARRAY_SIZE(sun6i_spi2_resources),
	.resource	= sun6i_spi2_resources,
	.dev		= {
		.platform_data = &sun6i_spi2_pdata,
	},
};

static struct resource sun6i_spi3_resources[] = {
	[0] = {
		.start	= SPI3_BASE_ADDR_START,
		.end	= SPI3_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AW_IRQ_SPI3,
		.end	= AW_IRQ_SPI3,
		.flags	= IORESOURCE_IRQ,
	}
};

struct sun6i_spi_platform_data sun6i_spi3_pdata = {
	.cs_bitmap	= 0x3,
	.num_cs		= 2,
	.clk_name = CLK_AHB_SPI3,
};

static struct platform_device sun6i_spi3_device = {
	.name		= "sun6i-spi",
	.id			= 3,
	.num_resources	= ARRAY_SIZE(sun6i_spi3_resources),
	.resource	= sun6i_spi3_resources,
	.dev		= {
		.platform_data = &sun6i_spi3_pdata,
	},
};
#endif
/* ---------------- spi resource and platform data end ----------------------- */

#ifdef CONFIG_AW_ASIC_EVB_PLATFORM
static struct spi_board_info *spi_boards = NULL;
int sun6i_spi_register_spidev(void)
{
    script_item_u spi_dev_num, temp_info;
    script_item_value_type_e type;
    int i, spidev_num;
    char spi_board_name[32] = {0};
    struct spi_board_info* board;

	type = script_get_item("spi_devices", "spi_dev_num", &spi_dev_num);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        SPI_ERR("Get spi devices number failed\n");
        return -1;
    }
	spidev_num = spi_dev_num.val;

    SPI_INF("[spi]: Found %d spi devices in config files\n", spidev_num);

    /* alloc spidev board information structure */
    spi_boards = (struct spi_board_info*)kzalloc(sizeof(struct spi_board_info) * spidev_num, GFP_KERNEL);
    if (spi_boards == NULL) {
        SPI_ERR("Alloc spi board information failed \n");
        return -1;
    }

    SPI_INF("%-16s %-16s %-16s %-8s %-4s %-4s\n", "boards_num", "modalias", "max_spd_hz", "bus_num", "cs", "mode");

    for (i=0; i<spidev_num; i++) {
        board = &spi_boards[i];
        sprintf(spi_board_name, "spi_board%d", i);

		type = script_get_item(spi_board_name, "modalias", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			SPI_ERR("Get spi devices modalias failed\n");
            goto fail;
		}
		sprintf(board->modalias, "%s", temp_info.str);

		type = script_get_item(spi_board_name, "max_speed_hz", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices max_speed_hz failed\n");
            goto fail;
		}
		board->max_speed_hz = temp_info.val;

		type = script_get_item(spi_board_name, "bus_num", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices bus_num failed\n");
            goto fail;
		}
		board->bus_num = temp_info.val;

		type = script_get_item(spi_board_name, "chip_select", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices chip_select failed\n");
            goto fail;
		}
		board->chip_select = temp_info.val;

		type = script_get_item(spi_board_name, "mode", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices mode failed\n");
            goto fail;
		}
		board->mode = temp_info.val;

        SPI_INF("%-16d %-16s %-16d %-8d %-4d %-4d\n", i, board->modalias, board->max_speed_hz,
                board->bus_num, board->chip_select, board->mode);
    }

    /* register boards */
    if (spi_register_board_info(spi_boards, spidev_num)) {
        SPI_ERR("Register board information failed\n");
        goto fail;
    }

    return 0;

fail:
    if (spi_boards)
    {
        kfree(spi_boards);
        spi_boards = NULL;
    }

    return -1;
}

static int sun6i_spi_get_cfg_csbitmap(int bus_num)
{
    script_item_u cs_bitmap;
    script_item_value_type_e type;
    char *main_name[] = {"spi0_para", "spi1_para", "spi2_para", "spi3_para"};

    type = script_get_item(main_name[bus_num], "spi_cs_bitmap", &cs_bitmap);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
        SPI_ERR("[spi-%d] get spi_cs_bitmap from sysconfig failed\n", bus_num);
        return 0;
    }

    return cs_bitmap.val;
}

/* get configuration in the script */
#define SPI0_USED_MASK 0x1
#define SPI1_USED_MASK 0x2
#define SPI2_USED_MASK 0x4
#define SPI3_USED_MASK 0x8
static int spi_used_mask = 0;

static int __init spi_sun6i_init(void)
{
    script_item_u used;
    script_item_value_type_e type;
    int i, ret = 0;
    char spi_para[16] = {0};

    for (i=0; i<4; i++) {
        sprintf(spi_para, "spi%d_para", i);

		type = script_get_item(spi_para, "spi_used", &used);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("[spi-%d] fetch para from sysconfig failed\n", i);
			continue;
		}

        if (used.val)
            spi_used_mask |= 1 << i;
    }

    ret = sun6i_spi_register_spidev();
    if (ret)
        SPI_ERR("register spi devices board info failed \n");

    if (spi_used_mask & SPI0_USED_MASK)
        platform_device_register(&sun6i_spi0_device);

    if (spi_used_mask & SPI1_USED_MASK)
        platform_device_register(&sun6i_spi1_device);

    if (spi_used_mask & SPI2_USED_MASK)
        platform_device_register(&sun6i_spi2_device);

    if (spi_used_mask & SPI3_USED_MASK)
        platform_device_register(&sun6i_spi3_device);

    if (spi_used_mask)
        return platform_driver_register(&sun6i_spi_driver);

    SPI_INF("cannot find any using configuration for all spi controllers!\n");

    return 0;
}

static void __exit spi_sun6i_exit(void)
{
    if (spi_used_mask)
	    platform_driver_unregister(&sun6i_spi_driver);
}
#else
// static struct spi_board_info test_spi_info[] __initdata = {
	// {
		// .modalias		= "spitest",
		// .bus_num		= 0,
		// .chip_select	= 0,
		// .max_speed_hz	= 8000000,
	// },
// };

static int __init spi_sun6i_init(void)
{
	// spi_register_board_info(test_spi_info, ARRAY_SIZE(test_spi_info));
	platform_device_register(&sun6i_spi0_device);
	platform_driver_register(&sun6i_spi_driver);

	return 0;
}

static void __exit spi_sun6i_exit(void)
{
	platform_driver_unregister(&sun6i_spi_driver);
}
#endif

module_init(spi_sun6i_init);
module_exit(spi_sun6i_exit);

MODULE_AUTHOR("pannan");
MODULE_DESCRIPTION("SUN6I SPI BUS Driver");
MODULE_ALIAS("platform:sun6i-spi");
MODULE_LICENSE("GPL");
