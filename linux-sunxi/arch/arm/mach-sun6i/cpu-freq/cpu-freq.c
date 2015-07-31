/*
 *  arch/arm/mach-sun6i/cpu-freq/cpu-freq.c
 *
 * Copyright (c) 2012 softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <mach/sys_config.h>
#include <linux/cpu.h>
#include <asm/cpu.h>

#include "cpu-freq.h"
#include <linux/pm.h>
#include <mach/ar100.h>
#include <mach/clock.h>
#include <mach/platform.h>
#include <linux/regulator/consumer.h>

static struct sunxi_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration  */
static unsigned int last_target = ~0;       /* backup last target frequency         */

struct regulator *cpu_vdd;  /* cpu vdd handler   */
static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */
static DEFINE_MUTEX(sunxi_cpu_lock);

static unsigned int cpu_freq_max = SUNXI_CPUFREQ_MAX / 1000;
static unsigned int cpu_freq_min = SUNXI_CPUFREQ_MIN / 1000;

#ifdef CONFIG_SMP
static struct cpumask sunxi_cpumask;
static int cpus_initialized;
#endif

#ifdef CONFIG_CPU_FREQ_SETFREQ_DEBUG
int setgetfreq_debug = 0;
unsigned long long setfreq_time_usecs = 0;
unsigned long long getfreq_time_usecs = 0;
#endif

int sunxi_dvfs_debug = 0;
void __iomem *temp_base_addr;

/*
 *check if the cpu frequency policy is valid;
 */
static int sunxi_cpufreq_verify(struct cpufreq_policy *policy)
{
    return 0;
}


/*
 * get the current cpu vdd;
 * return: cpu vdd, based on mv;
 */
int sunxi_cpufreq_getvolt(void)
{
    return regulator_get_voltage(cpu_vdd) / 1000;
}

/*
 * get the current cpu tempreture;
 * return: tempreture, based on C;
 */
int sunxi_cpufreq_gettemp(void)
{
    unsigned int value = 0;

    value = readl(temp_base_addr + 0x20);
    value *= 100;
    if (0 != value)
        do_div(value, 600);
    value -= 271;

    return value;
}

/*
 * get the frequency that cpu currently is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_get(unsigned int cpu)
{
    unsigned int current_freq = 0;
#ifdef CONFIG_CPU_FREQ_SETFREQ_DEBUG
    ktime_t calltime = ktime_set(0, 0), delta, rettime;

    if (unlikely(setgetfreq_debug)) {
        calltime = ktime_get();
    }
#endif

    current_freq = clk_get_rate(clk_cpu) / 1000;

#ifdef CONFIG_CPU_FREQ_SETFREQ_DEBUG
    if (unlikely(setgetfreq_debug)) {
        rettime = ktime_get();
        delta = ktime_sub(rettime, calltime);
        getfreq_time_usecs = ktime_to_ns(delta) >> 10;
        printk("[getfreq]: %Ld usecs\n", getfreq_time_usecs);
    }
#endif

    return current_freq;
}


/*
 *show cpu frequency information;
 */
static void sunxi_cpufreq_show(const char *pfx, struct sunxi_cpu_freq_t *cfg)
{
    printk("%s: pll=%u, cpudiv=%u, axidiv=%u\n", pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div);
}


/*
 * adjust the frequency that cpu is currently running;
 * policy:  cpu frequency policy;
 * freq:    target frequency to be set, based on khz;
 * relation:    method for selecting the target requency;
 * return:  result, return 0 if set target frequency successed, else, return -EINVAL;
 * notes:   this function is called by the cpufreq core;
 */
static int sunxi_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
    int                     i, ret = 0;
    unsigned int            index;
    struct sunxi_cpu_freq_t freq_cfg;
    struct cpufreq_freqs    freqs;
#ifdef CONFIG_CPU_FREQ_SETFREQ_DEBUG
    ktime_t calltime = ktime_set(0, 0), delta, rettime;

    if (unlikely(setgetfreq_debug)) {
        calltime = ktime_get();
    }
