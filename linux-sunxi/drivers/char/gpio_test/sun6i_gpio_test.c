/*
 * drivers/char/gpio_test/sun6i_gpio_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i gpio test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "sun6i_gpio_test.h"

//#define TEST_REQUEST_FREE	/* test for gpio_request/gpio_free */
//#define TEST_RE_REQUEST_FREE	/* test for re-gpio_request/re-gpio_free, so get warning */
//#define TEST_GPIOLIB_API	/* test the standard linux gpio api */
//#define TEST_CONFIG_API		/* test gpio multi-function */
#define TEST_GPIO_EINT_API	/* test gpio external interrupt */
#define TEST_GPIO_SCRIPT_API	/* test gpio script api */

/*
 * cur test case
 */
static enum gpio_test_case_e g_cur_test_case = GTC_API;

/**
 * gpio_chip_match - match function to check if gpio is in the gpio_chip
 * @chip:	gpio_chip to match
 * @data:	data to match
 *
 * Returns 1 if match, 0 if not match
 */
static inline int gpio_chip_match(struct gpio_chip * chip, void * data)
{
	u32 	num = 0;

	num = *(u32 *)data;
	if(num >= chip->base && num < chip->base + chip->ngpio) {
		return 1;
	}

	return 0;
}

/**
 * gpio_irq_handle_demo - gpio irq handle demo.
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_irq_handle_demo(void *para)
{
	printk("%s: para 0x%08x\n", __func__, (u32)para);
	return 0;
}

void __test_script_api(void)
{
	char main_key[256] = {0}, sub_key[256] = {0}, str_cmp[256] = {0};
	int gpio_cnt_cmp, gpio_cnt_get;
	script_item_u item_cmp, item_get, *list_get = NULL;
	script_item_value_type_e type_cmp, type_get;
	struct gpio_config card0_gpio[] = {
		{GPIOF(0)},
		{GPIOF(1)},
		{GPIOF(2)},
		{GPIOF(3)},
		{GPIOF(4)},
		{GPIOF(5)},
	};
	struct gpio_config gpio_group[] = {
		{GPIOF(2)},
		{GPIOF(3)},
		{GPIOF(1)},
		{GPIOF(0)},
		{GPIOF(5)},
		{GPIOF(4)},
		{GPIOA(8)},
	};

	/*
	[card0_boot_para]
	card_ctrl 		= 0
	card_high_speed 	= 1
	card_line       	= 4
	sdc_d1      		= port:PF0<2><1><default><default>
	sdc_d0      		= port:PF1<2><1><default><default>
	sdc_clk     		= port:PF2<2><1><default><default>
	sdc_cmd     		= port:PF3<2><1><default><default>
	sdc_d3      		= port:PF4<2><1><default><default>
	sdc_d2      		= port:PF5<2><1><default><default>

	[product]
	version = "100"
	machine = "evb"

	[mmc0_para]
	sdc_used          = 1
	sdc_detmode       = 2
	sdc_buswidth      = 4
	sdc_clk           = port:PF02<2><1><default><default>
	sdc_cmd           = port:PF03<2><1><default><default>
	sdc_d0            = port:PF01<2><1><default><default>
	sdc_d1            = port:PF00<2><1><default><default>
	sdc_d2            = port:PF05<2><1><default><default>
	sdc_d3            = port:PF04<2><1><default><default>
	sdc_det           = port:PA08<6><1><default><default>
	sdc_use_wp        = 0
	sdc_wp            =
	sdc_isio          = 0

	[lcd0_para]
	lcd_power                = port:power1<1><0><default><1>
	*/
	printk("%s, line %d\n", __func__, __LINE__);
	//script_dump_mainkey(NULL);

	/* test script api */
	strcpy(main_key, "card0_boot_para");
	script_dump_mainkey(main_key);

	/* test for type int */
	strcpy(sub_key, "card_ctrl");
	item_cmp.val = 0;
	type_cmp = SCIRPT_ITEM_VALUE_TYPE_INT;
	type_get = script_get_item(main_key, sub_key, &item_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(type_get != type_cmp, "%s err, line %d, %s->%s type should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, type_cmp, type_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(item_cmp.val != item_get.val, "%s err, line %d, %s->%s value should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, item_cmp.val, item_get.val);

	/* test for type gpio */
	strcpy(sub_key, "sdc_d3");
	type_cmp = SCIRPT_ITEM_VALUE_TYPE_PIO;
	type_get = script_get_item(main_key, sub_key, &item_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(type_get != type_cmp, "%s err, line %d, %s->%s type should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, type_cmp, type_get);
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config(&item_get.gpio, 1);

	/* test for gpio list */
	gpio_cnt_cmp = 6;
	PIOTEST_DBG_FUN_LINE;
	gpio_cnt_get = script_get_pio_list(main_key, &list_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(gpio_cnt_get != gpio_cnt_cmp, "%s err, line %d, %s gpio cnt should be %d, but get %d\n",
		__func__, __LINE__, main_key, gpio_cnt_cmp, gpio_cnt_get);
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config((struct gpio_config *)list_get, gpio_cnt_get);

	/* test for str */
	strcpy(main_key, "product");
	strcpy(sub_key, "machine");
	strcpy(str_cmp, "evb");
	script_dump_mainkey(main_key);
	type_cmp = SCIRPT_ITEM_VALUE_TYPE_STR;
	type_get = script_get_item(main_key, sub_key, &item_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(type_get != type_cmp, "%s err, line %d, %s->%s type should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, type_cmp, type_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(strcmp(str_cmp, item_get.str), "%s err, line %d, %s->%s value should be %s, but get %s\n",
		__func__, __LINE__, main_key, sub_key, str_cmp, item_get.str);
	PIOTEST_DBG_FUN_LINE;

	/* test for mmc0_para */
	strcpy(main_key, "mmc0_para");
	script_dump_mainkey(main_key);
	/* test for int */
	strcpy(sub_key, "sdc_detmode");
	item_cmp.val = 2;
	type_cmp = SCIRPT_ITEM_VALUE_TYPE_INT;
	type_get = script_get_item(main_key, sub_key, &item_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(type_get != type_cmp, "%s err, line %d, %s->%s type should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, type_cmp, type_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(item_cmp.val != item_get.val, "%s err, line %d, %s->%s value should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, item_cmp.val, item_get.val);
	PIOTEST_DBG_FUN_LINE;
	/* test for gpio list */
	gpio_cnt_cmp = 7;
	gpio_cnt_get = script_get_pio_list(main_key, &list_get);
	PIOTEST_DBG_FUN_LINE;
	WARN(gpio_cnt_get != gpio_cnt_cmp, "%s err, line %d, %s->%s gpio cnt should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, gpio_cnt_cmp, gpio_cnt_get);
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config((struct gpio_config *)list_get, gpio_cnt_get);
	PIOTEST_DBG_FUN_LINE;

	/* test for gpio config api */
	strcpy(main_key, "card0_boot_para");
	gpio_cnt_cmp = 6;
	gpio_cnt_get = script_get_pio_list(main_key, &list_get);
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(gpio_cnt_get != gpio_cnt_cmp);
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(0 != sw_gpio_setall_range(&list_get[0].gpio, gpio_cnt_get));
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(0 != sw_gpio_getall_range((struct gpio_config *)card0_gpio, gpio_cnt_get));
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config((struct gpio_config *)card0_gpio, gpio_cnt_get);
	PIOTEST_DBG_FUN_LINE;

	/* test for gpio config api */
	strcpy(main_key, "mmc0_para");
	//script_dump_mainkey(main_key);
	gpio_cnt_cmp = 7;
	gpio_cnt_get = script_get_pio_list(main_key, &list_get);
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(gpio_cnt_get != gpio_cnt_cmp);
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(0 != sw_gpio_setall_range(&list_get[0].gpio, gpio_cnt_get));
	PIOTEST_DBG_FUN_LINE;
	WARN_ON(0 != sw_gpio_getall_range((struct gpio_config *)gpio_group, gpio_cnt_get));
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config((struct gpio_config *)gpio_group, gpio_cnt_get);

	/* test for axp pin */
	strcpy(main_key, "lcd0_para");
	script_dump_mainkey(main_key);
	strcpy(sub_key, "lcd_power");
	type_cmp = SCIRPT_ITEM_VALUE_TYPE_PIO;
	PIOTEST_DBG_FUN_LINE;
	type_get = script_get_item(main_key, sub_key, &item_get);
	WARN(type_get != type_cmp, "%s err, line %d, %s->%s type should be %d, but get %d\n",
		__func__, __LINE__, main_key, sub_key, type_cmp, type_get);
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config((struct gpio_config *)&item_get.gpio, 1);
	PIOTEST_DBG_FUN_LINE;
#if 1	/* both ok, use sw_gpio_setall_range or standard linux gpio api */
	WARN_ON(0 != sw_gpio_setall_range(&item_get.gpio, 1));
#else
	if(1 == item_get.gpio.mul_sel) {
		if(0 != gpio_direction_output(item_get.gpio.gpio, item_get.gpio.data))
			printk("%s err, set axp gpio output failed\n", __func__);
		else {
			printk("%s, set axp gpio output success!\n", __func__);
			if(item_get.gpio.data != __gpio_get_value(item_get.gpio.gpio))
				printk("%s err, get axp gpio value NOT match!\n", __func__);
			else
				printk("%s ok, get axp gpio value match!\n", __func__);
		}
	} else if(0 == item_get.gpio.mul_sel) {
		if(0 != gpio_direction_input(item_get.gpio.gpio))
			printk("%s err, set axp gpio input failed\n", __func__);
		else {
			int val = __gpio_get_value(item_get.gpio.gpio);
			printk("%s, set axp gpio input success! get value %d\n", __func__, val);
		}
	} else
		printk("%s err, line %d\n", __func__, __LINE__);
#endif
	PIOTEST_DBG_FUN_LINE;
	item_cmp.gpio.gpio = item_get.gpio.gpio;
	WARN_ON(0 != sw_gpio_getall_range(&item_cmp.gpio, 1));
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config(&item_cmp.gpio, 1);
	printk("%s, line %d, end\n", __func__, __LINE__);
}

void __test_request_free(void)
{
	u32	uindex;
	char	name[256] = {0};

	PIOTEST_DBG_FUN_LINE;
	uindex = GPIOE(16);
	strcpy(name, "pe16");
	if(0 != gpio_request(uindex, name))
		printk("%s, line %d, request %s(%d) failed!\n", __func__, __LINE__, name, uindex);
	else {
		printk("%s, line %d, request %s(%d) success! now free it\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
	}

	PIOTEST_DBG_FUN_LINE;
	uindex = GPIOA(5);
	strcpy(name, "pa5");
	if(0 != gpio_request(uindex, name))
		printk("%s, line %d, request %s(%d) failed!\n", __func__, __LINE__, name, uindex);
	else {
		printk("%s, line %d, request %s(%d) success! now free it\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
	}

	PIOTEST_DBG_FUN_LINE;
	uindex = GPIOB(2);
	strcpy(name, "pb2");
	if(0 != gpio_request(uindex, name))
		printk("%s, line %d, request %s(%d) failed!\n", __func__, __LINE__, name, uindex);
	else {
		printk("%s, line %d, request %s(%d) success! now free it\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
	}

#if 0	/* request un-exist gpio, err */
	PIOTEST_DBG_FUN_LINE;
	uindex = GPIOA(0) + PA_NR;
	strcpy(name, "pa_end");
	if(0 != gpio_request(uindex, name))
		printk("%s, line %d, request %s(%d) failed!\n", __func__, __LINE__, name, uindex);
	else {
		printk("%s, line %d, request %s(%d) success! now free it\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
	}
#endif
}

void __test_re_request_free(void)
{
	u32	uindex;
	char	name[256] = {0};

	PIOTEST_DBG_FUN_LINE;
	uindex = GPIOE(16);
	strcpy(name, "pe16");
	if(0 != gpio_request(uindex, name))
		printk("%s, line %d, request %s(%d) failed!\n", __func__, __LINE__, name, uindex);
	else {
		printk("%s, line %d, request %s(%d) success!\n", __func__, __LINE__, name, uindex);
#if 0	/* re-request, err */
		printk("%s, line %d, try to re-request %s(%d)!\n", __func__, __LINE__, name, uindex);
		if(0 != gpio_request(uindex, name))
			printk("%s, line %d, re-request %s(%d) failed! good!\n", __func__, __LINE__, name, uindex);
		else
			printk("%s, line %d, re-request %s(%d) pass! err!\n", __func__, __LINE__, name, uindex);
#endif
		printk("%s, line %d, start free %s(%d)\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
#if 0	/* re-free, err */
		printk("%s, line %d, try to refree %s(%d)\n", __func__, __LINE__, name, uindex);
		gpio_free(uindex);
		printk("%s, line %d, after refree %s(%d)\n", __func__, __LINE__, name, uindex);
#endif
	}
	PIOTEST_DBG_FUN_LINE;
}

u32 __test_standard_api(void)
{
	u32 	uret = 0;
	u32	utemp = 0;
	u32	offset = 0;
	u32	upio_index = 0;
	char	name[256] = {0};
	struct gpio_chip *pchip = NULL;
	struct gpio gpio_arry[] = {
		{GPIOA(0), GPIOF_OUT_INIT_HIGH, "pa0"},
		{GPIOB(3), GPIOF_IN, "pb3"},
		{GPIOC(5), GPIOF_OUT_INIT_LOW, "pc5"},
		{GPIOH(2), GPIOF_IN, "ph2"},
	};
	struct gpio_config gpio_cfg_temp[4] = {
		{GPIOA(0)},
		{GPIOB(3)},
		{GPIOC(5)},
		{GPIOH(2)},
	};

	PIOTEST_DBG_FUN_LINE;
	/* test gpio_request_one */
	upio_index = GPIOE(16);
	strcpy(name, "pe_16");
	PIO_CHECK_RST(0 == gpio_request_one(upio_index, GPIOF_OUT_INIT_HIGH, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(1 != __gpio_get_value(upio_index)) { /* check if data ok */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	gpio_free(upio_index);

	PIO_CHECK_RST(0 == gpio_request_one(upio_index, GPIOF_OUT_INIT_LOW, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(0 != __gpio_get_value(upio_index)) { /* check if data ok */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	gpio_free(upio_index);

	/* test gpio_request_array */
	PIO_CHECK_RST(0 == gpio_request_array(gpio_arry, ARRAY_SIZE(gpio_arry)), uret, end);
	PIOTEST_DBG_FUN_LINE;
	/* test gpio_free_array */
	gpio_free_array(gpio_arry, ARRAY_SIZE(gpio_arry));
	PIOTEST_DBG_FUN_LINE;
	/* check if request array success */
	PIO_CHECK_RST(0 == sw_gpio_getall_range(gpio_cfg_temp, ARRAY_SIZE(gpio_cfg_temp)), uret, end);
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config(gpio_cfg_temp, ARRAY_SIZE(gpio_cfg_temp));
	PIOTEST_DBG_FUN_LINE;

	/* test gpiochip_find */
	offset = 5;
	upio_index = GPIOB(offset);
	strcpy(name, "pb_5");
	PIO_CHECK_RST(0 == gpio_request(upio_index, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(NULL == (pchip = gpiochip_find(&upio_index, gpio_chip_match))) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	} else
		printk("%s: gpiochip_find success, pchip 0x%08x\n", __func__, (u32)pchip);
	/* test gpiochip_is_requested */
	if(NULL == gpiochip_is_requested(pchip, offset)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	gpio_free(upio_index);
	PIOTEST_DBG_FUN_LINE;

	/* test gpio_direction_input/__gpio_get_value/gpio_get_value_cansleep */
	upio_index = GPIOE(16);
	strcpy(name, "pe_16");
	PIO_CHECK_RST(0 == gpio_request(upio_index, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(0 != gpio_direction_input(upio_index)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	utemp = __gpio_get_value(upio_index); /* __gpio_get_value */
	printk("%s: __gpio_get_value %s return %d\n", __func__, name, utemp);
	utemp = (u32)gpio_get_value_cansleep(upio_index); /* gpio_get_value_cansleep, success even can_sleep flag not set */
	printk("%s: gpio_get_value_cansleep %s return %d\n", __func__, name, utemp);
	gpio_free(upio_index);

	/* test gpio_direction_output/__gpio_get_value/__gpio_set_value/gpio_set_value_cansleep */
	upio_index = GPIOE(16);
	strcpy(name, "pe_16");
	PIO_CHECK_RST(0 == gpio_request(upio_index, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(0 != gpio_direction_output(upio_index, 1)) { /* gpio_direction_output */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(1 != __gpio_get_value(upio_index)) { /* __gpio_get_value */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(0 != gpio_direction_output(upio_index, 0)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(0 != __gpio_get_value(upio_index)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	__gpio_set_value(upio_index, 1); /* __gpio_set_value */
	PIOTEST_DBG_FUN_LINE;
	if(1 != __gpio_get_value(upio_index)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	gpio_set_value_cansleep(upio_index, 0); /* gpio_set_value_cansleep */
	PIOTEST_DBG_FUN_LINE;
	/* test gpio_set_debounce, gpio_chip->set_debounce not impletment, so err here */
	utemp = gpio_set_debounce(upio_index, 10);
	printk("%s: gpio_set_debounce %d return %d\n", __func__, upio_index, utemp);
	PIOTEST_DBG_FUN_LINE;
	/* test __gpio_cansleep */
	utemp = (u32)__gpio_cansleep(upio_index);
	printk("%s: __gpio_cansleep %d return %d\n", __func__, upio_index, utemp);
	/* test __gpio_to_irq */
	utemp = (u32)__gpio_to_irq(upio_index);
	printk("%s: __gpio_to_irq %d return %d\n", __func__, upio_index, utemp);
	/* free gpio */
	gpio_free(upio_index);
	PIOTEST_DBG_FUN_LINE;

end:
	if(0 != uret)
		printk("%s err, line %d\n", __func__, uret);
	return uret;
}

u32 __test_mul_fun_api(void)
{
	u32 	uret = 0;
	u32	upio_index = 0;
	char	name[256] = {0};
	struct gpio_config gpio_cfg[] = {
		/* use default if you donot care the pull or driver level status */
		{GPIOE(10), 3, GPIO_PULL_DEFAULT, GPIO_DRVLVL_DEFAULT, 0},
		{GPIOA(13), 2, 1, 2, -1},
		{GPIOD(2),  1, 2, 1, 1},
		{GPIOG(8),  0, 1, 1, 0},
	};

	/* test sw_gpio_getcfg with gpio_direction_output */
	upio_index = GPIOE(16);
	strcpy(name, "pe_16");
	PIO_CHECK_RST(0 == gpio_request(upio_index, name), uret, end);
	PIOTEST_DBG_FUN_LINE;
	if(0 != gpio_direction_output(upio_index, 0)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(1 != sw_gpio_getcfg(upio_index)) { /* sw_gpio_getcfg output */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;

	/* test sw_gpio_getcfg with gpio_direction_input */
	if(0 != gpio_direction_input(upio_index)) {
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(0 != sw_gpio_getcfg(upio_index)) { /* sw_gpio_getcfg input */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;

	/* test sw_gpio_setcfg/sw_gpio_getcfg  */
	if(0 != sw_gpio_setcfg(upio_index, 3)) { /* sw_gpio_setcfg */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(3 != sw_gpio_getcfg(upio_index)) { /* sw_gpio_getcfg */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;

	/* test sw_gpio_setpull/sw_gpio_getpull  */
	if(0 != sw_gpio_setpull(upio_index, 2)) { /* sw_gpio_setpull down */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(2 != sw_gpio_getpull(upio_index)) { /* sw_gpio_getpull */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;

	/* test sw_gpio_setdrvlevel/sw_gpio_getdrvlevel  */
	if(0 != sw_gpio_setdrvlevel(upio_index, 2)) { /* sw_gpio_setdrvlevel level2 */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(2 != sw_gpio_getdrvlevel(upio_index)) { /* sw_gpio_getdrvlevel */
		gpio_free(upio_index);
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	/* free gpio */
	gpio_free(upio_index);

	/* test sw_gpio_setall_range/sw_gpio_getall_range/sw_gpio_dump_config  */
	if(0 != sw_gpio_setall_range(gpio_cfg, ARRAY_SIZE(gpio_cfg))) { /* sw_gpio_setall_range */
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	if(0 != sw_gpio_getall_range(gpio_cfg, ARRAY_SIZE(gpio_cfg))) { /* sw_gpio_getall_range */
		uret = __LINE__;
		goto end;
	}
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_dump_config(gpio_cfg, ARRAY_SIZE(gpio_cfg)); /* sw_gpio_dump_config */

end:
	if(0 != uret)
		printk("%s err, line %d\n", __func__, uret);
	return uret;
}

u32 __test_eint_api(void)
{
	u32 	uret = 0;
	u32	upio_index = 0;
	u32 	irq_handles[5] = {0};
	struct gpio_config_eint_all cfg_eint[] = {
		/*
		 * NOTE: never set cfg_eint.enabled true, or there is warning:
		 * irq 43: nobody cared (try booting with the "irqpoll" option)
		 * gic_handle_irq->...->handle_irq_event_percpu->note_interrupt->__report_bad_irq
		 */
		{GPIOA(0) , GPIO_PULL_DEFAULT, GPIO_DRVLVL_DEFAULT, false, 0, TRIG_EDGE_POSITIVE},
		{GPIOA(26), 1                , 2                  , false, 0, TRIG_EDGE_DOUBLE  },
		{GPIOB(3) , GPIO_PULL_DEFAULT, 1                  , false, 0, TRIG_EDGE_NEGATIVE},
		{GPIOE(14), 3                , 0                  , false, 0, TRIG_LEVL_LOW     },
		/* __para_check err in sw_gpio_eint_setall_range, because rpl3 cannot be eint */
		//{GPIOL(3) , 1                , GPIO_DRVLVL_DEFAULT, false, 0, TRIG_LEVL_HIGH  },
		{GPIOL(8) , 2                , GPIO_DRVLVL_DEFAULT, false, 0, TRIG_LEVL_HIGH    },
		{GPIOM(4) , 1                , 2                  , false, 0, TRIG_LEVL_LOW     },
	};
	struct gpio_config_eint_all cfg_eint_temp[] = {
		{GPIOA(0) },
		{GPIOA(26)},
		{GPIOB(3) },
		{GPIOE(14)},
		{GPIOL(8) },
		{GPIOM(4) },
	};

	/* test sw_gpio_eint_setall_range/sw_gpio_eint_getall_range api */
	sw_gpio_eint_dumpall_range(cfg_eint, ARRAY_SIZE(cfg_eint)); /* dump the orginal struct */
	PIOTEST_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == sw_gpio_eint_setall_range(cfg_eint, ARRAY_SIZE(cfg_eint)), uret, end); /* sw_gpio_eint_setall_range */
	PIOTEST_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == sw_gpio_eint_getall_range(cfg_eint_temp, ARRAY_SIZE(cfg_eint_temp)), uret, end); /* sw_gpio_eint_getall_range */
	PIOTEST_DBG_FUN_LINE;
	sw_gpio_eint_dumpall_range(cfg_eint_temp, ARRAY_SIZE(cfg_eint_temp)); /* dump the struct get from hw */
	PIOTEST_DBG_FUN_LINE;

	/* test sw_gpio_irq_request/sw_gpio_irq_free api */
	upio_index = GPIOA(0);
	irq_handles[0] = sw_gpio_irq_request(upio_index, TRIG_EDGE_POSITIVE,
		(peint_handle)gpio_irq_handle_demo, (void *)upio_index);
	printk("%s, line %d, irq_handles[0] 0x%08x\n", __func__, __LINE__, irq_handles[0]);
	upio_index = GPIOA(1);
	irq_handles[1] = sw_gpio_irq_request(upio_index, TRIG_EDGE_NEGATIVE,
		(peint_handle)gpio_irq_handle_demo, (void *)upio_index);
	printk("%s, line %d, irq_handles[1] 0x%08x\n", __func__, __LINE__, irq_handles[1]);
	upio_index = GPIOC(0);
	irq_handles[2] = sw_gpio_irq_request(upio_index, TRIG_EDGE_DOUBLE,
		(peint_handle)gpio_irq_handle_demo, (void *)upio_index);
	printk("%s, line %d, irq_handles[2] 0x%08x\n", __func__, __LINE__, irq_handles[2]);
	upio_index = GPIOE(2);
	irq_handles[3] = sw_gpio_irq_request(upio_index, TRIG_LEVL_LOW,
		(peint_handle)gpio_irq_handle_demo, (void *)upio_index);
	printk("%s, line %d, irq_handles[3] 0x%08x\n", __func__, __LINE__, irq_handles[3]);

	if(0 != irq_handles[0])
		sw_gpio_irq_free(irq_handles[0]);
	PIOTEST_DBG_FUN_LINE;
	if(0 != irq_handles[1])
		sw_gpio_irq_free(irq_handles[1]);
	PIOTEST_DBG_FUN_LINE;
	if(0 != irq_handles[2])
		sw_gpio_irq_free(irq_handles[2]);
	PIOTEST_DBG_FUN_LINE;
	if(0 != irq_handles[3])
		sw_gpio_irq_free(irq_handles[3]);
	PIOTEST_DBG_FUN_LINE;

end:
	if(0 != uret)
		printk("%s err, line %d\n", __func__, uret);
	return uret;
}

/**
 * __gtc_api - gpio test case, for api
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __gtc_api(void)
{
	/* wait for request and free */
	msleep(1000);

	/* test for request and free */
#ifdef TEST_REQUEST_FREE
	__test_request_free();
	printk("%s: __test_request_free end\n", __func__);
#endif /* TEST_REQUEST_FREE */

	/* test for re-request and re-free the same pio */
#ifdef TEST_RE_REQUEST_FREE
	__test_re_request_free();
#endif /* TEST_RE_REQUEST_FREE */

	/* test for gpiolib api */
#ifdef TEST_GPIOLIB_API
	if(0 != __test_standard_api())
		printk("%s: __test_standard_api failed\n", __func__);
	else
		printk("%s: __test_standard_api success\n", __func__);
#endif /* TEST_GPIOLIB_API */

	/* test for gpio multi function config api */
#ifdef TEST_CONFIG_API
	if(0 != __test_mul_fun_api())
		printk("%s: __test_mul_fun_api failed\n", __func__);
	else
		printk("%s: __test_mul_fun_api success\n", __func__);
#endif /* TEST_CONFIG_API */

	/* test for gpio external interrupt api */
#ifdef TEST_GPIO_EINT_API
	if(0 != __test_eint_api())
		printk("%s: __test_eint_api failed\n", __func__);
	else
		printk("%s: __test_eint_api success\n", __func__);
#endif /* TEST_GPIO_EINT_API */

	/* test for script gpio api */
#ifdef TEST_GPIO_SCRIPT_API
	__test_script_api();
#endif /* TEST_GPIO_SCRIPT_API */

	return 0;
}

/**
 * __gpio_test_thread - gpio test main thread
 * @arg:	thread arg, not used
 *
 * Returns 0 if success, the err line number if failed.
 */
static int __gpio_test_thread(void * arg)
{
	u32 	uResult = 0;

	switch(g_cur_test_case) {
	case GTC_API:
		uResult = __gtc_api();
		break;
	default:
		uResult = __LINE__;
		break;
	}

	if(0 == uResult)
		printk("%s: test success!\n", __func__);
	else
		printk("%s: test failed!\n", __func__);

	return uResult;
}

/**
 * sw_gpio_test_init - enter the gpio test module
 */
static int __init sw_gpio_test_init(void)
{
	printk("%s enter\n", __func__);

	/*
	 * create the test thread
	 */
	kernel_thread(__gpio_test_thread, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

/**
 * sw_gpio_test_exit - exit the gpio test module
 */
static void __exit sw_gpio_test_exit(void)
{
	printk("sw_gpio_test_exit: enter\n");
}

#ifdef MODULE
module_init(sw_gpio_test_init);
module_exit(sw_gpio_test_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sun6i gpio Test driver code");
#else
__initcall(sw_gpio_test_init);
#endif /* MODULE */
