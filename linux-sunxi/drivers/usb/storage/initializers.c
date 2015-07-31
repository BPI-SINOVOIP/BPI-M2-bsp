/* Special Initializers for certain USB Mass Storage devices
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/errno.h>

#include "usb.h"
#include "initializers.h"
#include "debug.h"
#include "transport.h"

/* This places the Shuttle/SCM USB<->SCSI bridge devices in multi-target
 * mode */
int usb_stor_euscsi_init(struct us_data *us)
{
	int result;

	US_DEBUGP("Attempting to init eUSCSI bridge...\n");
	us->iobuf[0] = 0x1;
	result = usb_stor_control_msg(us, us->send_ctrl_pipe,
			0x0C, USB_RECIP_INTERFACE | USB_TYPE_VENDOR,
			0x01, 0x0, us->iobuf, 0x1, 5000);
	US_DEBUGP("-- result is %d\n", result);

	return 0;
}

/* This function is required to activate all four slots on the UCR-61S2B
 * flash reader */
int usb_stor_ucr61s2b_init(struct us_data *us)
{
	struct bulk_cb_wrap *bcb = (struct bulk_cb_wrap*) us->iobuf;
	struct bulk_cs_wrap *bcs = (struct bulk_cs_wrap*) us->iobuf;
	int res;
	unsigned int partial;
	static char init_string[] = "\xec\x0a\x06\x00$PCCHIPS";

	US_DEBUGP("Sending UCR-61S2B initialization packet...\n");

	bcb->Signature = cpu_to_le32(US_BULK_CB_SIGN);
	bcb->Tag = 0;
	bcb->DataTransferLength = cpu_to_le32(0);
	bcb->Flags = bcb->Lun = 0;
	bcb->Length = sizeof(init_string) - 1;
	memset(bcb->CDB, 0, sizeof(bcb->CDB));
	memcpy(bcb->CDB, init_string, sizeof(init_string) - 1);

	res = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, bcb,
			US_BULK_CB_WRAP_LEN, &partial);
	if (res)
		return -EIO;

	US_DEBUGP("Getting status packet...\n");
	res = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe, bcs,
			US_BULK_CS_WRAP_LEN, &partial);
	if (res)
		return -EIO;

	return 0;
}

/* This places the HUAWEI E220 devices in multi-port mode */
int usb_stor_huawei_e220_init(struct us_data *us)
{
#if 0
	int result;

	result = usb_stor_control_msg(us, us->send_ctrl_pipe,
				      USB_REQ_SET_FEATURE,
				      USB_TYPE_STANDARD | USB_RECIP_DEVICE,
				      0x01, 0x0, NULL, 0x0, 1000);
	US_DEBUGP("Huawei mode set result is %d\n", result);
	return 0;
#else
	printk("====usb_stor_huawei_e220_init===>\n");
	return -ENODEV;
#endif

}

/* This places the HUAWEI E303 devices in multi-port mode */
int usb_stor_huawei_e303_init(struct us_data *us)
{
	printk("====usb_stor_huawei_e303_init===>\n");
	return -ENODEV;
}

//-----------------------------------------------------------------------------
//  for huawei ril
//-----------------------------------------------------------------------------
int usb_stor_huawei_e220_init_ex(struct us_data *us)
{
	int result;

	result = usb_stor_control_msg(us, us->send_ctrl_pipe,
								USB_REQ_SET_FEATURE,
								USB_TYPE_STANDARD | USB_RECIP_DEVICE,
								0x01, 0x0, NULL, 0x0, 1000);
	US_DEBUGP("Huawei mode set result is %d\n", result);

	return 0;
}

#define IS_HUAWEI_DONGLES 1
#define NOT_HUAWEI_DONGLES 0

static int usb_stor_huawei_dongles_pid(struct us_data *us)
{
    int ret = NOT_HUAWEI_DONGLES;
    struct usb_interface_descriptor *idesc = NULL;

    idesc = &us->pusb_intf->cur_altsetting->desc;
    if(NULL != idesc) {
        if(0x0000 == idesc->bInterfaceNumber) {
            if ((0x1401 <= us->pusb_dev->descriptor.idProduct && 0x1600 >= us->pusb_dev->descriptor.idProduct)
                || (0x1c02 <= us->pusb_dev->descriptor.idProduct && 0x2202 >= us->pusb_dev->descriptor.idProduct)
                || (0x1001 == us->pusb_dev->descriptor.idProduct)
                || (0x1003 == us->pusb_dev->descriptor.idProduct)
                || (0x1004 == us->pusb_dev->descriptor.idProduct)) {
                if ((0x1501 <= us->pusb_dev->descriptor.idProduct) && (0x1504 >= us->pusb_dev->descriptor.idProduct)) {
                    ret = NOT_HUAWEI_DONGLES;
                } else {
                    ret = IS_HUAWEI_DONGLES;
                }
            }
        }
    }

    return ret;
}

