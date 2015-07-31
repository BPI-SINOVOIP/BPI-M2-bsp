#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/slab.h>
#include <linux/ctype.h>

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/dma-mapping.h>

#include <mach/dma.h>
#include "dma_class.h"
#define DMA_TEST_NUMBER 10
#define DMA_TEST_CHANNEL 16
//#define DMA_TEST_CHANNEL 15
#include <asm/cacheflush.h>

static struct class *sunxi_dma_test_class;	
static struct	dma_config_t dma_tia[DMA_TEST_CHANNEL];
struct test_result dma_test_result[DMA_TEST_NUMBER];
static unsigned int testcase_number;
#define SIZE_1M  (1024 * 1024)
#define SIZE_2M  (2 * 1024 * 1024)
#if 1
#define DEBUG_DMA_TEST_PANLONG
#define DMATEST_DEBUG(fmt...) printk(fmt)
#else
#define DMATEST_DEBUG(fmt...) do{}while(0)
#endif

static char *test_object[DMA_TEST_NUMBER]={"simple_test","signle_mode_test",
	"single_continue_test","single_enqs_test","app_cb_enqueue",
	"enq_aft_done","chain_all_channel","stop_from_running",
	"multichannel_to_one_destination","buffer_not_align",
};

static char	*channel_name[16]={"channel0","channel1","channel2","channel3",
				"channel4","channel5","channel6","channel7",
				"channel8","channel9","channel10","channel11",
				"channel12","channel13","channel14","channel15",
};

static unsigned char dma_tress_flag[DMA_TEST_CHANNEL];

static 	unsigned multichannel_to_one_destination_hd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[8].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x01);
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(*tmp_flag & 0x01))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[8].result=1;
	}
	return ret;
}

static 	unsigned multichannel_to_one_destination_fd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[8].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x02);
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		ret =0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(*tmp_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[8].result=1;
	}
	return ret;
}

static 	unsigned multichannel_to_one_destination_qd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[8].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x04);
		ret =0;
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(*tmp_flag & 0x04))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[8].result=1;
	}
	return ret;
}

static	int chain_all_channel_hd_flags;
static 	unsigned chain_all_channel_test_hd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[6].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x01);
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		chain_all_channel_hd_flags++;
		ret=0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! tmp_flag 0x%x %d\n",__FUNCTION__,*tmp_flag,__LINE__);
		if(!(*tmp_flag & 0x01))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[6].result=1;
	}
	return ret;
}

static	int chain_all_channel_fd_flags;
static 	unsigned chain_all_channel_test_fd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[6].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x02);
		chain_all_channel_fd_flags++;
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		ret=0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! tmp_flag 0x%x %d\n",__FUNCTION__,*tmp_flag,__LINE__);
		if(!(*tmp_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[6].result=1;
	}
	return ret;
}

static	int chain_all_channel_qd_flags;
static 	unsigned chain_all_channel_test_qd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	unsigned char *tmp_flag;
	int ret = 1;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[6].name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		DMATEST_DEBUG("before dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag &= (~0x04);
		DMATEST_DEBUG("after dipuse 0x%x %d\n",*tmp_flag,__LINE__);
		DMATEST_DEBUG("%s tmp_flag is 0x%x %d\n",__FUNCTION__,*tmp_flag,__LINE__);
		chain_all_channel_qd_flags++;
		ret=0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! tmp_flag 0x%x %d\n",__FUNCTION__,*tmp_flag,__LINE__);
		if(!(*tmp_flag & 0x04))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		dma_test_result[6].result=1;
	}
	return ret;
}
static	unsigned int enqueue_cb_test_flag;
static 	unsigned enqueue_cb_test_hd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret =1;
	u32	psrcu=0,pdstu=0;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		pdstu	= virt_to_phys(struct_result->vdstp);
		psrcu	= virt_to_phys(struct_result->vsrcp);
		DMATEST_DEBUG("enqueue_cb_test_flag is 0x%x\n",enqueue_cb_test_flag);
		if(enqueue_cb_test_flag & 0x01){
			if(sw_dma_enqueue(dma_hdl, psrcu+1024, pdstu+1024, 1024, ENQUE_PHASE_HD)){
				printk("sw_dma_enqueue fail in %s %d",__FUNCTION__,__LINE__);
			}else{
				struct_result->end_flag++;
				DMATEST_DEBUG("end_flag in %s is %d ,%d\n",__FUNCTION__,struct_result->end_flag,__LINE__);
				ret	= 0;
			}
			enqueue_cb_test_flag &= (~0x01);
		}else{
			ret =0;
		}
		struct_result->dma_qd_fd_hd_flag &= (~0x01);
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x01))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		struct_result->result=1;
	}

	return ret;
}

static 	unsigned enqueue_cb_test_fd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret =1;
	u32	psrcu=0,pdstu=0;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		if(struct_result->end_flag >=3){
			struct_result->end_flag++;
			printk("end_flag is %d\n",struct_result->end_flag);
		}

		pdstu	= virt_to_phys(struct_result->vdstp);
		psrcu	= virt_to_phys(struct_result->vsrcp);
		DMATEST_DEBUG("enqueue_cb_test_flag is 0x%x\n",enqueue_cb_test_flag);
		if(0x2 & enqueue_cb_test_flag){
			if(sw_dma_enqueue(dma_hdl, psrcu+1024*2, pdstu+1024*2, 1024, ENQUE_PHASE_FD)){
				printk("sw_dma_enqueue fail in %s %d\n",__FUNCTION__,__LINE__);
			}else{
				struct_result->end_flag++;
				DMATEST_DEBUG("end_flag in %s is %d ,%d\n",__FUNCTION__,struct_result->end_flag,__LINE__);
				ret	= 0;
			}
			enqueue_cb_test_flag &=  (~0x2);
		}else{
			ret =0;
		}
		struct_result->dma_qd_fd_hd_flag &= (~0x02);
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		struct_result->result=1;
	}

	return ret;
}

