/*
 * sound\soc\sun6i\sun6i-codec.c
 * (C) Copyright 2010-2016
 * reuuimllatech Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef CONFIG_PM
#define CONFIG_PM
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <mach/gpio.h>
#include <mach/sys_config.h>
#include <mach/system.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#include <asm/mach-types.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/scenelock.h>
#include <linux/extended_standby.h>
#include <mach/clock.h>
#include "sun6i-codec.h"

static struct regulator* hp_ldo = NULL;
static char *hp_ldo_str = NULL;
static unsigned int play_dmasrc 	= 0;
static unsigned int capture_dmadst 	= 0;

/*for pa gpio ctrl*/
static int req_status;
static script_item_u item;

/*for phone call flag*/
static bool codec_lineinin_en 			= false;
static bool codec_fm_headset_en 		= false;
static bool codec_fm_speaker_en 		= false;
static bool codec_lineincap_en 			= false;
static bool codec_speakerout_en 		= false;
static bool codec_adcphonein_en 		= false;
static bool codec_dacphoneout_en 		= false;
static bool codec_headphoneout_en 		= false;
static bool codec_earpieceout_en 		= false;
static bool codec_phonein_en 			= false;
static bool codec_phoneout_en 			= false;
static bool codec_speaker_en 			= false;
static bool codec_voice_record_en 		= false;
static bool codec_mainmic_en 			= false;
static bool codec_headsetmic_en 		= false;
static bool codec_noise_reduced_adcin_en= false;
static bool codec_phonein_left_en		= false;
static bool codec_speakerout_lntor_en	= false;
static bool codec_headphoneout_lntor_en = false;
static bool codec_dacphoneout_reduced_en= false;
static int codec_speaker_headset_earpiece_en= 0;

struct clk *codec_apbclk,*codec_pll2clk,*codec_moduleclk;

struct sun6i_codec {
	long 				samplerate;
	struct snd_card 	*card;
	struct snd_pcm 		*pcm;
	/*struct timer_list 	timer;
	struct work_struct 	work;*/
};

/*------------- Structure/enum declaration ------------------- */
typedef struct codec_board_info {
	struct device	*dev;	     		/* parent device */
	struct resource	*codec_base_res;   /* resources found */
	struct resource	*codec_base_req;   /* resources found */

	spinlock_t	lock;
} codec_board_info_t;

static struct sun6i_pcm_dma_params sun6i_codec_pcm_stereo_play = {
	.name		= "audio_play",
	.dma_addr	= CODEC_BASSADDRESS + SUN6I_DAC_TXDATA,//send data address	
};

static struct sun6i_pcm_dma_params sun6i_codec_pcm_stereo_capture = {
	.name   	= "audio_capture",
	.dma_addr	= CODEC_BASSADDRESS + SUN6I_ADC_RXDATA,//accept data address	
};

struct sun6i_playback_runtime_data {
	spinlock_t 	 lock;
	int 		 state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t   dma_start;
	dma_addr_t   dma_pos;
	dma_addr_t	 dma_end;
	dm_hdl_t	 dma_hdl;
	bool		 play_dma_flag;
	struct dma_cb_t play_done_cb;
	struct dma_cb_t play_hdone_cb;
	struct sun6i_pcm_dma_params	*params;
};

struct sun6i_capture_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t   dma_start;
	dma_addr_t   dma_pos;
	dma_addr_t	 dma_end;
	dm_hdl_t	 dma_hdl;
	bool		 capture_dma_flag;
	struct dma_cb_t capture_done_cb;
	struct dma_cb_t capture_hdone_cb;
	struct sun6i_pcm_dma_params	*params;
};

static struct snd_pcm_hardware sun6i_pcm_playback_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 |SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= 1024,
	.period_bytes_max	= 1024*32,
	.periods_min		= 2,
	.periods_max		= 8,
	.fifo_size	     	= 32,
};

static struct snd_pcm_hardware sun6i_pcm_capture_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= 1024,
	.period_bytes_max	= 1024*32,
	.periods_min		= 2,
	.periods_max		= 8,
	.fifo_size	     	= 32,
};

static unsigned int rates[] = {
	8000,11025,12000,16000,
	22050,24000,24000,32000,
	44100,48000,96000,192000
};

static struct snd_pcm_hw_constraint_list hw_constraints_rates = {
	.count	= ARRAY_SIZE(rates),
	.list	= rates,
	.mask	= 0,
};

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;
		
	old	=	codec_rdreg(reg);
	new	=	(old & ~mask) | value;
	codec_wrreg(reg,new);

	return 0;
}

/**
*	snd_codec_info_volsw	-	single	mixer	info	callback
*	@kcontrol:	mixer control
*	@uinfo:	control	element	information
*	Callback to provide information about a single mixer control
*
*	Returns 0 for success
*/
int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	struct	codec_mixer_control *mc	= (struct codec_mixer_control*)kcontrol->private_value;
	int	max	=	mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;

	if (max	== 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;//the info of type
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = shift ==	rshift	?	1:	2;	//the info of elem count
	uinfo->value.integer.min = 0;				//the info of min value
	uinfo->value.integer.max = max;				//the info of max value
	return	0;
}

/**
*	snd_codec_get_volsw	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*	return 0 for success.
*/
int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;

	ucontrol->value.integer.value[0] =	
		(codec_rdreg(reg)>>	shift) & mask;
	if (shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if(shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}
	
		return 0;
}

/**
*	snd_codec_put_volsw	-	single	mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to put the value of a single mixer control
*
* return 0 for success.
*/
int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1<<fls(max))-1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;
	
	val = (ucontrol->value.integer.value[0] & mask);
	if(invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if(shift != rshift){
		val2	= (ucontrol->value.integer.value[1] & mask);
		if(invert)
			val2	=	max	- val2;
		val_mask |= mask <<rshift;
		val |= val2 <<rshift;
	}
	
	return codec_wrreg_bits(reg,val_mask,val);
}

int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

int codec_rd_control(u32 reg, u32 bit, u32 *val)
{
	return 0;
}

/*
*	enable the codec function which should be enable during system init.
*/
static  void codec_init(void)
{
	int headphone_direct_used = 0;
	script_item_u val;
	script_item_value_type_e  type;
	enum sw_ic_ver  codec_chip_ver;

	codec_chip_ver = sw_get_ic_ver();
	type = script_get_item("audio_para", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_direct_used type err!\n");
    }
	headphone_direct_used = val.val;
	if (headphone_direct_used && (codec_chip_ver != MAGIC_VER_A)) {
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x3);
		codec_wr_control(SUN6I_PA_CTRL, 0x1, HPCOM_PRO, 0x1);
	} else {
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUN6I_PA_CTRL, 0x1, HPCOM_PRO, 0x0);
	}

	/*mute l_pa and r_pa.*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);

	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	codec_wr_control(SUN6I_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
	
	/*write 1 to flush tx fifo*/
	codec_wr_control(SUN6I_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
	/*write 1 to flush rx fifo*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);

	codec_wr_control(SUN6I_DAC_FIFOC, 0x1, FIR_VERSION, 0x1);
}

/*
*	the system voice come out from speaker
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_play_open(void)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio_para", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("[audiocodec] pa_double_used type err!\n");
	}

	pa_double_used = val.val;
	if (!pa_double_used) {
		type = script_get_item("audio_para", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_single_vol type err!\n");
		}
		pa_vol = val.val;
	} else {
		type = script_get_item("audio_para", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_double_vol type err!\n");
		}
		pa_vol = val.val;
	}

	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);
	if (!pa_double_used) {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	}
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);

	usleep_range(2000, 3000);
	item.gpio.data = 1;
	/*config gpio info of audio_pa_ctrl open*/
	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}
	msleep(62);

	return 0;
}

/*
*	the system voice come out from headphone
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_headphone_play_open(void)
{
	int i = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	u32 reg_val = 0;
	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;
	
	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);

	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);
	reg_val = codec_rdreg(SUN6I_DAC_ACTL);
	reg_val &= 0x3f;
	if (!reg_val) {
		for (i=0; i < headphone_vol; i++) {
			/*set HPVOL volume*/
			codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
			reg_val = codec_rdreg(SUN6I_DAC_ACTL);
			reg_val &= 0x3f;
			if ((i%2==0))
				usleep_range(1000,2000);
		}
	}
	return 0;
}

