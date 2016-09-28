#include <amconfigutils.h>

int GetSystemSettingString(const char *path, char *value, char *defaultv)
{
    return am_getconfig(path, value, defaultv);
}

int property_get(const char *key, char *value, const char *default_value)
{
    return am_getconfig(key, value, default_value);
}

