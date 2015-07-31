/*
 * arch/arm/mach-sun6i/dram-freq/dram-freq.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <mach/system.h>
#include <mach/sys_config.h>
#include <mach/dram-freq.h>

#undef DRAMFREQ_ERR
#undef DRAMFREQ_DBG
#if (1)
    #define DRAMFREQ_DBG(format,args...)   printk("[dramfreq] "format,##args)
#else
    #define DRAMFREQ_DBG(format,args...)   do{}while(0)
#endif
#define DRAMFREQ_ERR(format,args...)   printk(KERN_ERR "[dramfreq] ERR:"format,##args)

extern char *mdfs_bin_start;
extern char *mdfs_bin_end;

static struct srcu_notifier_head dramfreq_transition_notifier;
static bool init_dramfreq_transition_notifier_called;

static int __init init_dramfreq_transition_notifier(void)
{
	srcu_init_notifier_head(&dramfreq_transition_notifier);
	init_dramfreq_transition_notifier_called = true;
	return 0;
}
pure_initcall(init_dramfreq_transition_notifier);

static struct clk *clk_pll5; /* pll5 clock handler */
static unsigned int master_bw_usage[MASTER_MAX] = {0};
static unsigned int bw_cnt_hist[MASTER_MAX] = {0};
struct aw_mdfs_info mdfs_info;
static spinlock_t mdfs_spin_lock;
struct devfreq *this_df = NULL;
static unsigned int dram_clk_syscfg = 0;
static unsigned int pll5_freq = 0;
static unsigned int dram_freq_table[SUN6I_DRAMFREQ_TABLE_SIZE][2] = {{0}};

struct master_info master_info_list[MASTER_INFO_SIZE] = {
    { MASTER_CPUX, "CPUX" },
    { MASTER_GPU0, "GPU0" },
    { MASTER_GPU1, "GPU1" },
    { MASTER_CPUS, "CPUS" },
    { MASTER_ATH , "ATH"  },
    { MASTER_GMAC, "GMAC" },
    { MASTER_SDC0, "SDC0" },
    { MASTER_SDC1, "SDC1" },
    { MASTER_SDC2, "SDC2" },
    { MASTER_SDC3, "SDC3" },
    { MASTER_USB , "USB"  },
    { MASTER_NFC1, "NFC1" },
    { MASTER_DMAC, "DMAC" },
    { MASTER_VE  , "VE"   },
    { MASTER_MP  , "MP"   },
    { MASTER_NFC0, "NFC0" },
    { MASTER_DRC0, "DRC0" },
    { MASTER_DRC1, "DRC1" },
    { MASTER_DEU0, "DEU0" },
    { MASTER_DEU1, "DEU1" },
    { MASTER_BE0 , "BE0"  },
    { MASTER_FE0 , "FE0"  },
    { MASTER_BE1 , "BE1"  },
    { MASTER_FE1 , "FE1"  },
    { MASTER_CSI0, "CSI0" },
    { MASTER_CSI1, "CSI1" },
    { MASTER_TS  , "TS"   },
    { MASTER_ALL , "ALL"  },
};


int dramfreq_register_notifier(struct notifier_block *nb)
{
	WARN_ON(!init_dramfreq_transition_notifier_called);
	return srcu_notifier_chain_register(&dramfreq_transition_notifier, nb);
}
EXPORT_SYMBOL_GPL(dramfreq_register_notifier);

int dramfreq_unregister_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&dramfreq_transition_notifier, nb);
}
EXPORT_SYMBOL_GPL(dramfreq_unregister_notifier);

