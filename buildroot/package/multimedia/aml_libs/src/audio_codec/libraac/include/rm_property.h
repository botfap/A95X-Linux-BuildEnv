/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_property.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef RM_PROPERTY_H
#define RM_PROPERTY_H

#include "helix_types.h"

#define RM_PROPERTY_TYPE_UINT32  0
#define RM_PROPERTY_TYPE_BUFFER  1
#define RM_PROPERTY_TYPE_CSTRING 2

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */

/*
 * Property struct
 *
 * This struct can hold a UINT32 property, a CString
 * property, or a buffer property. The members
 * of this struct are as follows:
 *
 * pName:        NULL-terminated string which holds the name
 * ulType:       This can be either RM_PROPERTY_TYPE_UINT32,
 *                 RM_PROPERTY_TYPE_BUFFER, or RM_PROPERTY_TYPE_CSTRING.
 * pValue and ulValueLen:
 *     1) For type RM_PROPERTY_TYPE_UINT32, the value is held in
 *        the pValue pointer itself and ulValueLen is 0.
 *     2) For type RM_PROPERTY_TYPE_BUFFER, the value is held in
 *        the buffer pointed to by pValue and the length of
 *        the buffer is ulValueLen.
 *     3) For type RM_PROPERTY_TYPE_CSTRING, the value is a
 *        NULL terminated string in pValue. Therefore the pValue
 *        pointer can be re-cast as a (const char*) and the value
 *        string read from it. ulValueLen holds a length that
 *        is AT LEAST strlen((const char*) pValue) + 1. It may
 *        be more than that, so do not rely on ulValueLen being
 *        equal to strlen((const char*) pValue) + 1.
 */

typedef struct rm_property_struct
{
    char*  pName;
    UINT32 ulType;
    BYTE*  pValue;
    UINT32 ulValueLen;
} rm_property;

/*
 * rm_property Accessor Functions
 *
 * Users are strongly encouraged to use these accessor
 * functions to retrieve information from the 
 * rm_property struct, since the definition of rm_property
 * may change in the future.
 *
 * When retrieving the property name, the user should first
 * call rm_property_get_name_length() to see that the name
 * length is greater than 0. After that, the user can get
 * read-only access to the name by calling rm_property_get_name().
 */
const char* rm_property_get_name(rm_property* prop);
UINT32      rm_property_get_type(rm_property* prop);
UINT32      rm_property_get_value_uint32(rm_property* prop);
const char* rm_property_get_value_cstring(rm_property* prop);
UINT32      rm_property_get_value_buffer_length(rm_property* prop);
BYTE*       rm_property_get_value_buffer(rm_property* prop);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef RM_PROPERTY_H */
