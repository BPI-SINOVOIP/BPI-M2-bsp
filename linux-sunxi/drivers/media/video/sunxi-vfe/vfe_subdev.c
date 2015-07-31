#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>

#include "vfe.h"
#include "vfe_os.h"
#include "vfe_subdev.h"

/*
 * called by subdev in power on/off sequency
 * must be called after update_ccm_info
 */

//enable/disable pmic channel 
int vfe_set_pmu_channel(struct v4l2_subdev *sd, enum pmic_channel pmic_ch, enum on_off on_off)
{
  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  struct regulator *pmic;
  int ret;
  
  switch(pmic_ch) {
    case IOVDD:
      pmic = dev->power->iovdd;
		  if(pmic) {
		  	ret = regulator_set_voltage(pmic,dev->power->iovdd_vol,3300000);
				vfe_dbg(0,"set regulator iovdd = %d,return %x\n",dev->power->iovdd_vol,ret);
		  }
      break;
    case DVDD:
      pmic = dev->power->dvdd;
		  if(pmic) {
		  	ret = regulator_set_voltage(pmic,dev->power->dvdd_vol,1800000);
				vfe_dbg(0,"set regulator dvdd = %d,return %x\n",dev->power->dvdd_vol,ret);
		  }
      break;
    case AVDD:
      pmic = dev->power->avdd;
		  if(pmic) {
	      ret = regulator_set_voltage(pmic,dev->power->avdd_vol,3300000);
				vfe_dbg(0,"set regulator avdd = %d,return %x\n",dev->power->avdd_vol,ret);
		  }
      break;
    case AFVDD:
      pmic = dev->power->afvdd;
		  if(pmic) {
		  	ret = regulator_set_voltage(pmic,dev->power->afvdd_vol,3300000);
				vfe_dbg(0,"set regulator afvdd = %d,return %x\n",dev->power->afvdd_vol,ret);
		  }
      break;
    default:
      pmic = NULL;
  }
  
  if(on_off == OFF) {
    if(pmic) {
      if(!regulator_is_enabled(pmic)) {
      vfe_dbg(0,"regulator_is already disabled\n");
      	return 0;
      } else {
	      ret = regulator_disable(pmic);
	      vfe_dbg(0,"regulator_disable\n");
	      while(regulator_is_enabled(pmic)) {
	      	vfe_dbg(0,"regulator is checked enalbed\n");
	      }
	      return ret;
	    }
    }
  } else {
    if(pmic) {
      if(regulator_is_enabled(pmic)) {
      vfe_dbg(0,"regulator_is already enabled\n");
      	return 0;
      } else {
	      ret = regulator_enable(pmic);
	      vfe_dbg(0,"regulator_enable\n");
	      while(!regulator_is_enabled(pmic)) {
	      	vfe_dbg(0,"regulator is checked disalbed\n");
	      }
	      return ret;
	    }
    }
  }
  
  return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_pmu_channel);

//enable/disable master clock
int vfe_set_mclk(struct v4l2_subdev *sd, enum on_off on_off)
{
  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  switch(on_off) {
    case ON:
	  vfe_print("mclk on\n");
	  if(dev->clock.vfe_master_clk) {
        if(os_clk_enable(dev->clock.vfe_master_clk)) {
          vfe_err("vip%d master clock enable error\n",dev->vip_sel);
          return -1;
        }
      } else {
        vfe_err("vip%d master clock is null\n",dev->vip_sel);
        return -1;
      }
      break;
    case OFF:
		vfe_print("mclk off\n");
      if(dev->clock.vfe_master_clk) {
        os_clk_disable(dev->clock.vfe_master_clk);
      } else {
        vfe_err("vip%d master clock is null\n",dev->vip_sel);
        return -1;
      }
      break;
    default:
      return -1;
  }
  return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_mclk);

//set frequency of master clock
int vfe_set_mclk_freq(struct v4l2_subdev *sd, unsigned long freq)
{
	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct clk *master_clk_src;
  
  if(freq==24000000 || freq==12000000 || freq==6000000) {
    if(dev->clock.vfe_master_clk_24M_src) {
      master_clk_src = dev->clock.vfe_master_clk_24M_src;
    } else {
      vfe_err("vfe master clock 24M source is null\n");
      return -1;
    }
  } else {
    if(dev->clock.vfe_master_clk_pll_src) {
      master_clk_src = dev->clock.vfe_master_clk_pll_src;
    } else {
      vfe_err("vfe master clock pll source is null\n");
      return -1;
    }
  }
  
  if(dev->clock.vfe_master_clk) {
    if(os_clk_set_parent(dev->clock.vfe_master_clk, master_clk_src)) {
      vfe_err("set vfe master clock source failed \n");
      return -1;
    }
  } else {
    vfe_err("vfe master clock is null\n");
    return -1;
  }
  
  if(dev->clock.vfe_master_clk) {
    if(os_clk_set_rate(dev->clock.vfe_master_clk, freq)) {
      vfe_err("set vip%d master clock error\n",dev->vip_sel);
      return -1;
    }
  } else {
    vfe_err("vfe master clock is null\n");
    return -1;
  }
  
  return 0;
}
EXPORT_SYMBOL_GPL(vfe_set_mclk_freq);

