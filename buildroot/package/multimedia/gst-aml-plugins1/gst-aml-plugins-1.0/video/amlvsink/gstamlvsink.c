/* GStreamer */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "gstamlvsink.h"

GST_DEBUG_CATEGORY_STATIC (gst_aml_vsink_debug);
#define GST_CAT_DEFAULT gst_aml_vsink_debug
#define VERSION	"1.1"

enum
{
  ARG_0,
};

#if 0
static void gst_aml_vsink_get_times (GstBaseSink * basesink,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
#endif

static GstFlowReturn gst_aml_vsink_show_frame (GstVideoSink * videosink, GstBuffer * buff);

static gboolean gst_aml_vsink_start (GstBaseSink * bsink);
static gboolean gst_aml_vsink_stop (GstBaseSink * bsink);

static GstCaps *gst_aml_vsink_getcaps (GstBaseSink * bsink, GstCaps * filter);
static gboolean gst_aml_vsink_setcaps (GstBaseSink * bsink, GstCaps * caps);

static void gst_aml_vsink_finalize (GObject * object);
static void gst_aml_vsink_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_aml_vsink_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_aml_vsink_change_state (GstElement * element, GstStateChange transition);

#define VIDEO_CAPS "{ I420 }"

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE(VIDEO_CAPS))
    );

#define parent_class gst_aml_vsink_parent_class
G_DEFINE_TYPE (GstAmlVsink, gst_aml_vsink, GST_TYPE_VIDEO_SINK);

static void
gst_aml_vsink_class_init (GstAmlVsinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *basesink_class;
  GstVideoSinkClass *videosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  basesink_class = (GstBaseSinkClass *) klass;
  videosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = gst_aml_vsink_set_property;
  gobject_class->get_property = gst_aml_vsink_get_property;
  gobject_class->finalize = gst_aml_vsink_finalize;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_aml_vsink_change_state);

  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_aml_vsink_setcaps);
  basesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_aml_vsink_getcaps);
  basesink_class->start = GST_DEBUG_FUNCPTR (gst_aml_vsink_start);
  basesink_class->stop = GST_DEBUG_FUNCPTR (gst_aml_vsink_stop);

  videosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_aml_vsink_show_frame);

  gst_element_class_set_static_metadata (gstelement_class,
		  "Amlogic Video Sink",
		  "Sink/Video",
		  "Amlogic videosink",
		  "mm@amlogic.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));
}

static void
gst_aml_vsink_init (GstAmlVsink * amlvsink)
{
	GstVideoSink *bsink;
	bsink = GST_VIDEO_SINK (amlvsink);
	gst_base_sink_set_sync(GST_BASE_SINK(amlvsink), FALSE);
	gst_base_sink_set_async_enabled(GST_BASE_SINK(amlvsink), FALSE);
	amlvsink->amvideo_dev = NULL;
	amlvsink->framerate_d = 1;
	amlvsink->framerate_n = 0;

	amlvsink->height = -1;
	amlvsink->align_width = -1;
	amlvsink->mIonFd = 0;
	amlvsink->mOutBuffer = NULL;
	amlvsink->use_yuvplayer = 0;
#if DEBUG_DUMP
	amlvsink->dump_fd = open("/tmp/gst_aml_vsink.dump", O_CREAT | O_TRUNC | O_WRONLY, 0777);
#endif
}

int AllocDmaBuffers(GstAmlVsink *amlvsink)
{
	struct ion_handle *ion_hnd;
	int shared_fd;
	int ret = 0;
	int buffer_size;
	buffer_size = amlvsink->align_width * amlvsink->height * 3 / 2;
	amlvsink->mIonFd = ion_open();
	if (amlvsink->mIonFd < 0) {
		GST_ERROR("ion open failed!\n");
		return -1;
	}
	int i = 0;
	while (i < OUT_BUFFER_COUNT) {
		unsigned int ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
		ret = ion_alloc(amlvsink->mIonFd, buffer_size, 0, ION_HEAP_CARVEOUT_MASK, ion_flags, &ion_hnd);
		if (ret) {
			GST_ERROR("ion alloc error");
			ion_close(amlvsink->mIonFd);
			return -1;
		}
		ret = ion_share(amlvsink->mIonFd, ion_hnd, &shared_fd);
		if (ret) {
			GST_ERROR("ion share error!\n");
			ion_free(amlvsink->mIonFd, ion_hnd);
			ion_close(amlvsink->mIonFd);
			return -1;
		}
		void *cpu_ptr = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
		if (MAP_FAILED == cpu_ptr) {
			GST_ERROR("ion mmap error!\n");
			ion_free(amlvsink->mIonFd, ion_hnd);
			ion_close(amlvsink->mIonFd);
			return -1;
		}

		GST_INFO("[%s %d] i:%d shared_fd:%d cpu_ptr:%x\n", __FUNCTION__, __LINE__, i, shared_fd, cpu_ptr);

		amlvsink->mOutBuffer[i].index = i;
		amlvsink->mOutBuffer[i].fd = shared_fd;
		amlvsink->mOutBuffer[i].pBuffer = NULL;
		amlvsink->mOutBuffer[i].own_by_v4l = -1;
		amlvsink->mOutBuffer[i].fd_ptr = cpu_ptr;
		amlvsink->mOutBuffer[i].ion_hnd = ion_hnd;
		i++;
	}
	return ret;
}

