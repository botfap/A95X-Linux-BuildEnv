/****************************************
 * file: player_set_disp.c
 * description: set disp attr when playing
****************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <log_print.h>
#include <player_type.h>
#include <player_set_sys.h>
#include <linux/fb.h>
#include <sys/system_properties.h>
#include <Amsysfsutils.h>

static freescale_setting_t freescale_setting[] = {
    {
        DISP_MODE_480P,
        {0, 0, 800, 480, 0, 0, 18, 18},
        {0, 0, 800, 480, 0, 0, 18, 18},
        {20, 10, 700, 470},
        800,
        480,
        800,
        480
    },
    {
        DISP_MODE_720P,
        {240, 120, 800, 480, 240, 120, 18, 18},
        {0, 0, 800, 480, 0, 0, 18, 18},
        {40, 15, 1240, 705},
        800,
        480,
        800,
        480
    },
    {
        DISP_MODE_1080I,
        {560, 300, 800, 480, 560, 300, 18, 18},
        {0, 0, 800, 480, 0, 0, 18, 18},
        {40, 20, 1880, 1060},
        800,
        480,
        800,
        480
    },
    {
        DISP_MODE_1080P,
        {560, 300, 800, 480, 560, 300, 18, 18},
        {0, 0, 800, 480, 0, 0, 18, 18},
        {40, 20, 1880, 1060},
        800,
        480,
        800,
        480
    }
};

int set_sysfs_str(const char *path, const char *val)
{
    return amsysfs_set_sysfs_str(path, val);
}
int  get_sysfs_str(const char *path, char *valstr, int size)
{
    return amsysfs_get_sysfs_str(path, valstr, size);
}

int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}
int get_sysfs_int(const char *path)
{
    return amsysfs_get_sysfs_int16(path);
}


int set_black_policy(int blackout)
{
    return set_sysfs_int("/sys/class/video/blackout_policy", blackout);
}

int get_black_policy()
{
    return get_sysfs_int("/sys/class/video/blackout_policy") & 1;
}

int check_audiodsp_fatal_err()
{
    int fatal_err = 0;
    int fd = -1;
    int val = 0;
    char  bcmd[16];
    fd = open("/sys/class/audiodsp/codec_fatal_err", O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
        fatal_err = val & 0xf;
        if (fatal_err != 0) {
            log_print("[%s]get audio dsp error:%d!\n", __FUNCTION__, fatal_err);
        }
    } else
    {
        log_print("unable to open file check_audiodsp_fatal_err,err: %s", strerror(errno));
    }

    return fatal_err;
}

int check_audio_output()
{
    return get_sysfs_int("/sys/class/amaudio/output_enable") & 3;
}

int get_karaok_flag()
{
    return get_sysfs_int("/sys/class/amstream/karaok") & 1;
}

int set_tsync_enable(int enable)
{
    return set_sysfs_int("/sys/class/tsync/enable", enable);

}
int get_tsync_enable(void)
{
    char buf[32];
    int ret = 0;
    int val = 0;
    ret = get_sysfs_str("/sys/class/tsync/enable", buf, 32);
    if (!ret) {
        sscanf(buf, "%d", &val);
    }
    return val == 1 ? val : 0;
}
int set_tsync_discontinue(int discontinue)      //kernel set to 1,player clear to 0
{
    return set_sysfs_int("/sys/class/tsync/discontinue", discontinue);

}
int get_pts_discontinue()
{
    return get_sysfs_int("/sys/class/tsync/discontinue") & 1;
}

int set_subtitle_num(int num)
{
    return set_sysfs_int("/sys/class/subtitle/total", num);

}
int set_subtitle_index(int num)
{
    return set_sysfs_int("/sys/class/subtitle/index", num);
}
int set_subtitle_curr(int num)
{
    return set_sysfs_int("/sys/class/subtitle/curr", num);
}
int set_subtitle_enable(int num)
{
    return set_sysfs_int("/sys/class/subtitle/enable", num);

}

int set_subtitle_fps(int fps)
{
    return  set_sysfs_int("/sys/class/subtitle/fps", fps);

}

int set_subtitle_subtype(int subtype)
{
    return  set_sysfs_int("/sys/class/subtitle/subtype", subtype);
}

int av_get_subtitle_curr()
{
    return get_sysfs_int("/sys/class/subtitle/curr");
}

int set_subtitle_startpts(int pts)
{
    return  set_sysfs_int("/sys/class/subtitle/startpts", pts);
}

int set_fb0_blank(int blank)
{
    return  set_sysfs_int("/sys/class/graphics/fb0/blank", blank);
}

int set_fb1_blank(int blank)
{
    return  set_sysfs_int("/sys/class/graphics/fb1/blank", blank);
}

static int get_last_file(char *filename, int nsize)
{
    return get_sysfs_str("/sys/class/video/file_name", filename, nsize);
}

static int set_last_file(char *filename)
{
    return set_sysfs_str("/sys/class/video/file_name", filename);
}

int check_file_same(char *filename2)
{
    char filename1[512];
    int len1 = 0, len2 = 0;
    int ret = -1;
    memset(filename1, 0, 512);

    ret = get_last_file(filename1, 511);
    if (ret != 0) { //maybe invalid sysfs file handle
        log_print("invalid sysfs handle:%s,drop check file policy\n", "/sys/class/video/file_name");
        return 1;
    }
    len1 = strlen(filename1);
    len2 = strlen(filename2);

    log_print("[%s]file1=%d:%s\n", __FUNCTION__, len1 , filename1);
    log_print("[%s]file2=%d:%s\n", __FUNCTION__, len2 , filename2);

    set_last_file(filename2);

    if (((len1 - 1) != len2) && (len1 < 512)) {
        log_print("[%s]not match,1:%d 2:%d\n", __FUNCTION__, len1, len2);
        return 0;
    } else if (!strncmp(filename1, filename2, len2)) {
        log_print("[%s]match,return 1\n", __FUNCTION__);
        return 1;
    }
    return 0;
}



void get_display_mode(char *mode)
{

    char *path = "/sys/class/display/mode";
    if (!mode) {
        log_error("[get_display_mode]Invalide parameter!");
        return;
    }
    memset(mode, 0, 16); // clean buffer and read 15 byte to avoid strlen > 15

    get_sysfs_str(path, mode, 16);
    
    log_print("[get_display_mode]display_mode=%s\n", mode);
    return ;
}
void get_display_mode2(char *mode)
{
    int fd;
    char *path = "/sys/class/display2/mode";
    if (!mode) {
        log_error("[get_display_mode]Invalide parameter!");
        return;
    }
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(mode, 0, 16); // clean buffer and read 15 byte to avoid strlen > 15	
        read(fd, mode, 15);
        log_print("[get_display_mode]mode=%s strlen=%d\n", mode, strlen(mode));
        mode[strlen(mode)] = '\0';
        close(fd);
    } else {
        sprintf(mode, "%s", "fail");
    };
    log_print("[get_display_mode]display_mode=%s\n", mode);
    return ;
}



int set_fb0_freescale(int freescale)
{
    return  set_sysfs_int("/sys/class/graphics/fb0/free_scale", freescale);
}

int set_fb1_freescale(int freescale)
{
    return  set_sysfs_int("/sys/class/graphics/fb1/free_scale", freescale);

}

int set_display_axis(int *coordinate)
{

    char *path = "/sys/class/display/axis" ;
    char  bcmd[32];
    int x00, x01, x10, x11, y00, y01, y10, y11;
    if (coordinate) {
        x00 = coordinate[0];
        y00 = coordinate[1];
        x01 = coordinate[2];
        y01 = coordinate[3];
        x10 = coordinate[4];
        y10 = coordinate[5];
        x11 = coordinate[6];
        y11 = coordinate[7];
        sprintf(bcmd, "%d %d %d %d %d %d %d %d", x00, y00, x01, y01, x10, y10, x11, y11);
        return set_sysfs_str(path, bcmd);
    }
    return -1;
}

int set_video_axis(int *coordinate)
{

    char *path = "/sys/class/video/axis" ;
    char  bcmd[32];
    int x0, y0, x1, y1;
    if (coordinate) {
        x0 = coordinate[0];
        y0 = coordinate[1];
        x1 = coordinate[2];
        y1 = coordinate[3];
        sprintf(bcmd, "%d %d %d %d", x0, y0, x1, y1);
        return set_sysfs_str(path, bcmd);
    }
    return -1;
}

int set_fb0_scale_width(int width)
{
    char *path = "/sys/class/graphics/fb0/scale_width" ;
    return set_sysfs_int(path, width);
}
int set_fb0_scale_height(int height)
{
    char *path = "/sys/class/graphics/fb0/scale_height" ;
    return set_sysfs_int(path, height);
}
int set_fb1_scale_width(int width)
{
    char *path = "/sys/class/graphics/fb1/scale_width" ;
    return set_sysfs_int(path, width);
}
int set_fb1_scale_height(int height)
{
    char *path = "/sys/class/graphics/fb1/scale_height" ;
    return set_sysfs_int(path, height);
}

static int display_mode_convert(char *disp_mode)
{
    int ret = 0xff;
    log_print("[display_mode_convert]disp_mode=%s\n", disp_mode);
    if (!disp_mode) {
        ret = 0xeeee;
    } else if ((!strncmp(disp_mode,"480i", 4))||(!strncmp(disp_mode, "480cvbs", 7))) {
        ret = DISP_MODE_480I;
    } else if (!strncmp(disp_mode, "480p", 4)) {
        ret = DISP_MODE_480P;
    } else if ((!strncmp(disp_mode,"576i", 4))||(!strncmp(disp_mode, "576cvbs", 7))) {
        ret = DISP_MODE_576I;
    } else if (!strncmp(disp_mode, "576p", 4)) {
        ret = DISP_MODE_576P;
    } else if (!strncmp(disp_mode, "720p", 4)) {
        ret = DISP_MODE_720P;
    } else if (!strncmp(disp_mode, "1080i", 5)) {
        ret = DISP_MODE_1080I;
    } else if (!strncmp(disp_mode, "1080p", 5)) {
        ret = DISP_MODE_1080P;
    } else {
        ret = 0xffff;
    }
    log_print("[display_mode_convert]disp_mode=%s-->%x\n", disp_mode, ret);
    return ret;
}
//////////////////////////////////////////////
static void get_display_axis()
{
    char *path = "/sys/class/display/axis";
    char  bcmd[32];
    if (get_sysfs_str(path, bcmd, 32) != -1) {
        log_print("[get_disp_axis]%s\n", bcmd);
    } else {
        log_error("[%s:%d]open %s failed!\n", __FUNCTION__, __LINE__, path);
    }
}
static void get_video_axis()
{
    char *path = "/sys/class/video/axis";
    char  bcmd[32];
    if (get_sysfs_str(path, bcmd, 32) != -1) {
        log_print("[get_video_axis]%sn", bcmd);
    } else {
        log_error("[%s:%d]open %s failed!\n", __FUNCTION__, __LINE__, path);
    }
}
//////////////////////////////////////////////
struct fb_var_screeninfo vinfo;
void update_freescale_setting(void)
{
    int fd_fb;
    int osd_width, osd_height;
    int i;
    int num = sizeof(freescale_setting) / sizeof(freescale_setting[0]);

    if ((fd_fb = open("/dev/graphics/fb0", O_RDWR)) < 0) {
        log_error("open /dev/graphics/fb0 fail.");
        return;
    }

    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &vinfo) == 0) {
        osd_width = vinfo.xres;
        osd_height = vinfo.yres;
        log_print("osd_width = %d", osd_width);
        log_print("osd_height = %d", osd_height);

        for (i = 0; i < num; i ++) {
            if (freescale_setting[i].disp_mode == DISP_MODE_480P) {
                //freescale_setting[i].osd_disble_coordinate[0] = 0;
                //freescale_setting[i].osd_disble_coordinate[1] = 0;
                freescale_setting[i].osd_disble_coordinate[2] = osd_width;
                freescale_setting[i].osd_disble_coordinate[3] = osd_height;
                //freescale_setting[i].osd_disble_coordinate[4] = 0;
                //freescale_setting[i].osd_disble_coordinate[5] = 0;
                //freescale_setting[i].osd_disble_coordinate[6] = 18;
                //freescale_setting[i].osd_disble_coordinate[7] = 18;

            } else if (freescale_setting[i].disp_mode == DISP_MODE_720P) {
                freescale_setting[i].osd_disble_coordinate[0] = (1280 - osd_width) / 2;
                freescale_setting[i].osd_disble_coordinate[1] = (720 - osd_height) / 2;
                freescale_setting[i].osd_disble_coordinate[2] = osd_width;
                freescale_setting[i].osd_disble_coordinate[3] = osd_height;
                freescale_setting[i].osd_disble_coordinate[4] = (1280 - osd_width) / 2;
                freescale_setting[i].osd_disble_coordinate[5] = (720 - osd_height) / 2;
                //freescale_setting[i].osd_disble_coordinate[6] = 18;
                //freescale_setting[i].osd_disble_coordinate[7] = 18;

            } else if (freescale_setting[i].disp_mode == DISP_MODE_1080I ||
                       freescale_setting[i].disp_mode == DISP_MODE_1080P) {
                freescale_setting[i].osd_disble_coordinate[0] = (1920 - osd_width) / 2;
                freescale_setting[i].osd_disble_coordinate[1] = (1080 - osd_height) / 2;
                freescale_setting[i].osd_disble_coordinate[2] = osd_width;
                freescale_setting[i].osd_disble_coordinate[3] = osd_height;
                freescale_setting[i].osd_disble_coordinate[4] = (1920 - osd_width) / 2;
                freescale_setting[i].osd_disble_coordinate[5] = (1080 - osd_height) / 2;
                //freescale_setting[i].osd_disble_coordinate[6] = 18;
                //freescale_setting[i].osd_disble_coordinate[7] = 18;

            }

            //freescale_setting[i].osd_enable_coordinate[0] = 0;
            //freescale_setting[i].osd_enable_coordinate[1] = 0;
            freescale_setting[i].osd_enable_coordinate[2] = osd_width;
            freescale_setting[i].osd_enable_coordinate[3] = osd_height;
            //freescale_setting[i].osd_enable_coordinate[4] = 0;
            //freescale_setting[i].osd_enable_coordinate[5] = 0;
            //freescale_setting[i].osd_enable_coordinate[6] = 18;
            //freescale_setting[i].osd_enable_coordinate[6] = 18;

            freescale_setting[i].fb0_freescale_width = osd_width;
            freescale_setting[i].fb0_freescale_height = osd_height;
            freescale_setting[i].fb1_freescale_width = osd_width;
            freescale_setting[i].fb1_freescale_height = osd_height;
        }

    } else {
        log_error("get FBIOGET_VSCREENINFO fail.");
    }

    close(fd_fb);
    return;
}

/*
 * Sync from HdmiSwitch/jni/hdmiswitchjni.c
 */
