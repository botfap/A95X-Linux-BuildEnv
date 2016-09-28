
#ifndef __CHMGR_FILE_H
#define __CHMGR_FILE_H



#ifdef __CPLUSPLUS
extern "C"
{
#endif

int load_key_value(const char *filename,const char *keyword,int *keyvalue);
int load_freq_value(const char *filename,const char *keyword,int *keyvalue, int **keylist);


#ifdef __CPLUSPLUS
}
#endif


#endif