int usb_stor_huawei_scsi_init(struct us_data *us)
{
    int result = 0;
    int act_len = 0;

    unsigned char cmd[32] = {0x55, 0x53, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
                             0x06, 0x30, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    printk("====usb_stor_huawei_scsi_init===>\n"); //

    result = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, cmd, 31, &act_len);
    US_DEBUGP("usb_stor_bulk_transfer_buf performing result is %d, transfer the actual length=%d", result, act_len);

    return result;
}

int usb_stor_huawei_init(struct us_data *us)
{
    int result = 0;

    printk("====usb_stor_huawei_init===>\n"); //

    if (usb_stor_huawei_dongles_pid(us)) {
        if ((0x1446 <= us->pusb_dev->descriptor.idProduct)) {
            result = usb_stor_huawei_scsi_init(us);
        } else {
            result = usb_stor_huawei_e220_init_ex(us);
        }
    }

    return result;
}

//AC560--ZTE--	0x19d20026->0x19d20094	before convert to modem,don't report disk dev
int usb_stor_ZTE_AC580_init(struct us_data *us) // PID = 0x0026
{
#if 0
	int result = 0;
	int act_len = 0;

	result = usb_stor_control_msg(us, us->send_ctrl_pipe,USB_REQ_CLEAR_FEATURE,
		USB_TYPE_STANDARD | USB_RECIP_ENDPOINT,0x0, 0x89, NULL, 0x0, 1000);

	US_DEBUGP("usb_stor_control_msg performing result is %d\n", result);
	printk("====AC580/AC560===>usb_stor_control_msg performing result is %d\n", result);

	result = usb_stor_control_msg(us, us->send_ctrl_pipe,USB_REQ_CLEAR_FEATURE,
		USB_TYPE_STANDARD | USB_RECIP_ENDPOINT,0x0, 0x9, NULL, 0x0, 1000);

	US_DEBUGP("usb_stor_control_msg performing result is %d\n", result);
	printk("====AC580/AC560===>usb_stor_control_msg performing result is %d\n", result);
	return (result ? 0 : -ENODEV);
#else
	printk("====%s===>\n",__FUNCTION__);
	return -ENODEV;
#endif
}

//AC560--ZTE--	0x19d20026->0x19d20094	before convert to modem,don't report disk dev
int usb_stor_ZTE_AC580_init2(struct us_data *us) // PID = 0x0026
{
	printk("====%s===>\n",__FUNCTION__);
	return -ENODEV;
}

int usb_stor_ASB_init(struct us_data *us)
{
	printk("====%s===>\n",__FUNCTION__);
	return -ENODEV;
}

int usb_stor_TechFaith_init(struct us_data *us)
{
	printk("====%s===>\n",__FUNCTION__);
	usb_stor_port_reset(us);
	return -ENODEV;
}

int usb_stor_Shichuangxing_init(struct us_data *us)
{
	printk("====usb_stor_Shichuangxing_init===>\n");
	return -ENODEV;
}

int usb_stor_wangxun_init(struct us_data *us)
{

	printk("====usb_stor_wangxun_init===>\n");
	usb_stor_port_reset(us);
	return -ENODEV;

}
int usb_stor_people_init(struct us_data *us)
{
	printk("====people_init===>\n");
    usb_stor_control_msg(us, us->send_ctrl_pipe, USB_REQ_CLEAR_FEATURE,
		USB_TYPE_STANDARD | USB_RECIP_ENDPOINT,0x0, 0x83, NULL, 0x0, 1000);
    return -ENODEV;
}

int usb_modem_init(struct us_data *us)
{
	printk("====modem_init===>\n");
    usb_stor_control_msg(us, us->send_ctrl_pipe, USB_REQ_CLEAR_FEATURE,
		USB_TYPE_STANDARD | USB_RECIP_ENDPOINT,0x0, 0x85, NULL, 0x0, 1000);
    return -ENODEV;
}