#define  FBIOPUT_OSD_FREE_SCALE_ENABLE  0x4504
#define  FBIOPUT_OSD_FREE_SCALE_WIDTH   0x4505
#define  FBIOPUT_OSD_FREE_SCALE_HEIGHT  0x4506

struct fb_var_screeninfo vinfo;
char daxis_str[32];

int DisableFreeScale(display_mode mode, const int vpp) {
    
    int fd0 = -1, fd1 = -1;
    int osd_width = 0, osd_height = 0;	
    int ret = -1;

    char* daxis_path = NULL;
    char* ppmgr_path = "/sys/class/ppmgr/ppscaler";
    char* ppmgr_rect_path = "/sys/class/ppmgr/ppscaler_rect";
    char* video_path = "/sys/class/video/disable_video";
    char* vaxis_path = "/sys/class/video/axis";
    log_print("DisableFreeScale: mode=%d vpp=%d ", mode,vpp);

	int isM8 = 0;
	char value[128];
	memset(value, 0 ,128);
	property_get("ro.product.model", value, "1");
	if(strstr(value,"M8"))
	{
		isM8 =1;
		log_print("hi,amplayer DisableFreeScale say hello to new chip M8.");
	}




    if (mode < DISP_MODE_480I || mode > DISP_MODE_1080P)
        return 0;

    if (vpp) {
        
        daxis_path = "/sys/class/display2/axis";

        if((fd0 = open("/dev/graphics/fb2", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb2 fail.");
            goto exit;
        }

        memset(daxis_str,0,32);
        if(ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            
            osd_width = vinfo.xres;
            osd_height = vinfo.yres;
    
            log_print("osd_width = %d", osd_width);
            log_print("osd_height = %d", osd_height);
        } 
        else {
            
            log_print("get FBIOGET_VSCREENINFO fail.");
            goto exit;
        }
        
    } else {        

        daxis_path = "/sys/class/display/axis";

        if((fd0 = open("/dev/graphics/fb0", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb0 fail.");
            goto exit;
        }
        
        if((fd1 = open("/dev/graphics/fb1", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb1 fail.");
            goto exit;	
        }

        memset(daxis_str,0,32);
        if(ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            
            osd_width = vinfo.xres;
            osd_height = vinfo.yres;
    
            //log_print("osd_width = %d", osd_width);
            //log_print("osd_height = %d", osd_height);
        } 
        else {
            log_print("get FBIOGET_VSCREENINFO fail.");
            goto exit;
        }    
        
    }

    switch (mode) {

        case DISP_MODE_480P: //480p
            set_sysfs_str(ppmgr_path, "0");
            set_sysfs_str(video_path, "1");
			if(isM8==0)
				ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            
            sprintf(daxis_str, "0 0 %d %d 0 0 18 18", vinfo.xres, vinfo.yres);
            set_sysfs_str(daxis_path, daxis_str);
            set_sysfs_str(ppmgr_rect_path, "0 0 0 0 1");
            set_sysfs_str(vaxis_path, "0 0 0 0");
            ret = 0;
        break;
        
        case DISP_MODE_720P: //720p
            set_sysfs_str(ppmgr_path, "0");
            set_sysfs_str(video_path, "1");
			if(isM8==0)
           		 ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            if (!vpp) ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1280 > vinfo.xres ? (1280 - vinfo.xres) / 2 : 0,
                    720 > vinfo.yres ? (720 - vinfo.yres) / 2 : 0,
                    vinfo.xres,
                    vinfo.yres,
                    1280 > vinfo.xres ? (1280 - vinfo.xres) / 2 : 0,
                    720 > vinfo.yres ? (720 - vinfo.yres) / 2 : 0);
            set_sysfs_str(daxis_path, daxis_str);
            set_sysfs_str(ppmgr_rect_path, "0 0 0 0 1");
            set_sysfs_str(vaxis_path, "0 0 0 0");
            ret = 0;
        break;
        
        case DISP_MODE_1080I: //1080i
        case DISP_MODE_1080P: //1080p  
            set_sysfs_str(ppmgr_path, "0");
            set_sysfs_str(video_path, "1");
            if(isM8==0)
				ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1920 > vinfo.xres ? (1920 - vinfo.xres) / 2 : 0,
                    1080 > vinfo.yres ? (1080 - vinfo.yres) / 2 : 0,
                    vinfo.xres,
                    vinfo.yres,
                    1920 > vinfo.xres ? (1920 - vinfo.xres) / 2 : 0,
                    1080 > vinfo.yres ? (1080 - vinfo.yres) / 2 : 0);
            set_sysfs_str(daxis_path, daxis_str);
            set_sysfs_str(ppmgr_rect_path, "0 0 0 0 1");
            set_sysfs_str(vaxis_path, "0 0 0 0");
            ret = 0;
        break;
        
    default:
        break;
    }

exit:
    close(fd0);
    close(fd1);
    return ret;;

}

int EnableFreeScale(display_mode mode, const int vpp) {
    
    int fd0 = -1, fd1 = -1;
    int osd_width = 0, osd_height = 0;	
    int ret = -1;

    char* daxis_path = NULL;
    char* ppmgr_path = "/sys/class/ppmgr/ppscaler";
    char* ppmgr_rect_path = "/sys/class/ppmgr/ppscaler_rect";
    char* video_path = "/sys/class/video/disable_video";
    char* vaxis_path = "/sys/class/video/axis";
    log_print("EnableFreeScale: mode=%d", mode);	

	int isM8 = 0;
	char value[128];
	memset(value, 0 ,128);
	property_get("ro.product.model", value, "1");
	if(strstr(value,"M8"))
	{
		isM8 =1;
		log_print("hi,amplayer EnableFreeScale say hello to new chip M8.");
	}


    if (mode < DISP_MODE_480I || mode > DISP_MODE_1080P)
        return 0;
    
    if (vpp) {
        daxis_path = "/sys/class/display2/axis";

        if((fd0 = open("/dev/graphics/fb2", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb2 fail.");
            goto exit;
        }

        memset(daxis_str,0,32);
        if(ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            
            osd_width = vinfo.xres;
            osd_height = vinfo.yres;
            sprintf(daxis_str, "0 0 %d %d 0 0 18 18", vinfo.xres, vinfo.yres);
            
            log_print("osd_width = %d", osd_width);
            log_print("osd_height = %d", osd_height);
        } 
        else {
            log_print("get FBIOGET_VSCREENINFO fail.");
            goto exit;
        }    
    } else {
    
        daxis_path = "/sys/class/display/axis";

        if((fd0 = open("/dev/graphics/fb0", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb0 fail.");
            goto exit;
        }
        
        if((fd1 = open("/dev/graphics/fb1", O_RDWR)) < 0) {
            log_print("open /dev/graphics/fb1 fail.");
            goto exit;	
        }
    
        memset(daxis_str,0,32);
        if(ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            
            osd_width = vinfo.xres;
            osd_height = vinfo.yres;
            sprintf(daxis_str, "0 0 %d %d 0 0 18 18", vinfo.xres, vinfo.yres);

            //log_print("osd_width = %d", osd_width);
            //log_print("osd_height = %d", osd_height);
        } 
        else {
            log_print("get FBIOGET_VSCREENINFO fail.");
            goto exit;
        }
    }

    switch (mode) {
        //log_print("set mid mode=%d", mode);

        case DISP_MODE_480P: //480p
            set_sysfs_str(ppmgr_path, "1");
            set_sysfs_str(video_path, "1");
            if(set_sysfs_str(ppmgr_rect_path, "20 10 700 470 0") == -1) {
            set_sysfs_str(vaxis_path, "20 10 700 470");
            }
            
            set_sysfs_str(daxis_path, daxis_str);
			if(isM8==0)
            	ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            
            if (!vpp)
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_WIDTH, osd_width);
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_HEIGHT, osd_height);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_WIDTH,osd_width);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_HEIGHT,osd_height);  
            
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 1);
            ioctl(fd1, FBIOPUT_OSD_FREE_SCALE_ENABLE, 1);

            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,1);
            
            set_sysfs_str(video_path, "1");

            ret = 0;
        break;
        
        case DISP_MODE_720P: //720p
        
            set_sysfs_str(ppmgr_path, "1");
            set_sysfs_str(video_path, "1");
            if(set_sysfs_str(ppmgr_rect_path, "40 15 1240 705 0") == -1) {
                set_sysfs_str(vaxis_path, "40 15 1240 705");
            }
            
            set_sysfs_str(daxis_path, daxis_str);

			if(isM8==0)
            	ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_WIDTH, osd_width);
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_HEIGHT, osd_height);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_WIDTH,osd_width);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_HEIGHT,osd_height);  

			
			if(isM8==0)
                ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 1);

            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,1);
            
            set_sysfs_str(video_path, "1");

            ret = 0;
        break;
        
        case DISP_MODE_1080I: //1080i
        case DISP_MODE_1080P: //1080p
            set_sysfs_str(ppmgr_path, "1");
            set_sysfs_str(video_path, "1");
            
            if(set_sysfs_str(ppmgr_rect_path, "40 20 1880 1060 0") == -1) {
                set_sysfs_str(vaxis_path, "40 20 1880 1060");
            }
            
            set_sysfs_str(daxis_path, daxis_str);
			
			if(isM8==0)
            	ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 0);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,0);
            
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_WIDTH, osd_width);
            ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_HEIGHT, osd_height);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_WIDTH,osd_width);
            
            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_HEIGHT,osd_height);  

			
			if(isM8==0)
            	ioctl(fd0, FBIOPUT_OSD_FREE_SCALE_ENABLE, 1);

            if (!vpp) 
                ioctl(fd1,FBIOPUT_OSD_FREE_SCALE_ENABLE,1);
            set_sysfs_str(video_path, "1");
            ret = 0;
        break;
        
    default:
        break;
    }