static int codec_earpiece_play_open(void)
{
	int earpiece_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "earpiece_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	printk("[audiocodec] headphone_vol type err!\n");
    }
	earpiece_vol = val.val;
	
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);

	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	/*send last sample when dac fifo under run*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

	/*select the analog mixer input source*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	/*unmute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

	/*select HPL inverting output*/
	codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x1);
	/*set HPVOL volume*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, earpiece_vol);

	return 0;
}

/*
*	the system voice come out from headphone and speaker
*	while the phone call in, the phone use the headset, you can hear the voice from speaker and headset.
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_and_headset_play_open(void)
{
	int pa_vol = 0,	headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio_para", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("[audiocodec] pa_double_used type err!\n");
	}

	pa_double_used = val.val;
	if (!pa_double_used) {
		type = script_get_item("audio_para", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_single_vol type err!\n");
		}
		pa_vol = val.val;
	} else {
		type = script_get_item("audio_para", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_double_vol type err!\n");
		}
		pa_vol = val.val;
	}

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	printk("[audiocodec] headphone_vol type err!\n");
    	}
	headphone_vol = val.val;
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);
	if (!pa_double_used) {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	}
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);
	
	codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
	/*set HPVOL volume*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

	usleep_range(2000, 3000);
	item.gpio.data = 1;
	/*config gpio info of audio_pa_ctrl open*/
	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}
	msleep(62);
	/*unmute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

	return 0;
}

/*
*	use for phone record from main mic + phone in.
*	mic1 is use as main mic.
*/
static int codec_voice_main_mic_capture_open(void)
{
	/*enable mic1 pa*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*enable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);

	/*enable Right MIC1 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);

	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);

	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	msleep(200);
	return 0;
}

/*
*	use for phone record from sub mic + phone in.
*	mic2 is use as sub mic.
* 	mic2 is the headset mic.
*/
static int codec_voice_headset_mic_capture_open(void)
{
	/*enable Right MIC2 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC2 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);

	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	msleep(200);

	return 0;
}

/*
*	use for the line_in record
*/
static int codec_voice_linein_capture_open(void)
{
	/*enable LINEINR ADC*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTELINEINR, 0x1);
	/*enable LINEINL ADC*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTELINEINL, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	msleep(200);

	return 0;
}

/*
*	use for the phone noise reduced while in phone model.
*	use the mic1 and mic3 to reduecd the noise from the phone in
*	mic3 use the same channel of mic2.
*/
static int codec_noise_reduced_capture_open(void)
{
	/*enable mic1 pa*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*enable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x1);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
	/*select mic3 source:0:mic3,1:mic2 */
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2_SEL, 0x0);

	/*enable Right MIC2 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	msleep(200);

	return 0;
}

/*
*	use for the base system record(for pad record).
*/
static int codec_capture_open(void)
{
	int cap_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "cap_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("[audiocodec] cap_vol type err!\n");
	}
	cap_vol = val.val;

	/*enable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x1);

	if (codec_headsetmic_en){
		type = script_get_item("audio_para", "headset_mic_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		    printk("[audiocodec] codec_set_headsetmic type err!\n");
	        }

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*select mic3 source:0:mic3,1:mic2 */
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2_SEL, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC2BOOST,val.val);//36db
		/*enable Right MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
		/*enable Left MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
	} else if (codec_mainmic_en){
		type = script_get_item("audio_para", "main_mic_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		        printk("[audiocodec] codec_set_mainmic type err!\n");
	        }

		/*enable mic1 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC1BOOST,val.val);//36db
		/*enable Right MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	} else {
		/*enable mic1 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC1BOOST,cap_vol);//36db
		/*enable Right MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	} 

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);

	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	
	msleep(200);
	return 0;
}

static int codec_play_start(void)
{
	int i = 0;
	u32 reg_val;
	int headphone_vol = 0;
	/*enable dac drq*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
	/*DAC FIFO Flush,Write '1' to flush TX FIFO, self clear to '0'*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);

	if (codec_speaker_en || (codec_speaker_headset_earpiece_en==1)||(codec_speaker_headset_earpiece_en==2)) {
		;
	} else if ( (codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en || codec_dacphoneout_en) ){
		;
	} else {
		/*set the default output is HPOUTL/R for pad headphone*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);
	
		reg_val = codec_rdreg(SUN6I_DAC_ACTL);
		reg_val &= 0x3f;
		if (!reg_val) {
			for(i=0; i < headphone_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
				reg_val = codec_rdreg(SUN6I_DAC_ACTL);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
	}
	return 0;
}

static int codec_play_stop(void)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	/*disable dac drq*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);

	if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_dacphoneout_en || codec_lineinin_en || codec_voice_record_en ) ) {
		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl open*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return 0;
}

static int codec_capture_stop(void)
{
	/*disable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x0);
	/*disable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
	if (!(codec_voice_record_en||codec_mainmic_en||codec_headsetmic_en||codec_lineincap_en)) {
		/*disable mic1 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
		/*disable mic2/mic3 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);

		/*disable Right MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
		/*disable Left MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);

		/*disable Right MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
		/*disable Left MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);

		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);

		/*disable LINEINR ADC*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTELINEINR, 0x0);
		/*disable LINEINL ADC*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTELINEINL, 0x0);
	}
	/*disable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, ADCREN, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, ADCLEN, 0x0);

	return 0;
}

static int codec_dev_free(struct snd_device *device)
{
	return 0;
};

/*
*	codec_lineinin_en == 1, open the linein in.
*	codec_lineinin_en == 0, close the linein in.
*/
static int codec_set_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineinin_en = ucontrol->value.integer.value[0];

	if (codec_lineinin_en) {
		/*select LINEINR*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x4);
		/*select LINEINL*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x4);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);
	} else {
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}
	return 0;
}

static int codec_get_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineinin_en;
	return 0;
}

/*
*	use for fm.
*	voice come out from speaker.
*/
static int codec_set_fm_speaker(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;

	codec_fm_speaker_en = ucontrol->value.integer.value[0];

	if (codec_fm_speaker_en) {
		type = script_get_item("audio_para", "pa_double_used", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_double_used type err!\n");
		}

		pa_double_used = val.val;
		if (!pa_double_used) {
			type = script_get_item("audio_para", "pa_single_vol", &val);
			if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
				printk("[audiocodec] pa_single_vol type err!\n");
			}
			pa_vol = val.val;
		} else {
			type = script_get_item("audio_para", "pa_double_vol", &val);
			if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
				printk("[audiocodec] pa_double_vol type err!\n");
			}
			pa_vol = val.val;
		}
		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);

		/*enable dac digital*/
		codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x0);
		/*enable dac_l and dac_r*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);
		if (!pa_double_used) {
			codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
			codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);
		} else {
			codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
			codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		}

		/*select LINEINR*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x4);
		/*select LINEINL*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x4);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);

		usleep_range(2000, 3000);
		item.gpio.data = 1;
		/*config gpio info of audio_pa_ctrl open*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		msleep(62);
	} else {
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}
	return 0;
}

static int codec_get_fm_speaker(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_fm_headset_en;
	return 0;
}

/*
*	use for fm.
*	voice come out from headset.
*/
static int codec_set_fm_headset(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	codec_fm_headset_en = ucontrol->value.integer.value[0];

	if (codec_fm_headset_en) {
		type = script_get_item("audio_para", "headphone_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] headphone_vol type err!\n");
		}
		headphone_vol = val.val;
		/*enable dac digital*/
		codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x0);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x4);
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x4);

		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, headphone_vol);
	} else {
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}
	return 0;
}

static int codec_get_fm_headset(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_fm_headset_en;
	return 0;
}

/*
*	use for linein record
*/
static int codec_set_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineincap_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineincap_en;
	return 0;
}

