/*
 * \file        amlImage.c
 * \brief       Amlogic firmware image interface
 *
 * \version     1.0.0
 * \date        2013/5/21
 * \author      Vinson.Xu <binsheng.xu@amlogic.com>
 *
 * Copyright (c) 2013 Amlogic Inc. All Rights Reserved.
 *
 */
#include "image_packer_i.h"

extern "C" {
int gen_sha1sum_verify(const char* srFile, char* verifyData);
}

#define COMPILE_TYPE_CHK(expr, t)       typedef char t[(expr) ? 1 : -1]

COMPILE_TYPE_CHK(AML_IMG_ITEM_INFO_SZ == sizeof(ItemInfo), aa);
COMPILE_TYPE_CHK(AML_IMG_HEAD_INFO_SZ == sizeof(AmlFirmwareImg_t), bb);

#define RW_MAX_SIZE   65536
#define RW_HEAD_SIZE  8192
#define ITEM_ALGIN    4

#define debugP(...) //printf("dbg:"),printf(__VA_ARGS__)
#define errorP(...) printf("ERR:"),printf(__VA_ARGS__)

ImageDecoderIf_t  *ImageDecoder;

static HIMAGE image_open(const char* imagePath)
{
	FILE *fp = NULL;
	fp = fopen(imagePath,"rb");
	if(fp == NULL){
		fprintf(stderr,"image open error! [%s]\n",strerror(errno));
	}
	return fp;
}

static int image_check(HIMAGE hImage)
{
	FILE* fp = (FILE*)hImage;
	unsigned int crc32 = 0;
	
	if(ImageDecoder->AmlFirmwareImg->crc==0){
		fseeko(fp,0,SEEK_SET);
		memset(ImageDecoder->AmlFirmwareImg,0,sizeof(AmlFirmwareImg_t));
		if(fread(ImageDecoder->AmlFirmwareImg,sizeof(AmlFirmwareImg_t),1, fp) != 1) {
			fprintf(stderr,"cannot read %u bytes  [%s]\n", (unsigned)sizeof(AmlFirmwareImg_t),strerror(errno));
			return -1;
		}
	}

	if(ImageDecoder->AmlFirmwareImg->magic != IMAGE_MAGIC){
    	fprintf(stderr,"the magic number is not match \n");
    	return -1;
	}

	crc32 = calc_img_crc(fp, ImageDecoder->AmlFirmwareImg->itemAlginSize);
	printf("crc32==0x%x\n",crc32);
	printf("ImageDecoder->AmlFirmwareImg->crc==0x%x\n",ImageDecoder->AmlFirmwareImg->crc);
	if(crc32!=ImageDecoder->AmlFirmwareImg->crc){
    	fprintf(stderr,"crc check fail !!! \n");
    	return -1;
	}
	return 0;
}

static int image_close(HIMAGE hImage)
{
	if(hImage)
		return fclose((FILE*)hImage);
	return -1;
}

static HIMAGEITEM image_open_item(HIMAGE hImage, const char* mainType, const char* subType)
{
	FILE* fp = (FILE*)hImage;
	HIMAGEITEM item = NULL;
	unsigned int i = 0;
	if(ImageDecoder->AmlFirmwareImg->crc==0){
		memset(ImageDecoder->AmlFirmwareImg,0,sizeof(AmlFirmwareImg_t));
		fseeko(fp,0,SEEK_SET);

		if(fread(ImageDecoder->AmlFirmwareImg,sizeof(AmlFirmwareImg_t),1,fp)!= 1) {
			fprintf(stderr,"Read error[%s] \n", strerror(errno));
			return NULL;
		}
	}

	if(ImageDecoder->AmlFirmwareImg->magic != IMAGE_MAGIC){
    	fprintf(stderr,"the magic number is not match \n");
    	return NULL;
	}

	ItemInfo *AmlItem = NULL;
    AmlItem = (ItemInfo*)malloc(sizeof(ItemInfo)*ImageDecoder->AmlFirmwareImg->itemNum);
	if(AmlItem == NULL){
		fprintf(stderr,"malloc %u bytes memmory failed \n", (unsigned)sizeof(ItemInfo));
		return NULL;
	}

	fseeko(fp,sizeof(AmlFirmwareImg_t),SEEK_SET);
	memset(AmlItem,0,sizeof(ItemInfo)*ImageDecoder->AmlFirmwareImg->itemNum);
	unsigned int read_len = fread(AmlItem,sizeof(ItemInfo), ImageDecoder->AmlFirmwareImg->itemNum,fp);

	if( read_len!= ImageDecoder->AmlFirmwareImg->itemNum) {
		fprintf(stderr,"cannot read %u bytes  [%s] \n", ImageDecoder->AmlFirmwareImg->itemNum,strerror(errno));
		return NULL;
	}	
	
	for(;i<ImageDecoder->AmlFirmwareImg->itemNum;i++){
	
		if(!strncmp((AmlItem+i)->itemMainType,mainType,32)
				&&!strncmp((AmlItem+i)->itemSubType,subType,32)){
			item = malloc(sizeof(ItemInfo));
			memcpy((void*)item,(void*)(AmlItem+i),sizeof(ItemInfo));
			free(AmlItem);
			AmlItem = NULL;
			break;
		}	
	}
	
	if(AmlItem){
		free(AmlItem);
	}
	/*
	do{
		memset(AmlItem,0,sizeof(ItemInfo));
		if(fread(AmlItem,1, sizeof(ItemInfo),fp) != sizeof(ItemInfo)) {
			fprintf(stderr,"cannot read %ud bytes", sizeof(ItemInfo));
			return NULL;
		}

		if(!strncmp(AmlItem->itemMainType,mainType,32)
				&&!strncmp(AmlItem->itemSubType,subType,32)){
			item = (HIMAGEITEM)AmlItem;
			break;
		}

		count ++;
		if(count == ImageDecoder->AmlFirmwareImg->itemNum){
			fprintf(stderr,"Item is not found!");
			break;
		}
	}while(1);
	*/

	return item;
}


