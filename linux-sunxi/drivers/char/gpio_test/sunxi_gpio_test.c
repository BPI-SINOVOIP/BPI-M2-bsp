#include "sun6i_gpio_test.h"
#include <linux/ctype.h>
#define GPIO_TEST_NUMBER 12
#if 1
#define PIN_TEST_DEBUG(fmt...) printk(fmt)
#else
#define PIN_TEST_DEBUG(fmt...) do{}while(0)
#endif

struct gpio_result_class{
	char			*name;
	char			result;
};

/* this code maybe use later, but it only probability event
static	struct gpio gpio_arry[3]={
		{GPIOA(0), GPIOF_OUT_INIT_HIGH, "pa0"},
		{GPIOB(3), GPIOF_IN, "pb3"},
		{GPIOC(5), GPIOF_OUT_INIT_LOW, "pb3"},
};

static	struct gpio_config gpio_cfg_temp[4]={
	{GPIOA(0)},
	{GPIOB(4)},
	{GPIOH(1)},
	{GPIOH(2)},
};
static	struct gpio_config gpio_cfg[4]={
	{GPIOE(10), 3, GPIO_PULL_DEFAULT, GPIO_DRVLVL_DEFAULT},
	{GPIOA(13), 2, 1, 2},
};
*/
struct sunxi_gpio_test_class{
	unsigned	int	exec;
	unsigned	int	group;
	unsigned	int	number;
	unsigned	int	functions;
	unsigned	int	data;
	unsigned	int	drv_lvl;
	unsigned	int	pull;
/*	unsigned	int	irq;*/
	unsigned	int	trigger;
	struct	device	*dev;
};

static struct gpio_result_class	gpio_result[GPIO_TEST_NUMBER];
static struct class *sunxi_gpio_test_init_class;
struct mutex sunxi_gpio_test_mutex;
static unsigned int testcase_number;

static	u32 sw_gpio_port_to_index(u32 port, u32 port_num)
{
	u32 	usign = 0;
	struct pio_group {
		u32 	base;
		u32 	nr;
	};
	const struct pio_group pio_buf[] = {
		{PA_NR_BASE,         PA_NR},
		{PB_NR_BASE,         PB_NR},
		{PC_NR_BASE,         PC_NR},
		{PD_NR_BASE,         PD_NR},
		{PE_NR_BASE,         PE_NR},
		{PF_NR_BASE,         PF_NR},
		{PG_NR_BASE,         PG_NR},
		{PH_NR_BASE,         PH_NR},
		{GPIO_INDEX_INVALID, 0    },
		{GPIO_INDEX_INVALID, 0    },
		{GPIO_INDEX_INVALID, 0    },
		{PL_NR_BASE,         PL_NR},
		{PM_NR_BASE,         PM_NR}
	};

	/* para check */
	if(port - 1 >= ARRAY_SIZE(pio_buf)
		|| GPIO_INDEX_INVALID == pio_buf[port - 1].base) {
		usign = __LINE__;
		goto End;
	}

	/* check if port valid */
	if(port_num >= pio_buf[port - 1].nr) {
		usign = __LINE__;
		goto End;
	}

End:
	if(0 != usign) {
		return GPIO_INDEX_INVALID;
	} else {
		return (pio_buf[port - 1].base + port_num);
	}
}



