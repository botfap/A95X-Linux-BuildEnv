#include <stdio.h>
#include "intreadwrite.h"
#include "../../amadec/adec-armdec-mgt.h"
#include "../../amadec/audio-dec.h"
#include "../../amcodec/include/amports/aformat.h"
#include <sys/time.h>
#include <stdint.h>
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "PcmDecoder"
#define  PRINTF(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  PRINTF printf
#endif
/* Audio channel masks */
#define CH_FRONT_LEFT             0x00000001
#define CH_FRONT_RIGHT            0x00000002
#define CH_FRONT_CENTER           0x00000004
#define CH_LOW_FREQUENCY          0x00000008
#define CH_BACK_LEFT              0x00000010
#define CH_BACK_RIGHT             0x00000020
#define CH_FRONT_LEFT_OF_CENTER   0x00000040
#define CH_FRONT_RIGHT_OF_CENTER  0x00000080
#define CH_BACK_CENTER            0x00000100
#define CH_SIDE_LEFT              0x00000200
#define CH_SIDE_RIGHT             0x00000400
#define CH_TOP_CENTER             0x00000800
#define CH_TOP_FRONT_LEFT         0x00001000
#define CH_TOP_FRONT_CENTER       0x00002000
#define CH_TOP_FRONT_RIGHT        0x00004000
#define CH_TOP_BACK_LEFT          0x00008000
#define CH_TOP_BACK_CENTER        0x00010000
#define CH_TOP_BACK_RIGHT         0x00020000
#define CH_STEREO_LEFT            0x20000000  ///< Stereo downmix.
#define CH_STEREO_RIGHT           0x40000000  ///< See CH_STEREO_LEFT.
/* Audio channel convenience macros */
#define CH_LAYOUT_MONO              (CH_FRONT_CENTER)
#define CH_LAYOUT_STEREO            (CH_FRONT_LEFT|CH_FRONT_RIGHT)
#define CH_LAYOUT_2_1               (CH_LAYOUT_STEREO|CH_BACK_CENTER)
#define CH_LAYOUT_SURROUND          (CH_LAYOUT_STEREO|CH_FRONT_CENTER)
#define CH_LAYOUT_4POINT0           (CH_LAYOUT_SURROUND|CH_BACK_CENTER)
#define CH_LAYOUT_2_2               (CH_LAYOUT_STEREO|CH_SIDE_LEFT|CH_SIDE_RIGHT)
#define CH_LAYOUT_QUAD              (CH_LAYOUT_STEREO|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_5POINT0           (CH_LAYOUT_SURROUND|CH_SIDE_LEFT|CH_SIDE_RIGHT)
#define CH_LAYOUT_5POINT1           (CH_LAYOUT_5POINT0|CH_LOW_FREQUENCY)
#define CH_LAYOUT_5POINT0_BACK      (CH_LAYOUT_SURROUND|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_5POINT1_BACK      (CH_LAYOUT_5POINT0_BACK|CH_LOW_FREQUENCY)
#define CH_LAYOUT_7POINT0           (CH_LAYOUT_5POINT0|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_7POINT1           (CH_LAYOUT_5POINT1|CH_BACK_LEFT|CH_BACK_RIGHT)
#define CH_LAYOUT_7POINT1_WIDE      (CH_LAYOUT_5POINT1_BACK|CH_FRONT_LEFT_OF_CENTER|CH_FRONT_RIGHT_OF_CENTER)
#define CH_LAYOUT_STEREO_DOWNMIX    (CH_STEREO_LEFT|CH_STEREO_RIGHT)

#define CHECK_BLUERAY_PCM_HEADER
#define pcm_buffer_size     (1024*6)
#define bluray_pcm_size     (1024*17)
#define LOCAL   inline
#define SIGN_BIT        (0x80)
#define QUANT_MASK      (0xf)
#define NSEGS           (8)
#define SEG_SHIFT       (4)
#define SEG_MASK        (0x70)
#define BIAS            (0x84)
static short table[256];

typedef struct
{
   int ValidDataLen;
   int UsedDataLen;
   unsigned char *BufStart;
   unsigned char *pcur;
}pcm_read_ctl_t;

typedef struct 
{
    int pcm_channels;
    int pcm_samplerate;
    int bit_per_sample;
    int jump_read_head_flag;
    int frame_size_check;
    int frame_size_check_flag;
    int pcm_bluray_header;
    int pcm_bluray_size;
    unsigned char *pcm_buffer;
    int lpcm_header_parsed;
}pcm_priv_data_t;

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

