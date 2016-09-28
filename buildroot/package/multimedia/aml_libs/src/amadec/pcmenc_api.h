#ifndef __PCMENC_API_H
#define __PCMENC_API_H

typedef struct pcm51_encoded_info_s
{ 
    unsigned int InfoValidFlag;
	unsigned int SampFs;
	unsigned int NumCh;
	unsigned int AcMode;
	unsigned int LFEFlag;
	unsigned int BitsPerSamp;
}pcm51_encoded_info_t;


#define AUDIODSP_PCMENC_GET_RING_BUF_SIZE      _IOR('l', 0x01, unsigned long)
#define AUDIODSP_PCMENC_GET_RING_BUF_CONTENT   _IOR('l', 0x02, unsigned long)
#define AUDIODSP_PCMENC_GET_RING_BUF_SPACE     _IOR('l', 0x03, unsigned long)
#define AUDIODSP_PCMENC_SET_RING_BUF_RPTR	   _IOW('l', 0x04, unsigned long)
#define AUDIODSP_PCMENC_GET_PCMINFO	   	       _IOR('l', 0x05, unsigned long)	


extern int  pcmenc_init();
extern int  pcmenc_read_pcm(char *inputbuf,int size);
extern int  pcmenc_deinit();
extern int  pcmenc_get_pcm_info(pcm51_encoded_info_t *info);
#endif