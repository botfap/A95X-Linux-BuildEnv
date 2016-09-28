/* main.c */  

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <dirent.h>
#include "porting.h"

#define LOG_TAG "USBtestpm_mx_iddq"

#include "usbctrl_mx_iddq.h"


int usbcheck(int index) 
{
	int rc = 0;
	rc = usbpower_mx_iddq(index,USB_CMD_IF);
	return rc;
}

int usbset(int index,int cmd) 
{
	int rc = 0;
	rc = usbpower_mx_iddq(index,cmd);
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

int main(int argc,  char * argv[])
{   
	int usb_a_exist = 0;
	int usb_b_exist = 0;
	int a_on_flag,b_on_flag;
	int ret;
	char * parastr=NULL;
	int para_a=1,para_b=1;

	parastr = argv[1];

	if(parastr!=NULL)
	{
		if(strcmp(parastr, "AB")==0)
		{
			para_a = 1;
			para_b = 1;
		}
		else if(strcmp(parastr, "A")==0)
		{
			para_a = 1;
			para_b = 0;
		}
		else if(strcmp(parastr, "B")==0)
		{
			para_a = 0;
			para_b = 1;
		}
	}
	ret = get_dwc_driver_version();

	if(ret == -1)
	{
		printf("This dwc_otg version not support. Please check!\n");
		return -1;
	}
	
	if(para_a && (check_usb_devices_exists(0) == 0))
	{
		usb_a_exist = 1;
		a_on_flag = 1;
	}
	else
		usbset(0,USB_CMD_OFF);
	
	if (para_b &&(check_usb_devices_exists(1) == 0))
	{
		usb_b_exist = 1;
		b_on_flag = 1;
	}
	else
		usbset(1,USB_CMD_OFF);	
	
	if (usb_a_exist || usb_b_exist) {
		while(1) {
			if (usb_a_exist) {
				if(a_on_flag == 0)
				{
					usbset(0,USB_CMD_ON);
					a_on_flag = 1;	
					usleep(500000);
					if (usbcheck(0) != 1)
					{ 
						usbset(0,USB_CMD_OFF);
						a_on_flag = 0;
					}	
				}
				else
				{
					if (usbcheck(0) != 1)
					{ 
						usbset(0,USB_CMD_OFF);
						a_on_flag = 0;
					}	
				}					
			}
			
			if (usb_b_exist) {
				if(b_on_flag == 0)
				{
					usbset(1,USB_CMD_ON);
					b_on_flag = 1;
					usleep(500000);
					if (usbcheck(1) != 1)
					{ 
						usbset(1,USB_CMD_OFF);
						b_on_flag = 0;
					}
				}
				else
				{
					if (usbcheck(1) != 1)
					{ 
						usbset(1,USB_CMD_OFF);
						b_on_flag = 0;
					}
				}						
			}
			
			usleep(500000);	//
		
		}
	}
	
}
	













	