static 	unsigned enqueue_cb_test_qd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret =1;
	u32	psrcu=0,pdstu=0;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);
	DMATEST_DEBUG("struct_result->end_flag is %d %d\n",struct_result->end_flag,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		pdstu	= virt_to_phys(struct_result->vdstp);
		psrcu	= virt_to_phys(struct_result->vsrcp);
		DMATEST_DEBUG("enqueue_cb_test_flag is 0x%x\n",enqueue_cb_test_flag);
		if(0x4&enqueue_cb_test_flag){
			if(sw_dma_enqueue(dma_hdl, psrcu+1024*3, pdstu+1024*3, 1024, ENQUE_PHASE_QD)){
				printk("sw_dma_enqueue fail in %s %d\n",__FUNCTION__,__LINE__);
				ret =-1;
			}else{
				struct_result->end_flag++;
				DMATEST_DEBUG("end_flag in %s is %d ,%d\n",__FUNCTION__,struct_result->end_flag,__LINE__);
				ret = 0;
			}
		enqueue_cb_test_flag &= (~0x4);
		}else{
			ret =0;
		}
		struct_result->dma_qd_fd_hd_flag &= (~0x04);
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x04))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		struct_result->result=1;
	}

	return ret;
}

static	unsigned int single_mode_test_fd_cb(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	int *d_data,*s_data;
	void *tmps,*tmpd,*tmps_next,*tmpd_next;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		mdelay(10);
		d_data	= struct_result->vdstp;
		s_data	= struct_result->vsrcp;
		DMATEST_DEBUG("struct_result->end_flag  %d \n",struct_result->end_flag);
		DMATEST_DEBUG("d_data 0x%x d_data1 0x%x d_data1 0x%x %d\n",readl(d_data),readl(d_data+1),readl(d_data+2),__LINE__);
		DMATEST_DEBUG("s_data 0x%x s_data1 0x%x s_data2 0x%x %d\n",readl(s_data),readl(s_data+1),readl(s_data+2),__LINE__);
		tmpd		= d_data+struct_result->end_flag;
		tmps		= s_data+struct_result->end_flag;
		tmpd_next	= d_data+struct_result->end_flag+1;
		tmps_next	= s_data+struct_result->end_flag+1;
		DMATEST_DEBUG("tmpd 0x%x\ntmps 0x%x \ntmpd_next 0x%x\ntmps_next 0x%x  %d\n",(unsigned int)tmpd,(unsigned int)tmps,(unsigned int)tmpd_next,(unsigned int)tmps_next,__LINE__);

		if(memcmp(tmpd, tmps, 4) || !memcmp(tmpd_next, tmps_next, 4)){
			printk("dma tansmit err! time %d %d\n",struct_result->end_flag,__LINE__);
			struct_result->result =1;
		}
		struct_result->end_flag++;
		*(s_data+struct_result->end_flag) =0x11 * struct_result->end_flag;

		__cpuc_flush_dcache_area(struct_result->vsrcp, 1024 * 1024);
		__cpuc_flush_dcache_area(struct_result->vdstp, 1024 * 1024);
		DMATEST_DEBUG("struct_result->end_flag  %d \n",struct_result->end_flag);
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x %d\n",struct_result->dma_qd_fd_hd_flag,__LINE__);
		struct_result->dma_qd_fd_hd_flag &= (~0x02);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}

	if(ret){
		struct_result->result=1;
	}

	return ret;
}

static	unsigned int single_continue_test_fd_cb(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	int *d_data,*s_data;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);

	switch(cause){ 
	case DMA_CB_OK:
		d_data	= struct_result->vdstp;
		s_data	= struct_result->vsrcp;
		if(*d_data != *s_data){
			struct_result->result=1;
			ret=1;
			break;
		}
		DMATEST_DEBUG("before d_data 0x%x\ns_data 0x%x \n %d\n",readl(d_data),readl(s_data),__LINE__);
		*s_data	= ++struct_result->end_flag;
		DMATEST_DEBUG("after d_data 0x%x\ns_data 0x%x \n %d\n",readl(d_data),readl(s_data),__LINE__);	
		__cpuc_flush_dcache_area(struct_result->vsrcp, 1024 * 1024);
		__cpuc_flush_dcache_area(struct_result->vdstp, 1024 * 1024);
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x %d\n",struct_result->dma_qd_fd_hd_flag,__LINE__);
		struct_result->dma_qd_fd_hd_flag &= (~0x02);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT! %d\n",__FUNCTION__,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;
}
static	unsigned int simple_test_hd_cb(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s simple_test_hd_cb %d\n",struct_result->name,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		struct_result->dma_qd_fd_hd_flag &= (~0x01);
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x %d\n",struct_result->dma_qd_fd_hd_flag,__LINE__);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		DMATEST_DEBUG("%s DMA_CB_ABORT! dma_qd_fd_hd_flag 0x%x %d\n",__FUNCTION__,struct_result->dma_qd_fd_hd_flag,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x01))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions! %d\n",__FUNCTION__,__LINE__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;
}

static	unsigned int simple_test_fd_cb(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s simple_test_fd_cb %d\n",struct_result->name,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		struct_result->dma_qd_fd_hd_flag &= (~0x02);
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x %d\n",struct_result->dma_qd_fd_hd_flag,__LINE__);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT!\n",__FUNCTION__);
		DMATEST_DEBUG("%s DMA_CB_ABORT! dma_qd_fd_hd_flag 0x%x %d\n",__FUNCTION__,struct_result->dma_qd_fd_hd_flag,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions!\n",__FUNCTION__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;
}

static unsigned int enqueue_after_done_test_fd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		struct_result->dma_qd_fd_hd_flag &= (~0x02);
		struct_result->end_flag++;
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x end_flag %d %d\n",struct_result->dma_qd_fd_hd_flag,struct_result->end_flag,__LINE__);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT!\n",__FUNCTION__);
		DMATEST_DEBUG("%s DMA_CB_ABORT! dma_qd_fd_hd_flag 0x%x %d\n",__FUNCTION__,struct_result->dma_qd_fd_hd_flag,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x02))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions!\n",__FUNCTION__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;
}

static unsigned int enqueue_after_done_test_qd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s simple_test_qd_cb %d\n",struct_result->name,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		struct_result->dma_qd_fd_hd_flag &= (~0x04);
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x\n",struct_result->dma_qd_fd_hd_flag);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		printk("%s DMA_CB_ABORT!\n",__FUNCTION__);
		DMATEST_DEBUG("%s DMA_CB_ABORT! dma_qd_fd_hd_flag 0x%x %d\n",__FUNCTION__,struct_result->dma_qd_fd_hd_flag,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x04))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions!\n",__FUNCTION__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;

}

