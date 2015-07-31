#ifndef _LINUX_AXP_RW_H_
#define _LINUX_AXP_RW_H_

#include <linux/mfd/axp-mfd.h>
#include <mach/ar100.h>

static uint8_t axp_reg_addr = 0;

struct i2c_client *axp;
EXPORT_SYMBOL_GPL(axp);

static inline int __axp_read(struct i2c_client *client,
				int reg, uint8_t *val)
{
#ifdef	CONFIG_AXP_TWI_USED
	int ret;


	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	return 0;
#else
    int ret;
	uint8_t addr = (uint8_t)reg;
	uint8_t data;
	ret = ar100_axp_read_reg(&addr , &data, 1);
	if (ret != 0) {
		printk("failed reading at 0x%02x\n", reg);
		return ret;
	}
	*val = data;
	return 0;
#endif
}

static inline int __axp_reads(struct i2c_client *client, int reg,
				 int len, uint8_t *val)
{
#ifdef	CONFIG_AXP_TWI_USED
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}
	return 0;
#else
	int     ret, i, rd_len;
	uint8_t addr[AXP_TRANS_BYTE_MAX];
	uint8_t data[AXP_TRANS_BYTE_MAX];
	uint8_t *cur_data = val;
	
	/* fetch first register address */
	while (len > 0) {
		rd_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < rd_len; i++) {
			addr[i] = reg++;
		}
		/* read axp registers */
		ret = ar100_axp_read_reg(addr, data, rd_len);
		if (ret != 0) {
			printk("failed read to 0x%02x\n", reg);
			return ret;
		}
		/* copy data to user buffer */
		memcpy(cur_data, data, rd_len);
		cur_data = cur_data + rd_len;
		
		/* process next time read */
		len -= rd_len;
	}
	return 0;
#endif
}

static inline int __axp_write(struct i2c_client *client,
				 int reg, uint8_t val)
{
#ifdef	CONFIG_AXP_TWI_USED
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}
	return 0;
#else
	int ret;
	uint8_t addr = (uint8_t)reg;
	
	ret = ar100_axp_write_reg(&addr, &val, 1);
	if (ret != 0) {
		printk("failed writing 0x%02x to 0x%02x\n", val, reg);
		return ret;
	}
	return 0;
#endif
}


static inline int __axp_writes(struct i2c_client *client, int reg,
				  int len, uint8_t *val)
{
#ifdef	CONFIG_AXP_TWI_USED
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}
	return 0;
#else
	int     ret, i, first_flag, wr_len;
	uint8_t addr[AXP_TRANS_BYTE_MAX];
	uint8_t data[AXP_TRANS_BYTE_MAX];
	
	if ((reg == 0x48) && (len == 9)) {
	}
	
	/* fetch first register address */
	first_flag = 1;
	addr[0] = (uint8_t)reg;
	len = len + 1;	//+ first reg addr
	len = len >> 1;	//len = len / 2
	while (len > 0) {
		wr_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < wr_len; i++) {
			if (first_flag) {
				/* skip the first reg addr */
				data[i] = *val++;
				first_flag = 0;
			} else {
				addr[i] = *val++;
				data[i] = *val++;
			}
		}
		/* write axp registers */
		ret = ar100_axp_write_reg(addr, data, wr_len);
		if (ret != 0) {
			printk("failed writings to 0x%02x\n", reg);
			return ret;
		}
		/* process next time write */
		len -= wr_len;
	}
	return 0;
#endif
}

int axp_register_notifier(struct device *dev, struct notifier_block *nb,
				uint64_t irqs)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);

	chip->ops->enable_irqs(chip, irqs);
	if(NULL != nb) {
	    return blocking_notifier_chain_register(&chip->notifier_list, nb);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(axp_register_notifier);

int axp_unregister_notifier(struct device *dev, struct notifier_block *nb,
				uint64_t irqs)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);

	chip->ops->disable_irqs(chip, irqs);
	if(NULL != nb) {
	    return blocking_notifier_chain_unregister(&chip->notifier_list, nb);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_unregister_notifier);

int axp_write(struct device *dev, int reg, uint8_t val)
{
	return __axp_write(to_i2c_client(dev), reg, val);
}
EXPORT_SYMBOL_GPL(axp_write);

int axp_writes(struct device *dev, int reg, int len, uint8_t *val)
{
	return  __axp_writes(to_i2c_client(dev), reg, len, val);
}
EXPORT_SYMBOL_GPL(axp_writes);

int axp_read(struct device *dev, int reg, uint8_t *val)
{
	return __axp_read(to_i2c_client(dev), reg, val);
}
EXPORT_SYMBOL_GPL(axp_read);

int axp_reads(struct device *dev, int reg, int len, uint8_t *val)
{
	return __axp_reads(to_i2c_client(dev), reg, len, val);
}
EXPORT_SYMBOL_GPL(axp_reads);

int axp_set_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_mfd_chip *chip;

	chip = dev_get_drvdata(dev);
	mutex_lock(&chip->lock);
	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_set_bits);

int axp_clr_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_mfd_chip *chip;
	
	chip = dev_get_drvdata(dev);

	mutex_lock(&chip->lock);

	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_clr_bits);

int axp_update(struct device *dev, int reg, uint8_t val, uint8_t mask)
{
	struct axp_mfd_chip *chip = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&chip->lock);

	ret = __axp_read(chip->client, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = __axp_write(chip->client, reg, reg_val);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_update);

struct device *axp_get_dev(void)
{
	return &axp->dev;
}
EXPORT_SYMBOL_GPL(axp_get_dev);

#endif
