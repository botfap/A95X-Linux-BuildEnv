/********************************************
 *  name : player_profile.c
 *  date :2012.1.17
 *  function:update profile
 ********************************************/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "player_priv.h"

struct profile_manger {
    vdec_profile_t vdec_profiles;
    int profile_flags;
};
static struct profile_manger profile_mgt = {0};
#define FLAGS_PROFILE_INITED            (1<<0)
#define FLAGS_VPROFILE_CHANGED      (1<<1)
#define FLAGS_APROFILE_CHANGED      (1<<2)
#define FLAGS_PROFILE_CHANGED       (FLAGS_APROFILE_CHANGED | FLAGS_VPROFILE_CHANGED)


static inline struct profile_manger *get_profile_mgt(void) {
    return &profile_mgt;
}

static inline int get_profile_flags(void)
{
    return get_profile_mgt()->profile_flags;
}

static int clear_profile_flags_mask(int flags)
{
    get_profile_mgt()->profile_flags = (get_profile_mgt()->profile_flags & (~flags));
    return 0;
}

static int  set_profile_flags_mask(int flags)
{
    get_profile_mgt()->profile_flags = (get_profile_mgt()->profile_flags | (flags));
    return 0;
}


static int parse_vc1_param(char *str, sys_vc1_profile_t *para, int size)
{
    char *p;
    p = strstr(str, "progressive");
    if (p != NULL && ((p - str) < size)) {
        para->progressive_enable = 1;
    }
    p = strstr(str, "interlace");
    if (p != NULL && ((p - str) < size)) {
        para->interlace_enable = 1;
    }
    p = strstr(str, "wmv1");
    if (p != NULL && ((p - str) < size)) {
        para->wmv1_enable = 1;
    }
    p = strstr(str, "wmv2");
    if (p != NULL && ((p - str) < size)) {
        para->wmv2_enable = 1;
    }
    p = strstr(str, "wmv3");
    if (p != NULL && ((p - str) < size)) {
        para->wmv3_enable = 1;
    }
    log_info("[vc1 profile] progress:%d; interlace:%d; wmv1:%d; wmv2:%d; wmv3:%d\n",
             para->progressive_enable, para->interlace_enable, para->wmv1_enable, para->wmv2_enable, para->wmv3_enable);
    return 0;
}

static int parse_h264_param(char *str, sys_h264_profile_t *para, int size)
{
    para->exist = 1;
	
    log_info("h264 decoder exist.");
    if (strstr(str, "4k")) {
        para->support_4k= 1;
    }
    return 0;
}

static int parse_hevc_param(char *str, sys_hevc_profile_t *para, int size)
{
    para->exist = 1;
	
    log_info("hevc decoder exist.");

    if (strstr(str, "4k")) {
        para->support4k = 1;
    }
    if (strstr(str, "9bit")) {
        para->support_9bit = 1;
    }
    if (strstr(str, "10bit")) {
        para->support_10bit = 1;
    }
    if (strstr(str, "dwrite")) {
        para->support_dwwrite = 1;
    }
    if (strstr(str, "compressed")) {
        para->support_compressed = 1;
    }
    return 0;
}

static int parse_vp9_param(char *str, sys_hevc_profile_t *para, int size)
{
    para->exist = 1;

    log_info("vp9 decoder exist.");

    if (strstr(str, "4k")) {
        para->support4k = 1;
    }
    if (strstr(str, "9bit")) {
        para->support_9bit = 1;
    }
    if (strstr(str, "10bit")) {
        para->support_10bit = 1;
    }
    if (strstr(str, "dwrite")) {
        para->support_dwwrite = 1;
    }
    if (strstr(str, "compressed")) {
        para->support_compressed = 1;
    }
    return 0;
}


static int parse_real_param(char *str, sys_real_profile_t *para, int size)
{
    return 0;
}

static int parse_mpeg12_param(char *str, sys_mpeg12_profile_t *para, int size)
{
    return 0;
}

static int parse_mpeg4_param(char *str, sys_mpeg4_profile_t *para, int size)
{
    return 0;
}

static int parse_mjpeg_param(char *str, sys_mjpeg_profile_t *para, int size)
{
    return 0;
}

static int parse_h264_4k2k_param(char *str, sys_h264_4k2k_profile_t *para, int size)
{
    para->exist = 1;

    log_info("[h264_4k2k profile] decoder exist.");

    return 0;
}

static int parse_hmvc_param(char *str, sys_hmvc_profile_t *para, int size)
{
    para->exist = 1;
	
    log_info("hmvc decoder exist.");
	
    return 0;
}

