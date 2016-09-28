#include <stdio.h>
#include <limits.h>
//#include <asm/cache.h>

#ifndef __MW__
#include <inttypes.h>
#else
#include <core/types.h>
#endif
#include "raac_decode.h"
#include "aac_decode.h"
#include "include/rm_parse.h"
#include "include/ra_depack.h"
#include "include/ra_decode.h"

#include "include/rm_memory_default.h"
#include "aacdec.h"

#include "../../amadec/adec-armdec-mgt.h"
#ifdef ANDROID
#include <android/log.h>

#define  LOG_TAG    "RaacDecoder"
#define raac_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else 
#define raac_print printf
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

#define AUDIO_EXTRA_DATA_SIZE  (2048*2)

struct audio_info{
    int valid;
    int sample_rate;
    int channels;
    int bitrate;
    int codec_id;
    int block_align;
    int extradata_size;
    char extradata[AUDIO_EXTRA_DATA_SIZE];
};

struct frame_info
{
    int len;
    unsigned long  offset;/*steam start to here*/
    unsigned long  buffered_len;/*data buffer in  dsp,pcm datalen*/
    int reversed[1];/*for cache aligned 32 bytes*/
};

cook_IObuf cook_input;
cook_IObuf cook_output;

struct frame_info  cur_frame;
static unsigned aac_timestamplen = 0;
static unsigned aac_timestamp = 0;
static raac_decoder_info_t raac_dec_info ;
static rm_info_t raac_info ;
static char  file_header[AUDIO_EXTRA_DATA_SIZE];
#pragma align_to(64,file_header)
static char *cur_read_ptr = file_header;

void set_timestamp(unsigned val)
{
	aac_timestamp = val;
}

void set_timestamp_len(unsigned val)
{
	aac_timestamplen = val;
}

ra_decode*
ra_decode_create2(void*              pUserError,
                  rm_error_func_ptr  fpError,
                  void*              pUserMem,
                  rm_malloc_func_ptr fpMalloc,
                  rm_free_func_ptr   fpFree)
{
    ra_decode* pRet = HXNULL;

    if (fpMalloc && fpFree)
    {
        pRet = (ra_decode*) fpMalloc(pUserMem, sizeof(ra_decode));

        if (pRet)
        {
            /* Zero out the struct */
            memset((void*) pRet, 0, sizeof(ra_decode));
            /* Assign the error function */
            pRet->fpError = fpError;
            pRet->pUserError = pUserError;
            /* Assign the memory functions */
            pRet->fpMalloc = fpMalloc;
            pRet->fpFree   = fpFree;
            pRet->pUserMem = pUserMem;
        }
    }

    return pRet;
}

ra_decode*
ra_decode_create(void*              pUserError,
                 rm_error_func_ptr  fpError)
{
    return ra_decode_create2(pUserError,
                             fpError,
                             HXNULL,
                             rm_memory_default_malloc,
                             rm_memory_default_free);
}

/* ra_decode_destroy()
 * Deletes the decoder backend and frontend instances. */
void
ra_decode_destroy(ra_decode* pFrontEnd)
{
    rm_free_func_ptr fpFree;
    void* pUserMem;

    if (pFrontEnd && pFrontEnd->fpFree)
    {
        /* Save a pointer to fpFree and pUserMem */
        fpFree   = pFrontEnd->fpFree;
        pUserMem = pFrontEnd->pUserMem;

        if (pFrontEnd->pDecode && pFrontEnd->fpClose)
        {
            /* Free the decoder instance and backend */
            pFrontEnd->fpClose(pFrontEnd->pDecode,
                               pUserMem,
                               fpFree);
        }

        /* Free the ra_decode struct memory */
        fpFree(pUserMem, pFrontEnd);
    }
}

/* ra_decode_init()
 * Selects decoder backend with fourCC code.
 * Calls decoder backend init function with init params.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_init(ra_decode*         pFrontEnd,
               UINT32             ulFourCC,
               void*              pInitParams,
               UINT32             ulInitParamsSize,
               ra_format_info*    pStreamInfo)
{
    HX_RESULT retVal = HXR_OK;

    /* Assign the backend function pointers */
