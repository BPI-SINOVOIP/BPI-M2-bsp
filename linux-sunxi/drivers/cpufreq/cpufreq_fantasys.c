/*
 * drivers/cpufreq/cpufreq_fantasys.c
 *
 * copyright (c) 2012-2013 allwinnertech
 *
 * based on ondemand policy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/threads.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <mach/system.h>
#include <mach/sys_config.h>

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */
#define FANTASY_DEBUG_LEVEL     (1)

#if (FANTASY_DEBUG_LEVEL == 0 )
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)
    #define FANTASY_ERR(format,args...)
#elif(FANTASY_DEBUG_LEVEL == 1)
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#elif(FANTASY_DEBUG_LEVEL == 2)
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)     printk(KERN_WARNING "[fantasy] wrn "format,##args)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#elif(FANTASY_DEBUG_LEVEL == 3)
    #define FANTASY_DBG(format,args...)     printk(KERN_DEBUG "[fantasy] dbg "format,##args)
    #define FANTASY_WRN(format,args...)     printk(KERN_WARNING "[fantasy] wrn "format,##args)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#endif

#define DEF_CPU_UP_RATE                 (10)    /* check cpu up period is n*sample_rate=0.5 second */
#define DEF_CPU_DOWN_RATE               (10)    /* check cpu down period is n*sample_rate=0.5 second */
#define MAX_HOTPLUG_RATE                (80)    /* array size for save history sample data          */

#define DEF_MAX_CPU_LOCK                (0)
#define DEF_START_DELAY                 (0)

#define DEF_SAMPLING_RATE               (50000) /* check cpu frequency sample rate is 50ms */

#define CPU_STATE_DEBUG_TOTAL_MASK      (0x1)
#define CPU_STATE_DEBUG_CORES_MASK      (0x2)

extern int sunxi_cpufreq_getvolt(void);

extern int sunxi_cpufreq_gettemp(void);
static int temp_limit_high = 0;
static int temp_limit_low  = 0;
static int temp_limit_max_freq = 0;

/*
 * static data of cpu usage
 */
struct cpu_usage {
    unsigned int freq;              /* cpu frequency value              */
    unsigned int volt;              /* cpu voltage value                */
    unsigned int temp;              /* cpu tempreture                   */
    unsigned int loading[NR_CPUS];  /* cpu frequency loading            */
    unsigned int running[NR_CPUS];  /* cpu running list loading         */
    unsigned int iowait[NR_CPUS];   /* cpu waiting                      */
    unsigned int loading_avg;       /* system average freq loading      */
    unsigned int running_avg;       /* system average thread loading    */
    unsigned int iowait_avg;        /* system average waiting           */

};

/* record cpu history loading information */
struct cpu_usage_history {
    struct cpu_usage usage[MAX_HOTPLUG_RATE];
    unsigned int num_hist;          /* current number of history data   */
};
static struct cpu_usage_history *hotplug_history;


/*
 * get average loading of all cpu's run list
 */
static unsigned int get_rq_avg_sys(void)
{
    return nr_running_avg();
}

/*
 * get average loading of spec cpu's run list
 */
static unsigned int get_rq_avg_cpu(int cpu)
{
    return nr_running_avg_cpu(cpu);
}



/*
 * define thread loading policy table for cpu hotplug
 * if(rq > hotplug_rq[1][1]) cpu need plug in, run 4 cores;
 * if(rq < hotplug_rq[3][0]) cpu can plug out, run 2 cores;
 */
static int hotplug_rq_def[4][2] = {
    {0  , 0  },
    {0  , 350},
    {0  , 0  },
    {300, 0  },
};

/*
 * define frequncy loading policy table for cpu hotplug
 * if(freq > policy->cur * hotplug_freq[1][1]) cpu need hotplug in, run 4 cores;
 * if(freq < policy->cur * hotplug_freq[3][0]) cpu can hotplug out, run 2 cores;
 */
static int hotplug_freq_def[4][2] = {
    {0 , 0 },
    {0 , 60},
    {0 , 0 },
    {40, 0 },
};

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
/*
 * define frequency policy table for user event triger
 * switch cpu frequency to policy->max * 100% if single core currently
 * switch cpu frequency to policy->max * 80% if dule core currently
 * switch cpu frequency to policy->max * 80% if quad core currently
 */
static int usrevent_freq_def[4] = {
    100 ,
    80,
    0 ,
    80,
};
#endif

static int cpu_up_weight_tbl[10] = {
    25,
    20,
    15,
    10,
     5,
     5,
     5,
     5,
     5,
     5,
};


static int hotplug_rq[NR_CPUS][2];
static int hotplug_freq[NR_CPUS][2];

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
static int usrevent_freq[NR_CPUS];
#endif

static int cpu_up_rq_hist[DEF_CPU_UP_RATE];

#define TABLE_LENGTH (16)

struct cpufreq_dvfs {
    unsigned int freq;   /* cpu frequency */
    unsigned int volt;   /* voltage for the frequency */
};

static unsigned int table_length_syscfg = 0;
static struct cpufreq_dvfs dvfs_table_syscfg[TABLE_LENGTH];

/*
 * define data structure for dbs
 */
struct cpu_dbs_info_s {
    u64 prev_cpu_idle;          /* previous idle time statistic */
    u64 prev_cpu_iowait;        /* previous iowait time stat    */
    u64 prev_cpu_wall;          /* previous total time stat     */
    struct cpufreq_policy *cur_policy;
    struct delayed_work work;
    struct work_struct up_work;     /* cpu plug-in processor    */
    struct work_struct down_work;   /* cpu plug-out processer   */
    struct cpufreq_frequency_table *freq_table;
    int cpu;                    /* current cpu number           */
    /*
     * percpu mutex that serializes governor limit change with
     * do_dbs_timer invocation. We do not want do_dbs_timer to run
     * when user is changing the governor or limits.
     */
    struct mutex timer_mutex;   /* semaphore for protection     */
};

/*
 * define percpu variable for dbs information
 */
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
struct workqueue_struct *dvfs_workqueue;
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event);

/* number of CPUs using this policy */
static unsigned int dbs_enable;

/* cpu which to be reserved */
static int cpu_to_be_reserved;
/* suspend flag  */
static int is_suspended = false;

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);


