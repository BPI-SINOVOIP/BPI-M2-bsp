/*
 * Battery charger driver for X-Powers AXP22X
 *
 * Copyright (C) 2012 X-Powers, Ltd.
 *  Weijin Zhong <zhwj@x-powers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/mfd/axp-mfd.h>
#include <asm/div64.h>

#include <mach/sys_config.h>

#include <asm-generic/gpio.h>
#include <mach/gpio.h>
#include <mach/ar100.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "axp-cfg.h"
#include "axp-sply.h"

#define DBG_AXP_PSY 1
#if  DBG_AXP_PSY
#define DBG_PSY_MSG(format,args...)   printk("[AXP]"format,##args)
#else
#define DBG_PSY_MSG(format,args...)   do {} while (0)
#endif

static int axp_debug = 0;
static int vbus_curr_limit_debug = 1;
static int long_key_power_off = 1;
struct axp_adc_res adc;
struct delayed_work usbwork;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend axp_early_suspend;
int early_suspend_flag = 0;
#endif

int pmu_usbvolnew = 0;
int pmu_usbcurnew = 0;
int axp_usbcurflag = 0;
int axp_usbvolflag = 0;

int axp_usbvol(void)
{
	axp_usbvolflag = 1;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usb_det(void)
{
	uint8_t ret = 0;
	
	if(axp_charger == 0 || axp_charger->master == 0)
	{
		printk("get axp usb detect failure!");
		return ret;
	}
	axp_read(axp_charger->master,AXP22_CHARGE_STATUS,&ret);
	if(ret & 0x10)/*usb or usb adapter can be used*/
	{
		return 1;
	}
	else/*no usb or usb adapter*/
	{
		return 0;
	}
}
EXPORT_SYMBOL_GPL(axp_usb_det);

int axp_spk_det(void)
{
	uint8_t ret = 0;
	
	if(axp_charger == 0 || axp_charger->master == 0)
	{
		printk("get axp spk detect failure!");
		return ret;
	}
	axp_read(axp_charger->master,AXP22_CHARGE_STATUS,&ret);
	if((ret & 0x10)|| (ret & 0x40))/*usb or usb/ac adapter can be used*/
	{
		return 1;
	}
	else/*no usb or usb/ac adapter*/
	{
		return 0;
	}
}
EXPORT_SYMBOL_GPL(axp_spk_det);


int axp_usbcur(void)
{
    axp_usbcurflag = 1;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

int axp_usbvol_restore(void)
{
 	axp_usbvolflag = 0;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol_restore);

int axp_usbcur_restore(void)
{
	axp_usbcurflag = 0;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur_restore);

static ssize_t axpdebug_store(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1'){
	   axp_debug = 1; 
    }
    else{
	   axp_debug = 0;         
    }        
	return count;
}

static ssize_t axpdebug_show(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "bat-debug value is %d\n", axp_debug);
}

//static struct class_attribute axppower_class_attrs[] = {
//	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
//	__ATTR_NULL
//};
static ssize_t vbuslimit_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1'){
	   vbus_curr_limit_debug = 1;
    }
    else{
	   vbus_curr_limit_debug = 0;
    } 
	return count;
}

static ssize_t vbuslimit_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "vbus curr limit value is %d\n", vbus_curr_limit_debug);
}
/*for long key power off control added by zhwj at 20130502*/
static ssize_t longkeypoweroff_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	uint8_t addr;
	uint8_t data;
	if(buf[0] == '1'){
		long_key_power_off = 1;
	}
	else{
		long_key_power_off = 0;
	} 
	/*for long key power off control added by zhwj at 20130502*/
	addr = AXP22_POK_SET;
	ar100_axp_read_reg(&addr , &data, 1);
	data &= 0xf7;
	data |= (long_key_power_off << 3);
	ar100_axp_write_reg(&addr , &data, 1);
	return count;
}

static ssize_t longkeypoweroff_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "long key power off value is %d\n", long_key_power_off);
}

static ssize_t out_factory_mode_show(struct class *class,
    struct class_attribute *attr, char *buf)
{
  uint8_t addr = AXP22_BUFFERC;
  uint8_t data;
  ar100_axp_read_reg(&addr , &data, 1);
  return sprintf(buf, "0x%x\n",data);
}

static ssize_t out_factory_mode_store(struct class *class,
        struct class_attribute *attr, const char *buf, size_t count)
{
  uint8_t addr = AXP22_BUFFERC;
  uint8_t data;
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var){
    data = 0x0d;
    ar100_axp_write_reg(&addr , &data, 1);
  }
  else{
    data = 0x00;
    ar100_axp_write_reg(&addr , &data, 1);
  }
  return count;
}

static struct class_attribute axppower_class_attrs[] = {
	__ATTR(vbuslimit,S_IRUGO|S_IWUSR,vbuslimit_show,vbuslimit_store),
	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
	__ATTR(longkeypoweroff,S_IRUGO|S_IWUSR,longkeypoweroff_show,longkeypoweroff_store),
	__ATTR(out_factory_mode,S_IRUGO|S_IWUSR,out_factory_mode_show,out_factory_mode_store),
	__ATTR_NULL
};
static struct class axppower_class = {
    .name = "axppower",
    .class_attrs = axppower_class_attrs,
};

int ADC_Freq_Get(struct axp_charger *charger)
{
	uint8_t  temp;
	int  rValue = 25;

	axp_read(charger->master, AXP22_ADC_CONTROL3,&temp);
	temp &= 0xc0;
	switch(temp >> 6)
	{
		case 0:
			rValue = 100;
			break;
		case 1:
			rValue = 200;
			break;
		case 2:
			rValue = 400;
			break;
		case 3:
			rValue = 800;
			break;
		default:
			break;
	}
	return rValue;
}

static inline int axp22_vts_to_temp(int data)
{
	int temp;
	if (data < 80)
		return 30;
	else if (data < pmu_temp_para16)
		return 80;
	else if (data <= pmu_temp_para15) {
		temp = 70 + (pmu_temp_para15-data)*10/(pmu_temp_para15-pmu_temp_para16);
	} else if (data <= pmu_temp_para14) {
		temp = 60 + (pmu_temp_para14-data)*10/(pmu_temp_para14-pmu_temp_para15);
	} else if (data <= pmu_temp_para13) {
		temp = 55 + (pmu_temp_para13-data)*5/(pmu_temp_para13-pmu_temp_para14);
	} else if (data <= pmu_temp_para12) {
		temp = 50 + (pmu_temp_para12-data)*5/(pmu_temp_para12-pmu_temp_para13);
	} else if (data <= pmu_temp_para11) {
		temp = 45 + (pmu_temp_para11-data)*5/(pmu_temp_para11-pmu_temp_para12);
	} else if (data <= pmu_temp_para10) {
		temp = 40 + (pmu_temp_para10-data)*5/(pmu_temp_para10-pmu_temp_para11);
	} else if (data <= pmu_temp_para9) {
		temp = 30 + (pmu_temp_para9-data)*10/(pmu_temp_para9-pmu_temp_para10);
	} else if (data <= pmu_temp_para8) {
		temp = 20 + (pmu_temp_para8-data)*10/(pmu_temp_para8-pmu_temp_para9);
	} else if (data <= pmu_temp_para7) {
		temp = 10 + (pmu_temp_para7-data)*10/(pmu_temp_para7-pmu_temp_para8);
	} else if (data <= pmu_temp_para6) {
		temp = 5 + (pmu_temp_para6-data)*5/(pmu_temp_para6-pmu_temp_para7);
	} else if (data <= pmu_temp_para5) {
		temp = 0 + (pmu_temp_para5-data)*5/(pmu_temp_para5-pmu_temp_para6);
	} else if (data <= pmu_temp_para4) {
		temp = -5 + (pmu_temp_para4-data)*5/(pmu_temp_para4-pmu_temp_para5);
	} else if (data <= pmu_temp_para3) {
		temp = -10 + (pmu_temp_para3-data)*5/(pmu_temp_para3-pmu_temp_para4);
	} else if (data <= pmu_temp_para2) {
		temp = -15 + (pmu_temp_para2-data)*5/(pmu_temp_para2-pmu_temp_para3);
	} else if (data <= pmu_temp_para1) {
		temp = -25 + (pmu_temp_para1-data)*10/(pmu_temp_para1-pmu_temp_para2);
	} else
		temp = -25;
    return temp;
}

static inline int axp22_vbat_to_mV(uint16_t reg)
{
  return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}

static inline int axp22_ocvbat_to_mV(uint16_t reg)
{
  return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}


static inline int axp22_vdc_to_mV(uint16_t reg)
{
  return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 1700 / 1000;
}


static inline int axp22_ibat_to_mA(uint16_t reg)
{
    return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) ;
}