/*
*	codec_speakerout_lntor_en == 1, open the speaker.
*	codec_speakerout_lntor_en == 0, close the speaker.
*	if the phone in voice just use left channel for phone call(right channel used for noise reduced), 
*	the speaker's right channel's voice must use the left channel to transfer phone call voice.
*	lntor:left negative to right
*/
static int codec_set_speakerout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio_para", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] pa_double_used type err!\n");
    }
    pa_double_used = val.val;
    if (!pa_double_used) {
		type = script_get_item("audio_para", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        printk("[audiocodec] pa_single_vol type err!\n");
	    }
		pa_vol = val.val;
	} else {
		type = script_get_item("audio_para", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        printk("[audiocodec] pa_double_vol type err!\n");
	    }
		pa_vol = val.val;
	}

	codec_speakerout_lntor_en = ucontrol->value.integer.value[0];

	if (codec_speakerout_lntor_en) {
		/*close headphone and earpiece out routeway*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);

		usleep_range(2000, 3000);
		item.gpio.data = 1;
		/*config gpio info of audio_pa_ctrl open*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		msleep(62);

		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);

		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}

	return 0;
}
/*
*	lntor:left negative to right
*/
static int codec_get_speakerout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speakerout_lntor_en;
	return 0;
}

/*
*	codec_speakerout_en == 1, open the speaker.
*	codec_speakerout_en == 0, close the speaker.
*/
static int codec_set_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio_para", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] pa_double_used type err!\n");
    }
    pa_double_used = val.val;
    if (!pa_double_used) {
		type = script_get_item("audio_para", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        printk("[audiocodec] pa_single_vol type err!\n");
	    }
		pa_vol = val.val;
	} else {
		type = script_get_item("audio_para", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        printk("[audiocodec] pa_double_vol type err!\n");
	    }
		pa_vol = val.val;
	}

	codec_speakerout_en = ucontrol->value.integer.value[0];

	if (codec_speakerout_en) {
		/*close headphone and earpiece out routeway*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);

		usleep_range(2000, 3000);
		item.gpio.data = 1;
		/*config gpio info of audio_pa_ctrl open*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		msleep(62);

		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);

		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
	}

	return 0;
}

static int codec_get_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speakerout_en;
	return 0;
}

/*
*	codec_headphoneout_lntor_en == 1, open the headphone.
*	codec_headphoneout_lntor_en == 0, close the headphone.
* 	lntor:left negative to right
*/
static int codec_set_headphoneout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_headphoneout_lntor_en = ucontrol->value.integer.value[0];

	if (codec_headphoneout_lntor_en) {
		/*close speaker earpiece out routeway*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		/*select HPL inverting output*/
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		/*Left channel to right channel no mute*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LTRNMUTE, 0x1);
		/*select the analog mixer input source*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		/*select the default dac input source*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}
/*
* 	lntor:left negative to right
*/
static int codec_get_headphoneout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headphoneout_lntor_en;
	return 0;
}

/*
*	codec_headphoneout_en == 1, open the headphone.
*	codec_headphoneout_en == 0, close the headphone.
*/
static int codec_set_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_headphoneout_en = ucontrol->value.integer.value[0];

	if (codec_headphoneout_en) {
		/*close speaker earpiece out routeway*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		/*select HPL inverting output*/
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

		/*select the analog mixer input source*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		/*select the default dac input source*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}

static int codec_get_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headphoneout_en;
	return 0;
}

/*
*	codec_earpieceout_en == 1, open the earpiece.
*	codec_earpieceout_en == 0, close the earpiece.
*/
static int codec_set_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int earpiece_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "earpiece_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_vol type err!\n");
    }
	earpiece_vol = val.val;

	codec_earpieceout_en = ucontrol->value.integer.value[0];

	if (codec_earpieceout_en) {
		/*close speaker out routeway*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		item.gpio.data = 0;
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		/*close headphone routeway*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0x0);

		/*open earpiece out routeway*/
		/*unmute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		/*select the analog mixer input source*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x1);
		/*select HPL inverting output*/
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x1);

		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1); 
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, earpiece_vol);

		codec_speakerout_en = 0;
		codec_headphoneout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}

static int codec_get_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_earpieceout_en;
	return 0;
}

/*
*	codec_phonein_left_en == 1, the phone in left channel is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_phonein_left_en == 0. the phone in left channel is close.
*/
static int codec_set_phonein_left(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phonein_left_en = ucontrol->value.integer.value[0];

	if (codec_phonein_left_en) {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	} else {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
	}

	return 0;
}

static int codec_get_phonein_left(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonein_left_en;
	return 0;
}

/*
*	codec_phonein_en == 1, the phone in is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_phonein_en == 0. the phone in is close.
*/
static int codec_set_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phonein_en = ucontrol->value.integer.value[0];

	if (codec_phonein_en) {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x1);
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXMUTEPHONEPN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);
	} else {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x0);
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXMUTEPHONEPN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}

	return 0;
}

static int codec_get_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonein_en;
	return 0;
}

/*
*	codec_phoneout_en == 1, the phone out is open. receiver can hear the voice which you say.
*	codec_phoneout_en == 0,	the phone out is close.
*/
static int codec_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phoneout_en = ucontrol->value.integer.value[0];

	if (codec_phoneout_en) {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUT_EN, 0x1);
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUT_EN, 0x0);
	}

	return 0;
}

static int codec_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phoneout_en;
	return 0;
}

static int codec_dacphoneout_reduced_open(void)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x1);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x1);
	return 0;
}

/*
*	codec_dacphoneout_reduced_en == 1, the dac phone out is open. the receiver can hear the voice from system.
*	codec_dacphoneout_reduced_en == 0,	the dac phone out is close.
*/
static int codec_set_dacphoneout_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_dacphoneout_reduced_en = ucontrol->value.integer.value[0];

	if (codec_dacphoneout_reduced_en) {
		ret = codec_dacphoneout_reduced_open();
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return ret;
}

static int codec_get_dacphoneout_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_dacphoneout_reduced_en;
	return 0;
}

static int codec_dacphoneout_open(void)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXMUTEDACR, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXMUTEDACL, 0x1);

	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);
	
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x1);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x1);
	return 0;
}

/*
*	codec_dacphoneout_en == 1, the dac phone out is open. the receiver can hear the voice from system.
*	codec_dacphoneout_en == 0,	the dac phone out is close.
*/
static int codec_set_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_dacphoneout_en = ucontrol->value.integer.value[0];

	if (codec_dacphoneout_en) {
		ret = codec_dacphoneout_open();
	} else {
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return ret;
}

static int codec_get_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_dacphoneout_en;
	return 0;
}

static int codec_adcphonein_open(void)
{
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	return 0;
}

/*
*	codec_adcphonein_en == 1, the adc phone in is open. you can record the phonein from adc.
*	codec_adcphonein_en == 0,	the adc phone in is close.
*/
static int codec_set_adcphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_adcphonein_en = ucontrol->value.integer.value[0];

	if (codec_adcphonein_en) {
		ret = codec_adcphonein_open();
	} else {
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);
	}

	return ret;
}

static int codec_get_adcphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_adcphonein_en;
	return 0;
}

/*
*	codec_mainmic_en == 1, open mic1.
*	codec_mainmic_en == 0, close mic1.
*/
static int codec_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "main_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        printk("[audiocodec] codec_set_mainmic type err!\n");
        }

	codec_mainmic_en = ucontrol->value.integer.value[0];

	if (codec_mainmic_en) {
		/*close headset mic(mic2) routeway*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
		
		/*open main mic(mic1) routeway*/
		/*enable mic1 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC1BOOST,val.val);
		/*enable Master microphone bias*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS0, 0x1);

		/*enable Right MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);

		/*set the headset mic flag false*/
		codec_headsetmic_en = 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
	}

	return 0;
}

static int codec_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_mainmic_en;
	return 0;
}

/*
*	codec_voice_record_en == 1, set status.
*	codec_voice_record_en == 0, set status.
*/
static int codec_set_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_voice_record_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_voice_record_en;
	return 0;
}

/*
*	codec_noise_reduced_adcin_en == 1, set status.
*	codec_noise_reduced_adcin_en == 0, set status.
*/
static int codec_set_noise_adcin_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_noise_reduced_adcin_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_noise_adcin_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_noise_reduced_adcin_en;
	return 0;
}

