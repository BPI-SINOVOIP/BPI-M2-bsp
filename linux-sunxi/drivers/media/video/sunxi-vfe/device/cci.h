#ifndef __VFE__I2C__H__
#define __VFE__I2C__H__

#include <linux/i2c.h>

struct reg_list_a8_d8 {
  unsigned char addr;
  unsigned char data;
};

struct reg_list_a8_d16 {
  unsigned char addr;
  unsigned short data;
};

struct reg_list_a16_d8 {
  unsigned short addr;
  unsigned char data;
};

struct reg_list_a16_d16 {
  unsigned short addr;
  unsigned short data;
};

struct reg_list_w_a16_d16 {
  unsigned short width;
  unsigned short addr;
  unsigned short data;
};

extern int cci_read_a8_d8(struct i2c_client *client, unsigned char addr,unsigned char *value);
extern int cci_write_a8_d8(struct i2c_client *client, unsigned char addr,unsigned char value);
extern int cci_read_a8_d16(struct i2c_client *client, unsigned char addr,unsigned short *value);
extern int cci_write_a8_d16(struct i2c_client *client, unsigned char addr,unsigned short value);
extern int cci_read_a16_d8(struct i2c_client *client, unsigned short addr,unsigned char *value);
extern int cci_write_a16_d8(struct i2c_client *client, unsigned short addr,unsigned char value);
extern int cci_read_a16_d16(struct i2c_client *client, unsigned short addr,unsigned short *value);
extern int cci_write_a16_d16(struct i2c_client *client, unsigned short addr,unsigned short value);

#endif //__VFE__I2C__H__