static inline int axp22_icharge_to_mA(uint16_t reg)
{
    return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F)));
}

static inline int axp22_iac_to_mA(uint16_t reg)
{
    return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 625 / 1000;
}

static inline int axp22_iusb_to_mA(uint16_t reg)
{
    return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 375 / 1000;
}

static inline int axp22_vts_to_mV(uint16_t reg)
{
    return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 800 / 1000;
}

static inline void axp_read_adc(struct axp_charger *charger,
  struct axp_adc_res *adc)
{
  uint8_t tmp[8];
//
//  axp_reads(charger->master,AXP22_VACH_RES,8,tmp);
  adc->vac_res = 0;
  adc->iac_res = 0;
  adc->vusb_res = 0;
  adc->iusb_res = 0;
  axp_reads(charger->master,AXP22_VBATH_RES,6,tmp);
  adc->vbat_res = ((uint16_t) tmp[0] << 8 )| tmp[1];
  adc->ichar_res = ((uint16_t) tmp[2] << 8 )| tmp[3];
  adc->idischar_res = ((uint16_t) tmp[4] << 8 )| tmp[5];
  axp_reads(charger->master,AXP22_OCVBATH_RES,2,tmp);
  adc->ocvbat_res = ((uint16_t) tmp[0] << 8 )| tmp[1];

  axp_reads(charger->master,AXP22_VTS_RES,2,tmp);
  adc->ts_res = ((uint16_t) tmp[0] << 8 )| tmp[1];
}


static void axp_charger_update_state(struct axp_charger *charger)
{
  uint8_t val[2];
  uint16_t tmp;
  static int axp_usbvolflag_state = 0;
  axp_reads(charger->master,AXP22_CHARGE_STATUS,2,val);
  tmp = (val[1] << 8 )+ val[0];
  charger->is_on = (val[1] & AXP22_IN_CHARGE) ? 1 : 0;
  charger->fault = val[1];
  charger->bat_det = (tmp & AXP22_STATUS_BATEN)?1:0;
  charger->ac_det = (tmp & AXP22_STATUS_ACEN)?1:0;
  charger->usb_det = (tmp & AXP22_STATUS_USBEN)?1:0;
  charger->usb_valid = (tmp & AXP22_STATUS_USBVA)?1:0;
  charger->ac_valid = (tmp & AXP22_STATUS_ACVA)?1:0;
  charger->ext_valid = charger->ac_valid | charger->usb_valid;
  charger->bat_current_direction = (tmp & AXP22_STATUS_BATCURDIR)?1:0;
  charger->in_short = (tmp& AXP22_STATUS_ACUSBSH)?1:0;
  charger->batery_active = (tmp & AXP22_STATUS_BATINACT)?1:0;
  charger->int_over_temp = (tmp & AXP22_STATUS_ICTEMOV)?1:0;
  axp_read(charger->master,AXP22_CHARGE_CONTROL1,val);
  charger->charge_on = ((val[0] >> 7) & 0x01);

    if(axp_usbvolflag == 1){
        charger->usb_valid = 1;
        charger->ac_valid = 0;
    }

    if(axp_usbvolflag_state != axp_usbvolflag){
        power_supply_changed(&charger->batt);
        axp_usbvolflag_state = axp_usbvolflag;
   }
}

static void axp_charger_update(struct axp_charger *charger)
{
  uint16_t tmp;
  int bat_temp_mv;
  uint8_t val[2];
  //struct axp_adc_res adc;
  charger->adc = &adc;
  axp_read_adc(charger, &adc);
  tmp = charger->adc->vbat_res;
  charger->vbat = axp22_vbat_to_mV(tmp);
  tmp = charger->adc->ocvbat_res;
  charger->ocv = axp22_ocvbat_to_mV(tmp);
   //tmp = charger->adc->ichar_res + charger->adc->idischar_res;
  charger->ibat = ABS(axp22_icharge_to_mA(charger->adc->ichar_res)-axp22_ibat_to_mA(charger->adc->idischar_res));
  tmp = 00;
  charger->vac = axp22_vdc_to_mV(tmp);
  tmp = 00;
  charger->iac = axp22_iac_to_mA(tmp);
  tmp = 00;
  charger->vusb = axp22_vdc_to_mV(tmp);
  tmp = 00;
  charger->iusb = axp22_iusb_to_mA(tmp);
  axp_reads(charger->master,AXP22_INTTEMP,2,val);
  //DBG_PSY_MSG("TEMPERATURE:val1=0x%x,val2=0x%x\n",val[1],val[0]);
  tmp = (val[0] << 4 ) + (val[1] & 0x0F);
  charger->ic_temp = (int) tmp *1063/10000  - 2667/10;
  charger->disvbat =  charger->vbat;
  charger->disibat =  axp22_ibat_to_mA(charger->adc->idischar_res);
  tmp = charger->adc->ts_res;
  bat_temp_mv = axp22_vts_to_mV(tmp);
  charger->bat_temp = axp22_vts_to_temp(bat_temp_mv);
}

#if defined  (CONFIG_AXP_CHARGEINIT)
static void axp_set_charge(struct axp_charger *charger)
{
  uint8_t val=0x00;
  uint8_t tmp=0x00;
    if(charger->chgvol < 4200000){
      val &= ~(3 << 5);
	  //val |= 1 << 5;
      }
    else if (charger->chgvol<4220000)		
		{
			  val &= ~(3 << 5);
			  val |= 1 << 6;
		}

    else if (charger->chgvol<4240000){
      val &= ~(3 << 5);
      val |= 1 << 5;
      }
    else
      val |= 3 << 5;

		if(charger->chgcur == 0)
			charger->chgen = 0;

    if(charger->chgcur< 300000)
      charger->chgcur = 300000;
    else if(charger->chgcur > 2550000)
     charger->chgcur = 2550000;

    val |= (charger->chgcur - 300000) / 150000 ;
    if(charger ->chgend == 10){
      val &= ~(1 << 4);
    }
    else {
      val |= 1 << 4;
    }
    val &= 0x7F;
    val |= charger->chgen << 7;
      if(charger->chgpretime < 30)
      charger->chgpretime = 30;
    if(charger->chgcsttime < 360)
      charger->chgcsttime = 360;

    tmp = ((((charger->chgpretime - 40) / 10) << 6)  \
      | ((charger->chgcsttime - 360) / 120));
	axp_write(charger->master, AXP22_CHARGE_CONTROL1,val);
	axp_update(charger->master, AXP22_CHARGE_CONTROL2,tmp,0xC2);
}
#else
static void axp_set_charge(struct axp_charger *charger)
{

}
#endif

static enum power_supply_property axp_battery_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_STATUS,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_HEALTH,
  POWER_SUPPLY_PROP_TECHNOLOGY,
  POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
  POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
  //POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
  //POWER_SUPPLY_PROP_CHARGE_FULL,
  POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
  POWER_SUPPLY_PROP_CAPACITY,
  //POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
  //POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
  POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property axp_ac_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property axp_usb_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void axp_battery_check_status(struct axp_charger *charger,
            union power_supply_propval *val)
{
  if (charger->bat_det) {
    if (charger->ext_valid){
    	if( charger->rest_vol == 100)
        val->intval = POWER_SUPPLY_STATUS_FULL;
    	else if(charger->charge_on)
    		val->intval = POWER_SUPPLY_STATUS_CHARGING;
    	else
    		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
    }
    else
      val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
  }
  else
    val->intval = POWER_SUPPLY_STATUS_FULL;
}

static void axp_battery_check_health(struct axp_charger *charger,
            union power_supply_propval *val)
{
    if (charger->fault & AXP22_FAULT_LOG_BATINACT)
    val->intval = POWER_SUPPLY_HEALTH_DEAD;
  else if (charger->fault & AXP22_FAULT_LOG_OVER_TEMP)
    val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
  else if (charger->fault & AXP22_FAULT_LOG_COLD)
    val->intval = POWER_SUPPLY_HEALTH_COLD;
  else
    val->intval = POWER_SUPPLY_HEALTH_GOOD;
}

