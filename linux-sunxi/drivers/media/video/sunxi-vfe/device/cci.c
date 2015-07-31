#include <linux/module.h>
#include "cci.h"

#define cci_print(x,arg...) printk(KERN_INFO"[VFE_DEV_I2C]"x,##arg)
#define cci_err(x,arg...) printk(KERN_ERR"[VFE_DEV_I2C_ERR]"x,##arg)

/*
 * On most platforms, we'd rather do straight i2c I/O.
 */

int cci_read_a8_d8(struct i2c_client *client, unsigned char addr,
    unsigned char *value)
{
  unsigned char data[2];
  struct i2c_msg msg[2];
  int ret;
  
  data[0] = addr;
  data[1] = 0xee;
  /*
   * Send out the register address...
   */
  msg[0].addr = client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &data[0];
  /*
   * ...then read back the result.
   */
  msg[1].addr = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 1;
  msg[1].buf = &data[1];
  
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret >= 0) {
    *value = data[1];
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%2x, value = 0x%2x\n ",__func__, client->addr, addr,*value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_read_a8_d8);

int cci_write_a8_d8(struct i2c_client *client, unsigned char addr,
    unsigned char value)
{
  struct i2c_msg msg;
  unsigned char data[2];
  int ret;
  
  data[0] = addr;
  data[1] = value;
  
  msg.addr = client->addr;
  msg.flags = 0;
  msg.len = 2;
  msg.buf = data;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0) {
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%2x, value = 0x%2x\n ",__func__, client->addr, addr,value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_write_a8_d8);

int cci_read_a8_d16(struct i2c_client *client, unsigned char addr,
    unsigned short *value)
{
  unsigned char data[3];
  struct i2c_msg msg[2];
  int ret;
  
  data[0] = addr;
  data[1] = 0xee;
  data[2] = 0xee;
  /*
   * Send out the register address...
   */
  msg[0].addr = client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &data[0];
  /*
   * ...then read back the result.
   */
  msg[1].addr = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 2;
  msg[1].buf = &data[1];
  
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret >= 0) {
    *value = data[1]*256 + data[2];
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%2x, value = 0x%4x\n ",__func__, client->addr, addr,*value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_read_a8_d16);

int cci_write_a8_d16(struct i2c_client *client, unsigned char addr,
    unsigned short value)
{
  struct i2c_msg msg;
  unsigned char data[3];
  int ret;
  
  data[0] = addr;
  data[1] = (value&0xff00)>>8;
  data[2] = (value&0x00ff);
  
  msg.addr = client->addr;
  msg.flags = 0;
  msg.len = 3;
  msg.buf = data;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0) {
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%2x, value = 0x%4x\n ",__func__, client->addr, addr,value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_write_a8_d16);

int cci_read_a16_d8(struct i2c_client *client, unsigned short addr,
    unsigned char *value)
{
  unsigned char data[3];
  struct i2c_msg msg[2];
  int ret;
  
  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = 0xee;
  /*
   * Send out the register address...
   */ 
  msg[0].addr = client->addr;
  msg[0].flags = 0;
  msg[0].len = 2;
  msg[0].buf = &data[0];
  /*
   * ...then read back the result.
   */
  msg[1].addr = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 1;
  msg[1].buf = &data[2];
  
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret >= 0) {
    *value = data[2];
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%2x\n ",__func__, client->addr, addr,*value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_read_a16_d8);

int cci_write_a16_d8(struct i2c_client *client, unsigned short addr,
    unsigned char value)
{
  struct i2c_msg msg;
  unsigned char data[3];
  int ret;
  
  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = value;
  
  msg.addr = client->addr;
  msg.flags = 0;
  msg.len = 3;
  msg.buf = data;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0) {
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%4x\n ",__func__, client->addr, addr,value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_write_a16_d8);

int cci_read_a16_d16(struct i2c_client *client, unsigned short addr,
    unsigned short *value)
{
  unsigned char data[4];
  struct i2c_msg msg[2];
  int ret;
  
  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = 0xee;
  data[3] = 0xee;
  /*
   * Send out the register address...
   */
  msg[0].addr = client->addr;
  msg[0].flags = 0;
  msg[0].len = 2;
  msg[0].buf = &data[0];
  /*
   * ...then read back the result.
   */
  msg[1].addr = client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 2;
  msg[1].buf = &data[2];
  
  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret >= 0) {
    *value = data[2]*256 + data[3];
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%4x\n ",__func__, client->addr, addr,*value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_read_a16_d16);

int cci_write_a16_d16(struct i2c_client *client, unsigned short addr,
    unsigned short value)
{
  struct i2c_msg msg;
  unsigned char data[4];
  int ret;
  
  data[0] = (addr&0xff00)>>8;
  data[1] = (addr&0x00ff);
  data[2] = (value&0xff00)>>8;
  data[3] = (value&0x00ff);
  
  msg.addr = client->addr;
  msg.flags = 0;
  msg.len = 4;
  msg.buf = data;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret >= 0) {
    ret = 0;
  } else {
    cci_err("%s error! slave = 0x%x, addr = 0x%4x, value = 0x%4x\n ",__func__, client->addr, addr,value);
  }
  return ret;
}
EXPORT_SYMBOL_GPL(cci_write_a16_d16);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Camera Comunication Interface Abstract Layer for sunxi");