static unsigned	int	simple_test_qd_cb(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	int ret = 1;
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s simple_test_qd_cb %d\n",struct_result->name,__LINE__);
	switch(cause){ 
	case DMA_CB_OK:
		struct_result->dma_qd_fd_hd_flag &= (~0x04);
		struct_result->end_flag	= 1;
		DMATEST_DEBUG("dma_qd_fd_hd_flag 0x%x\n",struct_result->dma_qd_fd_hd_flag);
		ret = 0;
		break;
	case DMA_CB_ABORT:
		DMATEST_DEBUG("%s DMA_CB_ABORT! dma_qd_fd_hd_flag 0x%x %d\n",__FUNCTION__,struct_result->dma_qd_fd_hd_flag,__LINE__);
		if(!(struct_result->dma_qd_fd_hd_flag & 0x04))
			ret = 0;
		break;
	default:
		printk("%s whith unknown conditions!\n",__FUNCTION__);
		break;
	}
	if(ret){
		struct_result->result=1;
	}
	return ret;
}

unsigned int all_chain_test_cb_op(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	unsigned char *tmp_flag;
	tmp_flag	= parg;
	DMATEST_DEBUG("enter %s %s %d\n",dma_test_result[6].name,__FUNCTION__,__LINE__);
	switch(op){
	case DMA_OP_START:
		printk("%s DMA_OP_START! %d\n",__FUNCTION__,__LINE__);
		DMATEST_DEBUG("before dispute flag 0x%x %d\n",*tmp_flag,__LINE__);
		*tmp_flag	|= 0x07;
		DMATEST_DEBUG("after dispute flag 0x%x %d\n",*tmp_flag,__LINE__);
		break;
	case DMA_OP_STOP:
		printk("%s DMA_OP_STOP! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_HD_CB:
		printk("%s DMA_OP_SET_HD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_FD_CB:
		printk("%s DMA_OP_SET_FD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_QD_CB:
		printk("%s DMA_OP_SET_QD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_OP_CB:
		printk("%s DMA_OP_SET_OP_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	default:
		printk("%s ERR op\n! %d",__FUNCTION__,__LINE__);
		dma_test_result[6].result=1;
		break;
	}
	return 0;
}

unsigned int simple_test_cb_op(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	struct test_result *struct_result;
	struct_result=parg;
	DMATEST_DEBUG("enter %s %s %d\n",struct_result->name,__FUNCTION__,__LINE__);
	switch(op){
	case DMA_OP_START:
		printk("%s DMA_OP_START! %d\n",__FUNCTION__,__LINE__);
		struct_result->end_flag	= 0;
		break;
	case DMA_OP_STOP:
		printk("%s DMA_OP_STOP! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_HD_CB:
		printk("%s DMA_OP_SET_HD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_FD_CB:
		printk("%s DMA_OP_SET_FD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_QD_CB:
		printk("%s DMA_OP_SET_QD_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	case DMA_OP_SET_OP_CB:
		printk("%s DMA_OP_SET_OP_CB! %d\n",__FUNCTION__,__LINE__);
		break;
	default:
		printk("%s ERR op\n! %d",__FUNCTION__,__LINE__);
		struct_result->result=1;
		break;
	}
	return 0;
}

static int simple_test_func(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

#ifdef DEBUG_DMA_TEST_PANLONG
	unsigned int *test_value_s=NULL,*test_value_d=NULL;
#endif
	
	printk("simple_test start!\n");
	testcase_number=0;
	dma_test_result[0].dma_qd_fd_hd_flag=0x06;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[0].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_CHAIN);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",__FUNCTION__,__LINE__);
		ret = -1;
		goto ending;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending;
	}

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 1024 * 1024;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	done_cb.func = simple_test_fd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	DMATEST_DEBUG("1simple_test_flag is %d\n",dma_test_result[0].end_flag);
	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	 
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	DMATEST_DEBUG("2simple_test_flag is %d\n",dma_test_result[0].end_flag);
	i=1000;
	while((dma_test_result[0].end_flag < dma_test_result[0].loop_times) || dma_test_result[0].dma_qd_fd_hd_flag){
		if(i==0){
			ret = -1;
			printk("****************************\time out!\n*********************\n");
			break;
		}
		i--;
		mdelay(1);	
	}
	if(ret)
		goto ending;
	dma_test_result[0].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 1024 * 1024)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	dma_hdl = (dm_hdl_t)NULL;
	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);

	if(ret){
		ret = -1;
		printk("%s chain mode fail %d\n",dma_test_result[0].name,__LINE__);
		goto ending;
	}
	
	dma_test_result[0].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[0].dma_qd_fd_hd_flag);
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_SINGLE);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d",__FUNCTION__,__LINE__);
		ret = -1;
		goto ending;
	}

	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);	
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending;
	}
#ifdef DEBUG_DMA_TEST_PANLONG
	test_value_s=vsrcp;
	test_value_d=vdstp;
#endif
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));


	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	done_cb.func = simple_test_fd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[0];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	
	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[0].name,dma_test_result[0].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[0].name,dma_test_result[0].end_flag);
	i=1000;
	while(!dma_test_result[0].end_flag){
		if(i==0){
			ret = -1;
			break;
		}
		i--;
		mdelay(1);	
	}
	if(ret)
		goto ending;
	dma_test_result[0].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 1024 * 1024)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release  err! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}
	dma_hdl = (dm_hdl_t)NULL;
	

ending:
	if(ret || dma_test_result[0].dma_qd_fd_hd_flag){
		ret = -1;
		printk("simple_test fail! %d\n",__LINE__);
	}else{
		printk("simple_test success! %d\n",__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("simple_test dma handle STOP and release %d\n",__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("simple_test dma STOP fail! %d\n",__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("simple_test dma_hdl release fail! %d\n",__LINE__);
			ret = -1;
		}
	}
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));


	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));


	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int signle_mode_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[1].name);
	testcase_number=1;
	dma_test_result[1].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[1].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_SINGLE);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[1].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	dma_test_result[1].vdstp=vdstp;
	dma_test_result[1].vsrcp=vsrcp;
	memset(vsrcp,0x00,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);

	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 4;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
