/*
**********************************************************************************************************************
*                                                    Test 
*                                          software test for drivers
*                                              Test Sub-System
*
*                                   (c) Copyright 2007-2010, Grace.Miao China
*                                             All Rights Reserved
*
* Moudle  : test driver
* File    : nand_test.c
* Purpose : test driver of nand driver in Linux 
*
* By      : Grace Miao
* Version : v1.0
* Date    : 2011-3-16
* history : 
             2011-3-16     build the file     Grace Miao
**********************************************************************************************************************
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/scatterlist.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>



#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>

#include <asm/uaccess.h>
#include <linux/timer.h>

#include <linux/mutex.h>
#include <mach/clock.h>
#include <mach/dma.h>
#include <linux/wait.h>

#include <asm/cacheflush.h>
#include <linux/pm.h>

#include <mach/sys_config.h>



#include "../../../nand/nfd/nand_blk.h"
//#include "../lib/src/include/mbr.h"

#include "../../../nand/nfd/nand_lib.h"			//nand驱动编译成库时，用此头文件代替 

#include "nand_test.h"

//#ifdef CONFIG_SUN4I_NANDFLASH_TEST     //  open nand test module


typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

//for CLK
extern int NAND_ClkRequest(__u32 nand_index);
extern void NAND_ClkRelease(__u32 nand_index);
extern int NAND_SetClk(__u32 nand_index, __u32 nand_clk);
extern int NAND_GetClk(__u32 nand_index);

//for DMA
extern int NAND_RequestDMA(void);
extern int NAND_ReleaseDMA(void);
extern void NAND_DMAConfigStart(int rw, unsigned int buff_addr, int len);
extern int NAND_QueryDmaStat(void);
extern int NAND_WaitDmaFinish(void);
//for PIO
extern void NAND_PIORequest(__u32 nand_index);
extern void NAND_PIORelease(__u32 nand_index);

//for Int
extern void NAND_EnRbInt(void);
extern void NAND_ClearRbInt(void);
extern int NAND_WaitRbReady(void);
extern void NAND_EnDMAInt(void);
extern void NAND_ClearDMAInt(void);
extern void NAND_DMAInterrupt(void);

extern void NAND_Interrupt(__u32 nand_index);

extern __u32 NAND_GetIOBaseAddrCH0(void);
extern __u32 NAND_GetIOBaseAddrCH1(void);

spinlock_t     nand_test_int_lock;    


static struct nand_blk_ops nand_mytr = {
	.name 			=  "nand",
	.major 			= 93,
	.minorbits 		= 3,
	.owner 			= THIS_MODULE,
};

#ifdef __LINUX_NAND_SUPPORT_INT__	   
    static irqreturn_t nand_test_interrupt_ch0(int irq, void *dev_id)
    {
        unsigned long iflags;
	__u32 nand_index;

	//printk("nand_interrupt_ch0!\n");
	
	spin_lock_irqsave(&nand_test_int_lock, iflags);

	nand_index = NAND_GetCurrentCH();
	if(nand_index!=0)
	{
		//printk(" ch %d int in ch0\n", nand_index);
	}
	else
	{
		NAND_Interrupt(nand_index);
	}
	
        spin_unlock_irqrestore(&nand_test_int_lock, iflags);
    
    	return IRQ_HANDLED;
    }

    static irqreturn_t nand_test_interrupt_ch1(int irq, void *dev_id)
    {
        unsigned long iflags;
    	__u32 nand_index;

	//printk("nand_interrupt_ch1!\n");
	
        spin_lock_irqsave(&nand_test_int_lock, iflags);
        nand_index = NAND_GetCurrentCH();
	if(nand_index!=1)
	{
		//printk(" ch %d int in ch1\n", nand_index);
	}
	else
	{
		NAND_Interrupt(nand_index);
	}
        spin_unlock_irqrestore(&nand_test_int_lock, iflags);
    
    	return IRQ_HANDLED;
    }
#endif

/*
typedef enum
{
	FALSE = 0,	
	TRUE  = 1,	
	false = 0,	
	true  = 1
}BOOL,bool;
*/
#ifndef	NULL
#define NULL	0
#endif


#define NAND_TEST  "[nand_test]:"

// for unused
#define	DiskSize		128

#define RESULT_OK   (0)
#define RESULT_FAIL   (1)

#define MAX_SECTORS       (0x80)         // max alloc buffer
#define BUFFER_SIZE       (512*MAX_SECTORS)

static DEFINE_MUTEX(nand_test_lock);


static ssize_t nand_test_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count);
static ssize_t nand_test_show(struct kobject *kobject,struct attribute *attr, char *buf);

void obj_test_release(struct kobject *kobject);



struct nand_test_card {
    u8    *buffer;
    u8    *scratch;
    unsigned int sector_cnt;
};

struct nand_test_case {
    const char *name;
    int  sectors_cnt;
    int (*prepare)(struct nand_test_card *, int sectors_cnt);
    int (*run)(struct nand_test_card * );
    int (*cleanup)(struct nand_test_card *);
};

struct attribute prompt_attr = {
    .name = "nand_test",
    .mode = S_IRWXUGO
};

static struct attribute *def_attrs[] = {
    &prompt_attr,
    NULL
};


struct sysfs_ops obj_test_sysops =
{
    .show =  nand_test_show,
    .store = nand_test_store
};