static int axp_battery_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, batt);

  switch (psp) {
  case POWER_SUPPLY_PROP_STATUS:
    axp_battery_check_status(charger, val);
    break;
  case POWER_SUPPLY_PROP_HEALTH:
    axp_battery_check_health(charger, val);
    break;
  case POWER_SUPPLY_PROP_TECHNOLOGY:
    val->intval = charger->battery_info->technology;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
    val->intval = charger->battery_info->voltage_max_design;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
    val->intval = charger->battery_info->voltage_min_design;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    //val->intval = charger->ocv * 1000;
    val->intval = charger->vbat * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->ibat * 1000;
    break;
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->batt.name;
    break;
/*  case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
  case POWER_SUPPLY_PROP_CHARGE_FULL:
    val->intval = charger->battery_info->charge_full_design;
        break;
*/
  case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
    val->intval = charger->battery_info->energy_full_design;
  //  DBG_PSY_MSG("POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:%d\n",val->intval);
       break;
  case POWER_SUPPLY_PROP_CAPACITY:
    val->intval = charger->rest_vol;
    break;
/*  case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
    if(charger->bat_det && !(charger->is_on) && !(charger->ext_valid))
      val->intval = charger->rest_time;
    else
      val->intval = 0;
    break;
  case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
    if(charger->bat_det && charger->is_on)
      val->intval = charger->rest_time;
    else
      val->intval = 0;
    break;
*/
  case POWER_SUPPLY_PROP_ONLINE:
  {
  	/* in order to get hardware state, we must update charger state now.
  	 * by sunny at 2012-12-23 11:06:15.
  	 */
  	axp_charger_update_state(charger);
	val->intval = charger->bat_current_direction;
	if (charger->bat_temp > 50 || -5 < charger->bat_temp)
		val->intval = 0;
    break;
  }
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->bat_det;
    break;
  case POWER_SUPPLY_PROP_TEMP:
    //val->intval = charger->ic_temp - 200;
    if(1 == pmu_temp_protect_en) {
        val->intval =  charger->bat_temp * 10;
    } else {
        val->intval = 300;
    }
    break;
  default:
    ret = -EINVAL;
    break;
  }

  return ret;
}

static int axp_ac_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, ac);

  switch(psp){
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->ac.name;break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->ac_det;
    break;
  case POWER_SUPPLY_PROP_ONLINE:
    val->intval = charger->ac_valid;break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = charger->vac * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->iac * 1000;
    break;
  default:
    ret = -EINVAL;
    break;
  }
   return ret;
}

static int axp_usb_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, usb);

  switch(psp){
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->usb.name;break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->usb_det;
    break;
  case POWER_SUPPLY_PROP_ONLINE:
    val->intval = charger->usb_valid;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = charger->vusb * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->iusb * 1000;
    break;
  default:
    ret = -EINVAL;
    break;
  }
   return ret;
}

static void axp_change(struct axp_charger *charger)
{
	if(axp_debug)
		DBG_PSY_MSG("battery state change\n");
	axp_charger_update_state(charger);
	axp_charger_update(charger);
	cancel_delayed_work_sync(&usbwork);
	//set usb current limit to 500mA added by zhwj at 20131024
	axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
	axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
	schedule_delayed_work(&usbwork, msecs_to_jiffies(5* 1000));
  flag_state_change = 1;
  power_supply_changed(&charger->batt);
}

static void axp_presslong(struct axp_charger *charger)
{
	DBG_PSY_MSG("press long\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	ssleep(2);
	DBG_PSY_MSG("press long up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_pressshort(struct axp_charger *charger)
{
	DBG_PSY_MSG("press short\n");
  	input_report_key(powerkeydev, KEY_POWER, 1);
 	input_sync(powerkeydev);
 	msleep(100);
 	input_report_key(powerkeydev, KEY_POWER, 0);
 	input_sync(powerkeydev);
}

static void axp_keyup(struct axp_charger *charger)
{
	if(axp_debug)
		DBG_PSY_MSG("power key up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_keydown(struct axp_charger *charger)
{
	if(axp_debug)
		DBG_PSY_MSG("power key down\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
}

static void axp_capchange(struct axp_charger *charger)
{
	uint8_t val;
	int k;
	if(axp_debug)
	DBG_PSY_MSG("battery change\n");
	ssleep(2);
    axp_charger_update_state(charger);
    axp_charger_update(charger);
    axp_read(charger->master, AXP22_CAP,&val);
    charger->rest_vol = (int) (val & 0x7F);

    if((charger->bat_det == 0) || (charger->rest_vol == 127)){
  	charger->rest_vol = 100;
  }

  DBG_PSY_MSG("rest_vol = %d\n",charger->rest_vol);
  memset(Bat_Cap_Buffer, 0, sizeof(Bat_Cap_Buffer));
  for(k = 0;k < AXP22_VOL_MAX; k++){
    Bat_Cap_Buffer[k] = charger->rest_vol;
  }
  Total_Cap = charger->rest_vol * AXP22_VOL_MAX;
  power_supply_changed(&charger->batt);
}

static int axp_battery_event(struct notifier_block *nb, unsigned long event,
        void *data)
{
    uint8_t value;

    struct axp_charger *charger =
    container_of(nb, struct axp_charger, nb);

    uint8_t w[9];
	if(axp_debug){
		DBG_PSY_MSG("axp_battery_event enter...\n");
	}
    if((bool)data==0){
		if(axp_debug){
    			DBG_PSY_MSG("low 32bit status...\n");
		}
			if(event & (AXP22_IRQ_BATIN|AXP22_IRQ_BATRE)) {
				axp_capchange(charger);
			}
	
			if(event & (AXP22_IRQ_ACIN|AXP22_IRQ_USBIN|AXP22_IRQ_ACOV|AXP22_IRQ_USBOV|AXP22_IRQ_CHAOV
						|AXP22_IRQ_CHAST|AXP22_IRQ_TEMOV|AXP22_IRQ_TEMLO)) {
				axp_change(charger);
			}
	
			if(event & (AXP22_IRQ_ACRE|AXP22_IRQ_USBRE)) {
				axp_change(charger);
			}
	
			if(event & AXP22_IRQ_POKLO) {
				axp_presslong(charger);
			}
	
			if(event & AXP22_IRQ_POKSH) {
				axp_pressshort(charger);
			}
			if(event & AXP22_IRQ_TEMLO) {
				axp_change(charger);
			}
			if(event & AXP22_IRQ_TEMOV) {
				axp_change(charger);
			}
			w[0] = (uint8_t) ((event) & 0xFF);
    		w[1] = AXP22_INTSTS2;
    		w[2] = (uint8_t) ((event >> 8) & 0xFF);
    		w[3] = AXP22_INTSTS3;
    		w[4] = (uint8_t) ((event >> 16) & 0xFF);
    		w[5] = AXP22_INTSTS4;
    		w[6] = (uint8_t) ((event >> 24) & 0xFF);
    		w[7] = AXP22_INTSTS5;
    		w[8] = 0;
	} else {
		axp_read(charger->master, AXP22_BUFFERB, &value);
		if (0x0e == value) {
			axp_write(charger->master, AXP22_BUFFERB, 0);
		} else {
			if((event) & (AXP22_IRQ_PEKFE>>32)) {
				axp_keydown(charger);
			}

			if((event) & (AXP22_IRQ_PEKRE>>32)) {
				axp_keyup(charger);
			}
		}
		if(axp_debug){
			DBG_PSY_MSG("high 32bit status...\n");
		}
		w[0] = 0;
    	w[1] = AXP22_INTSTS2;
    	w[2] = 0;
    	w[3] = AXP22_INTSTS3;
    	w[4] = 0;
    	w[5] = AXP22_INTSTS4;
    	w[6] = 0;
    	w[7] = AXP22_INTSTS5;
    	w[8] = (uint8_t) ((event) & 0xFF);;
	}
	if(axp_debug)
	{
		DBG_PSY_MSG("event = 0x%x\n",(int) event);
	}
	axp_writes(charger->master,AXP22_INTSTS1,9,w);

    return 0;
}

static char *supply_list[] = {
  "battery",
};



static void axp_battery_setup_psy(struct axp_charger *charger)
{
  struct power_supply *batt = &charger->batt;
  struct power_supply *ac = &charger->ac;
  struct power_supply *usb = &charger->usb;
  struct power_supply_info *info = charger->battery_info;

  batt->name = "battery";
  batt->use_for_apm = info->use_for_apm;
  batt->type = POWER_SUPPLY_TYPE_BATTERY;
  batt->get_property = axp_battery_get_property;

  batt->properties = axp_battery_props;
  batt->num_properties = ARRAY_SIZE(axp_battery_props);

  ac->name = "ac";
  ac->type = POWER_SUPPLY_TYPE_MAINS;
  ac->get_property = axp_ac_get_property;

  ac->supplied_to = supply_list,
  ac->num_supplicants = ARRAY_SIZE(supply_list),

  ac->properties = axp_ac_props;
  ac->num_properties = ARRAY_SIZE(axp_ac_props);

  usb->name = "usb";
  usb->type = POWER_SUPPLY_TYPE_USB;
  usb->get_property = axp_usb_get_property;

  usb->supplied_to = supply_list,
  usb->num_supplicants = ARRAY_SIZE(supply_list),

  usb->properties = axp_usb_props;
  usb->num_properties = ARRAY_SIZE(axp_usb_props);
};

#if defined  (CONFIG_AXP_CHARGEINIT)
static int axp_battery_adc_set(struct axp_charger *charger)
{
   int ret ;
   uint8_t val;
   uint8_t mask;

  /*enable adc and set adc */
  val= AXP22_ADC_BATVOL_ENABLE | AXP22_ADC_BATCUR_ENABLE;
  mask = AXP22_ADC_BATVOL_ENABLE | AXP22_ADC_BATCUR_ENABLE | AXP22_ADC_TSVOL_ENABLE;
  if(1 == pmu_temp_protect_en){
    val |= AXP22_ADC_TSVOL_ENABLE;
  } else {
    val &= (~AXP22_ADC_TSVOL_ENABLE);
  }
  ret = axp_update(charger->master, AXP22_ADC_CONTROL, val , mask);

  if (ret)
    return ret;
    ret = axp_read(charger->master, AXP22_ADC_CONTROL3, &val);
  switch (charger->sample_time/100){
  case 1: val &= ~(3 << 6);break;
  case 2: val &= ~(3 << 6);val |= 1 << 6;break;
  case 4: val &= ~(3 << 6);val |= 2 << 6;break;
  case 8: val |= 3 << 6;break;
  default: break;
  }

  if (ret)
    return ret;

  return 0;
}
#else
static int axp_battery_adc_set(struct axp_charger *charger)
{
  return 0;
}
#endif

static int axp_battery_first_init(struct axp_charger *charger)
{
   int ret;
   uint8_t val;
   axp_set_charge(charger);
   ret = axp_battery_adc_set(charger);
   if(ret)
    return ret;

   ret = axp_read(charger->master, AXP22_ADC_CONTROL3, &val);
   switch ((val >> 6) & 0x03){
  case 0: charger->sample_time = 100;break;
  case 1: charger->sample_time = 200;break;
  case 2: charger->sample_time = 400;break;
  case 3: charger->sample_time = 800;break;
  default:break;
  }
  return ret;
}

static ssize_t chgen_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
  charger->chgen  = val >> 7;
  return sprintf(buf, "%d\n",charger->chgen);
}

static ssize_t chgen_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var){
    charger->chgen = 1;
    axp_set_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);
  }
  else{
    charger->chgen = 0;
    axp_clr_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);
  }
  return count;
}

static ssize_t chgmicrovol_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
  switch ((val >> 5) & 0x03){
    case 0: charger->chgvol = 4100000;break;
    case 1: charger->chgvol = 4220000;break;
    case 2: charger->chgvol = 4200000;break;
    case 3: charger->chgvol = 4240000;break;
  }
  return sprintf(buf, "%d\n",charger->chgvol);
}