/*
	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[1];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
*/
	done_cb.func = single_mode_test_fd_cb;
	done_cb.parg = &dma_test_result[1];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[1];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[1];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+4, pdstu+4, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+8, pdstu+8, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[1].name,dma_test_result[1].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[1].name,dma_test_result[1].end_flag);
	i=1000;
	while(dma_test_result[1].end_flag<3){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}

	dma_test_result[1].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 4) || memcmp(vsrcp+4, vdstp+4, 4) || memcmp(vsrcp+8, vdstp+8, 4)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int single_continue_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0,*tmp_value;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[2].name);
	testcase_number=2;
	dma_test_result[2].dma_qd_fd_hd_flag=0x03;
	dma_test_result[2].loop_times=10;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[2].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_SINGLE);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[2].name,__LINE__);
		ret = -1;
		goto ending2;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending2;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending2;
	}

	memset(vsrcp,0x11,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 4;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = true;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	dma_test_result[2].vsrcp	= vsrcp;
	dma_test_result[2].vdstp	= vdstp;
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
/*
	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[2];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}
*/
	done_cb.func = single_continue_test_fd_cb;
	done_cb.parg = &dma_test_result[2];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[2];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[2];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}
/*
	if(sw_dma_enqueue(dma_hdl, psrcu+4, pdstu+4, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending2;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+8, pdstu+8, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending2;
	}
*/
	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1single_continue_test_flag is %d\n",dma_test_result[2].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}
	DMATEST_DEBUG("2single_continue_test_flag is %d\n",dma_test_result[2].end_flag);
	i=1000;
	while(dma_test_result[2].end_flag < dma_test_result[2].loop_times){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending2;
		}
		i--;
		mdelay(1);	
	}

	dma_test_result[2].end_flag = 0;
	tmp_value=vdstp;
	if((*tmp_value < 9)){
		printk("data check fail! %d\n",__LINE__);
		printk("tmp_value %d\n",*tmp_value);
		ret = -1;
		goto ending2;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}
	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}
	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending2;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending2:
	if(ret){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("single_continue_test dma_hdl release fail! %d\n",__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int chain_enqueques_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[3].name);
	testcase_number=3;
	dma_test_result[3].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[3].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_CHAIN);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[3].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 1024;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[3];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_fd_cb;
	done_cb.parg = &dma_test_result[3];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[3];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[3];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+1028, pdstu+1028, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+1030, pdstu+1030, 1, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[3].name,dma_test_result[3].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[3].name,dma_test_result[3].end_flag);
	i=1000;
	while(!dma_test_result[3].end_flag){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}

	dma_test_result[3].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 1024) || memcmp(vsrcp+1028, vdstp+1028, 4) || memcmp(vsrcp+1030, vdstp+1030, 1) || (!memcmp(vsrcp, vdstp, 1031))){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret || dma_test_result[3].dma_qd_fd_hd_flag){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int enqueue_cb_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	DMATEST_DEBUG("%s start %d\n",dma_test_result[4].name,__LINE__);
	testcase_number=4;
	enqueue_cb_test_flag = 0x07;
	dma_test_result[4].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[4].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_SINGLE);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[4].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 1024;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = enqueue_cb_test_qd;
	done_cb.parg = &dma_test_result[4];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = enqueue_cb_test_fd;
	done_cb.parg = &dma_test_result[4];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = enqueue_cb_test_hd;
	done_cb.parg = &dma_test_result[4];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[4];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
/*
	if(sw_dma_enqueue(dma_hdl, psrcu+1028, pdstu+1028, 4, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+1030, pdstu+1030, 1, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}
*/
	dma_test_result[4].vdstp	= vdstp;
	dma_test_result[4].vsrcp	= vsrcp;
	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[4].name,dma_test_result[4].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[4].name,dma_test_result[4].end_flag);
	i=1000;
	while(dma_test_result[4].end_flag < 6){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}

