#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <cutils/properties.h>



#include "aaccommon.h"
#include "aacdec.h"
#include "../../amadec/adec-armdec-mgt.h"


int AACDataSource = 1;
static HAACDecoder hAACDecoder;
static HAACIOBuf hAACIOBuf = NULL;

static int mute_pcm_bytes;
/* check the normal frame size */
static unsigned last_frm_size;
static unsigned cur_frm_size;
static int lastFrameLen ;
static int lastSampPerFrm ;

#define FRAME_RECORD_NUM   20
static unsigned error_count = 0;
static unsigned mute_pcm_thread;
static unsigned his_index;
static unsigned frame_length_his[FRAME_RECORD_NUM];

static unsigned stream_in_offset = 0;
static unsigned enable_debug_print = 0;
/* Channel definitions */
#define FRONT_CENTER (0)
#define FRONT_LEFT   (1)
#define FRONT_RIGHT  (2)
#define SIDE_LEFT    (3)
#define SIDE_RIGHT   (4)
#define BACK_LEFT    (5)
#define LFE_CHANNEL  (6)





#define ASTREAM_DEV "/dev/uio0"
#define ASTREAM_ADDR "/sys/class/astream/astream-dev/uio0/maps/map0/addr"
#define ASTREAM_SIZE "/sys/class/astream/astream-dev/uio0/maps/map0/size"
#define ASTREAM_OFFSET "/sys/class/astream/astream-dev/uio0/maps/map0/offset"



#define AIU_AIFIFO_CTRL                            0x1580
#define AIU_AIFIFO_STATUS                          0x1581
#define AIU_AIFIFO_GBIT                            0x1582
#define AIU_AIFIFO_CLB                             0x1583
#define AIU_MEM_AIFIFO_START_PTR                   0x1584
#define AIU_MEM_AIFIFO_CURR_PTR                    0x1585
#define AIU_MEM_AIFIFO_END_PTR                     0x1586
#define AIU_MEM_AIFIFO_BYTES_AVAIL                 0x1587
#define AIU_MEM_AIFIFO_CONTROL                     0x1588
#define AIU_MEM_AIFIFO_MAN_WP                      0x1589
#define AIU_MEM_AIFIFO_MAN_RP                      0x158a
#define AIU_MEM_AIFIFO_LEVEL                       0x158b
#define AIU_MEM_AIFIFO_BUF_CNTL                    0x158c
#define AIU_MEM_AIFIFO_BUF_WRAP_COUNT              0x158d
#define AIU_MEM_AIFIFO2_BUF_WRAP_COUNT             0x158e
#define AIU_MEM_AIFIFO_MEM_CTL                     0x158f

volatile unsigned* reg_base = 0;
#define READ_MPEG_REG(reg) reg_base[reg-AIU_AIFIFO_CTRL]
#define WRITE_MPEG_REG(reg, val) reg_base[reg-AIU_AIFIFO_CTRL]=val
#define AIFIFO_READY  (((READ_MPEG_REG(AIU_MEM_AIFIFO_CONTROL)&(1<<9))))
#define min(x,y) ((x<y)?(x):(y))
static int fd_uio = -1;
static volatile unsigned memmap = MAP_FAILED;	
static int phys_size;
static volatile int exit_flag = 0;
static unsigned long amsysfs_get_sysfs_ulong(const char *path)
{
	int fd;
	char bcmd[24]="";
	unsigned long num=0;
	if ((fd = open(path, O_RDONLY)) >=0) {
    	read(fd, bcmd, sizeof(bcmd));
    	num = strtoul(bcmd, NULL, 0);
    	close(fd);
	} else {
        audio_codec_print("unable to open file %s,", path);
    }
	return num;
}
static unsigned long  get_num_infile(char *file)
{
	return amsysfs_get_sysfs_ulong(file);
}

