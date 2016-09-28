#ifndef BANDWIDTH_MEASURE_HHHH
#define BANDWIDTH_MEASURE_HHHH
#include "avio.h"
#include "internal.h"
void *bandwidth_measure_alloc(int measurenum,int flags);
int bandwidth_measure_add(void *band,int bytes,int delay_ns);
int bandwidth_measure_get_bandwidth(void  *band,int *fast_band,int *mid_band,int *avg_band);
void bandwidth_measure_start_read(void *band);
int bandwidth_measure_finish_read(void *band,int bytes);
int bandwidth_measure_free(void *band);

#endif