if (ulFourCC == 0x72616163 || /* 'raac' */
             ulFourCC == 0x72616370)   /* 'racp' */
    {
        pFrontEnd->fpInit = aac_decode_init;
        pFrontEnd->fpReset = aac_decode_reset;
        pFrontEnd->fpConceal = aac_decode_conceal;
        pFrontEnd->fpDecode = aac_decode_decode;
        pFrontEnd->fpGetMaxSize = aac_decode_getmaxsize;
        pFrontEnd->fpGetChannels = aac_decode_getchannels;
        pFrontEnd->fpGetChannelMask = aac_decode_getchannelmask;
        pFrontEnd->fpGetSampleRate = aac_decode_getrate;
        pFrontEnd->fpMaxSamp = aac_decode_getdelay;
        pFrontEnd->fpClose = aac_decode_close;
    }
    else
    {
        /* error - codec not supported */
		raac_print(" cook decode: not supported fourcc\n");
        retVal = HXR_DEC_NOT_FOUND;
    }

    if (retVal == HXR_OK && pFrontEnd && pFrontEnd->fpInit && pStreamInfo)
    {
        retVal = pFrontEnd->fpInit(pInitParams, ulInitParamsSize, pStreamInfo,
                                   &pFrontEnd->pDecode, pFrontEnd->pUserMem,
                                   pFrontEnd->fpMalloc, pFrontEnd->fpFree);
	}

    return retVal;
}