static int av_get_bits_per_sample(int codec_id)
{
     switch(codec_id)
    {
     case AFORMAT_ALAW:
     case AFORMAT_MULAW:
     case AFORMAT_PCM_U8:
           return 8;
     case AFORMAT_PCM_S16BE:
     case AFORMAT_PCM_S16LE:
          return 16;
     default:
          return 0;
    }
}

static int alaw2linear(unsigned char a_val)
{
    int t;
    int seg;
    a_val ^= 0x55;
    t = a_val & QUANT_MASK;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    if(seg) 
       t= (t + t + 1 + 32) << (seg + 2);
    else    
       t= (t + t + 1) << 3;
    return (a_val & SIGN_BIT) ? t : -t;
}

static int ulaw2linear(unsigned char u_val)
{
    int t;
    u_val = ~u_val;
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
    return (u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS);
}

/**
 * Parse the header of a LPCM frame read from a MPEG-TS stream
 * @param avctx the codec context
 * @param header pointer to the first four bytes of the data packet
 */
static int pcm_bluray_pheader(pcm_read_ctl_t *pcm_read_ctl,aml_audio_dec_t *audec, char *header, int *bps)
{
    audio_decoder_operations_t *adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;

    uint8_t bits_per_samples[4] = { 0, 16, 20, 24 };
    uint32_t channel_layouts[16] = {
        0, CH_LAYOUT_MONO, 0, CH_LAYOUT_STEREO, CH_LAYOUT_SURROUND,
        CH_LAYOUT_2_1, CH_LAYOUT_4POINT0, CH_LAYOUT_2_2, CH_LAYOUT_5POINT0,
        CH_LAYOUT_5POINT1, CH_LAYOUT_7POINT0, CH_LAYOUT_7POINT1, 0, 0, 0, 0
    };
    uint8_t channels[16] = {
        0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0
    };
    int frame_size = header[0]<<8|header[1];
    uint8_t channel_layout = header[2] >> 4;
    int frame_header = header[0]<<24|header[1]<<16|header[2]<<8|header[3];

    if(frame_header != pPcm_priv_data->pcm_bluray_header){
         PRINTF("[%s %d]NOTE:pcm_bluray_header: header = %02x%02x%02x%02x\n",__FUNCTION__,__LINE__,header[0], header[1], header[2], header[3]);
         pPcm_priv_data->pcm_bluray_header = frame_header;
    }
    if(frame_size != pPcm_priv_data->pcm_bluray_size){
         PRINTF("[%s %d]bluray pcm frame size is %d\n",__FUNCTION__,__LINE__,frame_size);
         pPcm_priv_data->pcm_bluray_size = frame_size;
    }

    /* get the sample depth and derive the sample format from it */
    *bps = bits_per_samples[header[3] >> 6];
    if(*bps == 0){
        PRINTF("[%s %d]unsupported sample datawitth (0)\n",__FUNCTION__,__LINE__);
        return -1;
    }

    /* get the sample rate. Not all values are known or exist. */
    switch (header[2] & 0x0f) {
    case 1:
        pPcm_priv_data->pcm_samplerate= 48000;
        break;
    case 4:
        pPcm_priv_data->pcm_samplerate = 96000;
        break;
    case 5:
        pPcm_priv_data->pcm_samplerate = 192000;
        break;
    default:
        pPcm_priv_data->pcm_samplerate = 0;
        PRINTF( "[%s %d]unsupported sample rate (%d)\n",__FUNCTION__,__LINE__,header[2] & 0x0f);
        return -1;
    }

    /*
     * get the channel number (and mapping). Not all values are known or exist.
     * It must be noted that the number of channels in the MPEG stream can
     * differ from the actual meaningful number, e.g. mono audio still has two
     * channels, one being empty.
     */
    //avctx->channel_layout  = channel_layouts[channel_layout];
    pPcm_priv_data->pcm_channels=  channels[channel_layout];
    if (!pPcm_priv_data->pcm_channels) {
        PRINTF( "[%s %d]unsupported channel configuration (%d)\n",__FUNCTION__,__LINE__,channel_layout);
        return -1;
    }
    return frame_size;
}