static int image_close_item(HIMAGEITEM hItem){
	if(hItem){
		free(hItem);
		hItem = NULL;
	}

	return 0;
}

//read item data, like standard fread
static __u32 image_read_item_data(HIMAGE hImage, HIMAGEITEM hItem, void* buff, const __u32 readSz){
	if(!hImage||!hItem){
		fprintf(stderr,"invalid param!");
	}
	FILE* fp = (FILE*)hImage;
	size_t  readlen = 0 ;
	  
	//static __u32 item_offset = 0;
	//static __u32 itemId_save = 0;
	ItemInfo *item = (ItemInfo*)hItem;

	//if(itemId_save!=item->itemId){
	//	item_offset = 0;
	//}

	if(item->curoffsetInItem >= item->itemSz){
		//item_offset = 0;
		//itemId_save = item->itemId;
		return 0;
	}

	fseeko(fp,item->offsetInImage+item->curoffsetInItem,SEEK_SET);
	if(readSz+item->curoffsetInItem >= item->itemSz){
		readlen = item->itemSz-item->curoffsetInItem;
	}else{
		readlen = readSz;
	}
	if(fread(buff,readlen,1,fp)!=1){
		fprintf(stderr,"read item error! [%s] \n",strerror(errno));
		return 0;
	}
	item->curoffsetInItem += readlen;
	//itemId_save = item->itemId;
	return readlen;
}

//relocate the read pointer to read the item data, like standard fseeko
static int image_seek_item(HIMAGE hImage, HIMAGEITEM hItem, __s64 offset, __u32 fromwhere)
{
	if(!hImage||!hItem){
		fprintf(stderr,"invalid param!");
	}
	int ret = 0;
	FILE* fp = (FILE*)hImage;
	ItemInfo *item = (ItemInfo*)hItem;
	fseeko(fp,item->offsetInImage,SEEK_SET);
	switch(fromwhere){
	case SEEK_SET:
		ret = fseeko(fp,offset,SEEK_CUR);
		item->curoffsetInItem = offset;
		break;
	case SEEK_CUR:
		ret = fseeko(fp,item->curoffsetInItem+offset,SEEK_CUR);
		item->curoffsetInItem += offset;
		break;
	case SEEK_END:
		ret = fseeko(fp,item->itemSz+offset,SEEK_CUR);
		item->curoffsetInItem = item->itemSz+offset;
		break;
	default:
		fprintf(stderr,"invalid param for fromwhere!");
		ret = -1;
		break;
	}

	return ret;
}

static __u64 image_get_item_size(HIMAGEITEM hItem)
{
	if(!hItem){
		fprintf(stderr,"invalid param!");
        return 0;
	}

	ItemInfo *item = (ItemInfo*)hItem;
	return item->itemSz;
}

//is the item source is duplicated with other item
//i.e, item data is empty
//return value: 0/1 is ok, <0 is failed
static int is_item_backup_for_other_item(HIMAGEITEM hItem, int* backItemId)
{
	ItemInfo *item = (ItemInfo*)hItem;
    if(!hItem){
        errorP("NULL item handle\n");
        return -1;
    }
    *backItemId = item->backUpItemId;
    return item->isBackUpItem;
}

static int image_get_item_type(HIMAGEITEM hItem)
{
	if(!hItem){
		fprintf(stderr,"invalid param!");
	}

	ItemInfo *item = (ItemInfo*)hItem;
	return item->fileType;
}


