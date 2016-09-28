
#define LOG_TAG "amavutils"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include "include/Amdisplayutils.h"


#define FB_DEVICE_PATH   "/sys/class/graphics/fb0/virtual_size"
#define SCALE_AXIS_PATH  "/sys/class/graphics/fb0/scale_axis"
#define SCALE_PATH       "/sys/class/graphics/fb0/scale"
#define SCALE_REQUEST 	 "/sys/class/graphics/fb0/request2XScale"
#define OSD_ROTATION_PATH "/sys/class/graphics/fb0/prot_angle"
#define SYSCMD_BUFSIZE 40

#ifndef LOGD
    #define LOGV ALOGV
    #define LOGD ALOGD
    #define LOGI ALOGI
    #define LOGW ALOGW
    #define LOGE ALOGE
#endif

//#define LOG_FUNCTION_NAME LOGI("%s-%d\n",__FUNCTION__,__LINE__);
#define LOG_FUNCTION_NAME

void get_display_mode(char *mode)
{
    int fd;
    char *path = "/sys/class/display/mode";
    if (!mode) {
        LOGE("[get_display_mode]Invalide parameter!");
        return;
    }
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(mode, 0, 16); // clean buffer and read 15 byte to avoid strlen > 15	
        read(fd, mode, 15);
        LOGI("[get_display_mode]mode=%s strlen=%d\n", mode, strlen(mode));
        mode[strlen(mode)] = '\0';
        close(fd);
    } else {
        sprintf(mode, "%s", "fail");
    };
    LOGI("[get_display_mode]display_mode=%s\n", mode);
    return ;
}
int amdisplay_utils_get_size(int *width, int *height)
{
    LOG_FUNCTION_NAME
    char buf[SYSCMD_BUFSIZE];
    int disp_w = 0;
    int disp_h = 0;
    int ret;
    ret = amsysfs_get_sysfs_str(FB_DEVICE_PATH, buf, SYSCMD_BUFSIZE);
    if (ret < 0) {
        return ret;
    }
    if (sscanf(buf, "%d,%d", &disp_w, &disp_h) == 2) {
        LOGI("disp resolution %dx%d\n", disp_w, disp_h);
        disp_h = disp_h / 2;
    } else {
        return -2;/*format unknow*/
    }
    *width = disp_w;
    *height = disp_h;
    return 0;
}

#define FB_DEVICE_PATH_FB2   "/sys/class/graphics/fb2/virtual_size"
int amdisplay_utils_get_size_fb2(int *width, int *height)
{
    LOG_FUNCTION_NAME
    char buf[SYSCMD_BUFSIZE];
    int disp_w = 0;
    int disp_h = 0;
    int ret;
    ret = amsysfs_get_sysfs_str(FB_DEVICE_PATH_FB2, buf, SYSCMD_BUFSIZE);
    if (ret < 0) {
        return ret;
    }
    if (sscanf(buf, "%d,%d", &disp_w, &disp_h) == 2) {
        LOGI("disp resolution %dx%d\n", disp_w, disp_h);
        disp_h = disp_h / 2;
    } else {
        return -2;/*format unknow*/
    }
    *width = disp_w;
    *height = disp_h;
    return 0;
}

int amdisplay_utils_set_scale_mode(int scale_wx, int scale_hx)
{
    int width, height;
    int ret;
    int neww, newh;
    char buf[40];

    /*scale mode only support x2,x1*/
    if ((scale_wx != 1 && scale_wx != 2) || (scale_hx != 1 && scale_hx != 2)) {
        LOGI("unsupport scaling mode,x1,x2 only\n", scale_wx, scale_hx);
        return -1;
    }
    
    if(scale_wx==2)
        ret = amsysfs_set_sysfs_str(SCALE_REQUEST, "1");
    else if(scale_wx==1)
        ret = amsysfs_set_sysfs_str(SCALE_REQUEST, "2");   
  
    if (ret < 0) {
        LOGI("set [%s]=[%s] failed\n", SCALE_AXIS_PATH, buf);
        return -2;
    }
    
    return ret;
}


int amdisplay_utils_get_osd_rotation()
{
    char buf[40];
    int ret;
    ret = amsysfs_get_sysfs_str(OSD_ROTATION_PATH, buf, SYSCMD_BUFSIZE);
    if(ret < 0)
        return 0;//no rotation

    int rotation = 0;
    if (sscanf(buf, "osd_rotate:%d", &rotation) == 1) {
        LOGI("get osd rotation  %d\n", rotation);
    }

    switch (rotation) {
        case 0:
            rotation = 0;
            break;
        case 1:
            rotation = 90;
            break;
        case 2:
            rotation = 270;
            break;
        default:
            break;
    }

    LOGD("amdisplay_utils_get_osd_rotation return %d",rotation);
    return rotation;
}