#define Emphasis_Off                         0
#define Emphasis_On                          1
#define Quantization_Word_16bit              0
#define Quantization_Word_Reserved           0xff
#define Audio_Sampling_44_1                  1
#define Audio_Sampling_48                    2
#define Audio_Sampling_Reserved              0xff
#define Audio_channel_Dual_Mono              0
#define Audio_channel_Stero                  1
#define Audio_channel_Reserved               0xff
#define FramesPerAU                          80         //according to spec of wifi display
#define Wifi_Display_Private_Header_Size     4


static int parse_wifi_display_pcm_header(aml_audio_dec_t *audec, char *header, int *bps)
{
    char number_of_frame_header, audio_emphasis, quant, sample, channel;
    int frame_size = -1;
    audio_decoder_operations_t *adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;
    //check sub id
    if (header[0] == 0xa0)
    {
        number_of_frame_header = header[1];
        audio_emphasis = header[2] & 1;
        quant  = header[3] >> 6;
        sample = (header[3] >> 3) & 7;
        channel = header[3] & 7;
        
        if (quant == Quantization_Word_16bit)
        {
            *bps = 16;
        }else{
            PRINTF("[%s %d]using reserved bps %d\n",__FUNCTION__,__LINE__, *bps);
        }
        
        if (sample == Audio_Sampling_44_1)
        {
            pPcm_priv_data->pcm_samplerate = 44100;
        }else if(sample == Audio_Sampling_48){
            pPcm_priv_data->pcm_samplerate = 48000;
        }else{
            PRINTF("[%s %d]using reserved sample_rate %d\n",__FUNCTION__,__LINE__,audec->samplerate);
            pPcm_priv_data->pcm_samplerate=audec->samplerate;
        }
        
        if (channel == Audio_channel_Dual_Mono){
            pPcm_priv_data->pcm_channels= 1;   //note: this is not sure
        }else if(channel == Audio_channel_Stero){
            pPcm_priv_data->pcm_channels = 2;
        }else{
            PRINTF("using reserved channel %d\n", audec->channels);
            pPcm_priv_data->pcm_channels=audec->channels;
        }
        pPcm_priv_data->lpcm_header_parsed = 1;
        
        frame_size = FramesPerAU * (*bps >> 3) * audec->channels * number_of_frame_header;
    }else{
        PRINTF("[%s %d]unknown sub id\n",__FUNCTION__,__LINE__);
    }
    
    return frame_size;
}
/**
 * Read PCM samples macro
 * @param type Datatype of native machine format
 * @param endian bytestream_get_xxx() endian suffix
 * @param src Source pointer (variable name)
 * @param dst Destination pointer (variable name)
 * @param n Total number of samples (variable name)
 * @param shift Bitshift (bits)
 * @param offset Sample value offset
 */
#define DEF_T(type, name, bytes, read, write)                             \
    static  type bytestream_get_ ## name(const uint8_t **b){\
        (*b) += bytes;\
        return read(*b - bytes);\
    }\
    static  void bytestream_put_ ##name(uint8_t **b, const type value){\
        write(*b, value);\
        (*b) += bytes;\
    }
#define DEF(name, bytes, read, write)     DEF_T(unsigned int, name, bytes, read, write)


DEF  (le32, 4, AV_RL32, AV_WL32)
DEF  (le24, 3, AV_RL24, AV_WL24)
DEF  (le16, 2, AV_RL16, AV_WL16)
DEF  (be32, 4, AV_RB32, AV_WB32)
DEF  (be24, 3, AV_RB24, AV_WB24)
DEF  (be16, 2, AV_RB16, AV_WB16)
DEF  (byte, 1, AV_RB8 , AV_WB8 )

#define DECODE(type, endian, src, dst, n, shift, offset) \
    dst_##type = (type*)dst; \
    for(;n>0;n--) { \
        register type v = bytestream_get_##endian((const unsigned char**)&src); \
        *dst_##type++ = (v - offset) << shift; \
    } \
    dst = (short*)dst_##type;
    

