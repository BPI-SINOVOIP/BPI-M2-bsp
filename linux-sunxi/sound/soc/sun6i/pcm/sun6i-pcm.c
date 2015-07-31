/*
 * sound\soc\sun6i\pcm\sun6i-pcm.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/clock.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/gpio.h>

#include "sun6i-pcmdma.h"
#include "sun6i-pcm.h"

struct sun6i_pcm_info sun6i_pcm;

static int regsave[8];
static int pcm_used 			= 0;
static int pcm_select 			= 0;
static int over_sample_rate 	= 0;
static int sample_resolution	= 0;
static int word_select_size 	= 0;
static int pcm_sync_period 		= 0;
static int msb_lsb_first 		= 0;
static int slot_index 			= 0;
static int slot_width 			= 0;
static int frame_width 			= 0;
static int tx_data_mode 		= 0;
static int rx_data_mode 		= 0;
static int sign_extend 			= 0;

static struct clk *pcm_apbclk 		= NULL;
static struct clk *pcm_pll2clk 		= NULL;
static struct clk *pcm_pllx8 		= NULL;
static struct clk *pcm_moduleclk	= NULL;

static struct sun6i_dma_params sun6i_pcm_pcm_stereo_out = {
	.name		= "pcm_play",
	.dma_addr	= SUN6I_PCMBASE + SUN6I_PCMTXFIFO,/*send data address	*/
};

static struct sun6i_dma_params sun6i_pcm_pcm_stereo_in = {
	.name   	= "pcm_capture",
	.dma_addr	=SUN6I_PCMBASE + SUN6I_PCMRXFIFO,/*accept data address	*/
};

static void sun6i_snd_txctrl_pcm(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_PCMTXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMTXCHSEL);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x3200;
	} else {
		reg_val = 0x3210;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMTXCHMAP);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_SDO0EN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	
	/*flush TX FIFO*/
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFCTL);
	reg_val |= SUN6I_PCMFCTL_FTX;	
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);
	
	/*clear TX counter*/
	writel(0, sun6i_pcm.regs + SUN6I_PCMTXCNT);

	if (on) {
		/* PCM TX ENABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_TXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
		
		/* enable DMA DRQ mode for play */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val |= SUN6I_PCMINT_TXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);
	} else {
		/* PCM TX DISABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_TXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val &= ~SUN6I_PCMINT_TXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);
	}		
}

static void sun6i_snd_rxctrl_pcm(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMRXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_PCMRXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMRXCHSEL);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMRXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x00003200;
	} else {
		reg_val = 0x00003210;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMRXCHMAP);
	
	/*flush RX FIFO*/
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFCTL);
	reg_val |= SUN6I_PCMFCTL_FRX;	
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);

	/*clear RX counter*/
	writel(0, sun6i_pcm.regs + SUN6I_PCMRXCNT);
	
	if (on) {
		/* PCM RX ENABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_RXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* enable DMA DRQ mode for record */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val |= SUN6I_PCMINT_RXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);
	} else {
		/* PCM RX DISABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_RXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val &= ~SUN6I_PCMINT_RXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);
	}		
}

static int sun6i_pcm_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
	u32 reg_val1;

	/*SDO ON*/
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= (SUN6I_PCMCTL_SDO0EN);
	if (pcm_select) {
		reg_val |= SUN6I_PCMCTL_PCM;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	/* master or slave selection */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master */
			reg_val |= SUN6I_PCMCTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave */
			reg_val &= ~SUN6I_PCMCTL_MS;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	/* pcm or pcm mode selection */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val1 = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	reg_val1 &= ~SUN6I_PCMFAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_I2S;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUN6I_PCMCTL_PCM;
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	writel(reg_val1, sun6i_pcm.regs + SUN6I_PCMFAT0);

	/* DAI signal inversions */
	reg_val1 = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			reg_val1 &= ~SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			reg_val1 &= ~SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			reg_val1 |= SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			reg_val1 |= SUN6I_PCMFAT0_BCP;
			break;
	}
	writel(reg_val1, sun6i_pcm.regs + SUN6I_PCMFAT0);
	
	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUN6I_PCMFCTL_RXTL(0x1f);				/*RX FIFO trigger level*/
	reg_val |= SUN6I_PCMFCTL_TXTL(0x40);				/*TX FIFO empty trigger level*/
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);

	return 0;
}

