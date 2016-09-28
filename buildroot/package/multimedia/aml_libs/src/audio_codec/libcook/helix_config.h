/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: helix_config.h,v 1.1.1.1.2.1 2005/05/04 18:20:57 hubbe Exp $
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

#ifndef HELIX_CONFIG_H
#define HELIX_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif  /* #ifdef __cplusplus */


/********************************************************************
 *
 * This file contains system wide configuration options.  Please look
 * through all the config options below to make sure the SDK is tuned
 * best for your system.
 *
 ********************************************************************
 */

    
/*
 * Endian'ness.
 *
 * This package supports both compile-time and run-time determination
 * of CPU byte ordering.  If ARCH_IS_BIG_ENDIAN is defined as 0, the
 * code will be compiled to run only on little-endian CPUs; if
 * ARCH_IS_BIG_ENDIAN is defined as non-zero, the code will be
 * compiled to run only on big-endian CPUs; if ARCH_IS_BIG_ENDIAN is
 * not defined, the code will be compiled to run on either big- or
 * little-endian CPUs, but will run slightly less efficiently on
 * either one than if ARCH_IS_BIG_ENDIAN is defined.
 *
 * If you run on a BIG engian box uncomment this:
 *
 *    #define ARCH_IS_BIG_ENDIAN 1
 * 
 * If you are on a LITTLE engian box uncomment this:
 *
 *    #define ARCH_IS_BIG_ENDIAN 0
 * 
 * Or you can just leave it undefined and have it determined
 * at runtime.
 *
 */







    
#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */

#endif /* #ifndef HELIX_CONFIG_H */
