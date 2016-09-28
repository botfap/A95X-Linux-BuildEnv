#include <stdio.h>
#include <stdint.h>
#include "adpcm.h"
#include "../../amadec/adec-armdec-mgt.h"
#include "../../amadec/audio-dec.h"
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "AdpcmDecoder"
#define  PRINTF(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else 
#define  PRINTF  printf  
#endif 

typedef struct
{
   int ValidDataLen;
   int UsedDataLen;
   unsigned char *BufStart;
   unsigned char *pcur;
}pcm_read_ctl_t;


static int pcm_read_init( pcm_read_ctl_t *pcm_read_ctx,unsigned char* inbuf,int size)
{
    pcm_read_ctx->ValidDataLen=size;
    pcm_read_ctx->UsedDataLen =0;
    pcm_read_ctx->BufStart    =inbuf;
    pcm_read_ctx->pcur        =inbuf;
    return 0;
}

static int pcm_read(pcm_read_ctl_t *pcm_read_ctx,unsigned char* outbuf,int size)
{
    int bytes_read=0;
    if(size<=pcm_read_ctx->ValidDataLen)
    {
        memcpy(outbuf,pcm_read_ctx->pcur,size);
        pcm_read_ctx->ValidDataLen -= size;
        pcm_read_ctx->UsedDataLen  += size;
        pcm_read_ctx->pcur         += size;
        bytes_read=size;
    }
    return bytes_read;
}

struct t_wave_buf{
    void *addr;
	unsigned size;
};
static unsigned wave_timestamplen = 0;
static unsigned wave_timestamp = 0;

