
PWD=$(shell pwd)

obj-$(CONFIG_SUNXI_MEM_TEST) += sunxi_mem_test.o

install: build
	cp sunxi_mem_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)
