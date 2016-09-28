/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tngpkt.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

/*
 * This is generated code, do not modify. Look in
 * /home/gregory/helix/protocol/transport/rdt/pub/tngpkt.pm to make
 * modifications
 */

#include "helix_types.h"
#include "rm_memory.h"

#ifndef PMC_PREDEFINED_TYPES
#define PMC_PREDEFINED_TYPES

typedef char* pmc_string;

typedef struct _buffer {
	 UINT32 len;
	 INT8* data;
} buffer;
#endif/*PMC_PREDEFINED_TYPES*/


#ifndef _TNGPKT_H_
#define _TNGPKT_H_

struct TNGDataPacket
{
    UINT8 length_included_flag;
    UINT8 need_reliable_flag;
    UINT8 stream_id;
    UINT8 is_reliable;
    UINT16 seq_no;
    UINT16 _packlenwhendone;
    UINT8 back_to_back_packet;
    UINT8 slow_data;
    UINT8 asm_rule_number;
    UINT32 timestamp;
    UINT16 stream_id_expansion;
    UINT16 total_reliable;
    UINT16 asm_rule_number_expansion;
    buffer data;
};

const UINT32 TNGDataPacket_static_size();
UINT8* TNGDataPacket_pack(UINT8* buf,
                          UINT32 len,
                          struct TNGDataPacket* pkt);
UINT8* TNGDataPacket_unpack(UINT8* buf,
                            UINT32 len,
                            struct TNGDataPacket* pkt);

struct TNGMultiCastDataPacket
{
    UINT8 length_included_flag;
    UINT8 need_reliable_flag;
    UINT8 stream_id;
    UINT8 is_reliable;
    UINT16 seq_no;
    UINT16 length;
    UINT8 back_to_back_packet;
    UINT8 slow_data;
    UINT8 asm_rule_number;
    UINT8 group_seq_no;
    UINT32 timestamp;
    UINT16 stream_id_expansion;
    UINT16 total_reliable;
    UINT16 asm_rule_number_expansion;
    buffer data;
};

const UINT32 TNGMultiCastDataPacket_static_size();

UINT8* TNGMultiCastDataPacket_pack(UINT8* buf,
                                   UINT32 len,
                                   struct TNGMultiCastDataPacket* pkt);
UINT8* TNGMultiCastDataPacket_unpack(UINT8* buf,
                                     UINT32 len,
                                     struct TNGMultiCastDataPacket* pkt);


struct TNGASMActionPacket
{
    UINT8 length_included_flag;
    UINT8 stream_id;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT16 packet_type;
    UINT16 reliable_seq_no;
    UINT16 length;
    UINT16 stream_id_expansion;
    buffer data;
};

const UINT32 TNGASMActionPacket_static_size();
UINT8* TNGASMActionPacket_pack(UINT8* buf,
                               UINT32 len,
                               struct TNGASMActionPacket* pkt);

UINT8* TNGASMActionPacket_unpack(UINT8* buf,
                                 UINT32 len,
                                 struct TNGASMActionPacket* pkt);



struct TNGBandwidthReportPacket
{
    UINT8 length_included_flag;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT16 packet_type;
    UINT16 length;
    UINT16 interval;
    UINT32 bandwidth;
    UINT8 sequence;
};

const UINT32 TNGBandwidthReportPacket_static_size();
UINT8* TNGBandwidthReportPacket_pack(UINT8* buf,
                                     UINT32 len,
                                     struct TNGBandwidthReportPacket* pkt);
UINT8* TNGBandwidthReportPacket_unpack(UINT8* buf,
                                       UINT32 len,
                                       struct TNGBandwidthReportPacket* pkt);



struct TNGReportPacket
{
    UINT8 length_included_flag;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT16 packet_type;
    UINT16 length;
    buffer data;
};

const UINT32 TNGReportPacket_static_size();
UINT8* TNGReportPacket_pack(UINT8* buf,
                            UINT32 len,
                            struct TNGReportPacket* pkt);
UINT8* TNGReportPacket_unpack(UINT8* buf,
                              UINT32 len,
                              struct TNGReportPacket* pkt);



struct TNGACKPacket
{
    UINT8 length_included_flag;
    UINT8 lost_high;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT16 packet_type;
    UINT16 length;
    buffer data;
};

const UINT32 TNGACKPacket_static_size();
UINT8* TNGACKPacket_pack(UINT8* buf,
                         UINT32 len,
                         struct TNGACKPacket* pkt);
UINT8* TNGACKPacket_unpack(UINT8* buf,
                           UINT32 len,
                           struct TNGACKPacket* pkt);


struct TNGRTTRequestPacket
{
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT8 dummy3;
    UINT16 packet_type;
};

const UINT32 TNGRTTRequestPacket_static_size();
UINT8* TNGRTTRequestPacket_pack(UINT8* buf,
                                UINT32 len,
                                struct TNGRTTRequestPacket* pkt);
UINT8* TNGRTTRequestPacket_unpack(UINT8* buf,
                                  UINT32 len,
                                  struct TNGRTTRequestPacket* pkt);

struct TNGRTTResponsePacket
{
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT8 dummy3;
    UINT16 packet_type;
    UINT32 timestamp_sec;
    UINT32 timestamp_usec;
};

const UINT32 TNGRTTResponsePacket_static_size();
UINT8* TNGRTTResponsePacket_pack(UINT8* buf,
                                 UINT32 len,
                                 struct TNGRTTResponsePacket* pkt);
