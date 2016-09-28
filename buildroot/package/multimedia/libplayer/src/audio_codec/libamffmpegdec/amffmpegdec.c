/*
** @file
 * amffmpegdec  decoder
 * @author lianlian zhu 
 *
 * This file is part of audio_codec.

*this decoder use ffmpeg decoder api and integret into amlogic
* libplayer
*/

#include "amffmpegdec.h"
#define audio_codec_print printf
//#include <android/log.h>
//#define  LOG_TAG    "amffmpegdec"
//#define  audio_codec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static int amffmpeg_read_init(amffmpeg_read_ctl_t *amffmpeg_read_ctx, unsigned char* inbuf, int size)
{
    amffmpeg_read_ctx->ValidDataLen = size;
    amffmpeg_read_ctx->UsedDataLen = 0;
    amffmpeg_read_ctx->BufStart    = inbuf;
    amffmpeg_read_ctx->pcur        = inbuf;
    return 0;
}

static int amffmpeg_read(amffmpeg_read_ctl_t *amffmpeg_read_ctx, unsigned char* outbuf, int size)
{
    int bytes_read = 0;
    if (size <= amffmpeg_read_ctx->ValidDataLen) {
        memcpy(outbuf, amffmpeg_read_ctx->pcur, size);
        amffmpeg_read_ctx->ValidDataLen -= size;
        amffmpeg_read_ctx->UsedDataLen  += size;
        amffmpeg_read_ctx->pcur         += size;
        bytes_read = size;
    }
    return bytes_read;
}

int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    audio_codec_print("\n\n[%s]BuildDate--%s  BuildTime--%s", __FUNCTION__, __DATE__, __TIME__);
    aml_audio_dec_t *audec = (aml_audio_dec_t *)(adec_ops->priv_data);
    adec_ops->nInBufSize = AMFFMPEG_INFRAME_BUFSIZE;
    adec_ops->nOutBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    av_register_all();
    ic = avcodec_alloc_context();
    if (!ic) {
            audio_codec_print("AVCodec Memory error\n");
            goto release;
    }

    ic->codec_id       = audec->codec_id;
    ic->codec_type     = AVMEDIA_TYPE_AUDIO;
    ic->channels = audec->channels;
    ic->sample_rate =audec->samplerate;
    ic->bit_rate = audec->bitrate;
    ic->extradata = audec->extradata;
    ic->extradata_size = audec->extradata_size;
    ic->block_align = audec->block_align;
    
    if (audec->codec_id == CODEC_ID_WMAV1 || audec->codec_id == CODEC_ID_WMAV2 || 
        audec->codec_id == CODEC_ID_WMAPRO ){
        Asf_audio_info_t *paudio_info=(Asf_audio_info_t*)audec->extradata;
        ic->extradata_size = paudio_info->extradata_size;
        ic->extradata = paudio_info->extradata;
        ic->block_align = paudio_info->block_align;
    } 
    codec = avcodec_find_decoder(ic->codec_id);
    if (!codec) {
            audio_codec_print("Codec not found\n");
            goto release;
        }
	if (avcodec_open(ic, codec) < 0) {
            audio_codec_print("Could not open codec\n");
            goto release;
        }
		return 0;
