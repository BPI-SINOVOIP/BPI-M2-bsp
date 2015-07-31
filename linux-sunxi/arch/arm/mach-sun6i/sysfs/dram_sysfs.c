/*
 * arch/arm/mach-sun6i/sysfs/dram_sysfs.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@Reuuimllatech.com>
 *
 * Dram Sysfs Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/platform.h>
#include <mach/dram_sysfs.h>

#if 0
    #define DRAM_DBG(x...)  printk(x)
    #define SHOW_HOST_CFG_REG(x)  printk("HostCfgReg: 0x%p\n",x)
#else
    #define DRAM_DBG(x...)  do{}while(0)
    #define SHOW_HOST_CFG_REG(x...)  do{}while(0)
#endif

#define RMCR_PORT_SIZE  (7)
#define MMCR_PORT_SIZE  (9)
#define MBACR_PORT_SIZE (6)

extern struct class *sunxi_class;
extern struct platform_device sunxi_dram_device;

struct device *dram_dev;
static struct kobject *rmcr_port_kobj[RMCR_PORT_SIZE];
static struct kobject *mmcr_port_kobj[MMCR_PORT_SIZE];
static struct kobject *mbacr_port_kobj[MBACR_PORT_SIZE];

static struct dram_host_cfg_reg *HostCfgReg = NULL;
static struct dram_host_mbagc_reg *HostMbagcReg = NULL;

u8 rmcr_port[RMCR_PORT_SIZE] = {DRAM_HOST_BE0,DRAM_HOST_FE0,DRAM_HOST_BE1,
            DRAM_HOST_FE1,DRAM_HOST_CSI0,DRAM_HOST_CSI1,DRAM_HOST_TS
};

u8 mmcr_port[MMCR_PORT_SIZE] = {DRAM_HOST_DMAC,DRAM_HOST_VE,DRAM_HOST_MP,
            DRAM_HOST_NAND0,DRAM_HOST_IEP0,DRAM_HOST_IEP1,DRAM_HOST_DEU0,
            DRAM_HOST_DEU1,DRAM_HOST_NAND1
};

u8 mbacr_port[MBACR_PORT_SIZE] = {DRAM_MBA_M0,DRAM_MBA_M1,DRAM_MBA_M2,
            DRAM_MBA_M3,DRAM_MBA_M4,DRAM_MBA_M5
};

static struct dram_type_id_tbl id_type[] = {
    {.id = DRAM_HOST_BE0,  .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_FE0,  .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_BE1,  .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_FE1,  .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_CSI0, .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_CSI1, .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_TS,   .type = DRAM_TYPE_RM},
    {.id = DRAM_HOST_DMAC, .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_VE,   .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_MP,   .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_NAND0,.type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_IEP0, .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_IEP1, .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_DEU0, .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_DEU1, .type = DRAM_TYPE_MM},
    {.id = DRAM_HOST_NAND1,.type = DRAM_TYPE_MM},
    {.id = DRAM_MBA_M0,    .type = DRAM_TYPE_MBA},
    {.id = DRAM_MBA_M1,    .type = DRAM_TYPE_MBA},
    {.id = DRAM_MBA_M2,    .type = DRAM_TYPE_MBA},
    {.id = DRAM_MBA_M3,    .type = DRAM_TYPE_MBA},
    {.id = DRAM_MBA_M4,    .type = DRAM_TYPE_MBA},
    {.id = DRAM_MBA_M5,    .type = DRAM_TYPE_MBA},
};

static struct dram_id_serial_tbl id_serial[] = {
    {.id = DRAM_HOST_BE0,  .serial = 0},
    {.id = DRAM_HOST_FE0,  .serial = 1},
    {.id = DRAM_HOST_BE1,  .serial = 2},
    {.id = DRAM_HOST_FE1,  .serial = 3},
    {.id = DRAM_HOST_CSI0, .serial = 4},
    {.id = DRAM_HOST_CSI1, .serial = 5},
    {.id = DRAM_HOST_TS,   .serial = 6},
    {.id = DRAM_HOST_DMAC, .serial = 0},
    {.id = DRAM_HOST_VE,   .serial = 1},
    {.id = DRAM_HOST_MP,   .serial = 2},
    {.id = DRAM_HOST_NAND0,.serial = 3},
    {.id = DRAM_HOST_IEP0, .serial = 4},
    {.id = DRAM_HOST_IEP1, .serial = 5},
    {.id = DRAM_HOST_DEU0, .serial = 6},
    {.id = DRAM_HOST_DEU1, .serial = 7},
    {.id = DRAM_HOST_NAND1,.serial = 8},
    {.id = DRAM_MBA_M0,    .serial = 0},
    {.id = DRAM_MBA_M1,    .serial = 1},
    {.id = DRAM_MBA_M2,    .serial = 2},
    {.id = DRAM_MBA_M3,    .serial = 3},
    {.id = DRAM_MBA_M4,    .serial = 4},
    {.id = DRAM_MBA_M5,    .serial = 5},
};

/* calculate host port id */
static void host_port_id_target(struct kobject *kobj, u8 *port)
{
    u8 i;

    for(i=0;i<ARRAY_SIZE(rmcr_port_kobj);i++){
        if(kobj == rmcr_port_kobj[i]){
            *port = rmcr_port[i];
            DRAM_DBG("rmcr\n");
            return;
        }
    }

    for(i=0;i<ARRAY_SIZE(mmcr_port_kobj);i++){
        if(kobj == mmcr_port_kobj[i]){
            *port = mmcr_port[i];
            DRAM_DBG("mmcr\n");
            return;
        }
    }

    for(i=0;i<ARRAY_SIZE(mbacr_port_kobj);i++){
        if(kobj == mbacr_port_kobj[i]){
            *port = mbacr_port[i];
            DRAM_DBG("mbacr\n");
            return;
        }
    }
}

