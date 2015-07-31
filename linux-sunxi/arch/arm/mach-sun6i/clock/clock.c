/*
 *  arch/arm/mach-sun6i/clock/clock.c
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * kevin.z.m (kevin@allwinnertech.com)
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <mach/clock.h>
#include <mach/sys_config.h>
#include "ccm_i.h"

// alloc memory for store clock informatioin
__ccu_clk_t          aw_clock[AW_CCU_CLK_CNT];
static struct clk_lookup    lookups[AW_CCU_CLK_CNT];

/*
*********************************************************************************************************
*                           clk_init
*
*Description: clock management initialise.
*
*Arguments  : none
*
*Return     : result
*               0,  initialise successed;
*              -1,  initialise failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int clk_init(void)
{
    int             i;
    struct clk      *clk;
    __u64           rate;
    script_item_u   script_item;

    CCU_DBG("aw clock manager init!\n");

    //initialise clock controller unit
    aw_ccu_init();
    //clear the data structure
    memset((void *)aw_clock, 0, sizeof(aw_clock));
    memset((void *)lookups, 0, sizeof(lookups));
    for(i=0; i<AW_CCU_CLK_CNT; i++) {
        /* initiate clk */
        if(aw_ccu_get_clk(i, &aw_clock[i]) != 0) {
            CCU_ERR("try toc get clock(id:%d) informaiton failed!\n", i);
        }
#ifdef CCU_LOCK_LIUGANG_20120930
	/* init clk spin lock */
	CCU_LOCK_INIT(&aw_clock[i].lock);
#endif /* CCU_LOCK_LIUGANG_20120930 */
        /* register clk device */
        lookups[i].con_id = aw_clock[i].aw_clk->name;
        lookups[i].clk    = &aw_clock[i];
        clkdev_add(&lookups[i]);
    }
    /* initiate some clocks */
    lookups[AW_MOD_CLK_SMPTWD].dev_id = "smp_twd";

    /* config plls */
    if(script_get_item("clock", "pll3", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll3 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL3];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }
    if(script_get_item("clock", "pll4", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll4 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL4];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }

    clk = &aw_clock[AW_SYS_CLK_PLL6];
    if(script_get_item("clock", "pll6", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll6 to %d Mhz\n", script_item.val);
        if((script_item.val < 30) || (script_item.val > 1800)) {
            script_item.val = 600;
        }
    } else{
        script_item.val = 600;
    }
    clk_enable(clk);
    clk_set_rate(clk, script_item.val*1000000);


    if(script_get_item("clock", "pll7", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll7 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL7];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }
    if(script_get_item("clock", "pll8", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll8 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL8];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }
    if(script_get_item("clock", "pll9", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll9 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL9];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }
    if(script_get_item("clock", "pll10", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config pll10 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 30) || (script_item.val > 600))) {
            clk = &aw_clock[AW_SYS_CLK_PLL10];
            clk_enable(clk);
            clk_set_rate(clk, script_item.val*1000000);
        }
    }

    /* switch ahb clock to pll6 */
    aw_ccu_switch_ahb_2_pll6();
    clk = &aw_clock[AW_SYS_CLK_AHB1];
    rate = clk_round_rate(clk, AHB1_FREQ_MAX);
    clk_set_rate(clk, rate);

    if(script_get_item("clock", "apb2", &script_item) == SCIRPT_ITEM_VALUE_TYPE_INT) {
        CCU_INF("script config apb2 to %d Mhz\n", script_item.val);
        if(!((script_item.val < 5) || (script_item.val > 120))) {
            aw_ccu_switch_apb_2_pll6();
            clk = &aw_clock[AW_SYS_CLK_APB2];
            rate = clk_round_rate(clk, script_item.val*1000000);
            clk_set_rate(clk, rate);
            clk_enable(clk);
        }
    }

    return 0;
}
arch_initcall(clk_init);


int __clk_get(struct clk *hclk)
{
    /* just noitify, do nothing now, if you want record if the clock used count, you can add code here */
    return 1;
}


void __clk_put(struct clk *clk)
{
    /* just noitify, do nothing now, if you want record if the clock used count, you can add code here */
    return;
}


int clk_enable(struct clk *clk)
{
    DEFINE_FLAGS(flags);

    if((clk == NULL) || IS_ERR(clk))
        return -EINVAL;
    if(!clk->ops || !clk->ops->set_status)
        return 0;

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);

    CCU_LOCK(&clk->lock, flags);

    if(!clk->enable) {
        clk->ops->set_status(clk->aw_clk->id, AW_CCU_CLK_ON);
    }
    clk->enable++;

    CCU_UNLOCK(&clk->lock, flags);
    return 0;
}
EXPORT_SYMBOL(clk_enable);