static int usb_stor_send_message(struct us_data *us, void * buf, int len)
{
    int result = 0;
    int act_len = 0;

    result = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, buf, len, &act_len);

    return result;
}

/*
static unsigned char ChinaMobileCmd[] = {

};

static unsigned char ChinaUnicomCmd[6] = {
	0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
	0xC0, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x71,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned char ChinaTelecomCmd[] = {

};
*/

static unsigned char eject[] = {
	0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1B,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static unsigned int dongle_vid[] = {
	0x05c6,   //Qualcomm
    0x12d1,   //huawei
	0x19d2,   //ZTE
	0x1e89,   //strongrising
	0x257A,   //yuga
};
#define DONGLE_VID_SIZE  sizeof(dongle_vid)/sizeof(dongle_vid[0])

struct dongle_info{
	char *vendor_name;
	char *production_name;

	unsigned int vid;
	unsigned int pid;
};

static struct dongle_info ignore_dongle[] = {
	//{NULL, NULL, 0x19d2, 0x2000},
	{NULL, NULL, 0, 0}
};
#define IGNORE_DONGLE_SIZE  sizeof(ignore_dongle)/sizeof(ignore_dongle[0])

static int usb_stor_ignore_dongle(char *vendor_name,
                                  char *production_name,
                                  unsigned int vid,
                                  unsigned int pid)
{
	int i = 0;

	for(i = 0; i < DONGLE_VID_SIZE; i++){
		if((vid == ignore_dongle[i].vid) && (pid == ignore_dongle[i].pid)){
			printk("%s: ignore dongle(%s, %s, %d, %d)\n", __func__, vendor_name, production_name, vid, pid);
			return 1;
		}
	}

	return 0;
}

static int usb_stor_inquiry(struct us_data *us, void *buffer, int len)
{
	unsigned char inquiry_msg[] = {
	  0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
	  0x24, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x12,
	  0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	char csw[13] = {0};
	int result;

	result = usb_stor_bulk_transfer_buf(us,
			us->send_bulk_pipe,
			inquiry_msg, sizeof(inquiry_msg), NULL);
	if (result != USB_STOR_XFER_GOOD) {
		result = USB_STOR_XFER_ERROR;
		goto out;
	}

	result = usb_stor_bulk_transfer_buf(us,
			us->recv_bulk_pipe,
			buffer, len, NULL);
	if (result != USB_STOR_XFER_GOOD) {
		result = USB_STOR_XFER_ERROR;
		goto out;
	}

	/* Read the CSW */
	usb_stor_bulk_transfer_buf(us,
			us->recv_bulk_pipe,
			csw, 13, NULL);

out:
	return result;
}

/* ���������Ϊ�����ˡ���ͨ��CD_ROM������ȥת�� */
int usb_stor_dongle_change_command(struct us_data *us)
{
    char buffer[36];
	int result;
	int i = 0;

    //if the device is 3g dongle, they have less than three interface.
	if(us->pusb_dev->actconfig->desc.bNumInterfaces > 3){
		printk("%s: the device have more than three interface, it may have changed \n", __func__);
		return -1;
	}

	for(i = 0; i < DONGLE_VID_SIZE; i++){
		if(dongle_vid[i] == us->pusb_dev->descriptor.idVendor){
			printk("%s: find vid_pid: 0x%x_0x%x\n", __func__,
				   us->pusb_dev->descriptor.idVendor, us->pusb_dev->descriptor.idProduct);

			if(usb_stor_ignore_dongle(NULL, NULL,
				us->pusb_dev->descriptor.idVendor,
				us->pusb_dev->descriptor.idProduct)){
				return -1;
			}

		    result = usb_stor_inquiry(us, buffer, 36);
			if(result == 0){
				/* is cd-rom ? */
				if((buffer[0] & 0x1f) == 0x5){
					usb_stor_send_message(us, (void *)eject, sizeof(eject));
				}
			}
		}
	}

    /* maybe is really a cdrom device, so must return zero */
    return 0;
}

int usb_stor_dongle_not_report_disk(struct us_data *us)
{
    printk("====usb_stor_dongle_not_report_disk===>\n");

    return -ENODEV;
}

int usb_stor_dongle_common_init(struct us_data *us)
{
	int result;

    printk("====usb_stor_dongle_common_init===>\n");

	result = usb_stor_send_message(us, (void *)eject, sizeof(eject));
    printk("usb_stor_dongle_common_init: result=%d\n", result);

	return 0;
}

