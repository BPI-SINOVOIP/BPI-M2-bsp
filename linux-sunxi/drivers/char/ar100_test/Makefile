#
# Makefile for sun6i ar100 test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_AR100_TEST_DRIVER)+= ar100_test.o

install: build
	cp ar100_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

#clean:
#	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
