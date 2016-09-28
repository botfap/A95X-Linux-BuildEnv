/*************************************************************************************
 *       @file  pcmenc_api.c
 *      @brief  
 *
 * Detailed description starts here.
 *
 *     @author  jian.xu, jian.xu@amlogic.com
 *
 *   @internal
 *     Created  20124-26
 *    Revision  v1.0 
 *    Compiler  gcc/g++
 *     Company  Amlogic Inc.
 *   Copyright  Copyright (c) 2011, herbert.hu
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 *************************************************************************************
 */ 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "pcmenc_api.h"
#define AUDIODSP_PCMENC_DEV_NAME  "/dev/audiodsp_pcmenc"

static char *map_buf;
static unsigned read_offset = 0;
static unsigned buffer_size = 0;
static int dev_fd = -1;
#include <log-print.h>
int pcmenc_init()
{
	buffer_size = 0;
	read_offset = 0;
	map_buf = 0xffffffff;
	dev_fd = -1;
    dev_fd = open(AUDIODSP_PCMENC_DEV_NAME, O_RDONLY);
    if(dev_fd < 0){
        //printf("can not open %s\n", AUDIODSP_PCMENC_DEV_NAME);
        adec_print("can not open %s\n", AUDIODSP_PCMENC_DEV_NAME);
        return -1;
    }
    ioctl(dev_fd, AUDIODSP_PCMENC_GET_RING_BUF_SIZE, &buffer_size); 
/* mapping the kernel buffer to user space to acess */    
    map_buf= mmap(0,buffer_size, PROT_READ , MAP_PRIVATE, dev_fd, 0);
    if((unsigned)map_buf == -1){
    	//printf("pcmenc:mmap failed,err id %d \n",errno);
    	adec_print("pcmenc:mmap failed,err id %d \n",errno);
    	close(dev_fd);
    	return -1;
    }
	return 0;	
}
static unsigned pcm_read_num = 0;
static int pcmenc_skip_pcm(int size)
{
    int ring_buf_content = 0;
    int len = 0;
    int tail = 0;	
    ioctl(dev_fd, AUDIODSP_PCMENC_GET_RING_BUF_CONTENT, &ring_buf_content); 
    if(ring_buf_content > size){
    	if(read_offset+size > buffer_size){
    		//tail = size - read_offset;
    		tail = buffer_size - read_offset;
   // 		memcpy(inputbuf,map_buf+read_offset,tail);
    		read_offset = 0;
    //		memcpy(inputbuf+tail,map_buf+read_offset,size-tail);
    		read_offset = size-tail;
    	}
    	else{
   // 		memcpy(inputbuf,map_buf+read_offset,size);
    		read_offset += size;
    		
    	}
		pcm_read_num += size;
		ioctl(dev_fd, AUDIODSP_PCMENC_SET_RING_BUF_RPTR,read_offset);	
		return size;
	} 
	else
		return 0;

}
int pcmenc_read_pcm(char *inputbuf,int size)
{
    int ring_buf_content = 0;
    int len = 0;
    int tail = 0;	
    ioctl(dev_fd, AUDIODSP_PCMENC_GET_RING_BUF_CONTENT, &ring_buf_content); 
	if(ring_buf_content > buffer_size*4/5){
		pcmenc_skip_pcm(size*4);
		memset(inputbuf,0,size);
		adec_print("pcmenc buffer full,skip %d bytes \n",4*size);
		return size;
		
		//ioctl(dev_fd, AUDIODSP_PCMENC_GET_RING_BUF_CONTENT, &ring_buf_content); 
	}
	//adec_print("read num %d,countent %d,total %d\n",pcm_read_num,ring_buf_content,pcm_read_num+ring_buf_content);
    if(ring_buf_content > size){
    	if(read_offset+size > buffer_size){
    		//tail = size - read_offset;
    		tail = buffer_size - read_offset;
    		memcpy(inputbuf,map_buf+read_offset,tail);
    		read_offset = 0;
    		memcpy(inputbuf+tail,map_buf+read_offset,size-tail);
    		read_offset = size-tail;
    	}
    	else{
    		memcpy(inputbuf,map_buf+read_offset,size);
    		read_offset += size;
    		
    	}
		pcm_read_num += size;
		ioctl(dev_fd, AUDIODSP_PCMENC_SET_RING_BUF_RPTR,read_offset);	
		return size;
	} 
	else
		return 0;
}
int  pcmenc_get_pcm_info(pcm51_encoded_info_t *info)
{
	int ret;
	ret = ioctl(dev_fd, AUDIODSP_PCMENC_GET_PCMINFO,info);	
	if(ret)
		return ret;
 	adec_print("InfoValidFlag %d,SampFs %d,NumCh %d,AcMode %d,LFEFlag %d,BitsPerSamp %d \n", \
	info->InfoValidFlag,info->SampFs,info->NumCh,info->AcMode,info->LFEFlag,info->BitsPerSamp);	
	return 0;
	
}
int pcmenc_deinit()
{
	pcm_read_num = 0;

	if((unsigned)map_buf != 0xffffffff)
		munmap(map_buf,buffer_size);
	if(dev_fd >= 0)
		close(dev_fd);
	return 0;		
}