/*
*	codec_headsetmic_en == 1, open mic2.
*	codec_headsetmic_en == 0, close mic2.
*/
static int codec_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headset_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    printk("[audiocodec] codec_set_headsetmic type err!\n");
    }

	codec_headsetmic_en = ucontrol->value.integer.value[0];

	if (codec_headsetmic_en) {
		/*close main mic(mic1) routeway*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
		
		/*open headset mic(mic2) routeway*/
		/*enable mic2 pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC2BOOST,val.val);

		/*select mic2 source */
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2_SEL, 0x1);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS1, 0x1);

		/*enable Right MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
		/*enable Left MIC2 Boost stage*/
		codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);

		/*set the main mic flag false*/
		codec_mainmic_en	= 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
	}

	return 0;
}

static int codec_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headsetmic_en;
	return 0;
}

/*
*	close all phone routeway
*/
static int codec_set_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	/*close adc phonein routeway*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);

	/*close dac phoneout routeway*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);

	/*close headset mic(mic2)*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
	
	/*close main mic(mic1)*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	
	/*close earpiece and headphone routeway*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
	codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0x0);

	/*close speaker routeway*/	
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	item.gpio.data = 0;
	/*config gpio info of audio_pa_ctrl close*/
	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}

	/*close analog phone in routeway*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x0);
	
	/*disable phone out*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, PHONEOUT_EN, 0x0);
	
	/*set all routeway flag false*/
	codec_adcphonein_en 	= 0;
	codec_dacphoneout_en	= 0;
	codec_headsetmic_en		= 0;
	codec_mainmic_en		= 0;

	codec_speakerout_en 	= 0;
	codec_earpieceout_en 	= 0;
	codec_headphoneout_en 	= 0;
	codec_phoneout_en		= 0;
	codec_phonein_en		= 0;
	return 0;
}

static int codec_get_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

/*
*	codec_speaker_en == 1, speaker is open, headphone is close.
*	codec_speaker_en == 0, speaker is closed, headphone is open.
*	this function just used for the system voice(such as music and moive voice and so on),
*	no the phone call.
*/
static int codec_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	codec_speaker_en = ucontrol->value.integer.value[0];

	if (codec_speaker_en) {
		ret = codec_pa_play_open();
	} else {
		item.gpio.data = 0;
		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		/*unmute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);

		type = script_get_item("audio_para", "headphone_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] headphone_vol type err!\n");
		}
		headphone_vol = val.val;
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, headphone_vol);
	}

	return 0;
}

static int codec_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_en;
	return 0;
}

/*
*	codec_speaker_headset_earpiece_en == 3, earpiece is open,speaker and headphone is close.
*	codec_speaker_headset_earpiece_en == 2, speaker is open, headphone is open.
*	codec_speaker_headset_earpiece_en == 1, speaker is open, headphone is close.
*	codec_speaker_headset_earpiece_en == 0, speaker is closed, headphone is open.
*	this function just used for the system voice(such as music and moive voice and so on),
*	no the phone call.
*/
static int codec_set_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0,i=0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	codec_speaker_headset_earpiece_en = ucontrol->value.integer.value[0];

	if (codec_speaker_headset_earpiece_en == 1) {
		ret = codec_pa_play_open();
	} else if (codec_speaker_headset_earpiece_en == 0) {
		item.gpio.data = 0;
		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}

		codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RMIXEN, 0x1);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

		type = script_get_item("audio_para", "headphone_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] headphone_vol type err!\n");
		}
		headphone_vol = val.val;
		reg_val = codec_rdreg(SUN6I_DAC_ACTL);
		reg_val &= 0x3f;
		if (!reg_val) {
			for (i=0; i < headphone_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
				reg_val = codec_rdreg(SUN6I_DAC_ACTL);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
	} else if (codec_speaker_headset_earpiece_en == 2) {
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_pa_and_headset_play_open();
	} else if (codec_speaker_headset_earpiece_en == 3) {
		item.gpio.data = 0;
		codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUN6I_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPIS, 0x0);
		/*config gpio info of audio_pa_ctrl close*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		codec_earpiece_play_open();
	}
	return 0;
}

static int codec_get_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_headset_earpiece_en;
	return 0;
}

static const char *spk_headset_earpiece_function[] = {"headset", "spk", "spk_headset", "earpiece"};
static const struct soc_enum spk_headset_earpiece_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_headset_earpiece_function), spk_headset_earpiece_function),
};

/*
* 	.info = snd_codec_info_volsw, .get = snd_codec_get_volsw,\.put = snd_codec_put_volsw,
*/
static const struct snd_kcontrol_new codec_snd_controls[] = {
	/*SUN6I_DAC_ACTL = 0x20,PAVOL*/
	CODEC_SINGLE("Master Playback Volume", SUN6I_DAC_ACTL, 0, 0x3f, 0),

	/*SUN6I_PA_CTRL = 0x24*/
	CODEC_SINGLE("MIC1_G boost stage output mixer control", 	SUN6I_PA_CTRL, 15, 0x7, 0),
	CODEC_SINGLE("MIC2_G boost stage output mixer control", 	SUN6I_PA_CTRL, 12, 0x7, 0),
	CODEC_SINGLE("LINEIN_G boost stage output mixer control", 	SUN6I_PA_CTRL, 9, 0x7, 0),
	CODEC_SINGLE("PHONE_G boost stage output mixer control", 	SUN6I_PA_CTRL, 6, 0x7, 0),
	CODEC_SINGLE("PHONE_PG boost stage output mixer control", 	SUN6I_PA_CTRL, 3, 0x7, 0),
	CODEC_SINGLE("PHONE_NG boost stage output mixer control", 	SUN6I_PA_CTRL, 0, 0x7, 0),

	/*SUN6I_MIC_CTRL = 0x28*/
	CODEC_SINGLE("MIC1 boost AMP gain control", SUN6I_MIC_CTRL,25,0x7,0),
	CODEC_SINGLE("MIC2 boost AMP gain control", SUN6I_MIC_CTRL,21,0x7,0),
	CODEC_SINGLE("Lineout volume control", 		SUN6I_MIC_CTRL,11,0x1f,0),
	CODEC_SINGLE("PHONEP-PHONEN pre-amp gain control", SUN6I_MIC_CTRL,8,0x7,0),
	CODEC_SINGLE("Phoneout gain control", 		SUN6I_MIC_CTRL,5,0x7,0),

	/*SUN6I_ADC_ACTL = 0x2c*/
	CODEC_SINGLE("ADC input gain ctrl", 		SUN6I_ADC_ACTL,27,0x7,0),

	SOC_SINGLE_BOOL_EXT("Audio Spk Switch", 	0, codec_get_spk, 		codec_set_spk),						/*for pad speaker,headphone switch*/
	SOC_SINGLE_BOOL_EXT("Audio phone out", 		0, codec_get_phoneout, 	codec_set_phoneout),				/*enable phoneout*/
	SOC_SINGLE_BOOL_EXT("Audio phone in", 		0, codec_get_phonein, 	codec_set_phonein),					/*open the phone in call*/
	SOC_SINGLE_BOOL_EXT("Audio phone in left", 	0, codec_get_phonein_left, 	codec_set_phonein_left),		/*open the phone in left channel call*/
	SOC_SINGLE_BOOL_EXT("Audio earpiece out", 	0, codec_get_earpieceout, 	codec_set_earpieceout),			/*set the phone in call voice through earpiece out*/
	SOC_SINGLE_BOOL_EXT("Audio headphone out", 	0, codec_get_headphoneout, 	codec_set_headphoneout),		/*set the phone in call voice through headphone out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 	0, codec_get_speakerout, 	codec_set_speakerout),			/*set the phone in call voice through speaker out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out left",   0, codec_get_speakerout_lntor,   codec_set_speakerout_lntor),
	SOC_SINGLE_BOOL_EXT("Audio headphone out left", 0, codec_get_headphoneout_lntor, codec_set_headphoneout_lntor),

	SOC_SINGLE_BOOL_EXT("Audio adc phonein", 	0, codec_get_adcphonein, 	codec_set_adcphonein), 			/*bluetooth voice*/
	SOC_SINGLE_BOOL_EXT("Audio dac phoneout", 	0, codec_get_dacphoneout, 	codec_set_dacphoneout),    		/*bluetooth voice */
	SOC_SINGLE_BOOL_EXT("Audio phone mic", 		0, codec_get_mainmic, 		codec_set_mainmic), 			/*set main mic(mic1)*/
	SOC_SINGLE_BOOL_EXT("Audio phone headsetmic", 	0, codec_get_headsetmic, 	codec_set_headsetmic),    	/*set headset mic(mic2)*/
	SOC_SINGLE_BOOL_EXT("Audio phone voicerecord", 	0, codec_get_voicerecord, 	codec_set_voicerecord),   	/*set voicerecord status*/
	SOC_SINGLE_BOOL_EXT("Audio phone endcall", 	0, codec_get_endcall, 	codec_set_endcall),    				/*set voicerecord status*/

	SOC_SINGLE_BOOL_EXT("Audio linein record", 	0, codec_get_lineincap, codec_set_lineincap),
	SOC_SINGLE_BOOL_EXT("Audio linein in", 		0, codec_get_lineinin, 	codec_set_lineinin),
	SOC_SINGLE_BOOL_EXT("Audio noise adcin reduced", 	0, codec_get_noise_adcin_reduced, codec_set_noise_adcin_reduced),
	SOC_SINGLE_BOOL_EXT("Audio noise dacphoneout reduced", 	0, codec_get_dacphoneout_reduced, codec_set_dacphoneout_reduced),

	SOC_SINGLE_BOOL_EXT("Audio fm headset", 	0, codec_get_fm_headset, codec_set_fm_headset),
	SOC_SINGLE_BOOL_EXT("Audio fm speaker", 	0, codec_get_fm_speaker, codec_set_fm_speaker),

	SOC_ENUM_EXT("Speaker Function", spk_headset_earpiece_enum[0], codec_get_spk_headset_earpiece, codec_set_spk_headset_earpiece),
};

int __init snd_chip_codec_mixer_new(struct sun6i_codec *chip)
{
	struct snd_card *card;
	int idx, err;

	static struct snd_device_ops ops = {
  		.dev_free	=	codec_dev_free,
  	};
  	card = chip->card;
	for (idx = 0; idx < ARRAY_SIZE(codec_snd_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&codec_snd_controls[idx],chip))) < 0) {
			return err;
		}
	}
	
	if ((err = snd_device_new(card, SNDRV_DEV_CODEC, chip, &ops)) < 0) {
		return err;
	}

	return 0;
}