static int pcm_init(aml_audio_dec_t *audec)
{
    int i;
    audio_decoder_operations_t *adec_ops=NULL;
    pcm_priv_data_t *pPcm_priv_data=NULL;
  //  PRINTF("[%s %d]audec=%x\n",__FUNCTION__,__LINE__,audec); 
    //PRINTF("[%s %d]audec->adec_ops=%x\n",__FUNCTION__,__LINE__,audec,audec->adec_ops);
    pPcm_priv_data=malloc(sizeof(pcm_priv_data_t));
    if(pPcm_priv_data==NULL){
        PRINTF("[%s %d]malloc memory failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    memset(pPcm_priv_data,0,sizeof(pcm_priv_data_t));
    adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    adec_ops->priv_dec_data=pPcm_priv_data;
    adec_ops->nInBufSize=pcm_buffer_size;
    adec_ops->nOutBufSize=0;
    pPcm_priv_data->pcm_buffer=malloc(bluray_pcm_size);
    if(pPcm_priv_data->pcm_buffer==NULL){
         free( adec_ops->priv_dec_data);
         adec_ops->priv_dec_data=NULL;
         PRINTF("[%s %d]malloc memory failed!\n",__FUNCTION__,__LINE__);
         return -1;
    }
    memset(pPcm_priv_data->pcm_buffer,0,bluray_pcm_size);

    PRINTF("[%s]audec->format/%d adec_ops->samplerate/%d adec_ops->channels/%d\n",
           __FUNCTION__,audec->format,adec_ops->samplerate,adec_ops->channels);

    pPcm_priv_data->pcm_samplerate=adec_ops->samplerate;
    pPcm_priv_data->pcm_channels  =adec_ops->channels;
    if(audec->format==AFORMAT_PCM_BLURAY)
    {
          pPcm_priv_data->pcm_bluray_size = 0;
          pPcm_priv_data->pcm_bluray_header = 0;
          pPcm_priv_data->frame_size_check=0;
          pPcm_priv_data->jump_read_head_flag=0;
          pPcm_priv_data->frame_size_check_flag=1;

          #ifdef CHECK_BLUERAY_PCM_HEADER 
          pPcm_priv_data->frame_size_check_flag=0;
          if(!audec->extradata || audec->extradata_size != 4){
              free(pPcm_priv_data->pcm_buffer);
              free(adec_ops->priv_dec_data);
              adec_ops->priv_dec_data=NULL;
              PRINTF("[%s %d] pcm_init failed: need extradata !\n",__FUNCTION__,__LINE__);
              return -1; 
          }
          memcpy(&pPcm_priv_data->pcm_bluray_header, audec->extradata, 4);
          PRINTF("[]blueray  frame 4 byte header[0x%x],[0x%x],[0x%x],[0x%x]\n", audec->extradata[0],audec->extradata[1],\
          audec->extradata[2],audec->extradata[3]);
          #endif

          return 0;
     }

     switch(audec->format){
        case AFORMAT_ALAW:
            for(i = 0; i < 256; i++){
                table[i] = alaw2linear(i);
            }
            break;
        case AFORMAT_MULAW:
            for(i = 0; i < 256; i++){
                table[i] = ulaw2linear(i);
            }
            break;
        default:
            break;
     }
     return 0;
}

#define CHECK_DATA_ENOUGH(Ctl,NeedBytes,UsedSetIfNo)    {                  \
     if((Ctl)->ValidDataLen < (NeedBytes)){                                \
           PRINTF("[%s %d]NOTE--> no enough data\n",__FUNCTION__,__LINE__);\
           (Ctl)->UsedDataLen-=(UsedSetIfNo);                              \
           return -1;                                                      \
      }                                                                    \              
}

           

static int check_frame_size(pcm_read_ctl_t *pcm_read_ctl,aml_audio_dec_t *audec ,int *bps)
{
    int index;
    int find_header_cnt=0,first_header_pos=-1;
    int size ;
    int header,frame_size;
    int first_head_pos=0;
    audio_decoder_operations_t *adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;
    
    CHECK_DATA_ENOUGH(pcm_read_ctl,4,0)
    pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, 4);

    index=0;
    while(1)
    {  
        header = (pPcm_priv_data->pcm_buffer[0]<<24) | (pPcm_priv_data->pcm_buffer[1]<<16) | \
                 (pPcm_priv_data->pcm_buffer[2]<<8 ) | (pPcm_priv_data->pcm_buffer[3]);
        if(header == pPcm_priv_data->pcm_bluray_header)
              break;
        pPcm_priv_data->pcm_buffer[0] = pPcm_priv_data->pcm_buffer[1];
        pPcm_priv_data->pcm_buffer[1] = pPcm_priv_data->pcm_buffer[2];
        pPcm_priv_data->pcm_buffer[2] = pPcm_priv_data->pcm_buffer[3];
        CHECK_DATA_ENOUGH(pcm_read_ctl,1,3)
        pcm_read(pcm_read_ctl,&pPcm_priv_data->pcm_buffer[3], 1);
        index++;
    }
   
    first_head_pos=pcm_read_ctl->UsedDataLen-4;
    PRINTF("[%s %d]First BluRay Header offset=%d\n",__FUNCTION__,__LINE__,first_head_pos);
    //---------------------------------------------

    index=1;
    if(!pcm_read(pcm_read_ctl,&pPcm_priv_data->pcm_buffer[4], 1)){
         PRINTF("[%s %d]NOTE--> no enough data\n",__FUNCTION__,__LINE__);
         pcm_read_ctl->UsedDataLen=first_head_pos;
         return -1;
    }

    while(1)
    {
         header=(pPcm_priv_data->pcm_buffer[index]<<24) | (pPcm_priv_data->pcm_buffer[index+1]<<16) | \
                (pPcm_priv_data->pcm_buffer[index+2]<<8)| (pPcm_priv_data->pcm_buffer[index+3]);
         if(header == pPcm_priv_data->pcm_bluray_header){
              pPcm_priv_data->frame_size_check_flag=1;
              pPcm_priv_data->frame_size_check=index-4;
              pPcm_priv_data->jump_read_head_flag=1;
              PRINTF("frame_size_check=%d\n",pPcm_priv_data->frame_size_check);
              break;
         }

         if(!pcm_read(pcm_read_ctl,&pPcm_priv_data->pcm_buffer[index+4], 1)){
             PRINTF("[%s %d]NOTE--> no enough data\n",__FUNCTION__,__LINE__);
             pcm_read_ctl->UsedDataLen=first_head_pos;
             return -1;
         }
         index++;
    }
    
    frame_size = pcm_bluray_pheader(pcm_read_ctl,audec, pPcm_priv_data->pcm_buffer, bps);
    memmove(pPcm_priv_data->pcm_buffer,&pPcm_priv_data->pcm_buffer[4],pPcm_priv_data->frame_size_check);
    if(frame_size!=pPcm_priv_data->frame_size_check){
        PRINTF("[%s %d]WARNING-->STREAM_ERR:frame_size!=frame_size_check %d/%d\n ",
                    __FUNCTION__,__LINE__,frame_size,pPcm_priv_data->frame_size_check);
        frame_size=pPcm_priv_data->frame_size_check;
    }
    return frame_size;

}

