/*
 * sound\soc\sun6i\i2s\sun6i-i2s.h
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

#ifndef SUN6I_I2S_H_
#define SUN6I_I2S_H_

/*------------------------------------------------------------*/
/* REGISTER definition */

/* IIS REGISTER */
#define SUN6I_IISBASE 							(0x01C22000)

#define SUN6I_IISCTL 	  						(0x00)
	#define SUN6I_IISCTL_SDO3EN					(1<<11)
	#define SUN6I_IISCTL_SDO2EN					(1<<10)
	#define SUN6I_IISCTL_SDO1EN					(1<<9)
	#define SUN6I_IISCTL_SDO0EN					(1<<8) 
	#define SUN6I_IISCTL_ASS					(1<<6) 
	#define SUN6I_IISCTL_MS						(1<<5)
	#define SUN6I_IISCTL_PCM					(1<<4)
	#define SUN6I_IISCTL_LOOP					(1<<3)
	#define SUN6I_IISCTL_TXEN					(1<<2)
	#define SUN6I_IISCTL_RXEN					(1<<1)
	#define SUN6I_IISCTL_GEN					(1<<0)
	                                			
#define SUN6I_IISFAT0 							(0x04)
	#define SUN6I_IISFAT0_LRCP					(1<<7)
	#define SUN6I_IISFAT0_BCP					(1<<6)
	#define SUN6I_IISFAT0_SR_RVD				(3<<4)
	#define SUN6I_IISFAT0_SR_16BIT				(0<<4)
	#define	SUN6I_IISFAT0_SR_20BIT				(1<<4)
	#define SUN6I_IISFAT0_SR_24BIT				(2<<4)
	#define SUN6I_IISFAT0_WSS_16BCLK			(0<<2)
	#define SUN6I_IISFAT0_WSS_20BCLK			(1<<2)
	#define SUN6I_IISFAT0_WSS_24BCLK			(2<<2)
	#define SUN6I_IISFAT0_WSS_32BCLK			(3<<2)
	#define SUN6I_IISFAT0_FMT_I2S				(0<<0)
	#define SUN6I_IISFAT0_FMT_LFT				(1<<0)
	#define SUN6I_IISFAT0_FMT_RGT				(2<<0)
	#define SUN6I_IISFAT0_FMT_RVD				(3<<0)
	
#define SUN6I_IISFAT1							(0x08)
	#define SUN6I_IISFAT1_SYNCLEN_16BCLK		(0<<12)
	#define SUN6I_IISFAT1_SYNCLEN_32BCLK		(1<<12)
	#define SUN6I_IISFAT1_SYNCLEN_64BCLK		(2<<12)
	#define SUN6I_IISFAT1_SYNCLEN_128BCLK		(3<<12)
	#define SUN6I_IISFAT1_SYNCLEN_256BCLK		(4<<12)
	#define SUN6I_IISFAT1_SYNCOUTEN				(1<<11)
	#define SUN6I_IISFAT1_OUTMUTE 				(1<<10)
	#define SUN6I_IISFAT1_MLS		 			(1<<9)
	#define SUN6I_IISFAT1_SEXT		 			(1<<8)
	#define SUN6I_IISFAT1_SI_1ST				(0<<6)
	#define SUN6I_IISFAT1_SI_2ND			 	(1<<6)
	#define SUN6I_IISFAT1_SI_3RD			 	(2<<6)
	#define SUN6I_IISFAT1_SI_4TH			 	(3<<6)
	#define SUN6I_IISFAT1_SW			 		(1<<5)
	#define SUN6I_IISFAT1_SSYNC	 				(1<<4)
	#define SUN6I_IISFAT1_RXPDM_16PCM			(0<<2)
	#define SUN6I_IISFAT1_RXPDM_8PCM			(1<<2)
	#define SUN6I_IISFAT1_RXPDM_8ULAW			(2<<2)
	#define SUN6I_IISFAT1_RXPDM_8ALAW  			(3<<2)
	#define SUN6I_IISFAT1_TXPDM_16PCM			(0<<0)
	#define SUN6I_IISFAT1_TXPDM_8PCM			(1<<0)
	#define SUN6I_IISFAT1_TXPDM_8ULAW			(2<<0)
	#define SUN6I_IISFAT1_TXPDM_8ALAW  			(3<<0)
	
#define SUN6I_IISTXFIFO 						(0x0C)

#define SUN6I_IISRXFIFO 						(0x10)

#define SUN6I_IISFCTL  							(0x14)
	#define SUN6I_IISFCTL_FIFOSRC				(1<<31)
	#define SUN6I_IISFCTL_FTX					(1<<25)
	#define SUN6I_IISFCTL_FRX					(1<<24)
	#define SUN6I_IISFCTL_TXTL(v)				((v)<<12)
	#define SUN6I_IISFCTL_RXTL(v)  				((v)<<4)
	#define SUN6I_IISFCTL_TXIM_MOD0				(0<<2)
	#define SUN6I_IISFCTL_TXIM_MOD1				(1<<2)
	#define SUN6I_IISFCTL_RXOM_MOD0				(0<<0)
	#define SUN6I_IISFCTL_RXOM_MOD1				(1<<0)
	#define SUN6I_IISFCTL_RXOM_MOD2				(2<<0)
	#define SUN6I_IISFCTL_RXOM_MOD3				(3<<0)
	
