#ifndef _PLAYER_HWDEC_H_
#define _PLAYER_HWDEC_H_

#include <player_error.h>
#include "player_av.h"
#include "player_priv.h"

#define ADTS_HEADER_SIZE        (7)
#define HDR_BUF_SIZE                (1024)

typedef struct {
    unsigned short syncword;
    unsigned short aac_frame_length;
    unsigned short adts_buffer_fullness;

    unsigned char id: 1;
    unsigned char layer: 2;
    unsigned char protection_absent: 1;
    unsigned char profile: 2;
    unsigned char original_copy: 1;
    unsigned char home: 1;

    unsigned char sample_freq_idx: 4;
    unsigned char private_bit: 1;
    unsigned char channel_configuration: 3;

    unsigned char copyright_identification_bit: 1;
    unsigned char copyright_identification_start: 1;
    unsigned char number_of_raw_data_blocks_in_frame: 2;
} adts_header_t;

typedef struct AVIStream {
    int64_t frame_offset; /* current frame (video) or byte (audio) counter
                         (used to compute the pts) */
    int remaining;
    int packet_size;

    int scale;
    int rate;
    int sample_size; /* size of one sample (or packet) (in the rate/scale sense) in bytes */

    int64_t cum_len; /* temporary storage (used during seek) */

    int prefix;                       ///< normally 'd'<<8 + 'c' or 'w'<<8 + 'b'
    int prefix_count;
    uint32_t pal[256];
    int has_pal;
    int dshow_block_align;            ///< block align variable used to emulate bugs in the MS dshow demuxer

    AVFormatContext *sub_ctx;
    AVPacket sub_pkt;
    uint8_t *sub_buffer;

    int64_t seek_pos;
    int sequence_head_size;
    unsigned int sequence_head_offset;
    char *sequence_head;
} AVIStream;

int pre_header_feeding(play_para_t *para);
int h264_add_frame_header(unsigned char* data, int size);
int h264_update_frame_header(am_packet_t *pkt);
void adts_add_header(play_para_t *para);
int extract_adts_header_info(play_para_t *para);
int divx3_prefix(am_packet_t *pkt);
int mpeg_check_sequence(play_para_t *para);
int get_vc1_di(unsigned char *data, int length);
int h264_write_end_header(play_para_t *para);

// hevc/h.265
int hevc_update_frame_header(am_packet_t *pkt);
int vp9_update_frame_header(am_packet_t *pkt);
#endif
