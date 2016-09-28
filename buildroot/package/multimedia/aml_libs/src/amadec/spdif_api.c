/*************************************************************************************
 *       @file  spdif_api.c
 *      @brief  
 * Simple SPDIF test tools
 * Detailed description starts here.
 *
 *     @author  jian.xu jian.xu@amlogic.com
 *
 *   @internal
 *     Created  2012-4-19
 *    Revision  v1.0 
 *    Compiler  gcc/g++
 *     Company  Amlogic Inc.
 *   Copyright  Copyright (c) 2011, 
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 *************************************************************************************
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "spdif_api.h"
#include <log-print.h>
#define AUDIO_SPDIF_DEV_NAME  "/dev/audio_spdif"

static unsigned hw_rd_offset = 0;
static unsigned wr_offset = 0;
static unsigned iec958_buffer_size = 0;
static int    dev_fd = -1;
static int use_kernel_wr = 0x1;
static unsigned stream_type = STREAM_DTS;
static short iec958_buf[6144/2];
static char *map_buf = 0xffffffff;
static unsigned first_write = 1;
#define IEC958_LANTENCY  20
int iec958_init()
{
	int ret = 0;
	iec958_buffer_size = 0;
	hw_rd_offset = 0;
	wr_offset = 0;
	first_write = 1;
	dev_fd = open(AUDIO_SPDIF_DEV_NAME, /*O_RDONLY|O_WRONLY*/O_RDWR);
	if(dev_fd < 0){
		printf("can not open %s\n", AUDIO_SPDIF_DEV_NAME);
		ret = -1;
		goto exit;
	}    	
	/* call 958 module init here */
	ioctl(dev_fd, AUDIO_SPDIF_SET_958_INIT_PREPARE, 1); 
	/* get 958 dma buffer size */
	ioctl(dev_fd, AUDIO_SPDIF_GET_958_BUF_SIZE, &iec958_buffer_size); 
	//adec_print("iec958 buffer size %x\n",iec958_buffer_size);
	wr_offset = hw_rd_offset + 4*48000*IEC958_LANTENCY/1000; //delay 
	if(wr_offset >= iec958_buffer_size)
		wr_offset = iec958_buffer_size;		
	ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);		
	/* mapping the kernel 958 dma buffer to user space to acess */    
	map_buf= mmap(0,iec958_buffer_size, PROT_READ|PROT_WRITE,MAP_SHARED/*MAP_PRIVATE*/, dev_fd, 0);
	if((unsigned)map_buf == 0xffffffff){
		printf("mmap failed,error num %d \n",errno);
		ret = -2;
		goto exit1;
	}
	/* enable 958 outout */
//	ioctl(dev_fd, AUDIO_SPDIF_SET_958_ENABLE,1); 
	return 0;	
exit3:
     if((unsigned)map_buf != 0xffffffff)
     	munmap(map_buf,iec958_buffer_size);            
exit1:
     if(dev_fd >= 0)
        close(dev_fd);
exit:
    return ret;	
}