static struct dbs_tuners {
    unsigned int sampling_rate;     /* dvfs sample rate                             */
    unsigned int io_is_busy;        /* flag to mark if iowait calculate as cpu busy */
    unsigned int cpu_up_rate;       /* history sample rate for cpu up               */
    unsigned int cpu_down_rate;     /* history sample rate for cpu down             */
    unsigned int pulse_freq_delay;  /* delay based on pulse for cpu frequency down  */
    unsigned int input_freq_delay;  /* delay based on input for cpu frequency down  */
    unsigned int max_cpu_lock;      /* max count of online cpu, user limit          */
    atomic_t hotplug_lock;          /* lock cpu online number, disable plug-in/out  */
    unsigned int dvfs_debug;        /* dvfs debug flag, print dbs information       */
    unsigned int cpu_state_debug;   /* cpu status debug flag, print cpu detail info */
} dbs_tuners_ins = {
    .cpu_up_rate = DEF_CPU_UP_RATE,
    .cpu_down_rate = DEF_CPU_DOWN_RATE,
    .pulse_freq_delay = 0,
    .input_freq_delay = 0,
    .max_cpu_lock = DEF_MAX_CPU_LOCK,
    .hotplug_lock = ATOMIC_INIT(0),
    .dvfs_debug = 0,
    .cpu_state_debug = 0,
};


struct cpufreq_governor cpufreq_gov_fantasys = {
    .name                   = "fantasys",
    .governor               = cpufreq_governor_dbs,
    .owner                  = THIS_MODULE,
};


/*
 * CPU hotplug lock interface
 */
atomic_t g_hotplug_lock = ATOMIC_INIT(0);

/*
 * CPU max power flag
 */
atomic_t g_max_power_flag = ATOMIC_INIT(0);

/*
 * CPU overclock flag
 */
atomic_t g_overclock_flag = ATOMIC_INIT(0);

/*
 * apply cpu hotplug lock, up or down cpu
 */
static void apply_hotplug_lock(void)
{
    int online, possible, lock, flag;
    struct work_struct *work;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    online = num_online_cpus();
    possible = num_possible_cpus();
    lock = atomic_read(&g_hotplug_lock);
    flag = lock - online;

    if (flag == 0)
        return;

    work = flag > 0 ? &dbs_info->up_work : &dbs_info->down_work;

    FANTASY_DBG("%s online:%d possible:%d lock:%d flag:%d %d\n",
         __func__, online, possible, lock, flag, (int)abs(flag));

    queue_work_on(dbs_info->cpu, dvfs_workqueue, work);
}

/*
 * lock cpu number, the number of onlie cpu should less then num_core
 */
int cpufreq_fantasys_cpu_lock(int num_core)
{
    int prev_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (num_core < 1 || num_core > num_possible_cpus()) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    prev_lock = atomic_read(&g_hotplug_lock);
    if (prev_lock != 0 && prev_lock < num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, num_core);
    apply_hotplug_lock();
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}

/*
 * unlock cpu hotplug number
 */
int cpufreq_fantasys_cpu_unlock(int num_core)
{
    int prev_lock = atomic_read(&g_hotplug_lock);
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (prev_lock != num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, 0);
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)                    \
static ssize_t show_##file_name                        \
(struct kobject *kobj, struct attribute *attr, char *buf)        \
{                                    \
    return sprintf(buf, "%u\n", dbs_tuners_ins.object);        \
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(cpu_up_rate, cpu_up_rate);
show_one(cpu_down_rate, cpu_down_rate);
show_one(max_cpu_lock, max_cpu_lock);
show_one(dvfs_debug, dvfs_debug);
show_one(cpu_state_debug, cpu_state_debug);
static ssize_t show_hotplug_lock(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_hotplug_lock));
}

static ssize_t show_max_power(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_max_power_flag));
}

static ssize_t show_overclock(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_overclock_flag));
}

static ssize_t show_pulse(struct kobject *kobj, struct attribute *attr, char *buf)
{
    //not atomic and may not exactly.
    return sprintf(buf, "%d\n", dbs_tuners_ins.pulse_freq_delay);
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.sampling_rate = input;
    return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    dbs_tuners_ins.io_is_busy = !!input;
    return count;
}


static ssize_t store_cpu_up_rate(struct kobject *a, struct attribute *b,
                 const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.cpu_up_rate = (unsigned int)min((int)input, (int)MAX_HOTPLUG_RATE);
    return count;
}

static ssize_t store_cpu_down_rate(struct kobject *a, struct attribute *b,
                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.cpu_down_rate = (unsigned int)min((int)input, (int)MAX_HOTPLUG_RATE);
    return count;
}

static ssize_t store_max_cpu_lock(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    input = min(input, num_possible_cpus());
    if ((input == 1) || (input == 2))
        input = 2;

    if ((input == 3) || (input == 4))
        input = 4;
    dbs_tuners_ins.max_cpu_lock = input;
    return count;
}

static ssize_t store_hotplug_lock(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    int prev_lock;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    input = min(input, num_possible_cpus());
    prev_lock = atomic_read(&dbs_tuners_ins.hotplug_lock);

    if (prev_lock)
        cpufreq_fantasys_cpu_unlock(prev_lock);

    if (input == 0) {
        atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
        return count;
    }

    if ((input == 1) || (input == 2))
        input = 2;

    if ((input == 3) || (input == 4))
        input = 4;

    ret = cpufreq_fantasys_cpu_lock(input);
    if (ret) {
        printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
            atomic_read(&g_hotplug_lock), input);
        return ret;
    }

    atomic_set(&dbs_tuners_ins.hotplug_lock, input);

    return count;
}

static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.dvfs_debug = input > 0;
    return count;
}

static ssize_t store_cpu_state_debug(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    if (input > 3) {
        return -EINVAL;
    } else {
        dbs_tuners_ins.cpu_state_debug = input;
    }

    return count;
}