static ssize_t chgmicrovol_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t tmp, val;
  var = simple_strtoul(buf, NULL, 10);
  switch(var){
    case 4100000:tmp = 0;break;
    case 4220000:tmp = 1;break;
    case 4200000:tmp = 2;break;
    case 4240000:tmp = 3;break;
    default:  tmp = 4;break;
  }
  if(tmp < 4){
    charger->chgvol = var;
    axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
    val &= 0x9F;
    val |= tmp << 5;
    axp_write(charger->master, AXP22_CHARGE_CONTROL1, val);
  }
  return count;
}

static ssize_t chgintmicrocur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
  charger->chgcur = (val & 0x0F) * 150000 +300000;
  return sprintf(buf, "%d\n",charger->chgcur);
}

static ssize_t chgintmicrocur_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t val,tmp;
  var = simple_strtoul(buf, NULL, 10);
  if(var >= 300000 && var <= 2550000){
    tmp = (var -200001)/150000;
    charger->chgcur = tmp *150000 + 300000;
    axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
    val &= 0xF0;
    val |= tmp;
    axp_write(charger->master, AXP22_CHARGE_CONTROL1, val);
  }
  return count;
}

static ssize_t chgendcur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master, AXP22_CHARGE_CONTROL1, &val);
  charger->chgend = ((val >> 4)& 0x01)? 15 : 10;
  return sprintf(buf, "%d\n",charger->chgend);
}

static ssize_t chgendcur_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var == 10 ){
    charger->chgend = var;
    axp_clr_bits(charger->master ,AXP22_CHARGE_CONTROL1,0x10);
  }
  else if (var == 15){
    charger->chgend = var;
    axp_set_bits(charger->master ,AXP22_CHARGE_CONTROL1,0x10);

  }
  return count;
}

static ssize_t chgpretimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master,AXP22_CHARGE_CONTROL2, &val);
  charger->chgpretime = (val >> 6) * 10 +40;
  return sprintf(buf, "%d\n",charger->chgpretime);
}

static ssize_t chgpretimemin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t tmp,val;
  var = simple_strtoul(buf, NULL, 10);
  if(var >= 40 && var <= 70){
    tmp = (var - 40)/10;
    charger->chgpretime = tmp * 10 + 40;
    axp_read(charger->master,AXP22_CHARGE_CONTROL2,&val);
    val &= 0x3F;
    val |= (tmp << 6);
    axp_write(charger->master,AXP22_CHARGE_CONTROL2,val);
  }
  return count;
}

static ssize_t chgcsttimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master,AXP22_CHARGE_CONTROL2, &val);
  charger->chgcsttime = (val & 0x03) *120 + 360;
  return sprintf(buf, "%d\n",charger->chgcsttime);
}

static ssize_t chgcsttimemin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t tmp,val;
  var = simple_strtoul(buf, NULL, 10);
  if(var >= 360 && var <= 720){
    tmp = (var - 360)/120;
    charger->chgcsttime = tmp * 120 + 360;
    axp_read(charger->master,AXP22_CHARGE_CONTROL2,&val);
    val &= 0xFC;
    val |= tmp;
    axp_write(charger->master,AXP22_CHARGE_CONTROL2,val);
  }
  return count;
}

static ssize_t adcfreq_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master, AXP22_ADC_CONTROL3, &val);
  switch ((val >> 6) & 0x03){
     case 0: charger->sample_time = 100;break;
     case 1: charger->sample_time = 200;break;
     case 2: charger->sample_time = 400;break;
     case 3: charger->sample_time = 800;break;
     default:break;
  }
  return sprintf(buf, "%d\n",charger->sample_time);
}

static ssize_t adcfreq_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t val;
  var = simple_strtoul(buf, NULL, 10);
  axp_read(charger->master, AXP22_ADC_CONTROL3, &val);
  switch (var/25){
    case 1: val &= ~(3 << 6);charger->sample_time = 100;break;
    case 2: val &= ~(3 << 6);val |= 1 << 6;charger->sample_time = 200;break;
    case 4: val &= ~(3 << 6);val |= 2 << 6;charger->sample_time = 400;break;
    case 8: val |= 3 << 6;charger->sample_time = 800;break;
    default: break;
    }
  axp_write(charger->master, AXP22_ADC_CONTROL3, val);
  return count;
}


static ssize_t vholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master,AXP22_CHARGE_VBUS, &val);
  val = (val>>6) & 0x01;
  return sprintf(buf, "%d\n",val);
}

static ssize_t vholden_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var)
    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
  else
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);

  return count;
}

static ssize_t vhold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  int vhold;
  axp_read(charger->master,AXP22_CHARGE_VBUS, &val);
  vhold = ((val >> 3) & 0x07) * 100000 + 4000000;
  return sprintf(buf, "%d\n",vhold);
}

static ssize_t vhold_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  uint8_t val,tmp;
  var = simple_strtoul(buf, NULL, 10);
  if(var >= 4000000 && var <=4700000){
    tmp = (var - 4000000)/100000;
    //printk("tmp = 0x%x\n",tmp);
    axp_read(charger->master, AXP22_CHARGE_VBUS,&val);
    val &= 0xC7;
    val |= tmp << 3;
    //printk("val = 0x%x\n",val);
    axp_write(charger->master, AXP22_CHARGE_VBUS,val);
  }
  return count;
}

static ssize_t iholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val;
  axp_read(charger->master,AXP22_CHARGE_VBUS, &val);
  return sprintf(buf, "%d\n",((val & 0x03) == 0x03)?0:1);
}

static ssize_t iholden_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var)
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
  else
    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);

  return count;
}

static ssize_t ihold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  uint8_t val,tmp;
  int ihold;
  axp_read(charger->master,AXP22_CHARGE_VBUS, &val);
  tmp = (val) & 0x03;
  switch(tmp){
    case 0: ihold = 900000;break;
    case 1: ihold = 500000;break;
    default: ihold = 0;break;
  }
  return sprintf(buf, "%d\n",ihold);
}

