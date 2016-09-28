
#include "sp_dec.h"
#include "interf_dec.h"
#include "dec_if.h"
#include "../../amadec/adec-armdec-mgt.h"
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "AmrDecoder"
#define amr_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define amr_print  printf
#endif 
#define DefaultReadSize  2*1024
#define DefaultOutBufSize 16*1024

typedef void (*_AMR_DECODE_FRAME)(void*,short*,short *outlen, char *inbuf, int *consume);

static 	_AMR_DECODE_FRAME amr_decode_frame_fun;
static int SampleRateOut=0;
static void * destate=NULL;
static const short amrnb_block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };

#if 0
int amr_read(unsigned char *pBuffer, int n)
{
	int i;
	i = read_buffer(pBuffer, n);
	return i;
}
#endif

void * Decoder_Init()
{
    if(SampleRateOut == 8000)
        return(void *)Decoder_Interface_init();
    else
        return (void *)D_IF_init();
}
void amrnb_decode_frame(void* destate, short* pOutBuffer,short *outlen, char *inbuf, int *consume)
{
    enum Mode dec_mode;
    int read_size;
    unsigned char analysis[32];   
    char *tpbuf;
    tpbuf = inbuf;
	
    memset(analysis,0,32);
    //amr_read(analysis, 1);
    analysis[0] = *tpbuf++;
	
    dec_mode = (analysis[0] >> 3) & 0x000F;
    read_size = amrnb_block_size[dec_mode];

    //note: the logic modification of this time refer from <opencore_souce code>:
    //     when  (dec_mode==15) and (read_size==0), ie,dec_mode==RX_NO_DATA
    //    still need to send data   to decoder:
    //amr_read(&analysis[1], read_size );
    memcpy(&analysis[1], tpbuf, read_size );
    *consume = read_size + 1;
    /* call decoder */
    Decoder_Interface_Decode(destate, analysis, pOutBuffer, 0);
}

//note:maybe this decoder logic  has some problem and too old now ,but can not find where
//   its souce comes from, so just let maitain current situations!!: 
void amrwb_decode_frame(void* destate, short* pOutBuffer,short *outlen, char *inbuf, int *consume)
{
    unsigned char serial[61];
    short mode;
    char *tpbuf;
    tpbuf = inbuf;
        
    static const short amrwb_block_size[16]= {18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1};
    //amr_read(serial, 1);
    serial[0] = *tpbuf++;

    mode = (short)((serial[0] >> 3) & 0x0F);

    if (amrwb_block_size[mode] - 1 > 0){
        //amr_read(&serial[1], amrwb_block_size[mode] - 1 );
        memcpy(&serial[1], tpbuf, amrwb_block_size[mode] - 1);
        tpbuf += amrwb_block_size[mode] - 1;
        D_IF_decode( destate, serial, pOutBuffer, 0);
    }
    else{
        *outlen=-1;
        amr_print("[%s %d]decoder err!\n",__FUNCTION__,__LINE__);
        // empty frame??? need clean history???
        // memset(pOutBuffer, 0, 320*2);
    }
	*consume += tpbuf - inbuf;
}
//static int amr_decode_frame(unsigned char *buf, int maxlen, struct frame_fmt *fmt)
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
	//amr_print("[%s:%d]enter into amr_decoder\n", __FUNCTION__,__LINE__);
    int consume = 0;
    short synth[320];
    int outputSample;
    short out_ret=0;
    memset(synth,0,320*sizeof(short));
    if(SampleRateOut == 8000)
    {
        outputSample = 160;
    }
    else
    {
        outputSample = 160*2;
    }
    amr_decode_frame_fun(destate, synth, &out_ret, inbuf, &consume);
	
    if(out_ret<0){
        *outlen = 0;
        return consume;
    }
    memcpy(outbuf, (char*)synth, outputSample*2);
    *outlen=outputSample*2;
    return consume;
}


//static int amr_decode_init(struct frame_fmt * fmt)
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
	//struct audio_info *real_data;
    //real_data = (struct audio_info *)fmt->private_data;
    amr_print("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
    if(adec_ops->samplerate == 16000)
        SampleRateOut=16000;
    else
        SampleRateOut=8000;
    destate=NULL;
    destate = Decoder_Init();
    if(SampleRateOut == 8000)
        amr_decode_frame_fun = amrnb_decode_frame;
    else
        amr_decode_frame_fun = amrwb_decode_frame;
	//fmt->valid=CHANNEL_VALID | SAMPLE_RATE_VALID | DATA_WIDTH_VALID;
	//fmt->sample_rate = SampleRateOut;
	//fmt->channel_num = real_data->channels;
	//fmt->data_width = 16;
	adec_ops->nInBufSize = DefaultReadSize;
	adec_ops->nOutBufSize = DefaultOutBufSize;
	amr_print("amr %s ,sr %d,ch num %d\n",(SampleRateOut == 8000)?"NB":"WB", adec_ops->samplerate, adec_ops->channels);
	return 0;
}

void Decoder_exit(void *st)
{
    if(SampleRateOut == 8000)
        Decoder_Interface_exit(st);
    else
        D_IF_exit(st);
}

//static int amr_decode_release(void)
int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
	Decoder_exit(destate);
	return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    return 0;
}

#if 0
static struct codec_type amr_codec=
{
  .name="amr",
  .init=amr_decode_init,
  .release=amr_decode_release,
  .decode_frame=amr_decode_frame,
};


void __used amr_codec_init(void)
{
	amr_print("register amr lib \n");
    register_codec(&amr_codec);
}

CODEC_INIT(amr_codec_init);
#endif