static int image_get_item_count(HIMAGE hImage,const char* mainType)
{
	int count = 0;
	unsigned int total = 0;
	FILE* fp = (FILE*)hImage;
	if(ImageDecoder->AmlFirmwareImg->crc==0){
		memset(ImageDecoder->AmlFirmwareImg,0,sizeof(AmlFirmwareImg_t));
		fseeko(fp,0,SEEK_SET);
		if(fread(ImageDecoder->AmlFirmwareImg,sizeof(AmlFirmwareImg_t),1, fp) != 1) {
			fprintf(stderr,"image_get_item_count : Read Err [%s] \n", strerror(errno));
			return -1;
		}
	}

	if(mainType == NULL){
		return ImageDecoder->AmlFirmwareImg->itemNum;
	}

	ItemInfo *AmlItem = NULL;
	AmlItem = (ItemInfo*)malloc(sizeof(ItemInfo));
	if(AmlItem == NULL){
		fprintf(stderr,"not enough memory \n");
		return -1;
	}
	fseeko(fp,sizeof(AmlFirmwareImg_t),SEEK_SET);
	do{
		memset(AmlItem,0,sizeof(ItemInfo));
		if(fread(AmlItem,1, sizeof(ItemInfo),fp) != sizeof(ItemInfo)) {
			fprintf(stderr,"cannot read %u bytes", (unsigned)sizeof(ItemInfo));
			return -1;
		}

		if(!strncmp(AmlItem->itemMainType,mainType,strlen(mainType))){
			count ++;
		}

		//fseeko(fp,AmlItem->itemSz,SEEK_CUR);
		total ++;
		if(total == ImageDecoder->AmlFirmwareImg->itemNum){
			break;
		}
	}while(1);

	free(AmlItem);
	return count;
}

static int image_get_next_item(HIMAGE hImage,unsigned int iItem, char* maintype, char* subtype)
{
	FILE* fp = (FILE*)hImage;
	unsigned int nItem = 0;
	if(ImageDecoder->AmlFirmwareImg->crc==0){
		memset(ImageDecoder->AmlFirmwareImg,0,sizeof(AmlFirmwareImg_t));
		fseeko(fp,0,SEEK_SET);
		if(fread(ImageDecoder->AmlFirmwareImg,1, sizeof(AmlFirmwareImg_t),fp) != sizeof(AmlFirmwareImg_t)) {
			fprintf(stderr,"cannot read %u bytes", (unsigned)sizeof(AmlFirmwareImg_t));
			return -1;
		}
	}

	if(iItem >= ImageDecoder->AmlFirmwareImg->itemNum){
        errorP("Item index %u > max %u\n", iItem, ImageDecoder->AmlFirmwareImg->itemNum - 1);
		return -1;
    }
		
	fseeko(fp,sizeof(AmlFirmwareImg_t)+iItem*sizeof(ItemInfo),SEEK_SET);	
	
	ItemInfo AmlItem;
	memset(&AmlItem,0,sizeof(ItemInfo));
	if(fread(&AmlItem,sizeof(ItemInfo), 1,fp) != 1) {
		fprintf(stderr,"image_get_next_item:Read Err [%s]", strerror(errno));
		return -1;
	}

	strncpy(maintype,AmlItem.itemMainType,32);
	strncpy(subtype,AmlItem.itemSubType,32);
    /*debugP("[%s, %s] verify = %d\n", maintype, subtype, AmlItem.verify);*/
	return 0;
}

//#define TAG_WORKDIR     "work_dir"
//#define TAG_ARCHIVE     "archive_file"
#define TAG_NORMALLIST  "[LIST_NORMAL]"
#define TAG_VERIFYLIST  "[LIST_VERIFY]"

#define TAG_LINE    "\r\n"
#define TAG_LINE_1  "\n"

#define FILE_TAG        "file="
#define MTYPE_TAG       "main_type="
#define STYPE_TAG       "sub_type="