release:
		if (ic) {
            audio_codec_print("AVCodec close\n");
            avcodec_close(ic);
            av_free(ic);
        }
		 return -1;
}
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
    amffmpeg_read_ctl_t amffmpeg_read_ctl = {0};
    int SyncFlag=0,codec_id = 0;
    int pkt_size=0,len=0,bytes =0;
    int framelen =0 ;
    uint8_t ptr_head[4]={0};
    unsigned char *sample ;
    AVPacket pkt,p;
    codec_id = ic->codec_id;
    sample = (unsigned char *)outbuf;
    char *outdata;
    outdata = av_malloc(AMFFMPEG_OUTFRAME_BUFSIZE);
    if(!outdata){
       audio_codec_print("malloc amffmpeg outdata failed");
       return 0;
    }
    amffmpeg_read_init(&amffmpeg_read_ctl, inbuf, inlen);

    while (amffmpeg_read_ctl.ValidDataLen) {
        SyncFlag=0,bytes=0; 
        framelen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
       // decode begin
        if(codec_id == CODEC_ID_VORBIS){
            if(amffmpeg_read(&amffmpeg_read_ctl,ptr_head,4)<4) {
            //audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
            break;
            }
            
            while(!SyncFlag) {
                if(ptr_head[0]=='H' && ptr_head[1]=='E' && ptr_head[2]=='A' && ptr_head[3]=='D'){
                    SyncFlag=1;
                    break;
                }
                ptr_head[0]=ptr_head[1];
                ptr_head[1]=ptr_head[2];
                ptr_head[2]=ptr_head[3];
                if(amffmpeg_read(&amffmpeg_read_ctl,&ptr_head[3],1)<1){
                    bytes=bytes + 3;
                    audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
                    break;
                }   
            }
            if(amffmpeg_read(&amffmpeg_read_ctl,(unsigned char*)(&pkt_size),4) < 4) {
                //audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
                if(SyncFlag)
                    bytes =4; 
                break;
            }
            if(pkt_size > 8192) {
                audio_codec_print("pkt_size > 8192\n");
                break;

            }
            p.data = indata;
            memset(indata,0,8192);	 
            p.size =  pkt_size ;
            pkt.data = NULL;
		// audio_codec_print("WARNING: vorbis_read p.size=%d [%s %d]!\n",p.size ,__FUNCTION__,__LINE__);	
            //pkt.data = av_malloc(pkt.size);
            if(amffmpeg_read(&amffmpeg_read_ctl,(unsigned char*)(p.data),p.size) < pkt_size){
                //audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
                bytes =8;
                break;
            }
        }else if(codec_id == CODEC_ID_WMAV1 || codec_id == CODEC_ID_WMAV2
        ||codec_id == CODEC_ID_WMAPRO){
            pkt.data = av_malloc(ic->block_align);
            pkt.size =  ic->block_align;
            //audio_codec_print("1.pkt.data:%p,pkt.size:%d\n",pkt.data,pkt.size);
            if(amffmpeg_read(&amffmpeg_read_ctl,(unsigned char*)(pkt.data),pkt.size) < pkt.size){
                //audio_codec_print("WARNING: wma_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
                break;
            }
        }else {
            audio_codec_print(".. to be supported");
            return -1;
        }
#if 0
        FILE *fp2= fopen("/data/audio_out3.dat","a+"); 
        if(fp2 ){ 
        int flen=fwrite(pkt.data,1,pkt.size,fp2); 
        audio_codec_print("pkt.data = %p--- pkt.size=%d \n", pkt.data,pkt.size);
        fclose(fp2); 
        }else{
        audio_codec_print("could not open file:audio_out3.vorbis");
        }
#endif
        memset(outdata,0,AMFFMPEG_OUTFRAME_BUFSIZE);
        if(codec_id == CODEC_ID_WMAPRO){
            char *buf;
            buf = pkt.data;
            while (pkt.size){
                framelen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                len = avcodec_decode_audio3(ic, (short *)outdata,&framelen, &pkt);
                int i =0,channels=0;
                short *samples;
                float *sampletmp;
                framelen = framelen/2;
                samples = av_malloc(framelen);
                sampletmp = outdata;
                for(;i< framelen/2;i++){
                    int32_t y = (int32_t)(sampletmp[i] * (1<<15) + 0.5f);
                    samples[i] =   Clip(y,-32768,32767);
                }
                //audio_codec_print("framelen:%d,len:%d\n",framelen,len);
                if(framelen > AMFFMPEG_OUTFRAME_BUFSIZE || framelen < 0) {
                    audio_codec_print("framelen > AMFFMPEG_OUTFRAME_BUFSIZE or < 0\n");
                    framelen = AMFFMPEG_OUTFRAME_BUFSIZE;      
                }
                if (len < 0) {
                    audio_codec_print("Error while decoding\n");
                    break;
                } 
                memcpy(sample,samples,framelen);
                if(samples){
                    av_free(samples);
                    samples = NULL;
                }
                sample = sample + framelen;
                pkt.size = pkt.size -len;
                pkt.data = pkt.data+len;
            }
            pkt.data = buf;

        }else{
            if(codec_id == CODEC_ID_VORBIS)
                len = avcodec_decode_audio3(ic, (short *)outdata,&framelen, &p);	
		else
               len = avcodec_decode_audio3(ic, (short *)outdata,&framelen, &pkt);
            //audio_codec_print("framelen:%d,len:%d\n",framelen,len);
            if(framelen > AMFFMPEG_OUTFRAME_BUFSIZE || framelen < 0) {
                audio_codec_print("framelen > AMFFMPEG_OUTFRAME_BUFSIZE or < 0\n");
                framelen = AMFFMPEG_OUTFRAME_BUFSIZE;
           
            }
            if (len < 0) {
                audio_codec_print("Error while decoding\n");
                break;
            }
            memcpy(sample,outdata,framelen);
            sample = sample + framelen;
        }
        if(pkt.data){
            av_free(pkt.data);
            pkt.data = NULL;
        }

    }   
    *outlen = sample - (unsigned char *)outbuf;
#if 0
        FILE *fp3= fopen("/data/audio_out3.pcm","a+"); 
        if(fp3 ){ 
        int flen=fwrite((char *)outbuf,1,*outlen,fp3); 
        audio_codec_print("outbuf = %p--- *outlen=%d \n", outbuf,  *outlen);
        fclose(fp3); 
        }else{
        audio_codec_print("could not open file:audio_out3.pcm");
        }
#endif
    
    if(outdata){
        av_free(outdata);
        outdata = NULL;
    }
    return amffmpeg_read_ctl.UsedDataLen - bytes;

}
int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
    audio_codec_print("vorbis audio_dec_release\n");
    if (ic) {
        audio_codec_print("AVCodec close\n");
        avcodec_close(ic);
        av_free(ic);
    }
    return 0;
}
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    //audio_codec_print("audio_dec_getinfo\n");
    ((AudioInfo *)pAudioInfo)->channels =adec_ops->channels>2?2:adec_ops->channels;;
    ((AudioInfo *)pAudioInfo)->samplerate = adec_ops->samplerate;
    return 0;
}
