/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_decode.h,v 1.1.1.1.2.2 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RV_DECODE_H__
#define RV_DECODE_H__

/* Simple unified decoder frontend for RealVideo */

#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"
#include "rm_error.h"
#include "rv_format_info.h"
#include "rv_decode_message.h"
#include "rv_backend.h"
#include "rv_backend_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The rv_decode struct contains the RealVideo decoder frontend
 * state variables and backend instance pointer. */

typedef struct rv_decode_struct
{
    rm_error_func_ptr          fpError;
        /* User defined error function.                                   */

    void*                      pUserError;
        /* User defined parameter for error function.                     */

    rm_malloc_func_ptr         fpMalloc;
        /* User defined malloc function.                                  */

    rm_free_func_ptr           fpFree;
        /* User defined free function.                                    */

    void*                      pUserMem;
        /* User defined parameter for malloc and free functions.          */

    UINT32                     ulSPOExtra;
        /* The SPO Extra bits. Opaque data appended to bitstream header.  */

    UINT32                     ulStreamVersion;
        /* The stream version. Opaque data following the SPO Extra bits.  */

    UINT32                     ulMajorBitstreamVersion;
        /* The major bitstream version. Indicates RV7, RV8, RV9, etc.     */

    UINT32                     ulMinorBitstreamVersion;
        /* The minor bitstream version. Indicates minor revision or RAW.  */

    UINT32                     ulNumResampledImageSizes;
        /* The number of RPR sizes. Optional RV7 and RV8 opaque data.     */

    UINT32                     ulEncodeSize;
        /* The maximum encoded frame dimensions. Optional RV9 opaque data.*/

    UINT32                     ulLargestPels;
        /* The maximum encoded frame width.                               */

    UINT32                     ulLargestLines;
        /* The maximum encoded frame height.                              */

    UINT32                     pDimensions[2*(8+1)];
        /* Table of encoded dimensions, including RPR sizes.              */

    UINT32                     ulOutSize;             
        /* The maximum size of the output frame in bytes.                 */

    UINT32                     ulECCMask;
        /* Mask for identifying ECC packets.                              */

    UINT32                     bInputFrameIsReference;
        /* Identifies whether input frame is a key frame or not.          */

    UINT32                     ulInputFrameQuant;
        /* The input frame quantization parameter.                        */

    rv_format_info            *pBitstreamHeader;
        /* The bitstream header.                                          */

    rv_frame                  *pInputFrame;      
        /* Pointer to the input frame struct.                             */

    void                      *pDecodeState;        
        /* Pointer to decoder backend state.                              */

    rv_backend                *pDecode; 
        /* Decoder backend function pointers.                             */

    rv_backend_init_params     pInitParams;    
        /* Initialization parameters for the decoder backend.             */

    rv_backend_in_params       pInputParams;
        /* The decoder backend input parameter struct.                    */

    rv_backend_out_params      pOutputParams;
        /* The decoder backend output parameter struct.                   */

} rv_decode;

/* rv_decode_create()
 * Creates RV decoder frontend struct, copies memory utilities.
 * Returns struct pointer on success, NULL on failure. */
rv_decode* rv_decode_create(void* pUserError, rm_error_func_ptr fpError);

rv_decode* rv_decode_create2(void* pUserError, rm_error_func_ptr fpError,
                             void* pUserMem, rm_malloc_func_ptr fpMalloc,
                             rm_free_func_ptr fpFree);

/* rv_decode_destroy()
 * Deletes decoder backend instance, followed by frontend. */
void rv_decode_destroy(rv_decode* pFrontEnd);

/* rv_decode_init()
 * Reads bitstream header, selects and initializes decoder backend.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT rv_decode_init(rv_decode* pFrontEnd, rv_format_info* pHeader);

/* rv_decode_stream_input()
 * Reads frame header and fills decoder input parameters struct. If there
 * is packet loss and ECC packets exist, error correction is attempted.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT rv_decode_stream_input(rv_decode* pFrontEnd, rv_frame* pFrame);

/* rv_decode_stream_decode()
 * Calls decoder backend to decode issued frame and produce an output frame.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT rv_decode_stream_decode(rv_decode* pFrontEnd, UINT8* pOutput);

/* rv_decode_stream_flush()
 * Flushes the latency frame from the decoder backend after the last frame
 * is delivered and decoded before a pause or the end-of-file.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT rv_decode_stream_flush(rv_decode* pFrontEnd, UINT8* pOutput);

/* rv_decode_custom_message()
 * Sends a custom message to the decoder backend.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT rv_decode_custom_message(rv_decode* pFrontEnd, RV_Custom_Message_ID *pMsg_id);


/**************** Accessor Functions *******************/
/* rv_decode_max_output_size()
 * Returns maximum size of YUV 4:2:0 output buffer in bytes. */
UINT32 rv_decode_max_output_size(rv_decode* pFrontEnd);

/* rv_decode_get_output_size()
 * Returns size of most recent YUV 4:2:0 output buffer in bytes. */
UINT32 rv_decode_get_output_size(rv_decode* pFrontEnd);

/* rv_decode_get_output_dimensions()
 * Returns width and height of most recent YUV output buffer. */
HX_RESULT rv_decode_get_output_dimensions(rv_decode* pFrontEnd, UINT32* pWidth,
                                          UINT32* pHeight);

/* rv_decode_frame_valid()
 * Checks decoder output parameters to see there is a valid output frame.
 * Returns non-zero value if a valid output frame exists, else zero. */
UINT32 rv_decode_frame_valid(rv_decode* pFrontEnd);

/* rv_decode_more_frames()
 * Checks decoder output parameters to see if more output frames can be
 * produced without additional input frames.
 * Returns non-zero value if more frames can be
 * produced without additional input, else zero. */
UINT32 rv_decode_more_frames(rv_decode* pFrontEnd);

#ifdef __cplusplus
}
#endif

#endif /* RV_DECODE_H__ */