static void sun6i_pcm_enqueue(struct snd_pcm_substream *substream)
{	
	int play_ret = 0, capture_ret = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	dma_addr_t play_pos = 0, capture_pos = 0;
	unsigned long play_len = 0, capture_len = 0;
	unsigned int play_limit = 0, capture_limit = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		play_pos = play_prtd->dma_pos;
		play_len = play_prtd->dma_period;
		play_limit = play_prtd->dma_limit; 
		while (play_prtd->dma_loaded < play_limit) {
			if ((play_pos + play_len) > play_prtd->dma_end) {
				play_len  = play_prtd->dma_end - play_pos;
			}
			/*because dma enqueue the first buffer while config dma,so at the beginning, can't add the buffer*/
			if (play_prtd->play_dma_flag) {
				play_ret = sw_dma_enqueue(play_prtd->dma_hdl, play_pos, play_prtd->params->dma_addr, play_len, ENQUE_PHASE_NORMAL);
			}
			play_prtd->play_dma_flag = true;
			if (play_ret == 0) {
				play_prtd->dma_loaded++;
				play_pos += play_prtd->dma_period;
				if(play_pos >= play_prtd->dma_end)
					play_pos = play_prtd->dma_start;
			} else {
				break;
			}
		}
		play_prtd->dma_pos = play_pos;
	} else {
		capture_prtd = substream->runtime->private_data;
		capture_pos = capture_prtd->dma_pos;
		capture_len = capture_prtd->dma_period;
		capture_limit = capture_prtd->dma_limit;
		while (capture_prtd->dma_loaded < capture_limit) {
			if ((capture_pos + capture_len) > capture_prtd->dma_end) {
				capture_len  = capture_prtd->dma_end - capture_pos;
			}
			/*because dma enqueue the first buffer while config dma,so at the beginning, can't add the buffer*/
			if (capture_prtd->capture_dma_flag) {
				capture_ret = sw_dma_enqueue(capture_prtd->dma_hdl, capture_prtd->params->dma_addr, capture_pos, capture_len, ENQUE_PHASE_NORMAL);
			}
			capture_prtd->capture_dma_flag = true;
			if (capture_ret == 0) {
			capture_prtd->dma_loaded++;
			capture_pos += capture_prtd->dma_period;
			if (capture_pos >= capture_prtd->dma_end)
				capture_pos = capture_prtd->dma_start;
			} else {
				break;
			}	  
		}
		capture_prtd->dma_pos = capture_pos;
	}
}

static u32 sun6i_audio_capture_hdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)
{
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_substream *substream = NULL;

	if ((result == DMA_CB_ABORT) || (parg == NULL)) {
		return 0;
	}
	substream = parg;
	capture_prtd = substream->runtime->private_data;

	spin_lock(&capture_prtd->lock);
	{
		capture_prtd->dma_loaded--;
		sun6i_pcm_enqueue(substream);
	}
	spin_unlock(&capture_prtd->lock);

	if ((substream) && (capture_prtd)) {
		snd_pcm_period_elapsed(substream);
	} else {
		return 0;
	}

	return 0;
}

static u32 sun6i_audio_play_hdone(dm_hdl_t dma_hdl, void *parg,
                                                 enum dma_cb_cause_e result)
{
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct snd_pcm_substream *substream = NULL;
	
	if ((result == DMA_CB_ABORT) || (parg == NULL)) {
		return 0;
	}

	substream = parg;
	play_prtd = substream->runtime->private_data;
	spin_lock(&play_prtd->lock);
	{
		play_prtd->dma_loaded--;
		sun6i_pcm_enqueue(substream);
	}
	spin_unlock(&play_prtd->lock);

	if ((substream) && (play_prtd)) {
		snd_pcm_period_elapsed(substream);
	}

	return 0;
}

static snd_pcm_uframes_t snd_sun6i_codec_pointer(struct snd_pcm_substream *substream)
{	
	unsigned long capture_res = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;
    snd_pcm_uframes_t play_offset = 0;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;
		play_prtd = play_runtime->private_data;
		spin_lock(&play_prtd->lock);
		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_GET_CUR_SRC_ADDR, &play_dmasrc)) {
			printk("err:%s, line:%d\n", __func__, __LINE__);
		}
		play_offset = bytes_to_frames(play_runtime, play_dmasrc - play_runtime->dma_addr);
		spin_unlock(&play_prtd->lock);
		if (play_offset >= play_runtime->buffer_size) {
			play_offset = 0;
		}
		return play_offset;
    } else {
    	capture_runtime = substream->runtime;
    	capture_prtd = capture_runtime->private_data;
		spin_lock(&capture_prtd->lock);
    	if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_GET_CUR_DST_ADDR, &capture_dmadst)) {
			printk("err:%s, line:%d\n", __func__, __LINE__);
		}
    	capture_res = capture_dmadst - capture_prtd->dma_start;
    	if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
				capture_res = 0;
		}
		spin_unlock(&capture_prtd->lock);
		return bytes_to_frames(substream->runtime, capture_res);
    }
}

