/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_backend_types.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RV_BACKEND_TYPES_H
#define RV_BACKEND_TYPES_H

#include "rv_format_info.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

typedef struct rv_backend_init_params_struct
{
    UINT16 usOuttype;
    UINT16 usPels;
    UINT16 usLines;
    UINT16 usPadWidth;   /* number of columns of padding on right to get 16 x 16 block*/
    UINT16 usPadHeight;  /* number of rows of padding on bottom to get 16 x 16 block*/

    UINT16 pad_to_32;   /* to keep struct member alignment independent of */
                        /* compiler options */
    UINT32 ulInvariants;
        /* ulInvariants specifies the invariant picture header bits */
    INT32  bPacketization;
    UINT32 ulStreamVersion;
} rv_backend_init_params;

typedef struct rv_backend_in_params_struct
{
    UINT32 dataLength;
    INT32  bInterpolateImage;
    UINT32 numDataSegments;
    rv_segment *pDataSegments;
    UINT32 flags;
        /* 'flags' should be initialized by the front-end before each */
        /* invocation to decompress a frame.  It is not updated by the decoder. */
        /* */
        /* If it contains RV_DECODE_MORE_FRAMES, it informs the decoder */
        /* that it is being called to extract the second or subsequent */
        /* frame that the decoder is emitting for a given input frame. */
        /* The front-end should set this only in response to seeing */
        /* an RV_DECODE_MORE_FRAMES indication in H263DecoderOutParams. */
        /* */
        /* If it contains RV_DECODE_DONT_DRAW, it informs the decoder */
        /* that it should decode the image (in order to produce a valid */
        /* reference frame for subsequent decoding), but that no image */
        /* should be returned.  This provides a "hurry-up" mechanism. */
    UINT32 timestamp;
} rv_backend_in_params;

typedef struct rv_backend_out_params_struct
{
    UINT32 numFrames;
    UINT32 notes;
        /* 'notes' is assigned by the transform function during each call to */
        /* decompress a frame.  If upon return the notes parameter contains */
        /* the indication RV_DECODE_MORE_FRAMES, then the front-end */
        /* should invoke the decoder again to decompress the same image. */
        /* For this additional invocation, the front-end should first set */
        /* the RV_DECODE_MORE_FRAMES bit in the 'H263DecoderInParams.flags' */
        /* member, to indicate to the decoder that it is being invoked to */
        /* extract the next frame. */
        /* The front-end should continue invoking the decoder until the */
        /* RV_DECODE_MORE_FRAMES bit is not set in the 'notes' member. */
        /* For each invocation to decompress a frame in the same "MORE_FRAMES" */
        /* loop, the front-end should send in the same input image. */
        /* */
        /* If the decoder has no frames to return for display, 'numFrames' will */
        /* be set to zero.  To avoid redundancy, the decoder does *not* set */
        /* the RV_DECODE_DONT_DRAW bit in 'notes' in this case. */


    UINT32 timestamp;
        /* The 'temporal_offset' parameter is used in conjunction with the */
        /* RV_DECODE_MORE_FRAMES note, to assist the front-end in */
        /* determining when to display each returned frame. */
        /* If the decoder sets this to T upon return, the front-end should */
        /* attempt to display the returned image T milliseconds relative to */
        /* the front-end's idea of the presentation time corresponding to */
        /* the input image. */
        /* Be aware that this is a signed value, and will typically be */
        /* negative. */

    UINT32 width;
    UINT32 height;
        /* Width and height of the returned frame. */
        /* This is the width and the height as signalled in the bitstream. */

} rv_backend_out_params;


/* definitions for output parameter notes */

#define RV_DECODE_MORE_FRAMES           0x00000001
#define RV_DECODE_DONT_DRAW             0x00000002
#define RV_DECODE_KEY_FRAME             0x00000004
    /* Indicates that the decompressed image is a key frame. */
    /* Note that enhancement layer EI frames are not key frames, in the */
    /* traditional sense, because they have dependencies on lower layer */
    /* frames. */

#define RV_DECODE_B_FRAME               0x00000008
    /* Indicates that the decompressed image is a B frame. */
    /* At most one of PIA_DDN_KEY_FRAME and PIA_DDN_B_FRAME will be set. */

