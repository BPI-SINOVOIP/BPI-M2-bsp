/*
 * sound\soc\sun6i\pcm\sun6i-pcm.h
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

#ifndef SUN6I_PCM_H_
#define SUN6I_PCM_H_

/*------------------------------------------------------------*/
/* REGISTER definition */

/* PCM REGISTER */
#define SUN6I_PCMBASE 							(0x01C22400)

#define SUN6I_PCMCTL 	  						(0x00)
	#define SUN6I_PCMCTL_SDO0EN					(1<<8) 
	#define SUN6I_PCMCTL_ASS					(1<<6)
	#define SUN6I_PCMCTL_MS						(1<<5)
	#define SUN6I_PCMCTL_PCM					(1<<4)
	#define SUN6I_PCMCTL_LOOP					(1<<3)
	#define SUN6I_PCMCTL_TXEN					(1<<2)
	#define SUN6I_PCMCTL_RXEN					(1<<1)
	#define SUN6I_PCMCTL_GEN					(1<<0)
	                                    		
#define SUN6I_PCMFAT0 							(0x04)
	#define SUN6I_PCMFAT0_LRCP					(1<<7)
	#define SUN6I_PCMFAT0_BCP					(1<<6)
	#define SUN6I_PCMFAT0_SR_RVD				(3<<4)
	#define SUN6I_PCMFAT0_SR_16BIT				(0<<4)
	#define	SUN6I_PCMFAT0_SR_20BIT				(1<<4)
	#define SUN6I_PCMFAT0_SR_24BIT				(2<<4)
	#define SUN6I_PCMFAT0_WSS_16BCLK			(0<<2)
	#define SUN6I_PCMFAT0_WSS_20BCLK			(1<<2)
	#define SUN6I_PCMFAT0_WSS_24BCLK			(2<<2)
	#define SUN6I_PCMFAT0_WSS_32BCLK			(3<<2)
	#define SUN6I_PCMFAT0_FMT_I2S				(0<<0)
	#define SUN6I_PCMFAT0_FMT_LFT				(1<<0)
	#define SUN6I_PCMFAT0_FMT_RGT				(2<<0)
	#define SUN6I_PCMFAT0_FMT_RVD				(3<<0)
	
#define SUN6I_PCMFAT1							(0x08)
	#define SUN6I_PCMFAT1_SYNCLEN_16BCLK		(0<<12)
	#define SUN6I_PCMFAT1_SYNCLEN_32BCLK		(1<<12)
	#define SUN6I_PCMFAT1_SYNCLEN_64BCLK		(2<<12)
	#define SUN6I_PCMFAT1_SYNCLEN_128BCLK		(3<<12)
	#define SUN6I_PCMFAT1_SYNCLEN_256BCLK		(4<<12)
	#define SUN6I_PCMFAT1_SYNCOUTEN				(1<<11)
	#define SUN6I_PCMFAT1_OUTMUTE 				(1<<10)
	#define SUN6I_PCMFAT1_MLS		 			(1<<9)
	#define SUN6I_PCMFAT1_SEXT		 			(1<<8)
	#define SUN6I_PCMFAT1_SI_1ST				(0<<6)
	#define SUN6I_PCMFAT1_SI_2ND			 	(1<<6)
	#define SUN6I_PCMFAT1_SI_3RD			 	(2<<6)
	#define SUN6I_PCMFAT1_SI_4TH			 	(3<<6)
	#define SUN6I_PCMFAT1_SW			 		(1<<5)
	#define SUN6I_PCMFAT1_SSYNC	 				(1<<4)
	#define SUN6I_PCMFAT1_RXPDM_16PCM			(0<<2)
	#define SUN6I_PCMFAT1_RXPDM_8PCM			(1<<2)
	#define SUN6I_PCMFAT1_RXPDM_8ULAW			(2<<2)
	#define SUN6I_PCMFAT1_RXPDM_8ALAW  			(3<<2)
	#define SUN6I_PCMFAT1_TXPDM_16PCM			(0<<0)
	#define SUN6I_PCMFAT1_TXPDM_8PCM			(1<<0)
	#define SUN6I_PCMFAT1_TXPDM_8ULAW			(2<<0)
	#define SUN6I_PCMFAT1_TXPDM_8ALAW  			(3<<0)
	
#define SUN6I_PCMTXFIFO 						(0x0C)

#define SUN6I_PCMRXFIFO 						(0x10)

