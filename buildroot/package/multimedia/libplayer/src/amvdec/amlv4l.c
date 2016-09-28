#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "amlv4l.h"
#include "amvdec_priv.h"
#define V4LDEVICE_NAME  "/dev/video10"

static int amlv4l_unmapbufs(amvideo_dev_t *dev);
static int amlv4l_mapbufs(amvideo_dev_t *dev);
int amlv4l_setfmt(amvideo_dev_t *dev, struct v4l2_format *fmt);
int amlv4l_init(amvideo_dev_t *dev, int type,int width,int height,int fmt)
{
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    struct v4l2_format v4lfmt;
    ret = open(V4LDEVICE_NAME, O_RDWR | O_NONBLOCK);
    if (ret < 0) {
        LOGE("v4l device opend failed!,ret=%d,%s(%d)\n", ret, strerror(errno), errno);
        return errno;
    }
    v4l->v4l_fd = ret;
    v4l->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l->width=width;
    v4l->height=height;
    v4l->pixformat=fmt;
    v4lfmt.type = v4l->type;
    v4lfmt.fmt.pix.width       = v4l->width; 
    v4lfmt.fmt.pix.height      =  v4l->height;
    v4lfmt.fmt.pix.pixelformat = v4l->pixformat;	
    ret=amlv4l_setfmt(dev,&v4lfmt);
    if(ret!=0){
	 goto error_out;
    }
    ret=amlv4l_mapbufs(dev);
error_out:	
    return ret;
}

static int amlv4l_ioctl(amvideo_dev_t *dev, int request, void *arg)
{
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    ret = ioctl(v4l->v4l_fd, request, arg);
    if (ret == -1 && errno) {
	  LOGE("amlv4l_ioctlfailed!,request=%x,ret=%d,%s(%d)\n",request, ret, strerror(errno), errno);	
         ret = -errno;
    }
    return ret;
}

int amlv4l_release(amvideo_dev_t *dev)
{
    int ret = -1;
    amlv4l_dev_t *v4l = dev->devpriv;
    if (v4l->v4l_fd < 0) {
        return 0;
    }
    amlv4l_stop(dev);
    amlv4l_unmapbufs(dev);
    if (v4l->v4l_fd >= 0) {
        ret = close(v4l->v4l_fd);
    }
    v4l->v4l_fd = -1;
    free(dev);	
    if (ret == -1 && errno) {
        ret = -errno;
    }

    return ret;
}

int amlv4l_dequeue_buf(amvideo_dev_t *dev, vframebuf_t *vf)
{
    struct v4l2_buffer vbuf;
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;	
    vbuf.type = v4l->type;
    vbuf.memory = V4L2_MEMORY_MMAP;	
    vbuf.index = v4l->bufferNum;
    ret = amlv4l_ioctl(dev, VIDIOC_DQBUF, &vbuf);
    if (!ret && vbuf.index < v4l->bufferNum) {
        memcpy(vf, &v4l->vframe[vbuf.index], sizeof(vframebuf_t));
    }
    return ret;
}

int amlv4l_queue_buf(amvideo_dev_t *dev, vframebuf_t *vf)
{
    struct v4l2_buffer vbuf;
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;		
    vbuf.index = vf->index;
    return amlv4l_ioctl(dev, VIDIOC_QBUF, &vbuf);
}

int amlv4l_start(amvideo_dev_t *dev)
{
    int type;
    amlv4l_dev_t *v4l = dev->devpriv;
    type=v4l->type;
    return amlv4l_ioctl(dev, VIDIOC_STREAMON,&type);
}

int amlv4l_stop(amvideo_dev_t *dev)
{
    int type;
    amlv4l_dev_t *v4l = dev->devpriv;
    type=v4l->type;
    return amlv4l_ioctl(dev, VIDIOC_STREAMOFF,&type);
}


int amlv4l_setfmt(amvideo_dev_t *dev, struct v4l2_format *fmt)
{
    return amlv4l_ioctl(dev, VIDIOC_S_FMT, fmt);
}


static int amlv4l_unmapbufs(amvideo_dev_t *dev)
{
    int i, ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    vframebuf_t *vf = v4l->vframe;
    ret = 0;
    if(!vf)	
	return 0;	
    for (i = 0; i < v4l->bufferNum; i++) {
        if (vf[i].vbuf == NULL || vf[i].vbuf == MAP_FAILED) {
            continue;
        }
      	ret = munmap(vf[i].vbuf, vf[i].length);
       vf[i].vbuf = NULL;
    }
    free(v4l->vframe);
    v4l->vframe=NULL;
    return ret;
}

static int amlv4l_mapbufs(amvideo_dev_t *dev)
{
    struct v4l2_buffer vbuf;
    int ret;
    int i;
    amlv4l_dev_t *v4l = dev->devpriv;
    vframebuf_t *vf;
    struct v4l2_requestbuffers req;

    req.count = 4;
    req.type = v4l->type;;
    req.memory = v4l->memory_mode;	;	
    ret=amlv4l_ioctl(dev, VIDIOC_REQBUFS, &req);
    if(ret!=0){
         LOGE("VIDIOC_REQBUFS failed,ret=%d\n", ret);
	  return ret;
    }
    v4l->bufferNum=req.count;		
    vf=(vframebuf_t *)malloc(sizeof(vframebuf_t)*req.count);
    if(!vf)
        goto errors_out;
    v4l->vframe=vf;
    vbuf.type = v4l->type;
    vbuf.memory = v4l->memory_mode;		
    for (i = 0; i < v4l->bufferNum; i++) {
	 vbuf.index = i;
        ret = amlv4l_ioctl(dev, VIDIOC_QUERYBUF, &vbuf);
        if (ret < 0) {
            LOGE("VIDIOC_QUERYBUF,ret=%d\n", ret);
            goto errors_out;
        }

        vf[i].index = i;
        vf[i].offset = vbuf.m.offset;
        vf[i].pts = 0;
        vf[i].duration = 0;
        vf[i].vbuf = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, v4l->v4l_fd, vbuf.m.offset);
        if (vf[i].vbuf == NULL || vf[i].vbuf == MAP_FAILED) {
            LOGE("mmap failed,index=%d,ret=%p, errstr=%s\n", i,vf[i].vbuf, strerror(errno));
            ret = -errno;
            goto errors_out;
        }
	vf[i].length=vbuf.length;
	 LOGI("mmaped buf %d,off=%d,vbuf=%p,len=%d\n", vf[i].index,vf[i].offset,vf[i].vbuf,vf[i].length);
    }
     LOGI("mmap ok,bufnum=%d\n", i);
    return 0;
errors_out:
    amlv4l_unmapbufs(dev);
    return ret;
}

amvideo_dev_t *new_amlv4l(void)
{
    //...
    amvideo_dev_t *dev;
    amlv4l_dev_t *v4l;
    dev = malloc(sizeof(amvideo_dev_t) + sizeof(amlv4l_dev_t));
    memset(dev, 0, sizeof(amvideo_dev_t) + sizeof(amlv4l_dev_t));
    dev->devpriv=(void *)((int)(&dev->devpriv)+4);
    v4l = dev->devpriv;
    v4l->memory_mode=V4L2_MEMORY_MMAP;
    dev->ops.init = amlv4l_init;
    dev->ops.release = amlv4l_release;
    dev->ops.dequeuebuf = amlv4l_dequeue_buf;
    dev->ops.queuebuf = amlv4l_queue_buf;
    dev->ops.start = amlv4l_start;
    dev->ops.stop = amlv4l_stop;
    /**/
    return dev;
}