static int sun6i_pcm_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun6i_dma_params *dma_data;
	
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun6i_pcm_pcm_stereo_out;
	else
		dma_data = &sun6i_pcm_pcm_stereo_in;
	
	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sun6i_pcm_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 reg_val;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_pcm(substream, 1);
			} else {
				sun6i_snd_txctrl_pcm(substream, 1);
			}
			/*Global Enable Digital Audio Interface*/
			reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
			reg_val |= SUN6I_PCMCTL_GEN;
			writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_pcm(substream, 0);
			} else {
			  	sun6i_snd_txctrl_pcm(substream, 0);
			}
			/*Global disable Digital Audio Interface*/
			reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
			reg_val &= ~SUN6I_PCMCTL_GEN;
			writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sun6i_pcm_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int i2s_pcm_select)
{
	if (clk_set_rate(pcm_pll2clk, freq)) {
		printk("try to set the pcm_pll2clk rate failed!\n");
	}
	if (i2s_pcm_select != 0) {
		pcm_select = 0;
	} else {
		pcm_select = 1;
	}

	return 0;
}

static int sun6i_pcm_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	u32 reg_val;
	u32 fs;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	fs = div;
	mclk = over_sample_rate;

	if (!pcm_select) {
		/*mclk div caculate*/
		switch(fs)
		{
			case 8000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 24;
								break;
					case 192:	mclk_div = 16;
								break;
					case 256:	mclk_div = 12;
								break;
					case 384:	mclk_div = 8;
								break;
					case 512:	mclk_div = 6;
								break;
					case 768:	mclk_div = 4;
								break;
				}
				break;
			}

			case 16000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 12;
								break;
					case 192:	mclk_div = 8;
								break;
					case 256:	mclk_div = 6;
								break;
					case 384:	mclk_div = 4;
								break;
					case 768:	mclk_div = 2;
								break;
				}
				break;
			}
			
			case 32000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 6;
								break;
					case 192:	mclk_div = 4;
								break;
					case 384:	mclk_div = 2;
								break;
					case 768:	mclk_div = 1;
								break;
				}
				break;
			}
	
			case 64000:
			{
				switch(mclk)
				{
					case 192:	mclk_div = 2;
								break;
					case 384:	mclk_div = 1;
								break;
				}
				break;
			}
			
			case 128000:
			{
				switch(mclk)
				{
					case 192:	mclk_div = 1;
								break;
				}
				break;
			}
		
			case 11025:
			case 12000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 16;
								break;
					case 256:	mclk_div = 8;
								break;
					case 512:	mclk_div = 4;
								break;
				}
				break;
			}
		
			case 22050:
			case 24000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 8;
								break;
					case 256:	mclk_div = 4;
								break;
					case 512:	mclk_div = 2;
								break;
				}
				break;
			}
		
			case 44100:
			case 48000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 4;
								break;
					case 256:	mclk_div = 2;
								break;
					case 512:	mclk_div = 1;
								break;
				}
				break;
			}
				
			case 88200:
			case 96000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 2;
								break;
					case 256:	mclk_div = 1;
								break;
				}
				break;
			}
				
			case 176400:
			case 192000:
			{
				mclk_div = 1;
				break;
			}
		
		}
		/*bclk div caculate*/
		bclk_div = mclk/(2*word_select_size);
	} else {
		mclk_div = 4;
		bclk_div = 12;
	}

	/*calculate MCLK Divide Ratio*/
	switch(mclk_div)
	{
		case 1: mclk_div = 0;
				break;
		case 2: mclk_div = 1;
				break;
		case 4: mclk_div = 2;
				break;
		case 6: mclk_div = 3;
				break;
		case 8: mclk_div = 4;
				break;
		case 12: mclk_div = 5;
				 break;
		case 16: mclk_div = 6;
				 break;
		case 24: mclk_div = 7;
				 break;
		case 32: mclk_div = 8;
				 break;
		case 48: mclk_div = 9;
				 break;
		case 64: mclk_div = 0xA;
				 break;
	}
	mclk_div &= 0xf;

	/*calculate BCLK Divide Ratio*/
	switch(bclk_div)
	{
		case 2: bclk_div = 0;
				break;
		case 4: bclk_div = 1;
				break;
		case 6: bclk_div = 2;
				break;
		case 8: bclk_div = 3;
				break;
		case 12: bclk_div = 4;
				 break;
		case 16: bclk_div = 5;
				 break;
		case 32: bclk_div = 6;
				 break;
		case 64: bclk_div = 7;
				 break;
	}
	bclk_div &= 0x7;
	
	/*set mclk and bclk dividor register*/
	reg_val = mclk_div;
	reg_val |= (bclk_div<<4);
	reg_val |= (0x1<<7);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCLKD);

	/* word select size */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	sun6i_pcm.ws_size = word_select_size;
	reg_val &= ~SUN6I_PCMFAT0_WSS_32BCLK;
	if(sun6i_pcm.ws_size == 16)
		reg_val |= SUN6I_PCMFAT0_WSS_16BCLK;
	else if(sun6i_pcm.ws_size == 20) 
		reg_val |= SUN6I_PCMFAT0_WSS_20BCLK;
	else if(sun6i_pcm.ws_size == 24)
		reg_val |= SUN6I_PCMFAT0_WSS_24BCLK;
	else
		reg_val |= SUN6I_PCMFAT0_WSS_32BCLK;
	
	sun6i_pcm.samp_res = sample_resolution;
	reg_val &= ~SUN6I_PCMFAT0_SR_RVD;
	if(sun6i_pcm.samp_res == 16)
		reg_val |= SUN6I_PCMFAT0_SR_16BIT;
	else if(sun6i_pcm.samp_res == 20) 
		reg_val |= SUN6I_PCMFAT0_SR_20BIT;
	else
		reg_val |= SUN6I_PCMFAT0_SR_24BIT;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFAT0);

	/* PCM REGISTER setup */
	sun6i_pcm.pcm_txtype = tx_data_mode;
	sun6i_pcm.pcm_rxtype = rx_data_mode;
	reg_val = sun6i_pcm.pcm_txtype&0x3;
	reg_val |= sun6i_pcm.pcm_rxtype<<2;

	sun6i_pcm.pcm_sync_type = frame_width;
	if(sun6i_pcm.pcm_sync_type)
		reg_val |= SUN6I_PCMFAT1_SSYNC;	

	sun6i_pcm.pcm_sw = slot_width;
	if(sun6i_pcm.pcm_sw == 16)
		reg_val |= SUN6I_PCMFAT1_SW;

	sun6i_pcm.pcm_start_slot = slot_index;
	reg_val |=(sun6i_pcm.pcm_start_slot & 0x3)<<6;		

	sun6i_pcm.pcm_lsb_first = msb_lsb_first;
	reg_val |= sun6i_pcm.pcm_lsb_first<<9;			

	sun6i_pcm.pcm_sync_period = pcm_sync_period;
	if(sun6i_pcm.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sun6i_pcm.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sun6i_pcm.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sun6i_pcm.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFAT1);

