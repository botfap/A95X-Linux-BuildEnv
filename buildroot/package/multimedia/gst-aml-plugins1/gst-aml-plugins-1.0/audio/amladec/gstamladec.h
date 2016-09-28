/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 U-AMLOGICTao.Guo <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_AMLADEC_H__
#define __GST_AMLADEC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudiodecoder.h>
//#include <player.h>
#include <amlaudioinfo.h>
#include <gstamlsysctl.h>
#include  "../../common/include/codec.h"

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_AMLADEC \
  (gst_aml_adec_get_type())
#define GST_AMLADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AMLADEC,GstAmlAdec))
#define GST_AMLADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AMLADEC,GstAmlAdecClass))
  #define GST_AMLADEC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_AMLADEC,GstAmlAdecClass)) 
#define GST_IS_AMLADEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AMLADEC))
#define GST_IS_AMLADEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AMLADEC))

typedef struct _GstAmlAdec      GstAmlAdec;
typedef struct _GstAmlAdecClass GstAmlAdecClass;

struct _GstAmlAdec
{
	GstAudioDecoder element;

	gboolean silent;
	guint codec_init_ok;
//	guint codec_data_len;                    // Header Extension obtained through caps negotiation
//	GstBuffer* codec_data;                    // Header data needed for audio
	gboolean is_headerfeed; /* flag for decoder initialization */
	AmlStreamInfo *info;
	gint sample_rate;                    ///< audio stream sample rate
	gint channels;                    ///< audio stream channels
	gint bitrate;                    ///< audio stream bit rate
//	gint codec_id;                    ///< codec format id
//	gint block_align;                    ///< audio block align
	gboolean is_paused;
	gboolean is_eos;
	GstTask * eos_task;
    GStaticRecMutex eos_lock;
//
////	AmlState eState;
	codec_para_t *pcodec;
//	GstSegment segment;
//	gboolean passthrough;
//	GstCaps *tmpcaps;
//	GstTask * eos_task;
//	GStaticRecMutex eos_lock;
//
//	guint order;
//	gboolean bpass;
//	/*for elapsed*/
//	gint64 basepcr;
//
//	/*private for ape format*/
////	gst_ape_parser *apeparser;
	gboolean is_ape;
//	gint64 duration;
//	guint64 filesize;
	gboolean adecomit;
     gboolean passthrough;

};

struct _GstAmlAdecClass 
{
	GstAudioDecoderClass parent_class;
};

struct AmlControl
{
  GstCaps *firstcaps;
  gboolean passthrough;
  guint adecnumber;
};

GType gst_aml_adec_get_type (void);

G_END_DECLS

#endif /* __GST_AMLADEC_H__ */
