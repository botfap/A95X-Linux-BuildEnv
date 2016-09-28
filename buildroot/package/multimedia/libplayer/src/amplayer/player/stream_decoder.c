#include <player_error.h>

#include "stream_decoder.h"
#include "player_priv.h"

#define MAX_DECODER (16)
static const stream_decoder_t *stream_decoder_list[MAX_DECODER];
static int stream_index = 0;

int register_stream_decoder(const stream_decoder_t *decoder)
{
    if (decoder != NULL && stream_index < MAX_DECODER) {
        stream_decoder_list[stream_index++] = decoder;
    } else {
        return PLAYER_FAILED;
    }
    return PLAYER_SUCCESS;
}

const stream_decoder_t *find_stream_decoder(pstream_type type)
{
    int i;
    const stream_decoder_t *decoder;

    for (i = 0; i < stream_index; i++) {
        decoder = stream_decoder_list[i];
        if (type == decoder->type) {
            return decoder;
        }
    }
    return NULL;
}