static ssize_t store_max_power(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret, nr_up, cpu, max_power_flag;
    struct cpu_dbs_info_s *dbs_info;

    printk(KERN_WARNING "store maxPower \"%s\" (pid %i)\n", current->comm, current->pid);
    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    ret = sscanf(buf, "%u", &input);
    if (ret != 1) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_max_power_flag, !!input);
    max_power_flag = atomic_read(&g_max_power_flag);
    if (strcmp(dbs_info->cur_policy->governor->name, "fantasys")) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    if (max_power_flag == 1) {
        if (atomic_read(&g_overclock_flag)) {
            if (dbs_info->cur_policy->max < dbs_info->cur_policy->cpuinfo.max_freq) {
                dbs_info->cur_policy->max = dbs_info->cur_policy->cpuinfo.max_freq;
                dbs_info->cur_policy->user_policy.max = dbs_info->cur_policy->cpuinfo.max_freq;
            }
        }

        __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        nr_up = nr_cpu_ids - num_online_cpus();
        for_each_cpu_not(cpu, cpu_online_mask) {
            if (cpu == 0)
                continue;

            if (nr_up-- == 0)
                break;

            cpu_up(cpu);
        }
    } else if (max_power_flag == 0) {
        if (atomic_read(&g_overclock_flag)) {
            if (dbs_info->cur_policy->max > temp_limit_max_freq) {
                dbs_info->cur_policy->max = temp_limit_max_freq;
                dbs_info->cur_policy->user_policy.max = temp_limit_max_freq;
            }
        }
        hotplug_history->num_hist = 0;
    } else {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }
    mutex_unlock(&dbs_info->timer_mutex);

    printk(KERN_WARNING "store maxPower out\n");
    return count;
}

static ssize_t store_pulse(struct kobject *a, struct attribute *b,
								const char *buf, size_t count)
{
    unsigned int input;
    int ret, nr_up, cpu, hotplug_lock, max_power_lock;
    struct cpu_dbs_info_s *dbs_info;

    printk(KERN_WARNING "Pulse: \"%s\" (pid %i)\n", current->comm, current->pid);
    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    ret = sscanf(buf, "%u", &input);
    if (ret != 1) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    if (strcmp(dbs_info->cur_policy->governor->name, "fantasys")) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    hotplug_lock = atomic_read(&g_hotplug_lock);
    if (hotplug_lock)
        goto unlock;

    max_power_lock = atomic_read(&g_max_power_flag);
    if (max_power_lock)
        goto unlock;

    if (atomic_read(&g_overclock_flag)) {
        if (dbs_info->cur_policy->max < dbs_info->cur_policy->cpuinfo.max_freq) {
            dbs_info->cur_policy->max = dbs_info->cur_policy->cpuinfo.max_freq;
            dbs_info->cur_policy->user_policy.max = dbs_info->cur_policy->cpuinfo.max_freq;
        }
    }

    __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
    nr_up = nr_cpu_ids - num_online_cpus();
    for_each_cpu_not(cpu, cpu_online_mask) {
        if (cpu == 0)
            continue;

        if (nr_up-- == 0)
            break;

        cpu_up(cpu);
    }

    /* avoid plug out cpu in the current cycle, reset the stat cycle */
    hotplug_history->num_hist = 0;
    /* delay 40 sampling cycle for frequency down */
    if(input > 60 || input < 2)
        dbs_tuners_ins.pulse_freq_delay = 40;
    else
        dbs_tuners_ins.pulse_freq_delay = input*20;

unlock:
    mutex_unlock(&dbs_info->timer_mutex);

    printk(KERN_WARNING "Pulse out\n");

    return count;
}

static ssize_t store_overclock(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    struct cpu_dbs_info_s *dbs_info;
    int ret;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    if (strcmp(dbs_info->cur_policy->governor->name, "fantasys")) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    ret = sscanf(buf, "%u", &input);
    if (ret != 1) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    if (input == 1) {
        if (dbs_info->cur_policy->max < dbs_info->cur_policy->cpuinfo.max_freq) {
            dbs_info->cur_policy->max = dbs_info->cur_policy->cpuinfo.max_freq;
            dbs_info->cur_policy->user_policy.max = dbs_info->cur_policy->cpuinfo.max_freq;
        }
    } else if (input == 0) {
        if (dbs_info->cur_policy->max > temp_limit_max_freq) {
            dbs_info->cur_policy->max = temp_limit_max_freq;
            dbs_info->cur_policy->user_policy.max = temp_limit_max_freq;
            if (dbs_info->cur_policy->cur > temp_limit_max_freq) {
                __cpufreq_driver_target(dbs_info->cur_policy, temp_limit_max_freq, CPUFREQ_RELATION_H);
            }
        }
    }

    atomic_set(&g_overclock_flag, input);
    mutex_unlock(&dbs_info->timer_mutex);

    return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(cpu_up_rate);
define_one_global_rw(cpu_down_rate);
define_one_global_rw(max_cpu_lock);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);
define_one_global_rw(cpu_state_debug);
define_one_global_rw(max_power);
define_one_global_rw(overclock);
define_one_global_rw(pulse);

static struct attribute *dbs_attributes[] = {
    &sampling_rate.attr,
    &io_is_busy.attr,
    &cpu_up_rate.attr,
    &cpu_down_rate.attr,
    &max_cpu_lock.attr,
    &hotplug_lock.attr,
    &dvfs_debug.attr,
    &cpu_state_debug.attr,
    &max_power.attr,
    &overclock.attr,
    &pulse.attr,
    NULL
};

static struct attribute_group dbs_attr_group = {
    .attrs = dbs_attributes,
    .name = "fantasys",
};


/*
 * cpu hotplug, just plug in 2 cpu
 */
static void cpu_up_work(struct work_struct *work)
{
    int cpu, nr_up, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_up = (hotplug_lock - online) > 0? (hotplug_lock-online) : 0;
    } else {
        nr_up = 2;
    }

    for_each_cpu_not(cpu, cpu_online_mask) {
        if (cpu == 0)
            continue;

        if (nr_up-- == 0)
            break;

        FANTASY_WRN("cpu up:%d\n", cpu);
        cpu_up(cpu);
    }
    mutex_unlock(&dbs_info->timer_mutex);
}

/*
 * cpu hotplug, cpu plugout
 */