struct kobj_type ktype = 
{
    .release = obj_test_release,
    .sysfs_ops=&obj_test_sysops,
    .default_attrs=def_attrs
};

void obj_test_release(struct kobject *kobject)
{
    printk(NAND_TEST "release .\n");
}

struct kobject kobj;




#ifdef		PREV_NAND_TEST_
/* prepare buffer data for read and write*/
static int __nand_test_prepare(struct nand_test_card *test, int sector_cnt,int write)
{
    int i;

    test->sector_cnt = sector_cnt;    

    if (write){
        memset(test->buffer, 0xDF, 512 * (sector_cnt) +4);
    }
    else {
        for (i = 0; i < 512 * (sector_cnt) + 4; i++){
            test->buffer[i] = i%256;
         }
    }

    return 0;
}


static int nand_test_prepare_write(struct nand_test_card *test, int sector_cnt)
{
    return __nand_test_prepare(test, sector_cnt, 1);
}

static int nand_test_prepare_read(struct nand_test_card *test, int sector_cnt)
{
    return __nand_test_prepare(test, sector_cnt, 0);
}


static int nand_test_prepare_pwm(struct nand_test_card *test, int sector_cnt)
{
    test->sector_cnt = sector_cnt; 
    return 0;
}


/* read /write one sector with out verification*/
static int nand_test_simple_transfer(struct nand_test_card *test,
                                    unsigned dev_addr,unsigned start, 
                                    unsigned nsector, int write)
{
    int ret;
    if (write){
      
#ifndef NAND_CACHE_RW
        ret = LML_Write(start, nsector, test->buffer + dev_addr); 
#else
        //printk("Ws %lu %lu \n",start, nsector);
        ret = NAND_CacheWrite(start, nsector, test->buffer + dev_addr);
#endif     
        if(ret){
            return -EIO;
        }
        return 0;
        }
    else { 
      
#ifndef NAND_CACHE_RW
        LML_FlushPageCache();
        ret = LML_Read(start, nsector, test->buffer + dev_addr);
#else
        //printk("Rs %lu %lu \n",start, nsector);
        LML_FlushPageCache();
        ret = NAND_CacheRead(start, nsector, test->buffer + dev_addr);
#endif                                                  // read
        
        if (ret){
            return -EIO;  
        } 
        return 0;
    }
}


/* read /write one or more sectors with verification*/
static int nand_test_transfer(struct nand_test_card *test,
                              unsigned dev_addr,unsigned start, 
                              unsigned nsector, int write)
{
    int ret;
    int i;
    
    if (!write){
        ret = nand_test_simple_transfer(test, 0, start, nsector, 1);  // write to sectors for read
        if (ret){
            return ret;
        }
        memset(test->buffer, 0, nsector * 512 +4);    // clean mem for read
    }
  
    if ( ( ret = nand_test_simple_transfer(test, dev_addr, start, nsector, write ) ) ) {   // read or write
        return ret;
    }
    if(write){     
        memset(test->buffer, 0, nsector * 512 + 4);    // clean mem for read
        ret = nand_test_simple_transfer(test, 0 , start, nsector, 0);   // read 
        if (ret){
            return ret;
        }
        for(i  = 0; i < nsector * 512; i++){    // verify data
            if (test->buffer[i] != 0xDF){
                printk(KERN_INFO "[nand_test] Ttest->buffer[i] = %d, i = %d, dev_addr = %d, nsector = %d\n", test->buffer[i],  i, dev_addr,nsector);
                return RESULT_FAIL;
            }
        } 
    }
    else {   //read 
        for(i  = 0 + dev_addr; i < nsector * 512 + dev_addr ; i++){   // verify data
    
            if (test->buffer[i] != (i-dev_addr)%256){
                printk(KERN_INFO "[nand_test] Ttest->buffer[i] = %d, i = %d, dev_addr = %d, nsector = %d\n", test->buffer[i],  i, dev_addr,nsector);
                return RESULT_FAIL;
            }
       }
    }
    return RESULT_OK;
}


  
/* write one sector without verification*/
static int nand_test_single_write(struct nand_test_card *test)
{
    int ret;


    ret = nand_test_simple_transfer(test, 0, 0, test->sector_cnt,1);
    
    if(ret){
        return ret;
    }
    return nand_test_simple_transfer(test, 0, DiskSize/2, test->sector_cnt, 1 );
   
}

/* read one sector without verification*/
static int nand_test_single_read(struct nand_test_card *test)
{
    int ret;

    ret = nand_test_simple_transfer(test, 0, 0, test->sector_cnt,0);
    if(ret){
        return ret;
    }
    return nand_test_simple_transfer(test, 0, DiskSize/2, test->sector_cnt, 0);
}

/* write one  sector with verification */
static int nand_test_verify_write(struct nand_test_card *test)
{
    return nand_test_transfer(test,  0, 1, test->sector_cnt, 1);
}

/* read one  sector with verification */
static int nand_test_verify_read(struct nand_test_card *test)
{
    return nand_test_transfer(test,  0, 1, test->sector_cnt, 0);
}

/* write multi sector with start sector num 5*/
static int nand_test_multi_write(struct nand_test_card *test)
{
    return nand_test_transfer(test,  0, 5, test->sector_cnt, 1);
}


/* write multi sector with start sector num 29*/

static int nand_test_multi_read(struct nand_test_card *test)
{
    return nand_test_transfer(test,  0, 29, test->sector_cnt, 0);
}