static int adpcm_step[89] =
{
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static int adpcm_index[16] =
{
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

// useful macros
// clamp a number between 0 and 88
#define CLAMP_0_TO_88(x)  if (x < 0) x = 0; else if (x > 88) x = 88;
// clamp a number within a signed 16-bit range
#define CLAMP_S16(x)  if (x < -32768) x = -32768; \
                      else if (x > 32767) x = 32767;
// clamp a number above 16
#define CLAMP_ABOVE_16(x)  if (x < 16) x = 16;
// sign extend a 16-bit value
#define SE_16BIT(x)  if (x & 0x8000) x -= 0x10000;
// sign extend a 4-bit value
#define SE_4BIT(x)  if (x & 0x8) x -= 0x10;
static t_adpcm_output_buf_manager g_mgr;
static int block_align  = 0;
static int offset = 0;
//extern unsigned char buffer[1024*64];
static unsigned char *pwavebuf = NULL;
static struct t_wave_buf wave_decoder_buffer[]={{0,0},{0,0}}; // 0 - stream, 1 - pcm


static int adpcm_init(aml_audio_dec_t *audec)
{
    audio_decoder_operations_t *adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    PRINTF("[%s]audec->format/%d adec_ops->samplerate/%d adec_ops->channels/%d\n",
           __FUNCTION__,audec->format,adec_ops->samplerate,adec_ops->channels);

    wave_decoder_buffer[0].addr = malloc(WAVE_BLOCK_SIZE);
	if(wave_decoder_buffer[0].addr== 0)
	{
		PRINTF("[%s %d]Error: malloc adpcm buffer failed!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	wave_decoder_buffer[0].size = WAVE_BLOCK_SIZE;
	wave_decoder_buffer[1].addr =(int)malloc(WAVE_BLOCK_SIZE*4*4);
	if(wave_decoder_buffer[1].addr== 0)
	{
		PRINTF("[%s %d]Error: malloc adpcm buffer failed!\n",__FUNCTION__,__LINE__);
		return -1;
	}
    adec_ops->nInBufSize=WAVE_BLOCK_SIZE;
    adec_ops->nOutBufSize=0;
	wave_decoder_buffer[1].size = WAVE_BLOCK_SIZE*4*4; // 2byte, 2ch, compress ratio 4
	g_mgr.start = 0;
	g_mgr.size = 0;
	g_mgr.bps = audec->data_width;
	g_mgr.ch = adec_ops->channels;
	g_mgr.sr = adec_ops->samplerate;
	g_mgr.wr = 0;
	g_mgr.last_rd = 0;	
	g_mgr.totalSample = 0;
	g_mgr.totalSamplePlayed = 0;
	g_mgr.totalSampleDecoded = 0;
	g_mgr.last_pts = 0;
	g_mgr.blk = 0;

	block_align  =audec->block_align;
	wave_timestamplen = 0;
	PRINTF("[%s %d]block_align/%d audec->codec_id/0x%x\n",__FUNCTION__,__LINE__,block_align,audec->codec_id);
	return 0;
}

#define CHECK_DATA_ENOUGH_SUB(Ctl,NeedBytes,UsedSetIfNo)    {              \
     if((Ctl)->ValidDataLen < (NeedBytes)){                                \
           PRINTF("[%s %d]NOTE--> no enough data\n",__FUNCTION__,__LINE__);\
           (Ctl)->UsedDataLen-=(UsedSetIfNo);                              \
           return -1;                                                      \
      }                                                                    \              
}

#define CHECK_DATA_ENOUGH_SET(Ctl,NeedBytes,UsedSetIfNo)    {              \
     if((Ctl)->ValidDataLen < (NeedBytes)){                                \
           PRINTF("[%s %d]NOTE--> no enough data\n",__FUNCTION__,__LINE__);\
           (Ctl)->UsedDataLen=(UsedSetIfNo);                              \
           return -1;                                                      \
      }                                                                    \              
}

static int refill(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char* buf, int len)
{
	static unsigned refill_timestamp_len = 0;
	unsigned char *pbuf = buf;
	unsigned char tmp_a = 0;
    unsigned char tmp_p = 0;
	unsigned char tmp_t = 0;
	unsigned char tmp_s = 0;
	int len_bak = len;
	int tmp = 0;
	if(wave_timestamplen==0)	// when no apts found
	{
		unsigned char timestamp[4] = {0};
		unsigned char block_length[4] = {0};
        
        CHECK_DATA_ENOUGH_SET(pcm_read_ctx,4,0) 
		pcm_read(pcm_read_ctx,&tmp_a,1);	
		pcm_read(pcm_read_ctx,&tmp_p,1);
		pcm_read(pcm_read_ctx,&tmp_t,1);
		pcm_read(pcm_read_ctx,&tmp_s,1);

		if(tmp_a == 'A' && tmp_p == 'P' && tmp_t == 'T' && tmp_s == 'S')
		{
             CHECK_DATA_ENOUGH_SET(pcm_read_ctx,8,0)
             pcm_read(pcm_read_ctx,timestamp,4);
			 wave_timestamp = (timestamp[0]<<24)|(timestamp[1]<<16)|(timestamp[2]<<8)|(timestamp[3]);
             pcm_read(pcm_read_ctx,block_length,4);
			 wave_timestamplen = (block_length[0]<<24)|(block_length[1]<<16)|(block_length[2]<<8)|(block_length[3]);	
			 refill_timestamp_len = wave_timestamplen;

             CHECK_DATA_ENOUGH_SET(pcm_read_ctx,refill_timestamp_len,0)	
	
		}else if(tmp_a == 'R' && tmp_p == 'I' && tmp_t == 'F' && tmp_s == 'F'){	
			 if((audec->codec_id==CODEC_ID_ADPCM_IMA_WAV)||(audec->codec_id==CODEC_ID_ADPCM_MS))
			 {
				  tmp=len;
				  while(tmp){
                       CHECK_DATA_ENOUGH_SET(pcm_read_ctx,8,0)	
					   pcm_read(pcm_read_ctx,&timestamp[0],1);
					   tmp --;
					   if(timestamp[0] == 'd'){
                           pcm_read(pcm_read_ctx,&timestamp[1],3);
						   tmp -=3;
						   if((timestamp[0] == 'd') && (timestamp[1] == 'a') && (timestamp[2] == 't') && (timestamp[3] == 'a') )
							   break;
					   }
				  }
                  pcm_read(pcm_read_ctx,timestamp,4);
				  wave_timestamplen = 0;
				  wave_timestamp = 0xffffffff;

                  CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,len,0)	
			  }else{
				  *pbuf++ = tmp_a;
				  *pbuf++ = tmp_p;
				  *pbuf++ = tmp_t;
				  *pbuf++ = tmp_s;
                  len -= 4;
				  wave_timestamplen = 0;
				  wave_timestamp = 0xffffffff;

                  CHECK_DATA_ENOUGH_SET(pcm_read_ctx,len,0)	
			  }
		}else{
			*pbuf++ = tmp_a;
			*pbuf++ = tmp_p;
			*pbuf++ = tmp_t;
			*pbuf++ = tmp_s;
            len -= 4;
            CHECK_DATA_ENOUGH_SET(pcm_read_ctx,len,0)	
			wave_timestamplen = 0;
			wave_timestamp = 0xffffffff;
		}
	}

    if(wave_timestamplen)
    {
         pcm_read(pcm_read_ctx,pbuf,refill_timestamp_len);
         return  refill_timestamp_len;
    }else{
         pcm_read(pcm_read_ctx,pbuf,len);
         return len_bak;
    }

}

/*IMA ADPCM*/
#define le2me_16(x) (x)
#define MS_IMA_ADPCM_PREAMBLE_SIZE 4
#define LE_16(x) (le2me_16(*(unsigned short *)(x)))

static void decode_nibbles(unsigned short *output,
  int output_size, int channels,
  int predictor_l, int index_l,
  int predictor_r, int index_r)
{
  int step[2];
  int predictor[2];
  int index[2];
  int diff;
  int i;
  int sign;
  int delta;
  int channel_number = 0;

  step[0] = adpcm_step[index_l];
  step[1] = adpcm_step[index_r];
  predictor[0] = predictor_l;
  predictor[1] = predictor_r;
  index[0] = index_l;
  index[1] = index_r;

  for (i = 0; i < output_size; i++)
  {
    delta = output[i];

    index[channel_number] += adpcm_index[delta];
    CLAMP_0_TO_88(index[channel_number]);

    sign = delta & 8;
    delta = delta & 7;

    diff = step[channel_number] >> 3;
    if (delta & 4) diff += step[channel_number];
    if (delta & 2) diff += step[channel_number] >> 1;
    if (delta & 1) diff += step[channel_number] >> 2;

    if (sign)
      predictor[channel_number] -= diff;
    else
      predictor[channel_number] += diff;

    CLAMP_S16(predictor[channel_number]);
    output[i] = predictor[channel_number];
    step[channel_number] = adpcm_step[index[channel_number]];

    // toggle channel
    channel_number ^= channels - 1;

  }
}

static int ima_adpcm_decode_block(unsigned short *output,
	unsigned char *input, int channels, int block_size)
{
  int predictor_l = 0;
  int predictor_r = 0;
  int index_l = 0;
  int index_r = 0;
  int i;
  int channel_counter;
  int channel_index;
  int channel_index_l;
  int channel_index_r;

  predictor_l = LE_16(&input[0]);
  SE_16BIT(predictor_l);
  index_l = input[2];
  if (channels == 2)
  {
    predictor_r = LE_16(&input[4]);
    SE_16BIT(predictor_r);
    index_r = input[6];
  }

  if (channels == 1)
    for (i = 0;i < (block_size - MS_IMA_ADPCM_PREAMBLE_SIZE * channels); i++)
    {
      output[i * 2 + 0] = input[MS_IMA_ADPCM_PREAMBLE_SIZE + i] & 0x0F;
      output[i * 2 + 1] = input[MS_IMA_ADPCM_PREAMBLE_SIZE + i] >> 4;
    }
  else
  {
    // encoded as 8 nibbles (4 bytes) per channel; switch channel every
    // 4th byte
    channel_counter = 0;
    channel_index_l = 0;
    channel_index_r = 1;
    channel_index = channel_index_l;
    for (i = 0;
      i < (block_size - MS_IMA_ADPCM_PREAMBLE_SIZE * channels); i++)
    {
      output[channel_index + 0] =
        input[MS_IMA_ADPCM_PREAMBLE_SIZE * 2 + i] & 0x0F;
      output[channel_index + 2] =
        input[MS_IMA_ADPCM_PREAMBLE_SIZE * 2 + i] >> 4;
      channel_index += 4;
      channel_counter++;
      if (channel_counter == 4)
      {
        channel_index_l = channel_index;
        channel_index = channel_index_r;
      }
      else if (channel_counter == 8)
      {
        channel_index_r = channel_index;
        channel_index = channel_index_l;
        channel_counter = 0;
      }
    }
  }
  
  decode_nibbles(output,
    (block_size - MS_IMA_ADPCM_PREAMBLE_SIZE * channels) * 2,
    channels,
    predictor_l, index_l,
    predictor_r, index_r);

  return (block_size - MS_IMA_ADPCM_PREAMBLE_SIZE * channels) * 2;
}
#define MSADPCM_ADAPT_COEFF_COUNT   7
static int AdaptationTable [] =
{	230, 230, 230, 230, 307, 409, 512, 614,
768, 614, 512, 409, 307, 230, 230, 230
} ;

/* TODO : The first 7 coef's are are always hardcode and must
appear in the actual WAVE file.  They should be read in
in case a sound program added extras to the list. */

static int AdaptCoeff1 [MSADPCM_ADAPT_COEFF_COUNT] =
{	256, 512, 0, 192, 240, 460, 392
} ;

static int AdaptCoeff2 [MSADPCM_ADAPT_COEFF_COUNT] =
{	0, -256, 0, 64, 0, -208, -232
} ;

static int ms_adpcm_decode_block(short *pcm_buf, unsigned char *buf, int channel, int block)
{
	int sampleblk = 2036;
    short bpred[2];
    short idelta[2];
    int blockindx=0;
    int sampleindx=0;
    short bytecode = 0;
    int predict = 0;
    int current = 0;
    int delta = 0;
    int i = 0;
	int j = 0;
    short s0 = 0;
    short s1 = 0;
    short s2 = 0;
    short s3 = 0;
    short s4 = 0;
    short s5 = 0;

    //sampleblk = sample_block;
		j = 0;
		if(channel==1)
    {
        bpred[0] = buf[0];
        bpred[1] = 0;
        if(bpred[0]>=7)
        {
            //printf("sync error\n");
            //goto _exit;
        }
        idelta[0] = buf[1]|buf[2]<<8;
        idelta[1] = 0;

        s1 = buf[3]|buf[4]<<8;        
        s0 = buf[5]|buf[6]<<8;

        blockindx = 7;
        sampleindx = 2;
    }
    else if(channel==2)
    {
        bpred[0] = buf[0];
        bpred[1] = buf[1];
        if(bpred[0]>=7 || bpred[1]>=7)
        {        
            //printf("sync error\n");
            //goto _exit;
        }
        idelta[0] = buf[2]|buf[3]<<8;
        idelta[1] = buf[4]|buf[5]<<8;
 
        s2 = buf[6]|buf[7]<<8;
        s3 = buf[8]|buf[9]<<8;
        s0 = buf[10]|buf[11]<<8;
        s1 = buf[12]|buf[13]<<8;
        blockindx = 14;
        sampleindx = 4;
		 }

    /*--------------------------------------------------------
    This was left over from a time when calculations were done
    as ints rather than shorts. Keep this around as a reminder
    in case I ever find a file which decodes incorrectly.
    
      if (chan_idelta [0] & 0x8000)
      chan_idelta [0] -= 0x10000 ;
      if (chan_idelta [1] & 0x8000)
      chan_idelta [1] -= 0x10000 ;
    --------------------------------------------------------*/
    
    /* Pull apart the packed 4 bit samples and store them in their
    ** correct sample positions.
    */

    /* Decode the encoded 4 bit samples. */
    int chan;
    
    for(i=channel*2;/*i<channel*sampleblk&&*/(blockindx < block);i++)
    {
        if(sampleindx<=i)
        {
            if(blockindx<block)
            {
                bytecode = buf[blockindx++];

          
                if(channel==1)
                {
                    s2 = (bytecode>>4)&0x0f;
                    s3 = bytecode&0x0f;
                }
                else if(channel==2)
                {
                    s4 = (bytecode>>4)&0x0f;
                    s5 = bytecode&0x0f;
                }
                sampleindx++;
                sampleindx++;

            }
        }
        chan = (channel>1)?(i%2):0;
   
        if(channel==1)
        {
            bytecode = s2&0x0f;
        }
        else if(channel==2)
        {
            bytecode = s4&0x0f;
        }
        /* Compute next Adaptive Scale Factor (ASF) */
        delta = idelta[chan];

        /* => / 256 => FIXED_POINT_ADAPTATION_BASE == 256 */
        idelta[chan] = (AdaptationTable[bytecode]*delta)>>8;

        if(idelta[chan]<16)
            idelta[chan]=16;
        if(bytecode&0x8)
            bytecode-=0x10;
         /* => / 256 => FIXED_POINT_COEFF_BASE == 256 */

        if(channel==1)
        {
            predict = s1*AdaptCoeff1[bpred[chan]];
            predict+= s0*AdaptCoeff2[bpred[chan]];
        }
        else if(channel==2)
        {
            predict = s2*AdaptCoeff1[bpred[chan]];
            predict+= s0*AdaptCoeff2[bpred[chan]];
        }

        predict>>=8;
        current = bytecode*delta+predict;        
#if 1       
        if (current > 32767)
            current = 32767 ;
        else if (current < -32768)
            current = -32768 ;
#else
		current = _min(current, 32767);
		current = _max(current, -32768);
#endif
        if(channel==1)
        {
            s2 = current;
				}
        else if(channel==2)
        {
            s4 = current;
				}
 
		pcm_buf[j++] = s0;

        if(channel==1)
        {
            s0 = s1;
            s1 = s2;
            s2 = s3;
        }
        else if(channel==2)
        {
            s0 = s1;
            s1 = s2;
            s2 = s3;
            s3 = s4;
            s4 = s5;
        }
    }

    if(channel==1)
    {
        pcm_buf[j++] = s0;
		pcm_buf[j++] = s1;
		}
    else if(channel==2)
    {
        pcm_buf[j++] = s0;
        pcm_buf[j++] = s1;
        pcm_buf[j++] = s2;
        pcm_buf[j++] = s3;
    }

	return j;
}


/*
 * u-law, A-law and linear PCM conversions.
 */

#define SIGN_BIT (0x80)  /* Sign bit for a A-law byte. */
#define QUANT_MASK (0xf)  /* Quantization field mask. */
#define NSEGS  (8)  /* Number of A-law segments. */
#define SEG_SHIFT (4)  /* Left shift for segment number. */
#define SEG_MASK (0x70)  /* Segment field mask. */
//static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
#define BIAS  (0x84)  /* Bias for linear code. */

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
int alaw2linear(unsigned char a_val)
{
	int  t;
	int  seg;
	a_val ^= 0x55;
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
 	switch (seg) {
 		case 0:
  		t += 8;
 	 	break;
 	case 1:
  		t += 0x108;
  		break;
 	default:
  		t += 0x108;
  		t <<= seg - 1;
 	}
 	return ((a_val & SIGN_BIT) ? t : -t);
}
/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
int ulaw2linear(unsigned char u_val)
{
	 int  t;
	 /* Complement to obtain normal u-law value. */
	 u_val = ~u_val;
	 /*
	  * Extract and bias the quantization bits. Then
	  * shift up by the segment number and subtract out the bias.
	  */
	 t = ((u_val & QUANT_MASK) << 3) + BIAS;
	 t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
	 return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

int runalawdecoder(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	int i= 0;
	int tmp = 0;
	short *pcm_buf = (short*)wave_decoder_buffer[1].addr;	

	tmp = refill(audec,pcm_read_ctx,pwavebuf, WAVE_BLOCK_SIZE);
	if(tmp<0)
       return -1;
	for(i=0; i<tmp; i++)
	{
		pcm_buf[i] = alaw2linear(pwavebuf[i]);			
	}
	memcpy(buf,(char*)pcm_buf,2*WAVE_BLOCK_SIZE);
	return (WAVE_BLOCK_SIZE)*2;
}
int runulawdecoder(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	int i= 0;
	short *pcm_buf = (short*)wave_decoder_buffer[1].addr;
	int tmp = 0;

	tmp = refill(audec,pcm_read_ctx,pwavebuf, WAVE_BLOCK_SIZE);
    if(tmp<0)
       return -1;
	for(i=0; i<tmp; i++)
	{
		pcm_buf[i] = ulaw2linear(pwavebuf[i]);			
	}
	memcpy(buf,(char*)pcm_buf,2*WAVE_BLOCK_SIZE);
	return (WAVE_BLOCK_SIZE)*2;

}
int runimaadpcmdecoder(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	short *pcm_buf = (short*)wave_decoder_buffer[1].addr;
	int Output_Size = 0;
	int tmp = 0;
	char buffer[5];
	unsigned  block_size = 0;
    int UsedDataLenSave=0;
	if (!block_align)
	{   
         CHECK_DATA_ENOUGH_SET(pcm_read_ctx,4,0)	
         pcm_read(pcm_read_ctx,buffer,4);
		 while(1){
			 if((buffer[0] == 0x11)&&(buffer[1] == 0x22)&&(buffer[2] == 0x33)&&(buffer[3] == 0x44)){//sync word
				break;
			 }
             CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,1,3)	
			 pcm_read(pcm_read_ctx,&buffer[4],1);
			 memmove(buffer,&buffer[1],4);
		 }
         CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,2,4)	
		 pcm_read(pcm_read_ctx,buffer,2);

		 block_size = (buffer[0]<< 8) | buffer[1];
         CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,block_size,6)
	}else{
		 block_size = block_align;
         CHECK_DATA_ENOUGH_SET(pcm_read_ctx,block_size,0)
    }
    
	if(block_size < 4){
		PRINTF("[%s %d]imaadpcm block align not valid: %d\n",__FUNCTION__,__LINE__,block_size);
		return 0;
	}
	
    UsedDataLenSave=pcm_read_ctx->UsedDataLen;
	tmp = refill(audec,pcm_read_ctx,pwavebuf, block_size);
    if(tmp<0){
       pcm_read_ctx->UsedDataLen=UsedDataLenSave;
       return -1;
    }

	if(tmp!=block_size)
		PRINTF("[%s %d]imaadpcm: data missalign\n",__FUNCTION__,__LINE__);
	Output_Size = ima_adpcm_decode_block((unsigned short *)pcm_buf, pwavebuf, g_mgr.ch, block_size);
	memcpy(buf,(char*)pcm_buf,2*Output_Size);
	return Output_Size*2;

}

