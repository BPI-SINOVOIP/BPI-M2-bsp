.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
CROSS_COMPILE=$(COMPILE_TOOL)/arm-linux-gnueabi-
OUTPUT_DIR=$(CURDIR)/output
BUILD_PATH=$(CURDIR)/build
ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz
Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

U_O_PATH=$(BUILD_PATH)/$(BOARD)/$(UBOOT_CONFIG)-u-boot
K_O_PATH=$(BUILD_PATH)/$(BOARD)/$(KERNEL_CONFIG)-kernel
U_CONFIG_H=$(U_O_PATH)/include/config.h
K_DOT_CONFIG=$(K_O_PATH)/.config

all: bsp

## DK, if u-boot and kernel KBUILD_OUT issue fix, u-boot-clean and kernel-clean
## are no more needed
clean: u-boot-clean kernel-clean
	rm -rf $(BUILD_PATH)
	rm -f chosen_board.mk

## pack
pack: sunxi-pack
	$(Q)scripts/mk_pack.sh

# u-boot
$(U_CONFIG_H): u-boot-sunxi
	#$(Q)mkdir -p $(U_O_PATH)
	#$(Q)$(MAKE) -C u-boot-sunxi $(UBOOT_CONFIG)_config O=$(U_O_PATH) CROSS_COMPILE=$(CROSS_COMPILE) -j$J
	$(Q)$(MAKE) -C u-boot-sunxi $(UBOOT_CONFIG)_config CROSS_COMPILE=$(CROSS_COMPILE) -j$J

u-boot: $(U_CONFIG_H)
	#$(Q)$(MAKE) -C u-boot-sunxi all O=$(U_O_PATH) CROSS_COMPILE=$(CROSS_COMPILE) -j$J
	$(Q)$(MAKE) -C u-boot-sunxi all CROSS_COMPILE=$(CROSS_COMPILE) -j$J

u-boot-clean:
	$(Q)$(MAKE) -C u-boot-sunxi CROSS_COMPILE=$(CROSS_COMPILE) -j$J distclean

## linux
$(K_DOT_CONFIG): linux-sunxi
	#$(Q)mkdir -p $(K_O_PATH)
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm $(KERNEL_CONFIG)
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm $(KERNEL_CONFIG)

kernel: $(K_DOT_CONFIG)
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm oldconfig
	#DK, standby compiled using only single thread
	$(Q)$(MAKE) -C linux-sunxi/arch/arm/mach-sun6i/pm/standby all ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} KDIR=$(CURDIR)/linux-sunxi
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output uImage modules
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J headers_install
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output uImage modules
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J headers_install
	#cd $(K_O_PATH) && ${CROSS_COMPILE}objcopy -R .note.gnu.build-id -S -O binary vmlinux bImage
	cd linux-sunxi && ${CROSS_COMPILE}objcopy -R .note.gnu.build-id -S -O binary vmlinux bImage

kernel-clean:
	$(Q)$(MAKE) -C linux-sunxi/arch/arm/mach-sun6i/pm/standby ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} KDIR=$(CURDIR)/linux-sunxi clean
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J distclean

kernel-config: $(K_DOT_CONFIG)
	#$(Q)$(MAKE) -C linux-sunxi O=$(K_O_PATH) ARCH=arm menuconfig
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$J menuconfig
	cp linux-sunxi/.config linux-sunxi/arch/arm/configs/$(KERNEL_CONFIG)

## bsp
bsp: u-boot kernel

## linux
linux: 
	$(Q)scripts/mk_linux.sh $(ROOTFS)

help:
	@echo ""
	@echo "Usage:"
	@echo "  make bsp             - Default 'make'"
	@echo "  make linux         - Build target for linux platform, as ubuntu, need permisstion confirm during the build process"
	@echo "   Arguments:"
	@echo "    ROOTFS=            - Source rootfs (ie. rootfs.tar.gz with absolute path)"
	@echo ""
	@echo "  make pack            - pack the images and rootfs to a PhenixCard download image."
	@echo "  make clean"
	@echo ""
	@echo "Optional targets:"
	@echo "  make kernel           - Builds linux kernel"
	@echo "  make kernel-config    - Menuconfig"
	@echo "  make u-boot          - Builds u-boot"
	@echo ""