#endif

    mutex_lock(&sunxi_cpu_lock);

#ifdef CONFIG_SMP
    /* Wait untill all CPU's are initialized */
    if (unlikely(cpus_initialized < num_online_cpus())) {
        ret = -EINVAL;
        goto out;
    }
#endif

    /* avoid repeated calls which cause a needless amout of duplicated
     * logging output (and CPU time as the calculation process is
     * done) */
    if (freq == last_target)
        goto out;

    /* try to look for a valid frequency value from cpu frequency table */
    if (cpufreq_frequency_table_target(policy, sunxi_freq_tbl, freq, relation, &index)) {
        CPUFREQ_ERR("try to look for a valid frequency for %u failed!\n", freq);
        ret = -EINVAL;
        goto out;
    }

    /* frequency is same as the value last set, need not adjust */
    if (sunxi_freq_tbl[index].frequency == last_target)
        goto out;

    freq = sunxi_freq_tbl[index].frequency;

    /* update the target frequency */
    freq_cfg.pll = sunxi_freq_tbl[index].frequency * 1000;
    freq_cfg.div = *(struct sunxi_clk_div_t *)&sunxi_freq_tbl[index].index;

    if (unlikely(sunxi_dvfs_debug))
        printk("[cpufreq] target frequency find is %u, entry %u\n", freq_cfg.pll, index);

    /* notify that cpu clock will be adjust if needed */
    if (policy) {
        freqs.cpu = policy->cpu;
        freqs.old = last_target;
        freqs.new = freq;

#ifdef CONFIG_SMP
        /* notifiers */
        for_each_cpu(i, policy->cpus) {
            freqs.cpu = i;
            cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
        }
#else
        cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
#endif
    }

    /* try to set cpu frequency */
    if (ar100_dvfs_set_cpufreq(freq, AR100_DVFS_SYN, NULL, NULL)) {
        CPUFREQ_ERR("set cpu frequency to %uMHz failed!\n", freq / 1000);
        /* set cpu frequency failed */
        if (policy) {
            freqs.cpu = policy->cpu;
            freqs.old = freqs.new;
            freqs.new = last_target;

#ifdef CONFIG_SMP
            /* notifiers */
            for_each_cpu(i, policy->cpus) {
                freqs.cpu = i;
                cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
            }
#else
            cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
#endif
        }

        ret = -EINVAL;
        goto out;
    }

    /* notify that cpu clock will be adjust if needed */
    if (policy) {
#ifdef CONFIG_SMP
        /*
         * Note that loops_per_jiffy is not updated on SMP systems in
         * cpufreq driver. So, update the per-CPU loops_per_jiffy value
         * on frequency transition. We need to update all dependent cpus
         */
        for_each_cpu(i, policy->cpus) {
            per_cpu(cpu_data, i).loops_per_jiffy =
                 cpufreq_scale(per_cpu(cpu_data, i).loops_per_jiffy, freqs.old, freqs.new);
            freqs.cpu = i;
            cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
        }
#else
        cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
#endif
    }

    last_target = freq;

    if (unlikely(sunxi_dvfs_debug))
        printk("[cpufreq] DVFS done! Freq[%uMHz] Volt[%umv] ok\n\n", \
            sunxi_cpufreq_get(0) / 1000, sunxi_cpufreq_getvolt());

out:
#ifdef CONFIG_CPU_FREQ_SETFREQ_DEBUG
    if (unlikely(setgetfreq_debug)) {
        rettime = ktime_get();
        delta = ktime_sub(rettime, calltime);
        setfreq_time_usecs = ktime_to_ns(delta) >> 10;
        printk("[setfreq]: %Ld usecs\n", setfreq_time_usecs);
    }
#endif
    mutex_unlock(&sunxi_cpu_lock);

    return ret;
}


/*
 * get the frequency that cpu average is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_getavg(struct cpufreq_policy *policy, unsigned int cpu)
{
    return clk_get_rate(clk_cpu) / 1000;
}


/*
 * get a valid frequency from cpu frequency table;
 * target_freq:	target frequency to be judge, based on KHz;
 * return: cpu frequency, based on khz;
 */
