/*
 * AMLOGIC Audio/Video streaming port driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef VFRAME_H
#define VFRAME_H

#include <linux/types.h>


#define VIDTYPE_PROGRESSIVE             0x0
#define VIDTYPE_INTERLACE_TOP           0x1
#define VIDTYPE_INTERLACE_BOTTOM        0x3
#define VIDTYPE_TYPEMASK                0x7
#define VIDTYPE_INTERLACE               0x1
#define VIDTYPE_INTERLACE_FIRST         0x8
#define VIDTYPE_PULLDOWN                0x10
#define VIDTYPE_VIU_422                 0x800
#define VIDTYPE_VIU_FIELD               0x1000
#define VIDTYPE_VIU_SINGLE_PLANE        0x2000
#define VIDTYPE_VIU_444                 0x4000
#define VIDTYPE_CANVAS_TOGGLE           0x8000

#define DISP_RATIO_FORCECONFIG          0x80000000
#define DISP_RATIO_CTRL_MASK            0x00000003
#define DISP_RATIO_NO_KEEPRATIO         0x00000000
#define DISP_RATIO_KEEPRATIO            0x00000001

#define DISP_RATIO_ASPECT_RATIO_MASK    0x0003ff00
#define DISP_RATIO_ASPECT_RATIO_BIT     8
#define DISP_RATIO_ASPECT_RATIO_MAX     0x3ff




/*
 * If pixel_sum[21:0] == 0, then all Histogram information are invalid
 */
typedef struct vframe_hist_s
{
    unsigned int   luma_sum;
    unsigned int   chroma_sum;
    unsigned int   pixel_sum;  // [31:30] POW [21:0] PIXEL_SUM
    unsigned char  luma_max;
    unsigned char  luma_min;
    unsigned short gamma[64];
} vframe_hist_t;


/*
 * If bottom == 0 or right == 0, then all Blackbar information are invalid
 */
typedef struct vframe_bbar_s
{
    unsigned short top;
    unsigned short bottom;
    unsigned short left;
    unsigned short right;
} vfame_bbar_t;


/*
 * If vsin == 0, then all Measurement information are invalid
 */
typedef struct vframe_meas_s
{
    //float          frin;      // Frame Rate of Video Input in the unit of Hz
    unsigned int        vs_span_cnt;
    unsigned long long  vs_cnt;
    unsigned int        hs_cnt0;
    unsigned int        hs_cnt1;
    unsigned int        hs_cnt2;
    unsigned int        hs_cnt3;
} vframe_meas_t;


/* vframe properties */
typedef struct vframe_prop_s
{
    struct vframe_hist_s hist;
    struct vframe_bbar_s bbar;
    struct vframe_meas_s meas;
} vframe_prop_t;

typedef struct vframe_s {
    u32 index;
    u32 type;
    u32 type_backup;
    u32 blend_mode;
    u32 duration;
    u32 duration_pulldown;
    u32 pts;

    u32 canvas0Addr;
    u32 canvas1Addr;

    u32 bufWidth;
    u32 width;
    u32 height;
    u32 ratio_control;

#if 1
    /* vframe properties */
    struct vframe_prop_s prop;
#endif
} vframe_t;

#if 0
struct vframe_prop_s * vdin_get_vframe_prop(u32 index);
#endif

#endif /* VFRAME_H */

