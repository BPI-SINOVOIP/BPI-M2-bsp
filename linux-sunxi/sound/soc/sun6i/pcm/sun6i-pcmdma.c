/*
 * sound\soc\sun6i\pcm\sun6i-pcmdma.c
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

#include "sun6i-pcm.h"
#include "sun6i-pcmdma.h"

static unsigned int capture_dmadst = 0;
static unsigned int play_dmasrc = 0;

static const struct snd_pcm_hardware sun6i_pcm_play_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024,
	.period_bytes_max	= 1024*16,
	.periods_min		= 2,
	.periods_max		= 8,
	.fifo_size		= 128,
};

static const struct snd_pcm_hardware sun6i_pcm_capture_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024,
	.period_bytes_max	= 1024*16,
	.periods_min		= 2,
	.periods_max		= 8,
	.fifo_size		= 128,
};

struct sun6i_playback_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	dm_hdl_t	dma_hdl;
	bool 		play_dma_flag;
	struct dma_cb_t play_done_cb;
	struct sun6i_dma_params *params;	
};

struct sun6i_capture_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	dm_hdl_t	dma_hdl;
	bool		capture_dma_flag;
	struct dma_cb_t capture_done_cb;	
	struct sun6i_dma_params *params;
};

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

			if (play_prtd->play_dma_flag)
			{
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
			if (capture_prtd->capture_dma_flag)
			{
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

static u32 sun6i_audio_capture_buffdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)		                                  
{
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_substream *substream = NULL;

	if ((result == DMA_CB_ABORT) || (parg == NULL)) {
		return 0;
	}

	substream = parg;
	capture_prtd = substream->runtime->private_data;
	if ((substream) && (capture_prtd)) {
		snd_pcm_period_elapsed(substream);
	}	

	spin_lock(&capture_prtd->lock);
	{
		capture_prtd->dma_loaded--;
		sun6i_pcm_enqueue(substream);
	}
	spin_unlock(&capture_prtd->lock);	
	return 0;
}

static u32 sun6i_audio_play_buffdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)
{
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct snd_pcm_substream *substream = NULL;
	
	if ((result == DMA_CB_ABORT) || (parg == NULL)) {
		return 0;
	}

	substream = parg;
	play_prtd = substream->runtime->private_data;
	if ((substream) && (play_prtd)) {
		snd_pcm_period_elapsed(substream);
	}

	spin_lock(&play_prtd->lock);
	{
		play_prtd->dma_loaded--;
		sun6i_pcm_enqueue(substream);
	}
	spin_unlock(&play_prtd->lock);
	return 0;
}

static int sun6i_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *play_runtime = NULL, *capture_runtime = NULL;
    struct sun6i_playback_runtime_data *play_prtd = NULL;
    struct sun6i_capture_runtime_data *capture_prtd = NULL;
    struct snd_soc_pcm_runtime *play_rtd = NULL;
    struct snd_soc_pcm_runtime *capture_rtd = NULL;
    struct sun6i_dma_params *play_dma = NULL;
    struct sun6i_dma_params *capture_dma = NULL;
    unsigned long play_totbytes = 0, capture_totbytes = 0;
    
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
    	play_runtime = substream->runtime;
    	play_prtd = play_runtime ->private_data;
    	play_rtd = substream->private_data;
    	play_totbytes = params_buffer_bytes(params);
    	play_dma = snd_soc_dai_get_dma_data(play_rtd->cpu_dai, substream);
    	
    	if (!play_dma) {
    		return 0;
    	}

		if (play_prtd->params == NULL) {		
			play_prtd->params = play_dma;
		/*
		 * requeset audio dma handle(we don't care about the channel!)
		 */
		play_prtd->dma_hdl = sw_dma_request(play_prtd->params->name, DMA_WORK_MODE_SINGLE);
		if (NULL == play_prtd->dma_hdl) {
			printk(KERN_ERR "failed to request audio_play dma handle\n");
			return -EINVAL;
		}
		/*
	 	* set callback
	 	*/
		memset(&play_prtd->play_done_cb, 0, sizeof(play_prtd->play_done_cb));
		play_prtd->play_done_cb.func = sun6i_audio_play_buffdone;
		play_prtd->play_done_cb.parg = substream;
		/*use the full buffer callback, maybe we should use the half buffer callback?*/
		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_SET_QD_CB, (void *)&(play_prtd->play_done_cb))) {
			sw_dma_release(play_prtd->dma_hdl);
			return -EINVAL;
		}
		}
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
		play_runtime->dma_bytes = play_totbytes;
		
		spin_lock_irq(&play_prtd->lock);
		play_prtd->dma_loaded = 0;
		play_prtd->dma_limit = play_runtime->hw.periods_min;
		play_prtd->dma_period = params_period_bytes(params);
		play_prtd->dma_start = play_runtime->dma_addr;
		play_prtd->dma_pos = play_prtd->dma_start;
		play_prtd->dma_end = play_prtd->dma_start + play_totbytes;
		spin_unlock_irq(&play_prtd->lock);
    
    } else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
    	capture_runtime = substream->runtime;
    	capture_prtd = capture_runtime ->private_data;
    	capture_rtd = substream->private_data;
    	capture_totbytes = params_buffer_bytes(params);
    	capture_dma = snd_soc_dai_get_dma_data(capture_rtd->cpu_dai, substream);
    	
    	if (!capture_dma) {
    		return 0;
    	}
		if (capture_prtd->params == NULL) {		
			capture_prtd->params = capture_dma;
		
		/*
		 * requeset audio_capture dma handle(we don't care about the channel!)
		 */
		capture_prtd->dma_hdl = sw_dma_request(capture_prtd->params->name, DMA_WORK_MODE_SINGLE);
		if (NULL == capture_prtd->dma_hdl) {
			printk(KERN_ERR "failed to request audio_capture dma handle\n");
			return -EINVAL;
		}
		/*
	 	* set callback
	 	*/
		memset(&capture_prtd->capture_done_cb, 0, sizeof(capture_prtd->capture_done_cb));
		capture_prtd->capture_done_cb.func = sun6i_audio_capture_buffdone;
		capture_prtd->capture_done_cb.parg = substream;
		/*use the full buffer callback, maybe we should use the half buffer callback?*/
		if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_SET_QD_CB, (void *)&(capture_prtd->capture_done_cb))) {
			sw_dma_release(capture_prtd->dma_hdl);
			return -EINVAL;
		}
		}
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
		capture_runtime->dma_bytes = capture_totbytes;
		
		spin_lock_irq(&capture_prtd->lock);
		capture_prtd->dma_loaded = 0;
		capture_prtd->dma_limit = capture_runtime->hw.periods_min;
		capture_prtd->dma_period = params_period_bytes(params);
		capture_prtd->dma_start = capture_runtime->dma_addr;
		capture_prtd->dma_pos = capture_prtd->dma_start;
		capture_prtd->dma_end = capture_prtd->dma_start + capture_totbytes;
		spin_unlock_irq(&capture_prtd->lock);
		
    } else {
    	return -EINVAL;
    }
    
	return 0;
}

