#
# Makefile for the linux kernel.
#

obj-y = core.o printk.o devices.o sunxi_dump_reg.o  chips.o
ifeq ($(CONFIG_ARM_ARCH_TIMER),)
obj-y += timer.o
endif
obj-y += clock/
obj-y += dma/
obj-y += gpio/
obj-y += ar100/
obj-y += sys_config.o
obj-$(CONFIG_CPU_FREQ) += cpu-freq/
obj-$(CONFIG_DEVFREQ_DRAM_FREQ) += dram-freq/
obj-$(CONFIG_PM) += pm/
obj-$(CONFIG_NET) += rf/
obj-$(CONFIG_SUNXI_SYSFS) += sysfs/
ifeq ($(CONFIG_SMP),y)
obj-y += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o

endif


