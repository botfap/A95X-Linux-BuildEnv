#ifndef AMUTILS_COMMON_H
#define AMUTILS_COMMON_H

#ifdef  __cplusplus
extern "C" {
#endif

int set_sys_int(const char *path,int val);
int get_sysfs_int(const char *path);

#ifdef  __cplusplus
}
#endif


#endif

