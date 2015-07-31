/*
 * sound\soc\sun6i\spdif\sndspdif.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * chenpailin <chenpailin@Reuuimllatech.com>
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
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
#include <mach/sys_config.h>

static int spdif_used = 0;
#define SNDSPDIF_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define SNDSPDIF_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)

struct sndspdif_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

static int sndspdif_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndspdif_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void sndspdif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int sndspdif_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int sndspdif_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndspdif_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	return 0;
}

static int sndspdif_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

struct snd_soc_dai_ops sndspdif_dai_ops = {
	.startup = sndspdif_startup,
	.shutdown = sndspdif_shutdown,
	.hw_params = sndspdif_hw_params,
	.digital_mute = sndspdif_mute,
	.set_sysclk = sndspdif_set_dai_sysclk,
	.set_clkdiv = sndspdif_set_dai_clkdiv,
	.set_fmt = sndspdif_set_dai_fmt,
};
struct snd_soc_dai_driver sndspdif_dai = {
	.name = "sndspdif",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 4,
		.rates = SNDSPDIF_RATES,
		.formats = SNDSPDIF_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 4,
		.rates = SNDSPDIF_RATES,
		.formats = SNDSPDIF_FORMATS,
	},
	/* pcm operations */
	.ops = &sndspdif_dai_ops,
};
EXPORT_SYMBOL(sndspdif_dai);

static int sndspdif_soc_probe(struct snd_soc_codec *codec)
{
	struct sndspdif_priv *sndspdif;	
	
	sndspdif = kzalloc(sizeof(struct sndspdif_priv), GFP_KERNEL);
	if(sndspdif == NULL){
		printk("%s,%d\n",__func__,__LINE__);
		return -ENOMEM;
	}	
	
	snd_soc_codec_set_drvdata(codec, sndspdif);
	
	return 0;
}

/* power down chip */
static int sndspdif_soc_remove(struct snd_soc_codec *codec)
{
	struct sndspdif_priv *sndspdif = snd_soc_codec_get_drvdata(codec);

	kfree(sndspdif);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndspdif = {
	.probe 	=	sndspdif_soc_probe,
	.remove =   sndspdif_soc_remove,
};

static int __devinit sndspdif_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndspdif, &sndspdif_dai, 1);	
}

static int __devexit sndspdif_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sndspdif_codec_device = {
	.name = "sun6i-spdif-codec",
};

/*method relating*/
static struct platform_driver sndspdif_codec_driver = {
	.driver = {
		.name = "sun6i-spdif-codec",
		.owner = THIS_MODULE,
	},
	.probe = sndspdif_codec_probe,
	.remove = __devexit_p(sndspdif_codec_remove),
};

static int __init sndspdif_codec_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("spdif_para", "spdif_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[SPDIF] type err!\n");
    }

	spdif_used = val.val;
	if (spdif_used) {
		if((err = platform_device_register(&sndspdif_codec_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sndspdif_codec_driver)) < 0)
			return err;
	} else {
        printk("[SPDIF]sndspdif cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }
	
	return 0;
}
module_init(sndspdif_codec_init);

static void __exit sndspdif_codec_exit(void)
{
	if (spdif_used) {	
		spdif_used = 0;
		platform_driver_unregister(&sndspdif_codec_driver);
	}
}
module_exit(sndspdif_codec_exit);

MODULE_DESCRIPTION("SNDSPDIF ALSA soc codec driver");
MODULE_AUTHOR("chenpailin");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun6i-spdif-codec");
