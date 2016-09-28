/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra_depack_internal.h,v 1.1.1.1.2.1 2005/05/04 18:21:33 hubbe Exp $
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

#ifndef RA_DEPACK_INTERNAL_H
#define RA_DEPACK_INTERNAL_H

#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"
#include "rm_error.h"
#include "ra_depack.h"
#include "stream_hdr_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Internal substream header struct
 */
typedef struct ra_substream_hdr_struct
{
    UINT16      usRAFormatVersion;     /* 3, 4, or 5 */
    UINT16      usRAFormatRevision;    /* should be 0 */
    UINT16      usHeaderBytes;         /* size of raheader info */
    UINT16      usFlavorIndex;         /* compression type */
    UINT32      ulGranularity;         /* size of one block of encoded data */
    UINT32      ulTotalBytes;          /* total bytes of ra data */
    UINT32      ulBytesPerMin;         /* data rate of encoded and interleaved data */
    UINT32      ulBytesPerMin2;        /* data rate of interleaved or non-interleaved data */
    UINT32      ulInterleaveFactor;    /* number of blocks per superblock */
    UINT32      ulInterleaveBlockSize; /* size of each interleave block */
    UINT32      ulCodecFrameSize;      /* size of each audio frame */
    UINT32      ulUserData;            /* extra field for user data */
    UINT32      ulSampleRate;          /* sample rate of decoded audio */
    UINT32      ulActualSampleRate;    /* sample rate of decoded audio */
    UINT32      ulSampleSize;          /* bits per sample in decoded audio */
    UINT32      ulChannels;            /* number of audio channels in decoded audio */
    UINT32      ulInterleaverID;       /* interleaver 4cc */
    UINT32      ulCodecID;             /* codec 4cc */
    BYTE        bIsInterleaved;        /* 1 if file has been interleaved */
    BYTE        bCopyByte;             /* copy enable byte, if 1 allow copies (SelectiveRecord) */
    BYTE        ucStreamType;          /* i.e. LIVE_STREAM, FILE_STREAM */
    BYTE        ucScatterType;         /* the interleave pattern type 0==cyclic,1==pattern */
    UINT32      ulNumCodecFrames;      /* number of codec frames in a superblock */
    UINT32*     pulInterleavePattern;  /* the pattern of interleave if not cyclic */
    UINT32      ulOpaqueDataSize;      /* size of the codec specific data */
    BYTE*       pOpaqueData;           /* codec specific data */
    HXDOUBLE    dBlockDuration;        /* Duration in ms of audio "block" */
    UINT32      ulLastSentEndTime;     /* Ending time of last sent audio frame */
    BYTE*       pFragBuffer;           /* Intermediate buffer for reconstructing VBR packets */
    UINT32      ulFragBufferSize;      /* Size of intermediate buffer */
    UINT32      ulFragBufferAUSize;    /* Size of AU being reconstructed */
    UINT32      ulFragBufferOffset;    /* Current offset within AU */
    UINT32      ulFragBufferTime;      /* Timestamp of AU being reconstructed */
    UINT32      ulSuperBlockSize;      /* ulInterleaveBlockSize * ulInterleaveFactor */
    UINT32      ulSuperBlockTime;      /* dBlockDuration * ulInterleaveFactor */
    UINT32      ulKeyTime;             /* Timestamp of keyframe packet */
    BYTE*       pIBuffer;              /* Buffer holding interleaved blocks    */
    BYTE*       pDBuffer;              /* Buffer holding de-interleaved blocks */
    UINT32*     pIPresentFlags;        /* number of UINT32s: ulInterleaveBlockSize */
    UINT32*     pDPresentFlags;        /* number of UINT32s: ulInterleaveBlockSize */
    UINT32      ulBlockCount;          /* number of blocks currently in superblock */
    UINT32*     pulGENRPattern;        /* Interleave pattern for GENR interleaver */
    UINT32*     pulGENRBlockNum;
    UINT32*     pulGENRBlockOffset;
    rm_packet   lastPacket;
    HX_BITFIELD bIsVBR : 1;
    HX_BITFIELD bSeeked : 1;
    HX_BITFIELD bLossOccurred : 1;
    HX_BITFIELD bHasKeyTime : 1;       /* Do we have a time for the key slot? */
    HX_BITFIELD bHasFrag : 1;
    HX_BITFIELD bAdjustTimestamps : 1;
    HX_BITFIELD bKnowIfAdjustNeeded : 1;
    HX_BITFIELD bHasLastPacket : 1;
} ra_substream_hdr;

/*
 * Internal ra_depack struct
 */
typedef struct ra_depack_internal_struct
{
    void*                   pAvail;
    ra_block_avail_func_ptr fpAvail;
    rm_error_func_ptr       fpError;
    void*                   pUserError;
    rm_malloc_func_ptr      fpMalloc;
    rm_free_func_ptr        fpFree;
    void*                   pUserMem;
    rm_rule_map             rule2Flag;
    rm_multistream_hdr      multiStreamHdr;
    ra_substream_hdr*       pSubStreamHdr;
    UINT32                  ulTrackStartTime;
    UINT32                  ulTrackEndTime;
    UINT32                  ulEndTime;
    UINT32                  ulStreamDuration;
    HX_BITFIELD             bForceTrackStartTime : 1;
    HX_BITFIELD             bForceTrackEndTime : 1;
    HX_BITFIELD             bStreamSwitchable : 1;
    HX_BITFIELD             bAllVBR : 1;
    HX_BITFIELD             bAllNonVBR : 1;
    HX_BITFIELD             bHasEndTime : 1;
} ra_depack_internal;