int FreeDmaBuffers(GstAmlVsink *amlvsink)
{
	int i = 0;
	int buffer_size;
	buffer_size = amlvsink->align_width * amlvsink->height * 3 / 2;
	while (i < OUT_BUFFER_COUNT) {
		munmap(amlvsink->mOutBuffer[i].fd_ptr, buffer_size);
		close(amlvsink->mOutBuffer[i].fd);
		GST_INFO("FreeDmaBuffers_mOutBuffer[i].fd=%d,mIonFd=%d\n", amlvsink->mOutBuffer[i].fd, amlvsink->mIonFd);
		ion_free(amlvsink->mIonFd, amlvsink->mOutBuffer[i].ion_hnd);
		i++;
	}
	int ret = ion_close(amlvsink->mIonFd);
	return ret;
}


static gboolean gst_aml_vsink_yuvplayer_init(GstAmlVsink *amlvsink)
{
	int ret, i;
	vframebuf_t vf;
	amsysfs_set_sysfs_str("/sys/class/vfm/map", "rm default");
	amsysfs_set_sysfs_str("/sys/class/vfm/map", "add default yuvplayer amvideo");
	amsysfs_set_sysfs_str("/sys/class/graphics/fb0/blank", "1");
	amsysfs_set_sysfs_str("/sys/class/graphics/fb1/blank", "1");

	amlvsink->mOutBuffer = (out_buffer_t *) malloc(sizeof(out_buffer_t) * OUT_BUFFER_COUNT);
	memset(amlvsink->mOutBuffer, 0, sizeof(out_buffer_t) * OUT_BUFFER_COUNT);
	AllocDmaBuffers(amlvsink);
	amlvsink->amvideo_dev = new_amvideo(FLAGS_V4L_MODE);
	amlvsink->amvideo_dev->display_mode = 0;

	ret = amvideo_init(amlvsink->amvideo_dev, 0, amlvsink->align_width, amlvsink->height, V4L2_PIX_FMT_YUV420,
			OUT_BUFFER_COUNT);
	if (ret < 0) {
		GST_ERROR("amvideo_init failed =%d\n", ret);
		amvideo_release(amlvsink->amvideo_dev);
		amlvsink->amvideo_dev = NULL;
		return -__LINE__;
	}
	i = 0;
	while (i < OUT_BUFFER_COUNT) {
		vf.index = amlvsink->mOutBuffer[i].index;
		vf.fd = amlvsink->mOutBuffer[i].fd;
		vf.length = amlvsink->align_width * amlvsink->height * 3 / 2;

		memset(amlvsink->mOutBuffer[i].fd_ptr, 0x0, amlvsink->align_width * amlvsink->height);
		memset(amlvsink->mOutBuffer[i].fd_ptr + amlvsink->align_width * amlvsink->height, 0x80,
				amlvsink->align_width * amlvsink->height / 2);

		int ret = amlv4l_queuebuf(amlvsink->amvideo_dev, &vf);
		if (ret < 0) {
			GST_ERROR("amlv4l_queuebuf failed =%d\n", ret);
		}

		if (i == 1) {
			ret = amvideo_start(amlvsink->amvideo_dev);
			GST_INFO("amvideo_start ret=%d\n", ret);
			if (ret < 0) {
				GST_ERROR("amvideo_start failed =%d\n", ret);
				amvideo_release(amlvsink->amvideo_dev);
				amlvsink->amvideo_dev = NULL;
			}
		}
		i++;
	}
	amlvsink->use_yuvplayer = 1;
	return TRUE;
}