/* write from buffer+1, buffer+2, and buffer+3, where buffer is  4  bytes algin */
static int nand_test_align_write(struct nand_test_card *test)
{
    int ret;
    int i;
  
    for (i = 1;i < 4;i++) {
        ret = nand_test_transfer(test,  i, 1, test->sector_cnt, 1);
    }
    return ret;
}


/* read to buffer+1, buffer+2, and buffer+3, where buffer is  4  bytes algin */

static int nand_test_align_read(struct nand_test_card *test)
{
    int ret;
    int i;
  
    for (i = 1;i < 4;i++) {
        ret = nand_test_transfer(test,  i, 1, test->sector_cnt, 0);
    }
    if (ret){
        return ret;
    }
  
    return 0;
}

/* write to incorrect sector num such as -1, DiskSize,  DiskSize +1 */
static int nand_test_negstart_write(struct nand_test_card *test)
{
    int ret;
    
    /* start + sectnum > 0, start < 0*/
    ret = nand_test_simple_transfer(test,  0, -5 , 11, 1);
    
    if (!ret){
        return RESULT_FAIL;
    }
    printk(NAND_TEST "start + sectnum > 0 pass\n");

    /* start + sectnum < 0 , start < 0 */
    ret = nand_test_simple_transfer(test,  0, -62, 5, 1);
    
    if (!ret){
        return RESULT_FAIL;
    }
    return RESULT_OK;

}


/* read from negative sector num   start + sectnum > 0, and start + setnum < 0 */
static int nand_test_negstart_read(struct nand_test_card *test)
{
    int ret;

    /* start + sectnum > 0, start < 0*/
    ret = nand_test_simple_transfer(test,  0, -1, 3, 0);
    
    if (!ret){
        return RESULT_FAIL;
    }
    printk(NAND_TEST "start + sectnum > 0 pass\n");

    /* start + sectnum < 0 , start < 0 */
    ret = nand_test_simple_transfer(test,  0, -90, 15, 0);
    
    if (!ret){
        return RESULT_FAIL;
    }
    return RESULT_OK;
  
}

static int nand_test_beyond(struct nand_test_card *test, int write)
{
    int ret;


    ret = nand_test_simple_transfer(test,  0, DiskSize -3 , 5, write);
    
    if (!ret){ 
        
        return 1;
    }
    printk(NAND_TEST "DiskSize -3 , 5 pass\n");
    ret = nand_test_simple_transfer(test,  0, DiskSize -1 , 2, write);
    
    if (!ret){ 
        
        return 1;
    }
    printk(NAND_TEST "DiskSize -1 , 2 pass\n");
    ret = nand_test_simple_transfer(test,  0, DiskSize , 3, write);
    
    if (!ret){ 
        
        return 1;
    }

    printk(NAND_TEST "DiskSize , 3 pass\n");
    
    ret = nand_test_simple_transfer(test,  0, DiskSize + 3 , 0, write);
    
    if (!ret){ 
        
        return 1;
    }

    printk(NAND_TEST "DiskSize + 3 , 0 pass\n");

    ret = nand_test_simple_transfer(test,  0, DiskSize - 3 , -2, write);
    
    if (!ret){ 
        
        return 1;
    }

    printk(NAND_TEST "DiskSize - 3 , -2 pass\n");
    
    return RESULT_OK;
}


static int nand_test_beyond_write(struct nand_test_card *test)
{   
    return (nand_test_beyond(test, 1));
}


/* read from incorrect sector num such as -1, DiskSize(max sector num + 1),  DiskSize +1 */
static int nand_test_beyond_read(struct nand_test_card *test)
{
    
    return (nand_test_beyond(test, 0));
  
}



/* write all sectors from sector num 0 to DiskSize - 1(max sector num )*/
static int nand_test_write_all_ascending(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    int j = 0;

    printk(KERN_INFO "DiskSize = %x\n", DiskSize);
   
  
    for (i = 0; i < DiskSize; i++) {   // write all sectors  
        ret = nand_test_simple_transfer(test, 0, i, test->sector_cnt,1);
        if(ret){ 
            printk(KERN_INFO "nand_test_write_all_ascending fail, sector num %d\n", i);
            return ret;
        }
    }
   
    /* start check */
    printk(KERN_INFO "[nand test]:start check\n");
   
    for (i = 0; i < DiskSize; i++){
        memset(test->buffer, 0, test->sector_cnt * 512);  // clear buffer
        
        ret = nand_test_simple_transfer(test, 0 , i, test->sector_cnt, 0);   // read 
        if(ret){
            return ret;
        }
        
        for(j  = 0; j < test->sector_cnt * 512; j++)  {  // verify
            if (test->buffer[j] != 0xDF){
                printk(KERN_INFO "nand_test_write_all_ascending, Ttest->buffer[j] = %d, i = %d\n", test->buffer[j],  i);
                return RESULT_FAIL;
            }
        }
   
    }
    return RESULT_OK;
}