exit:
    close(fd0);
    close(fd1);
    return ret;

}

int disable_freescale(int cfg)
{
#ifndef ENABLE_FREE_SCALE
    log_print("ENABLE_FREE_SCALE not define!\n");
    return 0;
#endif
    char prop2[16];
    int vpp2_freescale = 0;
    if (property_get("ro.vout.dualdisplay4", prop2, "false")
        && strcmp(prop2, "true") == 0) {
        vpp2_freescale = 1;
    }
    
    char mode[16];
    display_mode disp_mode;

    if (vpp2_freescale)
        get_display_mode2(mode);
    else
        get_display_mode(mode);
    if (strncmp(mode, "fail", 4)) { //mode !=fail
        disp_mode = display_mode_convert(mode);
        if (disp_mode >= DISP_MODE_480I && disp_mode <= DISP_MODE_1080P) {
            DisableFreeScale(disp_mode, vpp2_freescale);
        }
    }
    char prop[16];
    if (property_get("ro.vout.dualdisplay2", prop, "false")
        && strcmp(prop, "true") == 0) {
        property_set("rw.vout.scale", "off");
    }
    log_print("[disable_freescale]");
    return 0;
}
int enable_freescale(int cfg)
{
#ifndef ENABLE_FREE_SCALE
    log_print("ENABLE_FREE_SCALE not define!\n");
    return 0;
#endif
    char prop2[16];
    int vpp2_freescale = 0;
    if (property_get("ro.vout.dualdisplay4", prop2, "false")
        && strcmp(prop2, "true") == 0) {
        vpp2_freescale = 1;
    }

    char mode[16];
    display_mode disp_mode;

    if (vpp2_freescale)
        get_display_mode2(mode);
    else
        get_display_mode(mode);
    if (strncmp(mode, "fail", 4)) { //mode !=fail
        disp_mode = display_mode_convert(mode);
        if (disp_mode >= DISP_MODE_480I && disp_mode <= DISP_MODE_1080P) {
            EnableFreeScale(disp_mode, vpp2_freescale);
        }
    }
    char prop[16];
    if (property_get("ro.vout.dualdisplay2", prop, "false")
        && strcmp(prop, "true") == 0) {
        property_set("rw.vout.scale", "on");
    }
    log_print("[enable_freescale]");
    return 0;
}