static int sun6i_pcm_hw_free(struct snd_pcm_substream *substream)
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
		}
   	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
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
		}
   	} else {
		return -EINVAL;
	}
	
	return 0;
}

static int sun6i_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct dma_config_t play_dma_config;
	struct dma_config_t capture_dma_config;
	
	int play_ret = 0, capture_ret = 0;
	
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		if (!play_prtd->params) {
			return 0;
		}

		memset(&play_dma_config, 0, sizeof(play_dma_config));
		play_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;//DMAXFER_D_BWORD_S_BWORD; //old ,DMAXFER_D_BHALF_S_BHALF  ,,,,DMAXFER_D_BHALF_S_BWORD
		play_dma_config.address_type = DMAADDRT_D_IO_S_LN;
		play_dma_config.para = 0;
		play_dma_config.irq_spt = CHAN_IRQ_QD;
		play_dma_config.src_addr = play_prtd->dma_start;
		play_dma_config.dst_addr = play_prtd->params->dma_addr;
		play_dma_config.byte_cnt = play_prtd->dma_period;
		play_dma_config.bconti_mode = false;
		play_dma_config.src_drq_type = DRQSRC_SDRAM;
		play_dma_config.dst_drq_type = DRQDST_DAUDIO_1_TX;

		if (0 != sw_dma_config(play_prtd->dma_hdl, &play_dma_config, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}
		play_prtd->dma_loaded = 0;
		play_prtd->dma_pos = play_prtd->dma_start;
		play_prtd->play_dma_flag = false;
		/* enqueue dma buffers */
		sun6i_pcm_enqueue(substream);

		return play_ret;
	} else {			
		capture_prtd = substream->runtime->private_data;
		
		if (!capture_prtd->params) {
			return 0;
		}
		
	   	memset(&capture_dma_config, 0, sizeof(capture_dma_config));
		capture_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;
		capture_dma_config.address_type = DMAADDRT_D_LN_S_IO;
		capture_dma_config.para = 0;
		capture_dma_config.irq_spt = CHAN_IRQ_QD;
		capture_dma_config.src_addr = capture_prtd->params->dma_addr;
		capture_dma_config.dst_addr = capture_prtd->dma_start;
		capture_dma_config.byte_cnt = capture_prtd->dma_period;
		capture_dma_config.bconti_mode = false;
		capture_dma_config.src_drq_type = DRQSRC_DAUDIO_1_RX;
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

static int sun6i_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		spin_lock(&play_prtd->lock);
			
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/*
			* start dma transfer
			*/
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("play %s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
			}
			//sw_dma_dump_chan(play_prtd->dma_hdl);
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/*
			 * stop play dma transfer
			 */
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
			}
			break;
		default:
			ret = -EINVAL;
			break;
		}
		spin_unlock(&play_prtd->lock);
	} else {
		capture_prtd = substream->runtime->private_data;
		spin_lock(&capture_prtd->lock);

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/*
			* start dma transfer
			*/

			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("%s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
			}	
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
			}
			
			break;
		default:
			ret = -EINVAL;
			break;
		}
		spin_unlock(&capture_prtd->lock);
	}
	return ret;
}