static int  image_cfg_parse(char *data,int length)
{
	int ret = 0;

	char *key_normal, *key_verify;
	char *proc , *proc_end;
	FileList *list = NULL ;
	int verify = 0;

	key_normal = strstr(data,TAG_NORMALLIST);
	if(key_normal==NULL){
		printf("image.cfg parse err: miss [LIST_NORMAL] node \n");
	}

	key_verify = strstr(data,TAG_VERIFYLIST);
	if(key_verify==NULL){
		printf("image.cfg parse: no [LIST_VERIFY] node \n");
	}

	if(!key_normal&&!key_verify){
		return -1;
	}
	const char* tag_line = TAG_LINE;
	char* token = strtok(data, TAG_LINE);
	if(token==NULL){
		token = strtok(data, TAG_LINE_1);
		tag_line = TAG_LINE_1;
	}
	for( ;token != NULL; token = strtok(NULL,tag_line))
	{
		if(*token == '#'){
			debugP("skip line: %s \n",token);
			continue;
		}
		proc = strstr(token,FILE_TAG);
		if(proc == NULL){
			continue;
		}

		if(key_verify > key_normal){
			if(proc > key_normal && proc < key_verify){
				verify = 0;
			}else if(proc > key_verify ){
				verify = 1;
				ImageDecoder->gItemCount ++;
			}else{
				printf("invalid list\n");
				continue;
			}
		}else{
			if(proc > key_normal ){
				verify = 0;
			}else if(proc > key_verify && proc < key_normal){
				verify = 1;
				ImageDecoder->gItemCount ++;
			}else{
				printf("invalid list\n");
				continue;
			}
		}
		if(list == NULL){
			list = (FileList*)malloc(sizeof(FileList));
			ImageDecoder->gImageCfg.file_list = list;

		}else{
			list->next = (FileList*)malloc(sizeof(FileList));
			list = list->next;
		}

		memset(list,0,sizeof(FileList));
		list->verify = verify;

		proc = strchr(proc+strlen(FILE_TAG),'\"');
		proc_end = strchr(proc+1,'\"');
		strncpy(list->name,proc+1,proc_end-proc-1);
		//printf("file: %s  ",list->name);

		proc = strstr(token,MTYPE_TAG);
		if(proc == NULL){
			continue;
		}

		proc = strchr(proc+strlen(MTYPE_TAG),'\"');
		proc_end = strchr(proc+1,'\"');
		strncpy(list->main_type,proc+1,proc_end-proc-1);
		//printf("main_type: %s ",list->main_type);

		proc = strstr(token,STYPE_TAG);
		if(proc == NULL){
			continue;
		}

		proc = strchr(proc+strlen(STYPE_TAG),'\"');
		proc_end = strchr(proc+1,'\"');
		strncpy(list->sub_type,proc+1,proc_end-proc-1);
		debugP("sub_type: %s \n",list->sub_type);
		ImageDecoder->gItemCount ++;
	}

	list = ImageDecoder->gImageCfg.file_list;
	FileList *plist = ImageDecoder->gImageCfg.file_list;
	while(ret==0&&list){
		for(plist = ImageDecoder->gImageCfg.file_list;plist;plist=plist->next){

			if(plist!=list&&(!strcmp(list->main_type,plist->main_type))&&(!strcmp(list->sub_type,plist->sub_type))){
				fprintf(stderr,"Error: config file is illegal! \nmain_type:[%s] sub_type:[%s] is reduplicate \n",list->main_type,list->sub_type);
				ret = -1;
				break;
			}
		}
		list = list->next;
	}
	if(ret < 0){
		list = ImageDecoder->gImageCfg.file_list;
		while(list){
			plist = list->next;
			free(list);
			list = plist;
		}
	}
	return ret;
}

static int image_check_files(const char* src_dir)
{
	char path[256]={0};
	FileList *list = ImageDecoder->gImageCfg.file_list;
	do{
		memset(path,0,256);
		strcpy(path,src_dir);
		strcat(path,list->name);
		debugP("path: %s\n", path);
		//if(access(path,F_OK|R_OK) < 0){
		if(access(path, 0) < 0){
			printf("Err:%s is not found! \n",path);
			return -1;
		}
		list = list->next;
	}while(list!=NULL);

	return 0;
}

//return the item id, >= 0 if OK
/*
 * duplicated item like this, which source is the same file 'boot.PARTITION' but sub_type.main_type are different
 *file="boot.PARTITION"		main_type="PARTITION"		sub_type="boot"
 *file="boot.PARTITION"		main_type="PARTITION"		sub_type="bootbak"
 */
static const ItemInfo* previous_duplicated_item_id(const FileList* headFileList, const FileList* const curFileList, 
        const ItemInfo* headItemInfo, const ItemInfo* curItemInfo)
{
    int itemId = -1;
    int i = 0;

    for(;headFileList != curFileList && headFileList->next; headFileList = headFileList->next)
    {
        int rc = strcmp(curFileList->name, headFileList->name);
        if(!rc)//the file of headFileList is duplicated
        {
            for(; headItemInfo < curItemInfo; headItemInfo++)
            {
                if(!strcmp(headItemInfo->itemMainType, headFileList->main_type) 
                        && !strcmp(headItemInfo->itemSubType, headFileList->sub_type))
                {
                    return headItemInfo;
                }
            }
            errorP("find duplicated file(%s) but previous item not found\n", curFileList->name);
            return NULL;
        }
    }

    return NULL;
}