static gboolean gst_aml_vsink_yuvplayer_deinit(GstAmlVsink *amlvsink)
{
	vframebuf_t vf;
	int i;
	for (i = 0; i < OUT_BUFFER_COUNT; i++) {
		amlv4l_dequeuebuf(amlvsink->amvideo_dev, &vf);
	}

	if (amlvsink->amvideo_dev) {
		amvideo_stop(amlvsink->amvideo_dev);
		amvideo_release(amlvsink->amvideo_dev);
		amlvsink->amvideo_dev = NULL;
	}
	amsysfs_set_sysfs_str("/sys/class/video/disable_video", "2");
	amsysfs_set_sysfs_str("/sys/class/graphics/fb0/blank", "0");
	amsysfs_set_sysfs_str("/sys/class/graphics/fb1/blank", "0");
	amsysfs_set_sysfs_str("/sys/class/vfm/map", "rm default");
	amsysfs_set_sysfs_str("/sys/class/vfm/map", "add default decoder ppmgr deinterlace amvideo");

	FreeDmaBuffers(amlvsink);
	amlvsink->use_yuvplayer = 0;
	if (amlvsink->mOutBuffer)
		free(amlvsink->mOutBuffer);
	return TRUE;
}
static void
gst_aml_vsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAmlVsink *amlvsink;

  amlvsink = GST_AMLVSINK (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_aml_vsink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstAmlVsink *amlvsink;

  amlvsink = GST_AMLVSINK (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_aml_vsink_finalize (GObject * object)
{
  GstAmlVsink *amlvsink = GST_AMLVSINK (object);
  GST_DEBUG_OBJECT(amlvsink, "%s", __FUNCTION__);
  G_OBJECT_CLASS (parent_class)->finalize (object);
  if (amlvsink->use_yuvplayer) {
	  gst_aml_vsink_yuvplayer_deinit(amlvsink);
  }
}

static GstStateChangeReturn
gst_aml_vsink_change_state (GstElement * element, GstStateChange transition)
{
    GstAmlVsink *amlvsink= GST_AMLVSINK (element);
    GstStateChangeReturn result;
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
                //prepared function
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        default:
            break;
    }

    result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  
    switch (transition) {
	  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
	  	 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);

	      break;
        case GST_STATE_CHANGE_PAUSED_TO_READY: 
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:  
			 GST_ERROR("%s,%d\n",__FUNCTION__,__LINE__);
            break;
        default:
            break;
    }
	GST_ERROR("%s,%d,result=%d\n",__FUNCTION__,__LINE__,result);
  return result;
}

static gboolean
gst_aml_vsink_setcaps (GstBaseSink * bsink, GstCaps * vscapslist)
{
  GstAmlVsink *amlvsink;
  GstStructure *structure;
  const GValue *fps;

  amlvsink = GST_AMLVSINK (bsink);
  GST_DEBUG_OBJECT(amlvsink, "%s", __FUNCTION__);
  structure = gst_caps_get_structure (vscapslist, 0);
  gst_structure_get_int(structure, "width", &amlvsink->width);
  gst_structure_get_int(structure, "height", &amlvsink->height);
  gst_structure_get_fraction(structure, "framerate", &amlvsink->framerate_n, &amlvsink->framerate_d);
  amlvsink->align_width = (amlvsink->width + 63) & (~63);
  amlvsink->yuv_width = (amlvsink->width + 15) & (~15);
  return TRUE;
}

static GstCaps *
gst_aml_vsink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstAmlVsink *amlvsink;
  GstVideoFormat format;
  GstCaps *caps;

  amlvsink = GST_AMLVSINK (bsink);
  GST_DEBUG_OBJECT(amlvsink, "%s", __FUNCTION__);
  caps = gst_static_pad_template_get_caps (&sink_template);


  format = GST_VIDEO_FORMAT_I420;
  caps = gst_caps_make_writable (caps);
  gst_caps_set_simple (caps, "format", G_TYPE_STRING,
      gst_video_format_to_string (format), NULL);

done:

  if (filter != NULL) {
    GstCaps *icaps;

    icaps = gst_caps_intersect (caps, filter);
    gst_caps_unref (caps);
    caps = icaps;
  }
  return caps;
}

static gboolean
gst_aml_vsink_start (GstBaseSink * bsink)
{
  GstAmlVsink *amlvsink;

  amlvsink = GST_AMLVSINK (bsink);
  GST_DEBUG_OBJECT(amlvsink, "%s", __FUNCTION__);
  return TRUE;
}

static gboolean
gst_aml_vsink_stop (GstBaseSink * bsink)
{
  GstAmlVsink *amlvsink;

  amlvsink = GST_AMLVSINK (bsink);
  GST_DEBUG_OBJECT(amlvsink, "%s", __FUNCTION__);
  return TRUE;
}