int disable_freescale_MBX()
{
    char mode[16];
    char m1080scale[8];

    property_get("ro.platform.has.1080scale", m1080scale, "fail");
    if (!strncmp(m1080scale, "fail", 4)) {
        return 0;
    }
    get_display_mode(mode);
    if (strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))) {
        log_print("[disable_freescale_MBX] not freescale mode!\n");
        return 0;
    }
    char* freeScaleOsd0File_path = "/sys/class/graphics/fb0/free_scale";
    char* freeScaleOsd1File_path = "/sys/class/graphics/fb1/free_scale";
    char* ppmgr_path = "/sys/class/ppmgr/ppscaler";
    char* ppmgr_rect_path = "/sys/class/ppmgr/ppscaler_rect";
    
    //set_sysfs_str(ppmgr_path, "0");
    //set_sysfs_str(freeScaleOsd0File_path, "0");
    //set_sysfs_str(freeScaleOsd1File_path, "0");

    return 0;
}

int enable_freescale_MBX()
{
    char mode[16];
    char m1080scale[8];
    char vaxis_x_str[8];
    char vaxis_y_str[8];
    char vaxis_width_str[8];
    char vaxis_height_str[8];
    display_mode disp_mode;
    char vaxis_str[32];
    char ppmgr_rect_str[32];
    int vaxis_x, vaxis_y, vaxis_width, vaxis_height, vaxis_right, vaxis_bottom;

    int ret;
    char buf[32]={0};
    property_get("ro.platform.has.1080scale", m1080scale, "fail");
    if (!strncmp(m1080scale, "fail", 4)) {
        return 0;
    }
    get_display_mode(mode);
    if (strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))) {
        log_print("[enable_freescale_MBX]not freescale mode!\n");
        return 0;
    }
    ret = get_sysfs_str("/sys/class/graphics/fb0/free_scale", buf, 32);
       if((ret>=0) && strstr(buf, "1")){
        log_print("[enable_freescale_MBX] already enabled,no need to set again!\n");
        return 0;
    }
    char* freeScaleOsd0File_path = "/sys/class/graphics/fb0/free_scale";
    char* freeScaleOsd1File_path = "/sys/class/graphics/fb1/free_scale";
    char* axis_path = "/sys/class/video/axis";
    char* ppmgr_path = "/sys/class/ppmgr/ppscaler";
    char* ppmgr_rect_path = "/sys/class/ppmgr/ppscaler_rect";
    
    if (strncmp(mode, "fail", 4)) { //mode !=fail
        disp_mode = display_mode_convert(mode);
        if (disp_mode < DISP_MODE_480I || disp_mode > DISP_MODE_1080P) {
            log_print("display mode fail: %d", disp_mode);
            return 0;
        }
    }
    log_print("[enable_freescale_MBX] display mode: %d", disp_mode);
    switch (disp_mode) {
    case DISP_MODE_480I:
        property_get("ubootenv.var.480ioutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.480ioutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.480ioutputwidth", vaxis_width_str, "720");
        property_get("ubootenv.var.480ioutputheight", vaxis_height_str, "480");
        break;
    case DISP_MODE_480P:
        property_get("ubootenv.var.480poutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.480poutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.480poutputwidth", vaxis_width_str, "720");
        property_get("ubootenv.var.480poutputheight", vaxis_height_str, "480");
        break;
    case DISP_MODE_576I:
        property_get("ubootenv.var.576ioutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.576ioutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.576ioutputwidth", vaxis_width_str, "720");
        property_get("ubootenv.var.576ioutputheight", vaxis_height_str, "576");
        break;
    case DISP_MODE_576P:
        property_get("ubootenv.var.576poutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.576poutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.576poutputwidth", vaxis_width_str, "720");
        property_get("ubootenv.var.576poutputheight", vaxis_height_str, "576");
        break;
    case DISP_MODE_720P:
        property_get("ubootenv.var.720poutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.720poutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.720poutputwidth", vaxis_width_str, "1280");
        property_get("ubootenv.var.720poutputheight", vaxis_height_str, "720");
        break;
    case DISP_MODE_1080I:
        property_get("ubootenv.var.1080ioutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.1080ioutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.1080ioutputwidth", vaxis_width_str, "1920");
        property_get("ubootenv.var.1080ioutputheight", vaxis_height_str, "1080");
        break;
    case DISP_MODE_1080P:
        property_get("ubootenv.var.1080poutputx", vaxis_x_str, "0");
        property_get("ubootenv.var.1080poutputy", vaxis_y_str, "0");
        property_get("ubootenv.var.1080poutputwidth", vaxis_width_str, "1920");
        property_get("ubootenv.var.1080poutputheight", vaxis_height_str, "1080");
        break;
    default:
        break;
    }

    vaxis_x = atoi(vaxis_x_str);
    vaxis_y = atoi(vaxis_y_str);
    vaxis_width = atoi(vaxis_width_str);
    vaxis_height = atoi(vaxis_height_str);
    vaxis_right = vaxis_x + vaxis_width;
    vaxis_bottom = vaxis_y + vaxis_height;

    log_print("[enable_freescale_MBX]set video axis: %d %d %d %d \n", vaxis_x, vaxis_y, vaxis_right, vaxis_bottom);
    sprintf(vaxis_str, "%d %d %d %d", vaxis_x, vaxis_y, vaxis_right, vaxis_bottom);
    sprintf(ppmgr_rect_str, "%d %d %d %d 0", vaxis_x, vaxis_y, vaxis_right, vaxis_bottom);
    //if(fd_ppmgr_rect >= 0)
    //write(fd_ppmgr_rect, ppmgr_rect_str, strlen(ppmgr_rect_str));
    set_sysfs_str(axis_path, vaxis_str);
    
    set_sysfs_str(ppmgr_path, "1");
    set_sysfs_str(freeScaleOsd0File_path, "1");
    set_sysfs_str(freeScaleOsd1File_path, "1");
    log_print("[enable_freescale_MBX] \n");
    log_print("[enable_freescale_MBX] write(videoAxisFile \n");
    log_print("[enable_freescale_MBX] write(freeScaleOsd0File \n");
    log_print("[enable_freescale_MBX] write(freeScaleOsd1File \n");
    return 0;
}

