/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To match the U-Boot user interface on ARM platforms to the U-Boot
 * standard (as on PPC platforms), some messages with debug character
 * are removed from the default U-Boot build.
 *
 * Define DEBUG here if you want additional info as shown below
 * printed upon startup:
 *
 * U-Boot code: 00F00000 -> 00F3C774  BSS: -> 00FC3274
 * IRQ Stack: 00ebff7c
 * FIQ Stack: 00ebef7c
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>

//Justin Porting for BPI-M2 20150727 Start
#include <asm/arch/sc.h>
#include <sys_config.h>
//Justin Porting for BPI-M2 20150727 Start


#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif

#ifdef CONFIG_DRIVER_SMC91111
#include "../drivers/net/smc91111.h"
#endif
#ifdef CONFIG_DRIVER_LAN91C96
#include "../drivers/net/lan91c96.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

#ifdef CONFIG_HAS_DATAFLASH
extern int  AT91F_DataflashInit(void);
extern void dataflash_print_info(void);
#endif

//Justin Porting for BPI-M2 20150727 Start
extern int axp22_set_supply_status_byname(char *vol_name, int vol_value, int onoff);
//Justin Porting for BPI-M2 20150727 Start

#ifdef CONFIG_DRIVER_RTL8019
extern void rtl8019_get_enetaddr (uchar * addr);
#endif

#include <i2c.h>
#if defined(CONFIG_SUNXI_I2C)
	#include <i2c.h>
#elif defined(CONFIG_SUNXI_P2WI)
	#include <p2wi.h>
#elif defined(CONFIG_SUNXI_RSB)
	#include <rsb.h>
#else

#endif

#if defined(CONFIG_ALLWINNER)
	#include <boot_type.h>
	#include <sys_config.h>
	#include <sys_partition.h>
	#include <sunxi_board.h>
	#include <asm/arch/dma.h>
	#include <pmu.h>
	#include <../fs/aw_fs/ff.h>
#endif

/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
inline void __coloured_LED_init(void) {}
void coloured_LED_init(void)
	__attribute__((weak, alias("__coloured_LED_init")));
inline void __red_LED_on(void) {}
void red_LED_on(void) __attribute__((weak, alias("__red_LED_on")));
inline void __red_LED_off(void) {}
void red_LED_off(void) __attribute__((weak, alias("__red_LED_off")));
inline void __green_LED_on(void) {}
void green_LED_on(void) __attribute__((weak, alias("__green_LED_on")));
inline void __green_LED_off(void) {}
void green_LED_off(void) __attribute__((weak, alias("__green_LED_off")));
inline void __yellow_LED_on(void) {}
void yellow_LED_on(void) __attribute__((weak, alias("__yellow_LED_on")));
inline void __yellow_LED_off(void) {}
void yellow_LED_off(void) __attribute__((weak, alias("__yellow_LED_off")));
inline void __blue_LED_on(void) {}
void blue_LED_on(void) __attribute__((weak, alias("__blue_LED_on")));
inline void __blue_LED_off(void) {}
void blue_LED_off(void) __attribute__((weak, alias("__blue_LED_off")));

/*
 ************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * or dropped completely,
 * but let's get it working (again) first...
 */