//	dma_test_result[4].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 1024)){
		printk("data0 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data0 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024, vdstp+1024, 1024)){
		printk("data1 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data1 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024*2, vdstp+1024*2, 1024)){
		printk("data2 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data2 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024*3, vdstp+1024*3, 1024)){
		printk("data3 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data3 check ok! %d\n",__LINE__);
	}

	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret || dma_test_result[4].dma_qd_fd_hd_flag){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int enqueue_after_done_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[5].name);
	testcase_number=5;
	dma_test_result[5].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[5].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_SINGLE);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[5].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 1024;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = enqueue_after_done_test_qd;
	done_cb.parg = &dma_test_result[5];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = enqueue_after_done_test_fd;
	done_cb.parg = &dma_test_result[5];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[5];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[5];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[5].name,dma_test_result[5].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[5].name,dma_test_result[5].end_flag);
	i=1000;
	while(!dma_test_result[5].end_flag){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}

	sw_dma_enqueue(dma_hdl, psrcu+1024, pdstu+1024, 1024, ENQUE_PHASE_NORMAL);
	
	sw_dma_enqueue(dma_hdl, psrcu+1024*2, pdstu+1024*2, 1024, ENQUE_PHASE_NORMAL);
	
	sw_dma_enqueue(dma_hdl, psrcu+1024*3, pdstu+1024*3, 1024, ENQUE_PHASE_NORMAL);

	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	dma_test_result[5].end_flag=0;
	i=1000;
	while(dma_test_result[5].end_flag<3){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}

	dma_test_result[5].end_flag = 0;
	if(memcmp(vsrcp, vdstp, 1024)){
		printk("data0 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data0 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024, vdstp+1024, 1024)){
		printk("data1 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data1 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024*2, vdstp+1024*2, 1024)){
		printk("data2 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data2 check ok! %d\n",__LINE__);
	}

	if(memcmp(vsrcp+1024*3, vdstp+1024*3, 1024)){
		printk("data3 check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data3 check ok! %d\n",__LINE__);
	}




	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret || dma_test_result[5].dma_qd_fd_hd_flag){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int chain_all_channel_test(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,j,ret=0,available_channel=10;
	unsigned char tmp_flag;
	dm_hdl_t	dma_hdl[DMA_TEST_CHANNEL];
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;
#ifdef DEBUG_DMA_TEST_PANLONG
	unsigned int *test_value_s=NULL,*test_value_d=NULL;
#endif
	
	printk("%s start! %d\n",dma_test_result[6].name,__LINE__);
	testcase_number=6;
	for(i=0;i<DMA_TEST_CHANNEL;i++){
		dma_hdl[i]=(dm_hdl_t)NULL;
		dma_hdl[i] = sw_dma_request(channel_name[i], DMA_WORK_MODE_SINGLE);
		if(!dma_hdl[i]){
			printk("dma_hdl[%d] sw_dma_request fail! %d\n",i,__LINE__);
			available_channel=i-1;
			break;
		}else{
		dma_tress_flag[i] = (i << 4);
		}
	}
	DMATEST_DEBUG("available_channel is %d %d\n",available_channel,__LINE__);
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending;
	}
#ifdef DEBUG_DMA_TEST_PANLONG
	test_value_d=vdstp;
	test_value_s=vsrcp;
#endif
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));

	for(i=0;i<available_channel;i++){
		DMATEST_DEBUG("source 0x%x des 0x%x %d %d\n",psrcu + i*1024,pdstu + i*1024,i,__LINE__);
		dma_tia[i].src_addr	= psrcu+i*1024;
		dma_tia[i].dst_addr	= pdstu+i*1024;
		dma_tia[i].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
		dma_tia[i].byte_cnt	= 1024;
		dma_tia[i].xfer_type= DMAXFER_D_BWORD_S_BWORD;
		dma_tia[i].address_type = DMAADDRT_D_LN_S_LN;
		dma_tia[i].para = 0;
		dma_tia[i].bconti_mode = false;
		dma_tia[i].src_drq_type = DRQSRC_SDRAM;
		dma_tia[i].dst_drq_type = DRQDST_SDRAM;
	
		memset(&done_cb, 0, sizeof(done_cb));
		memset(&op_cb, 0, sizeof(op_cb));

		done_cb.func = chain_all_channel_test_qd;
		done_cb.parg = &dma_tress_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_QD_CB, (void *)&done_cb)) {
			printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending;
		}

		done_cb.func = chain_all_channel_test_fd;
		done_cb.parg = &dma_tress_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_FD_CB, (void *)&done_cb)){
			printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending;
		}

		done_cb.func = chain_all_channel_test_hd;
		done_cb.parg = &dma_tress_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_HD_CB, (void *)&done_cb)){
			printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending;
		}

		op_cb.func = all_chain_test_cb_op;
		op_cb.parg = &dma_tress_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_OP_CB, (void *)&op_cb)){
			printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
			ret = -1;
			goto ending;
		}

		if(sw_dma_config(dma_hdl[i], &dma_tia[i], ENQUE_PHASE_NORMAL)){
			printk("sw_dma_config err %d\n",__LINE__);
			ret = -1;
			goto ending;
		}
	}
	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);

	for(i=0;i<available_channel;i++){
		DMATEST_DEBUG("dma_tress_flag[%d] is 0x%2x before start %d\n",i,dma_tress_flag[i],__LINE__);
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_START, NULL)) {
			printk("sw_dma_ctl DMA_OP_START err NO%d %d\n",i,__LINE__);
			ret = -1;
			goto ending;
		}
		DMATEST_DEBUG("dma_tress_flag[%d] is 0x%2x after start %d\n",i,dma_tress_flag[i],__LINE__);
	}
	
	i=2000;
	DMATEST_DEBUG("loop start ! %d\n",__LINE__);
	while(i){
		tmp_flag =	0;
		for(j=0;j<available_channel;j++){
			tmp_flag |= dma_tress_flag[j];
		}
		if(!(tmp_flag & 0x0f))
			goto transmit_over;
		i--;
		mdelay(1);	
	}
	printk("%s time out ! tmp_flag 0x%x %d\n",__FUNCTION__,tmp_flag,__LINE__);
	dma_test_result[6].result	= -1;
	ret= -1;
	goto ending;

transmit_over:

	if(memcmp(vsrcp, vdstp, 1024*available_channel)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	for(i=0;i<available_channel;i++){
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_STOP, NULL)) {
			printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
			ret = -1;
			goto ending;
		}

		if(sw_dma_release(dma_hdl[i])){
			printk("sw_dma_release err! %d\n",__LINE__);
			ret = -1;
			goto ending;
		}
		dma_hdl[i] = (dm_hdl_t)NULL;
	}
ending:
	if(ret){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	for(i=0;i<DMA_TEST_CHANNEL;i++){
		if(dma_hdl[i]){
			DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
			if(sw_dma_ctl(dma_hdl[i], DMA_OP_STOP, NULL)){
				printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
				ret = -1;
			}
			if(sw_dma_release(dma_hdl[i])){
				printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
				ret = -1;
			}
		}
	}
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));


	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	DMATEST_DEBUG("source 0x%x 0x%x 0x%x 0x%x\n",*test_value_s,*(test_value_s+1),*(test_value_s+2),*(test_value_s+3));
	DMATEST_DEBUG("destination 0x%x 0x%x 0x%x 0x%x\n",*test_value_d,*(test_value_d+1),*(test_value_d+2),*(test_value_d+3));


	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int stop_from_running(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int ret=0;
	dm_hdl_t	dma_hdl;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[7].name);
	testcase_number=7;

	dma_test_result[7].dma_qd_fd_hd_flag=0x07;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[7].dma_qd_fd_hd_flag);

	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_CHAIN);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[7].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_2M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_2M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	get_random_bytes(vsrcp,SIZE_2M);
	memset(vdstp,0xff,SIZE_2M);

	dma_tia[0].src_addr	= psrcu;
	dma_tia[0].dst_addr	= pdstu;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= SIZE_2M;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));
#if 0
	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[7];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_fd_cb;
	done_cb.parg = &dma_test_result[7];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[7];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