int wait_play_end()
{
    int ret = 0;
    int waitcount = 0;
    char buf[32]={0};
    ret = amsysfs_get_sysfs_str("/sys/class/amstream/videobufused", buf, 32);
    log_print("[wait_Play_end] ret %d buf %s\n",ret,buf);
    while((ret>=0)&&(!strstr(buf, "0"))){
        
        log_print("[wait_Play_end] wait count %d\n",waitcount);
        if(waitcount > 500){
            
            return -1;

        }
        waitcount++;
        usleep(500);
        memset(buf,0,sizeof(buf));
        ret = amsysfs_get_sysfs_str("/sys/class/amstream/videobufused", buf, 32);     	
    } 
    return 0;
}

int disable_2X_2XYscale()
{
    char mode[16];
    char m1080scale[8];

    property_get("ro.platform.has.1080scale", m1080scale, "fail");
    if (!strncmp(m1080scale, "fail", 4)) {
        return 0;
    }
    get_display_mode(mode);
    if (strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))) {
        log_print("[disable_2X_2XYscale]not freescale mode!\n");
        return 0;
    }

    char* scale_path = "/sys/class/graphics/fb0/scale";
    char* scaleOsd1_path = "/sys/class/graphics/fb1/scale";

    set_sysfs_str(scale_path, "0");
    set_sysfs_str(scaleOsd1_path, "0");
    return 0;
}