int runmsadpcmdecoder(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	short *pcm_buf = (short*)wave_decoder_buffer[1].addr;
	int Output_Size = 0;
	unsigned tmp = 0;
	char buffer[5];
	unsigned  block_size = 0;
	int UsedDataLenSave=0;
	if (!block_align)
	{   
        CHECK_DATA_ENOUGH_SET(pcm_read_ctx,4,0)
        pcm_read(pcm_read_ctx,buffer,4);
		while(1){
			if((buffer[0] == 0x11)&&(buffer[1] == 0x22)&&(buffer[2] == 0x33)&&(buffer[3] == 0x44)){//sync word
				break;
			}
            CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,1,3)
            pcm_read(pcm_read_ctx,&buffer[4],1);
			memmove(buffer,&buffer[1],4);
		}

        CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,2,4)
        pcm_read(pcm_read_ctx,buffer,2);
		block_size = (buffer[0]<< 8) | buffer[1];
        CHECK_DATA_ENOUGH_SUB(pcm_read_ctx,block_size,6)
	}else{
		 block_size = block_align;
         CHECK_DATA_ENOUGH_SET(pcm_read_ctx,block_size,0)
    }

	if(block_size < 4){
		PRINTF("[%s %d]msadpcm block align not valid: %d\n",__FUNCTION__,__LINE__,block_size);
		return 0;
	}

    UsedDataLenSave=pcm_read_ctx->UsedDataLen;
	tmp = refill(audec,pcm_read_ctx,pwavebuf, block_size);
    if(tmp<0){ 
         pcm_read_ctx->UsedDataLen=UsedDataLenSave;
         return -1;

    }
	if(tmp!=block_size)
		PRINTF("[%s %d]msadpcm: data missalign\n",__FUNCTION__,__LINE__);

	Output_Size = ms_adpcm_decode_block(pcm_buf, pwavebuf, g_mgr.ch, block_size);
	Output_Size = Output_Size - Output_Size%g_mgr.ch; 
	memcpy(buf,(char*)pcm_buf,2*Output_Size);
	return Output_Size*2;

}