static enum dram_host_type host_port_type_target(enum dram_host_port port)
{
    int i;

    for (i=0;i<22;i++) {
        if (id_type[i].id == port) {
            break;
        }
    }

    return id_type[i].type;
}

/* set wait state according to port */
int dram_host_port_wait_state_set(enum dram_host_port port, unsigned int state)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->WaitState = state;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_wait_state_set);

/* get wait state according to port */
int dram_host_port_wait_state_get(enum dram_host_port port)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->WaitState;
}
EXPORT_SYMBOL_GPL(dram_host_port_wait_state_get);


/* set priority threshold according to port */
int dram_host_port_prio_threshold_set(enum dram_host_port port, unsigned int level)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->PrioThreshold = level;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_prio_threshold_set);

/* get priority threshold according to port */
int dram_host_port_prio_threshold_get(enum dram_host_port port)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->PrioThreshold;
}
EXPORT_SYMBOL_GPL(dram_host_port_prio_threshold_get);

/* set access number according to port */
int dram_host_port_acs_num_set(enum dram_host_port port, unsigned int num)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->AcsNum = num;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_acs_num_set);

/* get access number according to port */
int dram_host_port_acs_num_get(enum dram_host_port port)
{
    int ser = id_serial[port].serial;
    int type = host_port_type_target(port);

    if (type == DRAM_TYPE_RM)
        HostCfgReg = DRAM_RMCR_PORT(ser);
    else if (type == DRAM_TYPE_MM)
        HostCfgReg = DRAM_MMCR_PORT(ser);
    else if (type == DRAM_TYPE_MBA)
        HostCfgReg = DRAM_MBACR_PORT(ser);

    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->AcsNum;
}
EXPORT_SYMBOL_GPL(dram_host_port_acs_num_get);

/* set arbiter mode */
int dram_host_port_arbiter_mode_set(unsigned int mode)
{
    HostMbagcReg = DRAM_MBAGCR;
    SHOW_HOST_CFG_REG(HostMbagcReg);
    HostMbagcReg->Mode = mode;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_arbiter_mode_set);

/* get arbiter mode */
int dram_host_port_arbiter_mode_get(void)
{

    HostMbagcReg = DRAM_MBAGCR;
    SHOW_HOST_CFG_REG(HostMbagcReg);
    return HostMbagcReg->Mode;
}
EXPORT_SYMBOL_GPL(dram_host_port_arbiter_mode_get);

static ssize_t wait_state_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
    return sprintf(buf, "%u\n", dram_host_port_wait_state_get(port));
}

static ssize_t wait_state_store(struct kobject *kobj, struct kobj_attribute *attr,
            const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    u8 port = 0;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    host_port_id_target(kobj, &port);
    dram_host_port_wait_state_set(port, input);
    return count;
}

static ssize_t priority_threshold_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
    return sprintf(buf, "%u\n", dram_host_port_prio_threshold_get(port));
}

static ssize_t priority_threshold_store(struct kobject *kobj, struct kobj_attribute *attr,
            const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    u8 port = 0;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    host_port_id_target(kobj, &port);
    dram_host_port_prio_threshold_set(port, input);
    return count;
}

static ssize_t access_num_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
    return sprintf(buf, "%u\n", dram_host_port_acs_num_get(port));
}

static ssize_t access_num_store(struct kobject *kobj, struct kobj_attribute *attr,
            const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    u8 port = 0;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    host_port_id_target(kobj, &port);
    dram_host_port_acs_num_set(port, input);
    return count;
}