int dramfreq_notify_transition(unsigned int state, struct dramfreq_udata *data)
{
	switch (state) {
	case DRAMFREQ_PRECHANGE:
		srcu_notifier_call_chain(&dramfreq_transition_notifier, DRAMFREQ_PRECHANGE, data);
		break;
	case DRAMFREQ_POSTCHANGE:
		srcu_notifier_call_chain(&dramfreq_transition_notifier, DRAMFREQ_POSTCHANGE, data);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dramfreq_notify_transition);


static int dramfreq_table_target(unsigned long *target_freq, unsigned int *index)
{
    int i = 0;
    int j = 0;

    while (((dram_freq_table[i+1][0] >= *target_freq)
            || (dram_freq_table[i+1][0] == 0)) && (i < (SUN6I_DRAMFREQ_TABLE_SIZE - 1)))
    {
        if (dram_freq_table[i+1][0] != 0) {
            j = (i + 1);
        }
        i++;
    }
    *index = j;

    return 0;
}

static unsigned long __dramfreq_get(struct clk *pll5)
{
    unsigned long pll5_rate, dram_freq;
    unsigned int dram_div;

    pll5_rate = clk_get_rate(pll5) / 1000;
    dram_div = 1 + ((readl(CCM_DRAMCLK_CFG_CTRL) >> 8) & 0xf);
    dram_freq = pll5_rate / dram_div;

    return dram_freq;
}

/**
 * dramfreq_get - get the current DRAM frequency (in KHz)
 *
 */
unsigned long dramfreq_get(void)
{
    return __dramfreq_get(clk_pll5);
}
EXPORT_SYMBOL_GPL(dramfreq_get);

/**
 * 	freq_div: PLL5/freq_div = current frequency
 */
int __dramfreq_set(unsigned int freq_div)
{
    int (*mdfs_main)(struct aw_mdfs_info *mdfs);
    unsigned long flags;

    if ((freq_div < 1) || (freq_div > 16)) {
        DRAMFREQ_ERR("mdfs div=%u is invalid\n", freq_div);
        return -1;
    }

    mdfs_main = (int (*)(struct aw_mdfs_info *mdfs))SRAM_MDFS_START;

    /* move mdfs_main code to sram */
    memcpy((void *)SRAM_MDFS_START, (void *)&mdfs_bin_start, (int)&mdfs_bin_end - (int)&mdfs_bin_start);
    mdfs_info.div = freq_div;

    spin_lock_irqsave(&mdfs_spin_lock, flags);
    /* goto sram and run */
    mdfs_main(&mdfs_info);
    spin_unlock_irqrestore(&mdfs_spin_lock, flags);
    DRAMFREQ_DBG("MDFS finish\n");

	return 0;
}

static int dramfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
    struct platform_device *pdev = container_of(dev, struct platform_device, dev);
    struct devfreq *df = platform_get_drvdata(pdev);
    struct dramfreq_udata *user_data = (struct dramfreq_udata *)df->data;
    unsigned int dram_div = 0;
    unsigned long freq_target = 0;
    int index = 0;
    int ret = 0;

    if (*freq == df->previous_freq) {
        DRAMFREQ_DBG("freq_calc == df->previous_freq\n");
        return 0;
    }

    dramfreq_table_target(freq, &index);
    dram_div = dram_freq_table[index][1];

    freq_target = pll5_freq / dram_div;
    DRAMFREQ_DBG("dram target frequency is find: %luMHz, entry %u\n", freq_target / 1000, index);
    if (freq_target == df->previous_freq) {
        DRAMFREQ_DBG("freq_target == df->previous_freq\n");
        *freq = freq_target;
        return 0;
    }

    dramfreq_notify_transition(DRAMFREQ_PRECHANGE, user_data);
    ret = __dramfreq_set(dram_div);
    if (ret) {
        DRAMFREQ_ERR("set dram frequency hw failed!\n");
        user_data->freq_to_user = df->previous_freq;
        dramfreq_notify_transition(DRAMFREQ_POSTCHANGE, user_data);
        return ret;
    }

    *freq = freq_target;
    user_data->freq_to_user = freq_target;
    dramfreq_notify_transition(DRAMFREQ_POSTCHANGE, user_data);

    DRAMFREQ_DBG("dram: %luMHz->%luMHz ok!\n", df->previous_freq / 1000, freq_target / 1000);

    return 0;
}

static void __update_master_bw(void)
{
    int i, bw_cnt_cur, bw_usage;
    enum master_type mt;

    for (i = 0; i < ARRAY_SIZE(master_info_list); i++) {
        mt = master_info_list[i].type;
        writel(mt << 1 | 1, SDR_COM_MCGCR);
        bw_cnt_cur = readl(SDR_COM_BWCR);
        bw_usage = bw_cnt_cur - bw_cnt_hist[mt];
        if (MASTER_GPU0 == mt) {
            master_bw_usage[mt] = 2 * (bw_usage / 1024);
        } else {
            master_bw_usage[mt] = (bw_usage / 1024);
        }
        bw_cnt_hist[mt] = bw_cnt_cur;
    }
}

static void master_bw_dbg(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(master_info_list); i++) {
        DRAMFREQ_DBG("MASTER_%s=%u(MB)\n", master_info_list[i].name, master_bw_usage[master_info_list[i].type]);
    }
}

static int dramfreq_get_dev_status(struct device *dev,
				      struct devfreq_dev_status *stat)
{
    __update_master_bw();
    stat->current_frequency = dramfreq_get();
    stat->private_data = master_bw_usage;
    master_bw_dbg();

    return 0;
}

static struct devfreq_dev_profile dram_devfreq_profile = {
    .polling_ms     = SUN6I_DRAMFREQ_POLLING_MS,
    .target         = dramfreq_target,
    .get_dev_status	= dramfreq_get_dev_status,
};

static void __dramfreq_init_table(struct clk *pll5)
{
    int i;
    pll5_freq = clk_get_rate(pll5) / 1000;

    for (i = 0; i < SUN6I_DRAMFREQ_TABLE_SIZE; i++) {
        if (!(pll5_freq % (1000 * (i + 1)))) {
            dram_freq_table[i][0] = pll5_freq / (i + 1);
            dram_freq_table[i][1] = (i + 1);
        } else {
            dram_freq_table[i][0] = 0;
            dram_freq_table[i][1] = 0;
        }
    }
}

static ssize_t dram_table_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
    int i;

    for (i = 0; i < SUN6I_DRAMFREQ_TABLE_SIZE; i++) {
        printk("freq=%-6u   div=%-5u\n", dram_freq_table[i][0], dram_freq_table[i][1]);
    }

    return 0;
}

