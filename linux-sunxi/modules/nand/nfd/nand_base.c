#include "nand_blk.h"
#include "nand_dev.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*****************************************************************************/

extern struct nand_blk_ops mytr;
extern struct _nand_info* p_nand_info;
extern void NAND_Interrupt(__u32 nand_index);
extern __u32 NAND_GetCurrentCH(void);
extern int  init_blklayer(void);
extern void   exit_blklayer(void);
extern void set_cache_level(struct _nand_info*nand_info,unsigned short cache_level);

extern unsigned int flush_cache_num;

#define BLK_ERR_MSG_ON

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
#ifdef __LINUX_NAND_SUPPORT_INT__

spinlock_t     nand_int_lock;

static irqreturn_t nand_interrupt_ch0(int irq, void *dev_id)
{
    unsigned long iflags;
    __u32 nand_index;

    //nand_dbg_err("nand_interrupt_ch0!\n");
    spin_lock_irqsave(&nand_int_lock, iflags);

    nand_index = NAND_GetCurrentCH();
    if(nand_index!=0)
    {
        //nand_dbg_err(" ch %d int in ch0\n", nand_index);
    }
    else
    {
        NAND_Interrupt(nand_index);
    }
    spin_unlock_irqrestore(&nand_int_lock, iflags);

    return IRQ_HANDLED;
}

