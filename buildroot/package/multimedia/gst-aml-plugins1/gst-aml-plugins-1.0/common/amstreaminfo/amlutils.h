/*
 * amlutils.h
 *
 *  Created on: 2016年1月8日
 *      Author: Tao.Guo
 */

#ifndef EXTERNAL_GSTREAMER_SOURCE_AML_PLUGINS_COMMON_AMSTREAMINFO_AMLUTILS_H_
#define EXTERNAL_GSTREAMER_SOURCE_AML_PLUGINS_COMMON_AMSTREAMINFO_AMLUTILS_H_
#include <gst/gst.h>

#define AML_DUMP_BUFFER(x, name) { \
	aml_dump_buffer(x, name, __FUNCTION__, __LINE__); \
}

#define DEBUG_DUMP 0

void _gst_debug_dump_mem2 (GstDebugCategory * cat, const gchar * file,
    const gchar * func, gint line, GObject * obj, const gchar * msg,
    const guint8 * data, guint length);
void aml_dump_buffer(GstBuffer *x, gchar *name, gchar *f, guint l);
void aml_dump_structure(const GstStructure * structure);
void aml_dump_caps(const GstCaps * caps);

#endif /* EXTERNAL_GSTREAMER_SOURCE_AML_PLUGINS_COMMON_AMSTREAMINFO_AMLUTILS_H_ */
