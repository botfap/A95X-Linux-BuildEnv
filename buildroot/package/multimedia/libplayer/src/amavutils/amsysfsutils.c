
#define LOG_TAG "amavutils"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include "include/Amsysfsutils.h"
#include <Amsyswrite.h>

#ifndef LOGD
    #define LOGV ALOGV
    #define LOGD ALOGD
    #define LOGI ALOGI
    #define LOGW ALOGW
    #define LOGE ALOGE
#endif


//#define USE_SYSWRITE
#ifndef USE_SYSWRITE
int amsysfs_set_sysfs_str(const char *path, const char *val)
{
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}
int  amsysfs_get_sysfs_str(const char *path, char *valstr, int size)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
		memset(valstr,0,size);
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
        sprintf(valstr, "%s", "fail");
        return -1;
    };
    //LOGI("get_sysfs_str=%s\n", valstr);
    return 0;
}

int amsysfs_set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

int amsysfs_get_sysfs_int(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    }else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return val;
}

int amsysfs_set_sysfs_int16(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "0x%x", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    
    return -1;
}

int amsysfs_get_sysfs_int16(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return val;
}

unsigned long amsysfs_get_sysfs_ulong(const char *path)
{
	int fd;
	char bcmd[24]="";
	unsigned long num=0;
	if ((fd = open(path, O_RDONLY)) >=0) {
    	read(fd, bcmd, sizeof(bcmd));
    	num = strtoul(bcmd, NULL, 0);
    	close(fd);
	} else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
	return num;
}
#else
int amsysfs_set_sysfs_str(const char *path, const char *val)
{
    return amSystemWriteWriteSysfs(path, val);
}
int  amsysfs_get_sysfs_str(const char *path, char *valstr, int size)
{
    if(amSystemWriteReadNumSysfs(path, valstr, size) != -1) {
        return 0;
    }
    sprintf(valstr, "%s", "fail");
    return -1;
}

int amsysfs_set_sysfs_int(const char *path, int val)
{
    char  bcmd[16] = "";
    sprintf(bcmd,"%d",val);
    return amSystemWriteWriteSysfs(path, bcmd);
}

int amsysfs_get_sysfs_int(const char *path)
{
    char  bcmd[16]= "";
    int val = 0;
    if(amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtol(bcmd, NULL, 10);
    }
    return val;
}

int amsysfs_set_sysfs_int16(const char *path, int val)
{
    char  bcmd[16]= "";
    sprintf(bcmd, "0x%x", val);
    return amSystemWriteWriteSysfs(path, bcmd);
}

int amsysfs_get_sysfs_int16(const char *path)
{
    char  bcmd[16]= "";
    int val = 0;
    if(amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtol(bcmd, NULL, 16);
    }
    return val;
}

unsigned long amsysfs_get_sysfs_ulong(const char *path)
{
    char  bcmd[24]= "";
    int val = 0;
    if(amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtoul(bcmd, NULL, 0);
    }
    return val;
}

#endif