static int set_cfg_standard_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0,value;
	int ret=0;
	char name[4];
	gpio_result[0].name ="pin_set_cfg_standard_api_test";
	testcase_number=0;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d line %d\n",name,upio_index,__LINE__);

	if(gpio_request(upio_index, name)){
		printk("set_cfg_standard_api gpio_request fail! %d\n",__LINE__);
		ret = -1;
		gpio_result[0].result =1;
		return ret;
	}

	if(gpio_direction_output(upio_index, 1)){
		ret = -1;
		printk("call gpio_direction_output fail %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
	value	= __gpio_get_value(upio_index);
	if(!value || (sw_gpio_getcfg(upio_index) != 1)){
		ret = -1;
		printk("gpio_direction_output set 1 err\n");
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	if(gpio_direction_output(upio_index, 0)){
		ret = -1;
		printk("call gpio_direction_output fail %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	value	= __gpio_get_value(upio_index);
	if(value || (sw_gpio_getcfg(upio_index) != 1)){
		ret = ret;
		printk("gpio_direction_output set 0 err\n");
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	if(gpio_direction_input(upio_index)){
		ret = -1;
		printk("call gpio_direction_output fail %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
	if(sw_gpio_getcfg(upio_index)){
		ret = -1;
		printk("gpio_direction_input err\n");
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
	gpio_free(upio_index);

	if(gpio_request_one(upio_index, GPIOF_OUT_INIT_HIGH, name)){
		ret = -1;
		printk("gpio_request_one err %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	value	= __gpio_get_value(upio_index);
	if(!value || (sw_gpio_getcfg(upio_index) != 1)){
		ret = -1;
		printk("gpio_request_one output set err %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
	gpio_free(upio_index);

	if(gpio_request_one(upio_index, GPIOF_IN, name)){
		ret = -1;
		printk("gpio_request_one err %d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	if(sw_gpio_getcfg(upio_index)){
		ret = -1;
		printk("gpio_request_one input err%d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
	gpio_free(upio_index);

	if(gpio_request_one(upio_index, GPIOF_OUT_INIT_LOW, name)){
		ret = -1;
		printk("gpio_request_one err%d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}

	value	= __gpio_get_value(upio_index);
	if(sw_gpio_getcfg(upio_index)!=1 || value){
		ret = -1;
		printk("gpio_request_one input err%d\n",__LINE__);
		gpio_result[0].result =1;
		goto set_cfg_standard_api_err;
	}
set_cfg_standard_api_err:	
	gpio_free(upio_index);
	return ret;
}

static int set_cfg_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[1].name ="pin_set_cfg_api_test";
	testcase_number=1;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	if(gpio_request(upio_index, name)){
		printk("gpio_request fail in set_cfg_api\n");
		return -1;
	}

	if(sw_gpio_setcfg(upio_index,sunxi_gpio_test->functions)){
		printk("pio %s set functions %d err\n",name,sunxi_gpio_test->functions);
		gpio_result[1].result =1;
		ret = -1;
		goto  set_cfg_api_err;
	}

	if(sunxi_gpio_test->functions != sw_gpio_getcfg(upio_index)){
		printk("set cfg err! expect: %d true: %d\n",sunxi_gpio_test->functions,sw_gpio_getcfg(upio_index));
		ret = -1;
		gpio_result[1].result =1;
	}

set_cfg_api_err:
	gpio_free(upio_index);
	return ret;
}

static int set_drv_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[2].name ="pin_set_drv_api_test";
	testcase_number=2;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	if(gpio_request(upio_index, name)){
		printk("gpio_request fail in set_drv_api\n");
		return -1;
	}

	if(sw_gpio_setdrvlevel(upio_index, sunxi_gpio_test->drv_lvl)){
		printk("pio %s set drv_lvl %d err\n",name,sunxi_gpio_test->drv_lvl);
		gpio_result[2].result =1;
		ret = -1;
		goto set_drv_api_err;
	}

	if(sunxi_gpio_test->drv_lvl != sw_gpio_getdrvlevel(upio_index)){
		printk("set drv_lvl err! expect: %d true: %d\n",sunxi_gpio_test->drv_lvl,sw_gpio_getdrvlevel(upio_index));
		ret = -1;
		gpio_result[2].result =1;
	}

set_drv_api_err:
	gpio_free(upio_index);
	return ret;
}

static int set_pin_data_standard_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[3].name ="set_pin_data_standard_api";
	testcase_number=3;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	if(gpio_request(upio_index, name)){
		printk("gpio_request fail in set_pin_data_standard_api\n");
		return -1;
	}

	if(gpio_direction_output(upio_index, sunxi_gpio_test->data)){
		printk("pio %s set data %d with standard err\n",name,sunxi_gpio_test->data);
		gpio_result[3].result =1;
		ret = -1;
		goto set_pin_data_standard_api_err;
	}

	if(sunxi_gpio_test->data != __gpio_get_value(upio_index)){
		printk("set data err! expect: %d true: %d\n",sunxi_gpio_test->data,__gpio_get_value(upio_index));
		ret = -1;
		gpio_result[3].result =1;
	}

set_pin_data_standard_api_err:
	gpio_free(upio_index);
	return ret;
}


static int set_pin_pull_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[4].name ="set_pin_pull_api";
	testcase_number=4;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	if(gpio_request(upio_index, name)){
		printk("gpio_request fail in set_pin_pull_api\n");
		return -1;
	}

	if(sw_gpio_setpull(upio_index, sunxi_gpio_test->pull)){
		printk("pio %s set pull %d err\n",name,sunxi_gpio_test->pull);
		gpio_result[4].result =1;
		ret = -1;
		goto set_pin_pull_api_err;
	}

	if(sunxi_gpio_test->pull != sw_gpio_getpull(upio_index)){
		printk("set pull err! expect: %d true: %d\n",sunxi_gpio_test->pull,sw_gpio_getpull(upio_index));
		ret = -1;
		gpio_result[4].result =1;
	}

set_pin_pull_api_err:
	gpio_free(upio_index);
	return ret;
}

static int set_pin_data_api(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[5].name ="set_pin_data_api";
	testcase_number=5;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	if(gpio_request_one(upio_index, GPIOF_OUT_INIT_HIGH, name)){
		printk("gpio_request fail in set_pin_data_api\n");
		return -1;
	}
	
	__gpio_set_value(upio_index, sunxi_gpio_test->data);
	if(sunxi_gpio_test->data != __gpio_get_value(upio_index)){
		printk("set data err! expect: %d true: %d\n",sunxi_gpio_test->data,__gpio_get_value(upio_index));
		ret = -1;
		gpio_result[5].result =1;
	}

	gpio_free(upio_index);
	return ret;
}

static u32 gpio_irq_handle_demo(void *para)
{
	int *eint_flag;
	eint_flag	= para;
	*eint_flag	= 1;
	printk("%s: para 0x%08x\n", __FUNCTION__, (u32)para);
	return 0;
}

static int pin_eirq_request(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	gpio_result[6].name ="pin_eirq_request";
	testcase_number=6;
#if 0
	u32	upio_index = 0;
	int ret=0,flags=0;
	char name[4];
	u32 handle_temp =0;
	gpio_result[6].name ="pin_eirq_request";
	testcase_number=6;
	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);

	handle_temp	= sw_gpio_irq_request(upio_index, sunxi_gpio_test->trigger,(peint_handle)gpio_irq_handle_demo, &flags);
	
	if(!handle_temp){
		printk("request %s eint fail %d\n",name,__LINE__);
		return -1;
	}
	PIN_TEST_DEBUG("flags: %d\n",flags);
	__gpio_set_value(upio_index, 0);
	PIN_TEST_DEBUG("%s data is %d\n",name,__gpio_get_value(upio_index));
	__gpio_set_value(upio_index, 1);
	PIN_TEST_DEBUG("%s data is %d\n",name,__gpio_get_value(upio_index));
	__gpio_set_value(upio_index, 0);
	PIN_TEST_DEBUG("%s data is %d\n",name,__gpio_get_value(upio_index));
	PIN_TEST_DEBUG("flags: %d\n",flags);

	if(	flags != 1){
		printk("interrupt dispuse function execute fail!\n");
		ret = -1;
	}
	sw_gpio_irq_free(handle_temp);
	return ret;
#endif
	return 0;
}

static int pin_script_request(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	gpio_result[7].name ="pin_script_request";
	testcase_number=7;
	printk("api be remove !\n");
	return 0;
}

static int pin_request_inexistent(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	struct gpio gpio_arry[] = {
		{GPIOA(0), GPIOF_OUT_INIT_HIGH, "pa0"},
		{GPIOB(3), GPIOF_IN, "pb3"},
		{GPIOC(5), GPIOF_OUT_INIT_LOW, "pc5"},
		{GPIOH(99), GPIOF_IN, "ph99"},
	};

	gpio_result[8].name ="pin_request_inexistent";
	testcase_number=8;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s %d\n",name,upio_index);
	if(upio_index != GPIO_INDEX_INVALID){
		printk("gpio_request test fail in %d expect 0x%x now 0x%x",__LINE__,GPIO_INDEX_INVALID,upio_index);
		return -1;
	}

	if(!gpio_request(GPIOH(99), name)){
		printk("gpio_request test fail in %d\n",__LINE__);
		gpio_free(GPIOH(99));
		return -1;
	}

	if(!gpio_request_one(GPIOH(99), GPIOF_OUT_INIT_HIGH, name)){
		printk("gpio_request_one test fail in %d\n",__LINE__);
		gpio_free(GPIOH(99));
		return -1;
	}

	if(!gpio_request_one(GPIOH(99), GPIOF_OUT_INIT_LOW, name)){
		printk("gpio_request_one test fail in %d\n",__LINE__);
		gpio_free(GPIOH(99));
		return -1;
	}
	if(!gpio_request_array(gpio_arry, ARRAY_SIZE(gpio_arry))){
		printk("gpio_request_one test fail in %d\n",__LINE__);
		gpio_free_array(gpio_arry, ARRAY_SIZE(gpio_arry));
		return -1;
	}
	return ret;
}

static int pin_rqst_inexistent_irq(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0,flags=0;
	char name[4];
	u32 handle_temp =0;
	gpio_result[9].name ="pin_rqst_inexistent_irq";
	testcase_number=9;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	
	printk("test gpio is %s %d\n",name,upio_index);

	handle_temp	= sw_gpio_irq_request(upio_index, sunxi_gpio_test->trigger,(peint_handle)gpio_irq_handle_demo, &flags);
	printk("handle_temp is %x line 478\n",handle_temp);
	if(handle_temp){
		printk("pin_rqst_inexistent_irq fail! %d\n",__LINE__);
		ret = -1;
		gpio_result[9].result =1;
		sw_gpio_irq_free(handle_temp);
	}

	return ret;
}

static int repeat_request(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0;
	char name[4];
	gpio_result[10].name ="repeat_request";
	testcase_number=10;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s\n",name);

	if(gpio_request(upio_index, name)){
		printk("gpio_request test fail in %d\n",__LINE__);
		return -1;
	}

	if(!gpio_request(upio_index, name)){
		printk("repeat_request test fail in %d\n",__LINE__);
		gpio_free(upio_index);
		return -1;
	}
	gpio_free(upio_index);

	if(gpio_request_one(upio_index, GPIOF_OUT_INIT_HIGH, "name")){
		printk("gpio_request_one test fail in %d\n",__LINE__);
		gpio_result[10].result =1;
		return -1;
	}

	if(!gpio_request_one(upio_index, GPIOF_OUT_INIT_HIGH, "name")){
		printk("repeat_request test fail in %d\n",__LINE__);
		gpio_result[10].result =1;
		return -1;
	}
	gpio_free(upio_index);

	return ret;
}

static int repeat_request_irq(struct sunxi_gpio_test_class *sunxi_gpio_test)
{
	u32	upio_index = 0;
	int ret=0,flags=0;
	char name[4];
	u32 handle_temp =0,handle_temp_second=0;
	gpio_result[11].name ="repeat_request_irq";
	testcase_number=11;

	sprintf(name,"p%c%d",sunxi_gpio_test->group+48*2,sunxi_gpio_test->number);
	upio_index = sw_gpio_port_to_index(sunxi_gpio_test->group,sunxi_gpio_test->number);
	printk("test gpio is %s\n",name);

	handle_temp	= sw_gpio_irq_request(upio_index, sunxi_gpio_test->trigger,(peint_handle)gpio_irq_handle_demo, &flags);

	if(!handle_temp){
		printk("sw_gpio_irq_request fail %d\n",__LINE__);
		ret = -1;
		gpio_result[11].result =1;
	}

	handle_temp_second	= sw_gpio_irq_request(upio_index, sunxi_gpio_test->trigger,(peint_handle)gpio_irq_handle_demo, &flags);

	if(!handle_temp_second){
		printk("repeat_request_irq fail %d\n",__LINE__);
		ret = -1;
		gpio_result[11].result =1;
	}
	sw_gpio_irq_free(handle_temp);
	return ret;
}

static ssize_t exec_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->exec);
}

static ssize_t exec_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
/*	ssize_t ret = -EINVAL;*/
	char *after;
	int final=0;
	int exec_number = simple_strtoul(buf, &after, 10);
	switch(exec_number){
	case 0:
		final = set_cfg_standard_api(sunxi_gpio_test);
	break;
	case 1:
		final = set_cfg_api(sunxi_gpio_test);
	break;
	case 2:
		final = set_drv_api(sunxi_gpio_test);
	break;
	case 3:
		final = set_pin_data_standard_api(sunxi_gpio_test);
	break;
	case 4:
		final = set_pin_pull_api(sunxi_gpio_test);
	break;
	case 5:
		final = set_pin_data_api(sunxi_gpio_test);
	break;
	case 6:
		final = pin_eirq_request(sunxi_gpio_test);
	break;
	case 7:
		final = pin_script_request(sunxi_gpio_test);
	break;
	case 8:
		final = pin_request_inexistent(sunxi_gpio_test);
	break;
	case 9:
		final = pin_rqst_inexistent_irq(sunxi_gpio_test);
	break;
	case 10:
		final = repeat_request(sunxi_gpio_test);
	break;
	case 11:
		final = repeat_request_irq(sunxi_gpio_test);
	break;
	default :
		printk("you input number too large!\n");
		final = 1;
	break;
	}

	sunxi_gpio_test->exec	= exec_number;

	if(final){
		gpio_result[exec_number].result =1;
	}else{
		if(gpio_result[exec_number].result == 2)
			gpio_result[exec_number].result = 0;
	}

	return size;
}

static ssize_t group_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%c\n", sunxi_gpio_test->group + 2*48);
}

static ssize_t group_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int group = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->group	= group;
	}

	return ret;
}

static ssize_t number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->number);
}

static ssize_t number_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->number	= number;
	}

	return ret;
}

static ssize_t testcase_number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", testcase_number);
}

static ssize_t testcase_number_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;
	char *after;
	int number = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		testcase_number	= number;
	}

	return ret;
}

