/*
 * sound\soc\sun6i\hdmiaudio\sun6i-hdmipcm.c
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
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include "sun6i-hdmipcm.h"

static unsigned int dmasrc = 0;
static unsigned int dmadst = 0;

static const struct snd_pcm_hardware sun6i_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 4,
	.buffer_bytes_max	= 128*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024*4,
	.period_bytes_max	= 1024*32,
	.periods_min		= 4,
	.periods_max		= 8,
	.fifo_size			= 128,
};

struct sun6i_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int 	dma_loaded;
	unsigned int 	dma_limit;
	unsigned int 	dma_period;
	dma_addr_t 		dma_start;
	dma_addr_t 		dma_pos;
	dma_addr_t 		dma_end;
	dm_hdl_t		dma_hdl;
	bool 			play_dma_flag;
	struct dma_cb_t play_done_cb;
	struct dma_cb_t play_hdone_cb;
	struct sun6i_dma_params *params;
};

static void sun6i_pcm_enqueue(struct snd_pcm_substream *substream)
{
	int ret 			= 0;
	struct sun6i_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos 		= prtd->dma_pos;
	unsigned long len 	= prtd->dma_period;
	unsigned int limit	= prtd->dma_limit;

  	while (prtd->dma_loaded < limit) {
		if ((pos + len) > prtd->dma_end) {
			len  = prtd->dma_end - pos;			
		}
		/*because dma enqueue the first buffer while config dma,so at the beginning, can't add the buffer*/
		if (prtd->play_dma_flag) {
			ret = sw_dma_enqueue(prtd->dma_hdl, pos,
								prtd->params->dma_addr, len, ENQUE_PHASE_NORMAL);
		}
		prtd->play_dma_flag = true;
		if (ret == 0) {
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if(pos >= prtd->dma_end) {
				pos = prtd->dma_start;
			}
		}else {
			break;
		}
	}
	prtd->dma_pos = pos;
}

static u32 sun6i_audio_hdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)
{
	struct sun6i_runtime_data *play_prtd = NULL;
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

static u32 sun6i_audio_buffdone(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e result)                       
{
	struct sun6i_runtime_data *prtd 	= NULL;
	struct snd_pcm_substream *substream = parg;

	if ((result == DMA_CB_ABORT) || (parg == NULL)) {
		return 0;
	}
	substream = parg;
	prtd = substream->runtime->private_data;
	if ((substream) && (prtd)) {
		snd_pcm_period_elapsed(substream);
	}

	spin_lock(&prtd->lock);
	{
		prtd->dma_loaded--;
		sun6i_pcm_enqueue(substream);
	}
	spin_unlock(&prtd->lock);
	return 0;
}

static int sun6i_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun6i_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned long totbytes 			= params_buffer_bytes(params);
	struct sun6i_dma_params *dma 	= snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	if (!dma) {
		return 0;
	}

	if (prtd->params == NULL) {
		prtd->params = dma;

		switch(params_channels(params)) {
		case 1:/*pcm mode*/
		case 2:
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			prtd->dma_hdl = sw_dma_request(prtd->params->name, DMA_WORK_MODE_SINGLE);
			if (NULL == prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_play dma handle\n");
				return -EINVAL;
			}
			/*
		 	* set callback
		 	*/
			memset(&prtd->play_done_cb, 0, sizeof(prtd->play_done_cb));
			prtd->play_done_cb.func = sun6i_audio_buffdone;
			prtd->play_done_cb.parg = substream;
			/*use the full buffer callback, maybe we should use the half buffer callback?*/
			if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_SET_QD_CB, (void *)&(prtd->play_done_cb))) {
				printk("err:%s,line:%d\n", __func__, __LINE__);
				sw_dma_release(prtd->dma_hdl);
				return -EINVAL;
			}
			break;
		case 4:/*raw data mode*/
			printk("%s, line:%d\n", __func__, __LINE__);
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			prtd->dma_hdl = sw_dma_request(prtd->params->name, DMA_WORK_MODE_CHAIN);
			if (NULL == prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_play dma handle\n");
				return -EINVAL;
			}
			/* set half done callback */
			memset(&prtd->play_hdone_cb, 0, sizeof(prtd->play_hdone_cb));
			prtd->play_hdone_cb.func = sun6i_audio_hdone;
			prtd->play_hdone_cb.parg = substream;
			if(0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_SET_HD_CB, (void *)&(prtd->play_hdone_cb))) {
				sw_dma_release(prtd->dma_hdl);
				return -EINVAL;
			}
				break;
			default:
				return -EINVAL;
		}
	}
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes 	= totbytes;

	spin_lock_irq(&prtd->lock);
	prtd->dma_loaded 	= 0;
	prtd->dma_limit 	= runtime->hw.periods_min;
	prtd->dma_period 	= params_period_bytes(params);
	prtd->dma_start 	= runtime->dma_addr;
	prtd->dma_pos 		= prtd->dma_start;
	prtd->dma_end 		= prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);
	return 0;
}