static ssize_t ihold_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
  struct axp_charger *charger = dev_get_drvdata(dev);
  int var;
  var = simple_strtoul(buf, NULL, 10);
  if(var == 900000)
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
  else if (var == 500000){
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
  }
  return count;
}

static struct device_attribute axp_charger_attrs[] = {
  AXP_CHG_ATTR(chgen),
  AXP_CHG_ATTR(chgmicrovol),
  AXP_CHG_ATTR(chgintmicrocur),
  AXP_CHG_ATTR(chgendcur),
  AXP_CHG_ATTR(chgpretimemin),
  AXP_CHG_ATTR(chgcsttimemin),
  AXP_CHG_ATTR(adcfreq),
  AXP_CHG_ATTR(vholden),
  AXP_CHG_ATTR(vhold),
  AXP_CHG_ATTR(iholden),
  AXP_CHG_ATTR(ihold),
};

#if defined CONFIG_HAS_EARLYSUSPEND
static void axp_earlysuspend(struct early_suspend *h)
{
	uint8_t tmp;
	if(axp_debug)
		DBG_PSY_MSG("======early suspend=======\n");

#if defined (CONFIG_AXP_CHGCHANGE)
  	early_suspend_flag = 1;
  	if(pmu_earlysuspend_chgcur == 0)
  		axp_clr_bits(axp_charger->master,AXP22_CHARGE_CONTROL1,0x80);
    else if(1 == axp_charger->chgen){
  		axp_set_bits(axp_charger->master,AXP22_CHARGE_CONTROL1,0x80);

        if(pmu_earlysuspend_chgcur >= 300000 && pmu_earlysuspend_chgcur <= 2550000){
            tmp = (pmu_earlysuspend_chgcur -200001)/150000;
            axp_update(axp_charger->master, AXP22_CHARGE_CONTROL1, tmp,0x0F);
        }
    }
#endif

}
static void axp_lateresume(struct early_suspend *h)
{
	uint8_t tmp;
	uint8_t value;
	if(axp_debug)
		DBG_PSY_MSG("======late resume=======\n");

	axp_read(axp_charger->master, AXP22_BUFFERB, &value);
	if (0x0e == value)
		axp_write(axp_charger->master, AXP22_BUFFERB, 0);

#if defined (CONFIG_AXP_CHGCHANGE)
	early_suspend_flag = 0;
	if(pmu_runtime_chgcur == 0)
  		axp_clr_bits(axp_charger->master,AXP22_CHARGE_CONTROL1,0x80);
    else if(1 == axp_charger->chgen){
  		axp_set_bits(axp_charger->master,AXP22_CHARGE_CONTROL1,0x80);

        if(pmu_runtime_chgcur >= 300000 && pmu_runtime_chgcur <= 2550000){
            tmp = (pmu_runtime_chgcur -200001)/150000;
            axp_update(axp_charger->master, AXP22_CHARGE_CONTROL1, tmp,0x0F);
        } else if(pmu_runtime_chgcur < 300000){
            axp_clr_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
        } else {
            axp_set_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
        }
    }
#endif

}
#endif

int axp_charger_create_attrs(struct power_supply *psy)
{
  int j,ret;
  for (j = 0; j < ARRAY_SIZE(axp_charger_attrs); j++) {
    ret = device_create_file(psy->dev,
          &axp_charger_attrs[j]);
    if (ret)
      goto sysfs_failed;
  }
    goto succeed;

sysfs_failed:
  while (j--)
    device_remove_file(psy->dev,
         &axp_charger_attrs[j]);
succeed:
  return ret;
}

static void axp_charging_monitor(struct work_struct *work)
{
	struct axp_charger *charger;
	uint8_t	val,temp_val[4];
	int	pre_rest_vol,pre_bat_curr_dir;
//	uint16_t tmp;
	charger = container_of(work, struct axp_charger, work.work);
	pre_rest_vol = charger->rest_vol;
	pre_bat_curr_dir = charger->bat_current_direction;
	axp_charger_update_state(charger);
	axp_charger_update(charger);

	axp_read(charger->master, AXP22_CAP,&val);
	charger->rest_vol	= (int)	(val & 0x7F);
#if 0	
#if defined (CONFIG_AXP_CHGCHANGE)	
#if defined CONFIG_HAS_EARLYSUSPEND
		 	if(early_suspend_flag){
		 			if(pmu_earlysuspend_chgcur == 0){
		 				axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
		 			}
		 			else if(pmu_earlysuspend_chgcur >= 300000 && pmu_runtime_chgcur <= 1800000){
		 			  axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
						tmp = (pmu_earlysuspend_chgcur -200001)/150000;
						charger->chgcur = tmp *150000 + 300000;
						axp_update(charger->master, AXP20_CHARGE_CONTROL1, tmp, 0x0F);
					}
		 	}else
#endif
			{
				if(pmu_runtime_chgcur == 0){
					axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
				}
				else if (pmu_runtime_chgcur >= 300000 && pmu_runtime_chgcur <= 1800000){
					axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
    			tmp = (pmu_runtime_chgcur -200001)/150000;
    			charger->chgcur = tmp *150000 + 300000;
					axp_update(charger->master, AXP20_CHARGE_CONTROL1, tmp, 0x0F);
    			}else if(pmu_runtime_chgcur < 300000){
			    	axp_clr_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
			    }
			    else{
			    	axp_set_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
			    }
  			}
#endif
#endif
	if(axp_debug){
		DBG_PSY_MSG("charger->ic_temp = %d\n",charger->ic_temp);
		DBG_PSY_MSG("charger->bat_temp = %d\n",charger->bat_temp);
		DBG_PSY_MSG("charger->vbat = %d\n",charger->vbat);
		DBG_PSY_MSG("charger->ibat = %d\n",charger->ibat);
		DBG_PSY_MSG("charger->ocv = %d\n",charger->ocv);
		DBG_PSY_MSG("charger->disvbat = %d\n",charger->disvbat);
		DBG_PSY_MSG("charger->disibat = %d\n",charger->disibat);
		DBG_PSY_MSG("charger->rest_vol = %d\n",charger->rest_vol);
		axp_reads(charger->master,0xba,2,temp_val);
		DBG_PSY_MSG("Axp22 Rdc = %d\n",(((temp_val[0] & 0x1f) <<8) + temp_val[1])*10742/10000);
		axp_reads(charger->master,0xe0,2,temp_val);
		DBG_PSY_MSG("Axp22 batt_max_cap = %d\n",(((temp_val[0] & 0x7f) <<8) + temp_val[1])*1456/1000);
		axp_reads(charger->master,0xe2,2,temp_val);
		DBG_PSY_MSG("Axp22 coulumb_counter = %d\n",(((temp_val[0] & 0x7f) <<8) + temp_val[1])*1456/1000);
		axp_read(charger->master,0xb8,temp_val);
		DBG_PSY_MSG("Axp22 REG_B8 = %x\n",temp_val[0]);
		axp_reads(charger->master,0xe4,2,temp_val);
		DBG_PSY_MSG("Axp22 OCV_percentage = %d\n",(temp_val[0] & 0x7f));
		DBG_PSY_MSG("Axp22 Coulumb_percentage = %d\n",(temp_val[1] & 0x7f));
		DBG_PSY_MSG("charger->is_on = %d\n",charger->is_on);
		DBG_PSY_MSG("charger->bat_current_direction = %d\n",charger->bat_current_direction);
		DBG_PSY_MSG("charger->charge_on = %d\n",charger->charge_on);
		DBG_PSY_MSG("charger->ext_valid = %d\n",charger->ext_valid);
		DBG_PSY_MSG("pmu_runtime_chgcur           = %d\n",pmu_runtime_chgcur);
		DBG_PSY_MSG("pmu_earlysuspend_chgcur   = %d\n",pmu_earlysuspend_chgcur);
		DBG_PSY_MSG("pmu_suspend_chgcur        = %d\n",pmu_suspend_chgcur);
		DBG_PSY_MSG("pmu_shutdown_chgcur       = %d\n\n\n",pmu_shutdown_chgcur);
//		axp_chip_id_get(chip_id);
	}

	//for test usb detect
#if 0
	val = axp_usb_det();
	if(val)
	{
		printk("axp22 usb or usb adapter can be used!!\n");
	}
	else
	{ 
		printk("axp22 no usb or usb adaper!\n");
	}
#endif	

    /* add battery info print. */
    if(charger->rest_vol!=pre_rest_vol){
        if( (abs(charger->pre_print_vol-charger->rest_vol) >= 5) ||
            (charger->rest_vol<=5) ){
            charger->pre_print_vol = charger->rest_vol;
            printk(KERN_DEBUG "[axp]battery remaining power: %d\%\%\n", charger->rest_vol);
        }
    }

	/* if battery volume changed, inform uevent */
	if((charger->rest_vol - pre_rest_vol) || (charger->bat_current_direction != pre_bat_curr_dir)){
		if(axp_debug)
			{
				axp_reads(charger->master,0xe2,2,temp_val);
				axp_reads(charger->master,0xe4,2,(temp_val+2));
				printk("battery vol change: %d->%d \n", pre_rest_vol, charger->rest_vol);
				printk("for test %d %d %d %d %d %d\n",charger->vbat,charger->ocv,charger->ibat,(temp_val[2] & 0x7f),(temp_val[3] & 0x7f),(((temp_val[0] & 0x7f) <<8) + temp_val[1])*1456/1000);
			}
		pre_rest_vol = charger->rest_vol;
		power_supply_changed(&charger->batt);
	}
	/* reschedule for the next time */
	schedule_delayed_work(&charger->work, charger->interval);
}

