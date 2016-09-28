/*
 * \file        amlImage_if.h
 * \brief       Amlogic firmware image interface
 *
 * \version     1.0.0
 * \date        2013/5/21
 * \author      Sam.Wu <yihui.wu@Amlogic.com>
 *
 * Copyright (c) 2013 Amlogic Inc. All Rights Reserved.
 *
 */
#ifndef __AMLIMAGE_IF_H__
#define __AMLIMAGE_IF_H__

#include <sys/types.h>

#define AML_FIRMWARE_HEAD_SIZE      4096
#define IMAGE_MAGIC	                0x27b51956	 /* Image Magic Number		*/
#define VERSION                     0x0001
#define AML_IMG_ITEM_INFO_SZ        (128)//Don't never change to keep compatibility
#define AML_IMG_HEAD_INFO_SZ        (64)//Don't never change to keep compatibility

typedef unsigned long long  __u64;
typedef long long  __s64;
typedef unsigned int        __u32;
typedef int                 __s32;
typedef unsigned short      __u16;
typedef short               __s16;

#pragma pack(push,4)
typedef struct _AmlFirmwareItem_s
{
    __u32           itemId;
    __u32           fileType;           //image file type, sparse and normal
    __u64           curoffsetInItem;    //current offset in the item
    __u64           offsetInImage;      //item offset in the image
    __u64           itemSz;             //item size in the image
    char            itemMainType[32];   //item main type and sub type used to index the item
    char            itemSubType[32];    //item main type and sub type used to index the item
    __u32           verify;
    __u16           isBackUpItem;        //this item source file is the same as backItemId
    __u16           backUpItemId;        //if 'isBackUpItem', then this is the item id
    char            reserve[24];
}ItemInfo;
#pragma pack(pop)


#pragma pack(push,4)
typedef struct _AmlFirmwareImg_s
{
	__u32      crc;             //check sum of the image
    __u32      version;         //firmware version
    __u32      magic;           //magic No. to say it is Amlogic firmware image
    __u64      imageSz;         //total size of this image file
    __u32      itemAlginSize;   //align size for each item
    __u32      itemNum;         //item number in the image, each item a file
    char       reserve[36];
}AmlFirmwareImg_t;
#pragma pack(pop)



typedef void* HIMAGE;
typedef void* HIMAGEITEM;

#define IMAGE_ITEM_TYPE_NORMAL  0
#define IMAGE_ITEM_TYPE_SPARSE  0XFE


//open a Amlogic firmware image
//return value is a handle
typedef HIMAGE (*func_img_open)(const char* );

//check the image's crc32
//return 0 when check ok,otherwise return -1
typedef int (*func_img_check)(HIMAGE );

//close a Amlogic firmware image
typedef int (*func_img_close)(HIMAGE );

//open a item in the image
//@hImage: image handle;
//@mainType, @subType: main type and subtype to index the item, such as ["IMAGE", "SYSTEM"]
typedef HIMAGEITEM (*func_open_item)(HIMAGE , const char* , const char* );

//close a item
typedef int (*func_close_item)(HIMAGEITEM );

typedef __u64 (*func_get_item_size)(HIMAGEITEM );


//get image item type, current used type is normal or sparse
typedef int (*func_get_item_type)(HIMAGEITEM );

//get item count of specify main type
typedef int (*func_get_item_count)(HIMAGE ,const char* );

//read item data, like standard fread
typedef __u32 (*func_read_item_data)(HIMAGE , HIMAGEITEM , void* , const __u32 );

//relocate the read pointer to read the item data, like standard fseek
typedef int (*func_item_seek)(HIMAGE , HIMAGEITEM , __s64 , __u32 );

//get item main type and sub type
typedef int (*func_get_next_item)(HIMAGE ,unsigned int , char* , char* );


typedef struct filelist_t{
	char name[128];
	char main_type[32];
	char sub_type[32];
	int  verify;
	struct filelist_t *next;
}FileList;


typedef struct image_cfg_t{
	FileList *file_list;
}ImageCfg;


#pragma pack(push, 4)
typedef struct _ImageDecoder_if
{
	int                     gItemCount;
	ImageCfg                gImageCfg;
	AmlFirmwareImg_t*       AmlFirmwareImg ;
    func_img_open           img_open;      //open image
	func_img_check          img_check;     //check image
    func_img_close          img_close;     //close image
    func_open_item          open_item;     //open a item in the image, like c library fopen
    func_close_item         close_item;    //close a item, like c library fclose

    func_read_item_data     read_item;     //read item data, like c library fread
    func_item_seek          item_seek;     //seek the item read pointer, like c library fseek
	func_get_item_size      item_size;     //get item size in byte
	func_get_item_type      item_type;     //get item format, sparse, normal...
	func_get_item_count     item_count;    //get item count of specify main type
	func_get_next_item      get_next_item; //
}ImageDecoderIf_t;
#pragma pack(pop)

#endif//ifndef __AMLIMAGE_IF_H__