/* ra_decode_reset()
 * Calls decoder backend reset function.
 * Depending on which codec is in use, *pNumSamplesOut samples may
 * be flushed. After reset, the decoder returns to its initial state.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_reset(ra_decode* pFrontEnd,
                UINT16*    pSamplesOut,
                UINT32     ulNumSamplesAvail,
                UINT32*    pNumSamplesOut)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpReset)
    {
        retVal = pFrontEnd->fpReset(pFrontEnd->pDecode, pSamplesOut,
                                    ulNumSamplesAvail, pNumSamplesOut);
    }

    return retVal;
}

/* ra_decode_conceal()
 * Calls decoder backend conceal function.
 * On successive calls to ra_decode_decode(), the decoder will attempt
 * to conceal ulNumSamples. No input data should be sent while concealed
 * frames are being produced. Once the decoder has exhausted the concealed
 * samples, it can proceed normally with decoding valid input data.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_conceal(ra_decode*  pFrontEnd,
                  UINT32      ulNumSamples)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpConceal)
    {
        retVal = pFrontEnd->fpConceal(pFrontEnd->pDecode, ulNumSamples);
    }

    return retVal;
}

/* ra_decode_decode()
 * Calls decoder backend decode function.
 * pData             : input data (compressed frame).
 * ulNumBytes        : input data size in bytes.
 * pNumBytesConsumed : amount of input data consumed by decoder.
 * pSamplesOut       : output data (uncompressed frame).
 * ulNumSamplesAvail : size of output buffer.
 * pNumSamplesOut    : amount of ouput data produced by decoder.
 * ulFlags           : control flags for decoder.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_decode(ra_decode*  pFrontEnd,
                 UINT8*      pData,
                 UINT32      ulNumBytes,
                 UINT32*     pNumBytesConsumed,
                 UINT16*     pSamplesOut,
                 UINT32      ulNumSamplesAvail,
                 UINT32*     pNumSamplesOut,
                 UINT32      ulFlags,
                 UINT32		 ulTimeStamp)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpDecode)
    {
        retVal = pFrontEnd->fpDecode(pFrontEnd->pDecode, pData, ulNumBytes,
                                     pNumBytesConsumed, pSamplesOut,
                                     ulNumSamplesAvail, pNumSamplesOut, ulFlags, ulTimeStamp);
    }

    return retVal;
}


/**************** Accessor Functions *******************/
/* ra_decode_getmaxsize()
 * pNumSamples receives the maximum number of samples produced
 * by the decoder in response to a call to ra_decode_decode().
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_getmaxsize(ra_decode* pFrontEnd,
                     UINT32*    pNumSamples)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpGetMaxSize)
    {
        retVal = pFrontEnd->fpGetMaxSize(pFrontEnd->pDecode, pNumSamples);
    }

    return retVal;
}

/* ra_decode_getchannels()
 * pNumChannels receives the number of audio channels in the bitstream.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_getchannels(ra_decode* pFrontEnd,
                      UINT32*    pNumChannels)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpGetChannels)
    {
        retVal = pFrontEnd->fpGetChannels(pFrontEnd->pDecode, pNumChannels);
    }

    return retVal;
}

/* ra_decode_getchannelmask()
 * pChannelMask receives the 32-bit mapping of the audio output channels.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_getchannelmask(ra_decode* pFrontEnd,
                         UINT32*    pChannelMask)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpGetChannelMask)
    {
        retVal = pFrontEnd->fpGetChannelMask(pFrontEnd->pDecode, pChannelMask);
    }

    return retVal;
}

/* ra_decode_getrate()
 * pSampleRate receives the sampling rate of the output samples.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_getrate(ra_decode* pFrontEnd,
                  UINT32*    pSampleRate)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpGetSampleRate)
    {
        retVal = pFrontEnd->fpGetSampleRate(pFrontEnd->pDecode, pSampleRate);
    }

    return retVal;
}

/* ra_decode_getdelay()
 * pNumSamples receives the number of invalid output samples
 * produced by the decoder at startup.
 * If non-zero, it is up to the user to discard these samples.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
ra_decode_getdelay(ra_decode* pFrontEnd,
                   UINT32*    pNumSamples)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pFrontEnd && pFrontEnd->fpMaxSamp)
    {
        retVal = pFrontEnd->fpMaxSamp(pFrontEnd->pDecode, pNumSamples);
    }

    return retVal;
}


void rm_error(void* pError, HX_RESULT result, const char* pszMsg)
{
    //dsp_mailbox_send(1,M1B_IRQ7_DECODE_FATAL_ERR, 1, NULL, 0);
    //trans_err_code(DECODE_FATAL_ERR);
    raac_print("rm_error pError=0x%08x result=0x%08x msg=%s\n", pError, result, pszMsg);
    //while(1);
}
//static int raac_decode_frame(unsigned char *buf, int maxlen, struct frame_fmt *fmt)
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
	HX_RESULT retVal = HXR_OK;
	UINT32 ulBytesConsumed = 0;
    UINT32 ulTotalConsumed = 0;
    UINT32 ulBytesLeft     = 0;
    UINT32 ulNumSamplesOut = 0;
   	UINT32 ulMaxSamples    = 0;
	UINT32 out_data_len = 0;

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
        retVal = rm_parser_get_packet(raac_info.pParser, &raac_info.pPacket);
        if (retVal == HXR_OK) {
            //if (ra_info.pPacket->usStream == ra_info.usStreamNum)
            {
                retVal = ra_depack_add_packet(raac_info.pDepack, raac_info.pPacket);

            }
            rm_parser_destroy_packet(raac_info.pParser, &raac_info.pPacket);
        }
        if (retVal != HXR_OK){
        	raac_print("cook_decode_frame£º add packet failed\n");
			break;
		}
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
	//return raac_dec_info.decoded_size;
}


UINT32 rm_io_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{
    memcpy(pBuf,cur_read_ptr,ulBytesToRead);
    cur_read_ptr = cur_read_ptr + ulBytesToRead;
    if((unsigned)(cur_read_ptr - file_header) > AUDIO_EXTRA_DATA_SIZE){
		//trans_err_code(DECODE_INIT_ERR);			
		raac_print("warning :: raac.read byte exceed the the buffer then sent,%d \n",(unsigned)(cur_read_ptr - file_header) );
		while(1);
    }		
    return ulBytesToRead;	
}
void rm_io_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
        if (ulOrigin == HX_SEEK_ORIGIN_CUR)
            cur_read_ptr += ulOffset;
        else if (ulOrigin == HX_SEEK_ORIGIN_SET)
        	cur_read_ptr =(char *)pUserRead + ulOffset;
        else if (ulOrigin == HX_SEEK_ORIGIN_END)
                cur_read_ptr = sizeof(file_header)+(char *)pUserRead-ulOffset;
   	if((unsigned)(cur_read_ptr - file_header) > AUDIO_EXTRA_DATA_SIZE){
		//trans_err_code(DECODE_INIT_ERR);	
		raac_print("warning :: raac.seek buffer pos exceed the the buffer then sent,%d \n",(unsigned)(cur_read_ptr - file_header) );
		while(1);
   	}	

}
unsigned rm_ab_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{	
	int ret = 0;
#if 0
	if(pBuf&&ulBytesToRead)
		ret =  read_buffer(pBuf,ulBytesToRead);
	return ret;	
#endif
	if(pBuf && ulBytesToRead){
		ret = (ulBytesToRead > cook_input.buf_len) ? cook_input.buf_len : ulBytesToRead;
		memcpy(pBuf, cook_input.buf, ret);
		if(ret < cook_input.buf_len){
			memcpy(cook_input.buf, (cook_input.buf + ret),  cook_input.buf_len - ret);
		}
		cook_input.buf_len -= ret;
	}
	cook_input.cousume += ret;
	cook_input.all_consume += ret;
	return ret;	
}
void rm_ab_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
    int i;	
#if 0
    if (ulOrigin == HX_SEEK_ORIGIN_CUR){
    	for(i = 0;i <ulOffset;i++)
    		read_byte();
    }
#endif
	if (ulOrigin == HX_SEEK_ORIGIN_CUR){
		if(ulOffset <= cook_input.buf_len){
			//memcpy(cook_input.buf, cook_input.buf + ulOffset, cook_input.buf_len - ulOffset);
			cook_input.buf_len -= ulOffset;
			cook_input.cousume += ulOffset;
			//cook_input.all_consume += ulOffset;
			memcpy(cook_input.buf, cook_input.buf + ulOffset, cook_input.buf_len);
		}else{
			raac_print("rm_ab_seek failed\n");
		}
	}
	
}
HX_RESULT _raac_block_available(void* pAvail, UINT32 ulSubStream, ra_block* pBlock)
{
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
	//BYTE*   buf = raac_dec_info.input_buf;//passed from the audio dec thread
    	rm_info_t* pInfo = (rm_info_t*) pAvail;
   	 if (pAvail && pBlock && pBlock->pData && pBlock->ulDataLen){
		ulBytesLeft = pBlock->ulDataLen;
	        while (retVal == HXR_OK && ulBytesLeft)
	        {
			retVal = ra_decode_decode(raac_dec_info.pDecode,
	                                  pBlock->pData+ pBlock->ulDataLen-ulBytesLeft,
	                                  ulBytesLeft,
	                                  &ulBytesConsumed,
	                                  (UINT16*)raac_dec_info.pOutBuf,
	                                  raac_dec_info.ulOutBufSize / 2,
	                                  &ulNumSamplesOut,
                                  	  pBlock->ulDataFlags,
                                  	  pBlock->ulTimestamp*90);

			if (retVal == HXR_OK){
				 if (ulBytesConsumed){
	                		ulBytesLeft -= ulBytesConsumed;
				 }
				 if (ulNumSamplesOut){
					len = ulNumSamplesOut*2;
				 }
			}
			else if (retVal == HXR_NO_DATA){
				raac_print("raac decode not enough data.\n");
				return 0;
			}
			else{
				raac_print("raac decode error.\n");
				return 0;
			}
//#if 0
			//raac_print("ulTimestamp=%ld\n", pBlock->ulTimestamp);
			cur_frame.offset = pBlock->ulTimestamp*90 +1;
			cur_frame.buffered_len = 0;
			cur_frame.len= 0;
			delay_pts += ulNumSamplesOut;//(ulNumSamplesOut/raac_info.pRaInfo->usNumChannels)*90 /(raac_info.pRaInfo->ulSampleRate / 1000);
			//refresh_swap_register1(cur_frame.offset+delay_pts, len);
//#endif
			memcpy(cook_output.buf + cook_output.buf_len, raac_dec_info.pOutBuf, len);
			cook_output.buf_len += len;
			len = 0;
		}
		delay_pts = (delay_pts/raac_info.pRaInfo->usNumChannels)*90 /(raac_info.pRaInfo->ulSampleRate / 1000);
		cur_frame.offset += delay_pts;
		raac_dec_info.decoded_size = out_data_len;
    }
    return retVal;
}
//static int raac_decode_init(struct frame_fmt * fmt)
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    raac_print("\n\n[%s]BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
	raac_print("enter into %s:%d\n", __FUNCTION__, __LINE__);
	HX_RESULT retVal = HXR_OK;
	unsigned ulNumStreams = 0;
	rm_parser*          pParser = HXNULL;
	rm_stream_header *pHdr = HXNULL;
	ra_depack *pRADpack = HXNULL;
	ra_format_info *pRAInfo = HXNULL;
	int i;
#if 0
	struct audio_info *real_data;
	real_data = (struct audio_info *)fmt->private_data;
	if(!real_data ||!real_data->extradata){
		raac_print("[raac decode],got extra data failed\n");
		return -1;
	}
#endif
	struct audio_info real_data;

	real_data.bitrate = adec_ops->bps;
	real_data.channels = adec_ops->channels;
	real_data.extradata_size = adec_ops->extradata_size;
	real_data.sample_rate = adec_ops->samplerate;

	adec_ops->nInBufSize = DefaultReadSize;
	adec_ops->nOutBufSize = DefaultOutBufSize;
	raac_print("%s:%d\n", __FUNCTION__, __LINE__);
	memset(real_data.extradata, 0, AUDIO_EXTRA_DATA_SIZE);
	raac_print("%d,%d\n",real_data.extradata_size,adec_ops->extradata_size);
	for(i = 0; i < real_data.extradata_size; i++){
		real_data.extradata[i] = adec_ops->extradata[i];
	}
	
	raac_print("raac audioinfo four data [0x%x],	[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],\n",real_data.extradata[0],\
	real_data.extradata[1],real_data.extradata[2],real_data.extradata[3], \
	real_data.extradata[4],real_data.extradata[5],real_data.extradata[6],real_data.extradata[7]);

	memcpy(file_header, real_data.extradata, AUDIO_EXTRA_DATA_SIZE);

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
			raac_print("inbuf malloc failed\n");
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
			raac_print("outbuf malloc failed\n");
			return -1;
		}
	}

	if(cook_input.buf == NULL || cook_output.buf == NULL){
		raac_print("malloc buf failed\n");
		return -1;
	
	}
	//dsp_cache_wback((unsigned)file_header,AUDIO_EXTRA_DATA_SIZE);
	cur_frame.offset = 0;

	unsigned ulCodec4CC = 0;
	//int i;
	/*clear up the decoder structure */
	memset(&raac_dec_info,0,sizeof(raac_decoder_info_t));	
	memset(&raac_info,0,sizeof(rm_info_t));
	cur_read_ptr = file_header;	

	/* Create the parser struct */
	pParser = rm_parser_create(NULL, rm_error);
	if (!pParser)
	{
		raac_print("[raac decode],create parser failed\n");
		return -1;
	}
	/* Set the stream into the parser */
