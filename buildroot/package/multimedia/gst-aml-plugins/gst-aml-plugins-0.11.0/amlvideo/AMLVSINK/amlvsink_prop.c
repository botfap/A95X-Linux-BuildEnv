#include "gstamlvsink.h"
#include "amlvsink_prop.h"
#include "rectangleInfo.h"

#define MAX_BIT 5
gint parse_str2int(const gchar *input, const gchar boundary, const gchar end, gint *first, ...)
{
    va_list var_args;
    gint *name = NULL;
    const gchar *p = input;
    gint i = 0;
    gint index = 0;
    gint size = 0;
    gchar value[MAX_BIT]= {0};

    if(NULL == p){
        GST_WARNING("[%s:%d] null\n", __FUNCTION__, __LINE__);
        return -1;
    }
    size = strlen(p);
    va_start (var_args, first);
    name = first;
    while(name && (i <= size)){
        value[index] = p[i];
        if(index++ >= MAX_BIT){
            va_end (var_args);
             GST_ERROR("[%s:%d] error\n", __FUNCTION__, __LINE__);
            return -1;
        }
        if(p[i+1] == boundary){
            index = 0;
            
            *name = atoi(value);
            memset(value, 0, sizeof(value));
            i++;
            name = va_arg (var_args, gint*);
        }
        else if(p[i+1] == end){
            *name = atoi(value);
            break;
        }
        i++;
        
    }
    va_end (var_args);
    return 0;
}

/*video sink property*/
static void amlInstallPropSilent(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
}
static void amlInstallPropAsync(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("async", "Async", "Video synchronized with audio",
          TRUE, G_PARAM_READWRITE));
}
static void amlInstallPropTvMode(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_uint ("tvmode", "TvMode", "Define the television mode",
          0, 5, 0, G_PARAM_READWRITE));
}
static void amlInstallPropPlane(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("plane", "Plane", "Define the Pixel Plane to be used",
          -2000, 2000, 0, G_PARAM_READWRITE));
}
static void amlInstallPropRectangle(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_object ("rectangle", "Rectangle", "The destination rectangle",
          GST_TYPE_RECTANGLE_INFO, G_PARAM_READWRITE));
}
static void amlInstallPropFlushRepeatFrame(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("flushRepeatFrame", "FlushRepeatFrame", "Keep displaying the last frame rather than a black one whilst flushing",
          FALSE, G_PARAM_READWRITE));
}
static void amlInstallPropCurrentPTS(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int64 ("currentPTS", "CurrentPTS", "Display current PTS",
          0, 0x1FFFFFFF, 0, G_PARAM_READABLE));
}
static void amlInstallPropInterFrameDelay(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("frameDelay", "FrameDelay", "Enables fixed frame rate mode",
          FALSE, G_PARAM_READWRITE));
}
static void amlInstallPropSlowModeRate(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("slowModeRate", "SlowModeRate", "slow mode rate in normalised form",
          -20000, 20000, 0, G_PARAM_READWRITE));
}
static void amlInstallPropStepFrame(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("stepFrame", "StepFrame", "get content frame rate",
          -2, 2, 1, G_PARAM_READWRITE));
}

static void amlInstallPropMute(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("mute", "Mute", "Is video display?",
          FALSE, G_PARAM_READWRITE));
}

static int amlGetPropSilent(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_boolean (value, filter->bSilent);
    return 0;
}

static int amlGetPropAsync(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_boolean (value, filter->bAsync);
    
    return 0;
}

static int amlGetPropTvMode(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gchar data_array[64] = {0};
    guint mode = 0;
    get_sysfs_str("/sys/class/video/screen_mode", data_array, sizeof(data_array));
    data_array[63] = '\0';
    if(parse_str2int(data_array, ':', '\n', &mode, NULL) < 0){
        return -1;
    }
    
    g_value_set_uint (value, mode);
    return 0;
}
static int amlGetPropPlane(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_int (value, filter->plane);
    return 0;
}