static int pcm_decode_frame(pcm_read_ctl_t *pcm_read_ctl,unsigned char *buf, int len, aml_audio_dec_t *audec)
{
    short *sample;
    unsigned char *src;
    int size=0, n, i, j, bps=16, wifi_display_drop_header = 0;
    int sample_size;
    unsigned int header;
    int16_t *dst_int16_t;
    int32_t *dst_int32_t;
    int64_t *dst_int64_t;
    uint16_t *dst_uint16_t;
    uint32_t *dst_uint32_t;
    int frame_size;
    audio_decoder_operations_t *adec_ops=(audio_decoder_operations_t *)audec->adec_ops;
    pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;

    sample = (short *)buf;
    src = pPcm_priv_data->pcm_buffer;

    if(audec->format == AFORMAT_PCM_BLURAY){
        int i,j;
        short tmp_buf[10];
        if(!pPcm_priv_data->frame_size_check_flag){
            frame_size=check_frame_size(pcm_read_ctl,audec,&bps);
            if(frame_size<0)
               return 0;
        }else{
            if(pPcm_priv_data->jump_read_head_flag==1)
            {
                frame_size=pPcm_priv_data->frame_size_check;
                pPcm_priv_data->jump_read_head_flag=0;
            }else if(pPcm_priv_data->jump_read_head_flag==0){

                #ifdef CHECK_BLUERAY_PCM_HEADER 
                if(pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, 4)==0)
                    return 0;
                while(1){
                    header = (pPcm_priv_data->pcm_buffer[0]<<24) | (pPcm_priv_data->pcm_buffer[1]<<16) | \
                             (pPcm_priv_data->pcm_buffer[2]<<8)  | (pPcm_priv_data->pcm_buffer[3]);
                    if(header == pPcm_priv_data->pcm_bluray_header){
                        break;
                    }
                    pPcm_priv_data->pcm_buffer[0] = pPcm_priv_data->pcm_buffer[1];
                    pPcm_priv_data->pcm_buffer[1] = pPcm_priv_data->pcm_buffer[2];
                    pPcm_priv_data->pcm_buffer[2] = pPcm_priv_data->pcm_buffer[3];
                    if(!pcm_read(pcm_read_ctl,&pPcm_priv_data->pcm_buffer[3], 1)){
                         pcm_read_ctl->UsedDataLen-=3;
                         return 0;
                    }       
                }
                #else
                if(!pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, 4))
                     return 0;
                #endif

                frame_size = pcm_bluray_pheader(pPcm_priv_data,audec, pPcm_priv_data->pcm_buffer, &bps);
                pPcm_priv_data->bit_per_sample=bps;
                if(frame_size < 960 || frame_size > 17280){
                     PRINTF("STREAM_ERR:illegal blueray frame size \n");
                     return 0;
                }

                #ifdef CHECK_BLUERAY_PCM_HEADER 
                if(frame_size!=pPcm_priv_data->frame_size_check){
                    frame_size=pPcm_priv_data->frame_size_check;
                } 
                #endif

            }
            if(bps>0) pPcm_priv_data->bit_per_sample=bps;

            if(!pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, frame_size)){
                 pcm_read_ctl->UsedDataLen-=4;
                 return 0;
            }
        }

        if(pPcm_priv_data->bit_per_sample == 24){
            if(pPcm_priv_data->pcm_channels == 6){
                for(i=0,j=0; i<frame_size; ){
                    //short L, R, C, LS, RS, N;
                    *(tmp_buf)   = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=3;  //L
                    *(tmp_buf+1) = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=3;  //R    
                    *(tmp_buf+2) = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=3;   //C
                    i+=9;
                    sample[j++] = (*(tmp_buf)>>1) + ((*(tmp_buf+2))>>1);
                    sample[j++] = (*(tmp_buf+1)>>1) + ((*(tmp_buf+2))>>1);
                }
            }else if(pPcm_priv_data->pcm_channels == 2){
                for(i=0,j=0; i<frame_size; ){
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=3;
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=3;
                }
            }
        }else if(pPcm_priv_data->bit_per_sample == 16){
            if(pPcm_priv_data->pcm_channels == 1){
                for(i=0,j=0; i<frame_size; ){
                    sample[j+1] = sample[j] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; 
                    i+=2;
                    j+=2;
                }
            }else if(pPcm_priv_data->pcm_channels == 2){
                for(i=0,j=0; i<frame_size; ){
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;
                }
            }else if(pPcm_priv_data->pcm_channels== 6){
                for(i=0,j=0; i<frame_size; ){
                    *(tmp_buf)   = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;              //L
                    *(tmp_buf+1) = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;       //R    

                    *(tmp_buf+2) =(pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1];  i+=2;       //C
                    i+=6;
                    sample[j++] = (*(tmp_buf)>>1)  + (*(tmp_buf+2)  >> 1);
                    sample[j++] = (*(tmp_buf+1)>>1)  + (*(tmp_buf+2)  >> 1);
                }
            }
            else if(pPcm_priv_data->pcm_channels== 8){
                for(i=0,j=0; i<frame_size; ){
                    *(tmp_buf)   = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;                           //L
                    *(tmp_buf+1) = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;                       //R                 
                    *(tmp_buf+2) = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;                         //C
                    i+=10;
                    sample[j++] = (*(tmp_buf)>>1)  + (*(tmp_buf+2)  >> 1);
                    sample[j++] = (*(tmp_buf+1)>>1) + (*(tmp_buf+2)  >> 1);
                }
            }
        }else{
            PRINTF("[%s %d]blueray pcm is 20bps, don't process now\n",__FUNCTION__,__LINE__);
            return 0;
        }
        
        return j*2;
    }
    else if (audec->format == AFORMAT_PCM_WIFIDISPLAY)
    {
        //check audio info for wifi display LPCM, by zefeng.tong@amlogic.com
        if(!pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, Wifi_Display_Private_Header_Size))
              return 0;
        
        if (pPcm_priv_data->pcm_buffer[0] == 0xa0)
        {
            frame_size = parse_wifi_display_pcm_header(audec, pPcm_priv_data->pcm_buffer, &bps);
            if(!pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, frame_size)){
                 pcm_read_ctl->UsedDataLen-=Wifi_Display_Private_Header_Size;
                 return -1;
            }
            pPcm_priv_data->bit_per_sample=bps;
        }else{
            frame_size = Wifi_Display_Private_Header_Size;    //transimit error or something?
        }

        if(pPcm_priv_data->bit_per_sample== 16){
            if(pPcm_priv_data->pcm_channels== 1){
                for(i=0,j=0; i<frame_size; ){
                    sample[j+1] = sample[j] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1];
                    i+=2;
                    j+=2;
                }
            }else if(pPcm_priv_data->pcm_channels == 2){
                for(i=0,j=0; i<frame_size; ){
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;
                    sample[j++] = (pPcm_priv_data->pcm_buffer[i]<<8)|pPcm_priv_data->pcm_buffer[i+1]; i+=2;
                }
            }
            return frame_size;
        }else{
            PRINTF("[%s %d]wifi display:unimplemented bps %d\n",__FUNCTION__,__LINE__, bps);
        }
        
        return 0;
    }
    
    unsigned byte_to_read = pcm_buffer_size;
    while(pcm_read_ctl->ValidDataLen < byte_to_read) {
        byte_to_read = byte_to_read>>1;
    }
    size = pcm_read(pcm_read_ctl,pPcm_priv_data->pcm_buffer, byte_to_read);
    
    sample_size = av_get_bits_per_sample(audec->format)/8;
    n = size /sample_size;

    switch(audec->format){
        case AFORMAT_ALAW:
        case AFORMAT_MULAW:
             for(; n > 0; n--)
                 *sample++ = table[*src++];
                //dbuf.data_size = size *2;
                size *=2;
            break;
                
        case AFORMAT_PCM_S16LE:
            {
                int i,j;
                short tmp_buf[10];
                short* p;
                p = (short*)pPcm_priv_data->pcm_buffer;
                if(pPcm_priv_data->pcm_channels == 6){
                    for(i=0,j=0; (i + 6 ) < (size/2); ){
                        *(tmp_buf) = p[i];      //L
                        *(tmp_buf+1) = p[i+1];  //R    
                        *(tmp_buf+2)  = p[i+2]; //C
                        i+=6;

                        sample[j++] = (*(tmp_buf)>>1)  + (*(tmp_buf+2)  >> 1);
                        sample[j++] = (*(tmp_buf+1)>>1)  + (*(tmp_buf+2)  >> 1);
                    }
                    size = 2*j;
                }else{
                    memcpy(sample, src, n*sample_size);
                }
                //dbuf.data_size = size;

            }
            break;

        case AFORMAT_PCM_S16BE:
                DECODE(int16_t, be16, src, sample, n, 0, 0)
            break;

        case AFORMAT_PCM_U8:
                for(;n>0;n--) {
                        *sample++ = ((int)*src++ - 128) << 8;
                }
                //dbuf.data_size = size*2;
                size *=2;
            break;
            
        default:
            PRINTF("[%s %d]Not support format audec->format=%d!\n",__FUNCTION__,__LINE__,audec->format);
            break;
    }

    return size;
}


int audio_dec_init(audio_decoder_operations_t *adec_ops)
{  
     aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
    // PRINTF("\n\n[%s]audec--%x",__FUNCTION__,audec);	 
     PRINTF("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
     return pcm_init(audec);
    
}

int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
      aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
      pcm_read_ctl_t pcm_read_ctl={0};
       pcm_read_init(&pcm_read_ctl,inbuf,inlen);
      *outlen=pcm_decode_frame(&pcm_read_ctl,outbuf, *outlen, audec);
      return pcm_read_ctl.UsedDataLen;

}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
    pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;
    if(pPcm_priv_data!=NULL && pPcm_priv_data->pcm_buffer){
         free(pPcm_priv_data->pcm_buffer);
         free(pPcm_priv_data);
    }
    adec_ops->priv_dec_data=NULL;
    return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
     pcm_priv_data_t *pPcm_priv_data=(pcm_priv_data_t *)adec_ops->priv_dec_data;
    ((AudioInfo *)pAudioInfo)->channels = pPcm_priv_data->pcm_channels>2?2:pPcm_priv_data->pcm_channels;
    ((AudioInfo *)pAudioInfo)->samplerate = pPcm_priv_data->pcm_samplerate;
     return 0;
}



