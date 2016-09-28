#ifndef STREAM_DECODER_H
#define STREAM_DECODER_H

#include "player_priv.h"


typedef struct stream_decoder {
    char name[16];
    pstream_type type;
    int (*init)(play_para_t *);
    int (*add_header)(play_para_t *);
    int (*release)(play_para_t *);
} stream_decoder_t;

static inline codec_para_t * codec_alloc(void)
{
    return MALLOC(sizeof(codec_para_t));
}
static inline  void codec_free(codec_para_t * codec)
{
    if (codec) {
        FREE(codec);
    }
    codec = NULL;
}
int register_stream_decoder(const stream_decoder_t *decoder);

const stream_decoder_t *find_stream_decoder(pstream_type type);

#endif

