/*
 * amlutils.c
 *
 *  Created on: 2016年1月8日
 *      Author: Tao.Guo
 */

#include "amlutils.h"

void aml_dump_mem(gchar *buf, guint len)
{
	guint off = 0;
	guint i;
	while (off < len) {
		gchar buf[128];
		for (i = 0; i < 16 && i < len - off; i++) {
			GST_INFO("%02x ", buf[off + i]);
		}
		GST_INFO("\n");
		off += 16;
  }
}
void aml_dump_buffer(GstBuffer *x, gchar *name, gchar *f, guint l)
{
	GstMapInfo map;
	GST_INFO("AML_DUMP_BUFFER:%s:%s:%d", name, f, l);
	gst_buffer_map(x, &map, GST_MAP_READ);
	aml_dump_mem(map.data, map.size);
	gst_buffer_unmap(x, &map);
}

static gboolean aml_dump_structure_cb(GQuark field_id, const GValue * value, gpointer user_data)
{
     GST_WARNING("%s", g_quark_to_string(field_id));

	return TRUE;
}

void aml_dump_structure(const GstStructure * structure)
{
	 GST_WARNING("DUMP structure");
	gst_structure_foreach(structure, aml_dump_structure_cb, NULL);
	GST_INFO("-----------------------------------------------%s end\n",__FUNCTION__);
}
void aml_dump_caps(const GstCaps * caps)
{
	GstStructure *structure;
	GST_INFO("DUMP caps");
	structure = gst_caps_get_structure(caps, 0);
	aml_dump_structure(structure);
}
