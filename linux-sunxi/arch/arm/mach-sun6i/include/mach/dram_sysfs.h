/*
 * arch/arm/mach-sun6i/include/mach/dram_sysfs.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __DRAM_SYSFS_H__
#define __DRAM_SYSFS_H__

#define HOST_PORT_ATTR(_name)       \
{									\
	.attr = { .name = #_name,.mode = 0644 },    \
	.show =  _name##_show,          \
	.store = _name##_store,         \
}

#define DRAM_RMCR_BASE      (AW_VIR_DRAMCOM_BASE + 0x10)
#define DRAM_MMCR_BASE      (AW_VIR_DRAMCOM_BASE + 0x30)
#define DRAM_MBACR_BASE     (AW_VIR_DRAMCOM_BASE + 0x70)
#define DRAM_RMCR_PORT(x)   ((struct dram_host_cfg_reg *)(DRAM_RMCR_BASE  + 4*(x)))
#define DRAM_MMCR_PORT(x)   ((struct dram_host_cfg_reg *)(DRAM_MMCR_BASE  + 4*(x)))
#define DRAM_MBACR_PORT(x)  ((struct dram_host_cfg_reg *)(DRAM_MBACR_BASE + 4*(x)))
#define DRAM_MBAGCR         ((struct dram_host_mbagc_reg *)(AW_VIR_DRAMCOM_BASE + 0x6c))

struct dram_host_cfg_reg {
    unsigned int    AcsNum:8;           //bit0, master n continue access number
    unsigned int    WaitState:4;        //bit8, master n wait state
    unsigned int    reserved0:4;        //bit12
    unsigned int    PrioThreshold:16;   //bit16 host port command number
};

struct dram_host_mbagc_reg{
    unsigned int    Mode:2;             //mb arbiter mode
    unsigned int    reserved0:30;
};

enum dram_host_port {
    DRAM_HOST_BE0   = 0,
    DRAM_HOST_FE0   = 1,
    DRAM_HOST_BE1   = 2,
    DRAM_HOST_FE1   = 3,
    DRAM_HOST_CSI0  = 4,
    DRAM_HOST_CSI1  = 5,
    DRAM_HOST_TS    = 6,
    DRAM_HOST_DMAC  = 7,
    DRAM_HOST_VE    = 8,
    DRAM_HOST_MP    = 9,
    DRAM_HOST_NAND0 = 10,
    DRAM_HOST_IEP0  = 11,
    DRAM_HOST_IEP1  = 12,
    DRAM_HOST_DEU0  = 13,
    DRAM_HOST_DEU1  = 14,
    DRAM_HOST_NAND1 = 15,
    DRAM_MBA_M0     = 16,
    DRAM_MBA_M1     = 17,
    DRAM_MBA_M2     = 18,
    DRAM_MBA_M3     = 19,
    DRAM_MBA_M4     = 20,
    DRAM_MBA_M5     = 21,
};

enum dram_host_type {
    DRAM_TYPE_RM    = 0,
    DRAM_TYPE_MM    = 1,
    DRAM_TYPE_MBA   = 2,
};

struct dram_type_id_tbl {
    enum dram_host_type type;
    enum dram_host_port id;
};

struct dram_id_serial_tbl {
    enum dram_host_port id;
    int serial;
};

int dram_host_port_wait_state_set(enum dram_host_port port, unsigned int state);
int dram_host_port_wait_state_get(enum dram_host_port port);
int dram_host_port_prio_threshold_set(enum dram_host_port port, unsigned int level);
int dram_host_port_prio_threshold_get(enum dram_host_port port);
int dram_host_port_acs_num_set(enum dram_host_port port, unsigned int num);
int dram_host_port_acs_num_get(enum dram_host_port port);
int dram_host_port_arbiter_mode_set(unsigned int mode);
int dram_host_port_arbiter_mode_get(void);

#endif  /* __DRAM_SYSFS_H__ */