static int uio_init(){
	int pagesize = getpagesize();
	int phys_start;
	int phys_offset;


	fd_uio = open(ASTREAM_DEV, O_RDWR);
	if(fd_uio < 0){
		audio_codec_print("error open UIO 0\n");
		return -1;
	}
	phys_start = get_num_infile(ASTREAM_ADDR);
	phys_size = get_num_infile(ASTREAM_SIZE);
	phys_offset = get_num_infile(ASTREAM_OFFSET);

	audio_codec_print("add=%08x, size=%08x, offset=%08x\n", phys_start, phys_size, phys_offset);

	phys_size = (phys_size + pagesize -1) & (~ (pagesize-1));
	memmap = mmap(NULL, phys_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_uio, 0* pagesize);
	
	audio_codec_print("memmap = %x , pagesize = %x\n", memmap,pagesize);
	if(memmap == MAP_FAILED){
		audio_codec_print("map /dev/uio0 failed\n");
		return -1;
	}	 
	 
	reg_base = memmap + phys_offset;
	return 0;
}

#define EXTRA_DATA_SIZE 128

static inline void waiting_bits(int bits)
{
	int bytes;
	bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
	while(bytes*8<bits && !exit_flag){
		usleep(1000);
		bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
	}	
}

int read_buffer(unsigned char *buffer,int size)
{
	int bytes;
	int len;
	unsigned char *p=buffer; 
	int tmp;
	int space;
	int i;
	int wait_times=0,fifo_ready_wait = 0;
	
	int iii;

	iii = READ_MPEG_REG(AIU_MEM_AIFIFO_LEVEL)-EXTRA_DATA_SIZE;
	if(( size >=  iii))
	    return 0;
	
//	adec_print("read_buffer start while iii= %d!!\n", iii);
	for(len=0;len<size;)
	{	
			space=(size-len);
			bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
			//adec_print("read_buffer start AIU_MEM_AIFIFO_BYTES_AVAIL bytes= %d!!\n", bytes);
			wait_times=0;
			while(bytes==0)
			{
				waiting_bits((space>128)?128*8:(space*8));	/*wait 32 bytes,if the space is less than 32 bytes,wait the space bits*/
				bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
				
				audio_codec_print("read_buffer while AIU_MEM_AIFIFO_BYTES_AVAIL = %d!!\n", bytes);
				wait_times++;
				if(wait_times>10) {					
					audio_codec_print("goto out!!\n");
					goto out;
				}
			}
			bytes=min(space,bytes);
			
			//adec_print("read_buffer while bytes = %d!!\n", bytes);
			for(i=0;i<bytes;i++)
			{
				while(!AIFIFO_READY){
					fifo_ready_wait++;
					usleep(1000);
					if(fifo_ready_wait > 100){
						audio_codec_print("FATAL err,AIFIFO is not ready,check!!\n");
						return 0;
					}	
				}
				WRITE_MPEG_REG(AIU_AIFIFO_GBIT,8);
				tmp=READ_MPEG_REG(AIU_AIFIFO_GBIT);
				//adec_print("read_buffer while tmp = %d!!\n", tmp);
				
				*p++=tmp&0xff;
				fifo_ready_wait = 0;

			}
			len+=bytes;
	}
out:
	stream_in_offset+=len;
	return len;
}  
 int get_audiobuf_level()    
 {
 	int level = 0;
 	level =  READ_MPEG_REG(AIU_MEM_AIFIFO_LEVEL)-EXTRA_DATA_SIZE;
	if(level < 0)
		level = 0;
	return level;
 }
 	

static unsigned get_frame_size()
{
	int i;
	unsigned sum = 0;
	unsigned valid_his_num = 0;
	for(i = 0;i < FRAME_RECORD_NUM;i++)
	{
		if(frame_length_his[i] > 0){
			valid_his_num ++;
			sum	+= frame_length_his[i];
		}	
	}
	
	if(valid_his_num==0)
		return 0;
		
	return sum/valid_his_num;
}

unsigned  get_audio_inbuf_latency(int bytesin)
 {
 	int frame_size = 0;
	int latency_ms = 0;
 	frame_size = get_frame_size();
	if(frame_size >0){
 		latency_ms = bytesin*1024/(frame_size*48);
	}
	return latency_ms;
 }