static unsigned int __get_valid_freq(unsigned int target_freq)
{
    struct cpufreq_frequency_table *tmp = &sunxi_freq_tbl[0];

    while(tmp->frequency != CPUFREQ_TABLE_END){
        if((tmp+1)->frequency <= target_freq)
            tmp++;
        else
            break;
    }

    return tmp->frequency;
}


/*
 * init cpu max/min frequency from sysconfig;
 * return: 0 - init cpu max/min successed, !0 - init cpu max/min failed;
 */
static int __init_freq_syscfg(void)
{
    int ret = 0;
    script_item_u max, min;
    script_item_value_type_e type;

    type = script_get_item("dvfs_table", "max_freq", &max);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu max frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    cpu_freq_max = max.val;

    type = script_get_item("dvfs_table", "min_freq", &min);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu min frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    cpu_freq_min = min.val;

    if(cpu_freq_max > SUNXI_CPUFREQ_MAX || cpu_freq_max < SUNXI_CPUFREQ_MIN
        || cpu_freq_min < SUNXI_CPUFREQ_MIN || cpu_freq_min > SUNXI_CPUFREQ_MAX){
        CPUFREQ_ERR("cpu max or min frequency from sysconfig is more than range\n");
        ret = -1;
        goto fail;
    }

    if(cpu_freq_min > cpu_freq_max){
        CPUFREQ_ERR("cpu min frequency can not be more than cpu max frequency\n");
        ret = -1;
        goto fail;
    }

    /* get valid max/min frequency from cpu frequency table */
    cpu_freq_max = __get_valid_freq(cpu_freq_max / 1000);
    cpu_freq_min = __get_valid_freq(cpu_freq_min / 1000);

    return 0;

fail:
    /* use default cpu max/min frequency */
    cpu_freq_max = SUNXI_CPUFREQ_MAX / 1000;
    cpu_freq_min = SUNXI_CPUFREQ_MIN / 1000;

    return ret;
}


/*
 * cpu frequency initialise a policy;
 * policy:  cpu frequency policy;
 * result:  return 0 if init ok, else, return -EINVAL;
 */
static int sunxi_cpufreq_init(struct cpufreq_policy *policy)
{
    policy->cur = sunxi_cpufreq_get(0);
    policy->cpuinfo.min_freq = cpu_freq_min;
    policy->cpuinfo.max_freq = cpu_freq_max;
    policy->min = cpu_freq_min;
    policy->max = 1008000;
    policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

    /* feed the latency information from the cpu driver */
    policy->cpuinfo.transition_latency = SUNXI_FREQTRANS_LATENCY;
    cpufreq_frequency_table_get_attr(sunxi_freq_tbl, policy->cpu);

#ifdef CONFIG_SMP
    /*
     * both processors share the same voltage and the same clock,
     * but have dedicated power domains. So both cores needs to be
     * scaled together and hence needs software co-ordination.
     * Use cpufreq affected_cpus interface to handle this scenario.
     */
    policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
    cpumask_or(&sunxi_cpumask, cpumask_of(policy->cpu), &sunxi_cpumask);
    cpumask_copy(policy->cpus, &sunxi_cpumask);
    cpus_initialized++;
#endif

    return 0;
}


/*
 * get current cpu frequency configuration;
 * cfg:     cpu frequency cofniguration;
 * return:  result;
 */
static int sunxi_cpufreq_getcur(struct sunxi_cpu_freq_t *cfg)
{
    unsigned int    freq, freq0;

    if(!cfg) {
        return -EINVAL;
    }

    cfg->pll = clk_get_rate(clk_pll);
    freq = clk_get_rate(clk_cpu);
    cfg->div.cpu_div = cfg->pll / freq;
    freq0 = clk_get_rate(clk_axi);
    cfg->div.axi_div = freq / freq0;

    return 0;
}

