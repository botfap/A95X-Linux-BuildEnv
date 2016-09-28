/**
* @file
 * @brief rfc4184
 * @author yangle <le.yang@amlogic.com>
 *
 */

#include "libavutil/base64.h"
#include "libavutil/avstring.h"
#include "libavcodec/get_bits.h"
#include "avformat.h"
#include "mpegts.h"

#include <unistd.h>
#include "network.h"
#include <assert.h>

#include "rtpdec.h"
#include "rtpdec_formats.h"
/**
    RTP/AC3 specific private data.
*/
struct PayloadContext {
	uint8_t buf[10 * RTP_MAX_PACKET_LENGTH];
	int datasize;
};

static PayloadContext *ac3_new_context(void)
{
    return av_mallocz(sizeof(PayloadContext));
}

static void ac3_free_context(PayloadContext *data)
{
    if (!data)
        return;
    av_free(data);
}


// return 0 on packet, no more left, 1 on packet, 1 on partial packet...
static int ac3_handle_packet(AVFormatContext *ctx,
                              PayloadContext *data,
                              AVStream *st,
                              AVPacket * pkt,
                              uint32_t * timestamp,
                              const uint8_t * buf,
                              int len, int flags)
{
    int result= 0;

    // two-octet Payload Header
    const uint8_t *src= buf + 2;
    int src_len= len -2;

    // parse the param
    int Markerbit = (flags & RTP_FLAG_MARKER) ? 1 : 0;
    int FT = buf[0]&0x03;
    int NT = buf[1];
    // av_log(ctx, AV_LOG_ERROR, " Markerbit=%d, FT=%d, NT=%d \n", Markerbit, FT, NT);

    if(Markerbit || FT == 0){
    	if(data->datasize > 0){
    		av_new_packet(pkt, src_len+data->datasize);
    		memcpy(pkt->data, data->buf, data->datasize);
    		memcpy(pkt->data+data->datasize, src, src_len);
    	}
    	else{
    		av_new_packet(pkt, src_len);
    		memcpy(pkt->data, src, src_len);
    	}
    	pkt->stream_index=st->index;
    	data->datasize=0;
    } 
    else{
	memcpy(data->buf + data->datasize, src, src_len);
	data->datasize+=src_len;

	result=-1;
    }
    return result;
}

/**
This is the structure for expanding on the dynamic rtp protocols (makes everything static. yay!)
*/
RTPDynamicProtocolHandler ff_ac3_dynamic_handler = {
    .enc_name         = "ac3",
    .codec_type       = AVMEDIA_TYPE_AUDIO,
    .codec_id         = CODEC_ID_AC3,
    .parse_sdp_a_line = NULL,
    .alloc              = ac3_new_context,
    .free               = ac3_free_context,
    .parse_packet     = ac3_handle_packet
};