static void axp_usb(struct work_struct *work)
{
	int var;
	uint8_t tmp,val;
	struct axp_charger *charger;
	
	charger = axp_charger;
	if(axp_debug) {
		printk("[axp_usb]axp_usbcurflag = %d\n",axp_usbcurflag);
	}
	axp_read(axp_charger->master,AXP22_CHARGE_STATUS,&val);
	if((val & 0x10) == 0x00){/*usb or usb adapter can not be used*/
		if(axp_debug)
			printk("USB not insert!\n");
		axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
		axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
	}else if(axp_usbcurflag){ //usb pc insert
		if(axp_debug){
			printk("set usbcur_pc %d mA\n",pmu_usbcur_pc);
		}
		if(pmu_usbcur_pc){
			axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
			var = pmu_usbcur_pc * 1000;
			if(var >= 900000)
				axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
			else if ((var >= 500000)&& (var < 900000)){
				axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
			}
			else{
				printk("set usb limit current error,%d mA\n",pmu_usbcur_pc);	
			} 				
		}
		else//not limit
			axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);			
	}else {
		if(axp_debug){
			printk("set usbcur %d mA\n",pmu_usbcur);
		}
		if((pmu_usbcur) && (pmu_usbcur_limit)){
			axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
			var = pmu_usbcur * 1000;
			if(var >= 900000)
				axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
			else if ((var >= 500000)&& (var < 900000)){
				axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
			}
			else
				printk("set usb limit current error,%d mA\n",pmu_usbcur);	
		}
		else //not limit
			axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
	}

	if(!vbus_curr_limit_debug){ //usb current not limit
		if(axp_debug)
			printk("vbus_curr_limit_debug = %d\n",vbus_curr_limit_debug);
		axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
	}
		
	if(axp_usbvolflag){
		if(axp_debug)
		{
			printk("set usbvol_pc %d mV\n",pmu_usbvol_pc);
		}
		if(pmu_usbvol_pc){
		    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
		  	var = pmu_usbvol_pc * 1000;
		  	if(var >= 4000000 && var <=4700000){
		    	tmp = (var - 4000000)/100000;
		    	axp_read(charger->master, AXP22_CHARGE_VBUS,&val);
		    	val &= 0xC7;
		    	val |= tmp << 3;
		    	axp_write(charger->master, AXP22_CHARGE_VBUS,val);
		  	}
		  	else
		  		printk("set usb limit voltage error,%d mV\n",pmu_usbvol_pc);	
		}
		else
		    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
	}else {
		if(axp_debug)
		{
			printk("set usbvol %d mV\n",pmu_usbvol);
		}
		if((pmu_usbvol) && (pmu_usbvol_limit)){
		    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
		  	var = pmu_usbvol * 1000;
		  	if(var >= 4000000 && var <=4700000){
		    	tmp = (var - 4000000)/100000;
		    	axp_read(charger->master, AXP22_CHARGE_VBUS,&val);
		    	val &= 0xC7;
		    	val |= tmp << 3;
		    	axp_write(charger->master, AXP22_CHARGE_VBUS,val);
		  	}
		  	else
		  		printk("set usb limit voltage error,%d mV\n",pmu_usbvol);	
		}
		else
		    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
	}
	
	schedule_delayed_work(&usbwork, msecs_to_jiffies(1* 1000));
}

static int axp_battery_probe(struct platform_device *pdev)
{
  struct axp_charger *charger;
  struct axp_supply_init_data *pdata = pdev->dev.platform_data;
  int ret,var;
  uint8_t val2,tmp,val;
  uint8_t ocv_cap[63];
  int Cur_CoulombCounter,rdc;
  
  powerkeydev = input_allocate_device();
  if (!powerkeydev) {
    kfree(powerkeydev);
    return -ENODEV;
  }

  powerkeydev->name = pdev->name;
  powerkeydev->phys = "m1kbd/input2";
  powerkeydev->id.bustype = BUS_HOST;
  powerkeydev->id.vendor = 0x0001;
  powerkeydev->id.product = 0x0001;
  powerkeydev->id.version = 0x0100;
  powerkeydev->open = NULL;
  powerkeydev->close = NULL;
  powerkeydev->dev.parent = &pdev->dev;

  set_bit(EV_KEY, powerkeydev->evbit);
  set_bit(EV_REL, powerkeydev->evbit);
  //set_bit(EV_REP, powerkeydev->evbit);
  set_bit(KEY_POWER, powerkeydev->keybit);

  ret = input_register_device(powerkeydev);
  if(ret) {
    printk("Unable to Register the power key\n");
    }

  if (pdata == NULL)
    return -EINVAL;

  printk("axp charger not limit now\n");
  if (pdata->chgcur > 2550000 ||
      pdata->chgvol < 4100000 ||
      pdata->chgvol > 4240000){
        printk("charger milliamp is too high or target voltage is over range\n");
        return -EINVAL;
    }

  if (pdata->chgpretime < 40 || pdata->chgpretime >70 ||
    pdata->chgcsttime < 360 || pdata->chgcsttime > 720){
            printk("prechaging time or constant current charging time is over range\n");
        return -EINVAL;
  }

  charger = kzalloc(sizeof(*charger), GFP_KERNEL);
  if (charger == NULL)
    return -ENOMEM;

  charger->master = pdev->dev.parent;

  charger->chgcur      = pdata->chgcur;
  charger->chgvol     = pdata->chgvol;
  charger->chgend           = pdata->chgend;
  charger->sample_time          = pdata->sample_time;
  charger->chgen                   = pdata->chgen;
  charger->chgpretime      = pdata->chgpretime;
  charger->chgcsttime = pdata->chgcsttime;
  charger->battery_info         = pdata->battery_info;
  charger->disvbat			= 0;
  charger->disibat			= 0;
  charger->pre_print_vol    = 100;

  ret = axp_battery_first_init(charger);
  if (ret)
    goto err_charger_init;

  axp_battery_setup_psy(charger);
  ret = power_supply_register(&pdev->dev, &charger->batt);
  if (ret)
    goto err_ps_register;

  ret = power_supply_register(&pdev->dev, &charger->ac);
    if (ret){
        power_supply_unregister(&charger->batt);
        goto err_ps_register;
  	}

  ret = power_supply_register(&pdev->dev, &charger->usb);
  if (ret){
    power_supply_unregister(&charger->ac);
    power_supply_unregister(&charger->batt);
    goto err_ps_register;
  }

  ret = axp_charger_create_attrs(&charger->batt);
  if(ret){
  	printk("cat notaxp_charger_create_attrs!!!===\n ");
    return ret;
  }

  platform_set_drvdata(pdev, charger);
  
  /* usb voltage limit */
  if((pmu_usbvol) && (pmu_usbvol_limit)){
    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
  	var = pmu_usbvol * 1000;
  	if(var >= 4000000 && var <=4700000){
    	tmp = (var - 4000000)/100000;
    	axp_read(charger->master, AXP22_CHARGE_VBUS,&val);
    	val &= 0xC7;
    	val |= tmp << 3;
    	axp_write(charger->master, AXP22_CHARGE_VBUS,val);
  	}
  }
  else
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x40);
    
	/*usb current limit*/
