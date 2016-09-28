/*
 * dll.h
 *
 *  Created on: 2013-6-3
 *      Author: binsheng.xu@amlogic.com
 */

#ifndef DLL_H_
#define DLL_H_

#ifdef BUILD_DLL

/* DLL export */

#define EXPORT __declspec(dllexport)

#else

/* EXE import */

#define EXPORT __declspec(dllimport)

#endif

#ifdef __cplusplus    // If used by C++ code, 
//extern "C" {          // we need to export the C interface
#endif

EXPORT ImageDecoderIf_t* AmlImage_Init();
EXPORT void AmlImage_Final(ImageDecoderIf_t* pDecoder);
EXPORT int image_pack(const char* cfg_file,const char *src_dir ,const char* target_file);
EXPORT int image_unpack(const char* imagefile,const char *outpath);


#ifdef __cplusplus
//}
#endif

#endif /* DLL_H_ */