UINT8* TNGRTTResponsePacket_unpack(UINT8* buf,
                                   UINT32 len,
                                   struct TNGRTTResponsePacket* pkt);


struct TNGCongestionPacket
{
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT8 dummy3;
    UINT16 packet_type;
    INT32 xmit_multiplier;
    INT32 recv_multiplier;
};

const UINT32 TNGCongestionPacket_static_size();
UINT8* TNGCongestionPacket_pack(UINT8* buf,
                                UINT32 len,
                                struct TNGCongestionPacket* pkt);
UINT8* TNGCongestionPacket_unpack(UINT8* buf,
                                  UINT32 len,
                                  struct TNGCongestionPacket* pkt);


struct TNGStreamEndPacket
{
    UINT8 need_reliable_flag;
    UINT8 stream_id;
    UINT8 packet_sent;
    UINT8 ext_flag;
    UINT16 packet_type;
    UINT16 seq_no;
    UINT32 timestamp;
    UINT16 stream_id_expansion;
    UINT16 total_reliable;
    UINT8 reason_dummy[3];
    UINT32 reason_code;
    pmc_string reason_text;
};

const UINT32 TNGStreamEndPacket_static_size();
UINT8* TNGStreamEndPacket_pack(UINT8* buf,
                               UINT32 len,
                               struct TNGStreamEndPacket* pkt);

/* TNGStreamEndPacket_unpack will malloc room for the reason_text.
 * You must free this memory or it will be leaked */
UINT8* TNGStreamEndPacket_unpack(UINT8* buf,
                                 UINT32 len,
                                 struct TNGStreamEndPacket* pkt);

UINT8* TNGStreamEndPacket_unpack_fromPool(UINT8* buf,
                                         UINT32 len,
                                         struct TNGStreamEndPacket* pkt,
                                         rm_malloc_func_ptr fpMalloc,
                                         void* pMemoryPool);




struct TNGLatencyReportPacket
{
    UINT8 length_included_flag;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT16 packet_type;
    UINT16 length;
    UINT32 server_out_time;
};

const UINT32 TNGLatencyReportPacket_static_size();
UINT8* TNGLatencyReportPacket_pack(UINT8* buf,
                                   UINT32 len,
                                   struct TNGLatencyReportPacket* pkt);
UINT8* TNGLatencyReportPacket_unpack(UINT8* buf,
                                     UINT32 len,
                                     struct TNGLatencyReportPacket* pkt);




    /*
     * RDTFeatureLevel 3 packets
     */

struct RDTTransportInfoRequestPacket
{
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 request_rtt_info;
    UINT8 request_buffer_info;
    UINT16 packet_type;
    UINT32 request_time_ms;
};

const UINT32 RDTTransportInfoRequestPacket_static_size();
UINT8* RDTTransportInfoRequestPacket_pack(UINT8* buf,
                                          UINT32 len,
                                          struct RDTTransportInfoRequestPacket* pkt);
UINT8* RDTTransportInfoRequestPacket_unpack(UINT8* buf,
                                            UINT32 len,
                                            struct RDTTransportInfoRequestPacket* pkt);




struct RDTBufferInfo
{
    UINT16 stream_id;
    UINT32 lowest_timestamp;
    UINT32 highest_timestamp;
    UINT32 bytes_buffered;
};

const UINT32 RDTBufferInfo_static_size();
UINT8* RDTBufferInfo_pack(UINT8* buf,
                          UINT32 len, struct RDTBufferInfo* pkt);
UINT8* RDTBufferInfo_unpack(UINT8* buf,
                            UINT32 len, struct RDTBufferInfo* pkt);




struct RDTTransportInfoResponsePacket
{
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 has_rtt_info;
    UINT8 is_delayed;
    UINT8 has_buffer_info;
    UINT16 packet_type;
    UINT32 request_time_ms;
    UINT32 response_time_ms;
    UINT16 buffer_info_count;
    struct RDTBufferInfo *buffer_info;
};

const UINT32 RDTTransportInfoResponsePacket_static_size();
UINT8* RDTTransportInfoResponsePacket_pack(UINT8* buf,
                                           UINT32 len,
                                           struct RDTTransportInfoResponsePacket* pkt);
UINT8* RDTTransportInfoResponsePacket_unpack(UINT8* buf,
                                             UINT32 len,
                                             struct RDTTransportInfoResponsePacket* pkt);

UINT8* RDTTransportInfoResponsePacket_unpack_fromPool(UINT8* buf,
                                                      UINT32 len,
                                                      struct RDTTransportInfoResponsePacket* pkt,
                                                      rm_malloc_func_ptr fpMalloc,
                                                      void* pMemoryPool);






struct TNGBWProbingPacket
{
    UINT8 length_included_flag;
    UINT8 dummy0;
    UINT8 dummy1;
    UINT8 dummy2;
    UINT16 packet_type;
    UINT16 length;
    UINT8 seq_no;
    UINT32 timestamp;
    buffer data;
};

const UINT32 TNGBWProbingPacket_static_size();
UINT8* TNGBWProbingPacket_pack(UINT8* buf,
                               UINT32 len,
                               struct TNGBWProbingPacket* pkt);
UINT8* TNGBWProbingPacket_unpack(UINT8* buf,
                                 UINT32 len,
                                 struct TNGBWProbingPacket* pkt);
#endif /* _TNGPKT_H_ */