/* read all sectors from sector num 0 to DiskSize - 1(max sector num )*/
static int nand_test_read_all_ascending(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    int j = 0;

    /*  before reading, write */
    for (i = 0; i < DiskSize; i++) {
      
        ret = nand_test_simple_transfer(test, 0, i, test->sector_cnt,1);  // write all sectors
        if(ret){
            printk(KERN_INFO "nand_test_read_all_ascending fail, sector num %d\n", i);
            return ret;
        }
    }
   
   /* finish write,  start to read and check */
    for (i = 0; i < DiskSize; i++)
    {
        if (i%100000 == 0){
            printk(KERN_INFO "[nand test]: sector num:%d\n", i);
        }
        
        memset(test->buffer, 0, test->sector_cnt * 512);  // clear buffer
        
        ret = nand_test_simple_transfer(test, 0 , i, test->sector_cnt, 0);   // read 
        if(ret){
            return ret;
        }
        for(j  = 0 ; j < test->sector_cnt * 512  ; j++){
            if (test->buffer[j] != (j)%256){
                printk(KERN_INFO "nand_test_read_all_ascending fial! Ttest->buffer[j] = %d, i = %d\n", test->buffer[i],  j);
                return RESULT_FAIL;
            }
      
       }
    } 
    return RESULT_OK;
}


/* write all sectors from sector num  DiskSize - 1(max sector num ) to  0  */
static int nand_test_write_all_descending(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    int j = 0;

    printk(KERN_INFO "nand_test: DiskSize = %x\n", DiskSize);
   
    for (i = DiskSize - 1; i >= 0; i--){
      
        memset(test->buffer, i%256, 512);
        
        if (i%100000 == 0){
            printk(KERN_INFO "[nand test]: sector num:%d\n", i);
        }
        
        ret = nand_test_simple_transfer(test, 0, i, test->sector_cnt,1);  // write all sectors
        if(ret){
            printk(KERN_INFO "[nand_test]: nand_test_write_all_ascending fail, sector num %d\n", i);
            return ret;
        }
   }
   
   printk(KERN_INFO "[nand test]: check start\n");
   
   for (i = DiskSize - 1; i >= 0; i--){

       if (i%100000 == 0){
           printk(KERN_INFO "[nand test]: sector num:%d\n", i);
       }
       
       memset(test->buffer, 0, test->sector_cnt * 512);  // clear buffer
       
       ret = nand_test_simple_transfer(test, 0 , i, test->sector_cnt, 0);   // read 
       if(ret){
           return ret;
       }
       for(j  = 0; j < 512; j++){  // verify
            if (test->buffer[j] != i%256){
                printk(KERN_INFO "[nand_test]: nand_test_write_all_ascending, Ttest->buffer[j] = %d, i = %d\n", test->buffer[j],  i);
                return RESULT_FAIL;
            }
        }   
    }
    return RESULT_OK;
}

/* read all sectors from sector num  DiskSize - 1(max sector num ) to  0  */
static int nand_test_read_all_descending(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    int j = 0;
   
    for (i = DiskSize - 1; i >= 0; i--){
        memset(test->buffer, i%256, 512);
        
        ret = nand_test_simple_transfer(test, 0, i, test->sector_cnt,1);  // write all sectors
        if(ret){
            printk(KERN_INFO "[nand_test]: nand_test_read_all_ascending fail, sector num %d\n", i);
            return ret;
        }
    }
   
    printk(KERN_INFO "[nand test]: check start\n");
    for (i = DiskSize - 1; i >= 0; i--){
        if (i%100000 == 0){
            printk(KERN_INFO "[nand test]: sector num:%d\n", i);
        }
        memset(test->buffer, 0, test->sector_cnt * 512);  // clear buffer
        ret = nand_test_simple_transfer(test, 0 , i, test->sector_cnt, 0);   // read 
        if(ret){
            return ret;
        }
        for(j = 0 ; j < test->sector_cnt * 512  ; j++){      // verify data
            if (test->buffer[j] != (i)%256){
                printk(KERN_INFO "[nand_test]:nand_test_read_all_ascending fial! Ttest->buffer[j] = %d, i = %d\n", test->buffer[j],  i);
                return RESULT_FAIL;
            }
        }
    } 
    return RESULT_OK;
}

/* write a sector for  n times  without verification to test stability */
static int nand_test_repeat_single_write(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
   
    printk(NAND_TEST "DiskSize = %d\n", DiskSize);
    
    for(i = 0; i < REPEAT_TIMES*1000; i++){
       ret = nand_test_simple_transfer(test, 0 , DiskSize/7, test->sector_cnt, 1);
       if(ret){
           return ret;
       }
    }
    return 0;
}


/* read a sector for  n times with verification to test stability*/
static int nand_test_repeat_single_read(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    for(i = 0; i < REPEAT_TIMES*30; i++){
        ret = nand_test_simple_transfer(test, 0 , DiskSize/4 + 7, test->sector_cnt, 0);
        if(ret){
            return ret;
        }
   }
   return 0;
}

/* write multi sectors for  n times without verification to test stability*/
static int nand_test_repeat_multi_write(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    
    for(i = 0; i < 1100000; i++){
        ret = nand_test_simple_transfer(test, 0 , DiskSize/2, test->sector_cnt, 1);
        if(ret) {
            return ret;
        }
    }
    
    return 0;
}


/* read multi sectors for  n times without verification to test stability*/
static int nand_test_repeat_multi_read(struct nand_test_card *test)
{
    int ret; 
    int i = 0;
    for(i = 0; i < 9200000; i++){
        ret = nand_test_simple_transfer(test, 0 , DiskSize/3, test->sector_cnt, 0);
        if(ret){
            return ret;
       }
    }
    return 0;
}