#if 0
	for(i = 0;i < 1;i++)
	{
	raac_print("%d header data  [0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x]\n\t",i,\
	file_header[i*8+0],file_header[i*8+1],file_header[i*8+2],file_header[i*8+3],\
	file_header[i*8+4],file_header[i*8+5],file_header[i*8+6],file_header[i*8+7]);
	}
#endif /* 0 */
	retVal = rm_parser_init_io(pParser, file_header, rm_io_read, rm_io_seek);
	if (retVal != HXR_OK)
	{
		raac_print("[raac decode], parser init IO failed,errid %d\n",retVal);
		rm_parser_destroy(&pParser);
		return -1;
	}
	/* Read all the headers at the beginning of the .rm file */
	retVal = rm_parser_read_headers(pParser);
	if (retVal != HXR_OK)
	{
		raac_print("[raac decode], parser read header failed,errid %d\n",retVal);
		rm_parser_destroy(&pParser);
		return -1;
	}
	raac_print("raac: rm_parser_read_headers finished \n");
	/* Get the number of streams */
	ulNumStreams = rm_parser_get_num_streams(pParser);
	if (ulNumStreams == 0)
	{
		raac_print("[raac decode], no stream found\n");
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
				pRADpack = ra_depack_create((void*)pParser,_raac_block_available,NULL,rm_error);
				if (!pRADpack)
				{
					raac_print("[raac decode], create depack failed\n");
					rm_parser_destroy_stream_header(pParser, &pHdr);
					rm_parser_destroy(&pParser);
					return -1;
				}
				/* Initialize the RA depacketizer with the stream header */
				retVal = ra_depack_init(pRADpack, pHdr);
				if (retVal != HXR_OK)
				{
					raac_print("[raac decode],init depack failed,errid %d\n",retVal);
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
					raac_info.pRaInfo = pRAInfo;
				}
				else if ((ulCodec4CC == 0x72616163)||(ulCodec4CC == 0x72616370)) 
				/* raac racp */
				{
					retVal = ra_depack_get_codec_init_info(pRADpack, 0, &pRAInfo);
					raac_info.pRaInfo = pRAInfo;
				}
				raac_info.pDepack = pRADpack;
			}
			rm_parser_destroy_stream_header(pParser, &pHdr);
		}
		raac_print("raac rm_parser_get_stream_header finished\n");		
	}
	/* Set the stream into the parser */
	retVal = rm_parser_init_io(pParser, 0, rm_ab_read, rm_ab_seek);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			raac_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
		}
		raac_print("[raac decode],rm_parser_init_io failed,errid %d\n",retVal);
		return -1;		
	}
	raac_info.pParser = pParser;
	rm_parser_set_stream(&pParser, 0); 
	rm_parser_file_seek(pParser, 0);
	raac_dec_info.pDecode = ra_decode_create(HXNULL, rm_error);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			raac_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
		}
		raac_print("[raac decode],ra_decode_create failed,errid %d\n",retVal);
		return -1;		
	}
	raac_dec_info.ulStatus = RADEC_PLAY;
	raac_dec_info.ulTotalSample = 0;
	raac_dec_info.ulTotalSamplePlayed = 0;
	UINT32 ulBufSize = 2048/*max sample*/ * 2/*max channel*/ * sizeof(UINT16) * SBR_MUL;
	raac_dec_info.ulOutBufSize = ulBufSize;
	raac_dec_info.pOutBuf = (BYTE*) malloc(raac_dec_info.ulOutBufSize);
	if(raac_dec_info.pOutBuf == NULL)
	{
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			raac_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
			raac_info.pParser = NULL;
		}
		raac_print("[raac decode],dsp malloc  failed,request %s bytes\n",raac_dec_info.ulOutBufSize);
		return -1;
	}
	retVal = ra_decode_init(raac_dec_info.pDecode,ulCodec4CC, HXNULL, 0, raac_info.pRaInfo);
	if (retVal != HXR_OK){
		if(pRADpack){
			ra_depack_destroy(&pRADpack);
			raac_info.pDepack = NULL;
		}
		if(pParser){
			rm_parser_destroy(&pParser);
			raac_info.pParser = NULL;
		}
		raac_print("[raac decode],ra_decode_init failed,errid %d\n",retVal);
		return -1;		
	}
	//fmt->valid=CHANNEL_VALID | SAMPLE_RATE_VALID | DATA_WIDTH_VALID;
	//fmt->channel_num = raac_info.pRaInfo->usNumChannels;
	//fmt->data_width = 16;
