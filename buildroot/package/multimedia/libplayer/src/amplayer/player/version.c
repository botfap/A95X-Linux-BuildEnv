#include <log_print.h>
#include <stdio.h>
#include "version.h"
#include <string.h>


static char versioninfo[256] = "N/A";
static long long version_serial = 0;
static char gitversionstr[256] = "N/A";


static int player_version_info_init(void)
{
    static int info_is_inited = 0;
    char git_shor_version[20];
    unsigned int  shortgitversion;
    int dirty_num = 0;

    if (info_is_inited > 0) {
        return 0;
    }
    info_is_inited++;

#ifdef HAVE_VERSION_INFO
#ifdef LIBPLAYER_GIT_UNCOMMIT_FILE_NUM
#if LIBPLAYER_GIT_UNCOMMIT_FILE_NUM>0
    dirty_num = LIBPLAYER_GIT_UNCOMMIT_FILE_NUM;
#endif
#endif
#ifdef LIBPLAYER_GIT_VERSION
    if (dirty_num > 0) {
        snprintf(gitversionstr, 250, "%s-with-%d-dirty-files", LIBPLAYER_GIT_VERSION, dirty_num);
    } else {
        snprintf(gitversionstr, 250, "%s", LIBPLAYER_GIT_VERSION);
    }
#endif
#endif
    memcpy(git_shor_version, gitversionstr, 8);
    git_shor_version[8] = '\0';
    sscanf(git_shor_version, "%x", &shortgitversion);
    version_serial = (long long)(LIBPLAYER_VERSION_MAIN & 0xff) << 56   |
                     (long long)(LIBPLAYER_VERSION_SUB1 & 0xff) << 48    |
                     (long long)(LIBPLAYER_VERSION_SUB2 & 0xff) << 32    |
                     shortgitversion;

    snprintf(versioninfo, 256, "Version:%d.%d.%d.%s",
             LIBPLAYER_VERSION_MAIN,
             LIBPLAYER_VERSION_SUB1,
             LIBPLAYER_VERSION_SUB2,
             git_shor_version);

    return 0;

}


const char *player_get_version_info(void)
{
    player_version_info_init();
    return versioninfo;
}
long long player_get_version_serail(void)
{
    player_version_info_init();
    return version_serial;
}



const char *player_get_git_version_info(void)
{
    player_version_info_init();
    return gitversionstr;
}


const char *player_get_last_chaned_time_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBPLAYER_LAST_CHANGED
    return LIBPLAYER_LAST_CHANGED;
#endif
#endif
    return " Unknow ";
}

const char *player_get_build_time_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBPLAYER_BUILD_TIME
    return LIBPLAYER_BUILD_TIME;
#endif
#endif
    return " Unknow ";
}



const char *player_get_build_name_info(void)
{
#ifdef HAVE_VERSION_INFO
#ifdef LIBPLAYER_BUILD_NAME
    return LIBPLAYER_BUILD_NAME;
#endif
#endif
    return " Unknow ";
}


void print_version_info()
{
    player_version_info_init();
    log_print("LibPlayer version:%s\n", player_get_version_info());
    log_print("LibPlayer git version:%s\n", player_get_git_version_info());
    log_print("LibPlayer version serial:%llx\n", player_get_version_serail());
    log_print("LibPlayer Last Changed:%s\n", player_get_last_chaned_time_info());
    log_print("LibPlayer Last Build:%s\n", player_get_build_time_info());
    log_print("LibPlayer Builer Name:%s\n", player_get_build_name_info());
}