/*
	@param in buf, the input buffer for 61937 package,to be modify 
	@param in frame_size,the input data len
	@return , the packaged data length,maybe different than input length
*/
/* note ::: now fixed on 1.5M dts stream,maybe different */
int iec958_pack_frame(char *buf,int frame_size)
{
	
	short *left,*right,*pp;
	int i,j;	
	if(stream_type == STREAM_AC3){
		iec958_buf[0] = 0xF872;
		iec958_buf[1] = 0x4E1F;
		iec958_buf[2] = ((/*dd_bsmod*/0&7)<<8)|1;
		iec958_buf[3] = frame_size*8;//frame_size*8;//Pd as the Length-code
		memcpy((char*)iec958_buf+8,buf,frame_size);
		memset((char*)iec958_buf+frame_size+8,0,6144 - frame_size-8);	
		pp = (short*)iec958_buf;
		left = (short*)buf;
		right = left + 16;
		for(j = 0;j < 6144;j += 64){
			for(i = 0; i < 16 ; i++){
				*left++  = *pp;
				pp++;
				*right++ = *pp;
				pp++;
			}
			left += 16;
			right += 16;						
		} 
		return 6144; 		
	}
	/* dts iec60937 package according to DTS type and block num per frame */
	else if(stream_type == STREAM_DTS){
		iec958_buf[0] = 0xF872;
		iec958_buf[1] = 0x4E1F;
		iec958_buf[2] = 11;//2 block per frame,DTS type I
		iec958_buf[3] = 2040*8;//frame_size*8;//Pd as the Length-code
		memcpy((char*)iec958_buf+8,buf,frame_size);
		memset((char*)iec958_buf+frame_size+8,0,6144 - frame_size-8);	
		pp = (short*)iec958_buf;
		left = (short*)buf;
		right = left + 16;
		for(j = 0;j < 2048;j += 64){
			for(i = 0; i < 16 ; i++){
				*left++  = *pp;
				pp++;
				*right++ = *pp;
				pp++;
			}
			left += 16;
			right += 16;						
		} 
		return 2048; 		
		
	}
	/*raw wave file */
	else{
		memcpy(iec958_buf,buf,frame_size);
		pp = (short*)iec958_buf;
		left = (short*)buf;
		right = left + 16;
		for(j = 0;j < frame_size;j += 64){
			for(i = 0; i < 16 ; i++){
				*left++  = *pp;
				pp++;
				*right++ = *pp;
				pp++;
			}
			left += 16;
			right += 16;						
		} 
		return (frame_size>>6)<<6;
	}
	return 0;		
}
#define ALIGN 4096
static int iec958_buf_space_size(int dev_fd)
{
	int  space = 0;
	ioctl(dev_fd, AUDIO_SPDIF_GET_958_BUF_RD_OFFSET, &hw_rd_offset); 
	if(first_write == 1){
		if(hw_rd_offset >= wr_offset || (wr_offset-hw_rd_offset) < (4*48000*IEC958_LANTENCY/1000)){
			adec_print("reset iec958 hw wr ptr\n");
			wr_offset = hw_rd_offset + 4*48000*IEC958_LANTENCY/1000; //delay 
			if(wr_offset >= iec958_buffer_size)
				wr_offset = wr_offset - iec958_buffer_size;	
		}
		first_write = 0;
	}
	if(wr_offset > hw_rd_offset)	{
		space = iec958_buffer_size+hw_rd_offset - wr_offset;
	}
	else 
		space = hw_rd_offset - wr_offset;
	return space>ALIGN?(space-ALIGN):0/*&(~4095)*/;	
}
int iec958_packed_frame_write_958buf(char *buf,int frame_size)
{
	int tail = 0;
	int ret;
	/* check if i2s enable but 958 not enable */
	int status_958 = 0;
	int status_i2s = 0;
	ioctl(dev_fd, AUDIO_SPDIF_GET_958_ENABLE_STATUS, &status_958); 
	if(status_958 == 0){
		ioctl(dev_fd, AUDIO_SPDIF_GET_I2S_ENABLE_STATUS, &status_i2s); 
		if(status_i2s){
			status_958 = 1;
			ioctl(dev_fd, AUDIO_SPDIF_SET_958_ENABLE, status_958); 
			adec_print("spdif api:enable 958 output\n");
		}
		else{
			adec_print("discard data and wait i2s enable\n");
			return 0;//output are not enable yet,just return and wait 
		}	
	}
	while(iec958_buf_space_size(dev_fd) < frame_size){
//		printf("iec958 buffer full,space size %d,write size %d\n",iec958_buf_space_size(dev_fd),frame_size);
		//adec_print("iec958 buffer full,space size %d,write size %d\n",iec958_buf_space_size(dev_fd),frame_size);
		//usleep(5);
		return -1;
	}
	if(wr_offset ==  iec958_buffer_size)
		wr_offset = 0;	
	if(frame_size+wr_offset > iec958_buffer_size){
		tail = iec958_buffer_size -wr_offset;
		ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);
		//printf("0 tail %d,wr offset %d\n",tail,wr_offset);
		if(!use_kernel_wr){
			memcpy(map_buf+wr_offset,buf,tail);
			ret = msync(map_buf,iec958_buffer_size,MS_INVALIDATE|MS_SYNC);
			if(ret)
			     printf("msync0 err %d,error id %d addr %x\n",ret,errno,(unsigned)(map_buf+wr_offset));	
		}
		else
			write(dev_fd,buf,tail);
		wr_offset = 0;
		ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);
		//printf("1 tail %d,wr offset %d\n",frame_size-tail,wr_offset);
		if(!use_kernel_wr){
			memcpy(map_buf,buf+tail,frame_size-tail);
			ret = msync(map_buf,iec958_buffer_size,MS_INVALIDATE|MS_SYNC);
			if(ret)
				printf("msync1 err %d,error id %d addr %x\n",ret,errno,(unsigned)(map_buf));
		}	
		else	
			write(dev_fd,buf+tail,frame_size-tail);
		wr_offset = frame_size-tail;
		ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);

	}
	else
	{
		ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);	
		//printf("2 tail %d,wr offset %d\n",frame_size,wr_offset);
		if(!use_kernel_wr){
			memcpy(map_buf+wr_offset,buf,frame_size);
			ret = msync(map_buf+wr_offset,frame_size,MS_ASYNC|MS_INVALIDATE);
			if(ret)
			    printf("msync2 err %d,error id %d addr %x\n",ret,errno,(unsigned)(map_buf+wr_offset));
		}
		else
			write(dev_fd,buf,frame_size);			
		wr_offset += frame_size;
		ioctl(dev_fd,AUDIO_SPDIF_SET_958_WR_OFFSET,&wr_offset);

	}		
	return 0;
}
int iec958buf_fill_zero()
{
   int zero_filled_cnt=0,i2s_status=0,write_ret=0;
   char zerobuf[2048]={0};
   ioctl(dev_fd, AUDIO_SPDIF_GET_I2S_ENABLE_STATUS, &i2s_status);
   while((zero_filled_cnt<iec958_buffer_size) && i2s_status )
   {
       write_ret=iec958_packed_frame_write_958buf(zerobuf,2048);
	   if(write_ret)
	     break;
	   zero_filled_cnt+=2048;
	   ioctl(dev_fd, AUDIO_SPDIF_GET_I2S_ENABLE_STATUS, &i2s_status);
   }
   return 0;

}
/*
	@return 0:means 958 hw buffer maybe underrun,may need fill zero data 
	@return 1:means 958 hw buffer level is fine 
*/
#define IEC958_LEVEL_THREAD  4096
int iec958_check_958buf_level()
{
	int status_958 = 0;
	int hw_958_level = 0;
	ioctl(dev_fd, AUDIO_SPDIF_GET_958_ENABLE_STATUS, &status_958); 	
	if(status_958){
		hw_958_level = iec958_buffer_size - iec958_buf_space_size(dev_fd);
		if(hw_958_level < IEC958_LEVEL_THREAD)
		{
			return 0;
		}
	}
	return 1;
}
int iec958_deinit()
{
    if((unsigned)map_buf != 0xffffffff)
     	munmap(map_buf,iec958_buffer_size);            
    if(dev_fd >= 0)
		close(dev_fd);
	return 0;		
}