static ssize_t functions_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->functions);
}

static ssize_t functions_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int functions = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->functions	= functions;
	}

	return ret;
}

static ssize_t data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->data);
}

static ssize_t data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int data = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->data	= data;
	}

	return ret;
}

static ssize_t drv_lvl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->drv_lvl);
}

static ssize_t drv_lvl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int drv_lvl = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->drv_lvl	= drv_lvl;
	}

	return ret;
}

static ssize_t pull_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->pull);
}

static ssize_t pull_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int pull = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->pull	= pull;
	}

	return ret;
}

static ssize_t trigger_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sunxi_gpio_test->trigger);
}

static ssize_t trigger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int trigger = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		sunxi_gpio_test->trigger	= trigger;
	}

	return ret;
}

static ssize_t result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u",gpio_result[testcase_number].result);
}

/*this functions only be used to print without store result*/
static ssize_t result_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	/*reserve a point which point to sunxi_gpio_test_class*/
	//struct sunxi_gpio_test_class *sunxi_gpio_test = dev_get_drvdata(dev);
	int i;

	for(i=0;i<GPIO_TEST_NUMBER;i++){
		if(gpio_result[i].result==1){
			printk("%s: fail!\n",gpio_result[i].name);
		}else if(gpio_result[i].result==2){
			printk("%s: have no test!\n",gpio_result[i].name);
		}else{
			printk("%s: pass!\n",gpio_result[i].name);
		}
	}
	return size;
}

