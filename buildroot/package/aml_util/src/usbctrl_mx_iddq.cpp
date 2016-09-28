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

#define LOG_TAG "USBPower_mx_iddq"

#include "usbctrl_mx_iddq.h"

#define TOLOWER(x) ((x) | 0x20)

//driver support 0-kernel2.6 1-kernel3.0(before M6) 2-kernel3.0(M6&later)
#define DWC_DRIVER_1			0
#define DWC_DRIVER_2			1
#define DWC_DRIVER_3			2
#define DWC_DRIVER_MAX			3

#define DWC_DRIVER_VERSION_1	"2.60a"
#define DWC_DRIVER_VERSION_2  	"2.20a"
#define DWC_DRIVER_VERSION_3  	"2.94a"
#define DWC_DRIVER_VERSION_LEN	5

#define DWC_OTG_VERSION_DIR	"/sys/bus/logicmodule/drivers/dwc_otg/version"

#define IDX_ATTR_FILENAME	"/sys/devices/platform/usb_phy_control/index"

#define POWER_ATTR_FILENAME_1		"/sys/devices/platform/usb_phy_control/por"
#define POWER_ATTR_FILENAME_2		"/sys/devices/lm0/peri_iddq"
#define POWER_ATTR_FILENAME_3		"/sys/devices/lm0/peri_iddq"
//#define POWER_ATTR_FILENAME_2		"/sys/devices/lm0/peri_power"
//#define POWER_ATTR_FILENAME_3		"/sys/devices/lm0/peri_power"

#define OTG_DISABLE_FILE_NAME_1	"/sys/devices/platform/usb_phy_control/otgdisable"
#define OTG_DISABLE_FILE_NAME_2	"/sys/devices/lm0/peri_otg_disable"
#define OTG_DISABLE_FILE_NAME_3	"/sys/devices/lm0/peri_otg_disable"

#define CONNECT_STR			"Bus Connected = 0x"
#define CONNECT_FILE_NAME	"/sys/devices/lm0/busconnected"
#define PULLUP_FILE_NAME	"/sys/devices/lm0/pullup"

#define GOTGCTL_STR			"GOTGCTL = 0x"
#define GOTGCTL_FILE_NAME	"/sys/devices/lm0/gotgctl"


#define USB_ID			16
#define USB_ID_HOST		0x0
#define USB_ID_DEVICE	0x1

#define USB_SES			18
#define USB_SES_VALID	0x3

int dwc_driver_version=-1;   //if dwc_driver_version == -1(not support) 0--2(driver version)
char dwc_driver_version_str[DWC_DRIVER_MAX][DWC_DRIVER_VERSION_LEN+1] = 
{
			DWC_DRIVER_VERSION_1,
			DWC_DRIVER_VERSION_2,
			DWC_DRIVER_VERSION_3
};

char power_attr_filename_str[DWC_DRIVER_MAX][64] = 
{
			POWER_ATTR_FILENAME_1,
			POWER_ATTR_FILENAME_2,
			POWER_ATTR_FILENAME_3
};

char otg_disable_filename_str[DWC_DRIVER_MAX][64]=
{
			OTG_DISABLE_FILE_NAME_1,
			OTG_DISABLE_FILE_NAME_2,
			OTG_DISABLE_FILE_NAME_3
};

char usb_index_str[USB_IDX_MAX][2]=
{
	"A","B"
};

char usb_state_str[USB_CMD_MAX][8]=
{
	"on","off","if"
};

char usb_state_val[USB_CMD_MAX][2]=
{
	"0","1","2"
};
char pullup_filename_str[32];

static void usage(void)
{
    printf("usbpower USAGE:\n");
    printf("usbpower <portindex> <cmd> \n");
		printf("        index: A or B ; state: on/off/if(find device exist)\n");
    printf("for example: usbpower A	on\n");
    exit(1);
}

