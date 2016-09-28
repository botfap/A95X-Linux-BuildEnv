/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB Parser
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#ifndef _AM_PARSER_H
#define _AM_PARSER_H

#include "am_types.h"
#include "dvb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
 
 /* stream type */

#define DVB_STREAM_TYPE_MPEG1_VIDEO                          0x01 /* 11172 video */     
#define DVB_STREAM_TYPE_MPEG2_VIDEO                          0x02 /* 13818 video */
#define DVB_STREAM_TYPE_MPEG1_AUDIO                          0x03 /* 11172 audio */     
#define DVB_STREAM_TYPE_MPEG2_AUDIO                          0x04 /* 13818 audio */
#define DVB_STREAM_TYPE_PRIVATE_SECTION                      0x05
#define DVB_STREAM_TYPE_PRIVATE_DATA                         0x06
#define DVB_STREAM_TYPE_13522_MHPEG                          0x07
#define DVB_STREAM_TYPE_13818_DSMCC                          0x08
#define DVB_STREAM_TYPE_ITU_222_1                            0x09
#define DVB_STREAM_TYPE_ISO13818_6_A                         0x0a
#define DVB_STREAM_TYPE_ISO13818_6_B                         0x0b
#define DVB_STREAM_TYPE_ISO13818_6_C                         0x0c
#define DVB_STREAM_TYPE_ISO13818_6_D                         0x0d

#define DVB_STREAM_TYPE_MPEG4_VIDEO                          0x10 /* ISO/IEC 14496-2 */
#define DVB_STREAM_TYPE_MPEG_AAC                             0x11 /* ISO/IEC 14496-3 */
#define DVB_STREAM_TYPE_H264_VIDEO                           0x1b /* ITU Rec. H.264 |ISO/IEC 14496-10 */

#define DVB_STREAM_TYPE_DC_II_VIDEO                          0x80
#define DVB_STREAM_TYPE_AC3_AUDIO                            0x81


/****************************************************************************
 * Error code definitions
 ****************************************************************************/
/**\brief PARSER模块错误代码*/
enum AM_PARSER_ErrorCode
{
	AM_PARSER_ERROR_BASE=AM_ERROR_BASE(AM_MOD_PARSER),
	AM_PARSER_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_PARSER_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_PARSER_ERR_SYS,                     /**< 系统操作错误*/
	AM_PARSER_ERR_NO_DATA,                 /**< 没有收到数据*/
	AM_PARSER_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct pat_section_info pat_section_info_t;
typedef struct pmt_section_info pmt_section_info_t;
typedef struct sdt_section_info sdt_section_info_t;
typedef struct dvb_parser dvb_parser_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

extern AM_ErrorCode_t dvb_psi_parse_pat(const unsigned char *data, int length, pat_section_info_t *info);
extern AM_ErrorCode_t dvb_psi_parse_pmt(const unsigned char *data, int length, pmt_section_info_t *info);
extern AM_ErrorCode_t dvb_si_parse_sdt(const unsigned char *data, int length, sdt_section_info_t *info);
extern void dvb_psi_free_pat_info(pat_section_info_t *info);
extern void dvb_psi_free_pmt_info(pmt_section_info_t *info);
extern void dvb_si_free_sdt_info(sdt_section_info_t *info);
extern void dvb_psi_dump_pat_info(pat_section_info_t *info);
extern void dvb_psi_dump_pmt_info(pmt_section_info_t *info);
extern void dvb_si_dump_sdt_info(sdt_section_info_t *info, aml_dvb_channel_info_t **listChannels, int freq);
extern void dvb_psi_clear_pat_info(pat_section_info_t *info);
extern void dvb_psi_get_program_info(pat_section_info_t *info, int program_no, unsigned short *program_map_pid, unsigned short *program_number);
extern pat_section_info_t *dvb_psi_new_pat_info(void);
extern pmt_section_info_t *dvb_psi_new_pmt_info(void);
extern sdt_section_info_t *dvb_si_new_sdt_info(void);
extern int dvb_psi_get_pmt_numbers(pat_section_info_t *info);
extern int dvb_psi_get_program_stream_info(pmt_section_info_t *info, aml_dvb_param_t *parm);

#ifdef __cplusplus
}
#endif

#endif