int enable_2Xscale()
{
    char mode[16];
    char m1080scale[8];
    char saxis_str[32];
    display_mode disp_mode;

    property_get("ro.platform.has.1080scale", m1080scale, "fail");
    if (!strncmp(m1080scale, "fail", 4)) {
        return 0;
    }
    get_display_mode(mode);
    disp_mode = display_mode_convert(mode);
    if (disp_mode < DISP_MODE_1080I || disp_mode > DISP_MODE_1080P) {
        return 0;
    }
    if (strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5) && strncmp(mode, "720p", 4)))) {
        log_print("[enable_2Xscale]not freescale mode!\n");
    } else {
        return 0;
    }

    char* scale_path = "/sys/class/graphics/fb0/scale";
    char* scaleaxis_path = "/sys/class/graphics/fb0/scale_axis";
    
    memset(saxis_str, 0, 32);
    sprintf(saxis_str, "0 0 %d %d", vinfo.xres / 2 - 1, vinfo.yres - 1);

    set_sysfs_str(scaleaxis_path, saxis_str);
    set_sysfs_str(scale_path, "0x10000");
    return 0;
}

int enable_2XYscale()
{
    char mode[16];
    char m1080scale[8];
    int scaleFile = -1, scaleaxisFile = -1, scaleOsd1File = -1, scaleaxisOsd1File = -1;

    property_get("ro.platform.has.1080scale", m1080scale, "fail");
    if (!strncmp(m1080scale, "fail", 4)) {
        return 0;
    }
    get_display_mode(mode);
    if ((strncmp(m1080scale, "2", 1) && strncmp(m1080scale, "1", 1)) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5))) {
        log_print("[enable_2XYscale]not freescale mode!\n");
        return 0;
    }

    char* scale_path = "/sys/class/graphics/fb0/scale";
    char* scale_axis_path = "/sys/class/graphics/fb0/scale_axis";
    char* scaleOsd1_path = "/sys/class/graphics/fb1/scale";
    char* scaleOsd1_axis_path = "/sys/class/graphics/fb1/scale_axis";


    set_sysfs_str(scale_axis_path, "0 0 959 539");
    set_sysfs_str(scaleOsd1_axis_path, "1280 720 1920 1080");
    set_sysfs_str(scale_path, "0x10001");
    set_sysfs_str(scaleOsd1_path, "0x10001");

    return 0;
}