enum SampleFormat {
    SAMPLE_FMT_NONE = -1,
    SAMPLE_FMT_U8,              ///< unsigned 8 bits
    SAMPLE_FMT_S16,             ///< signed 16 bits
    SAMPLE_FMT_S32,             ///< signed 32 bits
    SAMPLE_FMT_FLT,             ///< float
    SAMPLE_FMT_DBL,             ///< double
    SAMPLE_FMT_NB               ///< Number of sample formats. DO NOT USE if dynamically linking to libavcodec
};

int runpcmdecoder(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	int i/*, j*/;
	short *pcm_buf = (short*)wave_decoder_buffer[1].addr;
	int tmp = 0;
       offset = 0;
	if(g_mgr.bps == SAMPLE_FMT_U8)
	{
		tmp = refill(audec,pcm_read_ctx,pwavebuf, WAVE_BLOCK_SIZE);
        if(tmp<0) return -1;
		for(i=0;i<tmp;)
		{
			pcm_buf[i] = (pwavebuf[i]-0x80)<<8; i++;
			pcm_buf[i] = (pwavebuf[i]-0x80)<<8; i++;
			pcm_buf[i] = (pwavebuf[i]-0x80)<<8; i++;
			pcm_buf[i] = (pwavebuf[i]-0x80)<<8; i++;
		}
		return (WAVE_BLOCK_SIZE)*2;
	}else{
		if(refill(audec,pcm_read_ctx,pwavebuf, WAVE_BLOCK_SIZE)<0)
             return -1;;
		return ((WAVE_BLOCK_SIZE>>1)*2);
	}
}