static int sun6i_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct sun6i_runtime_data *prtd = substream->runtime->private_data;
	
	snd_pcm_set_runtime_buffer(substream, NULL);
  
	if (prtd->params) {
		/*
		 * stop play dma transfer
		 */
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			return -EINVAL;
		}
		/*
		*	release play dma handle
		*/
		if (0 != sw_dma_release(prtd->dma_hdl)) {
			return -EINVAL;
		}
		prtd->dma_hdl = (dm_hdl_t)NULL;	
		prtd->params = NULL;
	}

	return 0;
}

static int sun6i_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct dma_config_t hdmiaudio_dma_conf;
	struct sun6i_runtime_data *prtd = substream->runtime->private_data;
	
	if (!prtd->params) {
		return 0;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/*
		 * config play dma para
		 */ 
		memset(&hdmiaudio_dma_conf, 0, sizeof(hdmiaudio_dma_conf));
		hdmiaudio_dma_conf.xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
		hdmiaudio_dma_conf.address_type = DMAADDRT_D_IO_S_LN;
		hdmiaudio_dma_conf.para 		= 0;
               if (substream->runtime->channels == 4) {
                       hdmiaudio_dma_conf.irq_spt = CHAN_IRQ_HD;
               } else {
                       hdmiaudio_dma_conf.irq_spt = CHAN_IRQ_QD;
               }
//		hdmiaudio_dma_conf.irq_spt 		= CHAN_IRQ_QD;
		hdmiaudio_dma_conf.src_addr		= prtd->dma_start;
		hdmiaudio_dma_conf.dst_addr		= prtd->params->dma_addr;
		hdmiaudio_dma_conf.byte_cnt		= prtd->dma_period;
		hdmiaudio_dma_conf.bconti_mode  = false;
		hdmiaudio_dma_conf.src_drq_type = DRQSRC_SDRAM;
		hdmiaudio_dma_conf.dst_drq_type = DRQDST_HDMI_AUDIO;
		dmasrc = prtd->dma_start;
		if (0 != sw_dma_config(prtd->dma_hdl, &hdmiaudio_dma_conf, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}
	} 

	prtd->dma_loaded = 0;
	prtd->dma_pos = prtd->dma_start;
	prtd->play_dma_flag = false;
	/* enqueue dma buffers */
	sun6i_pcm_enqueue(substream);

	return ret;	
}

static int sun6i_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret  = 0;
	struct sun6i_runtime_data *prtd = substream->runtime->private_data;

	spin_lock(&prtd->lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("%s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
		}
		break;
		
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
		}
		break;

	default:
		ret = -EINVAL;
		break;
	}
	spin_unlock(&prtd->lock);
	return ret;
}

static snd_pcm_uframes_t sun6i_pcm_pointer(struct snd_pcm_substream *substream)
{
	unsigned long res 			= 0;
	snd_pcm_uframes_t offset 	= 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun6i_runtime_data *prtd = runtime->private_data;
	
	spin_lock(&prtd->lock);
	
	if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_GET_CUR_SRC_ADDR, &dmasrc)) {
		printk("%s,err\n", __func__);
	}

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		res = dmadst - prtd->dma_start;
	} else {
		offset = bytes_to_frames(runtime, dmasrc - runtime->dma_addr);
	}
	spin_unlock(&prtd->lock);

	if(offset >= runtime->buffer_size) {
		offset = 0;
	}
	return offset;
}