static snd_pcm_uframes_t sun6i_pcm_pointer(struct snd_pcm_substream *substream)
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

		if(0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_GET_CUR_SRC_ADDR, &play_dmasrc))
		{
			printk("%s,err\n", __func__);
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
    	
    	if(0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_GET_CUR_DST_ADDR, &capture_dmadst))
		{
			printk("%s,err\n", __func__);
		}
		
    	capture_res = capture_dmadst - capture_prtd->dma_start;
    	
    	spin_unlock(&capture_prtd->lock);
		
		if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
				capture_res = 0;
		}
		return bytes_to_frames(substream->runtime, capture_res);
    }
}

static int sun6i_pcm_open(struct snd_pcm_substream *substream)
{	
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;

		snd_pcm_hw_constraint_integer(play_runtime, SNDRV_PCM_HW_PARAM_PERIODS);
		snd_soc_set_runtime_hwparams(substream, &sun6i_pcm_play_hardware);
		play_prtd = kzalloc(sizeof(struct sun6i_playback_runtime_data), GFP_KERNEL);
	
		if (play_prtd == NULL) {
			return -ENOMEM;
		}
	
		spin_lock_init(&play_prtd->lock);
	
		play_runtime->private_data = play_prtd;
	} else {
		capture_runtime = substream->runtime;
		
		snd_pcm_hw_constraint_integer(capture_runtime, SNDRV_PCM_HW_PARAM_PERIODS);
		snd_soc_set_runtime_hwparams(substream, &sun6i_pcm_capture_hardware);
		
		capture_prtd = kzalloc(sizeof(struct sun6i_capture_runtime_data), GFP_KERNEL);
		if (capture_prtd == NULL)
			return -ENOMEM;
			
		spin_lock_init(&capture_prtd->lock);
		
		capture_runtime->private_data = capture_prtd;
	}

	return 0;
}

static int sun6i_pcm_close(struct snd_pcm_substream *substream)
{
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;
		play_prtd = play_runtime->private_data;
		
		kfree(play_prtd);		
	} else {
		capture_runtime = substream->runtime;
		capture_prtd = capture_runtime->private_data;
		
		kfree(capture_prtd);
	}
	
	return 0;
}

static int sun6i_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;
		
		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     play_runtime->dma_area,
					     play_runtime->dma_addr,
					     play_runtime->dma_bytes);
	} else {
		capture_runtime = substream->runtime;
		
		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     play_runtime->dma_area,
					     play_runtime->dma_addr,
					     play_runtime->dma_bytes);
	}

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
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = 0;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		size = sun6i_pcm_play_hardware.buffer_bytes_max;
	} else {
		size = sun6i_pcm_capture_hardware.buffer_bytes_max;
	}
	
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	return 0;
}

static void sun6i_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;
	
	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 sun6i_pcm_mask = DMA_BIT_MASK(32);

static int sun6i_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
		
	int ret = 0;
	
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sun6i_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = sun6i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = sun6i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 	out:
		return ret;
}

static struct snd_soc_platform_driver sun6i_soc_platform = {
	.ops		= &sun6i_pcm_ops,
	.pcm_new	= sun6i_pcm_new,
	.pcm_free	= sun6i_pcm_free_dma_buffers,
};

static int __devinit sun6i_pcm_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &sun6i_soc_platform);
}

static int __devexit sun6i_pcm_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sun6i_pcm_pcm_device = {
	.name = "sun6i-pcm-pcm-audio",
};

/*method relating*/
static struct platform_driver sun6i_pcm_pcm_driver = {
	.probe = sun6i_pcm_pcm_probe,
	.remove = __devexit_p(sun6i_pcm_pcm_remove),
	.driver = {
		.name = "sun6i-pcm-pcm-audio",
		.owner = THIS_MODULE,
	},
};

static int __init sun6i_soc_platform_pcm_init(void)
{
	int err = 0;	
	if((err = platform_device_register(&sun6i_pcm_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sun6i_pcm_pcm_driver)) < 0)
		return err;
	return 0;	
}
module_init(sun6i_soc_platform_pcm_init);

static void __exit sun6i_soc_platform_pcm_exit(void)
{
	return platform_driver_unregister(&sun6i_pcm_pcm_driver);
}
module_exit(sun6i_soc_platform_pcm_exit);

MODULE_AUTHOR("All winner");
MODULE_DESCRIPTION("SUN6I PCM DMA module");
MODULE_LICENSE("GPL");

