#ifndef _USBCTRL_H_
#define _USBCTRL_H_

#define USB_IDX_A		0
#define USB_IDX_B		1
#define USB_IDX_MAX		2

#define USB_CMD_ON		0
#define USB_CMD_OFF	1
#define USB_CMD_IF		2
#define USB_CMD_MAX	3

extern char usb_index_str[USB_IDX_MAX][2];
extern char usb_state_str[USB_CMD_MAX][8];

extern int get_dwc_driver_version(void);
extern int usbpower(int index,int cmd);
#endif