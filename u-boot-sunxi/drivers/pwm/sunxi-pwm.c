/*
 * (C) Copyright 2012
 *     tyle@allwinnertech.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */
#include <common.h>
#include <asm/arch/pwm.h>
#include <asm/arch/platform.h>
#include <sys_config.h>
#include <pwm.h>

#define sys_get_wvalue(n)   (*((volatile uint *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile uint *)(n))  = (c))   /* word output */


uint pwm_active_sta[4] = {1, 0, 0, 0};
uint pwm_pin_count[4] = {0};

user_gpio_set_t pwm_gpio_info[PWM_NUM][2];

#define sunxi_pwm_debug 0
#undef  sunxi_pwm_debug

#ifdef sunxi_pwm_debug
	#define pwm_debug(fmt,args...)	printf(fmt ,##args)
#else
	#define pwm_debug(fmt,args...)
#endif

uint sunxi_pwm_read_reg(uint offset)
{
    uint value = 0;

   value = sys_get_wvalue(PWM03_BASE + offset);

    return value;
}

uint sunxi_pwm_write_reg(uint offset, uint value)
{
    sys_put_wvalue(PWM03_BASE + offset, value);

    return 0;
}

void sunxi_pwm_get_sys_config(int pwm)
{
    int ret, val;
    char primary_key[25];
    user_gpio_set_t gpio_info[1];

    sprintf(primary_key, "pwm%d_para", pwm);
    ret = script_parser_fetch(primary_key, "pwm_used", &val, 1);
    if(ret < 0) {
        pwm_debug("fetch script data fail\n");
        } else {
            if(val == 1) {
                ret = script_parser_fetch(primary_key, "pwm_positive", (int *)&gpio_info, sizeof(user_gpio_set_t) / sizeof(int));
                if(ret < 0) {
                    pwm_debug("fetch script data fail\n");
                    } else {
                        pwm_pin_count[pwm]++;
                        memcpy(&pwm_gpio_info[pwm][0], gpio_info, sizeof(user_gpio_set_t));
                        }

                ret = script_parser_fetch(primary_key, "pwm_negative", (int *)&gpio_info, sizeof(user_gpio_set_t) / sizeof(int));
                if(ret < 0) {
                    pwm_debug("fetch script data fail\n");
                    } else {
                        pwm_pin_count[pwm]++;
                        memcpy(&pwm_gpio_info[pwm][1], gpio_info, sizeof(user_gpio_set_t));
                        }
                }
            }
}

int sunxi_pwm_set_polarity(int pwm, enum pwm_polarity polarity)
{
    uint temp;

    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm * 0x10);

    if(polarity == PWM_POLARITY_NORMAL) {
        pwm_active_sta[pwm] = 1;
        temp |= 1 << 5;
    } else {
        pwm_active_sta[pwm] = 0;
        temp &= ~(1 << 5);
    }

    sunxi_pwm_write_reg(pwm * 0x10, temp);

    #elif ((defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN7IW1P1))

    temp = sunxi_pwm_read_reg(0);
    if(polarity == PWM_POLARITY_NORMAL) {
        pwm_active_sta[pwm] = 1;
        if(pwm == 0)
            temp |= 1 << 5;
        else
            temp |= 1 << 20;
        }else {
            pwm_active_sta[pwm] = 0;
            if(pwm == 0)
                temp &= ~(1 << 5);
            else
                temp &= ~(1 << 20);
            }

    #endif

    return 0;
}

int sunxi_pwm_config(int pwm, int duty_ns, int period_ns)
{
#if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    uint pre_scal[7] = {1, 2, 4, 8, 16, 32, 64};
    uint freq;
    uint pre_scal_id = 0;
    uint entire_cycles = 256;
    uint active_cycles = 192;
    uint entire_cycles_max = 65536;
    uint temp;
    uint calc;

    if(period_ns < 10667)
        freq = 93747;
    else if(period_ns > 174762666) {
        freq = 6;
        calc = period_ns / duty_ns;
        duty_ns = 174762666 / calc;
        period_ns = 174762666;
        }
    else
        freq = 1000000000 / period_ns;

    entire_cycles = 24000000 / freq /pre_scal[pre_scal_id];

    while(entire_cycles > entire_cycles_max) {
        pre_scal_id++;

        if(pre_scal_id > 6)
            break;

        entire_cycles = 24000000 / freq / pre_scal[pre_scal_id];
        }

    if(period_ns < 5*100*1000)
        active_cycles = (duty_ns * entire_cycles + (period_ns/2)) /period_ns;
    else if(period_ns >= 5*100*1000 && period_ns < 6553500)
        active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns /2 / 100)) / (period_ns/100);
    else
        active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns /2 / 10000)) / (period_ns/10000);

    temp = sunxi_pwm_read_reg(pwm * 0x10);

    temp = (temp & 0xfffffff0) | pre_scal_id;

    sunxi_pwm_write_reg(pwm * 0x10, temp);

    sunxi_pwm_write_reg(pwm * 0x10 + 0x04, ((entire_cycles - 1)<< 16) | active_cycles);

    pwm_debug("PWM _TEST: duty_ns=%d, period_ns=%d, freq=%d, per_scal=%d, period_reg=0x%x\n", duty_ns, period_ns, freq, pre_scal_id, temp);