#ifdef BUILD_DLL
EXPORT
#endif
int image_pack(const char* cfg_file, const char* src_dir ,const char* target_file)
{
	struct stat64 s;
	FILE* fp_read = NULL;
	FILE* fp_write  = NULL;
	char *tmp;
	int count = 0;
    size_t write_bytes = 0;
    ItemInfo* totalItemInfo = NULL;
	char *buff = NULL;
	int ret = 0;
    ItemInfo*  AmlItem = NULL;

	if(!cfg_file||!src_dir||!target_file){
		fprintf(stderr,"invalid param! \n");
		return -1;
	}

	if(stat64(cfg_file, &s)){
		fprintf(stderr,"could not stat '%s'\n", cfg_file);
		return -1;
	}

	fp_read = fopen(cfg_file, "rb");
    if (fp_read == NULL){
    	fprintf(stderr,"failed to open canned file \n");
    	return -1;
    }

    tmp = (char*) malloc(s.st_size);
    if(tmp == NULL) {
    	fprintf(stderr,"allocate memccpy failed  at %s  %d \n", __FILE__,__LINE__);
    	free(tmp);
    	return -1;
    }

    if(fread(tmp,1, s.st_size,fp_read) != s.st_size) {
    	fprintf(stderr,"file read error [%s] \n", strerror(errno));
    	fclose(fp_read);
    	free(tmp);
    	return -1;
    }
    fclose(fp_read);

    if(image_cfg_parse(tmp,s.st_size) < 0){
    	fprintf(stderr,"image_cfg_parse error! \n");
    	free(tmp);
    	return -1;
    }
    free(tmp); tmp = NULL;

    if(image_check_files(src_dir)<0){
    	fprintf(stderr,"image check file error! \n");
    	return -1;
    }

	fp_write = fopen(target_file, "wb+");
	if (fp_write == NULL){
		fprintf(stderr,"failed to create target file! \n");
		return -1;
	}
    AmlFirmwareImg_t aAmlImage;
	AmlFirmwareImg_t  *FirmwareImg = &aAmlImage;
	memset(FirmwareImg,0,sizeof(AmlFirmwareImg_t));

    if(fwrite(FirmwareImg,1, sizeof(AmlFirmwareImg_t),fp_write) != sizeof(AmlFirmwareImg_t)) {
    	fprintf(stderr,"cannot write %u bytes\n", (unsigned)sizeof(AmlFirmwareImg_t));
    	fclose(fp_write);
    	return -1;
    }

    const int TotalItemCnt = ImageDecoder->gItemCount;
    debugP("TotalItemCnt %d\n", TotalItemCnt);
    totalItemInfo = new ItemInfo[TotalItemCnt];

	memset(totalItemInfo,0,sizeof(ItemInfo) * TotalItemCnt);
    write_bytes = fwrite(totalItemInfo,1, sizeof(ItemInfo) * TotalItemCnt, fp_write);
	if(write_bytes != sizeof(ItemInfo)* TotalItemCnt){
        fprintf(stderr,"cannot write %d bytes \n", (int)sizeof(ItemInfo));
        ret = __LINE__; goto _exit;
    }

	buff = new char[RW_MAX_SIZE];
	if(buff==NULL){
		fprintf(stderr,"allocate memccpy failed  at %s  %d \n", __FILE__,__LINE__);
        ret = __LINE__; goto _exit;
	}

    //For each list:
    //1, create item info;  2,pack the item data; 3, crate verify item info; 4, pack verify data
    AmlItem = totalItemInfo;
    for(const FileList *list = ImageDecoder->gImageCfg.file_list;
            list; list = list->next, AmlItem++, count++)
	{
        struct stat64 info;
        char path[256]={0};

		memset(path,0,256);
		strcpy(path,src_dir);
		strcat(path,list->name);
		printf("pack [%-12s, %16s] from (%s)\n", list->main_type, list->sub_type, path);

        fp_read = fopen(path,"rb");
        if(fp_read == NULL){
            fprintf(stderr,"failed to open source file : %s \n",path);
            ret = -1;
            break;
        }

        if(stat64(path,&info)<0){
			fprintf(stderr,"stat %s failed!\n",path);
			ret =  -1;
			break;
		}
		AmlItem->itemSz = info.st_size;
		AmlItem->itemId = count;
		AmlItem->verify = 0;//1 if a verify item
		strcpy(AmlItem->itemMainType,list->main_type);
		strcpy(AmlItem->itemSubType,list->sub_type);

        const ItemInfo* foundSamFileItemInfo = previous_duplicated_item_id(ImageDecoder->gImageCfg.file_list, list, totalItemInfo, AmlItem);
        if(foundSamFileItemInfo)//this item is source from a duplicated file item
        {
            AmlItem->offsetInImage  = foundSamFileItemInfo->offsetInImage;
            AmlItem->fileType       = foundSamFileItemInfo->fileType;
            AmlItem->isBackUpItem   = 1;
            AmlItem->backUpItemId   = foundSamFileItemInfo->itemId;
        }
        else
        {
            AmlItem->offsetInImage = ftello(fp_write);
            if(AmlItem->offsetInImage % ITEM_ALGIN){
                AmlItem->offsetInImage += ITEM_ALGIN - AmlItem->offsetInImage % ITEM_ALGIN;
            }

            fseeko(fp_read,0,SEEK_SET);
            write_bytes = fread(buff,1,RW_MAX_SIZE,fp_read);
            if(optimus_simg_probe(buff,RW_HEAD_SIZE) == 0){
                AmlItem->fileType = IMAGE_ITEM_TYPE_NORMAL;
            }else{
                AmlItem->fileType = IMAGE_ITEM_TYPE_SPARSE;
            }

            //Following create the item data
            //
            fseeko(fp_write,AmlItem->offsetInImage,SEEK_SET);
            fseeko(fp_read,0,SEEK_SET);
            while((write_bytes = fread(buff,1,RW_MAX_SIZE,fp_read))>0)
            {
                if(fwrite(buff,1,write_bytes,fp_write)!=write_bytes){
                    fprintf(stderr,"write to image file fail! [%s] \n",strerror(errno));
                    ret = -1;
                    break;
                }
            }
            fclose(fp_read);
            if(ret < 0)
                break;
        }

		if(list->verify)
		{
            ++count;
            ++AmlItem;

			AmlItem->itemSz = 48;
			AmlItem->itemId = count;
			strcpy(AmlItem->itemMainType,"VERIFY");
			strcpy(AmlItem->itemSubType,list->sub_type);
			AmlItem->fileType = IMAGE_ITEM_TYPE_NORMAL;
		    AmlItem->verify = 1;

            if(foundSamFileItemInfo)//the source file is already packed 
            {
                const ItemInfo* verfiyItem4_foundItem = foundSamFileItemInfo + 1;
                //test if the next item following foundSamFileItemInfo is verify item
                if(!strcmp(foundSamFileItemInfo->itemSubType, verfiyItem4_foundItem->itemSubType) 
                        && !strcmp(verfiyItem4_foundItem->itemMainType, AmlItem->itemMainType))//"VERIFY"
                {
                    AmlItem->offsetInImage = verfiyItem4_foundItem->offsetInImage;
                    AmlItem->isBackUpItem   = 1;
                    AmlItem->backUpItemId   = verfiyItem4_foundItem->itemId;
                    continue;//skip followings
                }
            }

            AmlItem->offsetInImage = ftello(fp_write);
            if(AmlItem->offsetInImage % ITEM_ALGIN){
                AmlItem->offsetInImage += ITEM_ALGIN - AmlItem->offsetInImage % ITEM_ALGIN;
            }

		    memset(buff,0,48);
		    gen_sha1sum_verify(path, buff);

		    fseeko(fp_write,AmlItem->offsetInImage,SEEK_SET);
			if(fwrite(buff,1,48,fp_write)!=48){
				fprintf(stderr,"write verify data to image file fail! [%s] \n",strerror(errno));
				ret = -1;
				break;
			}
		}
	}
	free(buff); buff = 0;

	FirmwareImg->magic = IMAGE_MAGIC;
	FirmwareImg->version = VERSION;

	FirmwareImg->itemNum = count;
	fseeko(fp_write, 0L, SEEK_END);
	FirmwareImg->imageSz = ftello(fp_write);
	FirmwareImg->itemAlginSize = 4;
	fseeko(fp_write,0L,SEEK_SET);
    if(fwrite(FirmwareImg,1, sizeof(AmlFirmwareImg_t),fp_write) != sizeof(AmlFirmwareImg_t)) {
    	fprintf(stderr,"cannot write %u bytes [%s] [0x%p]\n", 
                (unsigned)sizeof(AmlFirmwareImg_t), strerror(errno), fp_write);
    	ret = -1;
    }

    write_bytes = fwrite(totalItemInfo, 1, sizeof(ItemInfo) * TotalItemCnt, fp_write);
    if(write_bytes != sizeof(ItemInfo) * TotalItemCnt){
        errorP("fail to update item info\n");
        ret = __LINE__; goto _exit;
    }

	FirmwareImg->crc = calc_img_crc(fp_write,FirmwareImg->itemAlginSize);
	fseeko(fp_write,0L,SEEK_SET);
    if(fwrite(&(FirmwareImg->crc),1,FirmwareImg->itemAlginSize,fp_write) != FirmwareImg->itemAlginSize) {
    	fprintf(stderr,"cannot write crc32 [0x%x] [%s] \n",FirmwareImg->crc ,strerror(errno));
    	ret = -1;
    }
	printf("Size=0x%llxB[%lluM], crc=0x%x, Install image[%s] OK\n",
            FirmwareImg->imageSz, (FirmwareImg->imageSz>>20), FirmwareImg->crc, target_file);

_exit:
    if(fp_write)fclose(fp_write), fp_write = NULL;
    if(totalItemInfo)delete[] totalItemInfo, totalItemInfo = NULL;
    if(buff) delete[] buff, buff = NULL;

    return ret;
}

