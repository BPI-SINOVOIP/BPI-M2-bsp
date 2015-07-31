/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX51-3Stack Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H


#define CONFIG_MX51	/* in a mx51 */
#define CONFIG_SYS_TEXT_BASE	0x97800000

#include <asm/arch/imx-regs.h>

#define CONFIG_SYS_MX5_HCLK	24000000
#define CONFIG_SYS_MX5_CLK32		32768
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_CMDLINE_TAG	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define BOARD_LATE_INIT

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(2048 * 1024)

/*
 * Hardware drivers
 */
#define CONFIG_MXC_UART
#define CONFIG_SYS_MX51_UART3
#define CONFIG_MXC_GPIO
#define CONFIG_MXC_SPI
#define CONFIG_HW_WATCHDOG

 /*
 * SPI Configs
 * */
#define CONFIG_FSL_SF
#define CONFIG_CMD_SF

#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_STMICRO

/*
 * Use gpio 4 pin 25 as chip select for SPI flash
 * This corresponds to gpio 121
 */
#define CONFIG_SPI_FLASH_CS	(1 | (121 << 8))
#define CONFIG_SF_DEFAULT_MODE   SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED  25000000

#define CONFIG_ENV_SPI_CS	(1 | (121 << 8))
#define CONFIG_ENV_SPI_BUS      0
#define CONFIG_ENV_SPI_MAX_HZ	25000000
#define CONFIG_ENV_SPI_MODE	SPI_MODE_0

#define CONFIG_ENV_OFFSET       (6 * 64 * 1024)
#define CONFIG_ENV_SECT_SIZE    (1 * 64 * 1024)
#define CONFIG_ENV_SIZE		(4 * 1024)

#define CONFIG_FSL_ENV_IN_SF
#define CONFIG_ENV_IS_IN_SPI_FLASH

/* PMIC Controller */
#define CONFIG_FSL_PMIC
#define CONFIG_FSL_PMIC_BUS	0
#define CONFIG_FSL_PMIC_CS	0
#define CONFIG_FSL_PMIC_CLK	2500000
#define CONFIG_FSL_PMIC_MODE	SPI_MODE_0
#define CONFIG_RTC_MC13783

/*
 * MMC Configs
 */
#define CONFIG_FSL_ESDHC
#ifdef CONFIG_FSL_ESDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	(0x70004000)
#define CONFIG_SYS_FSL_ESDHC_NUM	1

#define CONFIG_MMC

#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION
#endif

#define CONFIG_CMD_DATE

/*
 * Eth Configs
 */
#define CONFIG_HAS_ETH1
#define CONFIG_NET_MULTI
#define CONFIG_MII
#define CONFIG_DISCOVER_PHY

#define CONFIG_FEC_MXC
#define IMX_FEC_BASE				FEC_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		0x1F

#define CONFIG_CMD_PING
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX			3
#define CONFIG_BAUDRATE				115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

#define CONFIG_CMD_SPI
#undef CONFIG_CMD_IMLS

#define CONFIG_BOOTDELAY        3

#define CONFIG_LOADADDR	0x90800000	/* loadaddr env var */

#define	CONFIG_EXTRA_ENV_SETTINGS	\
		"netdev=eth0\0"		\
		"loadaddr=0x90800000\0"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP
#define	CONFIG_SYS_PROMPT		"Vision II U-boot > "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		64	/* max number of command args */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_MEMTEST_START	0x90000000
#define CONFIG_SYS_MEMTEST_END		0x10000

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ			1000
#define CONFIG_CMDLINE_EDITING
#define CONFIG_SYS_HUSH_PARSER
#define	CONFIG_SYS_PROMPT_HUSH_PS2	"Vision II U-boot > "

/*
 * Stack sizes
 */
#define CONFIG_STACKSIZE		(128 * 1024)	/* regular stack */

/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_1			CSD0_BASE_ADDR
#define PHYS_SDRAM_1_SIZE		(256 * 1024 * 1024)
#define PHYS_SDRAM_2			CSD1_BASE_ADDR
#define PHYS_SDRAM_2_SIZE		(256 * 1024 * 1024)
#define CONFIG_SYS_SDRAM_BASE		0x90000000
#define CONFIG_SYS_INIT_RAM_ADDR	0x1FFE8000

#define CONFIG_SYS_INIT_RAM_SIZE		(64 * 1024)
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_SIZE - \
					GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_INIT_RAM_ADDR + \
					CONFIG_SYS_GBL_DATA_OFFSET)
#define CONFIG_BOARD_EARLY_INIT_F

/* 166 MHz DDR RAM */
#define CONFIG_SYS_DDR_CLKSEL		0
#define CONFIG_SYS_CLKTL_CBCDR		0x19239100

#define CONFIG_SYS_NO_FLASH

/*
 * Framebuffer and LCD
 */
#define CONFIG_PREBOOT
#define CONFIG_LCD
#define CONFIG_VIDEO_MX5
#define CONFIG_SYS_CONSOLE_ENV_OVERWRITE
#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define LCD_BPP		LCD_COLOR16
#define CONFIG_SPLASH_SCREEN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP

#endif				/* __CONFIG_H */
