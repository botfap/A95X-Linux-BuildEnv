// $Id: DTSTranscode1m5.h,v 1.14 2007-08-09 01:41:03 dyee Exp $
#ifndef DTSTRANSCODE1M5_H_INCLUDED
#define DTSTRANSCODE1M5_H_INCLUDED

/*typedef struct pcm51_encoded_info_s
{ 
  unsigned int InfoValidFlag;
	unsigned int SampFs;
	unsigned int NumCh;
	unsigned int AcMode;
	unsigned int LFEFlag;
	unsigned int BitsPerSamp;
}pcm51_encoded_info_t;*/

int dts_transenc_init();
int dts_transenc_process_frame();
int dts_transenc_deinit();

#endif // DTSTRANSCODE1M5_H_INCLUDED
