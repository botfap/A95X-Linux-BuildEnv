#ifndef LIBFLAC_HEADER
#define LIBFLAC_HEADER
#include "codec_message.h"

struct codec_type
{
	char *name;
	int (*init)(struct frame_fmt *);
	int (*release)(void);	
	// return BYTEs number for the samples
	int (*decode_frame)(unsigned char *,int,struct frame_fmt *); 
};

/*data in*/
int read_bits(int bits);
int bits_left(void);
int reset_bits(void);
int read_byte(void);
int read_buffer(unsigned char *buffer,int size);
int get_inbuf_data_size(void);
unsigned long  get_stream_in_offset(void);
int stream_in_buffer_init(void);
	
/*data out*/
int write_buffer(unsigned char *buf,int size);
int get_outbuf_space(void);
int out_buffer_init(void);

/*mgt*/
int codec_start(void);
int codec_resume(void);
int codec_pause(void);
int register_codec(const struct codec_type	 *mc);
void decode_error_msg(int error);
void trans_err_code(error_code_t error);

#endif
