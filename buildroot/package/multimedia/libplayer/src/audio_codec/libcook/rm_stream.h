/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_stream.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RM_STREAM_H
#define RM_STREAM_H

#include "helix_types.h"
#include "helix_result.h"
#include "rm_property.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/*
 * Stream header definition.
 *
 * Users are strongly encouraged to use the accessor
 * functions below to retrieve information from the 
 * rm_stream_header struct, since the definition
 * may change in the future.
 */
typedef struct rm_stream_header_struct
{
    char*        pMimeType;
    char*        pStreamName;
    UINT32       ulStreamNumber;
    UINT32       ulMaxBitRate;
    UINT32       ulAvgBitRate;
    UINT32       ulMaxPacketSize;
    UINT32       ulAvgPacketSize;
    UINT32       ulDuration;
    UINT32       ulPreroll;
    UINT32       ulStartTime;
    UINT32       ulStartOffset;
    UINT32       ulStreamSize;
    UINT32       ulOpaqueDataLen;
    BYTE*        pOpaqueData;
    UINT32       ulNumProperties;
    rm_property* pProperty;
} rm_stream_header;

/*
 * These are the accessor functions used to retrieve
 * information from the stream header
 */
UINT32      rm_stream_get_number(rm_stream_header* hdr);
UINT32      rm_stream_get_max_bit_rate(rm_stream_header* hdr);
UINT32      rm_stream_get_avg_bit_rate(rm_stream_header* hdr);
UINT32      rm_stream_get_max_packet_size(rm_stream_header* hdr);
UINT32      rm_stream_get_avg_packet_size(rm_stream_header* hdr);
UINT32      rm_stream_get_start_time(rm_stream_header* hdr);
UINT32      rm_stream_get_preroll(rm_stream_header* hdr);
UINT32      rm_stream_get_duration(rm_stream_header* hdr);
UINT32      rm_stream_get_data_offset(rm_stream_header* hdr);
UINT32      rm_stream_get_data_size(rm_stream_header* hdr);
const char* rm_stream_get_name(rm_stream_header* hdr);
const char* rm_stream_get_mime_type(rm_stream_header* hdr);
UINT32      rm_stream_get_properties(rm_stream_header* hdr, rm_property** ppProp);
HXBOOL      rm_stream_is_realaudio(rm_stream_header* hdr);
HXBOOL      rm_stream_is_realvideo(rm_stream_header* hdr);
HXBOOL      rm_stream_is_realevent(rm_stream_header* hdr);
HXBOOL      rm_stream_is_realaudio_mimetype(const char* pszStr);
HXBOOL      rm_stream_is_realvideo_mimetype(const char* pszStr);
HXBOOL      rm_stream_is_realevent_mimetype(const char* pszStr);
HXBOOL      rm_stream_is_real_mimetype(const char* pszStr);
HX_RESULT   rm_stream_get_property_int(rm_stream_header* hdr,
                                       const char*       pszStr,
                                       UINT32*           pulVal);
HX_RESULT   rm_stream_get_property_buf(rm_stream_header* hdr,
                                       const char*       pszStr,
                                       BYTE**            ppBuf,
                                       UINT32*           pulLen);
HX_RESULT   rm_stream_get_property_str(rm_stream_header* hdr,
                                       const char*       pszStr,
                                       char**            ppszStr);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RM_STREAM_H */