/* random write one or more sectors*/
static int nand_test_random_write(struct nand_test_card *test)
{
    int ret; 
  
    ret = nand_test_simple_transfer(test, 0 , 0, test->sector_cnt, 1);
    if(ret){
        return ret;
    }
    
    ret = nand_test_simple_transfer(test, 0 , DiskSize -1, test->sector_cnt, 1);
    if(ret) {
        return ret;
    }
   
    ret = nand_test_simple_transfer(test, 0 , DiskSize/2, test->sector_cnt, 1);
    if(ret){
        return ret;
    }
    return 0;
}

/* random read one or more sectors*/
static int nand_test_random_read(struct nand_test_card *test)
{
    int ret; 
  
    ret = nand_test_simple_transfer(test, 0 , 0, test->sector_cnt, 0);
    if(ret) {
        return ret;
    }
   
    ret = nand_test_simple_transfer(test, 0 , DiskSize -1, test->sector_cnt, 0);
    if(ret){
        return ret;
    }
   
    ret = nand_test_simple_transfer(test, 0 , DiskSize/2, test->sector_cnt, 0);
    if(ret){
        return ret;
    }
   
    return 0;
}


/* clear r/w buffer to 0*/
static int nand_test_cleanup(struct nand_test_card *test)
{
    memset(test->buffer, 0, 512* (test->sector_cnt));
    return 0;
}


/* test cases */

static const struct nand_test_case nand_test_cases[] = {
    {
	    .name = "single sector write (no data verification)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_single_write,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "single sector read (no data verification)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_single_read,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "single sector write(verify data)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_verify_write,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "single sector read(verify data)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_verify_read,
	    .cleanup = nand_test_cleanup
    },
  
    /* multi read/write*/
    {
	    .name = "multi sector read(2 sectors, verify)",
	    .sectors_cnt = 2,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_multi_read,
	    .cleanup = nand_test_cleanup
    },

    {
	    .name = "multi sector read(3 sectors, verify)",
	    .sectors_cnt = 3,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_multi_read,
	    .cleanup = nand_test_cleanup
    },

    {
	    .name = "multi sector read(8 sectors, verify)",
	    .sectors_cnt = 8,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_multi_read,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "multi sector read(18 sectors, verify)",
	    .sectors_cnt = 18,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_multi_read,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "multi sector read(53 sectors, verify)",
	    .sectors_cnt = 53,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_multi_read,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "multi sector write(2 sectors ,verify)",
	    .sectors_cnt = 2,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup
    },
  
    {
	    .name = "multi sector write(5 sectors ,verify)",
	    .sectors_cnt = 5,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup,
    },
  
    {
	    .name = "multi sector write(12 sectors ,verify)",
	    .sectors_cnt = 12,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup,
    },
  
    {
	    .name = "multi sector write(15 sectors ,verify)",
	    .sectors_cnt = 15,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup,
    },

    {
	    .name = "multi sector write(26 sectors ,verify)",
	    .sectors_cnt = 26,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup,
    },

    {
	    .name = "multi sector write(93 sectors ,verify)",
	    .sectors_cnt = 93,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_multi_write,
	    .cleanup = nand_test_cleanup,
    },
    
    /*align test*/
    {
	    .name = "align write(1 sector ,verify)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_align_write,
	    .cleanup = nand_test_cleanup,
    },
    
    {
	    .name = "align read(1 sector ,verify)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_align_read,
	    .cleanup = nand_test_cleanup,
    },
    
    /* stability test */
    {
	    .name = "weird write(negative start)",   // 18
	    .sectors_cnt = 10,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_negstart_write,
	    .cleanup = nand_test_cleanup,
    },
  
    {
	    .name = "weid read(nagative satrt)",
	    .sectors_cnt = 10,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_negstart_read,
	    .cleanup = nand_test_cleanup,
    },

    {
	    .name = "weird write(beyond start)",   // 20
	    .sectors_cnt = 10,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_beyond_write,
	    .cleanup = nand_test_cleanup,
    },
  
    {
	    .name = "weid read(bayond start)",
	    .sectors_cnt = 10,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_beyond_read,
	    .cleanup = nand_test_cleanup,
    },

    {                                            // 22
	    .name = "write all ascending",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_write_all_ascending,
	    .cleanup = nand_test_cleanup,
    },

    {
	    .name = "read all ascending",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_read_all_ascending,
	    .cleanup = nand_test_cleanup,
    },
  
    {
	    .name = "write all descending",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_write_all_descending,
	     .cleanup = nand_test_cleanup,
    },
       

    {
	    .name = "read all descending",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_read_all_descending,
	    .cleanup = nand_test_cleanup,
    },
   
    {                                                     // 26    
	    .name = " repeat  write (no data verification) ",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_repeat_single_write,
	    .cleanup = nand_test_cleanup,
    },
  
    {                                                    // 27   
	    .name = " repeat  read (no data verification) ",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_repeat_single_read,
	    .cleanup = nand_test_cleanup,
   },

   {                                                     // 28  
	    .name = " repeat multi write (no data verification)",
	    .sectors_cnt = 43,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_repeat_multi_write,
	    .cleanup = nand_test_cleanup,
   },
   
   {                                                    // 29    
	    .name = " repeat multi read (no data verification)",
	    .sectors_cnt = 81,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_repeat_multi_read,
	    .cleanup = nand_test_cleanup,
    },

    {                                                    // 30   
	    .name = " random  write (no data verification)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_write,
	    .run = nand_test_random_write,
	    .cleanup = nand_test_cleanup,
    },
    
    {                                                    // 31   
	    .name = " random  read (no data verification)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_read,
	    .run = nand_test_random_read,
	    .cleanup = nand_test_cleanup,
    },
    
    {                                                    // 32 
	    .name = " pwm  test (no data verification)",
	    .sectors_cnt = 1,
	    .prepare = nand_test_prepare_pwm,
	    //.run = nand_test_pwm,
	    .cleanup = nand_test_cleanup,
    },
};


