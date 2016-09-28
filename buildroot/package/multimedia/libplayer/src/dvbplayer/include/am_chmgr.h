
#ifndef __AM_CHMGR_H
#define __AM_CHMGR_H

#include "dvb.h"


#ifdef __CPLUSPLUS
extern "C"
{
#endif


int aml_load_channel_info(const char *filename,aml_dvb_param_t *param);

int aml_load_dvb_param(const char *filename,aml_dvb_init_param_t *param);

int aml_check_is_channel_info_file(const char *filename);


#ifdef __CPLUSPLUS
}
#endif





#endif