static int amlGetPropRectangle(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gchar data_array[64] = {0};
    RectangleInfo *info_obj;
    gint x = 0;
    gint y = 0;
    gint width = 0;
    gint height = 0;

    get_sysfs_str("/sys/class/video/axis", data_array, sizeof(data_array));
    data_array[63] = '\0';

    if(parse_str2int(data_array, ' ', '\n', &x, &y, &width, &height) < 0){
        GST_ERROR("parse axis failed.\n");
        return -1;
    }
    info_obj = rectangle_info_new (x, y, width, height);

    g_value_take_object (value, info_obj);
    return 0;
}
static int amlGetPropFlushRepeatFrame(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_boolean (value, filter->bFlsRptFrm);
    g_value_set_boolean (value, get_black_policy());
    return 0;
}
static int amlGetPropCurrentPTS(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gint64 currentPTS = 0;
    int pts = get_sysfs_int("/sys/class/tsync/pts_video");
    //currentPTS = (gint64)codec_get_vpts(vpcodec);
    currentPTS = (gint64)pts;
    GST_WARNING("amlGetPropCurrentPTS pts=%d currentPTS=%lld\n", pts, currentPTS);
    g_value_set_int64 (value, currentPTS);
    return 0;
}
static int amlGetPropInterFrameDelay(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_boolean (value, amlsink->bInterFrmDly);
    return 0;
}
static int amlGetPropSlowModeRate(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_int (value, filter->slowModeRate);
    return 0;
}
static int amlGetPropStepFrame(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //g_value_set_int (value, filter->stepFrm);
    return 0;
}
static int amlSetPropSilent(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object);  
    gboolean bSilent = g_value_get_boolean (value);
    return 0;
}

static int amlSetPropAsync(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object);
    gboolean bAsync = FALSE;
    bAsync = g_value_get_boolean (value);
    gst_base_sink_set_async_enabled (GST_BASE_SINK(amlsink), bAsync);
    return 0;
}

static int amlSetPropTvMode(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    guint mode = g_value_get_uint(value);

    set_sysfs_int("/sys/class/video/screen_mode", mode);
    
    return 0;
}
static int amlSetPropPlane(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gint plane = g_value_get_int(value);
    return 0;
}
static int amlSetPropRectangle(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    RectangleInfo *info_obj = g_value_get_object(value);
    gchar data_array[64] = {0};

    sprintf(data_array, "%d %d %d %d", info_obj->x, info_obj->y, info_obj->width, info_obj->height);
    set_sysfs_str("/sys/class/video/axis", data_array);
    return 0;
}
static int amlSetPropFlushRepeatFrame(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    //amlsink->bFlsRptFrm = g_value_get_boolean(value);
    set_black_policy (g_value_get_boolean (value));
    return 0;
}

static int amlSetPropInterFrameDelay(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gboolean bInterFrmDly = g_value_get_boolean(value);
    return 0;
}
static int amlSetPropSlowModeRate(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gint slowModeRate = g_value_get_int(value);
    return 0;
}
static int amlSetPropStepFrame(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gint stepFrm = g_value_get_int(value);
    return 0;
}
static int amlSetPropMute(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlVsink *amlsink = GST_AMLVSINK (object); 
    gboolean mute = g_value_get_boolean(value);
    set_sysfs_int("/sys/class/video/disable_video", mute);
    return 0;
}

/*video render property install/get/set function pool*/
static const AmlPropType amlvsink_prop_pool[] = {
    {PROP_SILENT,                amlInstallPropSilent,                      amlGetPropSilent,                       amlSetPropSilent},
    {PROP_ASYNC ,                amlInstallPropAsync,                      amlGetPropAsync,                       amlSetPropAsync},
    {PROP_TVMODE,              amlInstallPropTvMode,                    amlGetPropTvMode,                    amlSetPropTvMode},
    {PROP_PLANE,                 amlInstallPropPlane,                       amlGetPropPlane,                       amlSetPropPlane},
    {PROP_RECTANGLE,         amlInstallPropRectangle,                 amlGetPropRectangle,                amlSetPropRectangle},
    {PROP_FLSH_RPT_FRM,    amlInstallPropFlushRepeatFrame,    amlGetPropFlushRepeatFrame,   amlSetPropFlushRepeatFrame},
    {PROP_CURT_PTS,           amlInstallPropCurrentPTS,               amlGetPropCurrentPTS,              NULL},
    {PROP_INTER_FRM_DELY, amlInstallPropInterFrameDelay,      amlGetPropInterFrameDelay,      amlSetPropInterFrameDelay},
    {PROP_SLOW_FRM_RATE, amlInstallPropSlowModeRate,          amlGetPropSlowModeRate,         amlSetPropSlowModeRate},
    {PROP_STEP_FRM,            amlInstallPropStepFrame,               amlGetPropStepFrame,                amlSetPropStepFrame},
    {PROP_MUTE,                   amlInstallPropMute,                        NULL,                                           amlSetPropMute},
    {-1,                                NULL,                                                NULL,                                           NULL},
};

AmlPropType *aml_get_vsink_prop_interface()
{
    return &amlvsink_prop_pool[0];
}