const char* const IMG_CFG_LINE = "file=\"%s.%s\"\t\tmain_type=\"%s\"\t\tsub_type=\"%s\"\r\n";

#ifdef BUILD_DLL
EXPORT
#endif

int image_unpack(const char* imagefile,const char *outpath)
{
	int ret = 0;
	unsigned int itemCountTotal = 0;
	unsigned int nItem = 0;
	unsigned int write_bytes = 0;
	char main_type[32] = {0};
	char sub_type[32] = {0};
	char outfile[512] = {0};
	char cfgfile[128] = {0};
	char buff[256] = {0};
	
	#ifdef BUILD_DLL
	if(ImageDecoder->AmlFirmwareImg == NULL){
		ImageDecoder->AmlFirmwareImg = (AmlFirmwareImg_t*)malloc(sizeof(AmlFirmwareImg_t));
		if(ImageDecoder->AmlFirmwareImg == NULL){
			fprintf(stderr,"not enough memory\n");
			return -1;
		}
	}	
	#endif
	
	if(outpath){
		strcpy(outfile,outpath);
		strcat(outfile,"\\");
		strcpy(cfgfile,outfile);
	}
	strcat(cfgfile,"image.cfg");
    
	HIMAGE hImage = image_open(imagefile);
	if(image_check(hImage) < 0){
		fprintf(stderr,"the image check fail!!\n");
		return -1;
	}else{
		printf("the image check ok!\n");
	}

    itemCountTotal = image_get_item_count(hImage,NULL);
    const unsigned itemCountVerify = image_get_item_count(hImage, "VERIFY");
    const unsigned itemCountNormal = itemCountTotal - itemCountVerify * 2;
    debugP("item cnt:total[%u], normal[%u], verify[%u]\n", itemCountTotal, itemCountNormal, itemCountVerify);
	
    char *unPackBuf = (char*)malloc(RW_MAX_SIZE);
    if(unPackBuf==NULL){
        fprintf(stderr,"allocate memccpy failed  at %s  %d \n", __FILE__,__LINE__);
        return __LINE__;
    }

    FILE *fp_cfg = fopen(cfgfile,"wb+");
    if(fp_cfg==NULL){
        fprintf(stderr,"create image.cfg failed ! [%s] \n",strerror(errno));
        return __LINE__;
    }
    fwrite(TAG_NORMALLIST,1,strlen(TAG_NORMALLIST), fp_cfg);
    fwrite("\r\n", 1, 2, fp_cfg);

    for(nItem = 0; nItem < itemCountTotal; ++nItem)
    {
    	if(image_get_next_item(hImage,nItem,main_type,sub_type) < 0){
    		ret = -1;
    		break;
    	}
        if(!strcmp("VERIFY", main_type))
        {
            continue;//skip verify item
        }
        if(nItem == itemCountNormal)//[List_normal]ends, [List_verify] starts
        {
            fwrite("\r\n", 1, 2, fp_cfg);
            fwrite(TAG_VERIFYLIST,1,strlen(TAG_VERIFYLIST), fp_cfg);
            fwrite("\r\n", 1, 2, fp_cfg);
        }

    	memset(outfile,0,64);
    	if(outpath){
    		strcpy(outfile,outpath);
			strcat(outfile,"/");
    	}

    	strcat(outfile,sub_type);
    	strcat(outfile,".");
    	strcat(outfile,main_type);
    	debugP("out file: %s  \n",outfile);//sub_type.main_type

	    HIMAGEITEM hItem = image_open_item(hImage,main_type,sub_type);
	    if(hItem == NULL){
	    	fprintf(stderr,"open item[%s, %s] failed!\n", main_type, sub_type);
	    	ret = -1;
	    	break;
	    }

        int backUpItemId = 0;
        int itemIsBacked = is_item_backup_for_other_item(hItem, &backUpItemId);
        if(itemIsBacked < 0){
            errorP("Fail to in test is_item_backup_for_other_item\n");
            ret = __LINE__; break;
        }
        if(itemIsBacked)//item is back item
        {
            char* CfgLine = (char*)unPackBuf;
            char srcBackItemMainType[32];
            char srcBackItemSubType[32];

            if(image_get_next_item(hImage, backUpItemId, srcBackItemMainType, srcBackItemSubType)){
                errorP("Fail to get the backui item head\n");
                ret = __LINE__; break;
            }
            sprintf(CfgLine, IMG_CFG_LINE, srcBackItemSubType, srcBackItemMainType, main_type, sub_type);
            fwrite(CfgLine,1,strlen(CfgLine), fp_cfg);
            continue;
        }

    	FILE *fp_out = fopen(outfile,"wb+");
	    if(fp_out == NULL){
	    	fprintf(stderr,"failed to create out file : %s \n",outfile);
			ret = -1;
			break;
	    }

	    while((write_bytes = image_read_item_data(hImage, hItem, unPackBuf, RW_MAX_SIZE))>0)
        {
	    	if(fwrite(unPackBuf,1,write_bytes,fp_out)!=write_bytes){
	    		fprintf(stderr,"write to image file fail! [%s] \n",strerror(errno));
	    		ret = -1;
	    		break;
	    	}
	    }
	    fclose(fp_out);
	    image_close_item(hItem);

        char* CfgLine = (char*)unPackBuf;
        sprintf(CfgLine, IMG_CFG_LINE, sub_type, main_type, main_type, sub_type);
        fwrite(CfgLine,1,strlen(CfgLine), fp_cfg);
    }
    free(unPackBuf);
    fclose(fp_cfg); fp_cfg = NULL;

	#ifdef BUILD_DLL
	if(ImageDecoder->AmlFirmwareImg){
		free(ImageDecoder->AmlFirmwareImg);
		ImageDecoder->AmlFirmwareImg = NULL;
	}	
	#endif
	return ret;
}

