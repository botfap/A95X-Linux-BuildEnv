/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB Parser内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#ifndef _AM_PARSER_INTERNAL_H
#define _AM_PARSER_INTERNAL_H

#include <am_parser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
 
#define PAT_SECTION_HEADER_LEN           (8)

#define PMT_SECTION_HEADER_LEN           (12)
#define PMT_STREAM_HEADER_LEN            (5)

#define SDT_SECTION_HEADER_LEN           (11)
#define SDT_SERVICE_HEADER_LEN           (5)
 
#define MAKE_SHORT_HL(exp)		           ((unsigned short)((exp##_hi<<8)|exp##_lo))

#define DVB_INVALID_PID                  0x1fff
#define DVB_INVALID_ID                   0xffff
#define DVB_INVALID_VERSION              0xff

#define DVB_PSI_PAT_TID                  0x00
#define DVB_PSI_PMT_TID                  0x02
#define DVB_SI_SDT_ACT_TID               0x42
#define DVB_SI_SDT_OTH_TID               0x46

#define DVB_VIDEO_STREAM_DESCRIPTOR      0x02
#define DVB_AUDIO_STREAM_DESCRIPTOR      0x03
#define DVB_CA_DESCRIPTOR                0x09
#define DVB_ISO_639_LANGUAGE_DESCRIPTOR  0x0a
#define DVB_SERVICE_DESCRIPTOR           0x48
#define DVB_MULTILINGUAL_SERVICE_NAME_DESCRIPTOR 0x5d
#define DVB_AC3_DESCRIPTOR               0x6a

/* stream type */

#define DVB_STREAM_TYPE_MPEG1_VIDEO      0x01 /* 11172 video */     
#define DVB_STREAM_TYPE_MPEG2_VIDEO      0x02 /* 13818 video */
#define DVB_STREAM_TYPE_MPEG1_AUDIO      0x03 /* 11172 audio */     
#define DVB_STREAM_TYPE_MPEG2_AUDIO      0x04 /* 13818 audio */
#define DVB_STREAM_TYPE_PRIVATE_SECTION  0x05
#define DVB_STREAM_TYPE_PRIVATE_DATA     0x06
#define DVB_STREAM_TYPE_13522_MHPEG      0x07
#define DVB_STREAM_TYPE_13818_DSMCC      0x08
#define DVB_STREAM_TYPE_ITU_222_1        0x09
#define DVB_STREAM_TYPE_ISO13818_6_A     0x0a
#define DVB_STREAM_TYPE_ISO13818_6_B     0x0b
#define DVB_STREAM_TYPE_ISO13818_6_C     0x0c
#define DVB_STREAM_TYPE_ISO13818_6_D     0x0d

#define DVB_STREAM_TYPE_MPEG4_VIDEO      0x10 /* ISO/IEC 14496-2 */
#define DVB_STREAM_TYPE_MPEG_AAC         0x11 /* ISO/IEC 14496-3 */
#define DVB_STREAM_TYPE_H264_VIDEO       0x1b /* ITU Rec. H.264 |ISO/IEC 14496-10 */

#define DVB_STREAM_TYPE_DC_II_VIDEO      0x80
#define DVB_STREAM_TYPE_AC3_AUDIO        0x81

#define PMT_PARSER_NUM                   (sizeof(pmt_parser)/sizeof(pmt_parser[0]))
#define SDT_PARSER_NUM                   (sizeof(sdt_parser)/sizeof(sdt_parser[0]))
#define DESCRIPTOR_SUPPORT_NUM           ((int)(sizeof(descriptor_map)/sizeof(descriptor_map[0])))

/****************************************************************************
 * Type definitions
 ***************************************************************************/
 
#pragma pack(1)

typedef struct pat_section_header
{
    unsigned char table_id                      :8;
    unsigned char section_length_hi             :4;
    unsigned char                               :3;
    unsigned char section_syntax_indicator      :1;
    unsigned char section_length_lo             :8;
    unsigned char transport_stream_id_hi        :8;
    unsigned char transport_stream_id_lo        :8;
    unsigned char current_next_indicator        :1;
    unsigned char version_number                :5;
    unsigned char                               :2;
    unsigned char section_number                :8;
    unsigned char last_section_number           :8;
}pat_section_header_t;

typedef struct pat_program_info
{
    unsigned char program_number_hi             :8;
    unsigned char program_number_lo             :8;
    unsigned char program_map_pid_hi            :5;
    unsigned char reserved                      :3;
    unsigned char program_map_pid_lo            :8;
}pat_program_info_t;

typedef struct pmt_section_header
{
	  unsigned char table_id                      :8;
    unsigned char section_length_hi             :4;
    unsigned char                               :3;
    unsigned char section_syntax_indicator      :1;
    unsigned char section_length_lo             :8;
    unsigned char program_number_hi             :8;
    unsigned char program_number_lo             :8;
    unsigned char current_next_indicator        :1;
    unsigned char version_number                :5;
    unsigned char                               :2;
    unsigned char section_number                :8;
    unsigned char last_section_number           :8;
    unsigned char pcr_pid_hi                    :5;
    unsigned char                               :3;
    unsigned char pcr_pid_lo                    :8;
    unsigned char program_info_length_hi        :4;
    unsigned char                               :4;
    unsigned char program_info_length_lo        :8;
}pmt_section_header_t;

