#ifndef AML_VIDEOHEADER_H_
#define _AML_VIDEOHEADER_H_

#include <gst/gst.h>
#include <../include/codec.h>
#define ADTS_HEADER_SIZE        (7)
#define HDR_BUF_SIZE                (1024)
#if 0
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
#endif
GstFlowReturn videopre_header_feeding(codec_para_t *vpcodec,GstAmlVout *amlvout,GstBuffer ** buf);
int h264_update_frame_header(GstBuffer * *buffer);
//void adts_add_header(GstBuffer * buf);
//int extract_adts_header_info(GstBuffer * buf;
//int divx3_prefix(GstBuffer * buf);
//int get_vc1_di(GstBuffer * buf, int length);
//int h264_write_end_header(GstAmlAmlPlugin *amlplugin,GstBuffer **buffer);
#endif
