
################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################
PWD=$(shell pwd)
obj-m+=nandtest.o 
nandtest-objs := nandtest/nand_test.o ../../nand/nfd/nand_osal_for_linux.o  libnand.a

install: build
	cp nandtest.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	cp ../../nand/libnand libnand.a
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)


clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~