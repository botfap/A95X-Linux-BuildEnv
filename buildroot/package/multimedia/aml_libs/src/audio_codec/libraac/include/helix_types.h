/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: helix_types.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef HELIX_TYPES_H
#define HELIX_TYPES_H
//#include <core/dsp.h>
#include "string.h"
#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */
    
/*
 * INT8 Type definition
 *
 * INT8 is a signed 8-bit type
 */
#ifndef INT8

#if defined(_SYMBIAN)
typedef TInt8 INT8;
#else /* #if defined(_SYMBIAN) */
typedef char INT8;
#endif /* #if defined(_SYMBIAN) */

#endif /* #ifndef INT8 */

    
/*
 * CHAR - signed 8-bit value
 */
typedef INT8 CHAR;


/*
 * UINT8 Type definition
 *
 * UINT8 is an unsigned 8-bit type
 */
#ifndef UINT8

#if defined(_SYMBIAN)
typedef TUint8 UINT8;
#else /* #if defined(_SYMBIAN) */
typedef unsigned char UINT8;
#endif /* #if defined(_SYMBIAN) */

#endif /* #ifndef UINT8 */

/*
 * BYTE Type definition
 *
 * BYTE is another name for a UINT8
 */
typedef UINT8 BYTE;

/*
 * UCHAR Unsigned 8 bit value.
 *
 */
typedef UINT8 UCHAR;

/*
 * INT16 Type definition
 *
 * INT16 is a signed 16-bit type
 */
#ifndef INT16

#if defined(_SYMBIAN)
typedef TInt16  INT16;
#else /* #if defined(_SYMBIAN) */
typedef short int INT16;
#endif /* #if defined(_SYMBIAN) */

#endif /* #ifndef INT16 */

/*
 * UINT16 Type definition
 *
 * UINT16 is an unsigned 16-bit type
 */
#ifndef UINT16

#if defined(_SYMBIAN)
typedef TUint16 UINT16;
#else /* #if defined(_SYMBIAN) */
typedef unsigned short int UINT16;
#endif /* #if defined(_SYMBIAN) */

#endif /* #ifndef UINT16 */

/*
 * INT32 Type definition
 *
 * INT32 is a signed 32-bit type
 */
#ifndef INT32

#if defined(_SYMBIAN)
typedef TInt32  INT32;
#elif defined(_UNIX) && defined(_LONG_IS_64)
typedef int INT32;
#elif defined(_VXWORKS)
typedef int INT32;
#else
typedef int INT32;
#endif

#endif /* #ifndef INT32 */

/*
 * LONG32 Type definition
 *
 * LONG32 is another name for a INT32
 */
typedef INT32 LONG32;

/*
 * UINT32 Type definition
 *
 * UINT32 is an unsigned 32-bit type
 */
#ifndef UINT32

#if defined(_SYMBIAN)
typedef TUint32 UINT32;
#elif defined(_UNIX) && defined(_LONG_IS_64)
typedef unsigned int UINT32;
#elif defined(_VXWORKS)
typedef unsigned int UINT32;
#else
typedef unsigned int UINT32;
#endif

#endif /* #ifndef UINT32 */

/*
 * UFIXED32 Type definition
 *
 * UFIXED32 is another name for a UINT32
 */
typedef UINT32 UFIXED32;

/*
 * ULONG32 Type definition
 *
 * ULONG32 is another name for a UINT32
 */
typedef UINT32 ULONG32;

/*
 * HX_MOFTAG Type definition
 *
 * HX_MOFTAG is of type UINT32
 */
typedef UINT32 HX_MOFTAG;

/*
 * HXBOOL Type definition
 *
 * HXBOOL is a boolean type
 */
#ifndef HXBOOL

#if defined(_SYMBIAN)
typedef TBool HXBOOL;
#else /* #if defined(_SYMBIAN) */
typedef int HXBOOL;
#endif /* #if defined(_SYMBIAN) */

#endif /* #ifndef HXBOOL */

/*
 * BOOL Type definition
 *
 * BOOL is another name for a HXBOOL
 */
typedef HXBOOL BOOL;

/*
 * TRUE and FALSE definitions
 */
#ifdef TRUE
#undef TRUE
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 * HX_BITFIELD Type definition
 *
 * HX_BITFIELD is a bitfield type. It would
 * be used in conjunction with a field width
 * parameter like this:
 *
 * HX_BITFIELD foo:2;
 */
#ifndef HX_BITFIELD

typedef unsigned char HX_BITFIELD;

#endif

/*
 * HXFLOAT Type definition
 *
 * HXFLOAT is a single-precision floating-point type
 */
#ifndef HXFLOAT

typedef float HXFLOAT;

#endif /* #ifndef HXFLOAT */

/*
 * HXDOUBLE Type definition
 *
 * HXDOUBLE is a double-precision floating-point type
 */
#ifndef HXDOUBLE

typedef double HXDOUBLE;

#endif /* #ifndef HXDOUBLE */

/*
 * HXNULL definition
 */
#ifndef HXNULL
#define HXNULL ((void *)0)
#endif

/*
 * Helix DATE type.
 */

#define HX_YEAR_OFFSET 1900

typedef struct system_time
{
    UINT16 second;    /* 0-59 */
    UINT16 minute;    /* 0-59 */
    UINT16 hour;      /* 0-23 */
    UINT16 dayofweek; /* 0-6  (Sunday = 0) */
    UINT16 dayofmonth;/* 1-31 */
    UINT16 dayofyear; /* 1-366 (January 1 = 1) */
    UINT16 month;     /* 1-12 (January = 1) */
    UINT16 year;      /* year - 1900 or (year - HX_YEAR_OFFSET) */
    INT16 gmtDelta;   /* Greenwich Mean Time Delta in +/- hours */
} HX_DATETIME;
    
HX_DATETIME HX_GET_DATETIME(void);

#define HXEXPORT_PTR        *
#define HXEXPORT
#define ENTRYPOINT(func) func

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef HELIX_TYPES_H */
