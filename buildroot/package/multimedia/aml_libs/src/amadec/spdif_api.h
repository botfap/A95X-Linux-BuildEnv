#ifndef __SPDIF_API_H__
#define __SPDIF_API_H__

enum{
STREAM_AC3 = 0,
STREAM_DTS,
STREAM_PCM,
};

#define AUDIO_SPDIF_GET_958_BUF_SIZE      	 _IOR('s', 0x01, unsigned long)
#define AUDIO_SPDIF_GET_958_BUF_CONTENT   	 _IOR('s', 0x02, unsigned long)
#define AUDIO_SPDIF_GET_958_BUF_SPACE     	 _IOR('s', 0x03, unsigned long)
#define AUDIO_SPDIF_GET_958_BUF_RD_OFFSET	 _IOR('s', 0x04, unsigned long)	
#define AUDIO_SPDIF_GET_958_ENABLE_STATUS	 _IOR('s', 0x05, unsigned long)	
#define AUDIO_SPDIF_GET_I2S_ENABLE_STATUS	 _IOR('s', 0x06, unsigned long)	
#define AUDIO_SPDIF_SET_958_ENABLE	   	 	 _IOW('s', 0x07, unsigned long)	
#define AUDIO_SPDIF_SET_958_INIT_PREPARE	 _IOW('s', 0x08, unsigned long)	
#define AUDIO_SPDIF_SET_958_WR_OFFSET	   	 _IOW('s', 0x09, unsigned long)	

extern int iec958_init();
extern int iec958_pack_frame(char *buf,int frame_size);
extern int iec958_packed_frame_write_958buf(char *buf,int frame_size);
extern int iec958_deinit();
extern int iec958_check_958buf_level();
extern int iec958buf_fill_zero();
#endif
