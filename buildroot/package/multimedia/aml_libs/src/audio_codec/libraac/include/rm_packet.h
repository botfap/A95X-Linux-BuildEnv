/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_packet.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RM_PACKET_H
#define RM_PACKET_H

#include "helix_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/*
 * Packet struct
 *
 * Users are strongly encouraged to use the
 * accessor functions below to retrieve information
 * from the packet, since the definition of this
 * struct may change in the future.
 */
typedef struct rm_packet_struct
{
    UINT32 ulTime;
    UINT16 usStream;
    UINT16 usASMFlags;
    BYTE   ucASMRule;
    BYTE   ucLost;
    UINT16 usDataLen;
    BYTE*  pData;
} rm_packet;

/*
 * Packet Accessor functions
 */
UINT32 rm_packet_get_timestamp(rm_packet* packet);
UINT32 rm_packet_get_stream_number(rm_packet* packet);
UINT32 rm_packet_get_asm_flags(rm_packet* packet);
UINT32 rm_packet_get_asm_rule_number(rm_packet* packet);
UINT32 rm_packet_get_data_length(rm_packet* packet);
BYTE*  rm_packet_get_data(rm_packet* packet);
HXBOOL rm_packet_is_lost(rm_packet* packet);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RM_PACKET_H */
