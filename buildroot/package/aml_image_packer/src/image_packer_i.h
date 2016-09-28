/********************************************************************
	created:	11-10-2013   15:43
	file 	:	image_packer_i.h
	author:		Sam Wu
	
	purpose:	
*********************************************************************/
#ifndef __IMAGE_PACKER_I_H__
#define __IMAGE_PACKER_I_H__

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#ifdef BUILD_WITH_MINGW
#define stat64  _stati64
#define fseeko  fseeko64
#define ftello  ftello64
#endif
#else//win32
#include <io.h>
#define stat64  _stati64
#define fseeko  _fseeki64
#define ftello  _ftelli64
#define F_OK    0x00
#define W_OK    0x02
#define R_OK    0x04
#endif// #ifndef WIN32

#include "amlImage_if.h"
#include "crc32.h"
#include "sparse_format.h"

#ifdef BUILD_DLL
#include "dll.h"
#endif

#endif // __IMAGE_PACKER_I$_H__