static int parse_avs_param(char *str, sys_avs_profile_t *para, int size)
{
    para->exist = 1;

    log_info("hevc decoder exist.");

    if (strstr(str, "avs+")) {
        para->support_avsplus = 1;
    }
    return 0;
}
static int parse_param(char *str, char **substr, int size, vdec_profile_t *para)
{
    if (!strcmp(*substr, "vc1:")) {
        parse_vc1_param(str, &para->vc1_para, size);
    } else if (!strcmp(*substr, "h264:")) {
        parse_h264_param(str, &para->h264_para, size);
    } else if (!strcmp(*substr, "hevc:")) {
        parse_hevc_param(str, &para->hevc_para, size);
    } else if (!strcmp(*substr, "vp9:")) {
        parse_vp9_param(str, &para->vp9_para, size);
    } else if (!strcmp(*substr, "real:")) {
        parse_real_param(str, &para->real_para, size);
    } else if (!strcmp(*substr, "mpeg12:")) {
        parse_mpeg12_param(str, &para->mpeg12_para, size);
    } else if (!strcmp(*substr, "mpeg4:")) {
        parse_mpeg4_param(str, &para->mpeg4_para, size);
    } else if (!strcmp(*substr, "mjpeg:")) {
        parse_mjpeg_param(str, &para->mjpeg_para, size);
    } else if (!strcmp(*substr, "h264_4k2k:")) {
        parse_h264_4k2k_param(str, &para->h264_4k2k_para, size);
    } else if (!strcmp(*substr, "hmvc:")) {
        parse_hmvc_param(str, &para->hmvc_para, size);
    } else if (!strcmp(*substr, "avs:")) {
        parse_avs_param(str, &para->avs_para, size);
    }
    return 0;
}

static int parse_sysparam_str(vdec_profile_t *m_vdec_profiles, char *str)
{
    int i, j;
    int pos_start, pos_end;
    char *p;
    char *substr[] = {"vc1:", "h264:", "real:", "mpeg12:", "mpeg4:", "mjpeg:",
         "h264_4k2k:", "hmvc:", "hevc:", "avs:","vp9:"};
    for (j = 0; j < sizeof(substr) / sizeof(char *); j ++) {
        char line[256];
        int line_len=0;
        p = strstr(str, substr[j]);
        if (p != NULL) {
            pos_start = p - str;
            i = pos_start;
            while (str[i] != '\n') {
                i ++;
            }
            pos_end = i;
            line_len = pos_end - pos_start;
            line_len = MIN(line_len,255);
            strncpy(line, str + pos_start, line_len);
            line[line_len] = '\0';
            log_print("[%s]parser tag:%s line:%s\n", __FUNCTION__, substr[j], line);
            parse_param(line, &substr[j], line_len, m_vdec_profiles);
        }
    }
    return 0;
}


static int player_update_vdec_profile(void)
{
#define READ_LINE_SIZE (1024)
    int fd = 0;
    int ret = 0;
    char valstr[READ_LINE_SIZE];
    char *path = "/sys/class/amstream/vcodec_profile";
    vdec_profile_t m_vdec_profiles;
    memset(&m_vdec_profiles, 0, sizeof(vdec_profile_t));
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return fd;
    }
    ret = read(fd, valstr, READ_LINE_SIZE);
    if (ret < 0) {
        return ret;
    }

    log_print("[%s]str=%s\n", __FUNCTION__, valstr);
    if (parse_sysparam_str(&m_vdec_profiles, valstr) == 0) {
        memcpy(&get_profile_mgt()->vdec_profiles, &m_vdec_profiles, sizeof(vdec_profile_t));
    }
    close(fd);
    clear_profile_flags_mask(FLAGS_VPROFILE_CHANGED);
    return 0;
}

static int player_update_adec_profile(void)
{
    clear_profile_flags_mask(FLAGS_APROFILE_CHANGED);
    return 0;
}

/*for update player's profile */
int player_update_profile(void)
{
    struct profile_manger *profile = get_profile_mgt();
    player_update_vdec_profile();
    player_update_adec_profile();
    clear_profile_flags_mask(FLAGS_PROFILE_CHANGED);
    return 0;
}

/*get player's profile,update if needed
flags=FLAGS_FORCE_UPDATEmay force update
*/
int player_get_vdec_profile(vdec_profile_t *vdec_profiles, int flags)
{
    struct profile_manger *profile = get_profile_mgt();
    int state = profile->profile_flags;
    if (!(state & FLAGS_PROFILE_INITED) ||   /*have not inited?*/
        (state & FLAGS_PROFILE_CHANGED) ||  /*profile have changed */
        (flags & FLAGS_FORCE_UPDATE)) {             /*force update by caller*/
        player_update_profile();
    }

    memcpy(vdec_profiles, &profile->vdec_profiles, sizeof(vdec_profile_t));
    return 0;
}
int player_basic_profile_init(int flags)
{
    struct profile_manger *profile = get_profile_mgt();
    memset(profile, 0, sizeof(struct profile_manger));
    profile->vdec_profiles.vc1_para.progressive_enable = 1;
    profile->vdec_profiles.vc1_para.interlace_enable = 0;
    profile->vdec_profiles.vc1_para.wmv1_enable = 0;
    profile->vdec_profiles.vc1_para.wmv2_enable = 0;
    profile->vdec_profiles.vc1_para.wmv3_enable = 1;
    player_update_profile();
    profile->profile_flags = FLAGS_PROFILE_INITED;
    return 0;
}

