
#ifndef AMSYSFS_UTILS_H
#define AMSYSFS_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amsysfs_set_sysfs_str(const char *path, const char *val);
    int amsysfs_get_sysfs_str(const char *path, char *valstr, int size);
    int amsysfs_set_sysfs_int(const char *path, int val);
    int amsysfs_get_sysfs_int(const char *path);
    int amsysfs_set_sysfs_int16(const char *path, int val);
    int amsysfs_get_sysfs_int16(const char *path);
    unsigned long amsysfs_get_sysfs_ulong(const char *path);

#ifdef  __cplusplus
}
#endif


#endif

