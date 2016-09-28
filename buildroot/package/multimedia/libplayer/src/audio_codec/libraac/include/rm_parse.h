/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_parse.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RM_PARSE_H
#define RM_PARSE_H

#include <stdio.h>
#include "helix_types.h"
#include "helix_result.h"
#include "rm_memory.h"
#include "rm_error.h"
#include "rm_property.h"
#include "rm_stream.h"
#include "rm_packet.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/*
 * Seek origin definitions - these are
 * exactly the same as the respective
 * definitions in <stdio.h>
 */
#define HX_SEEK_ORIGIN_SET 0
#define HX_SEEK_ORIGIN_CUR 1
#define HX_SEEK_ORIGIN_END 2

/* Function pointer definitions */
typedef UINT32 (*rm_read_func_ptr) (void*   pUserRead,
                                    BYTE*   pBuf, /* Must be at least ulBytesToRead long */
                                    UINT32  ulBytesToRead);
typedef void (*rm_seek_func_ptr) (void*  pUserRead,
                                  UINT32 ulOffset,
                                  UINT32 ulOrigin);

/*
 * rm_parser definition. This is opaque to the user.
 */
typedef void rm_parser;

/*
 * rm_parser_is_rm_file()
 *
 * Function which examines pBuf[0] up through pBuf[ulSize-1] and
 * tries to determine if the buffer contains the beginning of an .rm file
 */
HXBOOL rm_parser_is_rm_file(BYTE* pBuf, UINT32 ulSize);

/*
 * rm_parser_create
 *
 * This creates an rm_parser object. The user may provide an
 * optional error reporting function as well as a void* 
 * which will be passed into fpError. If fpError is NULL, then
 * the parser will not print out any error messages by default
 * and the user will have to interpret the error by the
 * return value.
 */
rm_parser* rm_parser_create(void*             pError,
                            rm_error_func_ptr fpError);

/*
 * rm_parser_create2
 *
 * This creates an rm_parser object, using custom memory
 * allocation and free functions that the user provides.
 * If fpMalloc or fpFree are NULL, then the default
 * allocation and free functions will be used.
 */
rm_parser* rm_parser_create2(void*              pError,
                             rm_error_func_ptr  fpError,
                             void*              pMem,
                             rm_malloc_func_ptr fpMalloc,
                             rm_free_func_ptr   fpFree);

/*
 * rm_parser_init_stdio
 *
 * Users would call this is they wanted to use stdio. This file
 * would need to already be fopen()'d and would need to be fclose()'d
 * after the library was finished. The parsing library would only
 * call fread(), fseek(), and perhaps ftell() on it.
 */
HX_RESULT rm_parser_init_stdio(rm_parser* pParser,
                               FILE*      fp);

/*
 * rm_parser_init_io
 *
 * If the user didn't want to use stdio, they would call
 * this function to set up their own I/O.
 */
HX_RESULT rm_parser_init_io(rm_parser*        pParser,
                            void*             pUserRead,
                            rm_read_func_ptr  fpRead,
                            rm_seek_func_ptr  fpSeek);

/*
 * rm_parser_read_headers
 *
 * This function reads all the headers at the beginning
 * of an .rm file. After this function is called,
 * then the rm_get_xxx accessor functions can be
 * called to get information about the file as a whole.
 */
HX_RESULT rm_parser_read_headers(rm_parser* pParser);

/*
 * File Info Accessor functions
 *
 * The most commonly-used properties of an .rm file can
 * be accessed via the rm_parser_get_xxx accessor functions
 * below. These functions return information about the .rm file as
 * a whole, as opposed to stream header properties, which give
 * information about a particular stream (i.e. - audio stream
 * or video stream).
 *
 * For the title meta-data, the user can determine if this information
 * is present by calling rm_parser_get_title_length(). This length
 * is the length in bytes of the string INCLUDING the null
 * terminator. If the title meta-data IS present, then the user
 * can get read-only access to the title by calling rm_parser_get_title().
 * The user must not alter the memory that is returned; otherwise undefined
 * behavior will result.
 *
 * The same applies for author, copyright, and comment meta-data.
 */
