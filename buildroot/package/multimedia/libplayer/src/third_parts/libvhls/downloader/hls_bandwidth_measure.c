#include <stdio.h>
#include <string.h>

#include "hls_bandwidth_measure.h"
#include "hls_utils.h"

struct rdata
{
	int bytes;
	int delay_us;
};

struct ratesdata
{
	int 	    rdata_index;
	int 	    rdata_num;	
	int64_t total_bytes;
	int 	    latest_m_duration_us;
	int 	    latest_m_bytes;
	int 	    latest_f_duration_us;
	int 	    latest_f_bytes;
	int 	    rdata_f_num;
	int 	    totol_rd_bytes;
	int64_t init_time_us;
	int64_t 	    last_start_read_time_us;
	struct rdata data[1];/*at least 1*/
};

#define INDEX_LOOP_ADD(I,N)	((I)=((I)+1)%(N))
#define LAST_F_INDEX(I,N,NN)	((((I)-(NN))<0)?(N+I-(NN)):((I)-(NN)))
void *bandwidth_measure_alloc(int measurenum,int flags)
{
	struct ratesdata *rates;
	rates=malloc(sizeof(struct ratesdata)+measurenum*sizeof(struct rdata));
	if(!rates)
		return NULL;
    memset(rates,0,sizeof(struct ratesdata)+measurenum*sizeof(struct rdata));
	rates->rdata_num=measurenum;
	rates->init_time_us=in_gettimeUs();
	rates->rdata_f_num=(measurenum/10);
	if(rates->rdata_f_num<2)
		rates->rdata_f_num=2;
	return rates;
}


int bandwidth_measure_add(void *band,int bytes,int delay_us)
{
	struct ratesdata *rates=(struct ratesdata *)band;
	struct rdata *rdata;
	rdata=&rates->data[rates->rdata_index];
	//av_log(NULL, AV_LOG_INFO, "bandwidth_measure_add %d,%d,rdata_index=%d",bytes,delay_us,rates->rdata_index);
	rates->latest_m_bytes-=rdata->bytes;
	rates->latest_m_duration_us-=rdata->delay_us;
	
	rates->latest_m_bytes+=bytes;
	rates->latest_m_duration_us+=delay_us;
	//av_log(NULL, AV_LOG_INFO, "rates->latest_m_bytes=%d,rates->latest_m_duration_us=%d",rates->latest_m_bytes,rates->latest_m_duration_us);
	rdata->bytes=bytes;
	rdata->delay_us=delay_us;

	rdata=&rates->data[LAST_F_INDEX(rates->rdata_index,rates->rdata_num,rates->rdata_f_num)];
	//av_log(NULL, AV_LOG_INFO, "bandwidth_measure_add LAST_F_INDEX=%d",LAST_F_INDEX(rates->rdata_index,rates->rdata_num,rates->rdata_f_num));
	
	rates->latest_f_bytes-=rdata->bytes;
	rates->latest_f_duration_us-=rdata->delay_us;
	//av_log(NULL, AV_LOG_INFO, "rates->latest_f_bytes=%d,rates->latest_f_duration_us=%d",rates->latest_f_bytes,rates->latest_f_duration_us);
	rates->latest_f_bytes+=bytes;
	rates->latest_f_duration_us+=delay_us;

	rates->total_bytes+=bytes;
	INDEX_LOOP_ADD(rates->rdata_index,rates->rdata_num);
	return 0;
}


void bandwidth_measure_start_read(void *band)
{
	struct ratesdata *rates=(struct ratesdata *)band;
	rates->last_start_read_time_us=in_gettimeUs();
}
int bandwidth_measure_finish_read(void *band,int bytes)
{
    struct ratesdata *rates=(struct ratesdata *)band;
    if(bytes<=0){
        bytes =0;
    }
    if(bytes>=0)
    {
    	int64_t delay=in_gettimeUs()-rates->last_start_read_time_us;
    	if(delay>0)
    		bandwidth_measure_add(band,bytes,delay);
    }
    return 0;
}
int bandwidth_measure_get_bandwidth(void  *band,int *fast_band,int *mid_band,int *avg_band)
{
	struct ratesdata *rates=(struct ratesdata *)band;
	int64_t time_us;
	
	time_us=rates->latest_f_duration_us;
	if(time_us>0 && rates->latest_f_bytes>0)
		*fast_band=(int64_t)rates->latest_f_bytes*8*1000*1000/time_us;/*bits per seconds*/
	else
		*fast_band=0;

	time_us=rates->latest_m_duration_us;
	if(time_us>0 && rates->latest_m_bytes>0)
		*mid_band=(int64_t)rates->latest_m_bytes*8*1000*1000/time_us;/*bits per seconds*/
	else
		*mid_band=0;

	time_us=in_gettimeUs()-rates->init_time_us;
	if(time_us>0)
		*avg_band=(int64_t)rates->total_bytes*8*1000*1000/time_us;/*bits per seconds*/
	else
		*avg_band=0;
	return 0;
}

int bandwidth_measure_free(void *band)
{
	free(band);
	return 0;
}
