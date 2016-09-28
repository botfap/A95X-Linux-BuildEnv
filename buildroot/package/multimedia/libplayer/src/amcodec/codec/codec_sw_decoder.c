/**
* @file codec_sw_decoder.c
* @brief  Definition of codec devices and function prototypes
* @author JunLiang Zhou <junliang.zhou@amlogic.com>
* @version 1.0.0
* @date 2016-01-28
*/
/* Copyright (C) 2007-2016, Amlogic Inc.
* All right reserved
* 
*/
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec_type.h>
#include <codec.h>
#include "codec_h_ctrl.h"
#include "codec_sw_decoder.h"
#include <sys/poll.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "ion_dev.h"
#include "ion.h"
#include "amvideo.h"

#include <list.h>
#include <pthread.h>

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

//#define SAVE_YUV_FILE

#ifdef SAVE_YUV_FILE
    int out_fp = -1;
#endif

#define YUVPLAYER_PATH "/dev/yuvplayer"

#define TSYNC_PCRSCR    "/sys/class/tsync/pts_pcrscr"
#define TSYNC_APTS      "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS      "/sys/class/tsync/pts_video"

float ratio_video;
int drop_count = 0;

#define DECODER_THREAD_NUM 4

#define ALIGN64(width) ((width+63)>>6)<<6

#define OUT_BUFFER_COUNT   4
static out_buffer_t * mOutBuffer = NULL;
static int mIonFd = 0;
static int out_num = 0;

static pthread_mutex_t pthread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  pthread_cond = PTHREAD_COND_INITIALIZER;

int  sysfs_get_sysfs_str(const char *path, char *valstr, int size)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
		memset(valstr,0,size);
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        //LOGE("unable to open file %s,err: %s", path, strerror(errno));
        sprintf(valstr, "%s", "fail");
        return -1;
    };
    //LOGI("get_sysfs_str=%s\n", valstr);
    return 0;
}

#define AVPKTNODE_LOCK(node)\
    do{pthread_mutex_lock(&node->list_mutex);\
    }while(0);

#define AVPKTNODE_UNLOCK(node)\
    do{pthread_mutex_unlock(&node->list_mutex);\
    }while(0);

#define AVPKTNODE_LOCK_INIT(node)\
    do{pthread_mutex_init(&node->list_mutex,NULL);\
    }while(0);

int avpkt_node_list_init(struct avpkt_node_list *avpkt_node_list)
{
    avpkt_node_list->node_count = 0;
	avpkt_node_list->data_size  = 0;
    INIT_LIST_HEAD(&avpkt_node_list->list);
    AVPKTNODE_LOCK_INIT(avpkt_node_list);
    return 0;
}

struct avpkt_node * avpkt_node_alloc(int ext) {
    return malloc(sizeof(struct avpkt_node) + ext);
}

void avpkt_node_free(struct avpkt_node *avpkt_node)
{
    free(avpkt_node);
}

int avpkt_node_list_add_tail(struct avpkt_node_list *avpkt_node_list, struct avpkt_node *avpkt_node)
{
	AVPKTNODE_LOCK(avpkt_node_list);
    list_add_tail(&avpkt_node->list, &avpkt_node_list->list);
    avpkt_node_list->node_count++;
	avpkt_node_list->data_size += avpkt_node->avpkt->size;
	AVPKTNODE_UNLOCK(avpkt_node_list);
    return 0;
}

struct avpkt_node *  avpkt_node_list_get_head(struct avpkt_node_list *avpkt_node_list) {
    struct avpkt_node *avpkt_node = NULL;

	AVPKTNODE_LOCK(avpkt_node_list);
    if (list_empty(&avpkt_node_list->list)) {
		AVPKTNODE_UNLOCK(avpkt_node_list);
        return NULL;
    }

    avpkt_node = list_first_entry(&avpkt_node_list->list, struct avpkt_node, list);
    if(NULL != avpkt_node) {
        list_del(&avpkt_node->list);
        avpkt_node_list->node_count--;
		avpkt_node_list->data_size -= avpkt_node->avpkt->size;
    }
	AVPKTNODE_UNLOCK(avpkt_node_list);

    return avpkt_node;
}