PCM_DBG("%s, line:%d, slot_index:%d\n", __func__, __LINE__, slot_index);
PCM_DBG("%s, line:%d, slot_width:%d\n", __func__, __LINE__, slot_width);
PCM_DBG("%s, line:%d, pcm_select:%d\n", __func__, __LINE__, pcm_select);
PCM_DBG("%s, line:%d, frame_width:%d\n", __func__, __LINE__, frame_width);
PCM_DBG("%s, line:%d, tx_data_mode:%d\n", __func__, __LINE__, tx_data_mode);
PCM_DBG("%s, line:%d, rx_data_mode:%d\n", __func__, __LINE__, rx_data_mode);
PCM_DBG("%s, line:%d, msb_lsb_first:%d\n", __func__, __LINE__, msb_lsb_first);
PCM_DBG("%s, line:%d, pcm_sync_period:%d\n", __func__, __LINE__, pcm_sync_period);
PCM_DBG("%s, line:%d, word_select_size:%d\n", __func__, __LINE__, word_select_size);
PCM_DBG("%s, line:%d, over_sample_rate:%d\n", __func__, __LINE__, over_sample_rate);
PCM_DBG("%s, line:%d, sample_resolution:%d\n", __func__, __LINE__, sample_resolution);

	return 0;
}

static int sun6i_pcm_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}