UINT32      rm_parser_get_max_bit_rate(rm_parser* pParser);
UINT32      rm_parser_get_avg_bit_rate(rm_parser* pParser);
UINT32      rm_parser_get_max_packet_size(rm_parser* pParser);
UINT32      rm_parser_get_avg_packet_size(rm_parser* pParser);
UINT32      rm_parser_get_num_packets(rm_parser* pParser);
UINT32      rm_parser_get_duration(rm_parser* pParser);
UINT32      rm_parser_get_preroll(rm_parser* pParser);
const char* rm_parser_get_title(rm_parser* pParser);
const char* rm_parser_get_author(rm_parser* pParser);
const char* rm_parser_get_copyright(rm_parser* pParser);
const char* rm_parser_get_comment(rm_parser* pParser);
UINT32      rm_parser_get_num_streams(rm_parser* pParser);
UINT32 		rm_parser_get_stream_number(rm_parser* pParser, UINT32 ulStreamNum);

/*
 * General file property accessors
 *
 * The most commonly-used properties of an .rm file
 * can be obtained with the rm_parser_get_xxx accessors above.
 * Alternatively, all of the file-level properties may
 * be obtained as an array of rm_property structs. This
 * array will include all of the common-used properties
 * above as well as any other properties that may be
 * present in the file.
 */
HX_RESULT rm_parser_get_file_properties(rm_parser*    pParser,
                                        rm_property** ppProp,
                                        UINT32*       pulNumProps);

/*
 * This function destroys an array of properties created
 * by either rm_parser_get_file_properties or
 * by rm_parser_get_stream_properties.
 */
void rm_parser_destroy_properties(rm_parser*    pParser,
                                  rm_property** ppProp,
                                  UINT32*       pulNumProps);

/*
 * rm_parser_get_stream_header
 *
 * This retrieves a stream header from the file. The number
 * of stream headers in the file can be found by calling
 * rm_get_num_streams(). Stream header properties may
 * be obtained by using the accessor functions found
 * in rm_stream.h. These accessors cover the most
 * commonly used properties. The entire set of properties
 * may be retrieved using rm_parser_get_stream_properties().
 */
HX_RESULT rm_parser_get_stream_header(rm_parser*         pParser,
                                      UINT32             ulStreamNum,
                                      rm_stream_header** ppHdr);

/*
 * rm_parser_destroy_stream_header
 *
 * This function frees any memory associated with
 * a stream header obtained by calling rm_parser_get_stream_header().
 */
void rm_parser_destroy_stream_header(rm_parser*         pParser,
                                     rm_stream_header** ppHdr);

/*
 * rm_parser_get_packet
 *
 * After the headers had been read, the user can repeatedly
 * call this function to retrieve packets from the .rm file.
 */
HX_RESULT rm_parser_get_packet(rm_parser*  pParser,
                               rm_packet** ppPacket);

/*
 * rm_parser_destroy_packet
 *
 * This is called to clean up packets created by
 * calls to rm_parser_get_packet().
 */
void rm_parser_destroy_packet(rm_parser*  pParser,
                              rm_packet** ppPacket);

/*
 * rm_parser_seek
 *
 * The user can call this function to seek to a
 * certain time in the .rm file
 */
HX_RESULT rm_parser_seek(rm_parser* pParser,
                         UINT32     ulTime);

/*
 * rm_parser_seek
 *
 * The user can call this function to seek to a
 * certain offset in the .rm file
 */
void rm_parser_file_seek(rm_parser* pParser, UINT32 ulOffset);

/*
 * rm_parser_skip
 *
 * The user can call this function to skip to a
 * certain offset from current position
 */
void rm_parser_file_skip(rm_parser* pParser, UINT32 ulOffset);

/*
 * rm_parser_build_seek_table
 *
 * The user can call this function to build a seek table
 * by index in the .rm file
 */
HX_RESULT rm_parser_build_seek_table(rm_parser* pParser);

/*
 * rm_parser_seek_in_seek_table
 *
 * The user can call this function to seek in the seek table
 * to find a certain time in the .rm file
 */
HX_RESULT rm_parser_seek_in_seek_table(rm_parser* pParser, INT32 lStreamNumber, UINT32 ulSeekTime, INT32 lDirection, UINT32* pulFoundTime, UINT32* pulDataOffset, UINT32* pulIndex);

/*
 * rm_parser_destroy
 *
 * The user calls this to clean up
 */
void rm_parser_destroy(rm_parser** ppParser);

UINT32 rm_parser_get_data_size(rm_parser* pParser);
UINT32 rm_parser_get_data_offset(rm_parser* pParser);
UINT32 rm_parser_get_index_offset(rm_parser* pParser);
void rm_parser_set_index_offset(rm_parser* pParser, UINT32 offset);
void rm_parser_set_stream(rm_parser** ppParser, UINT32 ulStreamNum);
void rm_parser_set_stream_size(rm_parser* pParser, UINT32 ulStreamSize);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RM_PARSE_H */
