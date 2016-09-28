/* GStreamer */


#ifndef __GST_GSTAMLVSINK_H__
#define __GST_GSTAMLVSINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include "ion_dev.h"
#include "ion.h"
#include "amvideo.h"


G_BEGIN_DECLS

#define OUT_BUFFER_COUNT   4
#define AMLDEC_FLAG (1<<16)

#define GST_TYPE_AMLVSINK \
  (gst_aml_vsink_get_type())
#define GST_AMLVSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AMLVSINK,GstAmlVsink))
#define GST_AMLVSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AMLVSINK,GstAmlVsinkClass))
#define GST_AMLVSINK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_AMLVSINK,GstAmlVsinkClass))  
#define GST_IS_AMLVSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AMLVSINK))
#define GST_IS_AMLVSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AMLVSINK))

typedef struct _GstAmlVsink GstAmlVsink;
typedef struct _GstAmlVsinkClass GstAmlVsinkClass;

typedef struct {
    int index;
    int fd;
    void * pBuffer;
    int own_by_v4l;
    void *fd_ptr; //only for non-nativebuffer!
    struct ion_handle *ion_hnd; //only for non-nativebuffer!
}out_buffer_t;

struct _GstAmlVsink {
  GstVideoSink videosink;
  struct amvideo_dev *amvideo_dev;
  out_buffer_t * mOutBuffer;
  int align_width, yuv_width, width, height;
  int framerate_n, framerate_d;
  int mIonFd;
  int use_yuvplayer;
#if DEBUG_DUMP
  int dump_fd;
#endif

};

struct _GstAmlVsinkClass {
  GstVideoSinkClass videosink_class;
};

GType gst_aml_vsink_get_type(void);

G_END_DECLS

#endif /* __GST_GSTAMLVSINK_H__ */