#define SUN6I_IISFSTA   						(0x18)
	#define SUN6I_IISFSTA_TXE					(1<<28)
	#define SUN6I_IISFSTA_TXECNT(v)				((v)<<16)
	#define SUN6I_IISFSTA_RXA					(1<<8)
	#define SUN6I_IISFSTA_RXACNT(v)				((v)<<0)
	
#define SUN6I_IISINT    						(0x1C)
	#define SUN6I_IISINT_TXDRQEN				(1<<7)
	#define SUN6I_IISINT_TXUIEN					(1<<6)
	#define SUN6I_IISINT_TXOIEN					(1<<5)
	#define SUN6I_IISINT_TXEIEN					(1<<4)
	#define SUN6I_IISINT_RXDRQEN				(1<<3)
	#define SUN6I_IISINT_RXUIEN					(1<<2)
	#define SUN6I_IISINT_RXOIEN					(1<<1)
	#define SUN6I_IISINT_RXAIEN					(1<<0)
	
#define SUN6I_IISISTA   						(0x20)
	#define SUN6I_IISISTA_TXUISTA				(1<<6)
	#define SUN6I_IISISTA_TXOISTA				(1<<5)
	#define SUN6I_IISISTA_TXEISTA				(1<<4)
	#define SUN6I_IISISTA_RXOISTA				(1<<1)
	#define SUN6I_IISISTA_RXAISTA				(1<<0)
		
#define SUN6I_IISCLKD   						(0x24)
	#define SUN6I_IISCLKD_MCLKOEN				(1<<7)
	#define SUN6I_IISCLKD_BCLKDIV_2				(0<<4)
	#define SUN6I_IISCLKD_BCLKDIV_4				(1<<4)
	#define SUN6I_IISCLKD_BCLKDIV_6				(2<<4)
	#define SUN6I_IISCLKD_BCLKDIV_8				(3<<4)
	#define SUN6I_IISCLKD_BCLKDIV_12			(4<<4)
	#define SUN6I_IISCLKD_BCLKDIV_16			(5<<4)
	#define SUN6I_IISCLKD_BCLKDIV_32			(6<<4)
	#define SUN6I_IISCLKD_BCLKDIV_64			(7<<4)
	#define SUN6I_IISCLKD_MCLKDIV_1				(0<<0)
	#define SUN6I_IISCLKD_MCLKDIV_2				(1<<0)
	#define SUN6I_IISCLKD_MCLKDIV_4				(2<<0)
	#define SUN6I_IISCLKD_MCLKDIV_6				(3<<0)
	#define SUN6I_IISCLKD_MCLKDIV_8				(4<<0)
	#define SUN6I_IISCLKD_MCLKDIV_12			(5<<0)
	#define SUN6I_IISCLKD_MCLKDIV_16			(6<<0)
	#define SUN6I_IISCLKD_MCLKDIV_24			(7<<0)
	#define SUN6I_IISCLKD_MCLKDIV_32			(8<<0)
	#define SUN6I_IISCLKD_MCLKDIV_48			(9<<0)
	#define SUN6I_IISCLKD_MCLKDIV_64			(10<<0)
		
#define SUN6I_IISTXCNT  						(0x28)
                            					
#define SUN6I_IISRXCNT  						(0x2C)
                            					
#define SUN6I_TXCHSEL							(0x30)
	#define SUN6I_TXCHSEL_CHNUM(v)				(((v)-1)<<0)

#define SUN6I_TXCHMAP							(0x34)
	#define SUN6I_TXCHMAP_CH7(v)				(((v)-1)<<28)
	#define SUN6I_TXCHMAP_CH6(v)				(((v)-1)<<24)
	#define SUN6I_TXCHMAP_CH5(v)				(((v)-1)<<20)
	#define SUN6I_TXCHMAP_CH4(v)				(((v)-1)<<16)
	#define SUN6I_TXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUN6I_TXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUN6I_TXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUN6I_TXCHMAP_CH0(v)				(((v)-1)<<0)

#define SUN6I_RXCHSEL							(0x38)
	#define SUN6I_RXCHSEL_CHNUM(v)				(((v)-1)<<0)

#define SUN6I_RXCHMAP							(0x3C)
	#define SUN6I_RXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUN6I_RXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUN6I_RXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUN6I_RXCHMAP_CH0(v)				(((v)-1)<<0)	

#define SUN6I_IISCLKD_MCLK_MASK   0x0f
#define SUN6I_IISCLKD_MCLK_OFFS   0
#define SUN6I_IISCLKD_BCLK_MASK   0x070
#define SUN6I_IISCLKD_BCLK_OFFS   4
#define SUN6I_IISCLKD_MCLKEN_OFFS 7

#define SUN6I_I2S_DIV_MCLK	(0)
#define SUN6I_I2S_DIV_BCLK	(1)

struct sun6i_i2s_info {
	void __iomem   *regs;

	u32 slave;
	u32 mono;
	u32 samp_fs;
	u32 samp_res;
	u32 samp_format;
	u32 ws_size;
	u32 mclk_rate;
	u32 lrc_pol;
	u32 bclk_pol;
	u32 pcm_txtype;
	u32 pcm_rxtype;	
	u32 pcm_sw;
	u32 pcm_sync_period;
	u32 pcm_sync_type;
	u32 pcm_start_slot;
	u32 pcm_lsb_first;
	u32 pcm_ch_num;
};

extern struct sun6i_i2s_info sun6i_i2s;
#endif