static GstFlowReturn
gst_aml_vsink_show_frame (GstVideoSink * videosink, GstBuffer * buf)
{

	GstAmlVsink *amlvsink;
	vframebuf_t vf;
	GstMapInfo map;
	int ret, retry = 0;
	void *cpu_ptr = NULL;
	amlvsink = GST_AMLVSINK(videosink);
	GST_DEBUG_OBJECT(amlvsink, "%s %llu", __FUNCTION__, GST_BUFFER_TIMESTAMP (buf));
		
	if(GST_BUFFER_FLAG_IS_SET(buf,(1<<16))){
         ;//g_print("AMDEC FLAG SET\n");	
	}else if(amlvsink->use_yuvplayer == 0) {
	    gst_aml_vsink_yuvplayer_init(amlvsink);
		g_print("yuvplayer\n");	
	}
	if (amlvsink->use_yuvplayer) {
		gst_buffer_map(buf, &map, GST_MAP_READ);
		while ((ret = amlv4l_dequeuebuf(amlvsink->amvideo_dev, &vf) ) < 0 && retry < 5) {
			//wait amlv4l ready
			usleep(10000);
			retry++;
		}
		if (ret >= 0) {
			int j, i = 0;
			while (i < OUT_BUFFER_COUNT) {
				if (vf.fd == amlvsink->mOutBuffer[i].fd) {
					cpu_ptr = amlvsink->mOutBuffer[i].fd_ptr;
					break;
				}
				i++;
			}
			//		output_frame_count++;

			vf.index = amlvsink->mOutBuffer[i].index;
			vf.fd = amlvsink->mOutBuffer[i].fd;
			vf.length = amlvsink->align_width * amlvsink->height * 3 / 2;
			if (GST_BUFFER_PTS_IS_VALID(buf))
		     vf.pts = GST_BUFFER_PTS(buf) * 9LL / 100000LL + 1L;
		     else if (GST_BUFFER_DTS_IS_VALID(buf))		
			vf.pts = GST_BUFFER_DTS(buf) * 9LL / 100000LL + 1L;
			vf.width = amlvsink->align_width;
			vf.height = amlvsink->height;
			GST_INFO_OBJECT(amlvsink, " yuv pts = %x", (unsigned long) vf.pts);
			for (j = 0; j < amlvsink->height; j++) {
				memcpy(cpu_ptr + amlvsink->align_width * j, map.data + j * amlvsink->width, amlvsink->width);
				memset(cpu_ptr + amlvsink->align_width * j + amlvsink->width, 0,
						amlvsink->align_width - amlvsink->width);
			}

			for (j = 0; j < amlvsink->height / 2; j++) {
				memcpy(cpu_ptr + amlvsink->align_width * amlvsink->height + amlvsink->align_width * j / 2,
						map.data + amlvsink->width * amlvsink->height + j * amlvsink->yuv_width / 2,
						amlvsink->yuv_width / 2);
				memset(
						cpu_ptr + amlvsink->align_width * amlvsink->height + amlvsink->align_width * j / 2
								+ amlvsink->width / 2, 0x80, (amlvsink->align_width - amlvsink->width) / 2);
			}

			for (j = 0; j < amlvsink->height / 2; j++) {
				memcpy(
						cpu_ptr + amlvsink->align_width * amlvsink->height
								+ amlvsink->align_width * amlvsink->height / 4 + amlvsink->align_width * j / 2,
						map.data + amlvsink->width * amlvsink->height + amlvsink->yuv_width * amlvsink->height / 4
								+ j * amlvsink->yuv_width / 2, amlvsink->width / 2);
				memset(
						cpu_ptr + amlvsink->align_width * amlvsink->height
								+ amlvsink->align_width * amlvsink->height / 4 + amlvsink->align_width * j / 2
								+ amlvsink->width / 2, 0x80, (amlvsink->align_width - amlvsink->width) / 2);
			}

#if DEBUG_DUMP
			if (amlvsink->dump_fd > 0) {
				write(amlvsink->dump_fd, cpu_ptr, amlvsink->align_width * amlvsink->height * 3 / 2);
			}
#endif

			ret = amlv4l_queuebuf(amlvsink->amvideo_dev, &vf);
			if (ret < 0) {
				GST_ERROR("amlv4l_queuebuf failed =%d\n", ret);
			}
		} else {
			GST_ERROR("skip frame");
		}
		gst_buffer_unmap(buf, &map);
	}
	
	return GST_FLOW_OK;
}

static gboolean
amlvsink_init (GstPlugin * plugin)
{
	GST_DEBUG_CATEGORY_INIT(gst_aml_vsink_debug, "amlvsink", 0, "Amlogic Video Sink");
	return gst_element_register(plugin, "amlvsink", GST_RANK_SECONDARY, GST_TYPE_AMLVSINK);
}


#ifndef PACKAGE
#define PACKAGE "gst-plugins-amlogic"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    amlvsink,
    "Amlogic Video Sink",
    amlvsink_init,
	VERSION,
	"LGPL",
	"Amlogic",
	"http://amlogic.com/"
)
