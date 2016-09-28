/*
 * crc32.c
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */
#include "image_packer_i.h"

#define BUFSIZE     1024*16

static unsigned int crc_table[256];


static void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++) {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }
}

/*计算buffer的crc校验码*/
static unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

/*
**计算大文件的CRC校验码:crc32函数,是对一个buffer进行处理,
**但如果一个文件相对较大,显然不能直接读取到内存当中
**所以只能将文件分段读取出来进行crc校验,
**然后循环将上一次的crc校验码再传递给新的buffer校验函数,
**到最后，生成的crc校验码就是该文件的crc校验码
*/
int calc_img_crc(FILE* fp, off_t offset)
{

    int nread;
    unsigned char buf[BUFSIZE];
    /*第一次传入的值需要固定,如果发送端使用该值计算crc校验码,
    **那么接收端也同样需要使用该值进行计算*/
    unsigned int crc = 0xffffffff;

    if (fp == NULL) {
        fprintf(stderr,"bad param!!\n");
        return -1;
    }

    init_crc_table();
    fseeko(fp,offset,SEEK_SET);
    while ((nread = fread(buf,1, BUFSIZE,fp)) > 0) {
        crc = crc32(crc, buf, nread);
    }

    if (nread < 0) {
        fprintf(stderr,"%d:read %s.\n", __LINE__, strerror(errno));
        return -1;
    }

    return crc;
}