static int sun6i_codec_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *play_runtime = NULL, *capture_runtime = NULL;
    struct sun6i_playback_runtime_data *play_prtd = NULL;
    struct sun6i_capture_runtime_data *capture_prtd = NULL;
    unsigned long play_totbytes = 0, capture_totbytes = 0;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	  	play_runtime = substream->runtime;
		play_prtd = play_runtime->private_data;
		play_totbytes = params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));

		if (play_prtd->params == NULL) {
			play_prtd->params = &sun6i_codec_pcm_stereo_play;
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			play_prtd->dma_hdl = sw_dma_request(play_prtd->params->name, DMA_WORK_MODE_CHAIN);
			if (NULL == play_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_play dma handle\n");
				return -EINVAL;
			}
			/*
		 	* set callback
		 	*/
			memset(&play_prtd->play_hdone_cb, 0, sizeof(play_prtd->play_hdone_cb));
			play_prtd->play_hdone_cb.func = sun6i_audio_play_hdone;
			play_prtd->play_hdone_cb.parg = substream;
			/*use the full buffer callback, maybe we should use the half buffer callback?*/
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_SET_HD_CB, (void *)&(play_prtd->play_hdone_cb))) {
				sw_dma_release(play_prtd->dma_hdl);
				return -EINVAL;
			}

			snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
			play_runtime->dma_bytes = play_totbytes;
   			spin_lock_irq(&play_prtd->lock);
			play_prtd->dma_loaded = 0;
			play_prtd->dma_limit = play_runtime->hw.periods_min;
			play_prtd->dma_period = params_period_bytes(params);
			play_prtd->dma_start = play_runtime->dma_addr;	

			play_dmasrc = play_prtd->dma_start;
			play_prtd->dma_pos = play_prtd->dma_start;
			play_prtd->dma_end = play_prtd->dma_start + play_totbytes;
			spin_unlock_irq(&play_prtd->lock);
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		capture_runtime = substream->runtime;
		capture_prtd = capture_runtime->private_data;
		capture_totbytes = params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (capture_prtd->params == NULL) {
			capture_prtd->params = &sun6i_codec_pcm_stereo_capture;			
			/*
			 * requeset audio_capture dma handle(we don't care about the channel!)
			 */
			capture_prtd->dma_hdl = sw_dma_request(capture_prtd->params->name, DMA_WORK_MODE_CHAIN);
			if (NULL == capture_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_capture dma handle\n");
				return -EINVAL;
			}
			/*
		 	* set callback
		 	*/
			memset(&capture_prtd->capture_hdone_cb, 0, sizeof(capture_prtd->capture_hdone_cb));
			capture_prtd->capture_hdone_cb.func = sun6i_audio_capture_hdone;
			capture_prtd->capture_hdone_cb.parg = substream;
			/*use the full buffer callback, maybe we should use the half buffer callback?*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_SET_HD_CB, (void *)&(capture_prtd->capture_hdone_cb))) {
				sw_dma_release(capture_prtd->dma_hdl);
				return -EINVAL;
			}

			snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
			capture_runtime->dma_bytes = capture_totbytes;
			spin_lock_irq(&capture_prtd->lock);
			capture_prtd->dma_loaded = 0;
			capture_prtd->dma_limit = capture_runtime->hw.periods_min;
			capture_prtd->dma_period = params_period_bytes(params);
			capture_prtd->dma_start = capture_runtime->dma_addr;
			capture_dmadst = capture_prtd->dma_start;
			capture_prtd->dma_pos = capture_prtd->dma_start;
			capture_prtd->dma_end = capture_prtd->dma_start + capture_totbytes;
			spin_unlock_irq(&capture_prtd->lock);
		}
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_sun6i_codec_hw_free(struct snd_pcm_substream *substream)
{
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;	
   	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (play_prtd->params) {
			/*
			 * stop play dma transfer
			 */
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			/*
			*	release play dma handle
			*/
			if (0 != sw_dma_release(play_prtd->dma_hdl)) {
				return -EINVAL;
			}
			play_prtd->dma_hdl = (dm_hdl_t)NULL;
			play_prtd->params = NULL;
			/*
			 * Clear out the DMA and any allocated buffers.
			*/
			snd_pcm_lib_free_pages(substream);
		}
   	} else {
		capture_prtd = substream->runtime->private_data;
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (capture_prtd->params) {
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			/*
			*	release capture dma handle
			*/
			if (0 != sw_dma_release(capture_prtd->dma_hdl)) {
				return -EINVAL;
			}
			capture_prtd->dma_hdl = (dm_hdl_t)NULL;
			capture_prtd->params = NULL;
			/*
			 * Clear out the DMA and any allocated buffers.
			 */
			snd_pcm_lib_free_pages(substream);
		}
   	}
	return 0;
}