static int sun6i_pcm_open(struct snd_pcm_substream *substream)
{
	struct sun6i_runtime_data *prtd = NULL;
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_soc_set_runtime_hwparams(substream, &sun6i_pcm_hardware);
	
	prtd = kzalloc(sizeof(struct sun6i_runtime_data), GFP_KERNEL);
	if (prtd == NULL) {
		return -ENOMEM;
	}
		
	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;
	return 0;
}

static int sun6i_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun6i_runtime_data *prtd = runtime->private_data;
	
	kfree(prtd);
	
	return 0;
}

static int sun6i_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
}

static struct snd_pcm_ops sun6i_pcm_ops = {
	.open			= sun6i_pcm_open,
	.close			= sun6i_pcm_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params		= sun6i_pcm_hw_params,
	.hw_free		= sun6i_pcm_hw_free,
	.prepare		= sun6i_pcm_prepare,
	.trigger		= sun6i_pcm_trigger,
	.pointer		= sun6i_pcm_pointer,
	.mmap			= sun6i_pcm_mmap,
};

static int sun6i_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf 			= &substream->dma_buffer;
	size_t size 			 			= sun6i_pcm_hardware.buffer_bytes_max;

	buf->dev.type 		= SNDRV_DMA_TYPE_DEV;
	buf->dev.dev 		= pcm->card->dev;
	buf->private_data 	= NULL;
	buf->area 			= dma_alloc_writecombine(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
	if (!buf->area) {
		return -ENOMEM;
	}
	buf->bytes = size;
	return 0;
}

static void sun6i_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	int stream 							= 0;
	struct snd_dma_buffer *buf 			= NULL;
	struct snd_pcm_substream *substream = NULL;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area) {
			continue;
		}

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 sun6i_pcm_mask = DMA_BIT_MASK(32);

static int sun6i_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_pcm *pcm 	= rtd->pcm;
	struct snd_card *card 	= rtd->card->snd_card;

	if (!card->dev->dma_mask) {
		card->dev->dma_mask = &sun6i_pcm_mask;
	}
	if (!card->dev->coherent_dma_mask) {
		card->dev->coherent_dma_mask = 0xffffffff;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = sun6i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret) {
			goto out;
		}
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = sun6i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret) {
			goto out;
		}
	}
 out:
	return ret;
}

static struct snd_soc_platform_driver sun6i_soc_platform_hdmiaudio = {
		.ops        = &sun6i_pcm_ops,
		.pcm_new	= sun6i_pcm_new,
		.pcm_free	= sun6i_pcm_free_dma_buffers,
};

static int __devinit sun6i_hdmiaudio_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &sun6i_soc_platform_hdmiaudio);
}

static int __devexit sun6i_hdmiaudio_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sun6i_hdmiaudio_pcm_device = {
	.name = "sun6i-hdmiaudio-pcm-audio",
};

static struct platform_driver sun6i_hdmiaudio_pcm_driver = {
	.probe 	= sun6i_hdmiaudio_pcm_probe,
	.remove = __devexit_p(sun6i_hdmiaudio_pcm_remove),
	.driver = {
		.name 	= "sun6i-hdmiaudio-pcm-audio",
		.owner 	= THIS_MODULE,
	},
};


static int __init sun6i_soc_platform_hdmiaudio_init(void)
{
	int err = 0;

	if ((err = platform_device_register(&sun6i_hdmiaudio_pcm_device)) < 0) {
		return err;
	}

	if ((err = platform_driver_register(&sun6i_hdmiaudio_pcm_driver)) < 0) {
		return err;
	}

	return 0;
}
module_init(sun6i_soc_platform_hdmiaudio_init);

static void __exit sun6i_soc_platform_hdmiaudio_exit(void)
{
	return platform_driver_unregister(&sun6i_hdmiaudio_pcm_driver);
}
module_exit(sun6i_soc_platform_hdmiaudio_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUN6I HDMIAUDIO DMA module");
MODULE_LICENSE("GPL");