static int get_device_if(int idx)
{
	int ret = 0;
	int err = 0;
	char line[32];
	char filename[32];
	FILE *fp;
	unsigned int busconnect,gotgctl;

	strcpy(filename,GOTGCTL_FILE_NAME);
	filename[15] = idx + '0';

	if((fp = fopen(filename,"r"))) {
		if (fgets(line, 32, fp)) {
			if (!strncmp(line, GOTGCTL_STR, strlen(GOTGCTL_STR))) {
				sscanf(line+strlen(GOTGCTL_STR),"%x",&gotgctl);
			}
			else
			{
				SLOGE("gotgctl txt is error\n");
				err =1;
			}
		} else {
			SLOGE("Failed to read gotgctl\n");
			err=2;
		}

		fclose(fp);
	} else {
		//SLOGW("No usb device\n");
		err=3;
	}
	
	if(err)
		return 0;
	
	strcpy(filename,CONNECT_FILE_NAME);
	filename[15] = idx + '0';

	if((fp = fopen(filename,"r"))) {
		if (fgets(line, 32, fp)) {
			if (!strncmp(line, CONNECT_STR, strlen(CONNECT_STR))) {
				sscanf(line+strlen(CONNECT_STR),"%x",&busconnect);
      			}else{
				SLOGE("busconnected txt is error\n");
				err=4;
			}
		} else {
			SLOGE("Failed to read busconnected\n");
			err=5;
		}

		fclose(fp);
	} else {
		//SLOGW("No usb device\n");
		err=6;
	}

	if(err)
		return 0;
		
	if(((gotgctl>>USB_ID)&0x1)==	USB_ID_HOST)
	{
		// this case,check busconnected
		if(busconnect==1)
			ret = 1;
	}
	else
	{
		// this case,check gotgctl USB_SES_VALID
		if(((gotgctl>>USB_SES)&0x3)== USB_SES_VALID)
			ret = 1;
	}
	return ret;
}

