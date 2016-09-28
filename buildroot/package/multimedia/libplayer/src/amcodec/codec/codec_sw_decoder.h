/**
* @file codec_sw_decoder.h
* @brief  Definition of codec devices and function prototypes
* @author JunLiang Zhou <junliang.zhou@amlogic.com>
* @version 1.0.0
* @date 2016-01-28
*/
/* Copyright (C) 2007-2016, Amlogic Inc.
* All right reserved
* 
*/
#ifndef CODEC_SW_DECODER_H
#define CODEC_SW_DECODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec_type.h>
#include <codec.h>
#include "codec_h_ctrl.h"
#include "codec_sw_decoder.h"

#include <list.h>
#include <pthread.h>

#include <libavcodec/avcodec.h>


#define SW_CODEC_STARTED 0
#define SW_CODEC_STOP    1
#define SW_CODEC_STOPED  2

#define MAX_AVPKT_NUM    128

struct avpkt_node {
    struct list_head list;
    AVPacket *avpkt;
};


struct avpkt_node_list {
    struct list_head list;
    int node_count;
    AVPacket *avpkt;
    unsigned int data_size;
    pthread_mutex_t list_mutex;
};

typedef struct {
    int index;
    int fd;
    void * pBuffer;
    int own_by_v4l;
    void *fd_ptr; //only for non-nativebuffer!
    struct ion_handle *ion_hnd; //only for non-nativebuffer!
}out_buffer_t;

int codec_sw_decoder_init(codec_para_t *pcodec);
int codec_sw_decoder_write(codec_para_t *pcodec, AVPacket *avpkt);
int codec_sw_decoder_release(codec_para_t *pcodec);
int codec_sw_decoder_getvpts(codec_para_t *pcodec, unsigned long *vpts);
int codec_sw_decoder_getpcr(codec_para_t *pcodec, unsigned long *pcr);
int codec_sw_decoder_set_ratio(float ratio);

#endif