static void sunxi_cpufreq_temp_init(void)
{
    writel(0x00a73fff,  temp_base_addr + 0x00);
    writel(0x130,       temp_base_addr + 0x04);
    writel(0x5,         temp_base_addr + 0x0c);
    writel(0x50313,     temp_base_addr + 0x10);
    writel(0x10000,     temp_base_addr + 0x18);
    return;
}


#ifdef CONFIG_PM

/*
 * cpu frequency configuration suspend;
 */
static int sunxi_cpufreq_suspend(struct cpufreq_policy *policy)
{
    return 0;
}

/*
 * cpu frequency configuration resume;
 */
static int sunxi_cpufreq_resume(struct cpufreq_policy *policy)
{
    /* invalidate last_target setting */
    last_target = ~0;
    sunxi_cpufreq_temp_init();

    return 0;
}


#else   /* #ifdef CONFIG_PM */

#define sunxi_cpufreq_suspend   NULL
#define sunxi_cpufreq_resume    NULL

#endif  /* #ifdef CONFIG_PM */


static struct cpufreq_driver sunxi_cpufreq_driver = {
    .name       = "sunxi",
    .flags      = CPUFREQ_STICKY,
    .init       = sunxi_cpufreq_init,
    .verify     = sunxi_cpufreq_verify,
    .target     = sunxi_cpufreq_target,
    .get        = sunxi_cpufreq_get,
    .getavg     = sunxi_cpufreq_getavg,
    .suspend    = sunxi_cpufreq_suspend,
    .resume     = sunxi_cpufreq_resume,
};


/*
 * cpu frequency driver init
 */
static int __init sunxi_cpufreq_initcall(void)
{
    int ret = 0;

    cpumask_clear(&sunxi_cpumask);

    clk_pll = clk_get(NULL, CLK_SYS_AC327);
    clk_cpu = clk_get(NULL, CLK_SYS_PLL1);
    clk_axi = clk_get(NULL, CLK_SYS_AXI);

    if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi)) {
        CPUFREQ_ERR("%s: could not get clock(s)\n", __func__);
        return -ENOENT;
    }

    printk("%s: clocks pll=%lu,cpu=%lu,axi=%lu\n", __func__,
           clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi));

    /* initialise current frequency configuration */
    sunxi_cpufreq_getcur(&cpu_cur);
    sunxi_cpufreq_show("cur", &cpu_cur);

    cpu_vdd = regulator_get(NULL, SUNXI_CPUFREQ_CPUVDD);
    if (IS_ERR(cpu_vdd)) {
        CPUFREQ_ERR("%s: could not get cpu vdd\n", __func__);
        return -ENOENT;
    }

    /* init cpu frequency from sysconfig */
    if(__init_freq_syscfg()) {
        CPUFREQ_ERR("%s, use default cpu max/min frequency, max freq: %uMHz, min freq: %uMHz\n",
                    __func__, cpu_freq_max/1000, cpu_freq_min/1000);
    }else{
        printk("%s, get cpu frequency from sysconfig, max freq: %uMHz, min freq: %uMHz\n",
                    __func__, cpu_freq_max/1000, cpu_freq_min/1000);
    }

    temp_base_addr = (void __iomem *)AW_VIR_TP_BASE;
    sunxi_cpufreq_temp_init();

    /* register cpu frequency driver */
    ret = cpufreq_register_driver(&sunxi_cpufreq_driver);

    return ret;
}


/*
 * cpu frequency driver exit
 */
static void __exit sunxi_cpufreq_exitcall(void)
{
    regulator_put(cpu_vdd);
    clk_put(clk_pll);
    clk_put(clk_cpu);
    clk_put(clk_axi);
    cpufreq_unregister_driver(&sunxi_cpufreq_driver);
}


MODULE_DESCRIPTION("cpufreq driver for sunxi SOCs");
MODULE_LICENSE("GPL");
module_init(sunxi_cpufreq_initcall);
module_exit(sunxi_cpufreq_exitcall);