void *sw_decoder_thread(codec_para_t *pcodec) {
	int index, ret;
    int got_picture = 0;
    AVCodecContext *ic = NULL;
    AVCodec *codec = NULL;
    AVFrame *picture = NULL;
	struct avpkt_node *avpkt_node = NULL;
	AVPacket *avpkt;
	int64_t vpts;
	int width, align_width, height;
	int output_frame_count = 0;

	align_width = ALIGN64(pcodec->am_sysinfo.width);
	width = pcodec->am_sysinfo.width;
	height = pcodec->am_sysinfo.height;

    ic		= pcodec->ic;
    picture = pcodec->picture;
	CODEC_PRINT("[%s %d]\n", __FUNCTION__, __LINE__);

	//amsysfs_set_sysfs_str("/sys/class/video/freerun_mode", "1");

    for (;;) {

        avpkt_node = NULL;
		avpkt      = NULL;

		avpkt_node = avpkt_node_list_get_head(pcodec->avpkt_node_list);

        if (SW_CODEC_STOP == pcodec->status && NULL == avpkt_node)
            break;

        if (NULL == avpkt_node)
            continue;

        avpkt = avpkt_node->avpkt;

        avcodec_decode_video2(ic, picture, &got_picture, avpkt);

		//CODEC_PRINT("[%s %d] data:%x pts:%llx pos:%llx width:%d height:%d line:%d %d %d got_picture:%d\n", __FUNCTION__, __LINE__, avpkt->data, avpkt->pts, avpkt->pos,
			//ic->width, ic->height, picture->linesize[0], picture->linesize[1], picture->linesize[2], got_picture);

        av_free_packet(avpkt_node->avpkt);
        avpkt_node_free(avpkt_node);

        if(drop_count > 0)
			drop_count--;

        if (got_picture && drop_count == 0) {
			while(1){
				vframebuf_t vf;
				int len = 0;
				void *cpu_ptr = NULL;
				ret = amlv4l_dequeuebuf(pcodec->amvideo_dev, &vf);
				//CODEC_PRINT("[%s %d] ret:%d index:%d fd:%d\n", __FUNCTION__, __LINE__, ret, vf.index, vf.fd);
				if (ret >= 0) {
					int i = 0, j = 0;
					while (i < OUT_BUFFER_COUNT) {
						if (vf.fd == mOutBuffer[i].fd) {
							cpu_ptr = mOutBuffer[i].fd_ptr;
							break;
						}
						i++;
					}
					output_frame_count++;

					vf.index = mOutBuffer[i].index;
					vf.fd = mOutBuffer[i].fd;
					vf.length = align_width*height*3/2;
					vf.pts = avpkt->pts * ratio_video;
					vf.width = align_width;
					vf.height = height;

					//CODEC_PRINT("count:%d pts:%lld %lld ratio_video:%f\n", output_frame_count, vf.pts, avpkt->pts, ratio_video);

					for (j = 0 ; j < height ; j++) {
						memcpy(cpu_ptr + align_width * j, picture->data[0] + j * picture->linesize[0], width);
						memset(cpu_ptr + align_width * j + width, 0, align_width - width);
					}
					for (j = 0 ; j < height / 2 ; j++) {
						memcpy(cpu_ptr + align_width*height + align_width * j / 2, picture->data[1] + j * picture->linesize[1], width / 2);
						memset(cpu_ptr + align_width*height + align_width * j / 2 + width / 2, 0x80, (align_width - width)/2);
					}
					for (j = 0 ; j < height / 2 ; j++) {
						memcpy(cpu_ptr + align_width*height + align_width*height/4 + align_width * j / 2, picture->data[2] + j * picture->linesize[2], width / 2);
						memset(cpu_ptr + align_width*height + align_width*height/4 + align_width * j / 2 + width/2, 0x80, (align_width - width)/2);
					}

					ret = amlv4l_queuebuf(pcodec->amvideo_dev, &vf);
					if (ret < 0) {
						printf("amlv4l_queuebuf failed =%d\n", ret);
					}
					break;
				}
			}

#ifdef SAVE_YUV_FILE
            if (out_fp >= 0) {
                int k;
                for (k = 0 ; k < height ; k++) {
                    write(out_fp, picture->data[0] + k * picture->linesize[0], width);
                }
                for (k = 0 ; k < height / 2 ; k++) {
                    write(out_fp, picture->data[1] + k * picture->linesize[1], width / 2);
                }
                for (k = 0 ; k < height / 2 ; k++) {
                    write(out_fp, picture->data[2] + k * picture->linesize[2], width / 2);
                }
            }
#endif
        }
    }

	pcodec->status = SW_CODEC_STOPED;
	CODEC_PRINT("[%s %d] stoped\n", __FUNCTION__, __LINE__);
	
	pthread_cond_signal(&pthread_cond);

    return;
}