static HAACDecoder aac_init_decoder()
{
    return AACInitDecoder();
}
static HAACIOBuf aac_init_iobuf()
{
   	return (HAACIOBuf)calloc(1, sizeof(AACIOBuf));
}
#define AUDIO_BUFFER_REMAINED 128
static void aac_refill_buffer(HAACIOBuf hIOBuf)
{
    	AACIOBuf *IOBuf = (AACIOBuf*)hIOBuf;
	int bytes_expected;
	int read_bytes;
	int read_level_count = 0;
	int  buf_level = 0;
	int retry_time = 0;	
		
start_read:
		if(exit_flag)
			return ;
		do{
			buf_level = get_audiobuf_level();
			if(buf_level <= 0){
					break;
			}
			
			if (IOBuf->bytesLeft>0)
	    { 
	    		while(IOBuf->bytesLeft > AAC_INPUTBUF_SIZE);
	        memcpy(IOBuf->readBuf, IOBuf->readPtr, IOBuf->bytesLeft);
	    }
	    
	    while(IOBuf->bytesLeft<0){
	    	printk("IOBuf->bytesLeft < 0\n", IOBuf->bytesLeft);
	    }
	    
	    bytes_expected = AAC_INPUTBUF_SIZE - IOBuf->bytesLeft;
	    
	    while(bytes_expected<0){
	    	printk("bytes_expected = %d\n",bytes_expected);
	    }
	
			if(bytes_expected > buf_level)
				bytes_expected  = buf_level;
			 		
			read_bytes = read_buffer(IOBuf->readBuf + IOBuf->bytesLeft,bytes_expected);
	
	    IOBuf->readPtr = IOBuf->readBuf;
	    IOBuf->bytesLeft += read_bytes;
  	}while(0);
    if(IOBuf->bytesLeft < get_frame_size()){
    		goto start_read;
    }
}

 static int aac_reset_decoder(HAACDecoder hAACDecoder, HAACIOBuf hIOBuf)
{
    AACIOBuf *IOBuf = (AACIOBuf*)hIOBuf;
    IOBuf->readPtr = IOBuf->readBuf;
    IOBuf->bytesLeft = 0;
    return AACFlushCodec(hAACDecoder);
}
static int aac_decode_frame(HAACDecoder hAACDecoder, HAACIOBuf hIOBuf)
{
    int err;
    AACIOBuf *IOBuf = (AACIOBuf*)hIOBuf;
    if (AACDataSource==1){
	    if (IOBuf->bytesLeft < AAC_INPUTBUF_SIZE/2 )
	    {
	        aac_refill_buffer(IOBuf);
	    }
    }
    if(exit_flag)
		return ERR_AAC_EXIT_DECODE;
    /* decode one AAC frame */
    err = AACDecode(hAACDecoder, &(IOBuf->readPtr), &(IOBuf->bytesLeft), (IOBuf->outBuf));
     return err;
}
int audio_dec_init(audio_decoder_operations_t *adp)
{
       printk("\n\n[%s]WFDAAC DEC BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
#ifdef ANDROID
	char value[PROPERTY_VALUE_MAX];
	if( property_get("media.wfd.debug_dec",value,NULL) > 0)
#else
      char * value;
      value=getenv("media.wfd.debug_dec");
	if(value!= NULL)
#endif 
	{
		enable_debug_print = atoi(value);
	}
	int err = 0,ch = 0,i;
    	AACFrameInfo aacFrameInfo = {0};
	audio_codec_print("helix_aac_decoder_init start \n");	
	err = uio_init	();
	if(err)
		return -1;
	hAACDecoder = aac_init_decoder();
	if(!hAACDecoder) 
	{
		printk("fatal error,helix aac decoder init failed\n");
		return -1;
	}
	hAACIOBuf = aac_init_iobuf();
	if(!hAACIOBuf) 
	{
		printk("fatal error,helix aac decoder init iobuf failed\n");

		return -1;
	}
	
	return 0;
}

/*
	channel configuration mapping 
	ch nums		position
	3			FC FL FR
	4			FC  FL FR  BC
	5       		FC  FL FR  BL BR
	6 			FC  FL FR BL  BR	LFE
	
*/
#define FATAL_ERR_RESET_COUNT  2000
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *buf, int *outlen, char *inbuf, int inlen)
{
    int err,i,sample_out = 0,ch;
    short *pcmbuf = (short*)buf;
    short *ouput = (short*)(((AACIOBuf*)hAACIOBuf)->outBuf);
    int ch_num;
	int sum;
	unsigned ch_map_scale[6] = {2,4,4,2,2,0}; //full scale == 8
    AACFrameInfo aacFrameInfo = {0};
	
    err = aac_decode_frame(hAACDecoder, hAACIOBuf);
    if (!err)
    {
		error_count = 0;
        AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);
		
	
        /* L = (l+c)/2;R = (r+c)/2 */	
        if(aacFrameInfo.nChans > 2) //should do downmix to 2ch output.
        {
            ch_num = 	aacFrameInfo.nChans;
            sample_out = aacFrameInfo.outputSamps/ch_num*2*2;//ch_num*sample_num*16bit
            if(ch_num == 3 || ch_num == 4){
				ch_map_scale[0] = 4; //50%
				ch_map_scale[1] = 4;//50%	
				ch_map_scale[2]	= 4;//50%
           		ch_map_scale[3] = 0;
           		ch_map_scale[4] = 0;
           		ch_map_scale[5] = 0;
            }
            for(i = 0;i <aacFrameInfo.outputSamps/ch_num;i++)
            {
            	sum = ((int)ouput[ch_num*i+FRONT_LEFT]*ch_map_scale[FRONT_LEFT]+(int)ouput[ch_num*i+FRONT_CENTER]*ch_map_scale[FRONT_CENTER]+(int)ouput[ch_num*i+BACK_LEFT]*ch_map_scale[BACK_LEFT]);
				pcmbuf[i*2] = sum >>3;
				sum = ((int)ouput[ch_num*i+FRONT_RIGHT]*ch_map_scale[FRONT_RIGHT]+(int)ouput[ch_num*i+FRONT_CENTER]*ch_map_scale[FRONT_CENTER]+(int)ouput[ch_num*i+BACK_LEFT]*ch_map_scale[BACK_LEFT]);
                pcmbuf[2*i+1] = sum>>3;              
			}	
        }
        else{
            memcpy(pcmbuf,ouput,aacFrameInfo.outputSamps*2);
            sample_out = aacFrameInfo.outputSamps*2;//ch_num*sample_num*16bit
        }
		
	  AACGetDecoderInfo(hAACDecoder, &aacFrameInfo);
//	  fmt->total_byte_parsed = aacFrameInfo.total_byte_parsed;
	//  fmt->total_sample_decoded = aacFrameInfo.total_sample_decoded;
//	  fmt->format = aacFrameInfo.format;
//	  fmt->bps = aacFrameInfo.bitRate;

      lastFrameLen = ((AACDecInfo*)hAACDecoder)->frame_length;	
	  /* record the frame length into the history buffer */
	  for(i = 0;i < FRAME_RECORD_NUM-1;i++)
	  {
		frame_length_his[i] = frame_length_his[i+1];
	  }
	  frame_length_his[FRAME_RECORD_NUM-1] = lastFrameLen;
	unsigned abuf_level = get_audiobuf_level();
	unsigned dec_cached = ((AACIOBuf*)hAACIOBuf)->bytesLeft;
	unsigned in_latency_ms = get_audio_inbuf_latency(abuf_level+dec_cached);
	if(enable_debug_print)
	printk("sampRateCore %d,sampRateOut %d,last frame len %d,avarge frame size %d,decode out sample %d,buf level %d,decode cached %d,total level %d ;latency %d ms  \n", \
		aacFrameInfo.sampRateCore,aacFrameInfo.sampRateOut,  \
		lastFrameLen,get_frame_size(),sample_out, \
		abuf_level,dec_cached,abuf_level+dec_cached,in_latency_ms);
    	 memcpy(inbuf,&in_latency_ms,sizeof( in_latency_ms));	  
      lastSampPerFrm = sample_out;
	adec_ops->samplerate = aacFrameInfo.sampRateOut;
	adec_ops->channels = aacFrameInfo.nChans > 2 ?2:aacFrameInfo.nChans;
    }else if (err==ERR_AAC_INDATA_UNDERFLOW){
            aac_refill_buffer(hAACIOBuf); 
    }
