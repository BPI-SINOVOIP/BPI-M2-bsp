
#ifndef __SW_DONGLE_H__
#define __SW_DONGLE_H__

int zte_a355_init(struct usb_interface *intf);
int zte_mu350_init(struct usb_interface *intf);
int sw_dongle_init(struct usb_serial *serial, struct usb_interface *intf);

#endif  //__SW_DONGLE_H__

