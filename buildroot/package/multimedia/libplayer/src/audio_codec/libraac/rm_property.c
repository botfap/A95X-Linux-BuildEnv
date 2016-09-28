/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rm_property.c,v 1.1.1.1.2.1 2005/05/04 18:21:24 hubbe Exp $
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

#include "../include/helix_types.h"
#include "../include/rm_property.h"

const char* rm_property_get_name(rm_property* prop)
{
    const char* pRet = HXNULL;

    if (prop)
    {
        pRet = (const char*) prop->pName;
    }

    return pRet;
}

UINT32 rm_property_get_type(rm_property* prop)
{
    UINT32 ulRet = 0;

    if (prop)
    {
        ulRet = prop->ulType;
    }

    return ulRet;
}

UINT32 rm_property_get_value_uint32(rm_property* prop)
{
    UINT32 ulRet = 0;

    if (prop)
    {
        ulRet = (UINT32) prop->pValue;
    }

    return ulRet;
}

const char* rm_property_get_value_cstring(rm_property* prop)
{
    const char* pRet = HXNULL;

    if (prop)
    {
        pRet = (const char*) prop->pValue;
    }

    return pRet;
}

UINT32 rm_property_get_value_buffer_length(rm_property* prop)
{
    UINT32 ulRet = 0;

    if (prop)
    {
        ulRet = prop->ulValueLen;
    }

    return ulRet;
}

BYTE* rm_property_get_value_buffer(rm_property* prop)
{
    BYTE* pRet = HXNULL;

    if (prop)
    {
        pRet = prop->pValue;
    }

    return pRet;
}