#if defined(CONFIG_ARM_DCC) && !defined(CONFIG_BAUDRATE)
#define CONFIG_BAUDRATE 115200
#endif
static int init_baudrate(void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_f("baudrate", tmp, sizeof(tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul(tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

static int display_banner(void)
{
	tick_printf("\n\n%s\n\n", version_string);
	debug("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _TEXT_BASE,
	       _bss_start_ofs + _TEXT_BASE, _bss_end_ofs + _TEXT_BASE);
#ifdef CONFIG_MODEM_SUPPORT
	debug("Modem Support enabled\n");
#endif
#ifdef CONFIG_USE_IRQ
	debug("IRQ Stack: %08lx\n", IRQ_STACK_START);
	debug("FIQ Stack: %08lx\n", FIQ_STACK_START);
#endif

	return (0);
}

/*
 * WARNING: this code looks "cleaner" than the PowerPC version, but
 * has the disadvantage that you either get nothing, or everything.
 * On PowerPC, you might see "DRAM: " before the system hangs - which
 * gives a simple yet clear indication which part of the
 * initialization if failing.
 */
static int display_dram_config(void)
{
	int i;

//#ifdef DEBUG
#if 0
	puts("RAM Configuration:\n");

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		printf("Bank #%d: %08lx ", i, gd->bd->bi_dram[i].start);
		print_size(gd->bd->bi_dram[i].size, "\n");
	}
#else
	ulong size = 0;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		size += gd->bd->bi_dram[i].size;

	tick_printf("DRAM:  ");
	//puts("DRAM:  ");
	print_size(size, "\n");
#endif

	return (0);
}

static int init_func_pmubus(void)
{
	tick_printf("pmbus:   ");

#if defined(CONFIG_SUNXI_I2C)
	i2c_init(CONFIG_SYS_I2C_SPEED,CONFIG_SYS_I2C_SLAVE);
#elif defined(CONFIG_SUNXI_P2WI)
	p2wi_init();
#else
	sunxi_rsb_init(0);
#endif
	puts("ready\n");
	return (0);
}

#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
#include <pci.h>
static int arm_pci_init(void)
{
	pci_init();
	return 0;
}
#endif /* CONFIG_CMD_PCI || CONFIG_PCI */

#if defined(CONFIG_SUNXI_AXP)
extern int power_init(void);
#endif
extern int check_update_key(void);
extern int display_inner(void);
extern int script_init(void);
extern int check_uart_input(void);
extern int power_source_init(void);
/*
 * Breathe some life into the board...
 *
 * Initialize a serial port as console, and carry out some hardware
 * tests.
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

int print_cpuinfo(void);

void __dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size =  gd->ram_size;
}
void dram_init_banksize(void)
	__attribute__((weak, alias("__dram_init_banksize")));

init_fnc_t *init_sequence[] = {
//#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init,		/* basic arch cpu dependent setup */
//#endif
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
#endif
	timer_init,		/* initialize timer */
#ifdef CONFIG_FSL_ESDHC
	get_clocks,
#endif
	env_init,			/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
	display_inner,      /* show the inner version */
	script_init,
#if defined(CONFIG_DISPLAY_CPUINFO)
	print_cpuinfo,		/* display cpu info (and speed) */
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
	checkboard,		/* display board info */
#endif
	init_func_pmubus,
	power_source_init,
    check_update_key,
    check_uart_input,
	dram_init,		/* configure available RAM banks */
	NULL,
};

void board_init_f(ulong bootflag)
{
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	gd_t *id;
	ulong addr, addr_sp;

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) ((CONFIG_SYS_INIT_SP_ADDR) & ~0x07);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset((void *)gd, 0, sizeof(gd_t));
	gd->mon_len = _bss_end_ofs + sizeof(struct spare_boot_head_t);

	//while((*(volatile unsigned int *)(0)) != 1);
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			//hang ();
			sunxi_board_run_fel_eraly();	/* modify by jerry */
		}
	}
	debug("monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("ramsize: %08lX\n", gd->ram_size);
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This should work
	 * for arch/ppc and arch/powerpc. Only Linux board ports in
	 * arch/powerpc with bootwrapper support, that recalculate the
	 * memory size from the SDRAM controller setup will have to
	 * get fixed.
	 */
	gd->ram_size -= CONFIG_SYS_MEM_TOP_HIDE;
#endif

	addr = CONFIG_SYS_SDRAM_BASE + gd->ram_size;

#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
	/* reserve kernel log buffer */
	addr -= (LOGBUFF_RESERVE);
	debug("Reserving %dk for kernel logbuffer at %08lx\n", LOGBUFF_LEN,
		addr);
#endif
#endif

#ifdef CONFIG_SUNXI_LOGBUFFER
	addr -= (SUNXI_DISPLAY_FRAME_BUFFER_SIZE);
#endif

#ifdef CONFIG_PRAM
	/*
	 * reserve protected RAM
	 */
	i = getenv_r("pram", (char *)tmp, sizeof(tmp));
	reg = (i > 0) ? simple_strtoul((const char *)tmp, NULL, 10) :
		CONFIG_PRAM;
	addr -= (reg << 10);		/* size is in kB */
	debug("Reserving %ldk for protected RAM at %08lx\n", reg, addr);
#endif /* CONFIG_PRAM */

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* reserve TLB table */
	addr -= (4096 * 4);

	/* round down to next 64 kB limit */
	addr &= ~(0x10000 - 1);

	gd->tlb_addr = addr;
	debug("TLB table at: %08lx\n", addr);
#endif

	/* round down to next 4 kB limit */
	addr &= ~(4096 - 1);
	debug("Top of RAM usable for U-Boot at: %08lx\n", addr);

#ifdef CONFIG_LCD
#ifdef CONFIG_FB_ADDR
	gd->fb_base = CONFIG_FB_ADDR;
#else
	/* reserve memory for LCD display (always full pages) */
	addr = lcd_setmem(addr);
	gd->fb_base = addr;
#endif /* CONFIG_FB_ADDR */
#endif /* CONFIG_LCD */

	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= gd->mon_len;
	addr &= ~(4096 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10, addr);

#ifndef CONFIG_SPL_BUILD
	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);