#ifdef BUILD_DLL
EXPORT
#endif
ImageDecoderIf_t* AmlImage_Init(){

	ImageDecoder = (ImageDecoderIf_t*)malloc(sizeof(ImageDecoderIf_t));
	memset(ImageDecoder,0,sizeof(ImageDecoderIf_t));
	ImageDecoder->AmlFirmwareImg = (AmlFirmwareImg_t*)malloc(sizeof(AmlFirmwareImg_t));
	if(ImageDecoder->AmlFirmwareImg == NULL){
		fprintf(stderr,"not enough memory\n");
		return NULL;
	}
	memset(ImageDecoder->AmlFirmwareImg,0,sizeof(AmlFirmwareImg_t));
	
	ImageDecoder->img_open = image_open;
	ImageDecoder->img_check = image_check;
	ImageDecoder->img_close = image_close;
	ImageDecoder->open_item = image_open_item;
	ImageDecoder->close_item = image_close_item;
	ImageDecoder->read_item = image_read_item_data;
	ImageDecoder->item_seek = image_seek_item;
	ImageDecoder->item_size = image_get_item_size;
	ImageDecoder->item_type = image_get_item_type;
	ImageDecoder->item_count = image_get_item_count;
	ImageDecoder->get_next_item = image_get_next_item;
	return ImageDecoder;
}