int AllocDmaBuffers(int buffer_size) {
    struct ion_handle *ion_hnd;
    int shared_fd;
    int ret = 0;
	
    mIonFd = ion_open();
    if (mIonFd < 0) {
        printf("ion open failed!\n");
        return -1;
    }
    int i = 0;
    while (i < OUT_BUFFER_COUNT) {
        unsigned int ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
        ret = ion_alloc(mIonFd, buffer_size, 0, ION_HEAP_CARVEOUT_MASK, ion_flags, &ion_hnd);
        if (ret) {
            printf("ion alloc error");
            ion_close(mIonFd);
            return -1;
        }
        ret = ion_share(mIonFd, ion_hnd, &shared_fd);
        if (ret) {
            printf("ion share error!\n");
            ion_free(mIonFd, ion_hnd);
            ion_close(mIonFd);
            return -1;
        }
        void *cpu_ptr = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
        if (MAP_FAILED == cpu_ptr) {
            printf("ion mmap error!\n");
            ion_free(mIonFd, ion_hnd);
            ion_close(mIonFd);
            return -1;
        }
		
        CODEC_PRINT("[%s %d] i:%d shared_fd:%d cpu_ptr:%x\n", __FUNCTION__, __LINE__, i, shared_fd, cpu_ptr);
		
        mOutBuffer[i].index = i;
        mOutBuffer[i].fd = shared_fd;
        mOutBuffer[i].pBuffer = NULL;
        mOutBuffer[i].own_by_v4l = -1;
        mOutBuffer[i].fd_ptr = cpu_ptr;
        mOutBuffer[i].ion_hnd = ion_hnd;
        i++;
    }
    return ret;
}

int FreeDmaBuffers(int buffer_size) {
    int i = 0;

    while (i < OUT_BUFFER_COUNT) {
        munmap(mOutBuffer[i].fd_ptr, buffer_size);
        close(mOutBuffer[i].fd);
        printf("FreeDmaBuffers_mOutBuffer[i].fd=%d,mIonFd=%d\n", mOutBuffer[i].fd, mIonFd);
        ion_free(mIonFd, mOutBuffer[i].ion_hnd);
        i++;
    }
    int ret = ion_close(mIonFd);
    return ret;
}


