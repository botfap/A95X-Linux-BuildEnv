/*
** @file
 * Vorbis I decoder
 * @author lianlian zhu 
 *
 * This file is part of audio_codec.

*this decoder use ffmpeg decoder api and integret into amlogic
* libplayer
*/

#include "vorbisdec.h"
#define audio_codec_print printf

static int vorbis_read_init(vorbis_read_ctl_t *vorbis_read_ctx, unsigned char* inbuf, int size)
{
    vorbis_read_ctx->ValidDataLen = size;
    vorbis_read_ctx->UsedDataLen = 0;
    vorbis_read_ctx->BufStart    = inbuf;
    vorbis_read_ctx->pcur        = inbuf;
    return 0;
}

static int vorbis_read(vorbis_read_ctl_t *vorbis_read_ctx, unsigned char* outbuf, int size)
{
    int bytes_read = 0;
    if (size <= vorbis_read_ctx->ValidDataLen) {
        memcpy(outbuf, vorbis_read_ctx->pcur, size);
        vorbis_read_ctx->ValidDataLen -= size;
        vorbis_read_ctx->UsedDataLen  += size;
        vorbis_read_ctx->pcur         += size;
        bytes_read = size;
    }
    return bytes_read;
}

int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    audio_codec_print("\n\n[%s]BuildDate--%s  BuildTime--%s", __FUNCTION__, __DATE__, __TIME__);

    adec_ops->nInBufSize = VORBIS_INFRAME_BUFSIZE;
    adec_ops->nOutBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    av_register_all();
    ic = avcodec_alloc_context();
    if (!ic) {
            audio_codec_print("AVCodec Memory error\n");
            goto release;
    }
    ic->codec_id       = CODEC_ID_VORBIS;
    ic->codec_type     = AVMEDIA_TYPE_AUDIO;
    ic->channels = adec_ops->channels;
    ic->sample_rate =adec_ops->samplerate;
    ic->bit_rate = adec_ops->bps;
    ic->extradata = adec_ops->extradata;
    ic->extradata_size = adec_ops->extradata_size;
	
#if 0
        FILE *fp2= fopen("/data/audio_out3.extradata","a+"); 
        if(fp2 ){ 
        int flen=fwrite(adec_ops->extradata ,1,adec_ops->extradata_size ,fp2); 
        audio_codec_print("-- p.size=%d ,p.data = %p-n", adec_ops->extradata_size,adec_ops->extradata);
        fclose(fp2); 
        }else{
        audio_codec_print("could not open file:audio_out3.vorbis");
        }
#endif
    audio_codec_print("adec_ops->extradata_size:%d,adec_ops->extradata:%s\n",adec_ops->extradata_size,adec_ops->extradata);
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
    vorbis_read_ctl_t vorbis_read_ctl = {0};
    int SyncFlag=0;
    int pkt_size=0,len=0,bytes =0;
    int framelen =0 ;
    uint8_t ptr_head[4]={0};
    unsigned char *sample ;
    AVPacket p;
    sample = (unsigned char *)outbuf;
    char *outdata;
    outdata = av_malloc(VORBIS_OUTFRAME_BUFSIZE);
    if(!outdata){
       audio_codec_print("malloc vorbis outdata failed");
       return 0;
    }
    vorbis_read_init(&vorbis_read_ctl, inbuf, inlen);
	
    while (vorbis_read_ctl.ValidDataLen) {
        SyncFlag=0,bytes=0; 
        framelen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        if(vorbis_read(&vorbis_read_ctl,ptr_head,4)<4) {
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
            if(vorbis_read(&vorbis_read_ctl,&ptr_head[3],1)<1){
                bytes=bytes + 3;
                audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
                break;
            }   
        }
        if(vorbis_read(&vorbis_read_ctl,(unsigned char*)(&pkt_size),4) < 4) {
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
    
        if(vorbis_read(&vorbis_read_ctl,(unsigned char*)(p.data),pkt_size) < pkt_size){
            //audio_codec_print("WARNING: vorbis_read readbytes failed [%s %d]!\n",__FUNCTION__,__LINE__);
            bytes =8;
            break;
        }
#if 0
        FILE *fp2= fopen("/data/audio_out3.vorbis","a+"); 
        if(fp2 ){ 
        int flen=fwrite(p.data,1,p.size,fp2); 
       // audio_codec_print("p.data = %p--- p.size=%d \n", p.data,  p.size);
        fclose(fp2); 
        }else{
        audio_codec_print("could not open file:audio_out3.vorbis");
        }
#endif
        memset(outdata,0,VORBIS_OUTFRAME_BUFSIZE);
        len = avcodec_decode_audio3(ic, (short *)outdata,&framelen, &p);
        if(framelen > VORBIS_OUTFRAME_BUFSIZE || framelen < 0) {
            audio_codec_print("framelen > VORBIS_OUTFRAME_BUFSIZE or < 0\n");
            framelen = VORBIS_OUTFRAME_BUFSIZE;
        }
        if (len < 0) {
            audio_codec_print("Error while decoding\n");
            break;

        }
        memcpy(sample,outdata,framelen);
        sample = sample + framelen;

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
    return vorbis_read_ctl.UsedDataLen - bytes;

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