#endif
	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[7];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	__cpuc_flush_dcache_area(vsrcp, SIZE_2M);
	__cpuc_flush_dcache_area(vdstp, SIZE_2M);

	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[7].name,dma_test_result[7].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[7].name,dma_test_result[7].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	if(dma_test_result[7].end_flag){
		printk("data have transmit over! %d\n",__LINE__);
		ret= -1;
		goto ending1;
	}

	if(!memcmp(vsrcp, vdstp, SIZE_2M)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int multichannel_to_one_destination(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,j,multichannel_channel_number=2,ret=0;
	unsigned char tmp_end_flag[multichannel_channel_number],tmp_calc_flag=0;
	dm_hdl_t	dma_hdl[multichannel_channel_number];
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[8].name);
	testcase_number=8;

	for(i=0;i<multichannel_channel_number;i++){
		tmp_end_flag[i]=0x07 |( i << 4);
		DMATEST_DEBUG("%s sw_dma_request %d\n",channel_name[i],__LINE__);
		dma_hdl[i] = sw_dma_request(channel_name[i], DMA_WORK_MODE_CHAIN);
		if(!dma_hdl[i]){
			printk("%s sw_dma_request fail! %d %d\n",dma_test_result[8].name,i,__LINE__);
			ret = -1;
			goto ending1;
		}
	}

	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("mem set over %d\n",__LINE__);
	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	for(i=0;i<multichannel_channel_number;i++){
		dma_tia[i].src_addr	= psrcu + i*1024;
		dma_tia[i].dst_addr	= pdstu;
		dma_tia[i].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
		dma_tia[i].byte_cnt	= SIZE_1M;
		dma_tia[i].xfer_type= DMAXFER_D_BWORD_S_BWORD;
		dma_tia[i].address_type = DMAADDRT_D_LN_S_LN;
		dma_tia[i].para = 0;
		dma_tia[i].bconti_mode = false;
		dma_tia[i].src_drq_type = DRQSRC_SDRAM;
		dma_tia[i].dst_drq_type = DRQDST_SDRAM;
	}
	DMATEST_DEBUG("dma tia set over! %d\n",__LINE__);
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	for(i=0;i<multichannel_channel_number;i++){
		done_cb.func = multichannel_to_one_destination_qd;
		done_cb.parg = &tmp_end_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_QD_CB, (void *)&done_cb)) {
			printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}

		done_cb.func = multichannel_to_one_destination_fd;
		done_cb.parg = &tmp_end_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_FD_CB, (void *)&done_cb)){
			printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}

		done_cb.func = multichannel_to_one_destination_hd;
		done_cb.parg = &tmp_end_flag[i];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_HD_CB, (void *)&done_cb)){
			printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}

		op_cb.func = simple_test_cb_op;
		op_cb.parg = &dma_test_result[8];
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_SET_OP_CB, (void *)&op_cb)){
			printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}


		if(sw_dma_config(dma_hdl[i], &dma_tia[i], ENQUE_PHASE_NORMAL)){
			printk("sw_dma_config err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}
	}
	DMATEST_DEBUG("dma set over! %d\n",__LINE__);
	__cpuc_flush_dcache_area(vsrcp, SIZE_1M);
	__cpuc_flush_dcache_area(vdstp, SIZE_1M);
	DMATEST_DEBUG("dcache flash over! %d\n",__LINE__);	
	for(i=0;i<multichannel_channel_number;i++){
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_START, NULL)) {
			printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}
	}
	i=1000;
	DMATEST_DEBUG("tmp_calc_flag 0x%x %d\n",tmp_calc_flag,__LINE__);
	DMATEST_DEBUG("loop start! multichannel_channel_number %d %d\n",multichannel_channel_number,__LINE__);
	while(1){
		if(!i){
			printk("%s timeout %d\n",__FUNCTION__,__LINE__);
			ret = -1;
			goto ending1;
		}
		tmp_calc_flag=0;
		for(j=0;j<multichannel_channel_number;j++)
			tmp_calc_flag |= tmp_end_flag[j];

		if(!(tmp_calc_flag & 0x0f)){
			DMATEST_DEBUG("loot out with ok! %d\n",__LINE__);
			break;
		}
		i--;
		mdelay(1);
	}
	tmp_calc_flag=0;

	for(i=0;i<multichannel_channel_number;i++){
		if(sw_dma_ctl(dma_hdl[i], DMA_OP_STOP, NULL)) {
			printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}
	

		if(sw_dma_release(dma_hdl[i])){
			printk("sw_dma_release err! %d\n",__LINE__);
			ret = -1;
			goto ending1;
		}
	dma_hdl[i] = (dm_hdl_t)NULL;
	}
