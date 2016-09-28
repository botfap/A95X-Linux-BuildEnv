#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/fb.h>
#include "gstamlsysctl.h"
static int axis[8] = {0};
int set_sysfs_str(const char *path, const char *val)
{
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
    }
    return -1;
}
int  get_sysfs_str(const char *path, char *valstr, int size)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        sprintf(valstr, "%s", "fail");
        return -1;
    };
    //log_print("get_sysfs_str=%s\n", valstr);
    return 0;
}

int set_sysfs_int(const char *path, int val)
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
    }
    return -1;
}
int get_sysfs_int(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
    }
    return val;
}


int set_black_policy(int blackout)
{
    return set_sysfs_int("/sys/class/video/blackout_policy", blackout);
}

int get_black_policy()
{
    return get_sysfs_int("/sys/class/video/blackout_policy") & 1;
}

int set_tsync_enable(int enable)
{
    return set_sysfs_int("/sys/class/tsync/enable", enable);

}
int set_ppscaler_enable(char *enable)
{
    return set_sysfs_str("/sys/class/ppmgr/ppscaler", enable);
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

int set_fb0_blank(int blank)
{
    return  set_sysfs_int("/sys/class/graphics/fb0/blank", blank);
}

int set_fb1_blank(int blank)
{
    return  set_sysfs_int("/sys/class/graphics/fb1/blank", blank);
}

int parse_para(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;
    if (!startp) {
        return 0;
    }
    len = strlen(startp);
    do {
        //filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len) {
            startp++;
            len--;
        }
        if (len == 0) {
            break;
        } 
       *out++ = strtol(startp, &endp, 0);
        len -= endp - startp;
        startp = endp;
        count++;
    } while ((endp) && (count < para_num) && (len > 0));
    return count;
}

int set_display_axis(int recovery)
{    
    int fd;    
    char *path = "/sys/class/display/axis";
    char str[128];
    int count, i;
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        if (!recovery) {
            read(fd, str, 128);
            printf("read axis %s, length %d\n", str, strlen(str));
            count = parse_para(str, 8, axis);
        }
        if (recovery) {
            sprintf(str, "%d %d %d %d %d %d %d %d",
                 axis[0],axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        } else {
            sprintf(str, "2048 %d %d %d %d %d %d %d",
                 axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }
    return -1;
}

/*property functions*/
static void aml_register_propfunc (GHashTable **propTable, gint key, AmlPropFunc func)
{
  gpointer ptr = (gpointer) func;

  if (propTable && !(*propTable)){
    *propTable = g_hash_table_new (g_direct_hash, g_direct_equal);
  }
  if (!g_hash_table_lookup (*propTable, key))
    g_hash_table_insert (*propTable, key, (gpointer) ptr);
}
static void aml_unregister_propfunc(GHashTable *propTable)
{
    if(propTable){
        g_hash_table_destroy(propTable);
    }
}

AmlPropFunc aml_find_propfunc (GHashTable *propTable, gint key)
{
    AmlPropFunc func = NULL;
		if(propTable == NULL) {
			return func;
		}
		else {
    	func = (AmlPropFunc)g_hash_table_lookup(propTable, key);
    	return func;
		}
}

void aml_Install_Property(
    GObjectClass *kclass, 
    GHashTable **getPropTable, 
    GHashTable **setPropTable, 
    AmlPropType *prop_pool)
{
    GObjectClass*gobject_class = (GObjectClass *) kclass;
    AmlPropType *p = prop_pool;

    while(p && (p->propID != -1)){
        if(p->installprop){
            p->installprop(gobject_class, p->propID);
        }
        if(p->getprop){
            aml_register_propfunc(getPropTable, p->propID, p->getprop);
        }
        if(p->setprop){
            aml_register_propfunc(setPropTable, p->propID, p->setprop);
        }
        p++;
    }

}

void aml_Uninstall_Property(GHashTable *getPropTable, GHashTable *setPropTable)
{
    aml_unregister_propfunc(getPropTable);
    aml_unregister_propfunc(setPropTable);
}