static void cpu_down_work(struct work_struct *work)
{
    int cpu, nr_down, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_down = (online - hotplug_lock) > 0? (online-hotplug_lock) : 0;
    } else {
        nr_down = 2;
        if ((cpu_to_be_reserved > 0) && (cpu_to_be_reserved < nr_cpu_ids)) {
            for_each_online_cpu(cpu) {
                if ((cpu == 0) || (cpu == cpu_to_be_reserved))
                    continue;

                if (nr_down-- == 0)
                    break;

                FANTASY_DBG("cpu down:%d\n", cpu);
                cpu_down(cpu);
            }
        }
        goto unlock;
    }

    for_each_online_cpu(cpu) {
        if (cpu == 0)
            continue;

        if (nr_down-- == 0)
            break;

        FANTASY_DBG("cpu down:%d\n", cpu);
        cpu_down(cpu);
    }

unlock:
    mutex_unlock(&dbs_info->timer_mutex);
}

/* check cpu if need to run max power according to one core's loading/running/iowait */
static int check_up(int num_hist)
{
    unsigned int cpu_rq_avg = 0;
    int i, online, index_mod, index_cur, up_rq;
    int hotplug_lock = atomic_read(&g_hotplug_lock);
    int max_power_lock = atomic_read(&g_max_power_flag);

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    /* max power has been locked, do nothing */
    if (max_power_lock > 0)
        return 0;

    online = num_online_cpus();
    up_rq = hotplug_rq[online-1][1];

    /* check if count of the cpu reached the max value */
    if (online == num_possible_cpus())
        return 0;

    if (dbs_tuners_ins.max_cpu_lock != 0 && online >= dbs_tuners_ins.max_cpu_lock)
        return 0;

    index_mod = num_hist%dbs_tuners_ins.cpu_up_rate;
    cpu_up_rq_hist[index_mod] = hotplug_history->usage[num_hist].running_avg;

    for (i=0; i<dbs_tuners_ins.cpu_up_rate ;i++) {
        index_cur = index_mod - i;
        index_cur = index_cur < 0 ? index_cur + dbs_tuners_ins.cpu_up_rate : index_cur;
        cpu_rq_avg += cpu_up_rq_hist[index_cur] * cpu_up_weight_tbl[i];
    }

    cpu_rq_avg /= 100;
    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s: cpu_rq_avg:%u, up_rq:%u\n", __func__, cpu_rq_avg, up_rq);
    }

    if (cpu_rq_avg > up_rq) {
        FANTASY_WRN("quick response, cpu_rq_avg: %d>%d\n", cpu_rq_avg, up_rq);
        hotplug_history->num_hist = 0;
        memset(cpu_up_rq_hist, 0, sizeof(cpu_up_rq_hist));
        return 1;
    }

    return 0;
}

/*
 * check if need plug out one cpu core
 */
static int check_down(struct cpufreq_policy *policy)
{
    struct cpu_usage *usage;
    int i, cpu, online, down_rq;
    int avg_rq = 0,  max_rq_avg = 0;
    int down_rate = dbs_tuners_ins.cpu_down_rate;
    int num_hist = hotplug_history->num_hist;
    int hotplug_lock = atomic_read(&g_hotplug_lock);
    int max_power_lock = atomic_read(&g_max_power_flag);
    unsigned int cpu_down_freq, cpus_load_avg[NR_CPUS];
    unsigned long rate = 0;

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    /* max power has been locked, do nothing */
    if (max_power_lock > 0)
        return 0;

    /* pulse event, do nothing */
    if (dbs_tuners_ins.pulse_freq_delay > 0)
        return 0;

    online = num_online_cpus();
    down_rq = hotplug_rq[online-1][0];
    cpu_down_freq = policy->max * hotplug_freq[online-1][0];

    /* just 2 cores, can't be plug out */
    if (online == 2)
        return 0;

    /* if count of online cpu is larger than the max value, plug out cpu */
    if (dbs_tuners_ins.max_cpu_lock != 0 && online > dbs_tuners_ins.max_cpu_lock)
        return 1;

    /* check if reached the switch point */
    if (num_hist == 0 || num_hist % down_rate)
        return 0;

    for_each_online_cpu(cpu) {
        cpus_load_avg[cpu] = 0;
    }

    /* check system average loading */
    for (i = num_hist - 1; i >= num_hist - down_rate; --i) {
        usage = &hotplug_history->usage[i];
        avg_rq = usage->running_avg;
        max_rq_avg = max(max_rq_avg, avg_rq);

        for_each_online_cpu(cpu) {
            cpus_load_avg[cpu] += usage->loading[cpu];
        }
    }

    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s: max_rq_avg:%u, down_rq:%u\n", __func__, max_rq_avg, down_rq);
        for_each_online_cpu(cpu) {
            printk("%s: cpus_load_avg[%d]:%u\n", __func__, cpu, cpus_load_avg[cpu]);
        }
    }

    if (max_rq_avg <= down_rq) {
        FANTASY_WRN("cpu need plugout, max_rq_avg: %d<%d\n", max_rq_avg, down_rq);
        for_each_online_cpu(cpu) {
            if (cpu == 0)
                continue;

            if (rate < cpus_load_avg[cpu]) {
                cpu_to_be_reserved = cpu;
                rate = cpus_load_avg[cpu];
            }
        }
        hotplug_history->num_hist = 0;
        /* need plug out cpu, reset the stat cycle */
        return 1;
    }

    return 0;
}


/* idle rate coarse adjust for cpu frequency down */
#define FANTASY_CPUFREQ_LOAD_MIN_RATE(freq)         \
        (freq<100000? 30 : (freq<200000? 40 : (freq<600000? 60 : (freq<900000? 70 : 70))))

/*
 * minimum rate for idle task, if idle rate is less than this
 * value, cpu frequency should be adjusted to the mauximum value
*/
#define FANTASY_CPUFREQ_LOAD_MAX_RATE(freq)         \
        (freq<100000? 70 : (freq<200000? 70 : (freq<600000? 80 : (freq<900000? 90 : 95))))

/*
 * define frequency limit for io-wait rate, cpu frequency should be limit to higher if io-wait higher
 */
#define FANTASY_CPUFREQ_IOW_LIMIT_RATE(iow)         \
    (iow<10? (iow>0? 20 : 0) : (iow<20? 40 : (iow<40? 60 : (iow<60? 80 : 100))))

static inline unsigned int __get_freq_vftbl(unsigned int freq)
{
    struct cpufreq_dvfs *dvfs_inf = NULL;
    dvfs_inf = &dvfs_table_syscfg[0];

    while((dvfs_inf+1)->freq > freq) dvfs_inf++;

    return dvfs_inf->freq;
}



