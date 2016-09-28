#ifndef LIBFLAC_CODEC_MESSAGE_HEADERS
#define LIBFLAC_CODEC_MESSAGE_HEADERS


#define SUB_FMT_VALID 			(1<<1)
#define CHANNEL_VALID 			(1<<2)	
#define SAMPLE_RATE_VALID     	(1<<3)	
#define DATA_WIDTH_VALID     	(1<<4)	

#define AUDIO_EXTRA_DATA_SIZE  (2048*2)

struct digit_raw_output_info
{
	int	framelength;
	unsigned char* framebuf;
	int frame_size;
	int frame_samples;
	unsigned char* rawptr;
    //for AC3
	int sampleratecode;
	int bsmod;

    int bpf;
    int brst;
    int length;
    int padsize;
    int mode;
    unsigned int syncword1;
    unsigned int syncword2;
    unsigned int syncword3;

    unsigned int syncword1_mask;
    unsigned int syncword2_mask;
    unsigned int syncword3_mask;

    unsigned chstat0_l;
    unsigned chstat0_r;
    unsigned chstat1_l;
    unsigned chstat1_r;

    unsigned can_bypass;
};
struct frame_fmt
{
    int valid;
    int sub_fmt;
    int channel_num;
    int sample_rate;
    int data_width;
    int buffered_len;/*dsp codec,buffered origan data len*/ 
    int format;
    unsigned int total_byte_parsed;
    union{	
    	 unsigned int total_sample_decoded;
        void  *pcm_encoded_info;	//used for encoded pcm info	 
    }data;		 
    unsigned int bps;
    void* private_data;
    struct digit_raw_output_info * digit_raw_output_info;
};

typedef struct pcm51_encoded_info_s
{ 
       unsigned int InfoValidFlag; //only if  InfoValidFlag==1 can userspace start  reading  the data in 51pcm_buf;
	unsigned int SampFs;          //sampling frequency
	unsigned int NumCh;          //total output valid channels( including LFE channel if LFE is valid)
	unsigned int AcMode;         //audio coding mode
	unsigned int LFEFlag;         //indicating the output buffers cotains LFE components if LFEFlag==1
	unsigned int BitsPerSamp; //bits count used to indicates a pcm_samples
}pcm51_encoded_info_t;


struct frame_info
{
    int len;
    unsigned long  offset;/*steam start to here*/
    unsigned long  buffered_len;/*data buffer in  dsp,pcm datalen*/
    int reversed[1];/*for cache aligned 32 bytes*/
};

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

typedef enum{
    DECODE_INIT_ERR,
    DECODE_FATAL_ERR,
}error_code_t;

struct dsp_working_info
{
	int status;
	int sp;
	int pc;
	int ilink1;
	int ilink2;
	int blink;
	int jeffies;
	int out_wp;
	int out_rp;
	int buffered_len;//pcm buffered at the dsp side
	int es_offset;//stream read offset since start decoder
	int reserved[5];
};

#endif

