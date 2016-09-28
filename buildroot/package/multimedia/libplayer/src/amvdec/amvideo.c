#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <amvideo.h>
#include <Amavutils.h>
#include "amvdec_priv.h"
#include "amlv4l.h"


amvideo_dev_t *new_amvideo(int flags)
{
    amvideo_dev_t *dev = NULL;
    if (flags & FLAGS_V4L_MODE) {
        dev = new_amlv4l();
	 if(!dev){
	 	LOGE("alloc v4l devices failed.\n");
	 }else{
        	dev->mode = FLAGS_V4L_MODE;
	 }
    }
    return dev;
}
int amvideo_setparameters(amvideo_dev_t *dev, int cmd, void * parameters)
{
    return 0;
}
int amvideo_init(amvideo_dev_t *dev,int flags,int width,int height,int fmt)
{
    int ret=-1;
    if (dev->ops.init) {
        ret=dev->ops.init(dev,O_RDWR | O_NONBLOCK,width,height,fmt);
	 LOGE("amvideo_init ret=%d\n",ret);
    }
    return ret;
}
int amvideo_start(amvideo_dev_t *dev)
{
    if (dev->ops.start) {
        return dev->ops.start(dev);
    }
    return 0;
}
int amvideo_stop(amvideo_dev_t *dev)
{
    if (dev->ops.stop) {
        return dev->ops.stop(dev);
    }
    return 0;
}
int amvideo_release(amvideo_dev_t *dev)
{
    if (dev->mode == FLAGS_V4L_MODE) {
        amlv4l_release(dev);
    }
    return 0;
}
int amlv4l_dequeuebuf(amvideo_dev_t *dev, vframebuf_t*vf)
{
    if (dev->ops.dequeuebuf) {
        return dev->ops.dequeuebuf(dev, vf);
    }
    return -1;
}
int amlv4l_queuebuf(amvideo_dev_t *dev, vframebuf_t*vf)
{
    if (dev->ops.queuebuf) {
        return dev->ops.queuebuf(dev, vf);
    }
    return 0;
}