int GL_2X_scale(int mSwitch)
{
    char mode[16];
    char m1080scale[8];
    char raxis_str[32],saxis_str[32];
    char writedata[40] = {0};
    char vaxis_newx_str[10] = {0};
    char vaxis_newy_str[10] = {0};
    char vaxis_width_str[10] = {0};
    char vaxis_height_str[10] = {0};
    int vaxis_newx= -1,vaxis_newy = -1,vaxis_width= -1,vaxis_height= -1;
    int ret = 0;
    char buf[32];
        
    property_get("ro.platform.has.1080scale",m1080scale,"fail");
    if(!strncmp(m1080scale, "fail", 4))
    {
        return 0;
    }
    get_display_mode(mode);
    if(strncmp(m1080scale, "2", 1) && (strncmp(m1080scale, "1", 1) || (strncmp(mode, "1080i", 5) && strncmp(mode, "1080p", 5))))
    {
        log_print("[enable_2XYscale]not freescale mode!\n");
        return 0;
    }
    
    /*ret = get_sysfs_str("/sys/class/graphics/fb0/free_scale", buf, 32);

    log_print("[GL_2X_scale]get free_scale %s!\n",buf);
    if((mSwitch==0)&&(ret>=0)&&strstr(buf, "0")){
        log_print("[GL_2X_scale] already enabled,no need to set again!\n");
        return 0;
    }*/

    char* scale_Osd0_path = "/sys/class/graphics/fb0/scale";
    char* scale_Osd1_path = "/sys/class/graphics/fb1/scale";
    char* scale_Osd0_axis_path = "/sys/class/graphics/fb0/scale_axis";
    char* scale_Osd1_axis_path = "/sys/class/graphics/fb1/scale_axis";
    char* display_axis_path = "/sys/class/display/axis";
    char* request2X_scale_path = "/sys/class/graphics/fb0/request2XScale";
    char* fb0_blank_path = "/sys/class/graphics/fb0/blank";
    char* fb1_blank_path = "/sys/class/graphics/fb1/blank";
    
    if(mSwitch == 0)
    {
        ret = get_sysfs_str("/sys/class/graphics/fb1/scale", buf, 32);
        
        if((ret >=0) && strncmp(buf, "scale:[0x10001]", strlen("scale:[0x10001]"))==0){
            set_sysfs_str(fb0_blank_path, "1");
        }
        else
        {
            log_print("[GL_2X_scale] already disable,no need to set again!\n");
            return 0;
        }

        ret = get_sysfs_str("/sys/class/graphics/fb0/request2XScale", buf, 32);
        
        if((ret >=0) && strncmp(buf, "2", strlen("2"))==0){
            set_sysfs_str(request2X_scale_path, "4");
            log_print("write 4 to request2XScaleFile!!");
        }
        else {
            set_sysfs_str(request2X_scale_path, "2");
            log_print("write 2 to request2XScaleFile!!");
        }

        if(!strncmp(mode, "1080i", 5) || !strncmp(mode, "1080p", 5)){
            set_sysfs_str(scale_Osd0_path, "0");
            set_sysfs_str(scale_Osd0_axis_path, "0 0 959 1079");          
        }
        
        set_sysfs_str(display_axis_path, "0 0 1280 720 0 0 18 18");
        set_sysfs_str(scale_Osd1_path, "0");
    }
    else if(mSwitch == 1){
        
        ret = get_sysfs_str("/sys/class/graphics/fb1/scale", buf, 32);
        
        if((ret >=0) && strncmp(buf, "scale:[0x0]", strlen("scale:[0x0]"))==0){
            set_sysfs_str(fb0_blank_path, "1");
        }
        else
        {
            log_print("[GL_2X_scale] already enable,no need to set again!\n");
            return 0;
        }
        if(!strncmp(mode, "480i", 4) || !strncmp(mode, "480p", 4) || !strncmp(mode, "480cvbs", 7)){

            if(!strncmp(mode, "480i", 4)){
                property_get("ubootenv.var.480ioutputx",vaxis_newx_str,"0");
                property_get("ubootenv.var.480ioutputy",vaxis_newy_str,"0");
                property_get("ubootenv.var.480ioutputwidth",vaxis_width_str,"720"); 
                property_get("ubootenv.var.480ioutputheight",vaxis_height_str,"480");
            }
            else{            
                property_get("ubootenv.var.480poutputx",vaxis_newx_str,"0");
                property_get("ubootenv.var.480poutputy",vaxis_newy_str,"0"); 
                property_get("ubootenv.var.480poutputwidth",vaxis_width_str,"720");  
                property_get("ubootenv.var.480poutputheight",vaxis_height_str,"480");        
            }

            vaxis_newx = atoi(vaxis_newx_str);
            vaxis_newy = atoi(vaxis_newy_str);
            vaxis_width = atoi(vaxis_width_str);
            vaxis_height = atoi(vaxis_height_str);

            log_print("vaxis_newx:%d vaxis_newy:%d vaxis_width:%d vaxis_height:%d\n",
                       vaxis_newx,vaxis_newy,vaxis_width,vaxis_height);

            sprintf(writedata,"%d %d 1280 720 %d %d 18 18",
                    vaxis_newx,vaxis_newy,vaxis_newx,vaxis_newy);
            set_sysfs_str(display_axis_path, writedata);

            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"16 %d %d",
                    vaxis_width,
                    vaxis_height);
            set_sysfs_str(request2X_scale_path, writedata);
            
            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"1280 720 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(scale_Osd1_axis_path, writedata);
            
            set_sysfs_str(scale_Osd1_path, "0x10001");
            
        }
        else if(!strncmp(mode, "576i", 4) || !strncmp(mode, "576p", 4) || !strncmp(mode, "576cvbs", 7)){
            
            if(!strncmp(mode, "576i", 4)){
                property_get("ubootenv.var.576ioutputx",vaxis_newx_str,"0");
                property_get("ubootenv.var.576ioutputy",vaxis_newy_str,"0");
                property_get("ubootenv.var.576ioutputwidth",vaxis_width_str,"720"); 
                property_get("ubootenv.var.576ioutputheight",vaxis_height_str,"576");
            }
            else{            
                property_get("ubootenv.var.576poutputx",vaxis_newx_str,"0"); 
                property_get("ubootenv.var.576poutputy",vaxis_newy_str,"0");
                property_get("ubootenv.var.576poutputwidth",vaxis_width_str,"720"); 
                property_get("ubootenv.var.576poutputheight",vaxis_height_str,"576");        
            }

            vaxis_newx = atoi(vaxis_newx_str);
            vaxis_newy = atoi(vaxis_newy_str);
            vaxis_width = atoi(vaxis_width_str);
            vaxis_height = atoi(vaxis_height_str);
            
            log_print("vaxis_newx:%d vaxis_newy:%d vaxis_width:%d vaxis_height:%d\n",
                       vaxis_newx,vaxis_newy,vaxis_width,vaxis_height);
            
            sprintf(writedata,"%d %d 1280 720 %d %d 18 18",
                    vaxis_newx,vaxis_newy,vaxis_newx,vaxis_newy);
            set_sysfs_str(display_axis_path, writedata);

            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"16 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(request2X_scale_path, writedata);
            
            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"1280 720 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(scale_Osd1_axis_path, writedata);
            
            set_sysfs_str(scale_Osd1_path, "0x10001");
        }
        else if(!strncmp(mode, "720p", 4)){

            property_get("ubootenv.var.720poutputx",vaxis_newx_str,"0");
            property_get("ubootenv.var.720poutputy",vaxis_newy_str,"0");  
            property_get("ubootenv.var.720poutputwidth",vaxis_width_str,"1280"); 
            property_get("ubootenv.var.720poutputheight",vaxis_height_str,"720");

            vaxis_newx = atoi(vaxis_newx_str);
            vaxis_newy = atoi(vaxis_newy_str);
            vaxis_width = atoi(vaxis_width_str);
            vaxis_height = atoi(vaxis_height_str);
            
            log_print("vaxis_newx:%d vaxis_newy:%d vaxis_width:%d vaxis_height:%d\n",
                       vaxis_newx,vaxis_newy,vaxis_width,vaxis_height);

            sprintf(writedata,"%d %d 1280 720 %d %d 18 18",
                    vaxis_newx,vaxis_newy,vaxis_newx,vaxis_newy);
            set_sysfs_str(display_axis_path, writedata);

            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"16 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(request2X_scale_path, writedata);
            
            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"1280 720 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(scale_Osd1_axis_path, writedata);
            
            set_sysfs_str(scale_Osd1_path, "0x10001");
            
        }
        else if(!strncmp(mode, "1080i", 5) || !strncmp(mode, "1080p", 5)){
            
            if(!strncmp(mode, "1080i", 5)){ 
                property_get("ubootenv.var.1080ioutputx",vaxis_newx_str,"0");
                property_get("ubootenv.var.1080ioutputy",vaxis_newy_str,"0");
                property_get("ubootenv.var.1080ioutputwidth",vaxis_width_str,"1920"); 
                property_get("ubootenv.var.1080ioutputheight",vaxis_height_str,"1080");
            }
            else{            
                property_get("ubootenv.var.1080poutputx",vaxis_newx_str,"0");
                property_get("ubootenv.var.1080poutputy",vaxis_newy_str,"0");  
                property_get("ubootenv.var.1080poutputwidth",vaxis_width_str,"1920");  
                property_get("ubootenv.var.1080poutputheight",vaxis_height_str,"1080");        
            }

            vaxis_newx = atoi(vaxis_newx_str);
            vaxis_newy = atoi(vaxis_newy_str);
            vaxis_width = atoi(vaxis_width_str);
            vaxis_height = atoi(vaxis_height_str);

            log_print("vaxis_newx:%d vaxis_newy:%d vaxis_width:%d vaxis_height:%d\n",
                       vaxis_newx,vaxis_newy,vaxis_width,vaxis_height);

            sprintf(writedata,"%d %d 1280 720 %d %d 18 18",
                    (vaxis_newx/2)*2,(vaxis_newy/2)*2,(vaxis_newx/2)*2,(vaxis_newy/2)*2);
            set_sysfs_str(display_axis_path, writedata);

            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"0 0 %d %d",
                    960-(vaxis_newx/2)-1,
                    1080-(vaxis_newy/2)-1);
            set_sysfs_str(scale_Osd0_axis_path, writedata);

            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"7 %d %d",(vaxis_width/2),vaxis_height);
            set_sysfs_str(request2X_scale_path, writedata);
            
            memset(writedata,0,strlen(writedata));
            sprintf(writedata,"1280 720 %d %d",vaxis_width,vaxis_height);
            set_sysfs_str(scale_Osd1_axis_path, writedata);
            
            set_sysfs_str(scale_Osd1_path, "0x10001");
            
        }
        
    }

    return 0;
    
}