/*
 * Internal ra_depack functions
 */
void*     ra_depacki_malloc(ra_depack_internal* pInt, UINT32 ulSize);
void      ra_depacki_free(ra_depack_internal* pInt, void* pMem);
HX_RESULT ra_depacki_init(ra_depack_internal* pInt, rm_stream_header* hdr);
HX_RESULT ra_depacki_unpack_rule_map(ra_depack_internal* pInt,
                                     rm_rule_map* pMap,
                                     BYTE**              ppBuf,
                                     UINT32*             pulLen);
HX_RESULT ra_depacki_unpack_multistream_hdr(ra_depack_internal* pInt,
                                            BYTE**              ppBuf,
                                            UINT32*             pulLen);
HX_RESULT ra_depacki_unpack_opaque_data(ra_depack_internal* pInt,
                                        BYTE*               pBuf,
                                        UINT32              ulLen);
void      ra_depacki_cleanup_substream_hdr(ra_depack_internal* pInt,
                                           ra_substream_hdr*   hdr);
void      ra_depacki_cleanup_substream_hdr_array(ra_depack_internal* pInt);
HX_RESULT ra_depacki_unpack_substream_hdr(ra_depack_internal* pInt,
                                          BYTE*               pBuf,
                                          UINT32              ulLen,
                                          ra_substream_hdr*   pHdr);
HX_RESULT ra_depacki_unpack_raformat3(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr);
HX_RESULT ra_depacki_unpack_raformat4(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr);
HX_RESULT ra_depacki_unpack_raformat5(ra_depack_internal* pInt,
                                      BYTE*               pBuf,
                                      UINT32              ulLen,
                                      ra_substream_hdr*   pHdr);
HX_RESULT ra_depacki_get_format_info(ra_depack_internal* pInt,
                                     UINT32              ulSubStream,
                                     ra_format_info*     pInfo);
void      ra_depacki_cleanup_format_info(ra_depack_internal* pInt,
                                         ra_format_info*     pInfo);
UINT32    ra_depacki_rule_to_flags(ra_depack_internal* pInt, UINT32 ulRule);
HXBOOL    ra_depacki_is_keyframe_rule(ra_depack_internal* pInt, UINT32 ulRule);
UINT32    ra_depacki_rule_to_substream(ra_depack_internal* pInt, UINT32 ulRule);
HX_RESULT ra_depacki_add_packet(ra_depack_internal* pInt,
                                rm_packet*          pPacket);
HX_RESULT ra_depacki_add_vbr_packet(ra_depack_internal* pInt,
                                    UINT32              ulSubStream,
                                    rm_packet*          pPacket);
HX_RESULT ra_depacki_add_non_vbr_packet(ra_depack_internal* pInt,
                                        UINT32              ulSubStream,
                                        rm_packet*          pPacket);
HX_RESULT ra_depacki_parse_vbr_packet(ra_depack_internal* pInt,
                                      rm_packet*          pPacket,
                                      UINT32*             pulNumAU,
                                      HXBOOL*             pbFragmented,
                                      UINT32*             pulAUSize,
                                      UINT32*             pulAUFragSize);
HX_RESULT ra_depacki_generate_and_send_loss(ra_depack_internal* pInt,
                                            UINT32              ulSubStream,
                                            UINT32              ulFirstStartTime,
                                            UINT32              ulLastEndTime);
HX_RESULT ra_depacki_send_block(ra_depack_internal* pInt,
                                UINT32              ulSubStream,
                                BYTE*               pBuf,
                                UINT32              ulLen,
                                UINT32              ulTime,
                                UINT32              ulFlags);
HX_RESULT ra_depacki_handle_frag_packet(ra_depack_internal* pInt,
                                        UINT32              ulSubStream,
                                        rm_packet*          pPacket,
                                        UINT32              ulAUSize,
                                        UINT32              ulAUFragSize);
HX_RESULT ra_depacki_handle_nonfrag_packet(ra_depack_internal* pInt,
                                           UINT32              ulSubStream,
                                           rm_packet*          pPacket,
                                           UINT32              ulNumAU);
HX_RESULT ra_depacki_init_frag_buffer(ra_depack_internal* pInt,
                                      ra_substream_hdr*   pHdr);
HX_RESULT ra_depacki_resize_frag_buffer(ra_depack_internal* pInt,
                                        ra_substream_hdr*   pHdr,
                                        UINT32              ulNewSize);
void      ra_depacki_clear_frag_buffer(ra_depack_internal* pInt,
                                       ra_substream_hdr*   hdr);
HX_RESULT ra_depacki_seek(ra_depack_internal* pInt, UINT32 ulTime);
HX_RESULT ra_depacki_deinterleave_send(ra_depack_internal* pInt, UINT32 ulSubStream);
HX_RESULT ra_depacki_deinterleave(ra_depack_internal* pInt, UINT32 ulSubStream);
HX_RESULT ra_depacki_deinterleave_sipr(ra_depack_internal* pInt, UINT32 ulSubStream);
HX_RESULT ra_depacki_init_genr(ra_depack_internal* pInt, UINT32 ulSubStream);
HX_RESULT ra_depacki_deinterleave_genr(ra_depack_internal* pInt, UINT32 ulSubStream);
HX_RESULT ra_depacki_deinterleave_no(ra_depack_internal* pInt, UINT32 ulSubStream);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef RA_DEPACK_INTERNAL_H */
