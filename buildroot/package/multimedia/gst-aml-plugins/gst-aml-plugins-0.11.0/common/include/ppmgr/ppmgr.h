/**
* @file ppmgr.h
* @brief  Porting from ppmgr driver for codec ioctl commands
* @author Tim Yao <timyao@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/

#ifndef PPMGR_H
#define PPMGR_H

#define PPMGR_IOC_MAGIC  'P'
//#define PPMGR_IOC_2OSD0		_IOW(PPMGR_IOC_MAGIC, 0x00, unsigned int)
//#define PPMGR_IOC_ENABLE_PP _IOW(PPMGR_IOC_MAGIC,0X01,unsigned int)
//#define PPMGR_IOC_CONFIG_FRAME  _IOW(PPMGR_IOC_MAGIC,0X02,unsigned int)
#define PPMGR_IOC_GET_ANGLE  _IOR(PPMGR_IOC_MAGIC,0X03,unsigned long)
#define PPMGR_IOC_SET_ANGLE  _IOW(PPMGR_IOC_MAGIC,0X04,unsigned long)

#endif /* PPMGR_H */