/*
int disable_freescale(int cfg)
{
#ifndef ENABLE_FREE_SCALE
    log_print("ENABLE_FREE_SCALE not define!\n");
    return 0;
#endif
    char mode[16];
    freescale_setting_t *setting;
    display_mode disp_mode;
    int i;
    int num;

    log_print("[disable_freescale]cfg = 0x%x\n", cfg);
    //if(cfg == MID_800_400_FREESCALE)
    {
        log_print("[disable_freescale]mid 800*400, do config...\n");
        get_display_mode(mode);
        log_print("[disable_freescale]display_mode=%s \n", mode);
        if (strncmp(mode, "fail", 4)) { //mode !=fail
            disp_mode = display_mode_convert(mode);
            update_freescale_setting();
            num = sizeof(freescale_setting) / sizeof(freescale_setting[0]);
            log_print("[%s:%d]num=%d\n", __FUNCTION__, __LINE__, num);
            if (disp_mode >= DISP_MODE_480I && disp_mode <= DISP_MODE_1080P) {
                for (i = 0; i < num; i ++) {
                    if (disp_mode == freescale_setting[i].disp_mode) {
                        setting = &freescale_setting[i];
                        break;
                    }
                }
                if (i == num) {
                    log_error("[%s:%d]display_mode [%s:%d] needn't set!\n", __FUNCTION__, __LINE__, mode, disp_mode);
                    return 0;
                }
                set_fb0_freescale(0);
                set_fb1_freescale(0);
                set_display_axis(setting->osd_disble_coordinate);
                log_print("[disable_freescale]mid 800*400 config success!\n");
            } else {
                log_error("[disable_freescale]mid 800*400 config failed, display mode invalid\n");
            }
            return 0;
        } else {
            log_error("[disable_freescale]get display mode failed\n");
        }
    }
    log_print("[disable_freescale]do not need config freescale, exit!");
    return -1;
}

int enable_freescale(int cfg)
{
#ifndef ENABLE_FREE_SCALE
    log_print("ENABLE_FREE_SCALE not define!\n");
    return 0;
#endif
    char mode[16];
    freescale_setting_t *setting;
    display_mode disp_mode;
    int i;
    int num;

    log_print("[enable_freescale]cfg = 0x%x\n", cfg);
    //if(cfg == MID_800_400_FREESCALE)
    {
        log_print("[enable_freescale]mid 800*400, do config...\n");
        get_display_mode(mode);
        log_print("[enable_freescale]display_mode=%s \n", mode);
        if (strncmp(mode, "fail", 4)) {
            disp_mode = display_mode_convert(mode);
            update_freescale_setting();
            num = sizeof(freescale_setting) / sizeof(freescale_setting[0]);
            log_print("[%s:%d]num=%d\n", __FUNCTION__, __LINE__, num);
            if (disp_mode >= DISP_MODE_480I && disp_mode <= DISP_MODE_1080P) {
                for (i = 0; i < num; i ++) {
                    if (disp_mode == freescale_setting[i].disp_mode) {
                        setting = &freescale_setting[i];
                        break;
                    }
                }
                if (i == num) {
                    log_error("[%s:%d]display_mode [%s:%d] needn't set!\n", __FUNCTION__, __LINE__, mode, disp_mode);
                    return 0;
                }
                set_video_axis(setting->video_enablecoordinate);
                set_display_axis(setting->osd_enable_coordinate);
                set_fb0_freescale(0);
                set_fb1_freescale(0);
                set_fb0_scale_width(setting->fb0_freescale_width);
                set_fb0_scale_height(setting->fb0_freescale_height);
                set_fb1_scale_width(setting->fb1_freescale_width);
                set_fb1_scale_height(setting->fb1_freescale_height);
                set_fb0_freescale(1);
                set_fb1_freescale(1);
                log_print("[enable_freescale]mid 800*400 config success!\n");
            } else {
                log_error("[enable_freescale]mid 800*400 config failed, display mode invalid\n");
            }
            return 0;
        } else {
            log_error("[enable_freescale]get display mode failed\n");
        }
    }
    log_print("[enable_freescale]do not need config freescale, exit!");
    return -1;
}
*/

int get_stb_source(char *strval, int size)
{

    char *path = "/sys/class/stb/source";
    if (get_sysfs_str(path, strval, size) != -1) {
        return 1;
    }
    return 0;
}
int set_stb_source_hiu()
{
    char *path = "/sys/class/stb/source";
    return set_sysfs_str(path, "hiu");
    
}

int get_stb_demux_source(char *strval, int size)
{
    char *path = "/sys/class/stb/demux1_source";
    if (get_sysfs_str(path, strval, size) != -1) {
        return 1;
    }
    return 0;
}

int set_stb_demux_source_hiu()
{
    char *path = "/sys/class/stb/demux1_source"; // use demux0 for record, and demux1 for playback
    return set_sysfs_str(path, "hiu");
}

int get_readend_set_flag()
{
    char str[100];
    get_sysfs_str("/sys/class/audiodsp/fread_end_flag", str, 7);
    return (strcmp(str, "F_NEND")); // 1 read end have been set 0 have not been set
}

int set_readend_flag(int is_read_end)
{
    return set_sysfs_int("/sys/class/audiodsp/fread_end_flag", is_read_end); // 0 success -1 failed
}
int get_decend_flag()
{
    char str[100];
    get_sysfs_str("/sys/class/audiodsp/fread_end_flag", str, 8);
    return (!strcmp(str, "DSP_END")); // 1 decode end 0 not decode end
}

int get_pcmend_flag()
{
    //int num=get_sysfs_int("/sys/class/audiodsp/pcm_left_len");
    char *path = "/sys/class/audiodsp/pcm_left_len";
    int val = 0;
    char  bcmd[16];
    if (get_sysfs_str(path, bcmd, 16) != -1) {
        val = strtol(bcmd, NULL, 10);
    } else {
        return 0;
    }
    //return val;
    log_print("get_pcmnum=%d %s \n", val, bcmd);
    return (val > 1024) ? 0 : 1; // 1 pcm end 0 pcm not end
}