static irqreturn_t nand_interrupt_ch1(int irq, void *dev_id)
{
    unsigned long iflags;
    __u32 nand_index;

    //nand_dbg_err("nand_interrupt_ch1!\n");

    spin_lock_irqsave(&nand_int_lock, iflags);
    nand_index = NAND_GetCurrentCH();
    if(nand_index!=1)
    {
        //nand_dbg_err(" ch %d int in ch1\n", nand_index);
    }
    else
    {
        NAND_Interrupt(nand_index);
    }
    spin_unlock_irqrestore(&nand_int_lock, iflags);

    return IRQ_HANDLED;
}
#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_early_suspend(void)
{
    struct _nftl_blk *nftl_blk;
    struct nand_blk_ops *tr = &mytr;

    nftl_blk = tr->nftl_blk_head.nftl_blk_next;

    nand_dbg_err("nand_early_suspend\n");
    while(nftl_blk != NULL)
    {
        nand_dbg_err("nand\n");
        mutex_lock(nftl_blk->blk_lock);
        nftl_blk->flush_write_cache(nftl_blk,0xffff);
        mutex_unlock(nftl_blk->blk_lock);
        nftl_blk = nftl_blk->nftl_blk_next;
    }
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_early_resume(void)
{
    nand_dbg_err("nand_early_resume\n");
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_suspend(struct platform_device *plat_dev, pm_message_t state)
{
    if(NORMAL_STANDBY== standby_type)
    {
        nand_dbg_err("[NAND] nand_suspend normal\n");

        NandHwNormalStandby();
    }
    else if(SUPER_STANDBY == standby_type)
    {
        nand_dbg_err("[NAND] nand_suspend super\n");
        NandHwSuperStandby();
    }

    nand_dbg_err("[NAND] nand_suspend ok \n");
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_resume(struct platform_device *plat_dev)
{

    if(NORMAL_STANDBY== standby_type){
        nand_dbg_err("[NAND] nand_resume normal\n");
        NandHwNormalResume();
    }else if(SUPER_STANDBY == standby_type){
        nand_dbg_err("[NAND] nand_resume super\n");
        NandHwSuperResume();
    }

    nand_dbg_err("[NAND] nand_resume ok \n");
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_probe(struct platform_device *plat_dev)
{
    nand_dbg_inf("nand_probe\n");
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_remove(struct platform_device *plat_dev)
{
    nand_dbg_inf("nand_remove\n");
    return 0;
}

static int nand_release_dev(struct device *dev)
{
    nand_dbg_inf("nand_release dev\n");
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 shutdown_flush_write_cache(void)
{
    struct _nftl_blk *nftl_blk;
    struct nand_blk_ops *tr = &mytr;

    nftl_blk = tr->nftl_blk_head.nftl_blk_next;

    while(nftl_blk != NULL)
    {
        nand_dbg_err("shutdown_flush_write_cache\n");
        mutex_lock(nftl_blk->blk_lock);
        nftl_blk->flush_write_cache(nftl_blk,0xffff);
        nftl_blk = nftl_blk->nftl_blk_next;
        //mutex_unlock(nftl_blk->blk_lock);
    }
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nand_shutdown(struct platform_device *plat_dev)
{
    struct nand_blk_dev *dev;
    struct nand_blk_ops *tr = &mytr;

    nand_dbg_err("[NAND]shutdown first\n");
    list_for_each_entry(dev, &tr->devs, list){
        while(blk_fetch_request(dev->rq) != NULL){
            nand_dbg_err("nand_shutdown wait dev %d\n",dev->devnum);
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(HZ>>3);
        }
    }

    nand_dbg_err("[NAND]shutdown second\n");
    list_for_each_entry(dev, &tr->devs, list){
        while(blk_fetch_request(dev->rq) != NULL){
            nand_dbg_err("nand_shutdown wait dev %d\n",dev->devnum);
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(HZ>>3);
        }
    }

    shutdown_flush_write_cache();
    NandHwShutDown();
    nand_dbg_err("[NAND]shutdown end\n");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static struct platform_driver nand_driver = {
    .probe = nand_probe,
    .remove = nand_remove,
    .shutdown =  nand_shutdown,
    .suspend = nand_suspend,
    .resume = nand_resume,
    .driver = {
        .name = "sw_nand",
        .owner = THIS_MODULE,
    }
};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int __init nand_init(void)
{
    int ret;
    script_item_u   nand0_used_flag;
    script_item_u   nand_cache_level;
    script_item_u   nand_flush_cache_num;
    script_item_value_type_e  type;
    char * dev_name = "nand_dev";
    char * dev_id = "nand_id";

#ifdef __LINUX_NAND_SUPPORT_INT__
    unsigned long irqflags_ch0, irqflags_ch1;
#endif


    /* ��ȡcard_lineֵ */
    type = script_get_item("nand0_para", "nand0_used", &nand0_used_flag);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        nand_dbg_err("nand type err! %d",type);
    }
    nand_dbg_err("[NAND]nand init start, nand0_used_flag is %d\n", nand0_used_flag.val);

    nand_cache_level.val = 0;
    type = script_get_item("nand0_para", "nand_cache_level", &nand_cache_level);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        nand_dbg_err("nand_cache_level err! %d",type);
        nand_cache_level.val = 0;
    }
    
    nand_flush_cache_num.val = 8;
    type = script_get_item("nand0_para", "nand_flush_cache_num", &nand_flush_cache_num);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        nand_dbg_err("nand_flush_cache_num err! %d",type);
        nand_flush_cache_num.val = 8;
    }

    flush_cache_num = nand_flush_cache_num.val;

    //nand_dbg_err("flush_cache_num ! %d",flush_cache_num);

#ifdef __LINUX_NAND_SUPPORT_INT__
    //nand_dbg_err("[NAND] nand driver version: 0x%x 0x%x, support int! \n", NAND_VERSION_0,NAND_VERSION_1);

    spin_lock_init(&nand_int_lock);
    irqflags_ch0 = IRQF_DISABLED;
    irqflags_ch1 = IRQF_DISABLED;

    if (request_irq(AW_IRQ_NAND0, nand_interrupt_ch0, IRQF_DISABLED, dev_name, &dev_id))
    {
        nand_dbg_err("nand interrupte ch0 irqno: %d register error\n", AW_IRQ_NAND0);
        return -EAGAIN;
    }
    else
    {
        //nand_dbg_err("nand interrupte ch0 irqno: %d register ok\n", AW_IRQ_NAND0);
    }

    if (request_irq(AW_IRQ_NAND1, nand_interrupt_ch1, IRQF_DISABLED, dev_name, &dev_id))
    {
        nand_dbg_err("nand interrupte ch1, irqno: %d register error\n", AW_IRQ_NAND1);
        return -EAGAIN;
    }
    else
    {
        //nand_dbg_err("nand interrupte ch1, irqno: %d register ok\n", AW_IRQ_NAND1);
    }
#endif

    if(nand0_used_flag.val == 0)
    {
        nand_dbg_err("nand driver is disabled \n");
        return 0;
    }

    nand_dbg_err("nand init start\n");

    p_nand_info = NandHwInit();
    if(p_nand_info == NULL)
    {
        return -1;
    }

    set_cache_level(p_nand_info,nand_cache_level.val);

    ret = nand_info_init(p_nand_info,0,8,NULL);
    if(ret != 0)
    {
        return ret;
    }

    platform_driver_register(&nand_driver);

    init_blklayer();

    nand_dbg_err("nand init end \n");

//    //init sysfs
//    nand_dbg_err("init nand sysfs !\n");
//    if((ret = kobject_init_and_add(&kobj,&ktype,NULL,"nand_driver")) != 0 ) {
//      nand_dbg_err("init nand sysfs fail!\n");
//      return ret;
//  }

#ifdef CONFIG_HAS_EARLYSUSPEND
	nand_dbg_err("==register_early_suspend =\n");
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	early_suspend.suspend = nand_early_suspend;
	early_suspend.resume	= nand_early_resume;
	register_early_suspend(&early_suspend);
#endif

    return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void __exit nand_exit(void)
{
    script_item_u   nand0_used_flag;
    script_item_value_type_e  type;

    /* ��ȡcard_lineֵ */
    type = script_get_item("nand0_para", "nand0_used", &nand0_used_flag);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    nand_dbg_err("nand type err!");
    nand_dbg_err("nand0_used_flag is %d\n", nand0_used_flag.val);

    if(nand0_used_flag.val == 0)
    {
        nand_dbg_err("nand driver is disabled \n");
    }

    platform_driver_unregister(&nand_driver);

    exit_blklayer();

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif

//  kobject_del(&kobj);
//  kobject_put(&kobj);
}

//module_init(nand_init);
//module_exit(nand_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("nand flash groups");
MODULE_DESCRIPTION ("Generic NAND flash driver code");