#if 0
  if((pmu_usbcur) && (pmu_usbcur_limit)){
    axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x02);
    var = pmu_usbcur * 1000;
  	if(var == 900000)
    	axp_clr_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
  	else if (var == 500000){
    	axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x01);
  	}
  }
  else
    axp_set_bits(charger->master, AXP22_CHARGE_VBUS, 0x03);
  #endif    

  // set lowe power warning/shutdown level
  axp_write(charger->master, AXP22_WARNING_LEVEL,((pmu_battery_warning_level1-5) << 4)+pmu_battery_warning_level2);

  ocv_cap[0]  = pmu_bat_para1;
  ocv_cap[1]  = 0xC1;
  ocv_cap[2]  = pmu_bat_para2;
  ocv_cap[3]  = 0xC2;
  ocv_cap[4]  = pmu_bat_para3;
  ocv_cap[5]  = 0xC3;
  ocv_cap[6]  = pmu_bat_para4;
  ocv_cap[7]  = 0xC4;
  ocv_cap[8]  = pmu_bat_para5;
  ocv_cap[9]  = 0xC5;
  ocv_cap[10] = pmu_bat_para6;
  ocv_cap[11] = 0xC6;
  ocv_cap[12] = pmu_bat_para7;
  ocv_cap[13] = 0xC7;
  ocv_cap[14] = pmu_bat_para8;
  ocv_cap[15] = 0xC8;
  ocv_cap[16] = pmu_bat_para9;
  ocv_cap[17] = 0xC9;
  ocv_cap[18] = pmu_bat_para10;
  ocv_cap[19] = 0xCA;
  ocv_cap[20] = pmu_bat_para11;
  ocv_cap[21] = 0xCB;
  ocv_cap[22] = pmu_bat_para12;
  ocv_cap[23] = 0xCC;
  ocv_cap[24] = pmu_bat_para13;
  ocv_cap[25] = 0xCD;
  ocv_cap[26] = pmu_bat_para14;
  ocv_cap[27] = 0xCE;
  ocv_cap[28] = pmu_bat_para15;
  ocv_cap[29] = 0xCF;
  ocv_cap[30] = pmu_bat_para16;
  ocv_cap[31] = 0xD0;
  ocv_cap[32] = pmu_bat_para17;
  ocv_cap[33] = 0xD1;
  ocv_cap[34] = pmu_bat_para18;
  ocv_cap[35] = 0xD2;
  ocv_cap[36] = pmu_bat_para19;
  ocv_cap[37] = 0xD3;
  ocv_cap[38] = pmu_bat_para20;
  ocv_cap[39] = 0xD4;
  ocv_cap[40] = pmu_bat_para21;
  ocv_cap[41] = 0xD5;
  ocv_cap[42] = pmu_bat_para22;
  ocv_cap[43] = 0xD6;
  ocv_cap[44] = pmu_bat_para23;
  ocv_cap[45] = 0xD7;
  ocv_cap[46] = pmu_bat_para24;
  ocv_cap[47] = 0xD8;
  ocv_cap[48] = pmu_bat_para25;
  ocv_cap[49] = 0xD9;
  ocv_cap[50] = pmu_bat_para26;
  ocv_cap[51] = 0xDA;
  ocv_cap[52] = pmu_bat_para27;
  ocv_cap[53] = 0xDB;
  ocv_cap[54] = pmu_bat_para28;
  ocv_cap[55] = 0xDC;
  ocv_cap[56] = pmu_bat_para29;
  ocv_cap[57] = 0xDD;
  ocv_cap[58] = pmu_bat_para30;
  ocv_cap[59] = 0xDE;
  ocv_cap[60] = pmu_bat_para31;
  ocv_cap[61] = 0xDF;
  ocv_cap[62] = pmu_bat_para32;
  axp_writes(charger->master, 0xC0,63,ocv_cap);
	/* pok open time set */
	axp_read(charger->master,AXP22_POK_SET,&val);
	if (pmu_pekon_time < 1000)
		val &= 0x3f;
	else if(pmu_pekon_time < 2000){
		val &= 0x3f;
		val |= 0x40;
	}
	else if(pmu_pekon_time < 3000){
		val &= 0x3f;
		val |= 0x80;
	}
	else {
		val &= 0x3f;
		val |= 0xc0;
	}
	axp_write(charger->master,AXP22_POK_SET,val);

	/* pok long time set*/
	if(pmu_peklong_time < 1000)
		pmu_peklong_time = 1000;
	if(pmu_peklong_time > 2500)
		pmu_peklong_time = 2500;
	axp_read(charger->master,AXP22_POK_SET,&val);
	val &= 0xcf;
	val |= (((pmu_peklong_time - 1000) / 500) << 4);
	axp_write(charger->master,AXP22_POK_SET,val);

	/* pek offlevel poweroff en set*/
	if(pmu_pekoff_en)
		{
			pmu_pekoff_en = 1;
		}
	else
		{
			pmu_pekoff_en = 0;			
		}
	axp_read(charger->master,AXP22_POK_SET,&val);
	val &= 0xf7;
	val |= (pmu_pekoff_en << 3);
	axp_write(charger->master,AXP22_POK_SET,val);
	
	/*Init offlevel restart or not */
	if(pmu_pekoff_func)
		{
			axp_set_bits(charger->master,AXP22_POK_SET,0x04); //restart
		}
	else
		{
			axp_clr_bits(charger->master,AXP22_POK_SET,0x04); //not restart
		}

	/* pek delay set */
	axp_read(charger->master,AXP22_OFF_CTL,&val);
	val &= 0xfc;
	val |= ((pmu_pwrok_time / 8) - 1);
	axp_write(charger->master,AXP22_OFF_CTL,val);

	/* pek offlevel time set */
	if(pmu_pekoff_time < 4000)
		pmu_pekoff_time = 4000;
	if(pmu_pekoff_time > 10000)
		pmu_pekoff_time =10000;
	pmu_pekoff_time = (pmu_pekoff_time - 4000) / 2000 ;
	axp_read(charger->master,AXP22_POK_SET,&val);
	val &= 0xfc;
	val |= pmu_pekoff_time ;
	axp_write(charger->master,AXP22_POK_SET,val);
	/*Init 16's Reset PMU en */
	if(pmu_reset)
		{
			axp_set_bits(charger->master,0x8F,0x08); //enable
		}
	else
		{
			axp_clr_bits(charger->master,0x8F,0x08); //disable
		}
		
		/*Init IRQ wakeup en*/
		if(pmu_IRQ_wakeup)
		{
			axp_set_bits(charger->master,0x8F,0x80); //enable
		}
		else
		{
			axp_clr_bits(charger->master,0x8F,0x80); //disable
		}
		
		/*Init N_VBUSEN status*/
		if(pmu_vbusen_func)
		{
			axp_set_bits(charger->master,0x8F,0x10); //output
		}
		else
		{
			axp_clr_bits(charger->master,0x8F,0x10); //input
		}
		
		/*Init InShort status*/
		if(pmu_inshort)
		{
			axp_set_bits(charger->master,0x8F,0x60); //InShort
		}
		else
		{
			axp_clr_bits(charger->master,0x8F,0x60); //auto detect
		}
		
		/*Init CHGLED function*/
		if(pmu_chgled_func)
		{
			axp_set_bits(charger->master,0x32,0x08); //control by charger
		}
		else
		{
			axp_clr_bits(charger->master,0x32,0x08); //drive MOTO
		}
		
		/*set CHGLED Indication Type*/
		if(pmu_chgled_type)
		{
			axp_set_bits(charger->master,0x34,0x10); //Type B
		}
		else
		{
			axp_clr_bits(charger->master,0x34,0x10); //Type A
		}
		
		/*Init PMU Over Temperature protection*/
		if(pmu_hot_shutdowm)
		{
			axp_set_bits(charger->master,0x8f,0x04); //enable
		}
		else
		{
			axp_clr_bits(charger->master,0x8f,0x04); //disable
		}

		/*Init battery capacity correct function*/
		if(pmu_batt_cap_correct)
		{
			axp_set_bits(charger->master,0xb8,0x20); //enable
		}
		else
		{
			axp_clr_bits(charger->master,0xb8,0x20); //disable
		}
		/* Init battery regulator enable or not when charge finish*/
		if(pmu_bat_regu_en)
		{
			axp_set_bits(charger->master,0x34,0x20); //enable
		}
		else
		{
			axp_clr_bits(charger->master,0x34,0x20); //disable
		}
 
  if(!pmu_batdeten)
  	axp_clr_bits(charger->master,AXP22_PDBC,0x40);
  else
  	axp_set_bits(charger->master,AXP22_PDBC,0x40);
  	

