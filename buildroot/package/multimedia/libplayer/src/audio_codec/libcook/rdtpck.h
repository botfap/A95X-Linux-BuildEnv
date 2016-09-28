/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rdtpck.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RDTPACKET_H
#define RDTPACKET_H

#include <string.h>
#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"

#ifdef __cplusplus
extern "C" {
#endif
    

/*
 * 
 * Packet util functions and types
 *
 */
    
#define RDT_ASM_ACTION_PKT       0xFF00
#define RDT_BW_REPORT_PKT        0xFF01
#define RDT_ACK_PKT              0xFF02
#define RDT_RTT_REQUEST_PKT      0xFF03
#define RDT_RTT_RESPONSE_PKT     0xFF04
#define RDT_CONGESTION_PKT       0xFF05
#define RDT_STREAM_END_PKT       0xFF06
#define RDT_REPORT_PKT           0xFF07
#define RDT_LATENCY_REPORT_PKT   0xFF08
#define RDT_TRANS_INFO_RQST_PKT  0xFF09
#define RDT_TRANS_INFO_RESP_PKT  0xFF0A
#define RDT_BANDWIDTH_PROBE_PKT  0xFF0B

#define RDT_DATA_PACKET          0xFFFE
#define RDT_UNKNOWN_TYPE         0xFFFF

    

/*
 *
 * The tngpkt.h file included below is auto-generated from the master
 * header file which contains the bit by bit definitions of the packet
 * structures(tngpkt.pm). The structures defined in tngpkt.h are
 * easier to use compared to the actual packet definitions in
 * tngpkt.pm. tngpkt.h also includes pack and unpack functions that
 * can convert these structures to and from the binary wire format of
 * the different packet types.
 *
 */
#include "tngpkt.h"


/* All seq numbers will be marked as lost [beg, end]. */
/* Caller is responsible for free'ing the bit-field that is allocated
 * ( "pkt->data.data").
 */
void createNAKPacket( struct TNGACKPacket* pkt,
                      UINT16 unStreamNum,
                      UINT16 unBegSeqNum,
                      UINT16 unEndSeqNum
                      );

void createNAKPacketFromPool( struct TNGACKPacket* pkt,
                              UINT16 unStreamNum,
                              UINT16 unBegSeqNum,
                              UINT16 unEndSeqNum,
                              rm_malloc_func_ptr fpMalloc,
                              void*  pMemoryPool
                              );



/* All seq numbers will be marked as received [beg, end].  The caller
 * must free the 'pkt->data.data' malloc'ed memory.
 */
void createACKPacket( struct TNGACKPacket* pkt,
                      UINT16 unStreamNum,
                      UINT16 unBegSeqNum,
                      UINT16 unEndSeqNum
                      );

void createACKPacketFromPool( struct TNGACKPacket* pkt,
                              UINT16 unStreamNum,
                              UINT16 unBegSeqNum,
                              UINT16 unEndSeqNum,
                              rm_malloc_func_ptr fpMalloc,
                              void*  pMemoryPool
                              );

/*
 * Returns the packet type of the packet in the buffer that was just
 * read off of the wire
 */
UINT16 GetRDTPacketType(UINT8* pBuf, UINT32 nLen);



/* 
 * Marshalling inline funcs.
 */

UINT8 getbyte(UINT8* data);
UINT16 getshort(UINT8* data);
INT32 getlong(UINT8* data);                           
void putbyte(UINT8* data, INT8 v);
void putshort(UINT8* data, UINT16 v);
void putlong(UINT8* data, UINT32 v);
UINT8* addbyte(UINT8* cp, UINT8 data);
UINT8* addshort(UINT8* cp, UINT16 data);
UINT8* addlong(UINT8* cp, UINT32 data);
UINT8* addstring(UINT8* cp, const UINT8* string, int len);

#ifdef __cplusplus
}
#endif 

#endif /* RDTPACKET_H */
