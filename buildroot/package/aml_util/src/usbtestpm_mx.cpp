/* main.c */  
/********************************************
this file because m6 usb power control is defferent with other
usbA and usbB por is connected.
*********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <dirent.h>
#include "porting.h"

#define LOG_TAG "USBtestpm_mx"

#include "usbctrl.h"


int usbcheck(int index) 
{
    int rc = 0;
	rc = usbpower(index,USB_CMD_IF);
    return rc;
}

int usbset(int index,int cmd) 
{
    int rc = 0;
	rc = usbpower(index,cmd);
    return rc;
}

static const char USBA_DEVICE_PATH[] = "/sys/devices/lm0";
static const char USBB_DEVICE_PATH[] = "/sys/devices/lm1";

int check_usb_devices_exists(int port)
{
	if (port == 0) {
	    if (access(USBA_DEVICE_PATH, R_OK) == 0) {    		
	        return 0;
	    } else {    		  
	        return -1;
	    }
	} else {
	    if (access(USBB_DEVICE_PATH, R_OK) == 0) {    		
	        return 0;
	    } else {    		  
	        return -1;
	    }	
	}
		
}

int main()
{   
	int usb_a_exist = 0;
	int usb_b_exist = 0;
	int on_flag;
	int checkflag,ret;

	ret = get_dwc_driver_version();

	if(ret == -1)
	{
		printf("This dwc_otg version not support. Please check!\n");
		return -1;
	}
	
	if (check_usb_devices_exists(0) == 0)
	{
		usb_a_exist = 1;
		on_flag = 1;
	}
	else
	{
		usb_a_exist = 0;
		on_flag = 0;
	}
	
	if (check_usb_devices_exists(1) == 0)
	{
		usb_b_exist = 1;
		on_flag = 1;
	}
	else
	{
	  usb_b_exist = 0;
		on_flag = 0;
	}
	
	if (usb_a_exist || usb_b_exist) {
		while(1) {
				if(on_flag == 0)
				{
					usbset(0,USB_CMD_ON);
					on_flag = 1;
					usleep(500000);	
				}
				else
				{
					checkflag = 0;
					if(usb_a_exist)
						checkflag |= usbcheck(0);
					if(usb_b_exist)
						checkflag |= usbcheck(1);
					if (checkflag == 0)
					{ 
						usbset(0,USB_CMD_OFF);
						on_flag = 0;
					}	
					sleep(1);
				}					
					
		}
	}
	else
	{
		usbset(0,USB_CMD_OFF);
	}
	
}
	













	