ending1:
	if(ret){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	for(i=0;i<multichannel_channel_number;i++){
		if(dma_hdl[i]){
			DMATEST_DEBUG("%s dma handle STOP and release %d %d\n",__FUNCTION__,i,__LINE__);
			if(sw_dma_ctl(dma_hdl[i], DMA_OP_STOP, NULL)){
				printk("%s dma STOP fail! %d %d\n",__FUNCTION__,i,__LINE__);
			}
			if(sw_dma_release(dma_hdl[i])){
				printk("%s dma_hdl release fail! %d %d\n",__FUNCTION__,i,__LINE__);
			}
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}

static int buffer_not_align(struct sunxi_dma_test_class *sunxi_dma_test)
{
	int i,ret=0;
	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	void	*vsrcp=NULL,*vdstp=NULL;
	u32		psrcu=0,pdstu=0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	printk("%s start!\n",dma_test_result[9].name);
	dma_test_result[9].dma_qd_fd_hd_flag=0x07;
	testcase_number=9;
	DMATEST_DEBUG("%d dma_qd_qd_hd_flag 0x%x\n",__LINE__,dma_test_result[9].dma_qd_fd_hd_flag);
	sunxi_dma_test->channel_number = 1;
	dma_hdl = sw_dma_request(channel_name[0], DMA_WORK_MODE_CHAIN);
	if(!dma_hdl){
		printk("%s sw_dma_request fail! %d\n",dma_test_result[9].name,__LINE__);
		ret = -1;
		goto ending1;
	}
	vsrcp	= kmalloc(SIZE_1M, GFP_KERNEL);
	psrcu	= virt_to_phys(vsrcp);
	//vsrcp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&psrcu, GFP_KERNEL);
	if(!vsrcp || !psrcu){
		printk("vsrcp: 0x%x psrcu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vsrcp,psrcu,__LINE__);
		ret = -1;
		goto ending1;
	}

	vdstp	= kmalloc(SIZE_1M, GFP_KERNEL);
	pdstu	= virt_to_phys(vdstp);
	//vdstp = dma_alloc_coherent(NULL, SIZE_1M, (dma_addr_t *)&pdstu, GFP_KERNEL);
	if(!vdstp || !pdstu ){
		printk("vdstp: 0x%x pdstu: 0x%x\ndma_alloc_coherent err! %d\n",(unsigned int)vdstp,pdstu,__LINE__);
		ret = -1;
		goto ending1;
	}

	get_random_bytes(vsrcp,SIZE_1M);
	memset(vdstp,0xff,SIZE_1M);
	dma_tia[0].src_addr	= psrcu + 1;
	dma_tia[0].dst_addr	= pdstu + 3;
	dma_tia[0].irq_spt	= CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	dma_tia[0].byte_cnt	= 1024;
	dma_tia[0].xfer_type= DMAXFER_D_BWORD_S_BWORD;
	dma_tia[0].address_type = DMAADDRT_D_LN_S_LN;
	dma_tia[0].para = 0;
	dma_tia[0].bconti_mode = false;
	dma_tia[0].src_drq_type = DRQSRC_SDRAM;
	dma_tia[0].dst_drq_type = DRQDST_SDRAM;

	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = simple_test_qd_cb;
	done_cb.parg = &dma_test_result[9];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		printk("sw_dma_ctl DMA_OP_SET_QD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_fd_cb;
	done_cb.parg = &dma_test_result[9];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_FD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	done_cb.func = simple_test_hd_cb;
	done_cb.parg = &dma_test_result[9];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)){
		printk("sw_dma_ctl DMA_OP_SET_HD_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	op_cb.func = simple_test_cb_op;
	op_cb.parg = &dma_test_result[9];
	if(sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)){
		printk("sw_dma_ctl DMA_OP_SET_OP_CB err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}


	if(sw_dma_config(dma_hdl, &dma_tia[0], ENQUE_PHASE_NORMAL)){
		printk("sw_dma_config err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+2048+3, pdstu+2048+2, 1024, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	if(sw_dma_enqueue(dma_hdl, psrcu+4096+2, pdstu+4096+1, 1024, ENQUE_PHASE_NORMAL)){
		ret = -1;
		printk("sw_dma_enqueue err %d\n",__LINE__);
		goto ending1;
	}

	__cpuc_flush_dcache_area(vsrcp, 1024 * 1024);
	__cpuc_flush_dcache_area(vdstp, 1024 * 1024);
	DMATEST_DEBUG("1%s flag is %d\n",dma_test_result[9].name,dma_test_result[9].end_flag);
	if(sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		printk("sw_dma_ctl DMA_OP_START err %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	DMATEST_DEBUG("3%s flag is %d\n",dma_test_result[9].name,dma_test_result[9].end_flag);
	i=1000;
	while(!dma_test_result[9].end_flag){
		if(i==0){
			ret = -1;
			printk("%s time out !%d\n",__FUNCTION__,__LINE__);
			goto ending1;
		}
		i--;
		mdelay(1);	
	}
	DMATEST_DEBUG("2%s flag is %d\n",dma_test_result[9].name,dma_test_result[9].end_flag);
	dma_test_result[9].end_flag = 0;
/*
	if(memcmp(vsrcp + 1, vdstp + 3, 1024) || memcmp(vsrcp+2048+3, vdstp+2048+2, 1024) || memcmp(vsrcp+4096+2, vdstp+4096+1, 1024)){
		printk("data check fail! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}else{
		printk("data check ok! %d\n",__LINE__);
	}
*/
	if(memcmp(vsrcp + 1, vdstp + 3, 1024)){
		printk("first buffer err! %d\n",__LINE__);
	}else{
		printk("first buffer OK! %d\n",__LINE__);
	}
	if(memcmp(vsrcp + 2048 + 3, vdstp + 2048+2, 1024)){
		printk("second buffer err! %d\n",__LINE__);
	}else{
		printk("second buffer OK! %d\n",__LINE__);
	}
	if(memcmp(vsrcp + 4096+2, vdstp + 4096+1, 1024)){
		printk("third buffer err! %d\n",__LINE__);
	}else{
		printk("third buffer OK! %d\n",__LINE__);
	}




	if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		printk("sw_dma_ctl DMA_OP_STOP err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}

	if(sw_dma_release(dma_hdl)){
		printk("sw_dma_release err! %d\n",__LINE__);
		ret = -1;
		goto ending1;
	}
	dma_hdl = (dm_hdl_t)NULL;

ending1:
	if(ret || dma_test_result[9].dma_qd_fd_hd_flag){
		ret = -1;
		printk("%s fail! %d\n",__FUNCTION__,__LINE__);
	}else{
		printk("%s success! %d\n",__FUNCTION__,__LINE__);
	}

	if(dma_hdl){
		DMATEST_DEBUG("%s dma handle STOP and release %d\n",__FUNCTION__,__LINE__);
		if(sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)){
			printk("%s dma STOP fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
		if(sw_dma_release(dma_hdl)){
			printk("%s dma_hdl release fail! %d\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}

	if(vdstp)
		kfree(vdstp);
	if(vsrcp)
		kfree(vsrcp);
	printk("%s over!\nret is %d %d\n",__FUNCTION__,ret,__LINE__);
	return ret ;
}
/*
simple_test			0
signle_mode_test	1
single_continue_test	2
single_enqs_test	3
app_cb_enqueue		4
enq_aft_done		5
chain_all_channel	6
stop_from_running	7
multichannel_to_one_destination	8
buffer_not_align	9
*/
static ssize_t exec_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int final=0;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		switch(exec_number){
		case 0:
			final = simple_test_func(sunxi_dma_test);
		break;
		case 1:	
			final = signle_mode_test(sunxi_dma_test);
		break;
		case 2:		
			final = single_continue_test(sunxi_dma_test);
		break;
		case 3:
			final = chain_enqueques_test(sunxi_dma_test);
		break;
		case 4:
			final = enqueue_cb_test(sunxi_dma_test);
		break;
		case 5:
			final = enqueue_after_done_test(sunxi_dma_test);
		break;
		case 6:
			final = chain_all_channel_test(sunxi_dma_test);
		break;
		case 7:
			final = stop_from_running(sunxi_dma_test);
		break;
		case 8:
			final = multichannel_to_one_destination(sunxi_dma_test);
		break;
		case 9:
			final = buffer_not_align(sunxi_dma_test);
		break;
		default :
			printk("you input number too large!\n");
			final = 1;
		break;
		}	
	}
	if(final)
		dma_test_result[exec_number].result=1;
	else{
		if( dma_test_result[exec_number].result==2 )
			dma_test_result[exec_number].result = 0;
	}
	return ret;
}

static ssize_t exec_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",sunxi_dma_test->exec);
}

static ssize_t update_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",sunxi_dma_test->update);
}

static ssize_t channel_no_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",sunxi_dma_test->channel_no);
}

static ssize_t channel_number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",sunxi_dma_test->channel_number);
}

static ssize_t address_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].address_type);
}

static ssize_t xfer_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].xfer_type);
}

static ssize_t para_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].para);
}