static unsigned int __get_freq_by_temp(struct cpufreq_policy *policy, unsigned int freq, int num_hist)
{
    unsigned int freq_to_set = freq;
    unsigned int temp = hotplug_history->usage[num_hist].temp;

    if (atomic_read(&g_max_power_flag))
        freq_to_set = policy->max;

    if (atomic_read(&g_overclock_flag) == 0)
        goto out;

    if (dbs_tuners_ins.dvfs_debug)
        printk("tempreture=%uC\n", temp);

    if (temp > temp_limit_high) {
        /* cpu max to temp_limit_max_freq */
        if (freq_to_set > temp_limit_max_freq) {
            freq_to_set = temp_limit_max_freq;
            policy->max = temp_limit_max_freq;
            policy->user_policy.max = temp_limit_max_freq;
            if (dbs_tuners_ins.dvfs_debug)
                printk("tempreture limit: %uC>%dC ,cpu max freq to %uKHz\n", \
                        temp, temp_limit_high, freq_to_set);
        }
    } else if (temp < temp_limit_low) {
        if (policy->max < policy->cpuinfo.max_freq) {
            policy->max = policy->cpuinfo.max_freq;
            policy->user_policy.max = policy->cpuinfo.max_freq;
            if (dbs_tuners_ins.dvfs_debug) {
                printk("tempreture limit: %uC<%dC, cpu max freq to %uKHz\n", \
                        temp, temp_limit_low, policy->max);
            }
        }
    }

out:
    return freq_to_set;
}


static int check_freq_up_down(struct cpu_dbs_info_s *this_dbs_info, unsigned int *target, int num_hist)
{
    int cpu;
    unsigned int max_load = 0, max_iow = 0;
    unsigned int freq_load, freq_iow, index;
    struct cpufreq_policy *policy = this_dbs_info->cur_policy;

    if (dbs_tuners_ins.pulse_freq_delay) {
        dbs_tuners_ins.pulse_freq_delay--;
        /* tempreture check */
        *target = __get_freq_by_temp(policy, policy->cur, num_hist);
        if (dbs_tuners_ins.pulse_freq_delay == 0) {
            if (atomic_read(&g_overclock_flag)) {
                if (atomic_read(&g_max_power_flag) == 0) {
                    if (policy->max > temp_limit_max_freq) {
                        policy->max = temp_limit_max_freq;
                        policy->user_policy.max = temp_limit_max_freq;
                    }
                }
            }
        }
        return 0;
    }

    if (dbs_tuners_ins.input_freq_delay) {
        dbs_tuners_ins.input_freq_delay--;
        /* tempreture check */
        *target = __get_freq_by_temp(policy, policy->cur, num_hist);
        return 0;
    }

    for_each_online_cpu(cpu) {
        max_load = max(max_load, hotplug_history->usage[num_hist].loading[cpu]);
        max_iow = max(max_iow, hotplug_history->usage[num_hist].iowait[cpu]);
    }

    if (max_load > FANTASY_CPUFREQ_LOAD_MAX_RATE(policy->cur)) {
        if (dbs_tuners_ins.dvfs_debug)
            printk("%s(%d): max_load=%d, cur_freq=%d, load_rate=%d\n",  \
                __func__, __LINE__, max_load, policy->cur, FANTASY_CPUFREQ_LOAD_MAX_RATE(policy->cur));
        /* tempreture check */
        *target = __get_freq_by_temp(policy, __get_freq_vftbl(policy->cur), num_hist);
        return 1;
    } else if (max_load >= FANTASY_CPUFREQ_LOAD_MIN_RATE(policy->cur)) {
        /* tempreture check */
        *target = __get_freq_by_temp(policy, policy->cur, num_hist);
        return 0;
    }

    /*
     * calculate freq load
     * freq_load = (cur_freq * max_load) / ((min_rate + max_rate)/2)
     */
    freq_load = (policy->cur * max_load) / 100;
    freq_load = (FANTASY_CPUFREQ_LOAD_MIN_RATE(freq_load) + FANTASY_CPUFREQ_LOAD_MAX_RATE(freq_load)) >> 1;
    freq_load = (policy->cur * max_load) / freq_load;

    /* check cpu io-waiting */
    freq_iow = (policy->max*FANTASY_CPUFREQ_IOW_LIMIT_RATE(max_iow))/100;

    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s(%d): max_load=%d, cur_freq=%d, load_rate=%d, freq_load=%d\n", \
            __func__, __LINE__, max_load, policy->cur, FANTASY_CPUFREQ_LOAD_MIN_RATE(policy->cur), freq_load);
        printk("%s(%d): max_iow=%d, limit_rate=%d, freq_iow=%d\n", \
            __func__, __LINE__, max_iow, FANTASY_CPUFREQ_IOW_LIMIT_RATE(max_iow), freq_iow);
    }

    /* select target frequency */
    *target = max(freq_load, freq_iow);

    if (cpufreq_frequency_table_target(policy, this_dbs_info->freq_table, *target, CPUFREQ_RELATION_L, &index)) {
        FANTASY_ERR("%s: failed to get next lowest frequency\n", __func__);
        /* tempreture check */
        *target = __get_freq_by_temp(policy, policy->max, num_hist);
        return 1;
    }

    /* tempreture check */
    *target = __get_freq_by_temp(policy, this_dbs_info->freq_table[index].frequency, num_hist);

    return 1;
}


#if ((LINUX_VERSION_CODE) < (KERNEL_VERSION(3, 3, 0)))
static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
                            cputime64_t *wall)
{
    cputime64_t idle_time;
    cputime64_t cur_wall_time;
    cputime64_t busy_time;

    cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
    busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
            kstat_cpu(cpu).cpustat.system);

    busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
    busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
    busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
    busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

    idle_time = cputime64_sub(cur_wall_time, busy_time);
    if (wall)
        *wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

    return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
    u64 idle_time = get_cpu_idle_time_us(cpu, wall);

    if (idle_time == -1ULL)
        return get_cpu_idle_time_jiffy(cpu, wall);

    return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
    u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

    if (iowait_time == -1ULL)
        return 0;

    return iowait_time;
}
#else
static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
    u64 idle_time;
    u64 cur_wall_time;
    u64 busy_time;

    cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

    busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
    busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

    idle_time = cur_wall_time - busy_time;
    if (wall)
        *wall = jiffies_to_usecs(cur_wall_time);

    return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
    u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

    if (idle_time == -1ULL)
        return get_cpu_idle_time_jiffy(cpu, wall);
    else
        idle_time += get_cpu_iowait_time_us(cpu, wall);

    return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
    u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

    if (iowait_time == -1ULL)
        return 0;

    return iowait_time;
}
#endif