void clk_disable(struct clk *clk)
{
    DEFINE_FLAGS(flags);

    if(clk == NULL || IS_ERR(clk) || !clk->enable)
        return;
    if(!clk->ops || !clk->ops->set_status)
        return;

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);

    CCU_LOCK(&clk->lock, flags);

    clk->enable--;
    if(clk->enable){
        CCU_UNLOCK(&clk->lock, flags);
        return;
    }
    clk->ops->set_status(clk->aw_clk->id, AW_CCU_CLK_OFF);

    CCU_UNLOCK(&clk->lock, flags);
    return;
}
EXPORT_SYMBOL(clk_disable);


unsigned long clk_get_rate(struct clk *clk)
{
    unsigned long ret = 0;
    DEFINE_FLAGS(flags);

    if((clk == NULL) || IS_ERR(clk))
        return 0;
    if(!clk->ops || !clk->ops->get_rate)
        return 0;

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);

    CCU_LOCK(&clk->lock, flags);

    clk->aw_clk->rate = clk->ops->get_rate(clk->aw_clk->id);
    ret = (unsigned long)clk->aw_clk->rate;

    CCU_UNLOCK(&clk->lock, flags);

    CCU_DBG("%s:%d:%s: %s (rate = %lu) !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, ret);

    return ret;
}
EXPORT_SYMBOL(clk_get_rate);


long clk_round_rate(struct clk *clk, unsigned long rate)
{
    unsigned long ret;

    DEFINE_FLAGS(flags);

    if(clk == NULL || IS_ERR(clk))
        return -1;
    if(!clk->ops || !clk->ops->round_rate)
        return rate;

    CCU_DBG("%s:%d:%s: %s (rate = %lu)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, rate);

    CCU_LOCK(&clk->lock, flags);
    ret = clk->ops->round_rate(clk->aw_clk->id, rate);
    CCU_UNLOCK(&clk->lock, flags);

    CCU_DBG("%s:%d:%s: %s (result = %lu)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, ret);

    return ret;
}
EXPORT_SYMBOL(clk_round_rate);


int clk_set_rate(struct clk *clk, unsigned long rate)
{
    DEFINE_FLAGS(flags);

    if(clk == NULL || IS_ERR(clk))
        return -1;
    if(!clk->ops || !clk->ops->get_rate || !clk->ops->set_rate)
        return 0;

    CCU_DBG("%s:%d:%s: %s (rate = %lu)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, rate);

    CCU_LOCK(&clk->lock, flags);

    if(clk->ops->set_rate(clk->aw_clk->id, rate) == 0) {
        clk->aw_clk->rate = clk->ops->get_rate(clk->aw_clk->id);
        CCU_UNLOCK(&clk->lock, flags);
        return 0;
    }

    CCU_UNLOCK(&clk->lock, flags);
    return -1;
}
EXPORT_SYMBOL(clk_set_rate);


struct clk *clk_get_parent(struct clk *clk)
{
    struct clk *clk_ret = NULL;
    DEFINE_FLAGS(flags);

    if((clk == NULL) || IS_ERR(clk)) {
        return NULL;
    }

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);

    CCU_LOCK(&clk->lock, flags);

    clk_ret = &aw_clock[clk->aw_clk->parent];

    CCU_UNLOCK(&clk->lock, flags);

    CCU_DBG("%s:%d:%s: %s (parent:%s)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, clk_ret->aw_clk->name);

    return clk_ret;
}
EXPORT_SYMBOL(clk_get_parent);


int clk_set_parent(struct clk *clk, struct clk *parent)
{
    DEFINE_FLAGS(flags);

    if((clk == NULL) || IS_ERR(clk) || (parent == NULL) || IS_ERR(parent)) {
        return -1;
    }
    if(!clk->ops || !clk->ops->get_parent || !clk->ops->set_parent || !clk->ops->get_rate)
        return 0;

    CCU_DBG("%s:%d:%s: %s (parent:%s)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, parent->aw_clk->name);

    CCU_LOCK(&clk->lock, flags);

    if(clk->ops->set_parent(clk->aw_clk->id, parent->aw_clk->id) == 0) {
        clk->aw_clk->parent = clk->ops->get_parent(clk->aw_clk->id);
        clk->aw_clk->rate   = clk->ops->get_rate(clk->aw_clk->id);

        CCU_UNLOCK(&clk->lock, flags);
        return 0;
    }

    CCU_UNLOCK(&clk->lock, flags);
    return -1;
}
EXPORT_SYMBOL(clk_set_parent);


int clk_reset(struct clk *clk, __aw_ccu_clk_reset_e reset)
{
    DEFINE_FLAGS(flags);

    if((clk == NULL) || IS_ERR(clk)) {
        return -EINVAL;
    }
    if(!clk->ops || !clk->ops->set_reset)
        return 0;

    CCU_DBG("%s:%d:%s: %s (rese:%d)!\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name, reset);

    CCU_LOCK(&clk->lock, flags);

    clk->ops->set_reset(clk->aw_clk->id, reset);
    clk->aw_clk->reset = reset;

    CCU_UNLOCK(&clk->lock, flags);
    return 0;
}
EXPORT_SYMBOL(clk_reset);