static ssize_t src_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].src_addr);
}

static ssize_t dst_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].dst_addr);
}

static ssize_t byte_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].byte_cnt);
}

static ssize_t bconti_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].bconti_mode);
}

static ssize_t src_drq_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].src_drq_type);
}

static ssize_t dst_drq_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	return sprintf(buf,"%u\n",dma_tia[sunxi_dma_test->channel_no].dst_drq_type);
}

static ssize_t testcase_number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", testcase_number);
}

static ssize_t testcase_number_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;
	char *after;
	int number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		testcase_number	= number;
	}

	return ret;
}

static ssize_t result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u",dma_test_result[testcase_number].result);
}

static ssize_t result_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	/*reserve a point which point to sunxi_gpio_test_class*/
//	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	int i;

	for(i=0;i<DMA_TEST_NUMBER;i++){
		if(dma_test_result[i].result==1){
			printk("%s: fail!\n",dma_test_result[i].name);
		}else if(dma_test_result[i].result==2){
			printk("%s: have not test!\n",dma_test_result[i].name);
		}else{
			printk("%s: pass!\n",dma_test_result[i].name);
		}
	}

	return size;
}

static ssize_t update_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t channel_no_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t channel_number_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t address_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t xfer_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t para_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t src_addr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t dst_addr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}


static ssize_t byte_cnt_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t bconti_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t src_drq_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static ssize_t dst_drq_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
/*
	struct sunxi_dma_test_class *sunxi_dma_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int exec_number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size)
		ret = count;
	printk("implement sooner or later\n");
	return ret;
*/
	return size;
}

static struct device_attribute dma_class_attrs[] = {
	__ATTR(exec, 0644, exec_show, exec_store),
	__ATTR(update, 0644, update_show, update_store),
	__ATTR(channel_no, 0644, channel_no_show, channel_no_store),
	__ATTR(channel_number, 0644, channel_number_show, channel_number_store),
	__ATTR(address_type, 0644,address_type_show,address_type_store),
	__ATTR(xfer_type, 0644, xfer_type_show, xfer_type_store),
	__ATTR(para, 0644,para_show,para_store),
	__ATTR(src_addr, 0644,src_addr_show,src_addr_store),
	__ATTR(dst_addr, 0644,dst_addr_show,dst_addr_store),
	__ATTR(byte_cnt, 0644,byte_cnt_show,byte_cnt_store),
	__ATTR(bconti_mode, 0644,bconti_mode_show,bconti_mode_store),
	__ATTR(src_drq_type, 0644,src_drq_type_show,src_drq_type_store),
	__ATTR(dst_drq_type, 0644,dst_drq_type_show,dst_drq_type_store),
	__ATTR(result, 0644,result_show,result_store),
	__ATTR(testcase_number,0644,testcase_number_show,testcase_number_store),
	__ATTR_NULL,
};

static int __devexit sunxi_dma_test_remove(struct platform_device *dev)
{
    struct sunxi_dma_test_class *sunxi_dma_test    = platform_get_drvdata(dev);

	device_unregister(sunxi_dma_test->dev);
	kfree(sunxi_dma_test);

	return 0;
}

static int __devinit sunxi_dma_test_probe(struct platform_device *dev)
{
	struct sunxi_dma_test_class *sunxi_dma_test;
	int i;

	sunxi_dma_test = kzalloc(sizeof(struct sunxi_dma_test_class), GFP_KERNEL);
	if (sunxi_dma_test == NULL) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}
	platform_set_drvdata(dev, sunxi_dma_test);
	sunxi_dma_test->dev = device_create(sunxi_dma_test_class, &dev->dev, 0, sunxi_dma_test,"sunxi_dma_test");

	if (IS_ERR(sunxi_dma_test->dev))
		return PTR_ERR(sunxi_dma_test->dev);

	for(i=0;i<DMA_TEST_NUMBER;i++){
		dma_test_result[i].name=test_object[i];
		dma_test_result[i].result=2;
	}
	return 0;
}

static void dma_release (struct device *dev)
{
	printk("dma_release ok!\n");
}

static struct platform_driver sunxi_dma_test_driver = {
	.probe		= sunxi_dma_test_probe,
	.remove		= sunxi_dma_test_remove,
	.driver		= {
		.name		= "sunxi_dma_test",
		.owner		= THIS_MODULE,
	},
};

static struct platform_device sunxi_dma_test_devices = {
    .name = "sunxi_dma_test",
    .id = 0,
    .dev = {
        .release = dma_release,
    },
};

static int __init sunxi_dma_test_init(void)
{
	int ret;
	sunxi_dma_test_class = class_create(THIS_MODULE, "sunxi_dma_test_class");
	if (IS_ERR(sunxi_dma_test_class))
		return PTR_ERR(sunxi_dma_test_class);

	ret	= platform_driver_register(&sunxi_dma_test_driver);
	if(ret < 0)
		goto err_dma_platform_driver_register;
		
	sunxi_dma_test_class->dev_attrs 	= dma_class_attrs;
	mutex_init(&sunxi_dma_test_mutex);

		ret	= platform_device_register(&sunxi_dma_test_devices);
	if(ret < 0)
		goto err_sunxi_dma_test_platform_device_register;

	return ret;

err_sunxi_dma_test_platform_device_register:
	platform_driver_unregister(&sunxi_dma_test_driver);
	mutex_destroy(&sunxi_dma_test_mutex);
err_dma_platform_driver_register:
	return ret;	
}

static void __exit sunxi_dma_test_exit(void)
{
	mutex_destroy(&sunxi_dma_test_mutex);
	platform_device_unregister(&sunxi_dma_test_devices);
	platform_driver_unregister(&sunxi_dma_test_driver);
	class_destroy(sunxi_dma_test_class);
}
module_init(sunxi_dma_test_init);
module_exit(sunxi_dma_test_exit);
MODULE_AUTHOR("panlong");
MODULE_DESCRIPTION("so ugly driver for company`s task, so lose face");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio_dma_test");