static int set_power_ctl(int idx,int cmd)
{
	int ret = 0;
	int err = 0;
	FILE *fp,*fpp = NULL,*fpo = NULL, *fp_gotgctl;	
  	unsigned int gotgctl;
  	char filename[32];
  	char line[32];
	int version = dwc_driver_version;
  
	if(cmd == USB_CMD_OFF)
	{
		ret = get_device_if(idx);
		if(ret == 1)
		{
			printf("Has devices on this port,can't power off!\n");
			return ret;
		}
	}
	
	strcpy(filename,GOTGCTL_FILE_NAME);
	filename[15] = idx + '0';

	if((fp_gotgctl = fopen(filename,"r"))) {
		if (fgets(line, 32, fp_gotgctl)) {
			if (!strncmp(line, GOTGCTL_STR, strlen(GOTGCTL_STR))) {
				sscanf(line+strlen(GOTGCTL_STR),"%x",&gotgctl);
			}
			else
			{
				SLOGE("gotgctl txt is error\n");
				err =1;
			}
		} else {
			SLOGE("Failed to read gotgctl\n");
			err=2;
		}

		fclose(fp_gotgctl);
	} else {
		//SLOGW("No usb device\n");
		err=3;
	}
	
	if(err)
		return -1;

	if(version == DWC_DRIVER_1)
	{
		if((fp = fopen(IDX_ATTR_FILENAME,"w"))){   
		    	fwrite(usb_index_str[idx], 1, strlen(usb_index_str[idx]),fp);
		    	fclose(fp);
		}
		else
		{
			ret = -1;
		}
		
		if(ret == 0)
		{
			if(((gotgctl>>USB_ID)&0x1)==	USB_ID_HOST)
			{
				if((fp = fopen(power_attr_filename_str[version],"w"))){
					fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fp);	//power	
				}
				else
				{
					ret = -2;
				}
				if(fp) fclose(fp);
			}
			else
			{
				strcpy(pullup_filename_str,PULLUP_FILE_NAME);
				pullup_filename_str[15] = idx + '0';
			  	if((fp = fopen(power_attr_filename_str[version],"w")) && (fpp = fopen(pullup_filename_str,"w"))
					 && (fpo = fopen(otg_disable_filename_str[version],"w"))){ 
					if(cmd == USB_CMD_OFF)
					{
					 	fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fpo);	//otg		
				    		fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fpp);	//pullup
				    		fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fp);	//power
					}
					else
					{ 
					  	fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fp);	//power 				
				    		fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fpo);	//otg		
				    		fwrite(usb_state_str[cmd], 1, strlen(usb_state_str[cmd]),fpp);	//pullup
				  	}				
			  	}
			  	else
			  	{
				  	ret = -2;
			  	}
			  	if(fp) 	fclose(fp);
	    			if(fpp)	fclose(fpp);
			  	if(fpo)	fclose(fpo);
			}		
		}
	}
	else 
	{
		power_attr_filename_str[version][15] = idx + '0';
		otg_disable_filename_str[version][15]= idx + '0';

		if(((gotgctl>>USB_ID)&0x1)==	USB_ID_HOST)
		{
			if((fp = fopen(power_attr_filename_str[version],"w"))){
				fwrite(usb_state_val[cmd], 1, strlen(usb_state_val[cmd]),fp);	//power	
			}
			else
			{
				ret = -2;
			}
			if(fp) fclose(fp);
		}
		else
		{
		  	if((fp = fopen(power_attr_filename_str[version],"w"))
				 && (fpo = fopen(otg_disable_filename_str[version],"w"))){ 
				if(cmd == USB_CMD_OFF)
				{
				 	fwrite(usb_state_val[cmd], 1, strlen(usb_state_val[cmd]),fpo);	//otg		
			    		fwrite(usb_state_val[cmd], 1, strlen(usb_state_val[cmd]),fp);	//power
				}
				else
				{ 
				  	fwrite(usb_state_val[cmd], 1, strlen(usb_state_val[cmd]),fp);	//power 				
			    		fwrite(usb_state_val[cmd], 1, strlen(usb_state_val[cmd]),fpo);	//otg		
			  	}				
		  	}
		  	else
		  	{
			  	ret = -2;
		  	}
		  	if(fp) 	fclose(fp);
		  	if(fpo)	fclose(fpo);
		}	
	}
	
	return ret;
}

char ver_buf[DWC_DRIVER_VERSION_LEN + 1];
int get_dwc_driver_version(void)
{
	FILE *fpv;
	int i;

	dwc_driver_version = -1;
	
	fpv = fopen(DWC_OTG_VERSION_DIR,"r");
	if(fpv){
		if(fread(ver_buf,1,DWC_DRIVER_VERSION_LEN + 1,fpv) > 0){
			for(i=DWC_DRIVER_1;i<DWC_DRIVER_MAX;i++){			
				if(strncmp(ver_buf,dwc_driver_version_str[i],DWC_DRIVER_VERSION_LEN)==0){
					dwc_driver_version = i;
					break;
				}
			}
		}
		fclose(fpv);
	}

   	if(dwc_driver_version==-1){
		//used other process for the version different from DWC_DRIVER_VERSION
		return -1;
	}

	if(dwc_driver_version == DWC_DRIVER_2)
	{
		//now we have not supported this version.It will be supported later.
		return -1;
	}
	return 0;
}

int usbpower_mx_iddq(int index,int cmd)
{   
	int ret=0;
		
	if((cmd==USB_CMD_ON)||(cmd==USB_CMD_OFF))
	{
		ret = set_power_ctl(index,cmd);
	}
	else if(cmd == USB_CMD_IF)
	{
		ret = get_device_if(index);
#if 0
		if(ret == 1 )
			printf("Has Device\n");
		else
			printf("No device\n");
#endif
	}

	return ret;
}