typedef struct pmt_es_info
{
    unsigned char stream_type                   :8;
    unsigned char elementary_pid_hi             :5;
    unsigned char                               :3;
    unsigned char elementary_pid_lo             :8;
    unsigned char es_info_length_hi             :4;
    unsigned char                               :4;
    unsigned char es_info_length_lo             :8;
}pmt_es_info_t;

typedef struct sdt_section_header
{
    unsigned char table_id                      :8;
    unsigned char section_length_hi             :4;
    unsigned char                               :3;
    unsigned char section_syntax_indicator      :1;
    unsigned char section_length_lo             :8;
    unsigned char transport_stream_id_hi        :8;
    unsigned char transport_stream_id_lo        :8;
    unsigned char current_next_indicator        :1;
    unsigned char version_number                :5;
    unsigned char                               :2;
    unsigned char section_number                :8;
    unsigned char last_section_number           :8;
    unsigned char original_network_id_hi        :8;
    unsigned char original_network_id_lo        :8;
    unsigned char                               :8;
    
}sdt_section_header_t;

typedef struct sdt_service_header
{
    unsigned char service_id_hi                 :8;
    unsigned char service_id_lo                 :8;
    unsigned char eit_present_following_flag    :1;
    unsigned char eit_schedule_flag             :1;
    unsigned char reserved                      :6;
    unsigned char descriptors_loop_length_hi    :4;
    unsigned char free_ca_mode                  :1;
    unsigned char runing_status                 :3;
    unsigned char descriptors_loop_length_lo    :8;

}sdt_service_header_t;

#pragma Pack()

typedef void* (*descriptor_new )(void);
typedef void  (*descriptor_free)(void* desp);
typedef int(*descriptor_parse)(unsigned char* data, unsigned int length, unsigned int descriptor);
typedef void  (*descriptor_dump)(void* desp);

struct descriptor_ctx
{
    char   *name;
    unsigned char   tag;
    descriptor_new new_func;
    descriptor_free free_func;
    descriptor_dump dump_func;
    descriptor_parse parse_func;
};

typedef struct pat_program_map
{
    unsigned short program_number;
    unsigned short program_map_pid;
}pat_program_map_t;

struct pat_section_info
{
    unsigned short transport_stream_id;
    unsigned char  version_number;
    unsigned char  program_num;
    struct pat_program_map *program_map;
    struct pat_section_info *next;
};

typedef struct descriptor
{
    unsigned char   tag;
    unsigned char   length;
    unsigned char   *data;
}descriptor_t;

typedef struct descriptor_info
{
    unsigned char   tag;
    unsigned int    descriptor;
    struct  descriptor_info *next;
}descriptor_info_t;

typedef struct service_descriptor
{
    unsigned char   tag;
    unsigned char   length;
    unsigned char   service_type;
    unsigned char   provider_name_len;
    unsigned char   service_name_len;
    unsigned char   *provider_name;
    unsigned char   *service_name;
}service_descriptor_t;

typedef struct pmt_stream_info
{
    unsigned char   stream_type;
    unsigned short  elementary_pid;
    struct  descriptor_info *es_info;
    struct  pmt_stream_info *next;
}pmt_stream_info_t;

struct pmt_section_info
{
    unsigned short  program_number;
    unsigned char   version_number;
    unsigned short  pcr_pid;
    struct  descriptor_info *program_info;
    struct  pmt_stream_info *stream_info;
    struct  dvb_parser *desp_parser;
};
 
typedef int (*parser_callback)(unsigned char* data, unsigned int length, unsigned int para);

struct dvb_parser
{
    char    *name;
    unsigned char   id;
    unsigned char   mask;
    parser_callback callback;
    struct dvb_parser *next;
};

typedef struct sdt_service_info
{
    unsigned short  service_id;
    unsigned char   eit_schedule_flag;
    unsigned char   eit_present_following_flag;
    unsigned char   runing_status;
    unsigned char   free_ca_mode;
    struct  descriptor_info *desp_info;
    struct  sdt_service_info *next;
}sdt_service_info_t;

struct sdt_section_info
{
    unsigned short  transport_stream_id;
    unsigned short  original_network_id;
    unsigned char   version_number;
    struct  sdt_service_info *service_info;
    struct  dvb_parser *desp_parser;
    struct  sdt_section_info *next;
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/
static int find_descriptor(const struct descriptor_ctx descriptor[], int low, int high, unsigned tag);
static int find_ext_descriptor(unsigned char tag);
static int dvb_descriptor_parse(unsigned char* data, unsigned int length, unsigned int descriptor);
static int dvb_service_descriptor_parse(unsigned char* data, unsigned int length, unsigned int descriptor);
static void *service_descriptor_new(void);
void service_descriptor_free(void *desp);
void service_descriptor_dump(void *desp);


#ifdef __cplusplus
}
#endif

#endif

