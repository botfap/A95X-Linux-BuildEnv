#ifndef _HLS_LIVESESSION_H
#define _HLS_LIVESESSION_H
#ifdef __cplusplus
extern "C" {
#endif

#include"file_list.h"

//hSesssion just set NULL,2012,0821
list_item_t* getCurrentSegment(void* hSession);

int switchNextSegment(void* hSession);
const char* getCurrentSegmentUrl(void* hSession);
long long getTotalDuration(void* hSession);


#ifdef __cplusplus
}
#endif

#endif