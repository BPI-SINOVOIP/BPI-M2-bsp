/*
 * sound\soc\sun6i\pcm\sun6i_sndpcm.c
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

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <mach/sys_config.h>

#include "sun6i-pcmdma.h"

static bool i2s_pcm_select  = 0;
static int pcm_used 		= 0;
static int pcm_master 		= 0;
static int audio_format 	= 0;
static int signal_inversion = 0;

/*
*	i2s_pcm_select == 0:-->	pcm
*	i2s_pcm_select == 1:-->	i2s
*/
static int sun6i_pcm_set_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	i2s_pcm_select = ucontrol->value.integer.value[0];
	if (i2s_pcm_select) {
		pcm_master 			= 4;
		audio_format 		= 1;
		signal_inversion 	= 1;
	} else {
		pcm_master 			= 4;
		audio_format 		= 4;
		signal_inversion 	= 3;
	}

	return 0;
}

static int sun6i_pcm_get_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = i2s_pcm_select;
	return 0;
}

/* I2s Or Pcm Audio Mode Select */
static const struct snd_kcontrol_new sun6i_pcm_controls[] = {
	SOC_SINGLE_BOOL_EXT("I2s Or Pcm Audio Mode Select", 0,
			sun6i_pcm_get_audio_mode, sun6i_pcm_set_audio_mode),
};

static int sun6i_sndpcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned long sample_rate = params_rate(params);
	
	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq = 24576000;
			break;
	}

	/*set system clock source freq*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, i2s_pcm_select);
	if (ret < 0) {
		return ret;
	}

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
		SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
	/*
	* codec clk & FRM master. AP as slave
	*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | (pcm_master<<12)));
	if (ret < 0) {
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}

	/*
	*	audio_format == SND_SOC_DAIFMT_DSP_A
	*	signal_inversion<<8 == SND_SOC_DAIFMT_IB_NF
	*	pcm_master<<12	== SND_SOC_DAIFMT_CBS_CFS
	*/
	PCM_DBG("%s,line:%d,audio_format:%d,SND_SOC_DAIFMT_DSP_A:%d\n",\
				__func__, __LINE__, audio_format, SND_SOC_DAIFMT_DSP_A);
	PCM_DBG("%s,line:%d,signal_inversion:%d,signal_inversion<<8:%d,SND_SOC_DAIFMT_IB_NF:%d\n",\
				__func__, __LINE__, signal_inversion, signal_inversion<<8, SND_SOC_DAIFMT_IB_NF);
	PCM_DBG("%s,line:%d,pcm_master:%d,pcm_master<<12:%d,SND_SOC_DAIFMT_CBS_CFS:%d\n",\
				__func__, __LINE__, pcm_master, pcm_master<<12, SND_SOC_DAIFMT_CBS_CFS);

	return 0;
}

/*
 * Card initialization
 */
static int sun6i_pcm_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	int ret;

	/* Add virtual switch */
	ret = snd_soc_add_controls(codec, sun6i_pcm_controls,
					ARRAY_SIZE(sun6i_pcm_controls));
	if (ret) {
		dev_warn(card->dev,
				"Failed to register audio mode control, "
				"will continue without it.\n");
	}
	return 0;
}

static struct snd_soc_ops sun6i_sndpcm_ops = {
	.hw_params 		= sun6i_sndpcm_hw_params,
};

static struct snd_soc_dai_link sun6i_sndpcm_dai_link = {
	.name 			= "PCM",
	.stream_name 	= "SUN6I-PCM",
	.cpu_dai_name 	= "sun6i-pcm.0",
	.codec_dai_name = "sndpcm",
	.init 			= sun6i_pcm_init,
	.platform_name 	= "sun6i-pcm-pcm-audio.0",	
	.codec_name 	= "sun6i-pcm-codec.0",
	.ops 			= &sun6i_sndpcm_ops,
};

static struct snd_soc_card snd_soc_sun6i_sndpcm = {
	.name 		= "sndpcm",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sun6i_sndpcm_dai_link,
	.num_links 	= 1,
};

static struct platform_device *sun6i_sndpcm_device;

static int __init sun6i_sndpcm_init(void)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("pcm_para", "pcm_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] type err!\n");
    }
	pcm_used = val.val;

	type = script_get_item("pcm_para", "pcm_master", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] pcm_master type err!\n");
    }
	pcm_master = val.val;	
	type = script_get_item("pcm_para", "audio_format", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] audio_format type err!\n");
    }
	audio_format = val.val;

	type = script_get_item("pcm_para", "signal_inversion", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] signal_inversion type err!\n");
    }
	signal_inversion = val.val;
    if (pcm_used) {
		sun6i_sndpcm_device = platform_device_alloc("soc-audio", 3);
		if(!sun6i_sndpcm_device)
			return -ENOMEM;
		platform_set_drvdata(sun6i_sndpcm_device, &snd_soc_sun6i_sndpcm);
		ret = platform_device_add(sun6i_sndpcm_device);		
		if (ret) {			
			platform_device_put(sun6i_sndpcm_device);
		}
	}else{
		printk("[PCM]sun6i_sndpcm cannot find any using configuration for controllers, return directly!\n");
        return 0;
	}

	return ret;
}

static void __exit sun6i_sndpcm_exit(void)
{
	if(pcm_used) {
		pcm_used = 0;
		platform_device_unregister(sun6i_sndpcm_device);
	}
}

module_init(sun6i_sndpcm_init);
module_exit(sun6i_sndpcm_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUN6I_sndpcm ALSA SoC audio driver");
MODULE_LICENSE("GPL");