/*
 * check if need plug in/out cpu, if need increase/decrease cpu frequency
 */
static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
    unsigned int cpu, freq_target = 0;
    struct cpufreq_policy *policy;
    int num_hist = hotplug_history->num_hist;
    int max_hotplug_rate = max((int)dbs_tuners_ins.cpu_up_rate, (int)dbs_tuners_ins.cpu_down_rate);

    policy = this_dbs_info->cur_policy;

    /* static cpu loading */
    hotplug_history->usage[num_hist].freq = policy->cur;
    hotplug_history->usage[num_hist].volt = sunxi_cpufreq_getvolt();
    hotplug_history->usage[num_hist].temp = sunxi_cpufreq_gettemp();
    hotplug_history->usage[num_hist].running_avg = get_rq_avg_sys();
    hotplug_history->usage[num_hist].loading_avg = 0;
    hotplug_history->usage[num_hist].iowait_avg = 0;
    ++hotplug_history->num_hist;

    for_each_online_cpu(cpu) {
        struct cpu_dbs_info_s *j_dbs_info;
        u64 cur_wall_time, cur_idle_time, cur_iowait_time;
        u64 prev_wall_time, prev_idle_time, prev_iowait_time;
        unsigned int idle_time, wall_time, iowait_time;
        unsigned int load = 0, iowait = 0;

        j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
        prev_wall_time = j_dbs_info->prev_cpu_wall;
        prev_idle_time = j_dbs_info->prev_cpu_idle;
        prev_iowait_time = j_dbs_info->prev_cpu_iowait;

        cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time);
        cur_iowait_time = get_cpu_iowait_time(cpu, &cur_wall_time);

        wall_time = cur_wall_time - prev_wall_time;
        j_dbs_info->prev_cpu_wall = cur_wall_time;

        idle_time = cur_idle_time - prev_idle_time;
        j_dbs_info->prev_cpu_idle = cur_idle_time;

        iowait_time = cur_iowait_time - prev_iowait_time;
        j_dbs_info->prev_cpu_iowait = cur_iowait_time;


        if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
            idle_time -= iowait_time;

        if(wall_time && (wall_time > idle_time)) {
            load = 100 * (wall_time - idle_time) / wall_time;
        } else {
            load = 0;
        }
        hotplug_history->usage[num_hist].loading[cpu] = load;

        if(wall_time && (iowait_time < wall_time)) {
            iowait = 100 * iowait_time / wall_time;
        } else {
            iowait = 0;
        }
        hotplug_history->usage[num_hist].iowait[cpu] = iowait;

        /* calculate system average loading */
        hotplug_history->usage[num_hist].running[cpu] = get_rq_avg_cpu(cpu);
        hotplug_history->usage[num_hist].loading_avg += load;
        hotplug_history->usage[num_hist].iowait_avg  += iowait;

        if (dbs_tuners_ins.cpu_state_debug & CPU_STATE_DEBUG_CORES_MASK) {
            printk("[cpu%u] %u,%u,%u,%u,%u,%u\n", cpu, \
                    hotplug_history->usage[num_hist].freq, \
                    hotplug_history->usage[num_hist].volt, \
                    hotplug_history->usage[num_hist].temp, \
                    hotplug_history->usage[num_hist].loading[cpu], \
                    hotplug_history->usage[num_hist].running[cpu], \
                    hotplug_history->usage[num_hist].iowait[cpu]);
        }
    }
    hotplug_history->usage[num_hist].loading_avg /= num_online_cpus();
    hotplug_history->usage[num_hist].iowait_avg  /= num_online_cpus();

    if (dbs_tuners_ins.cpu_state_debug & CPU_STATE_DEBUG_TOTAL_MASK) {
        printk("%u,%u,%u,%u,%u,%u,%u\n", hotplug_history->usage[num_hist].freq, \
                hotplug_history->usage[num_hist].volt, \
                hotplug_history->usage[num_hist].temp, \
                num_online_cpus(), \
                hotplug_history->usage[num_hist].loading_avg, \
                hotplug_history->usage[num_hist].running_avg, \
                hotplug_history->usage[num_hist].iowait_avg);
    }

    /* Check for every core is in high loading/running/iowait */
    if (check_up(num_hist)) {
        __cpufreq_driver_target(this_dbs_info->cur_policy, this_dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
        return;
    } else if (check_down(policy)) {
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->down_work);
    }

    if (check_freq_up_down(this_dbs_info, &freq_target, num_hist)) {
        FANTASY_WRN("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, freq_target);
        __cpufreq_driver_target(policy, freq_target, CPUFREQ_RELATION_L);
    } else {
        if (freq_target != policy->cur) {
            __cpufreq_driver_target(policy, freq_target, CPUFREQ_RELATION_L);
        }
    }

    /* check if history array is out of range */
    if (hotplug_history->num_hist == max_hotplug_rate)
        hotplug_history->num_hist = 0;
}


static void do_dbs_timer(struct work_struct *work)
{
    struct cpu_dbs_info_s *dbs_info = container_of(work, struct cpu_dbs_info_s, work.work);
    unsigned int cpu = dbs_info->cpu;
    int delay;

    mutex_lock(&dbs_info->timer_mutex);

    if (is_suspended)
        goto unlock;

    dbs_check_cpu(dbs_info);

    /* We want all CPUs to do sampling nearly on
     * same jiffy
     */
    delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

    queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);

unlock:
    mutex_unlock(&dbs_info->timer_mutex);
}