static ssize_t mdfs_table_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
    int m, n;

    for (m = 0; m < SUN6I_DRAMFREQ_TABLE_SIZE; m++) {
        for (n = 0; n < 8; n++) {
            DRAMFREQ_DBG("mdfs_table[%d][%d]=0x%x\n", m, n, mdfs_info.table[m][n]);
        }
    }

    return 0;
}

static ssize_t dual_channel_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
    return sprintf(buf, "%d\n", mdfs_info.is_dual_channel);
}

static DEVICE_ATTR(dram_table, S_IRUGO, dram_table_show, NULL);
static DEVICE_ATTR(mdfs_table, S_IRUGO, mdfs_table_show, NULL);
static DEVICE_ATTR(dual_channel, S_IRUGO, dual_channel_show, NULL);

static const struct attribute *dramfreq_arrrib[] = {
    &dev_attr_dram_table.attr,
    &dev_attr_mdfs_table.attr,
    &dev_attr_dual_channel.attr,
    NULL
};

static __devinit int sun6i_dramfreq_probe(struct platform_device *pdev)
{
    void *tmp_tbl = NULL;
    int err = 0;
    script_item_u used;
    script_item_value_type_e type;

    type = script_get_item("dram_para", "dram_clk", &used);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch para from sysconfig failed\n");
        return -ENODEV;
    }

    dram_clk_syscfg = used.val * 1000;
    DRAMFREQ_DBG("dram clk sysconfig=%uKHz\n", dram_clk_syscfg);

    tmp_tbl = __va(SYS_CONFIG_MEMBASE + SYS_CONFIG_MEMSIZE - 1024);
    memcpy(mdfs_info.table, tmp_tbl, sizeof(mdfs_info.table));
    mdfs_info.is_dual_channel = !!(readl(SDR_COM_CR) & (0x1<<19));

    clk_pll5 = clk_get(NULL, CLK_SYS_PLL5);
    if (!clk_pll5 || IS_ERR(clk_pll5)) {
        DRAMFREQ_ERR("try to get PLL5 failed!\n");
        err = -ENOENT;
        goto err_clk;
    }

    /* init dram freq table */
    __dramfreq_init_table(clk_pll5);

    dram_devfreq_profile.initial_freq = __dramfreq_get(clk_pll5);
    this_df = devfreq_add_device(&pdev->dev, &dram_devfreq_profile, &devfreq_userspace, NULL);
    // this_df = devfreq_add_device(&pdev->dev, &dram_devfreq_profile, &devfreq_vans, NULL);
	if (IS_ERR(this_df)) {
        DRAMFREQ_ERR("add devfreq device failed!\n");
        err = PTR_ERR(this_df);
		goto err_devfreq;
	}

    this_df->min_freq = this_df->scaling_min_freq = SUN6I_DRAMFREQ_MIN / 1000;
    this_df->max_freq = this_df->scaling_max_freq = SUN6I_DRAMFREQ_MAX / 1000;
    platform_set_drvdata(pdev, this_df);

    err = sysfs_create_files(&pdev->dev.kobj, dramfreq_arrrib);
    if (err) {
        DRAMFREQ_ERR("create sysfs file failed\n");
        err = -ENODEV;
        goto err_create_files;
    }

    DRAMFREQ_DBG("sun6i dramfreq probe ok!\n");

    return 0;

err_create_files:
    devfreq_remove_device(this_df);
err_devfreq:
    clk_put(clk_pll5);
    clk_pll5 = NULL;
err_clk:
    return err;
}

static __devexit int sun6i_dramfreq_remove(struct platform_device *pdev)
{
    struct devfreq *df = platform_get_drvdata(pdev);

    devfreq_remove_device(df);

    if (!clk_pll5 || IS_ERR(clk_pll5)) {
        DRAMFREQ_ERR("clk_pll5 handle is invalid, just return!\n");
        return -EINVAL;
    } else {
        clk_put(clk_pll5);
        clk_pll5 = NULL;
    }

    return 0;
}

static struct platform_driver sun6i_dramfreq_driver = {
    .probe  = sun6i_dramfreq_probe,
    .remove	= sun6i_dramfreq_remove,
    .driver = {
        .name   = "sun6i-dramfreq",
        .owner  = THIS_MODULE,
    },
};

struct platform_device sun6i_dramfreq_device = {
    .name       = "sun6i-dramfreq",
};

static int __init sun6i_dramfreq_init(void)
{
    int ret = 0;

    ret = platform_device_register(&sun6i_dramfreq_device);
    if (ret) {
        DRAMFREQ_ERR("dramfreq device init failed!\n");
        goto out;
    }

    ret = platform_driver_register(&sun6i_dramfreq_driver);
    if (ret) {
        DRAMFREQ_ERR("dramfreq driver init failed!\n");
        goto out;
    }

out:
    return ret;
}
late_initcall(sun6i_dramfreq_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUN6I dramfreq driver with devfreq framework");
MODULE_AUTHOR("pannan");