static int sun6i_pcm_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void pcmregsave(void)
{
	regsave[0] = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	regsave[1] = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	regsave[2] = readl(sun6i_pcm.regs + SUN6I_PCMFAT1);
	regsave[3] = readl(sun6i_pcm.regs + SUN6I_PCMFCTL) | (0x3<<24);
	regsave[4] = readl(sun6i_pcm.regs + SUN6I_PCMINT);
	regsave[5] = readl(sun6i_pcm.regs + SUN6I_PCMCLKD);
	regsave[6] = readl(sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	regsave[7] = readl(sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
}

static void pcmregrestore(void)
{
	writel(regsave[0], sun6i_pcm.regs + SUN6I_PCMCTL);
	writel(regsave[1], sun6i_pcm.regs + SUN6I_PCMFAT0);
	writel(regsave[2], sun6i_pcm.regs + SUN6I_PCMFAT1);
	writel(regsave[3], sun6i_pcm.regs + SUN6I_PCMFCTL);
	writel(regsave[4], sun6i_pcm.regs + SUN6I_PCMINT);
	writel(regsave[5], sun6i_pcm.regs + SUN6I_PCMCLKD);
	writel(regsave[6], sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	writel(regsave[7], sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
}

static int sun6i_pcm_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[PCM]Entered %s\n", __func__);

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val &= ~SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	pcmregsave();
	if ((NULL == pcm_moduleclk) ||(IS_ERR(pcm_moduleclk))) {
		printk("pcm_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(pcm_moduleclk);
	}
	if ((NULL == pcm_apbclk) ||(IS_ERR(pcm_apbclk))) {
		printk("pcm_apbclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		clk_disable(pcm_apbclk);
	}
	return 0;
}

static int sun6i_pcm_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[PCM]Entered %s\n", __func__);

	/*release the module clock*/
	if (clk_enable(pcm_apbclk)) {
		printk("try to enable pcm_apbclk output failed!\n");
	}

	/*release the module clock*/
	if (clk_enable(pcm_moduleclk)) {
		printk("try to enable pcm_moduleclk output failed!\n");
	}
	
	pcmregrestore();
	
	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	
	return 0;
}

#define SUN6I_PCM_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun6i_pcm_dai_ops = {
	.trigger 	= sun6i_pcm_trigger,
	.hw_params 	= sun6i_pcm_hw_params,
	.set_fmt 	= sun6i_pcm_set_fmt,
	.set_clkdiv = sun6i_pcm_set_clkdiv,
	.set_sysclk = sun6i_pcm_set_sysclk, 
};

static struct snd_soc_dai_driver sun6i_pcm_dai = {	
	.probe 		= sun6i_pcm_dai_probe,
	.suspend 	= sun6i_pcm_suspend,
	.resume 	= sun6i_pcm_resume,
	.remove 	= sun6i_pcm_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops 		= &sun6i_pcm_dai_ops,	
};		

static int __devinit sun6i_pcm_dev_probe(struct platform_device *pdev)
{
	int ret;
	int reg_val = 0;
	script_item_u val;
	script_item_value_type_e  type;

	sun6i_pcm.regs = ioremap(SUN6I_PCMBASE, 0x100);
	if (sun6i_pcm.regs == NULL)
		return -ENXIO;

	/*pcm apbclk*/
	pcm_apbclk = clk_get(NULL, CLK_APB_I2S1);
	if ((!pcm_apbclk)||(IS_ERR(pcm_apbclk))) {
		printk("try to get pcm_apbclk failed\n");
	}
	if (clk_enable(pcm_apbclk)) {
		printk("pcm_apbclk failed! line = %d\n", __LINE__);
	}
	
	pcm_pllx8 = clk_get(NULL, CLK_SYS_PLL2X8);
	if ((!pcm_pllx8)||(IS_ERR(pcm_pllx8))) {
		printk("try to get pcm_pllx8 failed\n");
	}
	if (clk_enable(pcm_pllx8)) {
		printk("enable pcm_pll2clk failed; \n");
	}

	/*pcm pll2clk*/
	pcm_pll2clk = clk_get(NULL, CLK_SYS_PLL2);
	if ((!pcm_pll2clk)||(IS_ERR(pcm_pll2clk))) {
		printk("try to get pcm_pll2clk failed\n");
	}
	if (clk_enable(pcm_pll2clk)) {
		printk("enable pcm_pll2clk failed; \n");
	}

	/*pcm module clk*/
	pcm_moduleclk = clk_get(NULL, CLK_MOD_I2S1);
	if ((!pcm_moduleclk)||(IS_ERR(pcm_moduleclk))) {
		printk("try to get pcm_moduleclk failed\n");
	}

	if (clk_set_parent(pcm_moduleclk, pcm_pll2clk)) {
		printk("try to set parent of pcm_moduleclk to pcm_pll2ck failed! line = %d\n",__LINE__);
	}

	if (clk_set_rate(pcm_moduleclk, 24576000/8)) {
		printk("set pcm_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
	
	if (clk_enable(pcm_moduleclk)) {
		printk("open pcm_moduleclk failed! line = %d\n", __LINE__);
	}
	if (clk_reset(pcm_moduleclk, AW_CCU_CLK_NRESET)) {
		printk("try to NRESET pcm module clk failed!\n");
	}

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	type = script_get_item("pcm_para", "over_sample_rate", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] over_sample_rate type err!\n");
    }
	over_sample_rate = val.val;

	type = script_get_item("pcm_para", "sample_resolution", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] sample_resolution type err!\n");
    }
	sample_resolution = val.val;

	type = script_get_item("pcm_para", "word_select_size", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] word_select_size type err!\n");
    }
	word_select_size = val.val;

	type = script_get_item("pcm_para", "pcm_sync_period", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] pcm_sync_period type err!\n");
    }
	pcm_sync_period = val.val;

	type = script_get_item("pcm_para", "msb_lsb_first", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] msb_lsb_first type err!\n");
    }
	msb_lsb_first = val.val;

	type = script_get_item("pcm_para", "sign_extend", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] sign_extend type err!\n");
    }
	sign_extend = val.val;

	type = script_get_item("pcm_para", "slot_index", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] slot_index type err!\n");
    }
	slot_index = val.val;

	type = script_get_item("pcm_para", "slot_width", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] slot_width type err!\n");
    }
	slot_width = val.val;

	type = script_get_item("pcm_para", "frame_width", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] frame_width type err!\n");
    }
	frame_width = val.val;

	type = script_get_item("pcm_para", "tx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] tx_data_mode type err!\n");
    }
	tx_data_mode = val.val;

	type = script_get_item("pcm_para", "rx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] rx_data_mode type err!\n");
    }
	rx_data_mode = val.val;

	ret = snd_soc_register_dai(&pdev->dev, &sun6i_pcm_dai);	
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __devexit sun6i_pcm_dev_remove(struct platform_device *pdev)
{
	if (pcm_used) {
		pcm_used = 0;
		if ((NULL == pcm_moduleclk) ||(IS_ERR(pcm_moduleclk))) {
			printk("pcm_moduleclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release the module clock*/
			clk_disable(pcm_moduleclk);
		}
		if ((NULL == pcm_pllx8) ||(IS_ERR(pcm_pllx8))) {
			printk("pcm_pllx8 handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pllx8clk*/
			clk_put(pcm_pllx8);
		}
		if ((NULL == pcm_pll2clk) ||(IS_ERR(pcm_pll2clk))) {
			printk("pcm_pll2clk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pll2clk*/
			clk_put(pcm_pll2clk);
		}
		if ((NULL == pcm_apbclk) ||(IS_ERR(pcm_apbclk))) {
			printk("pcm_apbclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release apbclk*/
			clk_put(pcm_apbclk);
		}	
		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sun6i_pcm_device = {
	.name = "sun6i-pcm",
};

/*method relating*/
static struct platform_driver sun6i_pcm_driver = {
	.probe = sun6i_pcm_dev_probe,
	.remove = __devexit_p(sun6i_pcm_dev_remove),
	.driver = {
		.name = "sun6i-pcm",
		.owner = THIS_MODULE,
	},
};

static int __init sun6i_pcm_init(void)
{	
	int err = 0;
	int cnt = 0;
	int i 	= 0;
	script_item_u val;
	script_item_u *list = NULL;
	script_item_value_type_e  type;

	type = script_get_item("pcm_para", "pcm_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] type err!\n");
    }
	pcm_used = val.val;
	
	type = script_get_item("pcm_para", "pcm_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] pcm_select type err!\n");
    }
	pcm_select = val.val;
	
	if (pcm_used) {
		/* get gpio list */
		cnt = script_get_pio_list("pcm_para", &list);
		if (0 == cnt) {
			printk("get pcm_para gpio list failed\n");
			return -EFAULT;
		}
		/* req gpio */
		for (i = 0; i < cnt; i++) {
			if (0 != gpio_request(list[i].gpio.gpio, NULL)) {
				printk("[pcm] request some gpio fail\n");
				goto end;
			}
		}
		/* config gpio list */
		if (0 != sw_gpio_setall_range(&list[0].gpio, cnt)) {
			printk("[pcm]sw_gpio_setall_range failed\n");
		}

		if((err = platform_device_register(&sun6i_pcm_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&sun6i_pcm_driver)) < 0)
			return err;	
	} else {
        printk("[PCM]sun6i-pcm cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }

end:
	/* release gpio */
	while(i--)
		gpio_free(list[i].gpio.gpio);

	return 0;
}
module_init(sun6i_pcm_init);

static void __exit sun6i_pcm_exit(void)
{	
	platform_driver_unregister(&sun6i_pcm_driver);
}
module_exit(sun6i_pcm_exit);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sun6i PCM SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun6i-pcm");

