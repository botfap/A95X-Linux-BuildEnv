#include <amconfigutils.h>

int GetSystemSettingString(const char *path, char *value, char *defaultv)
{
    printf("player system GetSystemSettingString[%s] %s\n", __FILE__, __FUNCTION__);
    return am_getconfig(path, value, defaultv);
}

int property_get(const char *key, char *value, const char *default_value)
{
    printf("player system property_get [%s] %s\n", __FILE__, __FUNCTION__);
    return am_getconfig(key, value, default_value);
}

