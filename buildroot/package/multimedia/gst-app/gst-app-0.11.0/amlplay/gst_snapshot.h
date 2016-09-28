/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 */
 
/*
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

#ifndef __GST_SNAPSHOT_H__
#define __GST_SNAPSHOT_H__

GstBuffer* gst_frame_convert(GstBuffer* buf, GstCaps* to_caps);
gboolean gst_save_bmp( char* gst_buffer, int width, int height, int red_mask, int green_mask, int blue_mask, char* filename );

#endif /* __GST_SNAPSHOT_H__ */

