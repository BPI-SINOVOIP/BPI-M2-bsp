
#ifndef __LCD_PANNEL_CFG_H__
#define __LCD_PANNEL_CFG_H__

#include "../bsp_display.h"
#include "panels/panels.h"

extern void LCD_OPEN_FUNC(__u32 sel, LCD_FUNC func, __u32 delay/*ms*/);
extern void LCD_CLOSE_FUNC(__u32 sel, LCD_FUNC func, __u32 delay/*ms*/);
extern void LCD_get_reg_bases(__reg_bases_t *para);
extern void LCD_delay_ms(__u32 ms) ;
extern void LCD_delay_us(__u32 ns);
extern void TCON_open(__u32 sel);
extern void TCON_close(__u32 sel);
extern __s32 LCD_PWM_EN(__u32 sel, __bool b_en);
extern __s32 LCD_BL_EN(__u32 sel, __bool b_en);
extern __s32 LCD_POWER_EN(__u32 sel, __bool b_en);
extern void LCD_CPU_register_irq(__u32 sel, void (*Lcd_cpuisr_proc) (void));
extern void LCD_CPU_WR(__u32 sel, __u32 index, __u32 data);
extern void LCD_CPU_WR_INDEX(__u32 sel,__u32 index);
extern void LCD_CPU_WR_DATA(__u32 sel, __u32 data);
extern void LCD_CPU_AUTO_FLUSH(__u32 sel, __bool en);
extern __s32 LCD_GPIO_request(__u32 sel, __u32 io_index);
extern __s32 LCD_GPIO_release(__u32 sel,__u32 io_index);
extern __s32 LCD_GPIO_set_attr(__u32 sel,__u32 io_index, __bool b_output);
extern __s32 LCD_GPIO_read(__u32 sel,__u32 io_index);
extern __s32 LCD_GPIO_write(__u32 sel,__u32 io_index, __u32 data);

extern __s32 dsi_dcs_wr(__u32 sel,__u8 cmd,__u8* para_p,__u32 para_num);
extern __s32 dsi_dcs_wr_0para(__u32 sel,__u8 cmd);
extern __s32 dsi_dcs_wr_1para(__u32 sel,__u8 cmd,__u8 para);
extern __s32 dsi_dcs_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
extern __s32 dsi_dcs_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
extern __s32 dsi_dcs_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
extern __s32 dsi_dcs_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);
extern __s32 lcd_iic_write(__u8 slave_addr, __u8 sub_addr, __u8 value);
extern __s32 lcd_iic_read(__u8 slave_addr, __u8 sub_addr, __u8* value);

extern __s32 pwm_set_para(__u32 channel, __pwm_info_t * pwm_info);
extern __s32 pwm_get_para(__u32 channel, __pwm_info_t * pwm_info);

#define mdelay LCD_delay_ms
#define udelay LCD_delay_us
//#define printk __inf
//#define pr_err __wrn
#define printk(...)
#define pr_err(...)
#define gpio_request OSAL_GPIO_Request
#define gpio_request_ex OSAL_GPIO_Request_Ex
#define gpio_release OSAL_GPIO_Release
#define gpio_get_all_pin_status OSAL_GPIO_DevGetAllPins_Status
#define gpio_get_one_pin_status OSAL_GPIO_DevGetONEPins_Status
#define gpio_set_one_pin_status OSAL_GPIO_DevSetONEPin_Status
#define gpio_set_one_pin_io_status OSAL_GPIO_DevSetONEPIN_IO_STATUS
#define gpio_set_one_pin_pull OSAL_GPIO_DevSetONEPIN_PULL_STATUS
#define gpio_read_one_pin_value OSAL_GPIO_DevREAD_ONEPIN_DATA
#define gpio_write_one_pin_value OSAL_GPIO_DevWRITE_ONEPIN_DATA
#define script_parser_fetch OSAL_Script_FetchParser_Data

#define BIT0		  0x00000001
#define BIT1		  0x00000002
#define BIT2		  0x00000004
#define BIT3		  0x00000008
#define BIT4		  0x00000010
#define BIT5		  0x00000020
#define BIT6		  0x00000040
#define BIT7		  0x00000080
#define BIT8		  0x00000100
#define BIT9		  0x00000200
#define BIT10		  0x00000400
#define BIT11		  0x00000800
#define BIT12		  0x00001000
#define BIT13		  0x00002000
#define BIT14		  0x00004000
#define BIT15		  0x00008000
#define BIT16		  0x00010000
#define BIT17		  0x00020000
#define BIT18		  0x00040000
#define BIT19		  0x00080000
#define BIT20		  0x00100000
#define BIT21		  0x00200000
#define BIT22		  0x00400000
#define BIT23		  0x00800000
#define BIT24		  0x01000000
#define BIT25		  0x02000000
#define BIT26		  0x04000000
#define BIT27		  0x08000000
#define BIT28		  0x10000000
#define BIT29		  0x20000000
#define BIT30		  0x40000000
#define BIT31		  0x80000000

#define sys_get_wvalue(n)   (*((volatile __u32 *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile __u32 *)(n))  = (c))   /* word output */


#endif

