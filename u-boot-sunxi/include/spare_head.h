/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef  __spare_head_h__
#define  __spare_head_h__

/* work mode */
#define WORK_MODE_PRODUCT      (1<<4)
#define WORK_MODE_UPDATE       (1<<5)

#define WORK_MODE_BOOT			0x00	//��������
#define WORK_MODE_USB_PRODUCT	0x10	//����USB����
#define WORK_MODE_CARD_PRODUCT	0x11	//���ڿ�����
#define WORK_MODE_USB_DEBUG	    0x12    //����usb����Э����ɵĲ���
#define WORK_MODE_USB_UPDATE	0x20	//����USB����
#define WORK_MODE_OUTER_UPDATE	0x21	//�����ⲿ������

#define WORK_MODE_USB_TOOL_PRODUCT	0x04	//��������
#define WORK_MODE_USB_TOOL_UPDATE	0x08	//��������

#define UBOOT_MAGIC				"uboot"
#define STAMP_VALUE             0x5F0A6C39
#define ALIGN_SIZE				16 * 1024
#define MAGIC_SIZE              8
#define STORAGE_BUFFER_SIZE     (256)

#define SUNXI_UPDATE_NEXT_ACTION_NORMAL			(1)
#define SUNXI_UPDATE_NEXT_ACTION_REBOOT			(2)
#define SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN		(3)
#define SUNXI_UPDATE_NEXT_ACTION_REUPDATE		(4)
#define SUNXI_UPDATE_NEXT_ACTION_BOOT			(5)

#define BOOT0_SDMMC_START_ADDR                  (16)
#define UBOOT_START_SECTOR_IN_SDMMC             (38192)

typedef struct _normal_gpio_cfg
{
    char      port;                       //�˿ں�
    char      port_num;                   //�˿��ڱ��
    char      mul_sel;                    //���ܱ��
    char      pull;                       //����״̬
    char      drv_level;                  //������������
    char      data;                       //�����ƽ
    char      reserved[2];                //����λ����֤����
}
normal_gpio_cfg;

//SD��������ݽṹ
typedef struct sdcard_spare_info_t
{
	int 			card_no[4];                   //��ǰ�����Ŀ����������
	int 			speed_mode[4];                //�����ٶ�ģʽ��0�����٣�����������
	int				line_sel[4];                  //�������ƣ�0: 1�ߣ�������4��
	int				line_count[4];                //��ʹ���ߵĸ���
}
sdcard_spare_info;



#endif