static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
    /* We want all CPUs to do sampling nearly on same jiffy */
    int delay = usecs_to_jiffies(DEF_START_DELAY * 1000 * 1000 + dbs_tuners_ins.sampling_rate);
    if (num_online_cpus() > 1)
        delay -= jiffies % delay;

    INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
    INIT_WORK(&dbs_info->up_work, cpu_up_work);
    INIT_WORK(&dbs_info->down_work, cpu_down_work);
    queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, delay + 2 * HZ);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
    cancel_delayed_work_sync(&dbs_info->work);
    cancel_work_sync(&dbs_info->up_work);
    cancel_work_sync(&dbs_info->down_work);
}


static int fantasys_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
    struct cpu_dbs_info_s *dbs_info;
    struct cpufreq_policy policy;
    int nr_up, cpu;

    if (cpufreq_get_policy(&policy, 0)) {
        printk(KERN_ERR "can not get cpu policy\n");
        return NOTIFY_BAD;
    }

    if (strcmp(policy.governor->name, "fantasys"))
        return NOTIFY_OK;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (event == PM_SUSPEND_PREPARE) {
        is_suspended = true;
        printk("sunxi cpufreq suspend ok\n");
    } else if (event == PM_POST_SUSPEND) {
        is_suspended = false;
        if (atomic_read(&g_max_power_flag)) {
            __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
            nr_up = nr_cpu_ids - num_online_cpus();
            for_each_cpu_not(cpu, cpu_online_mask) {
                if (cpu == 0)
                    continue;
                if (nr_up-- == 0)
                    break;
                cpu_up(cpu);
            }
        }
        if (dbs_tuners_ins.pulse_freq_delay) {
            __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        }
        queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, 0);
        printk("sunxi cpufreq resume ok\n");
    }

    mutex_unlock(&dbs_info->timer_mutex);
    return NOTIFY_OK;
}

static struct notifier_block fantasys_pm_notifier = {
    .notifier_call = fantasys_pm_notify,
};

/*
 * cpufreq dbs governor
 */
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info;
    unsigned int j;

    this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

    switch (event) {
        case CPUFREQ_GOV_START:
        {
            if ((!cpu_online(cpu)) || (!policy->cur))
                return -EINVAL;

            hotplug_history->num_hist = 0;

            mutex_lock(&dbs_mutex);

            dbs_enable++;
            for_each_possible_cpu(j) {
                struct cpu_dbs_info_s *j_dbs_info;
                j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
                j_dbs_info->cur_policy = policy;

                j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j, &j_dbs_info->prev_cpu_wall);
            }
            this_dbs_info->cpu = cpu;
            this_dbs_info->freq_table = cpufreq_frequency_get_table(cpu);

            /*
             * Start the timerschedule work, when this governor
             * is used for first time
             */
            if (dbs_enable == 1) {
                dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
                dbs_tuners_ins.io_is_busy = 1;
            }
            mutex_unlock(&dbs_mutex);

            mutex_init(&this_dbs_info->timer_mutex);
            dbs_timer_init(this_dbs_info);
            cpu_to_be_reserved = nr_cpu_ids;
            if (atomic_read(&g_overclock_flag)) {
                if (atomic_read(&g_max_power_flag) || dbs_tuners_ins.pulse_freq_delay) {
                    if (this_dbs_info->cur_policy->max < this_dbs_info->cur_policy->cpuinfo.max_freq) {
                        this_dbs_info->cur_policy->max = this_dbs_info->cur_policy->cpuinfo.max_freq;
                        this_dbs_info->cur_policy->user_policy.max = this_dbs_info->cur_policy->cpuinfo.max_freq;
                    }
                    __cpufreq_driver_target(this_dbs_info->cur_policy, this_dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
                } else {
                    if (this_dbs_info->cur_policy->max > temp_limit_max_freq) {
                        this_dbs_info->cur_policy->max = temp_limit_max_freq;
                        this_dbs_info->cur_policy->user_policy.max = temp_limit_max_freq;
                    }
                }
            } else {
                if (this_dbs_info->cur_policy->max > temp_limit_max_freq) {
                    this_dbs_info->cur_policy->max = temp_limit_max_freq;
                    this_dbs_info->cur_policy->user_policy.max = temp_limit_max_freq;
                }
            }

            break;
        }

        case CPUFREQ_GOV_STOP:
        {
            dbs_timer_exit(this_dbs_info);

            mutex_lock(&dbs_mutex);
            mutex_destroy(&this_dbs_info->timer_mutex);

            dbs_enable--;
            mutex_unlock(&dbs_mutex);

            break;
        }

        case CPUFREQ_GOV_LIMITS:
        {
            if (policy->max < this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->max, CPUFREQ_RELATION_H);
            else if (policy->min > this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->min, CPUFREQ_RELATION_L);

            break;
        }

        #ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
        case CPUFREQ_GOV_USRENET:
        {
            unsigned int freq_trig, index;
            /* cpu frequency limitation has changed, adjust current frequency */
            if (!mutex_trylock(&this_dbs_info->timer_mutex)) {
                FANTASY_WRN("CPUFREQ_GOV_USRENET try to lock mutex failed!\n");
                return 0;
            }

            freq_trig = (this_dbs_info->cur_policy->max*usrevent_freq[num_online_cpus() - 1])/100;
            if(!cpufreq_frequency_table_target(policy, this_dbs_info->freq_table, freq_trig, CPUFREQ_RELATION_L, &index)) {
                freq_trig = this_dbs_info->freq_table[index].frequency;
                if(this_dbs_info->cur_policy->cur < freq_trig) {
                    /* set cpu frequenc to the max value, and reset state machine */
                    FANTASY_DBG("CPUFREQ_GOV_USREVENT\n");
                    __cpufreq_driver_target(this_dbs_info->cur_policy, freq_trig, CPUFREQ_RELATION_L);
                    dbs_tuners_ins.input_freq_delay = 6;
                }
            }
            mutex_unlock(&this_dbs_info->timer_mutex);
            break;
        }
        #endif

    }
    return 0;
}