static int snd_sun6i_codec_prepare(struct snd_pcm_substream	*substream)
{
	struct dma_config_t play_dma_config;
	struct dma_config_t capture_dma_config;
	int play_ret = 0, capture_ret = 0;
	unsigned int reg_val;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (substream->runtime->rate) {
			case 44100:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 22050:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 11025:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 48000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 96000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(7<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 192000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(6<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 32000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 24000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 16000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 12000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 8000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			default:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);		
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val |=(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);			
				break;
			case 2:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
		}
	} else {
		switch (substream->runtime->rate) {
			case 44100:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 22050:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 11025:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 48000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 32000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 24000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 16000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 12000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 8000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			default:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					printk("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					printk("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);		
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
			case 2:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
			default:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
		}
	}
   if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
   	 	play_prtd = substream->runtime->private_data;
   	 	/* return if this is a bufferless transfer e.g.
	  	* codec <--> BT codec or GSM modem -- lg FIXME */       
   	 	if (!play_prtd->params) {
			return 0;
		}

   	 	/*open the dac channel register*/
   	 	if (codec_dacphoneout_reduced_en) {
   	 		codec_dacphoneout_reduced_open();
   	 	} else if (codec_dacphoneout_en) {
			play_ret = codec_dacphoneout_open();
		} else if (codec_speaker_headset_earpiece_en == 2) {
			play_ret = codec_pa_and_headset_play_open();
		} else if ((codec_speaker_headset_earpiece_en == 1)||(codec_speaker_en)) {
			play_ret = codec_pa_play_open();
		} else if (codec_speaker_headset_earpiece_en == 3) { 
			play_ret = codec_earpiece_play_open();
		} else {
			play_ret = codec_headphone_play_open();
		}
		
		memset(&play_dma_config, 0, sizeof(play_dma_config));
		play_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;
		play_dma_config.address_type = DMAADDRT_D_IO_S_LN;
		play_dma_config.para = 0;
		play_dma_config.irq_spt = CHAN_IRQ_HD;
		play_dma_config.src_addr = play_prtd->dma_start;
		play_dma_config.dst_addr = play_prtd->params->dma_addr;
		play_dma_config.byte_cnt = play_prtd->dma_period;
		play_dma_config.bconti_mode = false;
		play_dma_config.src_drq_type = DRQSRC_SDRAM;
		play_dma_config.dst_drq_type = DRQDST_AUDIO_CODEC;
		if (0 != sw_dma_config(play_prtd->dma_hdl, &play_dma_config, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}

		play_prtd->dma_loaded = 0;
		play_prtd->dma_pos = play_prtd->dma_start;
		play_prtd->play_dma_flag = false;
		/* enqueue dma buffers */
		sun6i_pcm_enqueue(substream);
		codec_play_start();
		return play_ret;
	} else {
		capture_prtd = substream->runtime->private_data;
   	 	/* return if this is a bufferless transfer e.g.
	  	 * codec <--> BT codec or GSM modem -- lg FIXME */
   	 	if (!capture_prtd->params) {
			return 0;
		}
	   	/*open the adc channel register*/
	   	if (codec_adcphonein_en) {
	   		codec_adcphonein_open();
			msleep(200);
		} else if (codec_voice_record_en && codec_mainmic_en) {
			codec_voice_main_mic_capture_open();
		} else if (codec_voice_record_en && codec_headsetmic_en){
			codec_voice_headset_mic_capture_open();
		} else if(codec_lineinin_en && codec_lineincap_en) {
			codec_voice_linein_capture_open();
		} else if(codec_voice_record_en && codec_noise_reduced_adcin_en){
			codec_noise_reduced_capture_open();
		} else {
	   		codec_capture_open();
		}
		memset(&capture_dma_config, 0, sizeof(capture_dma_config));
		capture_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;/*16bit*/
		capture_dma_config.address_type = DMAADDRT_D_LN_S_IO;
		capture_dma_config.para = 0;
		capture_dma_config.irq_spt = CHAN_IRQ_HD;
		capture_dma_config.src_addr = capture_prtd->params->dma_addr;
		capture_dma_config.dst_addr = capture_prtd->dma_start;
		capture_dma_config.byte_cnt = capture_prtd->dma_period;
		capture_dma_config.bconti_mode = false;
		capture_dma_config.src_drq_type = DRQSRC_AUDIO_CODEC;
		capture_dma_config.dst_drq_type = DRQDST_SDRAM;

		if (0 != sw_dma_config(capture_prtd->dma_hdl, &capture_dma_config, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}

		capture_prtd->dma_loaded = 0;
		capture_prtd->dma_pos = capture_prtd->dma_start;
		capture_prtd->capture_dma_flag = false;
		/* enqueue dma buffers */
		sun6i_pcm_enqueue(substream);
		return capture_ret;
	}
}

static int snd_sun6i_codec_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int headphone_vol = 0;
	int play_ret = 0, capture_ret = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		play_prtd = substream->runtime->private_data;
		switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_RESUME:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
				play_prtd->state |= ST_RUNNING;
				/*
				* start dma transfer
				*/
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_START, NULL)) {
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_SUSPEND:
				codec_play_stop();
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_STOP:
				play_prtd->state &= ~ST_RUNNING;
				codec_play_stop();
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
				play_prtd->state &= ~ST_RUNNING;
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			default:
				printk("error:%s,%d\n", __func__, __LINE__);
				play_ret = -EINVAL;
				break;
			}
	}else{
		capture_prtd = substream->runtime->private_data;
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			capture_prtd->state |= ST_RUNNING;	 

			codec_wr_control(SUN6I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
			/*
			* start dma transfer
			*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_START, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
			codec_capture_stop();
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			capture_prtd->state &= ~ST_RUNNING;
			codec_capture_stop();
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:		
			capture_prtd->state &= ~ST_RUNNING;
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		default:
			printk("error:%s,%d\n", __func__, __LINE__);
			capture_ret = -EINVAL;
			break;
		}
	}
	return 0;
}

static int snd_sun6icard_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun6i_capture_runtime_data *capture_prtd;

	capture_prtd = kzalloc(sizeof(struct sun6i_capture_runtime_data), GFP_KERNEL);
	if (capture_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&capture_prtd->lock);
	runtime->private_data = capture_prtd;
	runtime->hw = sun6i_pcm_capture_hardware;

	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;
        
	return 0;
}

static int snd_sun6icard_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	kfree(runtime->private_data);
	return 0;
}

static int snd_sun6icard_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun6i_playback_runtime_data *play_prtd;
	play_prtd = kzalloc(sizeof(struct sun6i_playback_runtime_data), GFP_KERNEL);
	if (play_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&play_prtd->lock);
	runtime->private_data = play_prtd;
	runtime->hw = sun6i_pcm_playback_hardware;
	
	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;

	return 0;
}

static int snd_sun6icard_playback_close(struct snd_pcm_substream *substream)
{
	int i = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	struct snd_pcm_runtime *runtime = substream->runtime;

	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_dacphoneout_en || codec_lineinin_en || codec_voice_record_en ) ) {
		for (i = headphone_vol; i > 0 ; i--) {
			/*set HPVOL volume*/
			codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
			usleep_range(2000,3000);
		}
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0);
		/*mute l_pa and r_pa*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	}
	/*disable dac drq*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC ,  0x1, DAC_EN, 0x0);
	kfree(runtime->private_data);
	return 0;
}

static struct snd_pcm_ops sun6i_pcm_playback_ops = {
	.open			= snd_sun6icard_playback_open,
	.close			= snd_sun6icard_playback_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params	    = sun6i_codec_pcm_hw_params,
	.hw_free	    = snd_sun6i_codec_hw_free,
	.prepare		= snd_sun6i_codec_prepare,
	.trigger		= snd_sun6i_codec_trigger,
	.pointer		= snd_sun6i_codec_pointer,
};

static struct snd_pcm_ops sun6i_pcm_capture_ops = {
	.open			= snd_sun6icard_capture_open,
	.close			= snd_sun6icard_capture_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params	    = sun6i_codec_pcm_hw_params,
	.hw_free	    = snd_sun6i_codec_hw_free,
	.prepare		= snd_sun6i_codec_prepare,
	.trigger		= snd_sun6i_codec_trigger,
	.pointer		= snd_sun6i_codec_pointer,
};

static int __init snd_card_sun6i_codec_pcm(struct sun6i_codec *sun6i_codec, int device)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(sun6i_codec->card, "M1 PCM", device, 1, 1, &pcm)) < 0){	
		printk("error,the func is: %s,the line is:%d\n", __func__, __LINE__);
		return err;
	}

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */
	 
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, 
					      snd_dma_isa_data(),
					      32*1024, 32*1024);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &sun6i_pcm_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &sun6i_pcm_capture_ops);
	pcm->private_data = sun6i_codec;
	pcm->info_flags = 0;
	strcpy(pcm->name, "sun6i PCM");
	/* setup DMA controller */
	return 0;
}

void snd_sun6i_codec_free(struct snd_card *card)
{

}

/*
extern int axp_spk_det(void);
static void spk_vol_work(struct work_struct *work)
{
	int ret;
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio_para", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk("[audiocodec] pa_double_used type err!\n");
	}

	pa_double_used = val.val;
	if (!pa_double_used) {
		type = script_get_item("audio_para", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_single_vol type err!\n");
		}
		pa_vol = val.val;
	} else {
		type = script_get_item("audio_para", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			printk("[audiocodec] pa_double_vol type err!\n");
		}
		pa_vol = val.val;
	}

	ret = axp_spk_det();
	if (ret == 1 && pa_vol > 0x1b)
		pa_vol = 0x1b;

	codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
}

static void speaker_timer_poll(unsigned long data)
{
	struct sun6i_codec	*chip =(struct sun6i_codec *)data;

	schedule_work(&chip->work);
	mod_timer(&chip->timer, jiffies +  5*HZ);
}
*/

