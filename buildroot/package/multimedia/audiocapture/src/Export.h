// Copyright 2012-2013 Yahoo! -- All rights reserved.
#ifndef __OEM_EXPORT_H__
#define __OEM_EXPORT_H__

// In order for your class and function to be visible, they
// must be prefixed with EXPORT.
#ifndef EXPORT
	#ifdef WIN32
		#define EXPORT __declspec(dllexport)
	#else
		#define EXPORT __attribute__((visibility("default")))
	#endif
#endif

#endif //#ifndef __OEM_EXPORT_H__
