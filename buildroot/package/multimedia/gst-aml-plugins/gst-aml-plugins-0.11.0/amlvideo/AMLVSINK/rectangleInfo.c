
#include <gst/gst.h>

#include "rectangleInfo.h"

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_HEIGHT,
  PROP_WIDTH
};

static void rectangle_info_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void rectangle_info_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);

GST_DEBUG_CATEGORY_STATIC (rectangleinfo_debug);
#define GST_CAT_DEFAULT (rectangleinfo_debug)

GST_BOILERPLATE (RectangleInfo, rectangle_info, GObject, G_TYPE_OBJECT);

RectangleInfo *
rectangle_info_new (guint16 x, guint16 y, guint16 width, guint16 height)
{
  RectangleInfo *info;

  info = g_object_new (GST_TYPE_RECTANGLE_INFO, NULL);

  info->x = x;
  info->y = y;
  info->width = width;
  info->height = height;

  return info;
}

static void
rectangle_info_base_init (gpointer klass)
{
}

static void
rectangle_info_class_init (RectangleInfoClass * klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) klass;

  gobject_klass->set_property = rectangle_info_set_property;
  gobject_klass->get_property = rectangle_info_get_property;

  g_object_class_install_property (gobject_klass, PROP_X,
      g_param_spec_uint ("x", "Rectangle X",
          "Video sink should support scaling co-ordinate X", 0, G_MAXUINT16, 0,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, PROP_Y,
      g_param_spec_uint ("y", "Rectangle Y",
          "Video sink should support scaling co-ordinate Y", 0, G_MAXUINT16, 0,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, PROP_HEIGHT,
      g_param_spec_uint ("height", "Rectangle Height",
          "Video sink should support scaling co-ordinate Height", 0, G_MAXUINT16, 720,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, PROP_WIDTH,
      g_param_spec_uint ("width", "Rectangle Width",
          "Video sink should support scaling co-ordinate Width", 0, G_MAXUINT16, 1280,
          G_PARAM_READWRITE));
}

static void
rectangle_info_init (RectangleInfo * rectangle_info, RectangleInfoClass * klass)
{
}

static void
rectangle_info_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec)
{
    RectangleInfo *rectangle_info;
    guint v;
    g_return_if_fail (GST_IS_RECTANGLE_INFO (object));
    
    rectangle_info = GST_RECTANGLE_INFO (object);
    v = g_value_get_uint (value);
    switch (prop_id) {
    case PROP_X:
      rectangle_info->x = v;
      break;
    case PROP_Y:
      rectangle_info->y = v;
      break;
    case PROP_HEIGHT:
      rectangle_info->height = v;
      break;
    case PROP_WIDTH:
      rectangle_info->width = v;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
      break;
    }
}

static void
rectangle_info_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec)
{
    RectangleInfo *rectangle_info;

    g_return_if_fail (GST_IS_RECTANGLE_INFO (object));

    rectangle_info = GST_RECTANGLE_INFO (object);

    switch (prop_id) {
    case PROP_X:
      g_value_set_uint (value, rectangle_info->x);
      break;
    case PROP_Y:
      g_value_set_uint (value, rectangle_info->y);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, rectangle_info->height);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, rectangle_info->width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
      break;
    }
}

