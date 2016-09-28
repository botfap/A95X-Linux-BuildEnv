/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: helix_utils.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef HELIX_UTILS_H
#define HELIX_UTILS_H

#include "helix_config.h"

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */


/*
 * General support macros
 *
 */

#define HX_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define HX_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/*
 * Macro: IS_BIG_ENDIAN()
 *
 * Endianness detection macro.
 * Requires local test_int = 0xFF for correct operation.
 * Set ARCH_IS_BIG_ENDIAN in helix_config.h for best performance.
 * 
 */

#if !defined(ARCH_IS_BIG_ENDIAN)
#define IS_BIG_ENDIAN(test_int) ((*((char*)&test_int)) == 0)
#else
#define IS_BIG_ENDIAN(test_int) ARCH_IS_BIG_ENDIAN
#endif

/*
 * Macro: HX_GET_MAJOR_VERSION()
 *
 * Given the encoded product version,
 * returns the major version number.
 */
#define HX_GET_MAJOR_VERSION(prodVer)   ((prodVer >> 28) & 0xF)

/*
 * Macro: HX_GET_MINOR_VERSION()
 *
 * Given the encoded product version,
 * returns the minor version number.
 */
#define HX_GET_MINOR_VERSION(prodVer)   ((prodVer >> 20) & 0xFF)

/*
 * Macro: HX_ENCODE_PROD_VERSION()
 *
 * Given the major version, minor version,
 * release number, and build number,
 * returns the encoded product version.
 */
#define HX_ENCODE_PROD_VERSION(major,minor,release,build)   \
            ((ULONG32)((ULONG32)major << 28) | ((ULONG32)minor << 20) | \
            ((ULONG32)release << 12) | (ULONG32)build)

#define HX_GET_PRIVATE_FIELD(ulversion)(ulversion & (UINT32)0xFF)

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */


#endif /* HELIX_UTILS_H */

