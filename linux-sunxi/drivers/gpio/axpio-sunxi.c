#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>

static int enable_axpio_power(void){
    int i,cnt,ret;
    script_item_u val;
    struct regulator* ldo = NULL;

    ret = script_get_item("axp_para", "axp_num", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != ret) {
        printk(KERN_ERR "can no find axp num para\n");
        return 0;
    }
    cnt = val.val;
    if(!cnt){
        printk(KERN_ERR "these is zero number for axp io\n");
        return 0;
    }else{
        printk("there is %d number for axp io\n",cnt);
    }
    for(i = 0; i < cnt; i++){
        char axp_n[16];
        sprintf(axp_n,"axp_%d",i);
        printk("axp name:%s",axp_n);
        ret = script_get_item("axp_para", axp_n, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_STR != ret) {
            printk(KERN_ERR "can no find axp %s\n", axp_n);
            continue;
        }
        ldo = regulator_get(NULL, val.str);
        if(!ldo){
            printk("a unknown axp io name %s\n", val.str);
            continue;
        }
        ret = regulator_set_voltage(ldo, 3300000, 3300000);
        if(ret < 0){
            printk("set voltage for %s fail\n", val.str);
            regulator_put(ldo);
            continue;
        }
        ret = regulator_enable(ldo);
        if (ret < 0) {
            printk("enable for %s fail\n", val.str);
        }
        regulator_put(ldo);
    }
	return 0;
}

static int disable_axpio_power(void){
    int i,cnt,ret;
    script_item_u val;
    struct regulator* ldo = NULL;

    ret = script_get_item("axp_para", "axp_num", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != ret) {
        printk(KERN_ERR "can no find axp num para\n");
        return 0;
    }
    /*
    cnt = script_get_pio_list("axp_para", &list);
    */
    cnt = val.val;
    if(!cnt){
        printk(KERN_ERR "these is zero number for axp io\n");
        return 0;
    }else{
        printk("there is %d number for axp io\n",cnt);
    }
    for(i = 0; i < cnt; i++){
        char axp_n[16];
        sprintf(axp_n,"axp_%d",i);
        printk("axp name:%s",axp_n);
        ret = script_get_item("axp_para", axp_n, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_STR != ret) {
            printk(KERN_ERR "can no find axp %s\n", axp_n);
            continue;
        }
        ldo = regulator_get(NULL, val.str);
        if(!ldo){
            printk("a unknown axp io name %s\n", val.str);
            continue;
        }
        ret = regulator_disable(ldo);
        if (ret < 0) {
            printk("disable for %s fail\n", val.str);
        }
        regulator_put(ldo);
    }
	return 0;
}



#ifdef CONFIG_PM
static int axpio_sw_suspend(struct device *dev){
    //disable the axp io when suspend
    printk("axp io suspend\n");
    disable_axpio_power();
	return 0;
}

static int axpio_sw_resume(struct device *dev){
    //enable the axp io when resume
    printk("axp io resume\n");
    enable_axpio_power();
	return 0;
}

static struct dev_pm_ops axpio_sw_ops = {
    .suspend = axpio_sw_suspend,
    .resume  = axpio_sw_resume,
};
#endif

static struct platform_device axpio_sw_dev = {
    .name = "axpio_sw",
};

static struct platform_driver axpio_sw_driver = {
    .driver.name = "axpio_sw",
    .driver.owner = THIS_MODULE,
#ifdef CONFIG_PM
    .driver.pm = &axpio_sw_ops,
#endif
};

/*
* try to init AXP IO,base on config
*/
static int __init axpio_sw_init(void){
    printk("__init axp io\n");
    enable_axpio_power();

    platform_device_register(&axpio_sw_dev);
    return platform_driver_register(&axpio_sw_driver);
}

static void __exit axpio_sw_exit(void){
	printk("__exit axp io\n");
    disable_axpio_power();
    platform_driver_unregister(&axpio_sw_driver);
}

module_init(axpio_sw_init);
module_exit(axpio_sw_exit);

MODULE_AUTHOR("chenjd");
MODULE_DESCRIPTION("SW AXP IO INIT driver");
MODULE_LICENSE("GPL");