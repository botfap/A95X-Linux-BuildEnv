
#ifndef __AML_RESAMPLE_H__
#define __AML_RESAMPLE_H__

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <audiodsp_update_format.h>
#define Q14(ratio)          ((ratio)*(1<<14))
#define Q14_INT_GET(value)  ((value)>>14)
#define Q14_FRA_GET(value)  ((value)&0x3fff)

#define RESAMPLE_DELTA_NUMSAMPS 1
#define DEFALT_NUMSAMPS_PERCH   128
#define MAX_NUMSAMPS_PERCH      (DEFALT_NUMSAMPS_PERCH + RESAMPLE_DELTA_NUMSAMPS)
#define DEFALT_NUMCH            2

#define RESAMPLE_TYPE_NONE      0
#define RESAMPLE_TYPE_DOWN      1
#define RESAMPLE_TYPE_UP        2


typedef struct af_resampe_ctl_s{
  int   SampNumIn;
  int   SampNumOut;
  int   InterpolateCoefArray[MAX_NUMSAMPS_PERCH];
  short InterpolateIndexArray[MAX_NUMSAMPS_PERCH];
  short ResevedBuf[MAX_NUMSAMPS_PERCH*DEFALT_NUMCH];
  short ResevedSampsValid;
  short OutSampReserveBuf[MAX_NUMSAMPS_PERCH*DEFALT_NUMCH];
  short OutSampReserveLen;
  short InitFlag;
  short LastResamType;
}af_resampe_ctl_t;

void af_resample_linear_init();

int af_get_resample_enable_flag();

af_resampe_ctl_t* af_resampler_ctx_get();

void af_resample_set_SampsNumRatio(af_resampe_ctl_t *paf_resampe_ctl);

void af_get_pcm_in_resampler(af_resampe_ctl_t *paf_resampe_ctl,short*buf,int *len);


void  af_resample_process_linear_inner(af_resampe_ctl_t *paf_resampe_ctl,
                                                   short *data_in, int *NumSamp_in,
                                                   short* data_out,int* NumSamp_out,int NumCh);
void  af_resample_stop_process(af_resampe_ctl_t *paf_resampe_ctl);


int af_get_delta_inputsampnum(af_resampe_ctl_t *paf_resampe_ctl,int Nch);

void  af_get_unpro_inputsampnum(af_resampe_ctl_t *paf_resampe_ctl,short *buf, int *num);

void af_resample_api_normal(char *buffer, unsigned int *size,int Chnum, aml_audio_dec_t *audec);

void af_resample_api(char* buffer, unsigned int * size, int Chnum, aml_audio_dec_t* audec, int enable, int delta);

#endif

















