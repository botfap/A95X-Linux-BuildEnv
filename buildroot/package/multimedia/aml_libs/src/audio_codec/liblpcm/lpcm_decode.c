#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <cutils/properties.h>
#include <unistd.h>

#include "../../amadec/adec-armdec-mgt.h"
#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "LPCMDEC"
#define  printk(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  audio_codec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define printk printf
#define  audio_codec_print printf
#endif 

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
static volatile int exit_flag = 0;
static char *memmap = MAP_FAILED;	
static int phys_size;
static unsigned enable_debug_print = 0;

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
        audio_codec_print("unable to open file %s", path);
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
		printk("waiting_bits \n"); 
		usleep(1000);
		bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
	}	
}
static unsigned stream_in_offset = 0;

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
	while(size > iii && (!exit_flag)){
		iii = READ_MPEG_REG(AIU_MEM_AIFIFO_LEVEL)-EXTRA_DATA_SIZE;
		
	}
	if(exit_flag){
		printk("exit flag set.exit dec\n");
		return 0;
	}	
	//if(( size >=  iii))
	//    return 0;
	
//	adec_print("read_buffer start while iii= %d!!\n", iii);
	for(len=0;len<size;)
	{	
			space=(size-len);
			bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
			//printk("read_buffer start AIU_MEM_AIFIFO_BYTES_AVAIL bytes= %d!!,exit %d \n", bytes,exit_flag);
			if(exit_flag){
				printk("exit 1 \n");
				return 0;
			}	
			wait_times=0;
			while(bytes==0)
			{
				waiting_bits((space>128)?128*8:(space*8));	/*wait 32 bytes,if the space is less than 32 bytes,wait the space bits*/
				bytes=READ_MPEG_REG(AIU_MEM_AIFIFO_BYTES_AVAIL);
				
//				audio_codec_print("read_buffer while AIU_MEM_AIFIFO_BYTES_AVAIL = %d!!\n", bytes);
				wait_times++;
				if(wait_times>10 || exit_flag) {					
					audio_codec_print("goto out!!\n");
					goto out;
				}
			}
			bytes=min(space,bytes);
			
			//adec_print("read_buffer while bytes = %d!!\n", bytes);
			for(i=0;i<bytes;i++)
			{
				if(exit_flag){
									printk("exit 2 \n");

					return 0;
				}	
				while(!AIFIFO_READY){
					fifo_ready_wait++;
					usleep(1000);
					printk("fifo not ready \n");
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

 

#define	pcm_buffer_size		(1024*6)
#define bluray_pcm_size		(1024*17)


static short table[256];
static unsigned char pcm_buffer[bluray_pcm_size];

#define	LOCAL	inline

#define	SIGN_BIT		(0x80)
#define	QUANT_MASK	(0xf)
#define	NSEGS			(8)
#define	SEG_SHIFT		(4)
#define	SEG_MASK		(0x70)

#define	BIAS			(0x84)

static int pcm_channels = 0;
static int pcm_samplerate = 0;
static int pcm_datewidth = 16;
static int pcm_bluray_header = 0;
static int pcm_bluray_size = 0;

static struct audio_info *pcm_info;


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

static int lpcm_header_parsed = 0;
static int parse_wifi_display_pcm_header(char *header, int *bps)
{
    char number_of_frame_header, audio_emphasis, quant, sample, channel;
    int frame_size = -1;
    
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
        }
        else
        {
            printk("using reserved bps %d\n", *bps);
        }
        
        if (sample == Audio_Sampling_44_1)
        {
            pcm_samplerate = 44100;
        }
        else if(sample == Audio_Sampling_48)
        {
           pcm_samplerate = 48000;
        }
        else
        {
            printk("using reserved sample_rate %d\n",pcm_samplerate);
        }
        
        if (channel == Audio_channel_Dual_Mono)
        {
            pcm_channels = 1;   //note: this is not sure
        }
        else if(channel == Audio_channel_Stero)
        {
            pcm_channels = 2;
        }
        else
        {
            printk("using reserved channel %d\n", pcm_channels);
        }
        
        
	    frame_size = FramesPerAU * (*bps >> 3) * pcm_channels * number_of_frame_header;
	    
	}
    else
    {
        printk("unknown sub id\n");
    }
    
    return frame_size;
}
	
int frame_size_check_flag=0;
int frame_size_check=0;
int jump_read_head_flag=0;
int audio_dec_init(audio_decoder_operations_t *adp)
{
       printk("\n\n[%s]WFD LPCMDEC BuildDate--%s  BuildTime--%s",__FUNCTION__,__DATE__,__TIME__);
#ifdef ANDROID
	char value[PROPERTY_VALUE_MAX];		
	if( property_get("media.wfd.debug_dec",value,NULL) > 0)
	{
		enable_debug_print = atoi(value);
	}
#else
       char *value;
       value=getenv("media.wfd.debug_dec");
	if(value!= NULL)
	{
            enable_debug_print = atoi(value);
	}
#endif
	stream_in_offset = 0;
	exit_flag = 0;
	int err;
	err = uio_init	();
	if(err)
		return -1;
	printk("LPCM--- audio_dec_init done \n");
	return 0;
}




int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *buf, int *outlen, char *inbuf, int inlen)
{
	short *sample;
	unsigned char *src;
	int size, n, i, j, bps, wifi_display_drop_header = 0;
	int sample_size;
	unsigned int header;
	int16_t *dst_int16_t;
	int32_t *dst_int32_t;
	int64_t *dst_int64_t;
	unsigned short *dst_uint16_t;
	unsigned int  *dst_uint32_t;
	int frame_size;
	int  skip_bytes = 0;
	sample = (short *)buf;
	src = pcm_buffer;
	*outlen = 0;

        //check audio info for wifi display LPCM
        size = read_buffer(pcm_buffer, Wifi_Display_Private_Header_Size);
resync:
	/*
	while(get_audiobuf_level()*1000 /(4*48000)  > 300){
		printk("skip byte buffer level %d \n",get_audiobuf_level());
		read_buffer(pcm_buffer,1024);
	 }	
	 */
	 if(exit_flag == 1){
		printk("exit flag set.exit dec1\n");	 	
	 	return 0;
	 }
	 if(enable_debug_print)
        	printk("wifi display: pcm read size%d %x-%x-%x-%x\n", size, pcm_buffer[0], pcm_buffer[1],pcm_buffer[2],pcm_buffer[3]);
        
        if (pcm_buffer[0] == 0xa0)
        {
    	     frame_size = parse_wifi_display_pcm_header( pcm_buffer, &bps);
	     if(frame_size > 1920){
		 	printk("frame size error ??? %d \n",frame_size);
			goto skipbyte;
	     }
            size = read_buffer(pcm_buffer, frame_size);

        }
        else
        {
skipbyte:   
	     pcm_buffer[0] = pcm_buffer[1];
	     pcm_buffer[1] = pcm_buffer[2];
	     pcm_buffer[2] = pcm_buffer[3];		 
	     read_buffer(&pcm_buffer[3],1);
	     skip_bytes++; 
	     goto resync;
            frame_size = Wifi_Display_Private_Header_Size;    //transimit error or something?
        }
	if(enable_debug_print)	
       	printk("wifi display: pcm read size%d %x-%x-%x-%x,skip bytes %d \n", size, pcm_buffer[0], pcm_buffer[1],pcm_buffer[2],pcm_buffer[3],skip_bytes);
	 	
        if(bps == 16){
            if(pcm_channels == 1){
                for(i=0,j=0; i<frame_size; ){
                    sample[j+1] = sample[j] = (pcm_buffer[i]<<8)|pcm_buffer[i+1];
                    i+=2;
                    j+=2;
                }
            }else if(pcm_channels == 2){
                for(i=0,j=0; i<frame_size; ){
                    sample[j++] = (pcm_buffer[i]<<8)|pcm_buffer[i+1]; i+=2;
                    sample[j++] = (pcm_buffer[i]<<8)|pcm_buffer[i+1]; i+=2;
                }
            }
            *outlen = frame_size;

	 
		/*
			before output the audio frame,check the audio buffer level to see if we need drop pcm 
			
		*/		
	     unsigned audio_latency = get_audiobuf_level()*1000/(48000*4);
	    // printk("audio latency %d \n",audio_latency);
	     memcpy(inbuf,&audio_latency,sizeof( audio_latency));
	    if(enable_debug_print)
			printk("sample rate %d, ch %d \n",pcm_samplerate,pcm_channels);
	    if(pcm_samplerate > 0 && pcm_channels > 0  ){
			adec_ops->channels= pcm_channels;
			adec_ops->samplerate = pcm_samplerate;
	    }			
            return stream_in_offset;
        }
        else{
            printk("wifi display:unimplemented bps %d\n", bps);
        }
        
	
    return stream_in_offset;
}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
	if(fd_uio >= 0)
		close(fd_uio);
	fd_uio = -1;
	if(memmap != NULL && memmap != MAP_FAILED)
		munmap(memmap,phys_size);
	printk("audio_dec_release done \n");
	return 0;
}


int audio_dec_getinfo(audio_decoder_operations_t *adec_ops)
{
	return 0;
}
void audio_set_exit_flag()
{
	exit_flag = 1;
	printk("adec decode exit flag set \n");
}