/*	 why add this ????
    else if (err==ERR_AAC_NCHANS_TOO_HIGH)
    {
        //maybe we meet zero frame
        ((AACIOBuf*)hAACIOBuf)->bytesLeft =0;
        sample_out =0;
    }	
*/	
   else if(err == ERR_AAC_EXIT_DECODE)
   	sample_out = 0;
    else
    {
        int frame_length;
	if(ERR_AAC_SSR_GAIN_NOT_ADDED != err)
		error_count++;
        // error, disable output, reset decoder and flush io
        if(/*!mute_frame_num*/(error_count%20) == 1){//only print the every 20 times error
		#if 0
            printk("helix aac decoder error,err num :%d,try output 40 mute frames and skip err bitstream\n",err);
            printk("frame_length = %d\n", get_frame_size());
            printk("sames per frame: %d\n", lastSampPerFrm);
		#endif	
        }
	if(error_count == FATAL_ERR_RESET_COUNT){// send to player to reset the player
		printk("decoder error count FATAL_ERR_RESET_COUNT %s\n",FATAL_ERR_RESET_COUNT);
//		trans_err_code(DECODE_FATAL_ERR);
	}
        //aac_reset_decoder(hAACDecoder, hAACIOBuf);
       // AACFlushCodec(hAACDecoder);
        frame_length = get_frame_size()*2/3/*((AACDecInfo*)hAACDecoder)->frame_length*/; //skip 2/3 of last frame size
        if(AACDataSource){
            if(frame_length > 0){
                if(((AACIOBuf*)hAACIOBuf)->bytesLeft > frame_length ){
                    ((AACIOBuf*)hAACIOBuf)->bytesLeft -= frame_length;
                    ((AACIOBuf*)hAACIOBuf)->readPtr  += frame_length;
				//	printk("skip1 %d bytes \n",frame_length);
                }else{				       
				//	printk("skip2 %d bytes \n",((AACIOBuf*)hAACIOBuf)->bytesLeft);
                
                    ((AACIOBuf*)hAACIOBuf)->readPtr  =  ((AACIOBuf*)hAACIOBuf)->readBuf;
                    ((AACIOBuf*)hAACIOBuf)->bytesLeft = 0;
					
                }
            }else{
                if(((AACIOBuf*)hAACIOBuf)->bytesLeft > 100){
                    ((AACIOBuf*)hAACIOBuf)->bytesLeft -= 100;
                    ((AACIOBuf*)hAACIOBuf)->readPtr  += 100;
					
				//	printk("skip3 %d bytes \n",100);
                }else{
				//	printk("skip4 %d bytes \n",((AACIOBuf*)hAACIOBuf)->bytesLeft);
                
                    ((AACIOBuf*)hAACIOBuf)->readPtr  =  ((AACIOBuf*)hAACIOBuf)->readBuf;
                    ((AACIOBuf*)hAACIOBuf)->bytesLeft = 0;
					
                }
            }
        }

        sample_out =  0;//lastSampPerFrm/*fmt->channel_num*2*1024*/;
        mute_pcm_bytes = mute_pcm_thread;
    }
    if(sample_out&& mute_pcm_bytes > 0){
        memset(buf,0,sample_out);
        mute_pcm_bytes -= sample_out;
    }	
    if(!err){
	mute_pcm_thread = 0;

    }

    	
    *outlen = 	sample_out;
    return stream_in_offset;        
}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
#if 0	
    if(hAACIOBuf)
    {
        free(hAACIOBuf);
        hAACIOBuf = NULL;
    }
    if (hAACDecoder)
    {
        AACFreeDecoder(hAACDecoder);
        hAACDecoder = NULL;
    }
#endif 
	if(fd_uio >= 0)
		close(fd_uio);
	fd_uio = -1;
	if(memmap != NULL && memmap != MAP_FAILED)
		munmap(memmap,phys_size);
	printk("WFDAAC audio_dec_release done \n");
	return 0; 
}    
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
	return 0;
}


void audio_set_exit_flag()
{
	exit_flag = 1;
	printk("adec decode exit flag set \n");
}