static struct device_attribute gpio_class_attrs[] = {
	__ATTR(exec, 0644, exec_show, exec_store),
	__ATTR(group, 0644, group_show, group_store),
	__ATTR(number, 0644, number_show, number_store),
	__ATTR(functions, 0644, functions_show, functions_store),
	__ATTR(data, 0644, data_show, data_store),
	__ATTR(drv_lvl, 0644, drv_lvl_show, drv_lvl_store),
	__ATTR(pull, 0644, pull_show, pull_store),
/*	__ATTR(irq, 0644, irq_show, irq_store),*/
	__ATTR(trigger, 0644, trigger_show, trigger_store),
	__ATTR(result, 0644,result_show,result_store),
	__ATTR(testcase_number, 0644,testcase_number_show,testcase_number_store),
	__ATTR_NULL,
};


static int __devexit sunxi_gpio_test_remove(struct platform_device *dev)
{
    struct sunxi_gpio_test_class *sunxi_gpio_test    = platform_get_drvdata(dev);

	device_unregister(sunxi_gpio_test->dev);
	kfree(sunxi_gpio_test);

	return 0;
}

static int __devinit sunxi_gpio_test_probe(struct platform_device *dev)
{
	struct sunxi_gpio_test_class *sunxi_gpio_test;

	sunxi_gpio_test = kzalloc(sizeof(struct sunxi_gpio_test_class), GFP_KERNEL);
	if (sunxi_gpio_test == NULL) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}
	platform_set_drvdata(dev, sunxi_gpio_test);
	sunxi_gpio_test->dev = device_create(sunxi_gpio_test_init_class, &dev->dev, 0, sunxi_gpio_test,"sunxi_gpio_test");

	if (IS_ERR(sunxi_gpio_test->dev))
		return PTR_ERR(sunxi_gpio_test->dev);
	return 0;
}

