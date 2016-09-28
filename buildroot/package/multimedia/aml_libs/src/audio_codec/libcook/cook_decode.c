#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


#include "cook_codec.h"
//#include <asm/cache.h>

#ifndef __MW__
#include <inttypes.h>
#else
#include <core/types.h>
#endif
#include "cook_decode.h"
#include "rm_parse.h"
#include "ra_depack.h"
#include "ra_decode.h"



/********arm decoder header file **********/

#include "../../amadec/adec-armdec-mgt.h"
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "CookDecoder"
#define libcook_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define libcook_print  printf
#endif 
#define DefaultReadSize  32*1024
#define DefaultOutBufSize 370*1024


typedef struct
{
    rm_parser*          pParser;
    ra_depack*          pDepack;
    ra_decode*          pDecode;
    ra_format_info*     pRaInfo;
    rm_packet*          pPacket;
    ra_block*           pBlock;
    rm_stream_header*   pHdr;
    UINT32              usStreamNum;
} rm_info_t;

typedef struct
{
	BYTE *buf;
	int buf_len;
	int buf_max;
	int cousume;
	int all_consume;
} cook_IObuf;

struct frame_info
{
    int len;
    unsigned long  offset;/*steam start to here*/
    unsigned long  buffered_len;/*data buffer in  dsp,pcm datalen*/
    int reversed[1];/*for cache aligned 32 bytes*/
};

static cook_IObuf cook_input;
static cook_IObuf cook_output;

struct frame_info  cur_frame;
static ra_decoder_info_t ra_dec_info ;
static rm_info_t ra_info ;
static char file_header[AUDIO_EXTRA_DATA_SIZE];
#pragma align_to(64,file_header)
static char *cur_read_ptr = file_header;


static void rm_error(void* pError, HX_RESULT result, const char* pszMsg)
{
    //dsp_mailbox_send(1,M1B_IRQ7_DECODE_FATAL_ERR, 1, NULL, 0);
    //trans_err_code(DECODE_FATAL_ERR);
    libcook_print("rm_error pError=0x%08x result=0x%08x msg=%s\n", pError, result, pszMsg);
    //while(1);
}