//set the gpio io status
int vfe_gpio_write(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status)
{
  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  struct gpio_config gpio;
  switch(gpio_type) {
    case POWER_EN:
      gpio = dev->gpio->power_en_io;
      break;
    case PWDN:
      gpio = dev->gpio->pwdn_io;
      break;
    case RESET:
      gpio = dev->gpio->reset_io;
      break;
    case AF_PWDN:
      gpio = dev->gpio->af_pwdn_io;
      break;
    case FLASH_EN:
      gpio = dev->gpio->flash_en_io;
      break;
    case FLASH_MODE:
      gpio = dev->gpio->flash_mode_io;
      break;
    default:
      break;
  }
  return os_gpio_write(&gpio, status);
}
EXPORT_SYMBOL_GPL(vfe_gpio_write);

//set the gpio io status
int vfe_gpio_set_status(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status)
{
  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  struct gpio_config gpio;
  switch(gpio_type) {
    case POWER_EN:
      gpio = dev->gpio->power_en_io;
      break;
    case PWDN:
      gpio = dev->gpio->pwdn_io;
      break;
    case RESET:
      gpio = dev->gpio->reset_io;
      break;
    case AF_PWDN:
      gpio = dev->gpio->af_pwdn_io;
      break;
    case FLASH_EN:
      gpio = dev->gpio->flash_en_io;
      break;
    case FLASH_MODE:
      gpio = dev->gpio->flash_mode_io;
      break;
    default:
      break;
  }
  return os_gpio_set_status(&gpio, status);
}
EXPORT_SYMBOL_GPL(vfe_gpio_set_status);

//get standby mode
void vfe_get_standby_mode(struct v4l2_subdev *sd, enum standby_mode *stby_mode)
{
  struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
  *stby_mode = dev->power->stby_mode;
}
EXPORT_SYMBOL_GPL(vfe_get_standby_mode);

int io_set_flash_ctrl(struct v4l2_subdev *sd, enum sunxi_flash_ctrl ctrl,
                      struct flash_dev_info *fls_info)
{
  int ret=0;
  unsigned int flash_en, flash_dis, flash_mode, torch_mode;
  //fl_prn("io_set_flash_ctrl!\n");
  
  if(fls_info==NULL)
  {
    fl_err("error flash config!\n");
    return -1;
  }
  
  flash_en=(fls_info->en_pol!=0)?1:0;       
  flash_dis=!flash_en;                      
  flash_mode=(fls_info->fl_mode_pol!=0)?1:0;
  torch_mode=!flash_mode;                   
  
//  fl_dbg("flash_en=%d\n",flash_en);
//  fl_dbg("flash_mode=%d\n",flash_mode);
  
  switch(ctrl) {
    case SW_CTRL_FLASH_OFF:
      fl_dbg("SW_CTRL_FLASH_OFF\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_dis);
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
//      vfe_gpio_set_status(sd,FLASH_EN,0);//set the gpio to hi-z
//      vfe_gpio_set_status(sd,FLASH_MODE,0);//set the gpio to hi-z
      break;
	  case SW_CTRL_FLASH_ON:
      fl_dbg("SW_CTRL_FLASH_ON\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_MODE, flash_mode);
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_en);
      break;
	  case SW_CTRL_TORCH_ON:
      fl_dbg("SW_CTRL_TORCH_ON\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_en);
      break;
	  default:
	    return -EINVAL;
	}
	if(ret!=0)
	{
	  fl_dbg("flash set ctrl fail, force shut off\n");
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_dis);
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
	}
  return ret;
}
EXPORT_SYMBOL_GPL(io_set_flash_ctrl);


int config_flash_mode(struct v4l2_subdev *sd, enum v4l2_flash_led_mode mode,
                      struct flash_dev_info *fls_info)
{
  if(fls_info==NULL)
  {
    fl_err("camera flash not support!\n");
    return -1;
  }
  if((fls_info->light_src!=0x01)&&(fls_info->light_src!=0x02)&&
     (fls_info->light_src!=0x10))
  {
    fl_err("unsupported light source, force LEDx1\n");
    fls_info->light_src=0x01;
  }
  
  switch (mode) {
    case V4L2_FLASH_LED_MODE_NONE:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_NONE;
      break;
    case V4L2_FLASH_LED_MODE_FLASH:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_ON;
      break;
    case V4L2_FLASH_LED_MODE_TORCH:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_TORCH_ON;
      break;
    case V4L2_FLASH_LED_MODE_AUTO:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_AUTO;
      break;
    case V4L2_FLASH_LED_MODE_RED_EYE:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_RED_EYE;
      break;
    default:
      return -EINVAL;
  }
  return 0;
}
EXPORT_SYMBOL_GPL(config_flash_mode);
MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end subdev for sunxi");