#define RV_DECODE_DEBLOCKING_FILTER     0x00000010
    /* Indicates that the returned frame has gone through the */
    /* deblocking filter. */

#define RV_DECODE_FRU_FRAME             0x00000020
    /* Indicates that the decompressed image is a B frame. */
    /* At most one of PIA_DDN_KEY_FRAME and PIA_DDN_B_FRAME will be set. */

#define RV_DECODE_SCRAMBLED_BUFFER      0x00000040
    /* Indicates that the input buffer is scrambled for security */
    /* decoder should de-scramble the buffer before use it */

#define RV_DECODE_LAST_FRAME            0x00000200
    /* Indicates that the accompanying input frame is the last in the */
    /* current sequence. If input frame is a dummy frame, the decoder */
    /* flushes the latency frame to the output. */

/* definitions for decoding opaque data in bitstream header */
/* Defines match ilvcmsg.h so that ulSPOExtra == rv10init.invariants */
#define RV40_SPO_FLAG_UNRESTRICTEDMV        0x00000001  /* ANNEX D */
#define RV40_SPO_FLAG_EXTENDMVRANGE         0x00000002  /* IMPLIES NEW VLC TABLES */
#define RV40_SPO_FLAG_ADVMOTIONPRED         0x00000004  /* ANNEX F */
#define RV40_SPO_FLAG_ADVINTRA              0x00000008  /* ANNEX I */
#define RV40_SPO_FLAG_INLOOPDEBLOCK         0x00000010  /* ANNEX J */
#define RV40_SPO_FLAG_SLICEMODE             0x00000020  /* ANNEX K */
#define RV40_SPO_FLAG_SLICESHAPE            0x00000040  /* 0: free running; 1: rect */
#define RV40_SPO_FLAG_SLICEORDER            0x00000080  /* 0: sequential; 1: arbitrary */
#define RV40_SPO_FLAG_REFPICTSELECTION      0x00000100  /* ANNEX N */
#define RV40_SPO_FLAG_INDEPENDSEGMENT       0x00000200  /* ANNEX R */
#define RV40_SPO_FLAG_ALTVLCTAB             0x00000400  /* ANNEX S */
#define RV40_SPO_FLAG_MODCHROMAQUANT        0x00000800  /* ANNEX T */
#define RV40_SPO_FLAG_BFRAMES               0x00001000  /* SETS DECODE PHASE */
#define RV40_SPO_BITS_DEBLOCK_STRENGTH      0x0000e000  /* deblocking strength */
#define RV40_SPO_BITS_NUMRESAMPLE_IMAGES    0x00070000  /* max of 8 RPR images sizes */
#define RV40_SPO_FLAG_FRUFLAG               0x00080000  /* FRU BOOL: if 1 then OFF; */
#define RV40_SPO_FLAG_FLIP_FLIP_INTL        0x00100000  /* FLIP-FLOP interlacing; */
#define RV40_SPO_FLAG_INTERLACE             0x00200000  /* de-interlacing prefilter has been applied; */
#define RV40_SPO_FLAG_MULTIPASS             0x00400000  /* encoded with multipass; */
#define RV40_SPO_FLAG_INV_TELECINE          0x00800000  /* inverse-telecine prefilter has been applied; */
#define RV40_SPO_FLAG_VBR_ENCODE            0x01000000  /* encoded using VBR; */
#define RV40_SPO_BITS_DEBLOCK_SHIFT            13
#define RV40_SPO_BITS_NUMRESAMPLE_IMAGES_SHIFT 16

#define OUT_OF_DATE_DECODER                 0x00000001
#define OK_VERSION                          0x00000000

#define CORRUPTED_BITSTREAM                 0x00
#define OK_DECODE                           0x0f
#define INCOMPLETE_FRAME                    0xffff
#define MALLOC_FAILURE                      0x1111

#define RV10_DITHER_PARAMS                  0x00001001
#define RV10_POSTFILTER_PARAMS              0x00001002
#define RV10_ADVANCED_MP_PARAMS             0x0001003
#define RV10_TEMPORALINTERP_PARAMS          0x00001004

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* RV_BACKEND_TYPES_H */