#ifdef CONFIG_NONCACHE_MEMORY
	addr_sp &= (~(0x00100000 -1));
	addr_sp -= CONFIG_NONCACHE_MEMORY_SIZE;
#endif
	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);

#ifdef CONFIG_MACH_TYPE
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE; /* board id for Linux */
#endif

	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/* setup stackpointer for exeptions */
	gd->irq_sp = addr_sp;
#ifdef CONFIG_USE_IRQ
	addr_sp -= (CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ);
	debug("Reserving %zu Bytes for IRQ stack at: %08lx\n",
		CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ, addr_sp);
#endif
	/* leave 3 words for abort-stack    */
	addr_sp -= 12;

	/* 8-byte alignment for ABI compliance */
	addr_sp &= ~0x07;
#else
	addr_sp += 128;	/* leave 32 words for abort-stack   */
	gd->irq_sp = addr_sp;
#endif

	debug("New Stack Pointer is: %08lx\n", addr_sp);

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run(NULL, POST_ROM | post_bootmode_get(0));
#endif

	gd->bd->bi_baudrate = gd->baudrate;
	/* Ram ist board specific, so move it to board code ... */
	dram_init_banksize();
	display_dram_config();	/* and display it */

	gd->relocaddr = addr + sizeof(struct spare_boot_head_t);
	gd->start_addr_sp = addr_sp;
	gd->reloc_off = addr - _TEXT_BASE;
	printf("relocation Offset is: %08lx\n", gd->reloc_off);
	memcpy((void *)addr, (void *)_TEXT_BASE, sizeof(struct spare_boot_head_t));
	debug("from %x to %x, size %x\n", (void *)_TEXT_BASE, (void *)addr, sizeof(struct spare_boot_head_t));
	memcpy(id, (void *)gd, sizeof(gd_t));

	relocate_code(addr_sp, id, addr + sizeof(struct spare_boot_head_t));

	/* NOTREACHED - relocate_code() does not return */
}

#if !defined(CONFIG_SYS_NO_FLASH)
static char *failed = "*** failed ***\n";
#endif

/*
 ************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r(gd_t *id, ulong dest_addr)
{
	char *s;
	//bd_t *bd;
	ulong malloc_start;
#ifdef CONFIG_NONCACHE_MEMORY
	uint  malloc_noncache_start;
#endif
#if !defined(CONFIG_SYS_NO_FLASH)
	ulong flash_size;
#endif
	int workmode;
	int ret;

	gd = id;
	//bd = gd->bd;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	monitor_flash_len = _end_ofs;

	i2c_init(CONFIG_SYS_I2C_SPEED,0x46);
	malloc_start = dest_addr - TOTAL_MALLOC_LEN - sizeof(struct spare_boot_head_t);
#ifdef CONFIG_NONCACHE_MEMORY_SIZE
	malloc_start &= (~(0x00100000 -1));
	malloc_noncache_start = malloc_start - CONFIG_NONCACHE_MEMORY_SIZE;
	gd->malloc_noncache_start = malloc_noncache_start;
#endif
	/* Enable caches */
	enable_caches();
	debug("monitor flash len: %08lX\n", monitor_flash_len);
	board_init();	/* Setup chipselects */
#ifdef CONFIG_SERIAL_MULTI
	serial_initialize();