int codec_sw_decoder_init(codec_para_t *pcodec) {
	int i;
    int ret = 0;
    pthread_t       tid;
    pthread_attr_t pthread_attr;
    char * mmap_addr;
	int yuv_buffer_size;
	struct amvideo_dev *amvideo_dev;
	int align_width, height;

    drop_count = 0;
	
	align_width = ALIGN64(pcodec->am_sysinfo.width);
	height = pcodec->am_sysinfo.height;

    av_register_all();

    AVCodecContext *ic = avcodec_alloc_context();
    if (!ic) {
        CODEC_PRINT("AVCodec Memory error\n");
        ic = NULL;
        return CODEC_OPEN_HANDLE_FAILED;
    }

    ic->codec_id = pcodec->codec_id;
    ic->codec_type = CODEC_TYPE_VIDEO;
    ic->pix_fmt = PIX_FMT_YUV420P;
	ic->thread_count = DECODER_THREAD_NUM;

    pcodec->ic = ic;
    pcodec->codec = avcodec_find_decoder(ic->codec_id);

	CODEC_PRINT("cap:%x\n", pcodec->codec->capabilities);

    if (!pcodec->codec) {
        CODEC_PRINT("Codec not found\n");
        return CODEC_OPEN_HANDLE_FAILED;
    }

	CODEC_PRINT("avcodec_open thread_count:%d\n", pcodec->ic->thread_count);
    if (avcodec_open(pcodec->ic, pcodec->codec) < 0) {
        CODEC_PRINT("Could not open codec\n");
        return CODEC_OPEN_HANDLE_FAILED;
    }

    pcodec->picture = avcodec_alloc_frame();
    if (!pcodec->picture) {
        CODEC_PRINT("Could not allocate picture\n");
        return CODEC_OPEN_HANDLE_FAILED;
    }

#ifdef SAVE_YUV_FILE
    out_fp = open("/mnt/udisk/output.yuv", O_CREAT | O_RDWR);
    if (out_fp < 0) {
        CODEC_PRINT("Create output file failed! fd=%d\n", out_fp);
    }
#endif

	amsysfs_set_sysfs_str("/sys/class/vfm/map", "rm default");
	amsysfs_set_sysfs_str("/sys/class/vfm/map", "add default yuvplayer amvideo");

    mOutBuffer = (out_buffer_t *)malloc(sizeof(out_buffer_t) * OUT_BUFFER_COUNT);
    memset(mOutBuffer, 0, sizeof(out_buffer_t) * OUT_BUFFER_COUNT);

    AllocDmaBuffers(align_width * height * 3 / 2);
    amvideo_dev = new_amvideo(FLAGS_V4L_MODE);
    amvideo_dev->display_mode = 0;
    ret = amvideo_init(amvideo_dev, 0, align_width, height, V4L2_PIX_FMT_YUV420, OUT_BUFFER_COUNT);

    if (ret < 0) {
        CODEC_PRINT("amvideo_init failed =%d\n", ret);
        amvideo_release(amvideo_dev);
        amvideo_dev = NULL;
    }
	
	pcodec->amvideo_dev = amvideo_dev;

    vframebuf_t vf;
    memset(&vf, 0, sizeof(vframebuf_t));

    while (i < OUT_BUFFER_COUNT) {
        vf.index = mOutBuffer[i].index;
        vf.fd = mOutBuffer[i].fd;
        vf.length = align_width*height*3/2;

		CODEC_PRINT("[%s %d] i:%d\n", __FUNCTION__, __LINE__, i);

		memset(mOutBuffer[i].fd_ptr, 0x0, align_width*height);
		memset(mOutBuffer[i].fd_ptr + align_width*height, 0x80, align_width*height/2);

        int ret = amlv4l_queuebuf(amvideo_dev, &vf);
        if (ret < 0) {
            CODEC_PRINT("amlv4l_queuebuf failed =%d\n", ret);
        }

        if (i == 1) {
            ret = amvideo_start(amvideo_dev);
            CODEC_PRINT("amvideo_start ret=%d\n", ret);
            if (ret < 0) {
                CODEC_PRINT("amvideo_start failed =%d\n", ret);
                amvideo_release(amvideo_dev);
                amvideo_dev = NULL;
            }
        }
        i++;
    }

	pcodec->avpkt_node_list = malloc(sizeof(struct avpkt_node_list));

    if(NULL != pcodec->avpkt_node_list)
        avpkt_node_list_init(pcodec->avpkt_node_list);
    else
        return CODEC_ERROR_NOMEM;

    pcodec->status = SW_CODEC_STARTED;
	CODEC_PRINT("[%s %d]\n", __FUNCTION__, __LINE__);

    pthread_mutex_init(&pthread_mutex, NULL);
    pthread_cond_init(&pthread_cond, NULL); 

    pthread_attr_init(&pthread_attr);
    pthread_attr_setstacksize(&pthread_attr, 0);   //default stack size maybe better
    ret = pthread_create(&tid, &pthread_attr, (void*)&sw_decoder_thread, (void*)pcodec);
    if (ret != 0) {
        CODEC_PRINT("creat software player thread failed !\n");
        return CODEC_ERROR_INVAL;
    }
	CODEC_PRINT("[%s %d]\n", __FUNCTION__, __LINE__);

    return CODEC_ERROR_NONE;
}

#define DUP_DATA(dst, src, size, padding) \
    do { \
        void *data; \
        if (padding) { \
            if ((unsigned)(size) > (unsigned)(size) + FF_INPUT_BUFFER_PADDING_SIZE) \
                return CODEC_ERROR_NONE; \
            data = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE); \
        } else { \
            data = av_malloc(size); \
        } \
        if (!data) \
            return CODEC_ERROR_NONE; \
        memcpy(data, src, size); \
        if (padding) \
            memset((uint8_t*)data + size, 0, FF_INPUT_BUFFER_PADDING_SIZE); \
        dst = data; \
    } while(0)

static int codec_dup_avpkt(AVPacket *dst, AVPacket *src){
    dst->pts                  = src->pts;
    dst->dts                  = src->dts;
    dst->stream_index         = src->stream_index;
    dst->flags                = src->flags;
    dst->side_data_elems      = src->side_data_elems;
    dst->duration             = src->duration;
    dst->pos                  = src->pos;
    dst->convergence_duration = src->convergence_duration;
    dst->size                 = src->size;

	if (src->side_data_elems) {
		int i;
	
		DUP_DATA(dst->side_data, src->side_data,
				 src->side_data_elems * sizeof(*src->side_data), 0);
		memset(dst->side_data, 0, dst->side_data_elems * sizeof(*dst->side_data));
		for (i = 0; i < dst->side_data_elems; i++) {
			DUP_DATA(dst->side_data[i].data, src->side_data[i].data,
					 src->side_data[i].size, 1);
		}
	}

    DUP_DATA(dst->data, src->data, src->size, 1);

    return CODEC_ERROR_NONE;
}

