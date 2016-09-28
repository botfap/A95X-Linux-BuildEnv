/*
 * sparse_format.c
 *
 *  Created on: 2013-6-3
 *      Author: binsheng.xu@amlogic.com
 */

//判断文件格式是否为sparse
#include <stdio.h>
#include "sparse_format.h"

//0 is not sparse packet header, else is sparse packet_header
int optimus_simg_probe(const char* source, const __u32 length)
{
	sparse_header_t *header = (sparse_header_t*) source;

    if(length < sizeof(sparse_header_t)) {
    	fprintf(stderr,"length %d < sparse_header_t len %d\n", length, FILE_HEAD_SIZE);
        return 0;
    }
	if (header->magic != SPARSE_HEADER_MAGIC) {
		//fprintf(stderr,"sparse bad magic, expect 0x%x but 0x%x\n", SPARSE_HEADER_MAGIC, header->magic);
		return 0;
	}

    if(!(SPARSE_HEADER_MAJOR_VER == header->major_version
                && FILE_HEAD_SIZE == header->file_hdr_sz
                && CHUNK_HEAD_SIZE == header->chunk_hdr_sz))
    {
        fprintf(stderr,"want 0x [%x, %x, %x], but [%x, %x, %x]\n",
                SPARSE_HEADER_MAJOR_VER,    FILE_HEAD_SIZE,             CHUNK_HEAD_SIZE,
                header->major_version,      header->file_hdr_sz,        header->chunk_hdr_sz);
        return 0;
    }
	return 1;
}