static struct kobj_attribute wait_state         = HOST_PORT_ATTR(wait_state);
static struct kobj_attribute priority_threshold = HOST_PORT_ATTR(priority_threshold);
static struct kobj_attribute access_num         = HOST_PORT_ATTR(access_num);

static const struct attribute *port_attrs[] = {
    &wait_state.attr,
    &priority_threshold.attr,
    &access_num.attr,
    NULL,
};

static ssize_t show_arbiter_mode(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    return sprintf(buf, "%u\n", dram_host_port_arbiter_mode_get());
}

static ssize_t store_arbiter_mode(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dram_host_port_arbiter_mode_set(input);
    return count;
}

static DEVICE_ATTR(arbiter_mode, 0600, show_arbiter_mode, store_arbiter_mode);

static struct device_attribute *arbiter_attrs[] = {
    &dev_attr_arbiter_mode,
    NULL
};

static int __init dram_init(void)
{
    int ret, i, j, k, m, n, p, q, s;
    char mmcr_name[16] = {0};
    char mbacr_name[16] = {0};

    dram_dev = device_create(sunxi_class, &sunxi_dram_device.dev, 0, NULL,
            (char *)sunxi_dram_device.dev.platform_data);

    ret = device_create_file(dram_dev, arbiter_attrs[0]);
    if (ret)
        goto out_err1;

    for (i = 0; i < ARRAY_SIZE(rmcr_port); i++) {
        sprintf(mmcr_name, "rmcr.%d", id_serial[rmcr_port[i]].serial);
        rmcr_port_kobj[i] = kobject_create_and_add(mmcr_name, &dram_dev->kobj);
        if (!rmcr_port_kobj[i]) {
            ret = -ENOMEM;
            printk("rmcr port: Failed to create dram kobj\n");
            goto out_err2;
        }

        ret = sysfs_create_files(rmcr_port_kobj[i], port_attrs);
        if (ret) {
            printk("rmcr port: Failed to add attrs");
            goto out_err3;
        }
    }

    for (j = 0; j < ARRAY_SIZE(mmcr_port); j++) {
        sprintf(mbacr_name, "mmcr.%d", id_serial[mmcr_port[j]].serial);
        mmcr_port_kobj[j] = kobject_create_and_add(mbacr_name, &dram_dev->kobj);
        if (!mmcr_port_kobj[j]) {
            ret = -ENOMEM;
            printk("mmcr port: Failed to create dram kobj\n");
            goto out_err4;
        }

        ret = sysfs_create_files(mmcr_port_kobj[j], port_attrs);
        if (ret) {
            printk("mmcr port: Failed to add attrs");
            goto out_err5;
        }
    }

    for (k = 0; k < ARRAY_SIZE(mbacr_port); k++) {
        sprintf(mbacr_name, "mbacr.%d", id_serial[mbacr_port[k]].serial);
        mbacr_port_kobj[k] = kobject_create_and_add(mbacr_name, &dram_dev->kobj);
        if (!mbacr_port_kobj[k]) {
            ret = -ENOMEM;
            printk("mbacr port: Failed to create dram kobj\n");
            goto out_err6;
        }

        ret = sysfs_create_files(mbacr_port_kobj[k], port_attrs);
        if (ret) {
            printk("mbacr port: Failed to add attrs");
            goto out_err7;
        }
    }

    return 0;

out_err7:
    for (s = 0; s < k; s++) {
        sysfs_remove_file(mbacr_port_kobj[s], port_attrs);
    }
    if (k < ARRAY_SIZE(mbacr_port)) {
        kobject_put(mbacr_port_kobj[k]);
    }
out_err6:
    for (q = 0; q < k; q++) {
        kobject_put(mbacr_port_kobj[q]);
    }
out_err5:
    for (n = 0; n < j; n++) {
        sysfs_remove_file(mmcr_port_kobj[n], port_attrs);
    }
    if (j < ARRAY_SIZE(mmcr_port)) {
        kobject_put(mmcr_port_kobj[j]);
    }
out_err4:
    for (p = 0; p < j; p++) {
        kobject_put(mmcr_port_kobj[p]);
    }
out_err3:
    for (m = 0; m < i; m++) {
        sysfs_remove_file(rmcr_port_kobj[m], port_attrs);
    }
    if (i < ARRAY_SIZE(rmcr_port)) {
        kobject_put(rmcr_port_kobj[i]);
    }
out_err2:
    for (n = 0; n < i; n++) {
        kobject_put(rmcr_port_kobj[n]);
    }
    device_remove_file(dram_dev, arbiter_attrs[0]);
out_err1:
    return ret;
}
late_initcall(dram_init);