static int adpcm_decode_frame(aml_audio_dec_t *audec,pcm_read_ctl_t *pcm_read_ctx,unsigned char *buf, int len)
{
	int buf_size=0;
	pwavebuf = (unsigned char*)wave_decoder_buffer[0].addr;

	switch(audec->codec_id)
	{		
		case CODEC_ID_PCM_ALAW:
			buf_size=runalawdecoder(audec,pcm_read_ctx,buf,len);
			break;
		
		case CODEC_ID_PCM_MULAW:
			buf_size=runulawdecoder(audec,pcm_read_ctx,buf,len);
			break;
		
		case CODEC_ID_ADPCM_IMA_WAV:
			buf_size=runimaadpcmdecoder(audec,pcm_read_ctx,buf,len);
			break;
			
		case CODEC_ID_ADPCM_MS:
			buf_size=runmsadpcmdecoder(audec,pcm_read_ctx,buf,len);
			break;
		default:
			buf_size=runpcmdecoder(audec,pcm_read_ctx,buf,len);
			break;		
	}
	return  buf_size;
}

static int adpcm_decode_release(void)
{
	if(wave_decoder_buffer[0].addr)
	{
        free(wave_decoder_buffer[0].addr);
		wave_decoder_buffer[0].addr=0;
		wave_decoder_buffer[0].size=0;
	}
	if(wave_decoder_buffer[1].addr)
	{
        free(wave_decoder_buffer[1].addr);
		wave_decoder_buffer[1].addr=0;
		wave_decoder_buffer[1].size=0;
	}
	return 0;
}

int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
      aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
      pcm_read_ctl_t pcm_read_ctl={0};
      pcm_read_init(&pcm_read_ctl,inbuf,inlen);
      *outlen= adpcm_decode_frame(audec,&pcm_read_ctl,outbuf, *outlen);
      return pcm_read_ctl.UsedDataLen;

}
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
    PRINTF("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
    adpcm_init(audec);
    return 0;
}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
    adpcm_decode_release();
    return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    return 0;
}


