
#ifndef __SW_DONGLE_INFO_H__
#define __SW_DONGLE_INFO_H__

#define HW_USB_DEVICE_AND_INTERFACE_INFO(vend, cl, sc, pr) \
    .match_flags = USB_DEVICE_ID_MATCH_INT_INFO | USB_DEVICE_ID_MATCH_VENDOR, \
    .idVendor = (vend), \
    .bInterfaceClass = (cl), \
    .bInterfaceSubClass = (sc), \
    .bInterfaceProtocol = (pr)

#endif  //__SW_DONGLE_INFO_H__

