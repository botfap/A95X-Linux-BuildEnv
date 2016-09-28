#ifndef GET_SYSTEM_SETTING_HHHH
#define GET_SYSTEM_SETTING_HHHH

int GetSystemSettingString(const char *path, char *value, char *defaultv);
float PlayerGetSettingfloat(const char* path);
int PlayerSettingIsEnable();
int PlayerGetVFilterFormat();

#endif

