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

#define LOG_TAG "USBPower"

#include "usbctrl.h"

static void usage(void)
{
    printf("usbpower USAGE:\n");
    printf("usbpower <portindex> <cmd> \n");
		printf("        index: A or B ; state: on/off/if(find device exist)\n");
    printf("for example: usbpower A	on\n");
    exit(1);
}

int main(int argc,  char * argv[])
{   
	int usagedisplay = 1;
	char * idxstr=NULL;
	char * statestr=NULL;
	char * ver_buf = NULL;
	int index,cmd;
	int ret=0;
	int i;
	FILE *fpv;

	if(argc<3)
	{
		usage();
	}
	else
	{
		idxstr = argv[1];
		statestr = argv[2];
	}

	ret = get_dwc_driver_version();

	if(ret == -1)
	{
		printf("This dwc_otg version not support. Please check!\n");
		return -1;
	}

	for(i=0;i<USB_IDX_MAX;i++)
	{
		if(strcmp(idxstr, usb_index_str[i])==0)
		{
			index = i;
			break;
		}
	}

	if(i>=USB_IDX_MAX)
	{
		SLOGE("idxstr must A or B\n");
		return -1;
	}
	
	for(i=0;i<USB_CMD_MAX;i++)
	{
		if(strcmp(statestr, usb_state_str[i]) == 0)
		{
			cmd = i;
			break;
		}
	}

	if(i>=USB_CMD_MAX)
	{
		SLOGE("statestr must on off or if\n");
		return -1;
	}
	
	ret = usbpower(index,cmd);

	if (ret == -1) {
        usage();
    	}

	return ret;
}
