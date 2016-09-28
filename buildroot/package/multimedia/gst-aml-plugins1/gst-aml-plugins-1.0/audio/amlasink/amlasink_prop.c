
#include "gstamlasink.h"
#include "amlasink_prop.h"

static void amlInstallPropVolume(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_double ("volume", "Volume", "alsa master playback volume",
          0, 1.0, 0.5, G_PARAM_READWRITE));
}

static void amlInstallPropMute(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_boolean ("mute", "Mute", "playback mute",
          FALSE, G_PARAM_READWRITE));
}

static void amlInstallPropAOutHdmi(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("audio-output-hdmi", "Audio-Output-Hdmi", "Define the audio output hdmi mode",
          0, 3, 1, G_PARAM_READWRITE));
}

static void amlInstallPropAOutSpdif(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("audio-output-spdif", "Audio-Output-Spdif", "Define the audio spdif mode",
          0, 3, 1, G_PARAM_READWRITE));
}

static void amlInstallPropAOutOther(GObjectClass *oclass,
    guint property_id)
{
    GObjectClass*gobject_class = (GObjectClass *) oclass;
    g_object_class_install_property (gobject_class, property_id, g_param_spec_int ("audio-output-other", "Audio-Output-Other", "Define any other audio mode",
          0, 3, 1, G_PARAM_READWRITE));
}

static int amlGetPropVolume(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    int val;
    dummy_codec_get_volume(&val);
    g_value_set_double(value,(gdouble)val/100);
    return 0;
}

static int amlGetPropMute(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    //g_value_set_boolean (value, filter->bSilent);
    return 0;
}

static int amlGetPropAOutHdmi(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    //g_value_set_boolean (value, filter->bSilent);
    return 0;
}

static int amlGetPropAOutSpdif(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    //g_value_set_boolean (value, filter->bSilent);
    return 0;
}

static int amlGetPropAOutOther(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    //g_value_set_boolean (value, filter->bSilent);
    return 0;
}

static int amlSetPropVolume(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object);  
    gint temp;
    temp = (gint)(g_value_get_double (value)*100);
    GST_WARNING("asink vol=%d\n",temp);
    dummy_codec_set_volume(temp);
    return 0;
}

static int amlSetPropMute(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object); 
    gint temp;
    if(g_value_get_boolean(value))
        temp = 1;
    else temp = 0;
    dummy_codec_set_mute(temp);
    return 0;
}

static int amlSetPropAOutHdmi(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object);  
    gint mode = g_value_get_int (value);
    return 0;
}

static int amlSetPropAOutSpdif(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object);  
    gint mode = g_value_get_int (value);
    return 0;
}

static int amlSetPropAOutOther(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAmlAsink *amlsink = GST_AMLASINK (object);  
    gint mode = g_value_get_int (value);
    return 0;
}


static const AmlPropType amlasink_prop_pool[] = {
    {PROP_VOLUME,         amlInstallPropVolume,     amlGetPropVolume,      amlSetPropVolume},
    {PROP_MUTE,             amlInstallPropMute,         amlGetPropMute,          amlSetPropMute},
    {PROP_AOUT_HDMI,    amlInstallPropAOutHdmi, amlGetPropAOutHdmi,  amlSetPropAOutHdmi},
    {PROP_AOUT_SPDIF,   amlInstallPropAOutSpdif,  amlGetPropAOutSpdif,  amlSetPropAOutSpdif},
    {PROP_AOUT_OTHER,  amlInstallPropAOutOther, amlGetPropAOutOther, amlSetPropAOutOther},
    {-1,                           NULL,                                 NULL,                            NULL}
};

AmlPropType *aml_get_asink_prop_interface()
{
    return &amlasink_prop_pool[0];
}

