#ifndef AMLV4L_HEAD_A__
#define AMLV4L_HEAD_A__
#include <amvideo.h>

typedef struct amlv4l_dev {
    int v4l_fd;
    unsigned int bufferNum;
    vframebuf_t *vframe;
    int type;
    int width;
    int height;
    int pixformat;
    int memory_mode;	
} amlv4l_dev_t;
amvideo_dev_t *new_amlv4l(void);
int amlv4l_release(amvideo_dev_t *dev);

#endif//AMLV4L_HEAD_A__