//static int cook_decode_frame(unsigned char *buf, int maxlen, struct frame_fmt *fmt)
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
	//libcook_print("audio_dec_decode, inlen = %d, cook_input.buf_len = %d\n", inlen, cook_input.buf_len);
    HX_RESULT retVal = HXR_OK;
    UINT32 ulBytesConsumed = 0;
    UINT32 ulTotalConsumed = 0;
    UINT32 ulBytesLeft     = 0;
    UINT32 ulNumSamplesOut = 0;
    UINT32 ulMaxSamples    = 0;
    UINT32 out_data_len = 0;

	//ra_dec_info.input_buf = buf;
	//ra_dec_info.input_buffer_size = maxlen;
	if(inlen > 0){
		int len = inlen - cook_input.buf_len;
		int read_len;
		read_len = (inlen > cook_input.buf_max) ? cook_input.buf_max : inlen;
		if(len > 0){
			int temp=memcpy(cook_input.buf + cook_input.buf_len,
				inbuf+cook_input.buf_len, read_len-cook_input.buf_len);
			cook_input.buf_len += read_len-cook_input.buf_len;
		}
	}
	//libcook_print("audio_dec_decoder cook_input.buf_len = %d\n", cook_input.buf_len);
	while(cook_output.buf_len <= 0){
		retVal = rm_parser_get_packet(ra_info.pParser, &ra_info.pPacket);
        if (retVal == HXR_OK) {
            //if (ra_info.pPacket->usStream == ra_info.usStreamNum)
            {
                retVal = ra_depack_add_packet(ra_info.pDepack, ra_info.pPacket);

            }
            rm_parser_destroy_packet(ra_info.pParser, &ra_info.pPacket);
        }
        if (retVal != HXR_OK){
        	libcook_print("cook_decode_frame£º add packet failed\n");	
			break;
        }
		if(cook_input.buf_len <= 2048)
			break;
	}
	
	*outlen = 0;
	if(cook_output.buf_len > 0){
		memcpy(outbuf, cook_output.buf, cook_output.buf_len);
		(*outlen) = cook_output.buf_len;
		cook_output.buf_len = 0;
	}
	
	int ret = 0;
	//ret = ra_dec_info.decoded_size + cook_input.cousume;
	ret = cook_input.cousume;
	//libcook_print("ret decoder = %d\n", ret);
	cook_input.cousume = 0;
	adec_ops->pts = cur_frame.offset;
	cur_frame.offset = 0;
	return ret;
}
static UINT32 rm_io_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{
    memcpy(pBuf,cur_read_ptr,ulBytesToRead);
    cur_read_ptr = cur_read_ptr + ulBytesToRead;
    if((unsigned)(cur_read_ptr - file_header) > AUDIO_EXTRA_DATA_SIZE){
		//trans_err_code(DECODE_INIT_ERR);	
		libcook_print("warning :: cook.read byte exceed the the buffer then sent,%d \n",(unsigned)(cur_read_ptr - file_header) );
		while(1);
		
    }		
    return ulBytesToRead;
 	
}
static void rm_io_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
        if (ulOrigin == HX_SEEK_ORIGIN_CUR)
            cur_read_ptr += ulOffset;
        else if (ulOrigin == HX_SEEK_ORIGIN_SET)
        	cur_read_ptr =(char *)pUserRead + ulOffset;
        else if (ulOrigin == HX_SEEK_ORIGIN_END)
                cur_read_ptr = sizeof(file_header)+(char *)pUserRead-ulOffset;
    	if((unsigned)(cur_read_ptr - file_header) > AUDIO_EXTRA_DATA_SIZE){
		//trans_err_code(DECODE_INIT_ERR);	
		libcook_print("warning :: cook.seek buffer pos exceed the the buffer then sent,%d \n",(unsigned)(cur_read_ptr - file_header) );
		while(1);
    	}	

}
static unsigned rm_ab_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{	
	int ret = 0;
	//libcook_print("rm_ab_read, ulBytesToRead = %d\n", ulBytesToRead);
	if(pBuf&&ulBytesToRead){
		//libcook_print("enter into copy data, buf_len = %d\n",cook_input.buf_len);
		if(cook_input.buf_len >= ulBytesToRead)//ret =  read_buffer(pBuf,ulBytesToRead);
		{
			memcpy(pBuf, cook_input.buf, ulBytesToRead);
			ret = ulBytesToRead;
			cook_input.buf_len -= ret;
			//libcook_print("inpoint2 = %d,  cook_input.buf_len = %d\n", cook_input.buf, cook_input.buf_len);
			memcpy(cook_input.buf, cook_input.buf + ret, cook_input.buf_len);
		}else{
			libcook_print("rm_ab_read data is not enough \n");
		}
	}
	cook_input.cousume += ret;
	cook_input.all_consume += ret;
	return ret;	
}
static void rm_ab_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
	//libcook_print("rm_ab_seek, buf_len = %d\n",cook_input.buf_len);
    int i;	
    if (ulOrigin == HX_SEEK_ORIGIN_CUR){
		if(ulOffset <= cook_input.buf_len){
			//memcpy(cook_input.buf, cook_input.buf + ulOffset, cook_input.buf_len - ulOffset);
			cook_input.buf_len -= ulOffset;
			cook_input.cousume += ulOffset;
			int i;
			BYTE * tmpbuf;
			tmpbuf = cook_input.buf + ulOffset;
			for(i = 0; i < cook_input.buf_len; i++){
				cook_input.buf[i] = tmpbuf[i];
			}
		}else{
			libcook_print("rm_ab_seek failed\n");
		}
#if 0
		for(i = 0;i <ulOffset;i++)
    		;//read_byte();
#endif
    }
	else if(/*ulOrigin == HX_SEEK_ORIGIN_SET*/0){
		int offbuf = ulOffset - cook_input.all_consume;
		if(offbuf > 0){
			if(cook_input.buf_len > offbuf){
				memcpy(cook_input.buf, cook_input.buf + offbuf,
					cook_input.buf_len - offbuf);
				cook_input.buf_len -= offbuf;
				cook_input.cousume += offbuf;
				cook_input.all_consume += offbuf;
			}
			else{
				libcook_print("data is not enough\n");
			}
		}
	}
}
static HX_RESULT _ra_block_available(void* pAvail, UINT32 ulSubStream, ra_block* pBlock)
{
	//libcook_print("[%s,%d] enter into _ra_block_available\n", __FUNCTION__,__LINE__);
	int len = 0, wlen;
	int offset = 0;
	HX_RESULT retVal = HXR_OK;
	UINT32 ulBytesConsumed = 0;
	UINT32 ulTotalConsumed = 0;
	UINT32 ulBytesLeft     = 0;
	UINT32 ulNumSamplesOut = 0;
	UINT32 ulMaxSamples    = 0;
	UINT32 out_data_len = 0;
	UINT32 delay_pts= 0;
	//BYTE*   buf = ra_dec_info.input_buf;//passed from the audio dec thread
    	rm_info_t* pInfo = (rm_info_t*) pAvail;
   	 if (pAvail && pBlock && pBlock->pData && pBlock->ulDataLen){
		ulBytesLeft = pBlock->ulDataLen;
	        if (retVal == HXR_OK && ulBytesLeft)
	        {
			retVal = ra_decode_decode(ra_dec_info.pDecode,
	                                 pBlock->pData+ ulTotalConsumed,
	                                pBlock->ulDataLen- ulTotalConsumed,
	                                  &ulBytesConsumed,
	                                  (UINT16*)ra_dec_info.pOutBuf,
	                                  ra_dec_info.ulOutBufSize / 2,
	                                  &ulNumSamplesOut,
                                  	pBlock->ulDataFlags,
                                  	pBlock->ulTimestamp*90+1);
			
			if (retVal == HXR_OK){
				 if (ulBytesConsumed){
	                		ulBytesLeft -= ulBytesConsumed;
							ulTotalConsumed += ulBytesConsumed;
				 }
				 if (ulNumSamplesOut){
					len += ulNumSamplesOut*2;
				 }
			}
			else if (retVal == HXR_NO_DATA){
				libcook_print("ra decode not enough data.\n");
				return 0;
			}
			else{
				libcook_print("ra decode error.\n");
				return 0;
			}
		}
		//ra_dec_info.decoded_size = out_data_len;
		ra_dec_info.decoded_size = ulBytesConsumed;
    }
#if 0
	FILE * fp1= fopen("/data/audio_out","a+"); 
	if(fp1){ 
		int flen=fwrite((char *)ra_dec_info.pOutBuf,1,len,fp1); 
		libcook_print("flen = %d---outlen=%d ", flen, len);
		fclose(fp1); 
	}else{
		libcook_print("could not open file:audio_out");
	}
#endif
	cur_frame.offset = pBlock->ulTimestamp*90 +1;
	cur_frame.buffered_len = 0;
	cur_frame.len= 0;
	delay_pts = (ulNumSamplesOut/ra_info.pRaInfo->usNumChannels)*90 /(ra_info.pRaInfo->ulSampleRate / 1000);
    cur_frame.offset += delay_pts;
	//refresh_swap_register1(cur_frame.offset+delay_pts, len);
//dsp_mailbox_send(1,M1B_IRQ4_DECODE_FINISH_FRAME,wlen,&cur_frame,sizeof(cur_frame));
date_trans:
	//wlen = write_buffer(ra_dec_info.pOutBuf + offset, len);
	memcpy(cook_output.buf + cook_output.buf_len, ra_dec_info.pOutBuf, len);
	cook_output.buf_len += len;
	len = 0;
#if 0
	wlen = memcpy(cook_output.buf + cook_output.buf_len, ra_dec_info.pOutBuf + offset, len);
	if(wlen > 0){
		offset += wlen;
		cook_output.buf_len += wlen;
		len -= wlen;
	}
	if(len > 0)
		goto date_trans;
#endif	
	//dsp_mailbox_send(1,M1B_IRQ4_DECODE_FINISH_FRAME,wlen,&cur_frame,sizeof(cur_frame));
    	return retVal;
}
//static int cook_decode_init(struct frame_fmt * fmt)
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
	HX_RESULT retVal = HXR_OK;
	unsigned ulNumStreams = 0;
	rm_parser*          pParser = HXNULL;
	rm_stream_header *pHdr = HXNULL;
	ra_depack *pRADpack = HXNULL;
	ra_format_info *pRAInfo = HXNULL;
	unsigned ulCodec4CC = 0;
	int i;	
	struct audio_info real_data;
	libcook_print("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
	real_data.bitrate = adec_ops->bps;
	real_data.channels = adec_ops->channels;
	real_data.extradata_size = adec_ops->extradata_size;
	real_data.sample_rate = adec_ops->samplerate;

	adec_ops->nInBufSize = DefaultReadSize;
	adec_ops->nOutBufSize = DefaultOutBufSize;
	
	memset(real_data.extradata, 0, AUDIO_EXTRA_DATA_SIZE);
	//libcook_print("%d,%d\n",real_data.extradata_size,adec_ops->extradata_size);
	for(i = 0; i < real_data.extradata_size; i++){
		real_data.extradata[i] = adec_ops->extradata[i];
	}


	libcook_print("cook audioinfo four data [0x%x],	[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],\n",real_data.extradata[0],\
	real_data.extradata[1],real_data.extradata[2],real_data.extradata[3], \
	real_data.extradata[4],real_data.extradata[5],real_data.extradata[6],real_data.extradata[7]);

	memcpy(file_header, real_data.extradata, AUDIO_EXTRA_DATA_SIZE);
    cur_frame.offset = 0;
	if(cook_input.buf == NULL){
		cook_input.buf = (BYTE *)malloc(DefaultReadSize * sizeof(BYTE));
		if(cook_input.buf != NULL){
			memset(cook_input.buf, 0, DefaultReadSize);
			cook_input.buf_len = 0;
			cook_input.buf_max = DefaultReadSize;
			cook_input.cousume = 0;
			cook_input.all_consume = 0;
		}
		else{
			libcook_print("inbuf malloc failed\n");
			return -1;
		}
	}
	if(cook_output.buf == NULL){
		cook_output.buf = (BYTE *)malloc(DefaultOutBufSize * sizeof(BYTE));
		if(cook_output.buf != NULL){
			memset(cook_output.buf, 0, DefaultOutBufSize);
			cook_output.buf_len = 0;
			cook_output.buf_max = DefaultOutBufSize;
			cook_output.cousume = 0;
		}else{
			libcook_print("outbuf malloc failed\n");
			return -1;
		}
	}

	if(cook_input.buf == NULL || cook_output.buf == NULL){
		libcook_print("malloc buf failed\n");
		return -1;
	
	}
	/**********wait for implement*******/
	//dsp_cache_wback((unsigned)file_header,AUDIO_EXTRA_DATA_SIZE);


	/*clear up the decoder structure */
	
	memset(&ra_dec_info,0,sizeof(ra_decoder_info_t));	
	memset(&ra_info,0,sizeof(rm_info_t));
	cur_read_ptr = file_header;	
	
	/* Create the parser struct */
	pParser = rm_parser_create(NULL, rm_error);
	if (!pParser)
	{
		libcook_print("[cook decode],create parser failed\n");
		return -1;
	}
	/* Set the stream into the parser */
#if 0	
	for(i = 0;i < AUDIO_EXTRA_DATA_SIZE/8;i++)
	{
	printk("%d header data  [0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x]\n\t",i,\
	file_header[i*8+0],file_header[i*8+1],file_header[i*8+2],file_header[i*8+3],\
	file_header[i*8+4],file_header[i*8+5],file_header[i*8+6],file_header[i*8+7]);
	}
#endif	
	retVal = rm_parser_init_io(pParser, file_header, rm_io_read, rm_io_seek);
	if (retVal != HXR_OK)
	{
		libcook_print("[cook decode], parser init IO failed\n");
		rm_parser_destroy(&pParser);
		return -1;
	}
	/* Read all the headers at the beginning of the .rm file */
	retVal = rm_parser_read_headers(pParser);
	if (retVal != HXR_OK)
	{
		libcook_print("[cook decode], parser read header failed\n");
		rm_parser_destroy(&pParser);
		return -1;
	}
	libcook_print(" rm_parser_read_headers finished \n");

	/* Get the number of streams */
	ulNumStreams = rm_parser_get_num_streams(pParser);
	if (ulNumStreams == 0)
	{
		libcook_print("[cook decode], no stream found\n");
		rm_parser_destroy(&pParser);
		return -1;
	}
	for (i = 0; i < ulNumStreams && retVal == HXR_OK; i++)
	{
		retVal = rm_parser_get_stream_header(pParser, i, &pHdr);
		if (retVal == HXR_OK)
		{
			if (rm_stream_is_realaudio(pHdr))
			{
				/* Create the RealAudio depacketizer */
				pRADpack = ra_depack_create((void*)pParser,_ra_block_available,NULL,rm_error);
				if (!pRADpack)
				{
					libcook_print("[cook decode], create depack failed\n");
					rm_parser_destroy_stream_header(pParser, &pHdr);
					rm_parser_destroy(&pParser);
					return -1;
				}
				/* Initialize the RA depacketizer with the stream header */
				retVal = ra_depack_init(pRADpack, pHdr);
				if (retVal != HXR_OK)
				{
					libcook_print("[cook decode],init depack failed\n");
					ra_depack_destroy(&pRADpack);
					rm_parser_destroy_stream_header(pParser, &pHdr);
					rm_parser_destroy(&pParser);
					return -1;
				}
				/*
				* Get the codec 4CC of substream 0. We 
				* arbitrarily choose substream 0 here.
				*/
				ulCodec4CC = ra_depack_get_codec_4cc(pRADpack, 0);
				if (ulCodec4CC == 0x636F6F6B) /* cook */
				{
					retVal = ra_depack_get_codec_init_info(pRADpack, 0, &pRAInfo);
					ra_info.pRaInfo = pRAInfo;

				}
				else if ((ulCodec4CC == 0x72616163)||(ulCodec4CC == 0x72616370)) 
				/* raac racp */
				{
					retVal = ra_depack_get_codec_init_info(pRADpack, 0, &pRAInfo);
					ra_info.pRaInfo = pRAInfo;
				}
				ra_info.pDepack = pRADpack;

			}
			rm_parser_destroy_stream_header(pParser, &pHdr);
		}
		libcook_print("cook rm_parser_get_stream_header finished\n");

	}
	/* Set the stream into the parser */
	//retVal = rm_parser_init_io(pParser, 0, rm_ab_read, rm_ab_seek);
	retVal = rm_parser_init_io(pParser, 0, rm_ab_read, rm_ab_seek);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			ra_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
		}
		libcook_print("[cook decode],rm_parser_init_io failed,errid %d\n",retVal);
		return -1;		
	}		
	ra_info.pParser = pParser;
	rm_parser_set_stream(&pParser, 0); 
	rm_parser_file_seek(pParser, 0);
	ra_dec_info.pDecode = ra_decode_create(HXNULL, rm_error);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			ra_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
		}
		libcook_print("[cook decode],ra_decode_create failed,errid %d\n",retVal);
		return -1;		
	}
	ra_dec_info.ulStatus = RADEC_PLAY;
	ra_dec_info.ulTotalSample = 0;
	ra_dec_info.ulTotalSamplePlayed = 0;
	UINT32 ulMaxSamples = 0;
	if (ra_dec_info.pOutBuf)
	{    
		ra_decode_reset(ra_dec_info.pDecode,
		(UINT16*)ra_dec_info.pOutBuf,
		ra_dec_info.ulOutBufSize / 2,
		&ulMaxSamples);
	}
	retVal = ra_decode_init(ra_dec_info.pDecode, ulCodec4CC, HXNULL, 0, ra_info.pRaInfo);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			ra_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
		}
		libcook_print("[cook decode],ra_decode_init failed,errid %d\n",retVal);
		return -1;		
	}
	if (ra_dec_info.pOutBuf)
	{
		ra_decode_getmaxsize(ra_dec_info.pDecode, &ulMaxSamples);
		if (ulMaxSamples * sizeof(UINT16)>ra_dec_info.ulOutBufSize)
		{
			free(ra_dec_info.pOutBuf);
			ra_dec_info.ulOutBufSize = ulMaxSamples * sizeof(UINT16);
			ra_dec_info.pOutBuf = (BYTE*) malloc(ra_dec_info.ulOutBufSize);
		}
	}
	else
	{
		ra_decode_getmaxsize(ra_dec_info.pDecode, &ulMaxSamples);
		ra_dec_info.ulOutBufSize = ulMaxSamples * sizeof(UINT16);
		if(ra_dec_info.ulOutBufSize > 0)
		{
			ra_dec_info.pOutBuf = (BYTE*) malloc(ra_dec_info.ulOutBufSize);
			if(ra_dec_info.pOutBuf == NULL)
			{
				if(pRADpack){
					ra_depack_destroy(&pRADpack);
					ra_info.pDepack = NULL;
				}
				if(pParser){
					rm_parser_destroy(&pParser);
					ra_info.pParser = NULL;
				}
				libcook_print("[cook decode],dsp malloc  failed,request %s bytes\n",ra_dec_info.ulOutBufSize);
				return -1;
			}
		}
			
	}

	return 0;
}