#endif
	debug("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

#ifdef CONFIG_LOGBUFFER
	logbuff_init_ptrs();
#endif
#ifdef CONFIG_POST
	post_output_backlog();
#endif

	/* The Malloc area is immediately below the monitor copy in DRAM */
	mem_malloc_init (malloc_start, TOTAL_MALLOC_LEN);
#ifdef CONFIG_NONCACHE_MEMORY
	mem_noncache_malloc_init(malloc_noncache_start, CONFIG_NONCACHE_MEMORY_SIZE);
#endif
	workmode = uboot_spare_head.boot_data.work_mode;
	debug("work mode %d\n", workmode);

	axp_reinit();
	//uboot_spare_head.boot_data.work_mode = WORK_MODE_CARD_PRODUCT;
#ifdef 	CONFIG_ARCH_HOMELET
	gpio_control();
#endif
#if 0
#if !defined(CONFIG_SYS_NO_FLASH)
	puts("Flash: ");

	flash_size = flash_init();
	if (flash_size > 0) {
# ifdef CONFIG_SYS_FLASH_CHECKSUM
		print_size(flash_size, "");
		/*
		 * Compute and print flash CRC if flashchecksum is set to 'y'
		 *
		 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
		 */
		s = getenv("flashchecksum");
		if (s && (*s == 'y')) {
			printf("  CRC: %08X", crc32(0,
				(const unsigned char *) CONFIG_SYS_FLASH_BASE,
				flash_size));
		}
		putc('\n');
# else	/* !CONFIG_SYS_FLASH_CHECKSUM */
		print_size(flash_size, "\n");
# endif /* CONFIG_SYS_FLASH_CHECKSUM */
	} else {
		puts(failed);
		hang();
	}
#endif
#endif
	/* set up exceptions */
	interrupt_init();
	/* enable exceptions */
	enable_interrupts();
	sunxi_dma_init();
#ifdef DEBUG
    puts("ready to config storage\n");
#endif
	if((workmode == WORK_MODE_BOOT) || (workmode == WORK_MODE_CARD_PRODUCT))
	{
#if (defined(CONFIG_SUNXI_DISPLAY) || defined(CONFIG_SUN7I_DISPLAY))
	    drv_disp_init();
#endif
		board_display_device_open();
		board_display_layer_request();
	}

	ret = sunxi_flash_handle_init();
	if(!ret)
	{
		sunxi_partition_init();
	}
//#else
//#if defined(CONFIG_CMD_NAND)
//	if(!storage_type){
//		puts("NAND:  ");
//		nand_init();        /* go init the NAND */
//	}
//#endif/*CONFIG_CMD_NAND*/
//
//#if defined(CONFIG_GENERIC_MMC)
//	if(storage_type){
//		puts("MMC:   ");
//		mmc_initialize(bd);
//	}
//#endif/*CONFIG_GENERIC_MMC*/

#ifdef 	CONFIG_ARCH_HOMELET
	if (!check_boot_recovery_key())
	{
		int ret;
		sprite_led_init();
		ret = sprite_form_sysrecovery();
		sprite_led_exit(ret);
	}
	if (!check_usb_recovery_key())
	{
		write_usb_recovery_to_misc();
	}
#endif

#ifdef CONFIG_HAS_DATAFLASH
	AT91F_DataflashInit();
	dataflash_print_info();
#endif

	/* initialize environment */
	env_relocate();
#if defined(CONFIG_CMD_PCI) || defined(CONFIG_PCI)
	arm_pci_init();
#endif

	/* IP Address */
	gd->bd->bi_ip_addr = getenv_IPaddr("ipaddr");
	stdio_init();	/* get the devices list going. */
	jumptable_init();
#if defined(CONFIG_API)
	/* Initialize API */
	api_init();
#endif

	console_init_r();	/* fully init console as a device */
#if defined(CONFIG_ARCH_MISC_INIT)
	/* miscellaneous arch dependent initialisations */
	arch_misc_init();
#endif
#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r();
#endif
	/* Perform network card initialisation if necessary */
#if defined(CONFIG_DRIVER_SMC91111) || defined (CONFIG_DRIVER_LAN91C96)
	/* XXX: this needs to be moved to board init */
	if (getenv("ethaddr")) {
		uchar enetaddr[6];
		eth_getenv_enetaddr("ethaddr", enetaddr);
		smc_set_mac_addr(enetaddr);
	}
#endif /* CONFIG_DRIVER_SMC91111 || CONFIG_DRIVER_LAN91C96 */

	/* Initialize from environment */
	s = getenv("loadaddr");
	if (s != NULL)
		load_addr = simple_strtoul(s, NULL, 16);
#if defined(CONFIG_CMD_NET)
	s = getenv("bootfile");
	if (s != NULL)
		copy_filename(BootFile, s, sizeof(BootFile));
#endif
#ifdef BOARD_LATE_INIT
	board_late_init();
#endif
#ifdef CONFIG_BITBANGMII
	bb_miiphy_init();
#endif
#if defined(CONFIG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	puts("Net:   ");
#endif
	eth_initialize(gd->bd);
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
#endif

#ifdef CONFIG_POST
	post_run(NULL, POST_RAM | post_bootmode_get(0));
#endif

#if defined(CONFIG_PRAM) || defined(CONFIG_LOGBUFFER)
	/*
	 * Export available size of memory for Linux,
	 * taking into account the protected RAM at top of memory
	 */
	{
		ulong pram;
		uchar memsz[32];
#ifdef CONFIG_PRAM
		char *s;

		s = getenv("pram");
		if (s != NULL)
			pram = simple_strtoul(s, NULL, 10);
		else
			pram = CONFIG_PRAM;
#else
		pram = 0;
#endif
#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
		/* Also take the logbuffer into account (pram is in kB) */
		pram += (LOGBUFF_LEN + LOGBUFF_OVERHEAD) / 1024;
#endif
#endif
		sprintf((char *)memsz, "%ldk", (gd->ram_size / 1024) - pram);
		setenv("mem", (char *)memsz);
	}
#endif
	//sprite_cartoon_test();
	if(workmode == WORK_MODE_BOOT)
    {
#if defined(CONFIG_SUNXI_SCRIPT_REINIT)
		{
			FATFS script_mount;
			int  ret;
			uint read_bytes = 0;
			FIL  script_fs;
			uchar  *buf = NULL;

			f_mount(0, &script_mount);
			ret = f_open (&script_fs, "0:script.bin", FA_OPEN_EXISTING | FA_READ | FA_WRITE );
			if(ret)
			{
				printf("cant open script.bin, maybe it is not exist\n");
			}
			else
			{
				buf = (uchar *)malloc(100 * 1024);
				memset(buf, 0, 100 * 1024);

				if(!f_read(&script_fs, buf, 100 * 1024, &read_bytes))
				{
					printf("f_read read bytes = %d\n", read_bytes);
				}
				f_close(&script_fs);
				puts("try to unlink file ");
				printf("%d\n", f_unlink("0:script.bin"));
			}
			f_mount(0, NULL);
			if(read_bytes > 0)
			{
				char *tmp_target_buffer = (char *)(CONFIG_SYS_TEXT_BASE - 0x01000000);

				sunxi_flash_exit(1);

				memcpy(tmp_target_buffer + uboot_spare_head.boot_head.uboot_length, buf, read_bytes);
				sunxi_sprite_download_uboot(tmp_target_buffer, uboot_spare_head.boot_data.storage_type, 1);

				reset_cpu(0);
			}
			if(buf)
			{
				free(buf);
			}
		}
#endif
    	printf("WORK_MODE_BOOT\n");
     //Justin Porting for BPI-M2 20150727 Start
    axp22_set_supply_status_byname("dldo2", 2800, 1);
	axp22_set_supply_status_byname("dldo3", 3300, 1);
	axp22_set_supply_status_byname("gpio1", 2800, 1);
	axp22_set_supply_status_byname("gpio2", 3300, 1);
	axp22_set_supply_status_byname("aldo1", 3300, 1);
     //Justin Porting for BPI-M2 20150727 Start
#if (defined(CONFIG_SUN6I) || defined(CONFIG_A50) || defined(CONFIG_SUN7I))
		if(!ret)
		{
#ifndef CONFIG_ARCH_HOMELET
			printf("board_status_probe\n");
			board_status_probe(0);
#endif
			printf("sunxi_bmp_display\n");
			sunxi_bmp_display("bootlogo.bmp");
		}
#endif
	}
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;)
	{
		main_loop();
	}
	hang();
	/* NOTREACHED - no way out of command loop except booting */
}

void hang(void)
{
	puts("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