/* run test cases*/
static void nand_test_run(struct nand_test_card *test, int testcase)
{
    int i, ret;
  
    //printk(KERN_INFO "[nand_test]: Starting tests of nand\n");
  
    for (i = 0;i < ARRAY_SIZE(nand_test_cases);i++) 
	{
        if (testcase && ((i + 1) != testcase))
		{		
            continue;
    	}
  
        printk(KERN_INFO "[nand_test]: Test case %d. %s...\n", i + 1, nand_test_cases[i].name);
    
        if (nand_test_cases[i].prepare) {
              ret = nand_test_cases[i].prepare(test, nand_test_cases[i].sectors_cnt);
          if (ret) {
              printk(KERN_INFO "[nand_test]: Result: Prepare stage failed! (%d)\n", ret);
              continue;
          }
        }
  
        ret = nand_test_cases[i].run(test);
        
        switch (ret) {
            case RESULT_OK:
                printk(KERN_INFO "[nand_test]: Result: OK\n");
                break;
            case RESULT_FAIL:
                printk(KERN_INFO "[nand_test]:Result: FAILED\n");
                break;
                //    case RESULT_UNSUP_HOST:                //grace del
                //      printk(KERN_INFO "%s: Result: UNSUPPORTED "
                //        "(by host)\n",
                //        mmc_hostname(test->card->host));
                //      break;
                //    case RESULT_UNSUP_CARD:
                //      printk(KERN_INFO "%s: Result: UNSUPPORTED "
                //        "(by card)\n",
                //        mmc_hostname(test->card->host));
                //      break;
            default:
                printk(KERN_INFO "[nand_test]:Result: ERROR (%d)\n", ret);
        }
    
        if (nand_test_cases[i].cleanup) {
            ret = nand_test_cases[i].cleanup(test);
            if (ret) {
                printk(KERN_INFO "[nand_test]:Warning: Cleanup"
                       "stage failed! (%d)\n", ret);
            }
        }
    }
  
    //mmc_release_host(test->card->host);
    
    printk(KERN_INFO "[nand_test]: Nand tests completed.\n");
}
#endif


/* do nothing */
// g_iShowVar >= 0  --> disksize
// g_iShowVar = -1  --> test ok
// g_iShowVar = -2  --> test fail
// g_iShowVar = -3  --> general err		such as: cmd fmt err
// g_iShowVar = -4  --> severe  err		such as: w/r nand exe err
int g_iShowVar = -1;
static ssize_t nand_test_show(struct kobject *kobject,struct attribute *attr, char *buf)
{
	ssize_t count = 0;

	count = sprintf(buf, "%i\n", g_iShowVar);

	return count;
}

/*
*/
static void initdata(BYTE *buf, DWORD cnt)
{
	DWORD i;
	for (i=0; i<cnt; i++)
	{
		buf[i] = (BYTE)i;
	}
}

/*
*/
static bool verifydata(const BYTE *buf, DWORD off, DWORD cnt)
{
	DWORD i;
	for (i=0; i<cnt; i++)
	{
		if (buf[i] != (BYTE)(i+off))
		{
			return false;
		}
	}

	return true;
}

