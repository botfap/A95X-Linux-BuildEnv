#ifndef _LIBAACDEC_H_
#define _LIBAACDEC_H_

//header file
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>

// ffmpeg headers
#include <stdlib.h>
#include "common.h"
#include "structs.h"


#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "neaacdec.h"
//#include "../../amadec/adec_write.h"
#include "../../amadec/adec-armdec-mgt.h"
//#include <mp4ff.h>
//int main(int argc, char *argv[]);

//extern audio_decoder_operations_t AudioAacDecoder;
int audio_dec_init(audio_decoder_operations_t *adec_ops);
int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen);
int audio_dec_release(audio_decoder_operations_t *adec_ops);
int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo);
//void libaacdec(void);
#endif