/* RDC initial */
	axp_read(charger->master, AXP22_RDC0,&val2);
	if((pmu_battery_rdc) && (!(val2 & 0x40)))		//如果配置电池内阻，则手动配置
	{
		rdc = (pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_write(charger->master, AXP22_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp_write(charger->master,AXP22_RDC1,rdc & 0x00FF);
	}

//probe 时初始化RDC，使其提前计算正确的OCV，然后在此处启动计量系统
	axp_read(charger->master,AXP22_BATCAP0,&val2);
	if((pmu_battery_cap) && (!(val2 & 0x80)))
	{
		Cur_CoulombCounter = pmu_battery_cap * 1000 / 1456;
		axp_write(charger->master, AXP22_BATCAP0, ((Cur_CoulombCounter >> 8) | 0x80));
		axp_write(charger->master,AXP22_BATCAP1,Cur_CoulombCounter & 0x00FF);		
	}
	else if(!pmu_battery_cap)
	{
		axp_write(charger->master, AXP22_BATCAP0, 0x00);
		axp_write(charger->master,AXP22_BATCAP1,0x00);
	}
  
  axp_charger_update_state((struct axp_charger *)charger);

  axp_read(charger->master, AXP22_CAP,&val2);
	charger->rest_vol = (int) (val2 & 0x7F);

  printk("now_rest_vol = %d\n",(val2 & 0x7F));
  charger->interval = msecs_to_jiffies(10 * 1000);
  INIT_DELAYED_WORK(&charger->work, axp_charging_monitor);
  schedule_delayed_work(&charger->work, charger->interval);

    /* set usb cur-vol limit*/
	INIT_DELAYED_WORK(&usbwork, axp_usb);
	schedule_delayed_work(&usbwork, msecs_to_jiffies(7 * 1000));

    /* register axp interrupt notifier */
    charger->nb.notifier_call = axp_battery_event;
    ret = axp_register_notifier(charger->master, &charger->nb, AXP22_NOTIFIER_ON);
    if (ret)
        goto err_notifier;

    /*给局部变量赋值*/
	axp_charger = charger;
#ifdef CONFIG_HAS_EARLYSUSPEND
	
    axp_early_suspend.suspend = axp_earlysuspend;
    axp_early_suspend.resume = axp_lateresume;
    axp_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
    register_early_suspend(&axp_early_suspend);
#endif
	/* 调试接口注册 */
	class_register(&axppower_class);

/*to aviod pa noise when power up,set aldo3 delay 1ms.added by zhwj 2013-03-29*/
	axp_write(charger->master,0xf4,0x06);
	axp_write(charger->master,0xf2,0x04);
	axp_write(charger->master,0xff,0x01);
	axp_write(charger->master,0x0a,0x01);
	axp_write(charger->master,0xff,0x00);
	axp_write(charger->master,0xf4,0x00);

	axp_write(charger->master,0x38,pmu_charge_ltf*10/128);
	axp_write(charger->master,0x39,pmu_charge_htf*10/128);
	axp_write(charger->master,0x3C,pmu_discharge_ltf*10/128);
	axp_write(charger->master,0x3D,pmu_discharge_htf*10/128);

    return ret;

err_notifier:
  cancel_delayed_work_sync(&charger->work);

err_ps_register:
  axp_unregister_notifier(charger->master, &charger->nb, AXP22_NOTIFIER_ON);

err_charger_init:
  kfree(charger);
  input_unregister_device(powerkeydev);
  kfree(powerkeydev);
  return ret;
}

static int axp_battery_remove(struct platform_device *dev)
{
    struct axp_charger *charger = platform_get_drvdata(dev);

    if(main_task){
        kthread_stop(main_task);
        main_task = NULL;
    }

    axp_unregister_notifier(charger->master, &charger->nb, AXP22_NOTIFIER_ON);
    cancel_delayed_work_sync(&charger->work);
    power_supply_unregister(&charger->usb);
    power_supply_unregister(&charger->ac);
    power_supply_unregister(&charger->batt);

    kfree(charger);
    input_unregister_device(powerkeydev);
    kfree(powerkeydev);

    return 0;
}


static int axp22_suspend(struct platform_device *dev, pm_message_t state)
{
    uint8_t irq_w[9];
    uint8_t tmp;

    struct axp_charger *charger = platform_get_drvdata(dev);


	cancel_delayed_work_sync(&charger->work);
	cancel_delayed_work_sync(&usbwork);

    /*clear all irqs events*/
    irq_w[0] = 0xff;
    irq_w[1] = AXP22_INTSTS2;
    irq_w[2] = 0xff;
    irq_w[3] = AXP22_INTSTS3;
    irq_w[4] = 0xff;
    irq_w[5] = AXP22_INTSTS4;
    irq_w[6] = 0xff;
    irq_w[7] = AXP22_INTSTS5;
    irq_w[8] = 0xff;
    axp_writes(charger->master, AXP22_INTSTS1, 9, irq_w);

    /* close all irqs*/
    axp_unregister_notifier(charger->master, &charger->nb, AXP22_NOTIFIER_ON);

#if defined (CONFIG_AXP_CHGCHANGE)
    if(pmu_suspend_chgcur == 0)
  		axp_clr_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);
    else if (1 == axp_charger->chgen) {
  		axp_set_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);

        if(axp_debug)
            printk("pmu_suspend_chgcur = %d\n", pmu_suspend_chgcur);

        if(pmu_suspend_chgcur >= 300000 && pmu_suspend_chgcur <= 2550000){
            tmp = (pmu_suspend_chgcur -200001)/150000;
            charger->chgcur = tmp *150000 + 300000;
            axp_update(charger->master, AXP22_CHARGE_CONTROL1, tmp,0x0F);
        }
    }
#endif

    return 0;
}

static int axp22_resume(struct platform_device *dev)
{
    struct axp_charger *charger = platform_get_drvdata(dev);

    int pre_rest_vol;
    uint8_t val,tmp;
		/*wakeup IQR notifier work sequence*/
    axp_register_notifier(charger->master, &charger->nb, AXP22_NOTIFIER_ON);

    axp_charger_update_state(charger);

		pre_rest_vol = charger->rest_vol;

		axp_read(charger->master, AXP22_CAP,&val);
		charger->rest_vol = val & 0x7f;

		if(charger->rest_vol - pre_rest_vol){
			printk("battery vol change: %d->%d \n", pre_rest_vol, charger->rest_vol);
			pre_rest_vol = charger->rest_vol;
			axp_write(charger->master,AXP22_DATA_BUFFER1,charger->rest_vol | 0x80);
			power_supply_changed(&charger->batt);
		}

#if defined (CONFIG_AXP_CHGCHANGE)
  	if(pmu_runtime_chgcur == 0)
  		axp_clr_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);
    else if(1 == axp_charger->chgen){
        axp_set_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);

        if(axp_debug)
            printk("pmu_runtime_chgcur = %d\n", pmu_runtime_chgcur);

        if(pmu_runtime_chgcur >= 300000 && pmu_runtime_chgcur <= 2550000){
            tmp = (pmu_runtime_chgcur -200001)/150000;
            charger->chgcur = tmp *150000 + 300000;
            axp_update(charger->master, AXP22_CHARGE_CONTROL1, tmp,0x0F);
        }else if(pmu_runtime_chgcur < 300000){
            axp_clr_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
        }
        else{
            axp_set_bits(axp_charger->master, AXP22_CHARGE_CONTROL1,0x0F);
        }
    }
#endif

	charger->disvbat = 0;
	charger->disibat = 0;
	axp_change(charger);
    schedule_delayed_work(&charger->work, charger->interval);
	schedule_delayed_work(&usbwork, msecs_to_jiffies(7 * 1000));

    return 0;
}

static void axp22_shutdown(struct platform_device *dev)
{
    uint8_t tmp;
    struct axp_charger *charger = platform_get_drvdata(dev);
    
    cancel_delayed_work_sync(&charger->work);

#if defined (CONFIG_AXP_CHGCHANGE)
  	if(pmu_shutdown_chgcur == 0)
  		axp_clr_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);
  	else
  		axp_set_bits(charger->master,AXP22_CHARGE_CONTROL1,0x80);

	if(axp_debug)
		printk("pmu_shutdown_chgcur = %d\n", pmu_shutdown_chgcur);

    if(pmu_shutdown_chgcur >= 300000 && pmu_shutdown_chgcur <= 2550000){
    	tmp = (pmu_shutdown_chgcur -200001)/150000;
    	charger->chgcur = tmp *150000 + 300000;
    	axp_update(charger->master, AXP22_CHARGE_CONTROL1, tmp, 0x0F);
    }
#endif
}

static struct platform_driver axp_battery_driver = {
  .driver = {
    .name = "axp22-supplyer",
    .owner  = THIS_MODULE,
  },
  .probe = axp_battery_probe,
  .remove = axp_battery_remove,
  .suspend = axp22_suspend,
  .resume = axp22_resume,
  .shutdown = axp22_shutdown,
};

static int axp_battery_init(void)
{
	int ret =0;
  ret = platform_driver_register(&axp_battery_driver);
  return ret;
}

static void axp_battery_exit(void)
{
  platform_driver_unregister(&axp_battery_driver);
}

subsys_initcall(axp_battery_init);
module_exit(axp_battery_exit);

MODULE_DESCRIPTION("AXP22 battery charger driver");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");