#elif ((defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN7IW1P1))
	tick_printf("+++++++++++++++++++test10+++++++++++++++++++++\n");	

    uint pre_scal[11][2] = {{15, 1}, {0, 120}, {1, 180}, {2, 240}, {3, 360}, {4, 480}, {8, 12000}, {9, 24000}, {10, 36000}, {11, 48000}, {12, 72000}};
    uint freq;
    uint pre_scal_id = 0;
    uint entire_cycles = 256;
    uint active_cycles = 192;
    uint entire_cycles_max = 65536;
    uint temp;

    if(period_ns < 10667)
        freq = 93747;
    else if(period_ns > 1000000000)
        freq = 1;
    else
        freq = 1000000000 / period_ns;

    entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];

    while(entire_cycles > entire_cycles_max) {
        pre_scal_id++;

        if(pre_scal_id > 10)
            break;

        entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];
        }

    if(period_ns < 5*100*1000)
        active_cycles = (duty_ns * entire_cycles + (period_ns/2)) /period_ns;
    else if(period_ns >= 5*100*1000 && period_ns < 6553500)
        active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns /2 / 100)) / (period_ns/100);
    else
        active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns /2 / 10000)) / (period_ns/10000);

    temp = sunxi_pwm_read_reg(0);

    if(pwm == 0)
        temp = (temp & 0xfffffff0) |pre_scal[pre_scal_id][0];
    else
        temp = (temp & 0xfff87fff) |pre_scal[pre_scal_id][0];

    sunxi_pwm_write_reg(0, temp);

    sunxi_pwm_write_reg((pwm + 1)  * 0x04, ((entire_cycles - 1)<< 16) | active_cycles);

    pwm_debug("PWM _TEST: duty_ns=%d, period_ns=%d, freq=%d, per_scal=%d, period_reg=0x%x\n", duty_ns, period_ns, freq, pre_scal_id, temp);

#endif

    return 0;
}

int sunxi_pwm_enable(int pwm)
{
    uint temp;

#ifndef CONFIG_FPGA_V4_PLATFORM

    int i;
    uint ret = 0;

    for(i = 0; i < pwm_pin_count[pwm]; i++) {
        ret = gpio_request(&pwm_gpio_info[pwm][i], 1);
        if(ret == 0) {
            pwm_debug("pwm gpio request failed!\n");
        }

        gpio_release(ret, 2);
    }

#endif

#if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm * 0x10);

    temp |= 1 << 4;
    temp |= 1 << 6;

    sunxi_pwm_write_reg(pwm * 0x10, temp);

#elif ((defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN7IW1P1))

    temp = sunxi_pwm_read_reg(0);

    if(pwm == 0) {
        temp |= 1 << 4;
        temp |= 1 << 6;
        } else {
            temp |= 1 << 19;
            temp |= 1 << 21;
            }

    sunxi_pwm_write_reg(0, temp);

#endif

    return 0;
}

void sunxi_pwm_disable(int pwm)
{
    uint temp;

#ifndef CONFIG_FPGA_V4_PLATFORM

    int i;
    uint ret = 0;

    for(i = 0; i < pwm_pin_count[pwm]; i++) {
        ret = gpio_request(&pwm_gpio_info[pwm][i], 1);
        if(ret == 0) {
            pwm_debug("pwm gpio request failed!\n");
        }

        gpio_release(ret, 2);
    }

    #endif

#if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm * 0x10);

    temp &= ~(1 << 4);
    temp &= ~(1 << 6);

    sunxi_pwm_write_reg(pwm * 0x10, temp);

#elif ((defined CONFIG_ARCH_SUN8IW3P1) ||(defined CONFIG_ARCH_SUN7IW1P1))

    temp = sunxi_pwm_read_reg(0);

    if(pwm == 0) {
        temp &= ~(1 << 4);
        temp &= ~(1 << 6);
        } else {
            temp &= ~(1 << 19);
            temp &= ~(1 << 21);
            }

#endif
}

void sunxi_pwm_init(void)
{
    int i;

    for(i = 0; i < PWM_NUM; i++)
        sunxi_pwm_get_sys_config(i);
}