/*
* 读写bufoff必须保持一致
* 返回值为：
* -1  --> test ok
* -2  --> test fail
* -3  --> general err		such as: cmd fmt err
* -4  --> severe  err		such as: w/r nand exe err
*/
static int nand_rw_ops(unsigned int startsec, unsigned int secnum, void *buf, unsigned int off, bool rw, bool cache, bool verify)
{
	int		i;
	int		rws;
	int		restsecs;
	BYTE	*startaddr;
	char 	cTemp[20];

	struct timeval	starttime;	
	struct timeval 	endtime;	
	unsigned int 		usedtime;	
	unsigned int 		speed;
	bool			verifyres = false;

	startaddr = (BYTE *)buf + off;
	rws = secnum / MAX_SECTORS;
	restsecs = secnum % MAX_SECTORS;

	mutex_lock(&nand_test_lock);
	do_gettimeofday(&starttime);
	if (rw)						//write
	{
		for (i=0; i<rws; i++)
		{
			if (0 != cache ? NAND_CacheWrite(startsec+MAX_SECTORS*i, MAX_SECTORS, startaddr) 
							: LML_Write(startsec+MAX_SECTORS*i, MAX_SECTORS, startaddr))
			{
				printk(KERN_INFO "nand write fail!\n");
				return -4;
			}
		}
		if (0 != cache ? NAND_CacheWrite(startsec+MAX_SECTORS*rws, restsecs, startaddr) 
						: LML_Write(startsec+MAX_SECTORS*rws, restsecs, startaddr))
		{
			printk(KERN_INFO "nand write fail!\n");
			return -4;
		}
	}
	else						//read
	{
		//LML_FlushPageCache();

		for (i=0; i<rws; i++)
		{
			if (0 != cache ? NAND_CacheRead(startsec+MAX_SECTORS*i, MAX_SECTORS, startaddr) 
							: LML_Read(startsec+MAX_SECTORS*i, MAX_SECTORS, startaddr))
			{
				printk(KERN_INFO "nand read fail!\n");
				return -4;
			}
			if (verify)
			{
				verifyres = verifydata(startaddr, off, MAX_SECTORS*512);
				if (!verifyres)
				{
					goto VERIFY_END;
				}
			}
		}
		if (0 != cache ? NAND_CacheRead(startsec+MAX_SECTORS*rws, restsecs, startaddr) 
						: LML_Read(startsec+MAX_SECTORS*rws, restsecs, startaddr))
		{
			printk(KERN_INFO "nand read fail!\n");
			return -4;
		}
		if (verify)
		{
			verifyres = verifydata(startaddr, off, restsecs*512);
		}		
	}	
VERIFY_END:
	do_gettimeofday(&endtime);
	mutex_unlock(&nand_test_lock);

	usedtime = (endtime.tv_sec - starttime.tv_sec) * 1000 * 1000 + (endtime.tv_usec - starttime.tv_usec);		// us
	//usedtime = 1000 * (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec) / 1000;		// ms
	
	memset(cTemp, 0, sizeof(cTemp));
	if (usedtime >= 1000)
	{
		usedtime /= 1000;		//ms
		sprintf(cTemp, "%u", usedtime);
		speed = secnum * 500 / usedtime;																		// KB/S
	}
	else if (0 == usedtime)
	{
		sprintf(cTemp, "%u", usedtime);
		speed = 999999;
	}
	else
	{
		speed = secnum * 500000 / usedtime;																	// KB/S
		if (usedtime >= 100)
		{
			sprintf(cTemp, "0.%u", usedtime);
		}
		else if (usedtime >= 10)
		{
			sprintf(cTemp, "0.0%u", usedtime);
		}
		else
		{
			sprintf(cTemp, "0.00%u", usedtime);
		}
	}

	//func		filesize/startsec		filenum/secnum		packagesize/off		usedtime	speed	cache	verify
	printk(KERN_INFO "%s\t%u\t\t%u\t\t%u\t\t%s\t\t%u\t\t%c\t\t%c/%c\n", 
			rw?"nand_kernel_write":"nand_kernel_read ", startsec, secnum, off, cTemp, speed, cache?'y':'n', verify?'y':'n', verifyres?'y':'n');

	if (verify && !verifyres)
	{
		return -2;
	} 
	else
	{
		return -1;
	}
}

/* receive testcase num from echo command */
static ssize_t nand_test_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count)
{
	int 	argnum = 0;
	char 	func = 'x';
	char 	cache = 'x';
	char 	verify = 'x';
	unsigned int 	startsec = ~(DWORD)0;
	unsigned int	secnum = ~(DWORD)0;
	unsigned int	bufoff = ~(DWORD)0;
	BYTE	*databuf;

	g_iShowVar = -1;
	
	databuf = (BYTE *)kzalloc(BUFFER_SIZE+4, GFP_KERNEL);  			// alloc buffer for r/w
	if (NULL == databuf)
	{
		printk(KERN_INFO "kzalloc fail!\n");
		g_iShowVar = -3;
		goto NAND_TEST_STORE_EXIT;
	}
                          
	argnum = sscanf(buf, "%c %u %u %u %c %c", &func, &startsec, &secnum, &bufoff, &cache, &verify);
	//printk(KERN_INFO "argnum=%i, func=%c, startsec=%u, secnum=%u, bufoff=%u, cache=%c, verify=%c\n", 
	//	argnum, func, startsec, secnum, bufoff, cache, verify);

	if (-1 == argnum)
	{
CMD_FORMAT_ERR:
		printk(KERN_INFO "cmd format err!\n");
		g_iShowVar = -3;
		goto NAND_TEST_STORE_EXIT;
	}	

	switch (func)
	{
	case 'w':			//write
		if ((5!=argnum) || (bufoff>4) || (('y'!=cache)&&('n'!=cache)))
		{
			goto CMD_FORMAT_ERR;
		}
		if (bufoff < 4)
		{
			initdata(databuf, BUFFER_SIZE+4);
			//get_random_bytes(&randNum[i], sizeof(unsigned long));
		}
		else
		{
			memset(databuf, 0, BUFFER_SIZE+4);
			bufoff = 0;
		}
		g_iShowVar = nand_rw_ops(startsec, secnum, databuf, bufoff, true, 'y'==cache, false);
		break;

	case 'r':			//read
		if ((6!=argnum) || (bufoff>=4) || (('y'!=cache)&&('n'!=cache)) || (('y'!=verify)&&('n'!=verify)))
		{
			goto CMD_FORMAT_ERR;
		}
		g_iShowVar = nand_rw_ops(startsec, secnum, databuf, bufoff, false, 'y'==cache, 'y'==verify);
/*		{
			int i;
			printk(KERN_INFO "after reading:\n");
			for (i=0; i<520; i++)
			{
				printk(KERN_INFO "%x ", databuf[i]);
			}
		}	*/
		break;

	case 'f':			//flush
		if ((2!=argnum) || ((0!=startsec)&&(1!=startsec)))
		{
			goto CMD_FORMAT_ERR;
		}
		if (0 == startsec)
		{
			if (0 != LML_FlushPageCache())
			{
				printk(KERN_INFO "LML_FlushPageCache err!\n");
				g_iShowVar = -4;
			}
		}
		else
		{
			NAND_CacheFlush();
		}
		break;

	case 'h':
		if (1 != argnum)
		{
			goto CMD_FORMAT_ERR;
		}
		//func		filesize/startsec		filenum/secnum		packagesize/off		usedtime	speed	cache	verify
		printk(KERN_INFO "用例名称\t\t\t开始扇区\t扇区数\t\tbuf偏移(Byte)\t使用时间(ms)\t速度(KB/s)\t是否cache\t是否校验/校验结果\n");
		break;

	case 's':
		if (1 != argnum)
		{
			goto CMD_FORMAT_ERR;
		}
		//printk(KERN_INFO "%u", NAND_GetDiskSize());
		g_iShowVar = NAND_GetDiskSize();
		break;

	case 'b':
		if (1 != argnum)
		{
			goto CMD_FORMAT_ERR;
		}
		printk(KERN_INFO "burn nand\n");
		break;

	default:
		goto CMD_FORMAT_ERR;
		//printk(KERN_INFO "cmd format err!\n");
		//break;
	}

NAND_TEST_STORE_EXIT: 
	kfree(databuf);	

	return count;


/*
    testcase = simple_strtol(buf, NULL, 10);  // get test case number     >> grace

	if ((testcase<0) || (testcase>ARRAY_SIZE(nand_test_cases)))
	{
		return -ENOMEM;
	}

	printk(KERN_INFO "testcase: %i\n", testcase);
  
    test = kzalloc(sizeof(struct nand_test_card), GFP_KERNEL);
    if (!test){
        return -ENOMEM;
    }
  
    test->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);  // alloc buffer for r/w
    test->scratch = kzalloc(BUFFER_SIZE, GFP_KERNEL); // not used now
    
    if (test->buffer && test->scratch) {
        mutex_lock(&nand_test_lock);
        nand_test_run(test, testcase);             // run test cases
        mutex_unlock(&nand_test_lock);
    }
  
  
    kfree(test->buffer);
    kfree(test->scratch);
    kfree(test);
  
    return count;
*/
}