static void ra_depack_cleanup(void)
{
    /* Destroy the codec init info */
    if (ra_info.pRaInfo)
    {
        ra_depack_destroy_codec_init_info(ra_info.pDepack, &ra_info.pRaInfo);
    }
    /* Destroy the depacketizer */
    if (ra_info.pDepack)
    {
        ra_depack_destroy(&ra_info.pDepack);
    }
    /* If we have a packet, destroy it */
    if (ra_info.pPacket)
    {
        rm_parser_destroy_packet(ra_info.pParser, &ra_info.pPacket);
    }
    if(ra_info.pParser)
   	 rm_parser_destroy(&ra_info.pParser);
    memset(&ra_info,0,sizeof(ra_info));	
}
    

//static int cook_decode_release(void)
int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
	if(cook_input.buf != NULL){
		free(cook_input.buf);
		cook_input.buf = NULL;
	}
	if(cook_output.buf != NULL){
		free(cook_output.buf);
		cook_output.buf = NULL;
	}
	
	ra_decode_destroy(ra_dec_info.pDecode);
	ra_dec_info.pDecode = HXNULL;
    if (ra_dec_info.pOutBuf)
    {
    	free(ra_dec_info.pOutBuf);
        ra_dec_info.pOutBuf = HXNULL;
    }
	ra_dec_info.ulStatus = RADEC_IDLE;
	ra_depack_cleanup();
	libcook_print(" cook decoder release \n");
	return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    return 0;
}