static void gpio_release (struct device *dev)
{
	PIN_TEST_DEBUG("gpio_sw_release good !\n");
}

static struct platform_driver sunxi_gpio_test_driver = {
	.probe		= sunxi_gpio_test_probe,
	.remove		= sunxi_gpio_test_remove,
	.driver		= {
		.name		= "sunxi_gpio_test",
		.owner		= THIS_MODULE,
	},
};

static struct platform_device sunxi_gpio_test_devices = {
    .name = "sunxi_gpio_test",
    .id = 0,
    .dev = {
        .release = gpio_release,
    },
};

static int __init sunxi_gpio_test_init(void)
{
	int ret,i;
	for(i=0;i<GPIO_TEST_NUMBER;i++){
		gpio_result[i].result=2;
	}
	sunxi_gpio_test_init_class = class_create(THIS_MODULE, "sunxi_gpio_test_init_class");
	if (IS_ERR(sunxi_gpio_test_init_class))
		return PTR_ERR(sunxi_gpio_test_init_class);

	ret	= platform_driver_register(&sunxi_gpio_test_driver);
	if(ret < 0)
		goto err_gpio_platform_driver_register;
	sunxi_gpio_test_init_class->dev_attrs 	= gpio_class_attrs;
	mutex_init(&sunxi_gpio_test_mutex);

	ret	= platform_device_register(&sunxi_gpio_test_devices);
	if(ret < 0)
		goto err_sunxi_gpio_test_platform_device_register;

	return ret;

err_sunxi_gpio_test_platform_device_register:
	platform_driver_unregister(&sunxi_gpio_test_driver);
	mutex_destroy(&sunxi_gpio_test_mutex);
err_gpio_platform_driver_register:
	return ret;
}

static void __exit sunxi_gpio_test_exit(void)
{
	mutex_destroy(&sunxi_gpio_test_mutex);
	platform_device_unregister(&sunxi_gpio_test_devices);
	platform_driver_unregister(&sunxi_gpio_test_driver);
	class_destroy(sunxi_gpio_test_init_class);
}
module_init(sunxi_gpio_test_init);
module_exit(sunxi_gpio_test_exit);

MODULE_AUTHOR("panlong");
MODULE_DESCRIPTION("so ugly driver for company`s task, so lose face");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio_sw_test");