int codec_sw_decoder_write(codec_para_t *pcodec, AVPacket *avpkt){
	AVPacket *pkt = NULL;
    struct avpkt_node *avpkt_node_t = NULL;
	struct avpkt_node_list *avpkt_node_list;

	avpkt_node_list = pcodec->avpkt_node_list;

    if (avpkt_node_list->node_count > MAX_AVPKT_NUM)
        return CODEC_ERROR_NOMEM;
	
	avpkt_node_t = avpkt_node_alloc(0);

    if(NULL == avpkt_node_t)
        return CODEC_ERROR_NOMEM;

    if (!(pkt = av_mallocz(sizeof(AVPacket))) || av_new_packet(pkt, avpkt->size) < 0){
        av_free(pkt);
        return CODEC_ERROR_NOMEM;
    }

	codec_dup_avpkt(pkt, avpkt);

    avpkt_node_t->avpkt = pkt;

	//CODEC_PRINT("[%s %d]size:%d pts:%lld pos:%lld\n", __FUNCTION__, __LINE__,  pkt->size, pkt->pts, pkt->pos);

    avpkt_node_list_add_tail(pcodec->avpkt_node_list, avpkt_node_t);

    return CODEC_ERROR_NONE;
}

int codec_sw_decoder_release(codec_para_t *pcodec){	
	
	
    int i;
	vframebuf_t vf;
	int align_width, height;

	align_width = ALIGN64(pcodec->am_sysinfo.width);
	height = pcodec->am_sysinfo.height;

	CODEC_PRINT("[%s %d]\n", __FUNCTION__, __LINE__);
	
	pcodec->status = SW_CODEC_STOP;
	pthread_cond_wait(&pthread_cond, &pthread_mutex);

	CODEC_PRINT("[%s %d]\n", __FUNCTION__, __LINE__);

    usleep(1000*1000);
    for(i=0;i<10;i++) {
        amlv4l_dequeuebuf(pcodec->amvideo_dev, &vf);
    }

    if (pcodec->amvideo_dev) {
        amvideo_stop(pcodec->amvideo_dev);
        amvideo_release(pcodec->amvideo_dev);
        pcodec->amvideo_dev = NULL;
    }
    amsysfs_set_sysfs_str("/sys/class/vfm/map", "rm default");
    amsysfs_set_sysfs_str("/sys/class/vfm/map", "add default decoder ppmgr deinterlace amvideo");

    FreeDmaBuffers(align_width * height * 3 / 2);
    if (mOutBuffer != NULL) {
        free(mOutBuffer);
        mOutBuffer = NULL;
    }

	if(pcodec->ic){
		CODEC_PRINT("close ffmpeg context\n");
		avcodec_close(pcodec->ic);
		av_free(pcodec->ic);
	}

	CODEC_PRINT("[%s %d] stoped\n", __FUNCTION__, __LINE__);
	
#ifdef SAVE_YUV_FILE
		close(out_fp);
#endif

	pthread_cond_destroy(&pthread_cond);
	pthread_mutex_destroy(&pthread_mutex);

    return CODEC_ERROR_NONE;
}

int codec_sw_decoder_getvpts(codec_para_t *pcodec, unsigned long *vpts){

    char buf[32];
	unsigned long video_pts;

	if (sysfs_get_sysfs_str(TSYNC_VPTS, buf, sizeof(buf)) == -1) {
		*vpts = -1;
		CODEC_PRINT("unable to open file %s,err: %s", TSYNC_VPTS, strerror(errno));
		return -1;
	}
	if (sscanf(buf, "0x%lx", &video_pts) < 1) {
		CODEC_PRINT("unable to get vpts from: %s", buf);
		*vpts = -1;
		return -1;
	}

    *vpts = video_pts;

	return 0;
}

int codec_sw_decoder_getpcr(codec_para_t *pcodec, unsigned long *pcr){

    char buf[32];
	unsigned long pcrscr;

	if (sysfs_get_sysfs_str(TSYNC_PCRSCR, buf, sizeof(buf)) == -1) {
		*pcr = -1;
		CODEC_PRINT("unable to open file %s,err: %s", TSYNC_PCRSCR, strerror(errno));
		return -1;
	}
	if (sscanf(buf, "0x%lx", &pcrscr) < 1) {
		CODEC_PRINT("unable to get vpts from: %s", buf);
		*pcr = -1;
		return -1;
	}

    *pcr = pcrscr;

	return 0;
}

int codec_sw_decoder_set_ratio(float ratio){
    ratio_video = ratio;
}

