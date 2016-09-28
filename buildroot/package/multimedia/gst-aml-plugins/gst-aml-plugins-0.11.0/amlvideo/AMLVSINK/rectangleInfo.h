
#ifndef __RECTANGLE_INFO_H__
#define __RECTANGLE_INFO_H__

#include <glib.h>

G_BEGIN_DECLS typedef struct RectangleInfoClass
{
    GObjectClass parent_class;
} RectangleInfoClass;

typedef struct RectangleInfo
{
    GObject parent;

    guint16 x;
    guint16 y;
    guint16 width;
    guint16 height;
} RectangleInfo;

#define GST_TYPE_RECTANGLE_INFO (rectangle_info_get_type ())
#define GST_IS_RECTANGLE_INFO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RECTANGLE_INFO))
#define GST_RECTANGLE_INFO(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RECTANGLE_INFO, RectangleInfo))

GType rectangle_info_get_type (void);

RectangleInfo *rectangle_info_new (guint16 x, guint16 y, guint16 width , guint16 height);

G_END_DECLS
#endif