static int __init_vftable_syscfg(void)
{
    script_item_u val;
    script_item_value_type_e type = SCIRPT_ITEM_VALUE_TYPE_INVALID;
    int i ,ret = -1;
    char name[16] = {0};
    char tbl_name[16] = {0};
    enum sw_ic_ver version;

    version = sw_get_ic_ver();
    switch (version) {
        case MAGIC_VER_A:
        {
            printk("IC version A\n");
            sprintf(tbl_name, "dvfs_table");
            type = script_get_item(tbl_name, "LV_count", &val);
            break;
        }
        case MAGIC_VER_B:
        {
            printk("IC version B\n");
            sprintf(tbl_name, "dvfs_table");
            type = script_get_item(tbl_name, "LV_count", &val);
            break;
        }
        case MAGIC_VER_C:
        {
            printk("IC version C\n");
            sprintf(tbl_name, "dvfs_table");
            type = script_get_item(tbl_name, "LV_count", &val);
            break;
        }
        case MAGIC_VER_D:
        {
            printk("IC version D\n");
            sprintf(tbl_name, "ver_d_dvfs_table");
            type = script_get_item(tbl_name, "LV_count", &val);
            break;
        }
        case MAGIC_VER_NULL:
        {
            pr_err("IC version invalid\n");
            goto fail;
        }
    }

    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        FANTASY_ERR("fetch LV_count from sysconfig failed\n");
        goto fail;
    }

    table_length_syscfg = val.val;
    if(table_length_syscfg >= TABLE_LENGTH){
        FANTASY_ERR("LV_count from sysconfig is out of bounder\n");
        goto fail;
    }

    for (i = 1; i <= table_length_syscfg; i++){
        sprintf(name, "LV%d_freq", i);
        type = script_get_item(tbl_name, name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
            FANTASY_ERR("get LV%d_freq from sysconfig failed\n", i);
            goto fail;
        }
        dvfs_table_syscfg[i-1].freq = val.val / 1000;

        sprintf(name, "LV%d_volt", i);
        type = script_get_item(tbl_name, name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
            FANTASY_ERR("get LV%d_volt from sysconfig failed\n", i);
            goto fail;
        }
        dvfs_table_syscfg[i-1].volt = val.val;
    }

    return 0;

fail:
    return ret;
}

static void __vftable_show(void)
{
    int i;

    printk("-------------------CPU V-F Table--------------------\n");
    for(i = 0; i < table_length_syscfg; i++){
        printk("\tfrequency = %4dKHz \tvoltage = %4dmv\n", \
                dvfs_table_syscfg[i].freq, dvfs_table_syscfg[i].volt);
    }
    printk("-----------------------------------------------------\n");
}

static int __init_temp_syscfg(void)
{
    script_item_u val;
    script_item_value_type_e type;
    int ret = -1;
    enum sw_ic_ver version;
    char name[16] = {0};

    version = sw_get_ic_ver();
    if (MAGIC_VER_D == version) {
        sprintf(name, "ver_d_dvfs_table");
    } else {
        sprintf(name, "dvfs_table");
    }

    type = script_get_item(name, "temp_limit_freq", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        FANTASY_ERR("get temp_limit_freq from sysconfig failed\n");
        goto fail;
    }
    temp_limit_max_freq = val.val / 1000;

    type = script_get_item(name, "temp_limit_high", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        FANTASY_ERR("get temp_limit_high from sysconfig failed\n");
        goto fail;
    }
    temp_limit_high = val.val;

    type = script_get_item(name, "temp_limit_low", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        FANTASY_ERR("get temp_limit_low from sysconfig failed\n");
        goto fail;
    }
    temp_limit_low = val.val;

    if (temp_limit_low >= temp_limit_high) {
        FANTASY_ERR("temp_limit_low(%d) must be less than temp_limit_high(%d)\n", \
                    temp_limit_low, temp_limit_high);
        goto fail;
    }

    return 0;

fail:
    return ret;
}

/*
 * cpufreq governor dbs initiate
 */
static int __init cpufreq_gov_dbs_init(void)
{
    int i, ret;

    /* init policy table */
    for(i=0; i<NR_CPUS; i++) {
        hotplug_rq[i][0] = hotplug_rq_def[i][0];
        hotplug_rq[i][1] = hotplug_rq_def[i][1];

        hotplug_freq[i][0] = hotplug_freq_def[i][0];
        hotplug_freq[i][1] = hotplug_freq_def[i][1];

        #ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
        usrevent_freq[i] = usrevent_freq_def[i];
        #endif
    }
    hotplug_rq[NR_CPUS-1][1] = INT_MAX;
    hotplug_freq[NR_CPUS-1][1] = INT_MAX;

    ret = __init_vftable_syscfg();
    if (ret) {
        FANTASY_ERR("%s get V-F table failed\n", __func__);
        goto err_vftable;
    } else {
        __vftable_show();
    }

    ret = __init_temp_syscfg();
    if (ret) {
        FANTASY_ERR("%s __init_temp_syscfg failed\n", __func__);
        goto err_temp;
    } else {
        printk("temp_limit_max_freq=%dKHz, temp_limit_high=%d, temp_limit_low=%d\n", \
                temp_limit_max_freq, temp_limit_high, temp_limit_low);
    }

    hotplug_history = kzalloc(sizeof(struct cpu_usage_history), GFP_KERNEL);
    if (!hotplug_history) {
        FANTASY_ERR("%s cannot create hotplug history array\n", __func__);
        ret = -ENOMEM;
        goto err_hist;
    }

    /* create dvfs daemon */
    dvfs_workqueue = create_singlethread_workqueue("fantasys");
    if (!dvfs_workqueue) {
        pr_err("%s cannot create workqueue\n", __func__);
        ret = -ENOMEM;
        goto err_queue;
    }

    /* register cpu freq governor */
    ret = cpufreq_register_governor(&cpufreq_gov_fantasys);
    if (ret)
        goto err_reg;

    ret = sysfs_create_group(cpufreq_global_kobject, &dbs_attr_group);
    if (ret) {
        goto err_governor;
    }

    register_pm_notifier(&fantasys_pm_notifier);

    return ret;

err_governor:
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
err_reg:
    destroy_workqueue(dvfs_workqueue);
err_queue:
    kfree(hotplug_history);
err_hist:
err_vftable:
err_temp:
    return ret;
}

/*
 * cpufreq governor dbs exit
 */
static void __exit cpufreq_gov_dbs_exit(void)
{
    unregister_pm_notifier(&fantasys_pm_notifier);
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
    destroy_workqueue(dvfs_workqueue);
    kfree(hotplug_history);
}

MODULE_AUTHOR("kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("'cpufreq_fantasys' - A dynamic cpufreq/cpuhotplug governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_FANTASYS
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
