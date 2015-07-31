/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_messages.h
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __AR100_MESSAGES_H__
#define __AR100_MESSAGES_H__

#include <mach/ar100.h>

/* message attributes(only use 8bit) */
#define AR100_MESSAGE_ATTR_SOFTSYN      (1<<0)  /* need soft syn with another cpu */
#define AR100_MESSAGE_ATTR_HARDSYN      (1<<1)  /* need hard syn with another cpu */

/* message states */
#define AR100_MESSAGE_FREED             (0x0)   /* freed state       */
#define AR100_MESSAGE_ALLOCATED         (0x1)   /* allocated state   */
#define AR100_MESSAGE_INITIALIZED       (0x2)   /* initialized state */
#define AR100_MESSAGE_RECEIVED          (0x3)   /* received state    */
#define AR100_MESSAGE_PROCESSING        (0x4)   /* processing state  */
#define AR100_MESSAGE_PROCESSED         (0x5)   /* processed state   */
#define AR100_MESSAGE_FEEDBACKED        (0x6)   /* feedback state    */

/* call back struct */
typedef struct ar100_msg_cb
{
	ar100_cb_t   handler;
	void        *arg;
} ar100_msg_cb_t;

/*
 * the structure of message frame,
 * this structure will transfer between ar100 and ac327.
 * sizeof(struct message) : 64Byte.
 */
typedef struct ar100_message
{
	unsigned char            state;     /* identify the used status of message frame */
	unsigned char            attr;      /* message attribute : SYN OR ASYN           */
	unsigned char            type;      /* message type : DVFS_REQ                   */
	unsigned char            result;    /* message process result                    */
	struct ar100_message         *next; /* pointer of next message frame             */
	struct ar100_msg_cb      cb;        /* the callback function and arg of message  */
	void                 *private;      /* message private data                      */
	unsigned int             paras[11]; /* the parameters of message                 */
} ar100_message_t;

/* the base of messages */
#define AR100_MESSAGE_BASE       (0x10)

/* standby commands */
#define AR100_SSTANDBY_ENTER_REQ        (AR100_MESSAGE_BASE + 0x00)  /* request to enter       (ac327 to ar100)       */
#define AR100_SSTANDBY_RESTORE_NOTIFY   (AR100_MESSAGE_BASE + 0x01)  /* restore finished       (ac327 to ar100)       */
#define AR100_NSTANDBY_ENTER_REQ        (AR100_MESSAGE_BASE + 0x02)  /* request to enter       (ac327 to ar100)       */
#define AR100_NSTANDBY_WAKEUP_NOTIFY    (AR100_MESSAGE_BASE + 0x03)  /* wakeup notify          (ar100 to ac327)       */
#define AR100_NSTANDBY_RESTORE_REQ      (AR100_MESSAGE_BASE + 0x04)  /* request to restore     (ac327 to ar100)       */
#define AR100_NSTANDBY_RESTORE_COMPLETE (AR100_MESSAGE_BASE + 0x05)  /* ar100 restore complete (ar100 to ac327)       */
#define AR100_ESSTANDBY_ENTER_REQ       (AR100_MESSAGE_BASE + 0x06)  /* request to enter       (ac327 to ar100)       */
#define AR100_FAKE_POWER_OFF_REQ        (AR100_MESSAGE_BASE + 0x07)  /* request to enter       (ac327 to ar100)       */
#define AR100_TSTANDBY_ENTER_REQ        (AR100_MESSAGE_BASE + 0x08)  /* request to enter       (ac327 to ar100)       */
#define AR100_TSTANDBY_RESTORE_NOTIFY   (AR100_MESSAGE_BASE + 0x09)  /* restore finished       (ac327 to ar100)       */

/* dvfs commands */
#define AR100_CPUX_DVFS_REQ             (AR100_MESSAGE_BASE + 0x20)  /* request dvfs           (ac327 to ar100)       */
#define AR100_CPUX_DVFS_CFG_VF_REQ      (AR100_MESSAGE_BASE + 0x21)  /* request config dvfs v-f table(ac327 to ar100) */

/* pmu commands */
#define AR100_AXP_READ_REGS             (AR100_MESSAGE_BASE + 0x41)  /* read registers          (ac327 to ar100)      */
#define AR100_AXP_WRITE_REGS            (AR100_MESSAGE_BASE + 0x42)  /* write registers        (ac327 to ar100)       */
#define AR100_AXP_INT_COMING_NOTIFY     (AR100_MESSAGE_BASE + 0x45)  /* interrupt coming notify(ar100 to ac327)       */
#define AR100_AXP_DISABLE_IRQ           (AR100_MESSAGE_BASE + 0x46)  /* disable axp irq of ar100                      */
#define AR100_AXP_ENABLE_IRQ            (AR100_MESSAGE_BASE + 0x47)  /* enable axp irq of ar100                       */
#define AR100_AXP_CLR_REGS_BITS_SYNC    (AR100_MESSAGE_BASE + 0x48)  /* clear registers bits sync   (ac327 to ar100)  */
#define AR100_AXP_SET_REGS_BITS_SYNC    (AR100_MESSAGE_BASE + 0x49)  /* set registers bits sync   (ac327 to ar100)    */
#define AR100_AXP_GET_CHIP_ID           (AR100_MESSAGE_BASE + 0x4a)  /* axp get chip id                               */
#define AR100_AXP_SET_PARAS             (AR100_MESSAGE_BASE + 0x4b)  /* config axp parameters (ac327 to ar100)        */

/* set ar100 debug level commands */
#define AR100_SET_DEBUG_LEVEL           (AR100_MESSAGE_BASE + 0x50)  /* set ar100 debug level  (ac327 to ar100)      */
#define AR100_MESSAGE_LOOPBACK          (AR100_MESSAGE_BASE + 0x51)  /* loopback message  (ac327 to ar100)           */
#define AR100_SET_UART_BAUDRATE         (AR100_MESSAGE_BASE + 0x52)  /* set uart baudrate (ac327 to ar100)           */
#define AR100_SET_DRAM_PARAS            (AR100_MESSAGE_BASE + 0x53)  /* config dram parameter (ac327 to ar100)       */
#define AR100_SET_DEBUG_DRAM_CRC_PARAS  (AR100_MESSAGE_BASE + 0x54)  /* config dram crc parameters (ac327 to ar100)  */
#define AR100_SET_IR_PARAS              (AR100_MESSAGE_BASE + 0x55)  /* config ir parameter (ac327 to ar100)         */

/* ar100 initialize state notify commands */
#define AR100_STARTUP_NOTIFY            (AR100_MESSAGE_BASE + 0x80)  /* ar100 init state notify(ar100 to ac327)      */

#endif  /* __AR100_MESSAGES_H */
