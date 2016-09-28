/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra_depack.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RA_DEPACK_H
#define RA_DEPACK_H

#include "helix_types.h"
#include "helix_result.h"
#include "rm_error.h"
#include "rm_memory.h"
#include "rm_stream.h"
#include "rm_packet.h"
#include "ra_format_info.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/* Encoded audio block definition. */
typedef struct ra_block_struct
{
    BYTE*  pData;
    UINT32 ulDataLen;
    UINT32 ulTimestamp;
    UINT32 ulDataFlags;
} ra_block;

/* Callback functions */
typedef HX_RESULT (*ra_block_avail_func_ptr) (void*     pAvail,
                                              UINT32    ulSubStream,
                                              ra_block* block);

/*
 * ra_depack definition. Opaque to user.
 */
typedef void ra_depack;

/*
 * ra_depack_create
 *
 * Users should use this function if they don't need to
 * customize their memory allocation routines. In this
 * case, the SDK will use malloc() and free() for 
 * memory allocation.
 */
ra_depack* ra_depack_create(void*                   pAvail,
                            ra_block_avail_func_ptr fpAvail,
                            void*                   pUserError,
                            rm_error_func_ptr       fpError);

/*
 * ra_depack_create2
 *
 * Users should use this function if they need to
 * customize the memory allocation routines.
 */
ra_depack* ra_depack_create2(void*                   pAvail,
                             ra_block_avail_func_ptr fpAvail,
                             void*                   pUserError,
                             rm_error_func_ptr       fpError,
                             void*                   pUserMem,
                             rm_malloc_func_ptr      fpMalloc,
                             rm_free_func_ptr        fpFree);

/*
 * ra_depack_init
 *
 * The depacketizer will be initialized with a RealAudio stream header.
 */
HX_RESULT ra_depack_init(ra_depack* pDepack, rm_stream_header* pHdr);

/*
 * ra_depack_get_num_substreams
 *
 * This accessor function tells how many audio substreams there are
 * in this audio stream. For single-rate files, this will be 1. For
 * SureStream audio streams, this could be greater than 1.
 */
UINT32 ra_depack_get_num_substreams(ra_depack* pDepack);

/*
 * ra_depack_get_codec_4cc
 *
 * This accessor function returns the 4cc of the codec. This 4cc
 * will be used to determine which codec to use for this substream.
 */
UINT32 ra_depack_get_codec_4cc(ra_depack* pDepack, UINT32 ulSubStream);

/*
 * ra_depack_get_codec_init_info
 *
 * This function fills in the structure which is used to initialize the codec.
 */
HX_RESULT ra_depack_get_codec_init_info(ra_depack*       pDepack,
                                        UINT32           ulSubStream,
                                        ra_format_info** ppInfo);

/*
 * ra_depack_destroy_codec_init_info
 *
 * This function frees the memory associated with the ra_format_info object
 * created by ra_depack_get_codec_init_info().
 */
void ra_depack_destroy_codec_init_info(ra_depack*       pDepack,
                                       ra_format_info** ppInfo);

/*
 * ra_depack_add_packet
 *
 * Put an audio packet into the depacketizer. When enough data is
 * present to deinterleave, then the user will be called back
 * on the rm_block_avail_func_ptr set in ra_depack_create().
 */
HX_RESULT ra_depack_add_packet(ra_depack* pDepack, rm_packet* pPacket);

/*
 * ra_depack_destroy_block
 *
 * This method cleans up a block received via the
 * ra_block_avail_func_ptr callback.
 */
void ra_depack_destroy_block(ra_depack* pDepack, ra_block** ppBlock);

/*
 * ra_depack_seek
 *
 * The user calls this function if the stream is seeked. It
 * should be called before passing any post-seek packets.
 */
HX_RESULT ra_depack_seek(ra_depack* pDepack, UINT32 ulTime);

/*
 * ra_depack_destroy
 *
 * This cleans up all memory allocated by the ra_depack_* calls
 */
void ra_depack_destroy(ra_depack** ppDepack);


#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RA_DEPACK_H */