#ifdef BUILD_DLL
EXPORT
#endif
void AmlImage_Final(ImageDecoderIf_t* pDecoder){


	if(pDecoder){
		if(pDecoder->AmlFirmwareImg){
			free(pDecoder->AmlFirmwareImg);
			pDecoder->AmlFirmwareImg = NULL;
		}
		free(pDecoder);
	};
}


#ifndef BUILD_DLL

static const char * const usage = "%s usage:\n\
\n\
  # pack files in directory to the archive\n\
  %s -r <config file> <src dir> <target file> \n\
  # unpack files in directory to the archive\n\
  %s -d <archive> [out dir] \n\
  # check the image    \n\
  %s -c <archive>  \n";

int main(int argc, char * const* argv)
{
	int ret = 0;
	int c = 0;
	const char* optstring = "d:r:c:";

	
	if(argc < 3||argc > 5) {
		printf("invalid arguments argc==%d\n", argc);
		fprintf(stderr,usage,argv[0],argv[0],argv[0],argv[0]);
		exit(-1);
	}

	ImageDecoderIf_t* pDecoder = AmlImage_Init();

    //for (int i = 1; i < argc; ++i)
    {
        const char* option = argv[1];
        char* const*optarg = argv + 2;

        if(!strcmp("-d", option))//unpack: argv[0] -d <archive> [out dir] 
		{
            const char* amlImg = optarg[0];
            const char* unPackDir = optarg[1];
			if(access(amlImg,F_OK|R_OK) < 0){
				printf("Err:%s is not found! \n",amlImg);
                return __LINE__;
			}
			if(argc == 4)
				ret = image_unpack(amlImg,unPackDir);
			else
				ret = image_unpack(amlImg,NULL);
		}
		else if(!strcmp("-r", option))//pack: argv[0] -r cfgFile workDir targetFile
		{
			if(argc != 5){
				printf("invalid arguments argc==%d\n", argc);
				fprintf(stderr,usage,argv[0],argv[0],argv[0],argv[0]);
				exit(-1);
			}
            const char* cfgFile = optarg[0];
            const char* workdir = optarg[1];
            const char* targetFile = optarg[2];
			if(access(cfgFile,F_OK|R_OK) < 0){
				fprintf(stderr,"Err:%s is not found! \n", cfgFile);
			}
			printf("image_pack cfg: %s\n", cfgFile);

			ret = image_pack(cfgFile, workdir, targetFile);
		}
		else if(!strcmp("-c", option))//check: argv[0] amlImage
		{	
            const char* amlImg = optarg[0];
			if(access(amlImg,F_OK|R_OK) < 0){
				fprintf(stderr,"Err:%s is not found!\n", amlImg);
			}
			FILE *fp = fopen(amlImg,"rb");
			if(image_check((HIMAGE) fp) < 0){
				fprintf(stderr,"the image check fail!!\n");
			}else{
				printf("the image check ok!\n");
			}
			fclose(fp);

		}
		else if(!strcmp("-?", option))
		{
			fprintf(stderr,usage,argv[0],argv[0],argv[0],argv[0]);
			ret = -1;
		}
		else{
			fprintf(stderr,usage,argv[0],argv[0],argv[0],argv[0]);
		}
    }

	AmlImage_Final(pDecoder);
	return ret;
}
#endif// #ifndef BUILD_DLL