static int __init nand_test_init(void)
{
  int ret;
  #ifdef __LINUX_NAND_SUPPORT_INT__	
	unsigned long irqflags_ch0, irqflags_ch1;
	#endif

	printk(KERN_INFO "nand_test_init start...\n");

  if((ret = kobject_init_and_add(&kobj,&ktype,NULL,"nand")) != 0 ) {
  	return ret; 
  }

	ClearNandStruct();

	ret = PHY_Init();
	if (ret) {
		PHY_Exit();
		return -1;
	}
	
#ifdef __LINUX_NAND_SUPPORT_INT__	
    printk("[NAND] nand driver version: 0x%x 0x%x, support int! \n", NAND_VERSION_0,NAND_VERSION_1);
#ifdef __LINUX_SUPPORT_RB_INT__
    NAND_ClearRbInt();
#endif
#ifdef __LINUX_SUPPORT_DMA_INT__
    NAND_ClearDMAInt();
#endif

	spin_lock_init(&nand_test_int_lock);
	irqflags_ch0 = IRQF_DISABLED;
	irqflags_ch1 = IRQF_DISABLED;

	if (request_irq(AW_IRQ_NAND0, nand_test_interrupt_ch0, IRQF_DISABLED, nand_mytr.name, &nand_mytr))
	{
	    printk("nand interrupte ch0 irqno: %d register error\n", AW_IRQ_NAND0);
	    return -EAGAIN;
	}
	else
	{
	    printk("nand interrupte ch0 irqno: %d register ok\n", AW_IRQ_NAND0);
	}	

	if (request_irq(AW_IRQ_NAND1, nand_test_interrupt_ch1, IRQF_DISABLED, nand_mytr.name, &nand_mytr))
	{
	    printk("nand interrupte ch1, irqno: %d register error\n", AW_IRQ_NAND1);
	    return -EAGAIN;
	}
	else
	{
	    printk("nand interrupte ch1, irqno: %d register ok\n", AW_IRQ_NAND1);
	}
#endif		

	ret = SCN_AnalyzeNandSystem();
	if (ret < 0)
		return ret;


	ret = PHY_ChangeMode(1);
	if (ret < 0)
		return ret;

    ret = PHY_ScanDDRParam();
    if (ret < 0)
        return ret;

	ret = FMT_Init();
	if (ret < 0)
		return ret;

	ret = FMT_FormatNand();
	if (ret < 0)
		return ret;
	FMT_Exit();

	/*init logic layer*/
	ret = LML_Init();
	if (ret < 0)
		return ret;

	#ifdef NAND_CACHE_RW
		NAND_CacheOpen();
	#endif

   printk(KERN_INFO "nand_test_init ok\n");

   return 0;  // init success 
}



static void __exit nand_test_exit(void)
{
    printk(KERN_INFO "nand_test_exit start...\n");
	
    kobject_del(&kobj);
	kobject_put(&kobj); 
	

    //LML_FlushPageCache();	
    //BMM_WriteBackAllMapTbl();
    
    //nand_flush(NULL);
#ifdef NAND_CACHE_RW
    NAND_CacheClose();
#endif
    LML_Exit();
    FMT_Exit();
    PHY_Exit(); 



	printk(KERN_INFO "nand_test_exit ok\n");

    return;
}
  
  
module_init(nand_test_init);
module_exit(nand_test_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Nand test driver");
MODULE_AUTHOR("Grace Miao");
//#endif