#define SUN6I_PCMFCTL  							(0x14)
	#define SUN6I_PCMFCTL_FIFOSRC				(1<<31)
	#define SUN6I_PCMFCTL_FTX					(1<<25)
	#define SUN6I_PCMFCTL_FRX					(1<<24)
	#define SUN6I_PCMFCTL_TXTL(v)				((v)<<12)
	#define SUN6I_PCMFCTL_RXTL(v)  				((v)<<4)
	#define SUN6I_PCMFCTL_TXIM_MOD0				(0<<2)
	#define SUN6I_PCMFCTL_TXIM_MOD1				(1<<2)
	#define SUN6I_PCMFCTL_RXOM_MOD0				(0<<0)
	#define SUN6I_PCMFCTL_RXOM_MOD1				(1<<0)
	#define SUN6I_PCMFCTL_RXOM_MOD2				(2<<0)
	#define SUN6I_PCMFCTL_RXOM_MOD3				(3<<0)
	                                    		
#define SUN6I_PCMFSTA   						(0x18)
	#define SUN6I_PCMFSTA_TXE					(1<<28)
	#define SUN6I_PCMFSTA_TXECNT(v)				((v)<<16)
	#define SUN6I_PCMFSTA_RXA					(1<<8)
	#define SUN6I_PCMFSTA_RXACNT(v)				((v)<<0)
	
#define SUN6I_PCMINT    						(0x1C)
	#define SUN6I_PCMINT_TXDRQEN				(1<<7)
	#define SUN6I_PCMINT_TXUIEN					(1<<6)
	#define SUN6I_PCMINT_TXOIEN					(1<<5)
	#define SUN6I_PCMINT_TXEIEN					(1<<4)
	#define SUN6I_PCMINT_RXDRQEN				(1<<3)
	#define SUN6I_PCMINT_RXUIEN					(1<<2)
	#define SUN6I_PCMINT_RXOIEN					(1<<1)
	#define SUN6I_PCMINT_RXAIEN					(1<<0)
	                                        	
#define SUN6I_PCMISTA   						(0x20)
	#define SUN6I_PCMISTA_TXUISTA				(1<<6)
	#define SUN6I_PCMISTA_TXOISTA				(1<<5)
	#define SUN6I_PCMISTA_TXEISTA				(1<<4)
	#define SUN6I_PCMISTA_RXOISTA				(1<<1)
	#define SUN6I_PCMISTA_RXAISTA				(1<<0)
		                                    	
#define SUN6I_PCMCLKD   						(0x24)
	#define SUN6I_PCMCLKD_MCLKOEN				(1<<7)
	#define SUN6I_PCMCLKD_BCLKDIV_2				(0<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_4				(1<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_6				(2<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_8				(3<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_12			(4<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_16			(5<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_32			(6<<4)
	#define SUN6I_PCMCLKD_BCLKDIV_64			(7<<4)
	#define SUN6I_PCMCLKD_MCLKDIV_1				(0<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_2				(1<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_4				(2<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_6				(3<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_8				(4<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_12			(5<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_16			(6<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_24			(7<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_32			(8<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_48			(9<<0)
	#define SUN6I_PCMCLKD_MCLKDIV_64			(10<<0)
		
#define SUN6I_PCMTXCNT  						(0x28)

#define SUN6I_PCMRXCNT  						(0x2C)

#define SUN6I_PCMTXCHSEL						(0x30)
	#define SUN6I_PCMTXCHSEL_CHNUM(v)			(((v)-1)<<0)

#define SUN6I_PCMTXCHMAP						(0x34)
	#define SUN6I_PCMTXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUN6I_PCMTXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUN6I_PCMTXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUN6I_PCMTXCHMAP_CH0(v)				(((v)-1)<<0)

#define SUN6I_PCMRXCHSEL						(0x38)
	#define SUN6I_PCMRXCHSEL_CHNUM(v)			(((v)-1)<<0)

#define SUN6I_PCMRXCHMAP						(0x3C)
	#define SUN6I_PCMRXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUN6I_PCMRXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUN6I_PCMRXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUN6I_PCMRXCHMAP_CH0(v)				(((v)-1)<<0)	


/* Clock dividers */
#define SUN6I_PCMDIV_MCLK	0
#define SUN6I_PCMDIV_BCLK	1

#define SUN6I_PCMCLKD_MCLK_MASK   0x0f
#define SUN6I_PCMCLKD_MCLK_OFFS   0
#define SUN6I_PCMCLKD_BCLK_MASK   0x070
#define SUN6I_PCMCLKD_BCLK_OFFS   4
#define SUN6I_PCMCLKD_MCLKEN_OFFS 7

struct sun6i_pcm_info {
	void __iomem   *regs;    /* PCM BASE */

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

extern struct sun6i_pcm_info sun6i_pcm;
#endif