static int __init sun6i_codec_probe(struct platform_device *pdev)
{
	int err;
	int ret;
	script_item_value_type_e  type;

	struct snd_card *card;
	struct sun6i_codec *chip;
	struct codec_board_info  *db;    
    printk("enter sun6i Audio codec!!!\n");
	/* register the soundcard */
	ret = snd_card_create(SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1, THIS_MODULE, sizeof(struct sun6i_codec), &card);
	if (ret != 0) {
		return -ENOMEM;
	}

	chip = card->private_data;
	card->private_free = snd_sun6i_codec_free;
	chip->card = card;
	chip->samplerate = AUDIO_RATE_DEFAULT;

	if ((err = snd_chip_codec_mixer_new(chip)))
		goto nodev;

	if ((err = snd_card_sun6i_codec_pcm(chip, 0)) < 0)
	    goto nodev;

	strcpy(card->driver, "sun6i-CODEC");
	strcpy(card->shortname, "audiocodec");
	sprintf(card->longname, "sun6i-CODEC  Audio Codec");
	snd_card_set_dev(card, &pdev->dev);
	if ((err = snd_card_register(card)) == 0) {
		printk( KERN_INFO "sun6i audio support initialized\n" );
		platform_set_drvdata(pdev, card);
	}else{
		printk("err:%s,line:%d\n", __func__, __LINE__);
		return err;
	}
	db = kzalloc(sizeof(*db), GFP_KERNEL);
	if (!db)
		return -ENOMEM;
  	/* codec_apbclk */
	codec_apbclk = clk_get(NULL, CLK_APB_ADDA);
	if ((!codec_apbclk)||(IS_ERR(codec_apbclk))) {
		printk("try to get codec_apbclk failed!\n");
	}
	if (clk_enable(codec_apbclk)) {
		printk("enable codec_apbclk failed; \n");
	}
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL, CLK_SYS_PLL2);
	if ((!codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		printk("try to get codec_pll2clk failed!\n");
	}
	if (clk_enable(codec_pll2clk)) {
		printk("enable codec_pll2clk failed; \n");
	}
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL, CLK_MOD_ADDA);
	if ((!codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		printk("try to get codec_moduleclk failed!\n");
	}
	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		printk("err:try to set parent of codec_moduleclk to codec_pll2clk failed!\n");
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
		printk("err:set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (clk_enable(codec_moduleclk)) {
		printk("err:open codec_moduleclk failed; \n");
	}
	if (clk_reset(codec_moduleclk, AW_CCU_CLK_NRESET)) {
		printk("try to NRESET ve module clk failed!\n");
	}
	db->codec_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	db->dev = &pdev->dev;
	if (db->codec_base_res == NULL) {
		ret = -ENOENT;
		printk("err:codec insufficient resources\n");
		goto out;
	}
	/* codec address remap */
	db->codec_base_req = request_mem_region(db->codec_base_res->start, 0x40, pdev->name);
	if (db->codec_base_req == NULL) {
		ret = -EIO;
		printk("err:cannot claim codec address reg area\n");
		goto out;
	}
	baseaddr = ioremap(db->codec_base_res->start, 0x40);
	if (baseaddr == NULL) {
		 ret = -EINVAL;
		 dev_err(db->dev,"failed to ioremap codec address reg\n");
		 goto out;
	}
	kfree(db);

	codec_init();

	/* check if hp_vcc_ldo exist, if exist enable it */
	type = script_get_item("audio_para", "audio_hp_ldo", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		printk("script_get_item return type err, consider it no ldo\n");
	} else {
		if (!strcmp(item.str, "none"))
			hp_ldo = NULL;
		else {
			hp_ldo_str = item.str;
			hp_ldo = regulator_get(NULL, hp_ldo_str);
			if (!hp_ldo) {
				printk("get audio hp-vcc(%s) failed\n", hp_ldo_str);
				return -EFAULT;
			}
			regulator_set_voltage(hp_ldo, 3000000, 3000000);
			regulator_enable(hp_ldo);
		}
	}
	/*get the default pa val(close)*/
	type = script_get_item("audio_para", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item return type err\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(item.gpio.gpio, NULL);
	if (0 != req_status) {
		printk("request gpio failed!\n");
	}
	/*config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex).*/
	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}

	/*
	init_timer(&chip->timer);
	chip->timer.function = speaker_timer_poll;
	chip->timer.data = (unsigned long)chip;
	mod_timer(&chip->timer, jiffies +  5*HZ );
	INIT_WORK(&chip->work, spk_vol_work);
	*/

	printk("sun6i Audio codec init end\n");
	 return 0;
out:
		 dev_err(db->dev, "not found (%d).\n", ret);

	 nodev:
		snd_card_free(card);
		return err;
}

#ifdef CONFIG_PM
static int snd_sun6i_codec_suspend(struct platform_device *pdev,pm_message_t state)
{
	int i =0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	/* check if called in talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		printk("codec: In talking standby, enable wakeup srouce!!\n");
		enable_wakeup_src(CPUS_CODEC_SRC, 0);
		return 0;
	}
	printk("[audio codec]:suspend\n");

	item.gpio.data = 0;
	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	for (i = headphone_vol; i > 0 ; i--) {
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
		reg_val = codec_rdreg(SUN6I_DAC_ACTL);
		reg_val &= 0x3f;
		if ((i%2==0))
			usleep_range(2000, 3000);
	}
	codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC ,  0x1, DAC_EN, 0x0);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASEN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASADCEN, 0x0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	msleep(100);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	msleep(20);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);

	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		printk("codec_moduleclk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_disable(codec_moduleclk);
	}
	printk("[audio codec]:suspend end\n");
	return 0;
}

static int snd_sun6i_codec_resume(struct platform_device *pdev)
{
	script_item_u val;
	script_item_value_type_e  type;
	int headphone_direct_used = 0;
	enum sw_ic_ver  codec_chip_ver;

	/* do nothing if resume from talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		disable_wakeup_src(CPUS_CODEC_SRC, 0);
		printk("codec: resume from talking standby, disable wakeup source!!\n");
		return 0;
	}

	codec_chip_ver = sw_get_ic_ver();
	printk("[audio codec]:resume start\n");

	if (clk_enable(codec_moduleclk)) {
		printk("open codec_moduleclk failed; \n");
	}

	type = script_get_item("audio_para", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[audiocodec] headphone_direct_used type err!\n");
    }
	headphone_direct_used = val.val;
	if (headphone_direct_used && (codec_chip_ver != MAGIC_VER_A)) {
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x3);
		codec_wr_control(SUN6I_PA_CTRL, 0x1, HPCOM_PRO, 0x1);
	} else {
		codec_wr_control(SUN6I_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUN6I_PA_CTRL, 0x1, HPCOM_PRO, 0x0);
	}

	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
	/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
		/*when TX FIFO available room less than or equal N,
		* DRQ Requeest will be de-asserted.
		*/
		codec_wr_control(SUN6I_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
		/*write 1 to flush tx fifo*/
		codec_wr_control(SUN6I_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
		/*write 1 to flush rx fifo*/
		codec_wr_control(SUN6I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	}

	codec_wr_control(SUN6I_DAC_FIFOC, 0x1, FIR_VERSION, 0x1);
	printk("[audio codec]:resume end\n");
	return 0;
}
#endif

static int __devexit sun6i_codec_remove(struct platform_device *devptr)
{
	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		printk("codec_moduleclk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_disable(codec_moduleclk);
	}
	if ((NULL == codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		printk("codec_pll2clk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_put(codec_pll2clk);
	}
	if ((NULL == codec_apbclk)||(IS_ERR(codec_apbclk))) {
		printk("codec_apbclk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_put(codec_apbclk);
	}
	/* disable audio hp-vcc ldo if it exist */
	if (hp_ldo) {
		regulator_disable(hp_ldo);
		hp_ldo = NULL;
	}
	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}

static void sun6i_codec_shutdown(struct platform_device *devptr)
{
	int i =0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	item.gpio.data = 0;
	type = script_get_item("audio_para", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	printk("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_wr_control(SUN6I_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	for (i = headphone_vol; i > 0 ; i--) {
		/*set HPVOL volume*/
		codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, i);
		reg_val = codec_rdreg(SUN6I_DAC_ACTL);
		reg_val &= 0x3f;
		if ((i%2==0))
			usleep_range(2000, 3000);
	}
	codec_wr_control(SUN6I_DAC_ACTL, 0x3f, VOLUME, 0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC ,  0x1, DAC_EN, 0x0);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASEN, 0x0);
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASADCEN, 0x0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	msleep(100);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	msleep(20);

	codec_wr_control(SUN6I_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);

	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		printk("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable(codec_moduleclk);
	}
}

static struct resource sun6i_codec_resource[] = {
	[0] = {
    	.start = CODEC_BASSADDRESS,
        .end   = CODEC_BASSADDRESS + 0x40,
		.flags = IORESOURCE_MEM,
	},
};

/*data relating*/
static struct platform_device sun6i_device_codec = {
	.name = "sun6i-codec",
	.id = -1,
	.num_resources = ARRAY_SIZE(sun6i_codec_resource),
	.resource = sun6i_codec_resource,
};

/*method relating*/
static struct platform_driver sun6i_codec_driver = {
	.probe		= sun6i_codec_probe,
	.remove		= sun6i_codec_remove,
	.shutdown   = sun6i_codec_shutdown,
#ifdef CONFIG_PM
	.suspend	= snd_sun6i_codec_suspend,
	.resume		= snd_sun6i_codec_resume,
#endif
	.driver		= {
		.name	= "sun6i-codec",
	},
};

static int __init sun6i_codec_init(void)
{
	int err = 0;
	if((platform_device_register(&sun6i_device_codec))<0)
		return err;

	if ((err = platform_driver_register(&sun6i_codec_driver)) < 0)
		return err;
	
	return 0;
}

static void __exit sun6i_codec_exit(void)
{
	platform_driver_unregister(&sun6i_codec_driver);
}

module_init(sun6i_codec_init);
module_exit(sun6i_codec_exit);

MODULE_DESCRIPTION("sun6i CODEC ALSA codec driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");