#ifdef AAC_ENABLE_SBR
	//fmt->sample_rate = raac_info.pRaInfo->ulActualRate;
#else
	//fmt->sample_rate = raac_info.pRaInfo->ulSampleRate;
	aac_decode  *pdec =   (aac_decode *)raac_dec_info.pDecode->pDecode;
	if(pdec && pdec->ulSampleRateCore){
		//fmt->sample_rate = pdec->ulSampleRateCore;
		raac_print("ac sr %d, sr %d,sbr %d,core sr %d,aac sr %d \n",raac_info.pRaInfo->ulActualRate, \
			raac_info.pRaInfo->ulSampleRate,pdec->bSBR,\
		      pdec->ulSampleRateCore,pdec->ulSampleRateOut);	
	}
#endif

	return 0;
	}

void ra_depack_cleanup(void)
{
    /* Destroy the codec init info */
    if (raac_info.pRaInfo)
    {
        ra_depack_destroy_codec_init_info(raac_info.pDepack, &raac_info.pRaInfo);
    }
    /* Destroy the depacketizer */
    if (raac_info.pDepack)
    {
        ra_depack_destroy(&raac_info.pDepack);
    }
    /* If we have a packet, destroy it */
    if (raac_info.pPacket)
    {
        rm_parser_destroy_packet(raac_info.pParser, &raac_info.pPacket);
    }
    if(raac_info.pParser)
   	 rm_parser_destroy(&raac_info.pParser);
    memset(&raac_info,0,sizeof(raac_info));	
}
    

//static int raac_decode_release(void)
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

	ra_decode_destroy(raac_dec_info.pDecode);
	raac_dec_info.pDecode = HXNULL;
       if (raac_dec_info.pOutBuf)
       {
        	  free(raac_dec_info.pOutBuf);
              raac_dec_info.pOutBuf = HXNULL;
       }
	raac_dec_info.ulStatus = RADEC_IDLE;
	ra_depack_cleanup();
    raac_print(" raac decoder release\n");

	return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
    return 0;
}


#if 0
static struct codec_type raac_codec=
{
  .name="raac",
  .init=raac_decode_init,
  .release=raac_decode_release,
  .decode_frame=raac_decode_frame,
};


void __used raac_codec_init(void)
{
        raac_print("register raac lib \n");
       register_codec(&raac_codec);
}

CODEC_INIT(raac_codec_init);
#endif
