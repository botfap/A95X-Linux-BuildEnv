/*
 * HEVC video Decoder (just used for hevc parser by senbai.tao)
 *
 * Copyright (C) 2012 - 2013 Guillaume Martres
 * Copyright (C) 2012 - 2013 Mickael Raulet
 * Copyright (C) 2012 - 2013 Gildas Cocherel
 * Copyright (C) 2012 - 2013 Wassim Hamidouche
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "hevc.h"
#include "golomb.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include <pthread.h>

static pthread_mutex_t atomic_lock = PTHREAD_MUTEX_INITIALIZER;

//////////// copy from player_hwdec.c ///////////////////////
int check_size_in_buffer(unsigned char *p, int len)
{
    unsigned int size;
    unsigned char *q = p;
    while ((q + 4) < (p + len)) {
        size = (*q << 24) | (*(q + 1) << 16) | (*(q + 2) << 8) | (*(q + 3));
        if (size & 0xff000000) {
            return 0;
        }

        if (q + size + 4 == p + len) {
            return 1;
        }

        q += size + 4;
    }
    return 0;
}

int check_size_in_buffer3(unsigned char *p, int len)
{
    unsigned int size;
    unsigned char *q = p;
    while ((q + 3) < (p + len)) {
        size = (*q << 16) | (*(q + 1) << 8) | (*(q + 2));

        if (q + size + 3 == p + len) {
            return 1;
        }

        q += size + 3;
    }
    return 0;
}
////////////////////////////////////////////////

 int ff_hevc_extract_rbsp(HEVCContext *s, const uint8_t *src, int length,
                         HEVCNAL *nal)
{
    int i, si, di;
    uint8_t *dst;

    s->skipped_bytes = 0;
#define STARTCODE_TEST                                                  \
        if (i + 2 < length && src[i + 1] == 0 && src[i + 2] <= 3) {     \
            if (src[i + 2] != 3) {                                      \
                /* startcode, so we must be past the end */             \
                length = i;                                             \
            }                                                           \
            break;                                                      \
        }
#if HAVE_FAST_UNALIGNED
#define FIND_FIRST_ZERO                                                 \
        if (i > 0 && !src[i])                                           \
            i--;                                                        \
        while (src[i])                                                  \
            i++
#if HAVE_FAST_64BIT
    for (i = 0; i + 1 < length; i += 9) {
        if (!((~AV_RN64A(src + i) &
               (AV_RN64A(src + i) - 0x0100010001000101ULL)) &
              0x8000800080008080ULL))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 7;
    }
#else
    for (i = 0; i + 1 < length; i += 5) {
        if (!((~AV_RN32A(src + i) &
               (AV_RN32A(src + i) - 0x01000101U)) &
              0x80008080U))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 3;
    }
#endif
#else
    for (i = 0; i + 1 < length; i += 2) {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
#endif

    if (i >= length - 1) { // no escaped 0
        nal->data = src;
        nal->size = length;
        return length;
    }

    av_fast_malloc(&nal->rbsp_buffer, &nal->rbsp_buffer_size,
                   length + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!nal->rbsp_buffer)
        return AVERROR(ENOMEM);

    dst = nal->rbsp_buffer;

    memcpy(dst, src, i);
    si = di = i;
    while (si + 2 < length) {
        // remove escapes (very rare 1:2^22)
        if (src[si + 2] > 3) {
            dst[di++] = src[si++];
            dst[di++] = src[si++];
        } else if (src[si] == 0 && src[si + 1] == 0) {
            if (src[si + 2] == 3) { // escape
                dst[di++] = 0;
                dst[di++] = 0;
                si       += 3;

#if 0
                s->skipped_bytes++;
                if (s->skipped_bytes_pos_size < s->skipped_bytes) {
                    s->skipped_bytes_pos_size *= 2;
                    av_reallocp_array(&s->skipped_bytes_pos,
                            s->skipped_bytes_pos_size,
                            sizeof(*s->skipped_bytes_pos));
                    if (!s->skipped_bytes_pos)
                        return AVERROR(ENOMEM);
                }
                if (s->skipped_bytes_pos)
                    s->skipped_bytes_pos[s->skipped_bytes-1] = di - 1;
#endif
                continue;
            } else // next start code
                goto nsc;
        }

        dst[di++] = src[si++];
    }
    while (si < length)
        dst[di++] = src[si++];
nsc:

    memset(dst + di, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    nal->data = dst;
    nal->size = di;
    return si;
}


int ff_hevc_compute_poc(HEVCContext *s, int poc_lsb)
{
    int max_poc_lsb  = 1 << s->sps->log2_max_poc_lsb;
    int prev_poc_lsb = s->pocTid0 % max_poc_lsb;
    int prev_poc_msb = s->pocTid0 - prev_poc_lsb;
    int poc_msb;

    if ((poc_lsb < prev_poc_lsb) && ((prev_poc_lsb - poc_lsb) >= max_poc_lsb / 2))
        poc_msb = prev_poc_msb + max_poc_lsb;
    else if ((poc_lsb > prev_poc_lsb) && ((poc_lsb - prev_poc_lsb) > (max_poc_lsb / 2)))
        poc_msb = prev_poc_msb - max_poc_lsb;
    else
        poc_msb = prev_poc_msb;

    // For BLA picture types, POCmsb is set to 0.
    if (s->nal_unit_type == NAL_BLA_W_LP ||
        s->nal_unit_type == NAL_BLA_W_RADL ||
        s->nal_unit_type == NAL_BLA_N_LP)
        poc_msb = 0;

    return poc_msb + poc_lsb;
}

const uint8_t *avpriv_find_start_code(const uint8_t * p,const uint8_t *end,uint32_t * state)
{
    int i;

    if (p >= end)
        return end;

    for (i = 0; i < 3; i++) {
        uint32_t tmp = *state << 8;
        *state = tmp + *(p++);
        if (tmp == 0x100 || p == end)
            return p;
    }

    while (p < end) {
        if      (p[-1] > 1      ) p += 3;
        else if (p[-2]          ) p += 2;
        else if (p[-3]|(p[-1]-1)) p++;
        else {
            p++;
            break;
        }
    }

    p = FFMIN(p, end) - 4;
    *state = AV_RB32(p);

    return p + 4;
}

static int avpriv_atomic_int_add_and_fetch(volatile int *ptr, int inc)
{
    int res;

    pthread_mutex_lock(&atomic_lock);
    *ptr += inc;
    res = *ptr;
    pthread_mutex_unlock(&atomic_lock);

    return res;
}

void av_buffer_unref(AVBufferRef **buf)
{
    AVBuffer *b;

    if (!buf || !*buf)
        return;
    b = (*buf)->buffer;
    av_freep(buf);

    if (!avpriv_atomic_int_add_and_fetch(&b->refcount, -1)) {
        b->free(b->opaque, b->data);
        av_freep(&b);
    }
}

/**
 * Always treat the buffer as read-only, even when it has only one
 * reference.
 */
#define AV_BUFFER_FLAG_READONLY (1 << 0)

/**
 * The buffer is always treated as read-only.
 */
#define BUFFER_FLAG_READONLY      (1 << 0)

static void av_buffer_default_free(void *opaque, uint8_t *data)
{
    av_free(data);
}

static AVBufferRef *av_buffer_create(uint8_t *data, int size,
                              void (*free)(void *opaque, uint8_t *data),
                              void *opaque, int flags)
{
    AVBufferRef *ref = NULL;
    AVBuffer    *buf = NULL;

    buf = av_mallocz(sizeof(*buf));
    if (!buf)
        return NULL;

    buf->data     = data;
    buf->size     = size;
    buf->free     = free ? free : av_buffer_default_free;
    buf->opaque   = opaque;
    buf->refcount = 1;

    if (flags & AV_BUFFER_FLAG_READONLY)
        buf->flags |= BUFFER_FLAG_READONLY;

    ref = av_mallocz(sizeof(*ref));
    if (!ref) {
        av_freep(&buf);
        return NULL;
    }

    ref->buffer = buf;
    ref->data   = data;
    ref->size   = size;

    return ref;
}

static AVBufferRef *av_buffer_alloc(int size)
{
    AVBufferRef *ret = NULL;
    uint8_t    *data = NULL;

    data = av_malloc(size);
    if (!data)
        return NULL;

    ret = av_buffer_create(data, size, av_buffer_default_free, NULL, 0);
    if (!ret)
        av_freep(&data);

    return ret;
}


static AVBufferRef *av_buffer_allocz(int size)
{
    AVBufferRef *ret = av_buffer_alloc(size);
    if (!ret)
        return NULL;

    memset(ret->data, 0, size);
    return ret;
}

//////////// hevc_ps.c //////////////////
static const uint8_t ff_hevc_diag_scan4x4_x[16] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 1, 2,
    3, 2, 3, 3,
};

static const uint8_t ff_hevc_diag_scan4x4_y[16] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 3, 2,
    1, 3, 2, 3,
};

static const uint8_t diag_scan4x4_inv[4][4] = {
    { 0,  2,  5,  9, },
    { 1,  4,  8, 12, },
    { 3,  7, 11, 14, },
    { 6, 10, 13, 15, },
};

static const uint8_t ff_hevc_diag_scan8x8_x[64] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 0, 1,
    2, 3, 4, 0,
    1, 2, 3, 4,
    5, 0, 1, 2,
    3, 4, 5, 6,
    0, 1, 2, 3,
    4, 5, 6, 7,
    1, 2, 3, 4,
    5, 6, 7, 2,
    3, 4, 5, 6,
    7, 3, 4, 5,
    6, 7, 4, 5,
    6, 7, 5, 6,
    7, 6, 7, 7,
};

static const uint8_t ff_hevc_diag_scan8x8_y[64] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 4, 3,
    2, 1, 0, 5,
    4, 3, 2, 1,
    0, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 7,
    6, 5, 4, 3,
    2, 7, 6, 5,
    4, 3, 7, 6,
    5, 4, 7, 6,
    5, 7, 6, 7,
};

static const uint8_t default_scaling_list_intra[] = {
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29,36, 47, 65, 88, 115
};

static const uint8_t default_scaling_list_inter[] = {
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91
};

static const AVRational vui_sar[] = {
    { 0,   1  },
    { 1,   1  },
    { 12,  11 },
    { 10,  11 },
    { 16,  11 },
    { 40,  33 },
    { 24,  11 },
    { 20,  11 },
    { 32,  11 },
    { 80,  33 },
    { 18,  11 },
    { 15,  11 },
    { 64,  33 },
    { 160, 99 },
    { 4,   3  },
    { 3,   2  },
    { 2,   1  },
};

static void set_default_scaling_list_data(ScalingList *sl)
{
    int matrixId;

    for (matrixId = 0; matrixId < 6; matrixId++) {
        // 4x4 default is 16
        memset(sl->sl[0][matrixId], 16, 16);
        sl->sl_dc[0][matrixId] = 16; // default for 16x16
        sl->sl_dc[1][matrixId] = 16; // default for 32x32
    }
    memcpy(sl->sl[1][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][1], default_scaling_list_inter, 64);
}

static int scaling_list_data(HEVCContext *s, ScalingList *sl)
{
    GetBitContext *gb = &s->HEVClc->gb;
    uint8_t scaling_list_pred_mode_flag[4][6];
    int32_t scaling_list_dc_coef[2][6];

    int size_id, matrix_id, i, pos, delta;
    for (size_id = 0; size_id < 4; size_id++)
        for (matrix_id = 0; matrix_id < ((size_id == 3) ? 2 : 6); matrix_id++) {
            scaling_list_pred_mode_flag[size_id][matrix_id] = get_bits1(gb);
            if (!scaling_list_pred_mode_flag[size_id][matrix_id]) {
                delta = get_ue_golomb_long(gb);
                // Only need to handle non-zero delta. Zero means default, which should already be in the arrays.
                if (delta) {
                    // Copy from previous array.
                    if (matrix_id - delta < 0) {
                        av_log(s->avctx, AV_LOG_ERROR,
                               "Invalid delta in scaling list data: %d.\n", delta);
                        return AVERROR_INVALIDDATA;
                    }

                    memcpy(sl->sl[size_id][matrix_id],
                           sl->sl[size_id][matrix_id - delta],
                           size_id > 0 ? 64 : 16);
                    if (size_id > 1)
                        sl->sl_dc[size_id - 2][matrix_id] = sl->sl_dc[size_id - 2][matrix_id - delta];
                }
            } else {
                int next_coef;
                int coef_num;
                int32_t scaling_list_delta_coef;

                next_coef = 8;
                coef_num = FFMIN(64, (1  <<  (4 + (size_id  <<  1))));
                if (size_id > 1) {
                    scaling_list_dc_coef[size_id - 2][matrix_id] = get_se_golomb(gb) + 8;
                    next_coef = scaling_list_dc_coef[size_id - 2][matrix_id];
                    sl->sl_dc[size_id - 2][matrix_id] = next_coef;
                }
                for (i = 0; i < coef_num; i++) {
                    if (size_id == 0)
                        pos = 4 * ff_hevc_diag_scan4x4_y[i] + ff_hevc_diag_scan4x4_x[i];
                    else
                        pos = 8 * ff_hevc_diag_scan8x8_y[i] + ff_hevc_diag_scan8x8_x[i];

                    scaling_list_delta_coef = get_se_golomb(gb);
                    next_coef = (next_coef + scaling_list_delta_coef + 256 ) % 256;
                    sl->sl[size_id][matrix_id][pos] = next_coef;
                }
            }
        }

    return 0;
}

static int ff_hevc_decode_short_term_rps(HEVCContext *s, ShortTermRPS *rps,
                                  const HEVCSPS *sps, int is_slice_header)
{
    HEVCLocalContext *lc = s->HEVClc;
    uint8_t rps_predict = 0;
    int delta_poc;
    int k0 = 0;
    int k1 = 0;
    int k  = 0;
    int i;

    GetBitContext *gb = &lc->gb;

    if (rps != sps->st_rps && sps->nb_st_rps)
        rps_predict  = get_bits1(gb);

    if (rps_predict) {
        const ShortTermRPS *rps_ridx;
        int delta_rps, abs_delta_rps;
        uint8_t use_delta_flag = 0;
        uint8_t delta_rps_sign;

        if (is_slice_header) {
            int delta_idx = get_ue_golomb_long(gb) + 1;
            if (delta_idx > sps->nb_st_rps) {
                av_log(s->avctx, AV_LOG_ERROR, "Invalid value of delta_idx "
                       "in slice header RPS: %d > %d.\n", delta_idx,
                       sps->nb_st_rps);
                return AVERROR_INVALIDDATA;
            }
            rps_ridx = &sps->st_rps[sps->nb_st_rps - delta_idx];
        } else
            rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];

        delta_rps_sign = get_bits1(gb);
        abs_delta_rps  = get_ue_golomb_long(gb) + 1;
        delta_rps      = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
        for (i = 0; i <= rps_ridx->num_delta_pocs; i++) {
            int used = rps->used[k] = get_bits1(gb);

            if (!used)
                use_delta_flag = get_bits1(gb);

            if (used || use_delta_flag) {
                if (i < rps_ridx->num_delta_pocs)
                    delta_poc = delta_rps + rps_ridx->delta_poc[i];
                else
                    delta_poc = delta_rps;
                rps->delta_poc[k] = delta_poc;
                if (delta_poc < 0)
                    k0++;
                else
                    k1++;
                k++;
            }
        }

        rps->num_delta_pocs    = k;
        rps->num_negative_pics = k0;
        // sort in increasing order (smallest first)
        if (rps->num_delta_pocs != 0) {
            int used, tmp;
            for (i = 1; i < rps->num_delta_pocs; i++) {
                delta_poc = rps->delta_poc[i];
                used      = rps->used[i];
                for (k = i-1 ; k >= 0;  k--) {
                    tmp = rps->delta_poc[k];
                    if (delta_poc < tmp ) {
                        rps->delta_poc[k+1] = tmp;
                        rps->used[k+1]      = rps->used[k];
                        rps->delta_poc[k]   = delta_poc;
                        rps->used[k]        = used;
                    }
                }
            }
        }
        if ((rps->num_negative_pics >> 1) != 0) {
            int used;
            k = rps->num_negative_pics - 1;
            // flip the negative values to largest first
            for (i = 0; i < rps->num_negative_pics>>1; i++) {
                delta_poc          = rps->delta_poc[i];
                used               = rps->used[i];
                rps->delta_poc[i]  = rps->delta_poc[k];
                rps->used[i]       = rps->used[k];
                rps->delta_poc[k]  = delta_poc;
                rps->used[k]       = used;
                k--;
            }
        }
    } else {
        unsigned int prev, nb_positive_pics;
        rps->num_negative_pics = get_ue_golomb_long(gb);
        nb_positive_pics       = get_ue_golomb_long(gb);

        if (rps->num_negative_pics >= MAX_REFS ||
            nb_positive_pics >= MAX_REFS) {
            av_log(s->avctx, AV_LOG_ERROR, "Too many refs in a short term RPS.\n");
            return AVERROR_INVALIDDATA;
        }

        rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
        if (rps->num_delta_pocs) {
            prev = 0;
            for (i = 0; i < rps->num_negative_pics; i++) {
                delta_poc = get_ue_golomb_long(gb) + 1;
                prev -= delta_poc;
                rps->delta_poc[i] = prev;
                rps->used[i] = get_bits1(gb);
            }
            prev = 0;
            for (i = 0; i < nb_positive_pics; i++) {
                delta_poc = get_ue_golomb_long(gb) + 1;
                prev += delta_poc;
                rps->delta_poc[rps->num_negative_pics + i] = prev;
                rps->used[rps->num_negative_pics + i] = get_bits1(gb);
            }
        }
    }
    return 0;
}

static int decode_profile_tier_level(HEVCContext *s, PTL *ptl, int max_num_sub_layers)
{
    HEVCLocalContext *lc = s->HEVClc;
    GetBitContext *gb = &lc->gb;
    int i, j;

    ptl->general_profile_space = get_bits(gb, 2);
    ptl->general_tier_flag = get_bits1(gb);
    ptl->general_profile_idc = get_bits(gb, 5);
    if (ptl->general_profile_idc == 1)
        av_log(s->avctx, AV_LOG_DEBUG, "Main profile bitstream\n");
    else if (ptl->general_profile_idc == 2)
        av_log(s->avctx, AV_LOG_DEBUG, "Main10 profile bitstream\n");
    else
        av_log(s->avctx, AV_LOG_WARNING, "No profile indication! (%d)\n", ptl->general_profile_idc);

    for (i = 0; i < 32; i++)
        ptl->general_profile_compatibility_flag[i] = get_bits1(gb);
    skip_bits1(gb);// general_progressive_source_flag
    skip_bits1(gb);// general_interlaced_source_flag
    skip_bits1(gb);// general_non_packed_constraint_flag
    skip_bits1(gb);// general_frame_only_constraint_flag
    if (get_bits(gb, 16) != 0) // XXX_reserved_zero_44bits[0..15]
        return -1;
    if (get_bits(gb, 16) != 0) // XXX_reserved_zero_44bits[16..31]
        return -1;
    if (get_bits(gb, 12) != 0) // XXX_reserved_zero_44bits[32..43]
        return -1;

    ptl->general_level_idc = get_bits(gb, 8);
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        ptl->sub_layer_profile_present_flag[i] = get_bits1(gb);
        ptl->sub_layer_level_present_flag[i] = get_bits1(gb);
    }
    if (max_num_sub_layers - 1 > 0)
        for (i = max_num_sub_layers - 1; i < 8; i++)
            skip_bits(gb, 2); // reserved_zero_2bits[i]
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        if (ptl->sub_layer_profile_present_flag[i]) {
            ptl->sub_layer_profile_space[i] = get_bits(gb, 2);
            ptl->sub_layer_tier_flag[i] = get_bits(gb, 1);
            ptl->sub_layer_profile_idc[i] = get_bits(gb, 5);
            for (j = 0; j < 32; j++)
                ptl->sub_layer_profile_compatibility_flags[i][j] = get_bits1(gb);
            skip_bits1(gb);// sub_layer_progressive_source_flag
            skip_bits1(gb);// sub_layer_interlaced_source_flag
            skip_bits1(gb);// sub_layer_non_packed_constraint_flag
            skip_bits1(gb);// sub_layer_frame_only_constraint_flag

            if (get_bits(gb, 16) != 0) // sub_layer_reserved_zero_44bits[0..15]
                return -1;
            if (get_bits(gb, 16) != 0) // sub_layer_reserved_zero_44bits[16..31]
                return -1;
            if (get_bits(gb, 12) != 0) // sub_layer_reserved_zero_44bits[32..43]
                return -1;
        }
        if (ptl->sub_layer_level_present_flag[i])
            ptl->sub_layer_level_idc[i] = get_bits(gb, 8);
    }
    return 0;
}

static void decode_sublayer_hrd(HEVCContext *s, int nb_cpb, int subpic_params_present)
{
    GetBitContext *gb = &s->HEVClc->gb;
    int i;

    for (i = 0; i < nb_cpb; i++) {
        get_ue_golomb_long(gb); // bit_rate_value_minus1
        get_ue_golomb_long(gb); // cpb_size_value_minus1

        if (subpic_params_present) {
            get_ue_golomb_long(gb); // cpb_size_du_value_minus1
            get_ue_golomb_long(gb); // bit_rate_du_value_minus1
        }
        skip_bits1(gb); // cbr_flag
    }
}

static void decode_hrd(HEVCContext *s, int common_inf_present, int max_sublayers)
{
    GetBitContext *gb = &s->HEVClc->gb;
    int nal_params_present = 0, vcl_params_present = 0;
    int subpic_params_present = 0;
    int i;

    if (common_inf_present) {
        nal_params_present = get_bits1(gb);
        vcl_params_present = get_bits1(gb);

        if (nal_params_present || vcl_params_present) {
            subpic_params_present = get_bits1(gb);

            if (subpic_params_present) {
                skip_bits(gb, 8); // tick_divisor_minus2
                skip_bits(gb, 5); // du_cpb_removal_delay_increment_length_minus1
                skip_bits(gb, 1); // sub_pic_cpb_params_in_pic_timing_sei_flag
                skip_bits(gb, 5); // dpb_output_delay_du_length_minus1
            }

            skip_bits(gb, 4); // bit_rate_scale
            skip_bits(gb, 4); // cpb_size_scale

            if (subpic_params_present)
                skip_bits(gb, 4); // cpb_size_du_scale

            skip_bits(gb, 5); // initial_cpb_removal_delay_length_minus1
            skip_bits(gb, 5); // au_cpb_removal_delay_length_minus1
            skip_bits(gb, 5); // dpb_output_delay_length_minus1
        }
    }

    for (i = 0; i < max_sublayers; i++) {
        int low_delay = 0;
        int nb_cpb = 1;
        int fixed_rate = get_bits1(gb);

        if (!fixed_rate)
            fixed_rate = get_bits1(gb);

        if (fixed_rate)
            get_ue_golomb_long(gb); // elemental_duration_in_tc_minus1
        else
            low_delay = get_bits1(gb);

        if (!low_delay)
            nb_cpb = get_ue_golomb_long(gb) + 1;

        if (nal_params_present)
            decode_sublayer_hrd(s, nb_cpb, subpic_params_present);
        if (vcl_params_present)
            decode_sublayer_hrd(s, nb_cpb, subpic_params_present);
    }
}

static void decode_vui(HEVCContext *s, HEVCSPS *sps)
{
    VUI *vui = &sps->vui;
    GetBitContext *gb = &s->HEVClc->gb;
    int sar_present;

    av_log(s->avctx, AV_LOG_DEBUG, "Decoding VUI\n");

    sar_present = get_bits1(gb);
    if (sar_present) {
        uint8_t sar_idx = get_bits(gb, 8);
        if (sar_idx < FF_ARRAY_ELEMS(vui_sar))
            vui->sar = vui_sar[sar_idx];
        else if (sar_idx == 255) {
            vui->sar.num = get_bits(gb, 16);
            vui->sar.den = get_bits(gb, 16);
        } else
            av_log(s->avctx, AV_LOG_WARNING, "Unknown SAR index: %u.\n",
                   sar_idx);
    }

    vui->overscan_info_present_flag = get_bits1(gb);
    if (vui->overscan_info_present_flag)
        vui->overscan_appropriate_flag = get_bits1(gb);

    vui->video_signal_type_present_flag = get_bits1(gb);
    if (vui->video_signal_type_present_flag) {
        vui->video_format                    = get_bits(gb, 3);
        vui->video_full_range_flag           = get_bits1(gb);
        vui->colour_description_present_flag = get_bits1(gb);
        if (vui->colour_description_present_flag) {
            vui->colour_primaries        = get_bits(gb, 8);
            vui->transfer_characteristic = get_bits(gb, 8);
            vui->matrix_coeffs           = get_bits(gb, 8);
        }
    }

    vui->chroma_loc_info_present_flag = get_bits1(gb);
    if (vui->chroma_loc_info_present_flag) {
        vui->chroma_sample_loc_type_top_field    = get_ue_golomb_long(gb);
        vui->chroma_sample_loc_type_bottom_field = get_ue_golomb_long(gb);
    }

    vui->neutra_chroma_indication_flag = get_bits1(gb);
    vui->field_seq_flag                = get_bits1(gb);
    vui->frame_field_info_present_flag = get_bits1(gb);

    vui->default_display_window_flag = get_bits1(gb);
    if (vui->default_display_window_flag) {
        //TODO: * 2 is only valid for 420
        vui->def_disp_win.left_offset   = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.right_offset  = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.top_offset    = get_ue_golomb_long(gb) * 2;
        vui->def_disp_win.bottom_offset = get_ue_golomb_long(gb) * 2;
/*
        if (s->strict_def_disp_win &&
            s->avctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {
            av_log(s->avctx, AV_LOG_DEBUG,
                   "discarding vui default display window, "
                   "original values are l:%u r:%u t:%u b:%u\n",
                   vui->def_disp_win.left_offset,
                   vui->def_disp_win.right_offset,
                   vui->def_disp_win.top_offset,
                   vui->def_disp_win.bottom_offset);

            vui->def_disp_win.left_offset   =
            vui->def_disp_win.right_offset  =
            vui->def_disp_win.top_offset    =
            vui->def_disp_win.bottom_offset = 0;
        }
*/
    }

    vui->vui_timing_info_present_flag = get_bits1(gb);
    if (vui->vui_timing_info_present_flag) {
        vui->vui_num_units_in_tick               = get_bits(gb, 32);
        vui->vui_time_scale                      = get_bits(gb, 32);
        vui->vui_poc_proportional_to_timing_flag = get_bits1(gb);
        if (vui->vui_poc_proportional_to_timing_flag)
            vui->vui_num_ticks_poc_diff_one_minus1 = get_ue_golomb_long(gb);
        vui->vui_hrd_parameters_present_flag = get_bits1(gb);
        if (vui->vui_hrd_parameters_present_flag)
            decode_hrd(s, 1, sps->max_sub_layers);
    }

    vui->bitstream_restriction_flag = get_bits1(gb);
    if (vui->bitstream_restriction_flag) {
        vui->tiles_fixed_structure_flag              = get_bits1(gb);
        vui->motion_vectors_over_pic_boundaries_flag = get_bits1(gb);
        vui->restricted_ref_pic_lists_flag           = get_bits1(gb);
        vui->min_spatial_segmentation_idc            = get_ue_golomb_long(gb);
        vui->max_bytes_per_pic_denom                 = get_ue_golomb_long(gb);
        vui->max_bits_per_min_cu_denom               = get_ue_golomb_long(gb);
        vui->log2_max_mv_length_horizontal           = get_ue_golomb_long(gb);
        vui->log2_max_mv_length_vertical             = get_ue_golomb_long(gb);
    }
}

int ff_hevc_decode_nal_vps(HEVCContext *s)
{
    int i,j;
    GetBitContext *gb = &s->HEVClc->gb;
    int vps_id = 0;
    VPS *vps;

    av_log(s->avctx, AV_LOG_DEBUG, "Decoding VPS\n");

    vps = av_mallocz(sizeof(*vps));
    if (!vps)
        return AVERROR(ENOMEM);

    vps_id = get_bits(gb, 4);
    if (vps_id >= MAX_VPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "VPS id out of range: %d\n", vps_id);
        goto err;
    }

    if (get_bits(gb, 2) != 3) { // vps_reserved_three_2bits
        av_log(s->avctx, AV_LOG_ERROR, "vps_reserved_three_2bits is not three\n");
        goto err;
    }

    vps->vps_max_layers               = get_bits(gb, 6) + 1;
    vps->vps_max_sub_layers           = get_bits(gb, 3) + 1;
    vps->vps_temporal_id_nesting_flag = get_bits1(gb);

    if (get_bits(gb, 16) != 0xffff) { // vps_reserved_ffff_16bits
        av_log(s->avctx, AV_LOG_ERROR, "vps_reserved_ffff_16bits is not 0xffff\n");
        goto err;
    }

    if (vps->vps_max_sub_layers > MAX_SUB_LAYERS) {
        av_log(s->avctx, AV_LOG_ERROR, "vps_max_sub_layers out of range: %d\n",
               vps->vps_max_sub_layers);
        goto err;
    }

    if (decode_profile_tier_level(s, &vps->ptl, vps->vps_max_sub_layers) < 0) {
        av_log(s->avctx, AV_LOG_ERROR, "Error decoding profile tier level.\n");
        goto err;
    }
    vps->vps_sub_layer_ordering_info_present_flag = get_bits1(gb);

    i = vps->vps_sub_layer_ordering_info_present_flag ? 0 : vps->vps_max_sub_layers - 1;
    for (; i < vps->vps_max_sub_layers; i++) {
        vps->vps_max_dec_pic_buffering[i] = get_ue_golomb_long(gb) + 1;
        vps->vps_num_reorder_pics[i]      = get_ue_golomb_long(gb);
        vps->vps_max_latency_increase[i]  = get_ue_golomb_long(gb) - 1;

        if (vps->vps_max_dec_pic_buffering[i] > MAX_DPB_SIZE) {
            av_log(s->avctx, AV_LOG_ERROR, "vps_max_dec_pic_buffering_minus1 out of range: %d\n",
                   vps->vps_max_dec_pic_buffering[i] - 1);
            goto err;
        }
        if (vps->vps_num_reorder_pics[i] > vps->vps_max_dec_pic_buffering[i] - 1) {
            av_log(s->avctx, AV_LOG_ERROR, "vps_max_num_reorder_pics out of range: %d\n",
                   vps->vps_num_reorder_pics[i]);
            goto err;
        }
    }

    vps->vps_max_layer_id   = get_bits(gb, 6);
    vps->vps_num_layer_sets = get_ue_golomb_long(gb) + 1;
    for (i = 1; i < vps->vps_num_layer_sets; i++)
        for (j = 0; j <= vps->vps_max_layer_id; j++)
            skip_bits(gb, 1); // layer_id_included_flag[i][j]

    vps->vps_timing_info_present_flag = get_bits1(gb);
    if (vps->vps_timing_info_present_flag) {
        vps->vps_num_units_in_tick               = get_bits_long(gb, 32);
        vps->vps_time_scale                      = get_bits_long(gb, 32);
        vps->vps_poc_proportional_to_timing_flag = get_bits1(gb);
        if (vps->vps_poc_proportional_to_timing_flag)
            vps->vps_num_ticks_poc_diff_one = get_ue_golomb_long(gb) + 1;
        vps->vps_num_hrd_parameters = get_ue_golomb_long(gb);
        for (i = 0; i < vps->vps_num_hrd_parameters; i++) {
            int common_inf_present = 1;

            get_ue_golomb_long(gb); // hrd_layer_set_idx
            if (i)
                common_inf_present = get_bits1(gb);
            decode_hrd(s, common_inf_present, vps->vps_max_sub_layers);
        }
    }
    get_bits1(gb); /* vps_extension_flag */

    av_free(s->vps_list[vps_id]);
    s->vps_list[vps_id] = vps;
    return 0;

err:
    av_free(vps);
    return AVERROR_INVALIDDATA;
}

int ff_hevc_decode_nal_sps(HEVCContext *s)
{
    const AVPixFmtDescriptor *desc;
    GetBitContext *gb = &s->HEVClc->gb;
    int ret    = 0;
    int sps_id = 0;
    int log2_diff_max_min_transform_block_size;
    int bit_depth_chroma, start, vui_present, sublayer_ordering_info;
    int i;

    HEVCSPS *sps;
    AVBufferRef *sps_buf = av_buffer_allocz(sizeof(*sps));

    if (!sps_buf)
        return AVERROR(ENOMEM);
    sps = (HEVCSPS*)sps_buf->data;

    av_log(s->avctx, AV_LOG_DEBUG, "Decoding SPS\n");

    // Coded parameters

    sps->vps_id = get_bits(gb, 4);
    if (sps->vps_id >= MAX_VPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "VPS id out of range: %d\n", sps->vps_id);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    sps->max_sub_layers = get_bits(gb, 3) + 1;
    if (sps->max_sub_layers > MAX_SUB_LAYERS) {
        av_log(s->avctx, AV_LOG_ERROR, "sps_max_sub_layers out of range: %d\n",
               sps->max_sub_layers);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    skip_bits1(gb); // temporal_id_nesting_flag
    if (decode_profile_tier_level(s, &sps->ptl, sps->max_sub_layers) < 0) {
        av_log(s->avctx, AV_LOG_ERROR, "error decoding profile tier level\n");
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    sps_id = get_ue_golomb_long(gb);
    if (sps_id >= MAX_SPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "SPS id out of range: %d\n", sps_id);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    sps->chroma_format_idc = get_ue_golomb_long(gb);
    if (sps->chroma_format_idc != 1) {
        //avpriv_report_missing_feature(s->avctx, "chroma_format_idc != 1\n");
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    if (sps->chroma_format_idc == 3)
        sps->separate_colour_plane_flag = get_bits1(gb);

    sps->width  = get_ue_golomb_long(gb);
    sps->height = get_ue_golomb_long(gb);
    if ((ret = av_image_check_size(sps->width,
                                   sps->height, 0, s->avctx)) < 0)
        goto err;

    if (get_bits1(gb)) { // pic_conformance_flag
        //TODO: * 2 is only valid for 420
        sps->pic_conf_win.left_offset   = get_ue_golomb_long(gb) * 2;
        sps->pic_conf_win.right_offset  = get_ue_golomb_long(gb) * 2;
        sps->pic_conf_win.top_offset    = get_ue_golomb_long(gb) * 2;
        sps->pic_conf_win.bottom_offset = get_ue_golomb_long(gb) * 2;

/*
        if (s->avctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {
            av_log(s->avctx, AV_LOG_DEBUG,
                   "discarding sps conformance window, "
                   "original values are l:%u r:%u t:%u b:%u\n",
                   sps->pic_conf_win.left_offset,
                   sps->pic_conf_win.right_offset,
                   sps->pic_conf_win.top_offset,
                   sps->pic_conf_win.bottom_offset);

            sps->pic_conf_win.left_offset   =
            sps->pic_conf_win.right_offset  =
            sps->pic_conf_win.top_offset    =
            sps->pic_conf_win.bottom_offset = 0;
        }
*/
        sps->output_window = sps->pic_conf_win;
    }

    sps->bit_depth   = get_ue_golomb_long(gb) + 8;
    bit_depth_chroma = get_ue_golomb_long(gb) + 8;
    if (bit_depth_chroma != sps->bit_depth) {
        av_log(s->avctx, AV_LOG_ERROR,
               "Luma bit depth (%d) is different from chroma bit depth (%d), this is unsupported.\n",
               sps->bit_depth, bit_depth_chroma);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    if (sps->chroma_format_idc == 1) {
        switch (sps->bit_depth) {
        case 8:  sps->pix_fmt = PIX_FMT_YUV420P;   break;
        case 9:  sps->pix_fmt = PIX_FMT_YUV420P9;  break;
        case 10: sps->pix_fmt = PIX_FMT_YUV420P10; break;
        default:
            av_log(s->avctx, AV_LOG_ERROR, "Unsupported bit depth: %d\n",
                   sps->bit_depth);
            ret = AVERROR_PATCHWELCOME;
            goto err;
        }
    } else {
        av_log(s->avctx, AV_LOG_ERROR, "non-4:2:0 support is currently unspecified.\n");
        return AVERROR_PATCHWELCOME;
    }

    desc = av_pix_fmt_desc_get(sps->pix_fmt);
    if (!desc) {
        ret = AVERROR(EINVAL);
        goto err;
    }

    sps->hshift[0] = sps->vshift[0] = 0;
    sps->hshift[2] = sps->hshift[1] = desc->log2_chroma_w;
    sps->vshift[2] = sps->vshift[1] = desc->log2_chroma_h;

    sps->pixel_shift = sps->bit_depth > 8;

    sps->log2_max_poc_lsb = get_ue_golomb_long(gb) + 4;
    if (sps->log2_max_poc_lsb > 16) {
        av_log(s->avctx, AV_LOG_ERROR, "log2_max_pic_order_cnt_lsb_minus4 out range: %d\n",
               sps->log2_max_poc_lsb - 4);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    sublayer_ordering_info = get_bits1(gb);
    start = sublayer_ordering_info ? 0 : sps->max_sub_layers - 1;
    for (i = start; i < sps->max_sub_layers; i++) {
        sps->temporal_layer[i].max_dec_pic_buffering = get_ue_golomb_long(gb) + 1;
        sps->temporal_layer[i].num_reorder_pics      = get_ue_golomb_long(gb);
        sps->temporal_layer[i].max_latency_increase  = get_ue_golomb_long(gb) - 1;
        if (sps->temporal_layer[i].max_dec_pic_buffering > MAX_DPB_SIZE) {
            av_log(s->avctx, AV_LOG_ERROR, "sps_max_dec_pic_buffering_minus1 out of range: %d\n",
                   sps->temporal_layer[i].max_dec_pic_buffering - 1);
            ret = AVERROR_INVALIDDATA;
            goto err;
        }
        if (sps->temporal_layer[i].num_reorder_pics > sps->temporal_layer[i].max_dec_pic_buffering - 1) {
            av_log(s->avctx, AV_LOG_ERROR, "sps_max_num_reorder_pics out of range: %d\n",
                   sps->temporal_layer[i].num_reorder_pics);
            ret = AVERROR_INVALIDDATA;
            goto err;
        }
    }

    if (!sublayer_ordering_info) {
        for (i = 0; i < start; i++){
            sps->temporal_layer[i].max_dec_pic_buffering = sps->temporal_layer[start].max_dec_pic_buffering;
            sps->temporal_layer[i].num_reorder_pics      = sps->temporal_layer[start].num_reorder_pics;
            sps->temporal_layer[i].max_latency_increase  = sps->temporal_layer[start].max_latency_increase;
        }
    }

    sps->log2_min_cb_size             = get_ue_golomb_long(gb) + 3;
    sps->log2_diff_max_min_coding_block_size    = get_ue_golomb_long(gb);
    sps->log2_min_tb_size                       = get_ue_golomb_long(gb) + 2;
    log2_diff_max_min_transform_block_size      = get_ue_golomb_long(gb);
    sps->log2_max_trafo_size                    = log2_diff_max_min_transform_block_size + sps->log2_min_tb_size;

    if (sps->log2_min_tb_size >= sps->log2_min_cb_size) {
        av_log(s->avctx, AV_LOG_ERROR, "Invalid value for log2_min_tb_size");
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    sps->max_transform_hierarchy_depth_inter = get_ue_golomb_long(gb);
    sps->max_transform_hierarchy_depth_intra = get_ue_golomb_long(gb);

    sps->scaling_list_enable_flag = get_bits1(gb);
    if (sps->scaling_list_enable_flag) {
        set_default_scaling_list_data(&sps->scaling_list);

        if (get_bits1(gb)) {
            ret = scaling_list_data(s, &sps->scaling_list);
            if (ret < 0)
                goto err;
        }
    }

    sps->amp_enabled_flag = get_bits1(gb);
    sps->sao_enabled      = get_bits1(gb);

    sps->pcm_enabled_flag = get_bits1(gb);
    if (sps->pcm_enabled_flag) {
        sps->pcm.bit_depth   = get_bits(gb, 4) + 1;
        sps->pcm.bit_depth_chroma = get_bits(gb, 4) + 1;
        sps->pcm.log2_min_pcm_cb_size = get_ue_golomb_long(gb) + 3;
        sps->pcm.log2_max_pcm_cb_size = sps->pcm.log2_min_pcm_cb_size +
                                        get_ue_golomb_long(gb);
        if (sps->pcm.bit_depth > sps->bit_depth) {
            av_log(s->avctx, AV_LOG_ERROR,
                   "PCM bit depth (%d) is greater than normal bit depth (%d)\n",
                   sps->pcm.bit_depth, sps->bit_depth);
            ret = AVERROR_INVALIDDATA;
            goto err;
        }

        sps->pcm.loop_filter_disable_flag = get_bits1(gb);
    }

    sps->nb_st_rps = get_ue_golomb_long(gb);
    if (sps->nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "Too many short term RPS: %d.\n",
               sps->nb_st_rps);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    for (i = 0; i < sps->nb_st_rps; i++) {
        if ((ret = ff_hevc_decode_short_term_rps(s, &sps->st_rps[i],
                                                 sps, 0)) < 0)
            goto err;
    }

    sps->long_term_ref_pics_present_flag = get_bits1(gb);
    if (sps->long_term_ref_pics_present_flag) {
        sps->num_long_term_ref_pics_sps = get_ue_golomb_long(gb);
        for (i = 0; i < sps->num_long_term_ref_pics_sps; i++) {
            sps->lt_ref_pic_poc_lsb_sps[i]       = get_bits(gb, sps->log2_max_poc_lsb);
            sps->used_by_curr_pic_lt_sps_flag[i] = get_bits1(gb);
        }
    }

    sps->sps_temporal_mvp_enabled_flag          = get_bits1(gb);
    sps->sps_strong_intra_smoothing_enable_flag = get_bits1(gb);
    sps->vui.sar = (AVRational){0, 1};
    vui_present = get_bits1(gb);
    if (vui_present)
        decode_vui(s, sps);
    skip_bits1(gb); // sps_extension_flag

    if (s->strict_def_disp_win) {
        sps->output_window.left_offset   += sps->vui.def_disp_win.left_offset;
        sps->output_window.right_offset  += sps->vui.def_disp_win.right_offset;
        sps->output_window.top_offset    += sps->vui.def_disp_win.top_offset;
        sps->output_window.bottom_offset += sps->vui.def_disp_win.bottom_offset;
    }
    if (sps->output_window.left_offset & (0x1F >> (sps->pixel_shift))) {
        sps->output_window.left_offset &= ~(0x1F >> (sps->pixel_shift));
        av_log(s->avctx, AV_LOG_WARNING, "Reducing left output window to %d "
               "chroma samples to preserve alignment.\n",
               sps->output_window.left_offset);
    }
    sps->output_width  = sps->width -
                         (sps->output_window.left_offset + sps->output_window.right_offset);
    sps->output_height = sps->height -
                         (sps->output_window.top_offset + sps->output_window.bottom_offset);
    if (sps->output_width <= 0 || sps->output_height <= 0) {
        av_log(s->avctx, AV_LOG_WARNING, "Invalid visible frame dimensions: %dx%d.\n",
               sps->output_width, sps->output_height);
        //if (s->avctx->err_recognition & AV_EF_EXPLODE) {
        //    ret = AVERROR_INVALIDDATA;
        //    goto err;
        //}
        av_log(s->avctx, AV_LOG_WARNING, "Displaying the whole video surface.\n");
        sps->pic_conf_win.left_offset   =
        sps->pic_conf_win.right_offset  =
        sps->pic_conf_win.top_offset    =
        sps->pic_conf_win.bottom_offset = 0;
        sps->output_width  = sps->width;
        sps->output_height = sps->height;
    }

    // Inferred parameters
    sps->log2_ctb_size     = sps->log2_min_cb_size
                             + sps->log2_diff_max_min_coding_block_size;
    sps->log2_min_pu_size  = sps->log2_min_cb_size - 1;

    sps->ctb_width         = (sps->width  + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    sps->ctb_height        = (sps->height + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    sps->ctb_size          = sps->ctb_width * sps->ctb_height;

    sps->min_cb_width      = sps->width  >> sps->log2_min_cb_size;
    sps->min_cb_height     = sps->height >> sps->log2_min_cb_size;
    sps->min_tb_width      = sps->width  >> sps->log2_min_tb_size;
    sps->min_tb_height     = sps->height >> sps->log2_min_tb_size;
    sps->min_pu_width      = sps->width  >> sps->log2_min_pu_size;
    sps->min_pu_height     = sps->height >> sps->log2_min_pu_size;

    sps->qp_bd_offset      = 6 * (sps->bit_depth - 8);

    if (sps->width  & ((1 << sps->log2_min_cb_size) - 1) ||
        sps->height & ((1 << sps->log2_min_cb_size) - 1)) {
        av_log(s->avctx, AV_LOG_ERROR, "Invalid coded frame dimensions.\n");
        goto err;
    }

    if (sps->log2_ctb_size > MAX_LOG2_CTB_SIZE) {
        av_log(s->avctx, AV_LOG_ERROR, "CTB size out of range: 2^%d\n", sps->log2_ctb_size);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_inter > sps->log2_ctb_size - sps->log2_min_tb_size) {
        av_log(s->avctx, AV_LOG_ERROR, "max_transform_hierarchy_depth_inter out of range: %d\n",
               sps->max_transform_hierarchy_depth_inter);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_intra > sps->log2_ctb_size - sps->log2_min_tb_size) {
        av_log(s->avctx, AV_LOG_ERROR, "max_transform_hierarchy_depth_intra out of range: %d\n",
               sps->max_transform_hierarchy_depth_intra);
        goto err;
    }
    if (sps->log2_max_trafo_size > FFMIN(sps->log2_ctb_size, 5)) {
        av_log(s->avctx, AV_LOG_ERROR, "max transform block size out of range: %d\n",
               sps->log2_max_trafo_size);
        goto err;
    }

    if (s->avctx->debug & FF_DEBUG_BITSTREAM) {
        av_log(s->avctx, AV_LOG_DEBUG, "Parsed SPS: id %d; coded wxh: %dx%d; "
               "cropped wxh: %dx%d; pix_fmt: %s.\n",
               sps_id, sps->width, sps->height,
               sps->output_width, sps->output_height,
               av_get_pix_fmt_name(sps->pix_fmt));
    }

    /* check if this is a repeat of an already parsed SPS, then keep the
     * original one.
     * otherwise drop all PPSes that depend on it */
    if (s->sps_list[sps_id] &&
        !memcmp(s->sps_list[sps_id]->data, sps_buf->data, sps_buf->size)) {
        av_buffer_unref(&sps_buf);
    } else {
        for (i = 0; i < FF_ARRAY_ELEMS(s->pps_list); i++) {
            if (s->pps_list[i] && ((HEVCPPS*)s->pps_list[i]->data)->sps_id == sps_id)
                av_buffer_unref(&s->pps_list[i]);
        }
        av_buffer_unref(&s->sps_list[sps_id]);
        s->sps_list[sps_id] = sps_buf;
    }

    return 0;
err:

    av_buffer_unref(&sps_buf);
    return ret;
}

static void hevc_pps_free(void *opaque, uint8_t *data)
{
    HEVCPPS *pps = (HEVCPPS*)data;

    av_freep(&pps->column_width);
    av_freep(&pps->row_height);
    av_freep(&pps->col_bd);
    av_freep(&pps->row_bd);
    av_freep(&pps->col_idxX);
    av_freep(&pps->ctb_addr_rs_to_ts);
    av_freep(&pps->ctb_addr_ts_to_rs);
    av_freep(&pps->tile_pos_rs);
    av_freep(&pps->tile_id);
    av_freep(&pps->min_cb_addr_zs);
    av_freep(&pps->min_tb_addr_zs);

    av_freep(&pps);
}

int ff_hevc_decode_nal_pps(HEVCContext *s)
{
    GetBitContext *gb = &s->HEVClc->gb;
    HEVCSPS      *sps = NULL;
    int pic_area_in_ctbs, pic_area_in_min_cbs, pic_area_in_min_tbs;
    int log2_diff_ctb_min_tb_size;
    int i, j, x, y, ctb_addr_rs, tile_id;
    int ret    = 0;
    int pps_id = 0;

    AVBufferRef *pps_buf;
    HEVCPPS *pps = av_mallocz(sizeof(*pps));

    if (!pps)
        return AVERROR(ENOMEM);

    pps_buf = av_buffer_create((uint8_t*)pps, sizeof(*pps), hevc_pps_free, NULL, 0);
    if (!pps_buf) {
        av_freep(&pps);
        return AVERROR(ENOMEM);
    }

    av_log(s->avctx, AV_LOG_DEBUG, "Decoding PPS\n");

    // Default values
    pps->loop_filter_across_tiles_enabled_flag = 1;
    pps->num_tile_columns                      = 1;
    pps->num_tile_rows                         = 1;
    pps->uniform_spacing_flag                  = 1;
    pps->disable_dbf                           = 0;
    pps->beta_offset                           = 0;
    pps->tc_offset                             = 0;

    // Coded parameters
    pps_id = get_ue_golomb_long(gb);
    if (pps_id >= MAX_PPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "PPS id out of range: %d\n", pps_id);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    pps->sps_id = get_ue_golomb_long(gb);
    if (pps->sps_id >= MAX_SPS_COUNT) {
        av_log(s->avctx, AV_LOG_ERROR, "SPS id out of range: %d\n", pps->sps_id);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    if (!s->sps_list[pps->sps_id]) {
        av_log(s->avctx, AV_LOG_ERROR, "SPS does not exist \n");
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    sps = (HEVCSPS*)s->sps_list[pps->sps_id]->data;

    pps->dependent_slice_segments_enabled_flag = get_bits1(gb);
    pps->output_flag_present_flag              = get_bits1(gb);
    pps->num_extra_slice_header_bits           = get_bits(gb, 3);

    pps->sign_data_hiding_flag = get_bits1(gb);

    pps->cabac_init_present_flag = get_bits1(gb);

    pps->num_ref_idx_l0_default_active = get_ue_golomb_long(gb) + 1;
    pps->num_ref_idx_l1_default_active = get_ue_golomb_long(gb) + 1;

    pps->pic_init_qp_minus26 = get_se_golomb(gb);

    pps->constrained_intra_pred_flag = get_bits1(gb);
    pps->transform_skip_enabled_flag = get_bits1(gb);

    pps->cu_qp_delta_enabled_flag = get_bits1(gb);
    pps->diff_cu_qp_delta_depth   = 0;
    if (pps->cu_qp_delta_enabled_flag)
        pps->diff_cu_qp_delta_depth = get_ue_golomb_long(gb);

    pps->cb_qp_offset = get_se_golomb(gb);
    if (pps->cb_qp_offset < -12 || pps->cb_qp_offset > 12) {
        av_log(s->avctx, AV_LOG_ERROR, "pps_cb_qp_offset out of range: %d\n",
               pps->cb_qp_offset);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    pps->cr_qp_offset = get_se_golomb(gb);
    if (pps->cr_qp_offset < -12 || pps->cr_qp_offset > 12) {
        av_log(s->avctx, AV_LOG_ERROR, "pps_cr_qp_offset out of range: %d\n",
               pps->cr_qp_offset);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }
    pps->pic_slice_level_chroma_qp_offsets_present_flag = get_bits1(gb);

    pps->weighted_pred_flag   = get_bits1(gb);
    pps->weighted_bipred_flag = get_bits1(gb);

    pps->transquant_bypass_enable_flag    = get_bits1(gb);
    pps->tiles_enabled_flag               = get_bits1(gb);
    pps->entropy_coding_sync_enabled_flag = get_bits1(gb);

    if (pps->tiles_enabled_flag) {
        pps->num_tile_columns     = get_ue_golomb_long(gb) + 1;
        pps->num_tile_rows        = get_ue_golomb_long(gb) + 1;
        if (pps->num_tile_columns == 0 ||
            pps->num_tile_columns >= sps->width) {
            av_log(s->avctx, AV_LOG_ERROR, "num_tile_columns_minus1 out of range: %d\n",
                   pps->num_tile_columns - 1);
            ret = AVERROR_INVALIDDATA;
            goto err;
        }
        if (pps->num_tile_rows == 0 ||
            pps->num_tile_rows >= sps->height) {
            av_log(s->avctx, AV_LOG_ERROR, "num_tile_rows_minus1 out of range: %d\n",
                   pps->num_tile_rows - 1);
            ret = AVERROR_INVALIDDATA;
            goto err;
        }

        pps->column_width = av_malloc_array(pps->num_tile_columns, sizeof(*pps->column_width));
        pps->row_height   = av_malloc_array(pps->num_tile_rows,    sizeof(*pps->row_height));
        if (!pps->column_width || !pps->row_height) {
            ret = AVERROR(ENOMEM);
            goto err;
        }

        pps->uniform_spacing_flag = get_bits1(gb);
        if (!pps->uniform_spacing_flag) {
            int sum = 0;
            for (i = 0; i < pps->num_tile_columns - 1; i++) {
                pps->column_width[i] = get_ue_golomb_long(gb) + 1;
                sum += pps->column_width[i];
            }
            if (sum >= sps->ctb_width) {
                av_log(s->avctx, AV_LOG_ERROR, "Invalid tile widths.\n");
                ret = AVERROR_INVALIDDATA;
                goto err;
            }
            pps->column_width[pps->num_tile_columns - 1] = sps->ctb_width - sum;

            sum = 0;
            for (i = 0; i < pps->num_tile_rows - 1; i++) {
                pps->row_height[i] = get_ue_golomb_long(gb) + 1;
                sum += pps->row_height[i];
            }
            if (sum >= sps->ctb_height) {
                av_log(s->avctx, AV_LOG_ERROR, "Invalid tile heights.\n");
                ret = AVERROR_INVALIDDATA;
                goto err;
            }
            pps->row_height[pps->num_tile_rows - 1] = sps->ctb_height - sum;
        }
        pps->loop_filter_across_tiles_enabled_flag = get_bits1(gb);
    }

    pps->seq_loop_filter_across_slices_enabled_flag = get_bits1(gb);

    pps->deblocking_filter_control_present_flag = get_bits1(gb);
    if (pps->deblocking_filter_control_present_flag) {
        pps->deblocking_filter_override_enabled_flag = get_bits1(gb);
        pps->disable_dbf = get_bits1(gb);
        if (!pps->disable_dbf) {
            pps->beta_offset = get_se_golomb(gb) * 2;
            pps->tc_offset = get_se_golomb(gb) * 2;
            if (pps->beta_offset/2 < -6 || pps->beta_offset/2 > 6) {
                av_log(s->avctx, AV_LOG_ERROR, "pps_beta_offset_div2 out of range: %d\n",
                       pps->beta_offset/2);
                ret = AVERROR_INVALIDDATA;
                goto err;
            }
            if (pps->tc_offset/2 < -6 || pps->tc_offset/2 > 6) {
                av_log(s->avctx, AV_LOG_ERROR, "pps_tc_offset_div2 out of range: %d\n",
                       pps->tc_offset/2);
                ret = AVERROR_INVALIDDATA;
                goto err;
            }
        }
    }

    pps->pps_scaling_list_data_present_flag = get_bits1(gb);
    if (pps->pps_scaling_list_data_present_flag) {
        set_default_scaling_list_data(&pps->scaling_list);
        ret = scaling_list_data(s, &pps->scaling_list);
        if (ret < 0)
            goto err;
    }
    pps->lists_modification_present_flag = get_bits1(gb);
    pps->log2_parallel_merge_level       = get_ue_golomb_long(gb) + 2;
    if (pps->log2_parallel_merge_level > sps->log2_ctb_size) {
        av_log(s->avctx, AV_LOG_ERROR, "log2_parallel_merge_level_minus2 out of range: %d\n",
               pps->log2_parallel_merge_level - 2);
        ret = AVERROR_INVALIDDATA;
        goto err;
    }

    pps->slice_header_extension_present_flag = get_bits1(gb);
    pps->pps_extension_flag                  = get_bits1(gb);

    // Inferred parameters
    pps->col_bd   = av_malloc_array(pps->num_tile_columns + 1, sizeof(*pps->col_bd));
    pps->row_bd   = av_malloc_array(pps->num_tile_rows + 1,    sizeof(*pps->row_bd));
    pps->col_idxX = av_malloc_array(sps->ctb_width,    sizeof(*pps->col_idxX));
    if (!pps->col_bd || !pps->row_bd || !pps->col_idxX) {
        ret = AVERROR(ENOMEM);
        goto err;
    }

    if (pps->uniform_spacing_flag) {
        if (!pps->column_width) {
            pps->column_width = av_malloc_array(pps->num_tile_columns, sizeof(*pps->column_width));
            pps->row_height   = av_malloc_array(pps->num_tile_rows,    sizeof(*pps->row_height));
        }
        if (!pps->column_width || !pps->row_height) {
            ret = AVERROR(ENOMEM);
            goto err;
        }

        for (i = 0; i < pps->num_tile_columns; i++) {
            pps->column_width[i] = ((i + 1) * sps->ctb_width) / pps->num_tile_columns -
                                   (i * sps->ctb_width) / pps->num_tile_columns;
        }

        for (i = 0; i < pps->num_tile_rows; i++) {
            pps->row_height[i] = ((i + 1) * sps->ctb_height) / pps->num_tile_rows -
                                 (i * sps->ctb_height) / pps->num_tile_rows;
        }
    }

    pps->col_bd[0] = 0;
    for (i = 0; i < pps->num_tile_columns; i++)
        pps->col_bd[i + 1] = pps->col_bd[i] + pps->column_width[i];

    pps->row_bd[0] = 0;
    for (i = 0; i < pps->num_tile_rows; i++)
        pps->row_bd[i + 1] = pps->row_bd[i] + pps->row_height[i];

    for (i = 0, j = 0; i < sps->ctb_width; i++) {
         if (i > pps->col_bd[j])
             j++;
         pps->col_idxX[i] = j;
    }

    /**
     * 6.5
     */
    pic_area_in_ctbs     = sps->ctb_width    * sps->ctb_height;
    pic_area_in_min_cbs  = sps->min_cb_width * sps->min_cb_height;
    pic_area_in_min_tbs  = sps->min_tb_width * sps->min_tb_height;

    pps->ctb_addr_rs_to_ts = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->ctb_addr_rs_to_ts));
    pps->ctb_addr_ts_to_rs = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->ctb_addr_ts_to_rs));
    pps->tile_id           = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->tile_id));
    pps->min_cb_addr_zs    = av_malloc_array(pic_area_in_min_cbs, sizeof(*pps->min_cb_addr_zs));
    pps->min_tb_addr_zs    = av_malloc_array(pic_area_in_min_tbs, sizeof(*pps->min_tb_addr_zs));
    if (!pps->ctb_addr_rs_to_ts || !pps->ctb_addr_ts_to_rs ||
        !pps->tile_id || !pps->min_cb_addr_zs || !pps->min_tb_addr_zs) {
        ret = AVERROR(ENOMEM);
        goto err;
    }

    for (ctb_addr_rs = 0; ctb_addr_rs < pic_area_in_ctbs; ctb_addr_rs++) {
        int tb_x   = ctb_addr_rs % sps->ctb_width;
        int tb_y   = ctb_addr_rs / sps->ctb_width;
        int tile_x = 0;
        int tile_y = 0;
        int val    = 0;

        for (i = 0; i < pps->num_tile_columns; i++) {
            if (tb_x < pps->col_bd[i + 1]) {
                tile_x = i;
                break;
            }
        }

        for (i = 0; i < pps->num_tile_rows; i++) {
            if (tb_y < pps->row_bd[i + 1]) {
                tile_y = i;
                break;
            }
        }

        for (i = 0; i < tile_x; i++ )
            val += pps->row_height[tile_y] * pps->column_width[i];
        for (i = 0; i < tile_y; i++ )
            val += sps->ctb_width * pps->row_height[i];

        val += (tb_y - pps->row_bd[tile_y]) * pps->column_width[tile_x] +
               tb_x - pps->col_bd[tile_x];

        pps->ctb_addr_rs_to_ts[ctb_addr_rs] = val;
        pps->ctb_addr_ts_to_rs[val] = ctb_addr_rs;
    }

    for (j = 0, tile_id = 0; j < pps->num_tile_rows; j++)
        for (i = 0; i < pps->num_tile_columns; i++, tile_id++)
            for (y = pps->row_bd[j]; y < pps->row_bd[j + 1]; y++)
                for (x = pps->col_bd[i]; x < pps->col_bd[i + 1]; x++)
                    pps->tile_id[pps->ctb_addr_rs_to_ts[y * sps->ctb_width + x]] = tile_id;

    pps->tile_pos_rs = av_malloc_array(tile_id, sizeof(*pps->tile_pos_rs));
    if (!pps->tile_pos_rs) {
        ret = AVERROR(ENOMEM);
        goto err;
    }

    for (j = 0; j < pps->num_tile_rows; j++)
        for (i = 0; i < pps->num_tile_columns; i++)
            pps->tile_pos_rs[j * pps->num_tile_columns + i] = pps->row_bd[j] * sps->ctb_width + pps->col_bd[i];

    for (y = 0; y < sps->min_cb_height; y++) {
        for (x = 0; x < sps->min_cb_width; x++) {
            int tb_x = x >> sps->log2_diff_max_min_coding_block_size;
            int tb_y = y >> sps->log2_diff_max_min_coding_block_size;
            int ctb_addr_rs = sps->ctb_width * tb_y + tb_x;
            int val = pps->ctb_addr_rs_to_ts[ctb_addr_rs] <<
                      (sps->log2_diff_max_min_coding_block_size * 2);
            for (i = 0; i < sps->log2_diff_max_min_coding_block_size; i++) {
                int m = 1 << i;
                val += (m & x ? m * m : 0) + (m & y ? 2 * m * m : 0);
            }
            pps->min_cb_addr_zs[y * sps->min_cb_width + x] = val;
        }
    }

    log2_diff_ctb_min_tb_size = sps->log2_ctb_size - sps->log2_min_tb_size;
    for (y = 0; y < sps->min_tb_height; y++) {
        for (x = 0; x < sps->min_tb_width; x++) {
            int tb_x = x >> log2_diff_ctb_min_tb_size;
            int tb_y = y >> log2_diff_ctb_min_tb_size;
            int ctb_addr_rs = sps->ctb_width * tb_y + tb_x;
            int val = pps->ctb_addr_rs_to_ts[ctb_addr_rs] <<
                      (log2_diff_ctb_min_tb_size * 2);
            for (i = 0; i < log2_diff_ctb_min_tb_size; i++) {
                int m = 1 << i;
                val += (m & x ? m * m : 0) + (m & y ? 2 * m * m : 0);
            }
            pps->min_tb_addr_zs[y * sps->min_tb_width + x] = val;
        }
    }

    av_buffer_unref(&s->pps_list[pps_id]);
    s->pps_list[pps_id] = pps_buf;

    return 0;

err:
    av_buffer_unref(&pps_buf);
    return ret;
}


static int HEVC_getNextNALUnit(
        const uint8_t **_data, size_t *_size,
        const uint8_t **nalStart, size_t *nalSize,
        int startCodeFollows) {
    const uint8_t *data = *_data;
    size_t size = *_size;

    *nalStart = NULL;
    *nalSize = 0;

    if (size == 0) {
        return -1;
    }

    // Skip any number of leading 0x00.

    size_t offset = 0;
    while (offset < size && data[offset] == 0x00) {
        ++offset;
    }

    if (offset == size) {
        return -1;
    }

    // A valid startcode consists of at least two 0x00 bytes followed by 0x01.

    if (offset < 2 || data[offset] != 0x01) {
        return -1;
    }

    ++offset;

    size_t startOffset = offset;

    for (;;) {
        while (offset < size && data[offset] != 0x01) {
            ++offset;
        }

        if (offset == size) {
            if (startCodeFollows == 1) {
                offset = size + 2;
                break;
            }

            return -1;
        }

        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00) {
            break;
        }

        ++offset;
    }

    size_t endOffset = offset - 2;
    while (endOffset > startOffset + 1 && data[endOffset - 1] == 0x00) {
        --endOffset;
    }

    *nalStart = &data[startOffset];
    *nalSize = endOffset - startOffset;

    if (offset + 2 < size) {
        *_data = &data[offset - 2];
        *_size = size - offset + 2;
    } else {
        *_data = NULL;
        *_size = 0;
    }

    return 0;
}


int HEVC_decode_SPS(const uint8_t *buf,int size,struct hevc_info*info)
{
	HEVCContext s;
	AVCodecContext avctx;
	HEVCLocalContext HEVClc;
	memset(&s,0,sizeof(s));
	memset(&avctx,0,sizeof(avctx));
	memset(&HEVClc,0,sizeof(HEVClc));
	s.avctx=&avctx;
	s.HEVClc=&HEVClc;
	HEVCNAL nal;
	memset(&nal,0,sizeof(nal));
       const uint8_t *nalStart;
       int nalsize;
       while(!HEVC_getNextNALUnit(&buf, &size, &nalStart, &nalsize, 1)){
           if (((nalStart[0]>>1) & 0x3f) == 33) { //SPS
               nalStart += 2;
               nalsize -= 2;
               ff_hevc_extract_rbsp(&s,nalStart,nalsize,&nal);
	        init_get_bits8(&s.HEVClc->gb,nal.data, nal.size);
	        int err=ff_hevc_decode_nal_sps(&s);

	       if(s.sps_list[0])
	       {
		    HEVCSPS *sps = (HEVCSPS*)s.sps_list[0]->data;
		    info->mwidth=sps->width;
		    info->mheight=sps->height;
		    info->bit_depth=sps->bit_depth;
		    info->long_term_ref_pics_present_flag=sps->long_term_ref_pics_present_flag;
		    info->num_long_term_ref_pics_sps=sps->num_long_term_ref_pics_sps;
	       }else{
		    return -1;
	       }
	       return 0;
           }
      }
      return -1;
}


////////////////// sei /////////////////////////
static void decode_nal_sei_decoded_picture_hash(HEVCContext *s, int payload_size)
{
    int cIdx, i;
    uint8_t hash_type;
    //uint16_t picture_crc;
    //uint32_t picture_checksum;
    GetBitContext *gb = &s->HEVClc->gb;
    hash_type = get_bits(gb, 8);


    for( cIdx = 0; cIdx < 3/*((s->sps->chroma_format_idc == 0) ? 1 : 3)*/; cIdx++ ) {
        if ( hash_type == 0 ) {
            s->is_md5 = 1;
            for( i = 0; i < 16; i++) {
                s->md5[cIdx][i] = get_bits(gb, 8);
            }
        } else if( hash_type == 1 ) {
            // picture_crc = get_bits(gb, 16);
            skip_bits(gb, 16);
        } else if( hash_type == 2 ) {
            // picture_checksum = get_bits(gb, 32);
            skip_bits(gb, 32);
        }
    }
}

static void decode_nal_sei_frame_packing_arrangement(HEVCLocalContext *lc)
{
    GetBitContext *gb = &lc->gb;
    int cancel, type, quincunx;

    get_ue_golomb(gb);                      // frame_packing_arrangement_id
    cancel = get_bits1(gb);                 // frame_packing_cancel_flag
    if ( cancel == 0 )
    {
        type = get_bits(gb, 7);             // frame_packing_arrangement_type
        quincunx = get_bits1(gb);           // quincunx_sampling_flag
        skip_bits(gb, 6);                   // content_interpretation_type

        // the following skips spatial_flipping_flag frame0_flipped_flag
        // field_views_flag current_frame_is_frame0_flag
        // frame0_self_contained_flag frame1_self_contained_flag
        skip_bits(gb, 6);

        if ( quincunx == 0 && type != 5 )
            skip_bits(gb, 16);              // frame[01]_grid_position_[xy]
        skip_bits(gb, 8);                   // frame_packing_arrangement_reserved_byte
        skip_bits1(gb);                     // frame_packing_arrangement_persistance_flag
    }
    skip_bits1(gb);                         // upsampled_aspect_ratio_flag
}

static int decode_nal_sei_message(HEVCContext *s)
{
    GetBitContext *gb = &s->HEVClc->gb;

    int payload_type = 0;
    int payload_size = 0;
    int byte = 0xFF;
    av_log(s->avctx, AV_LOG_DEBUG, "Decoding SEI\n");

    while (byte == 0xFF) {
        byte = get_bits(gb, 8);
        payload_type += byte;
    }
    byte = 0xFF;
    while (byte == 0xFF) {
        byte = get_bits(gb, 8);
        payload_size += byte;
    }
    if (s->nal_unit_type == NAL_SEI_PREFIX) {
        if (payload_type == 256 /*&& s->decode_checksum_sei*/)
            decode_nal_sei_decoded_picture_hash(s, payload_size);
        else if (payload_type == 45)
            decode_nal_sei_frame_packing_arrangement(s->HEVClc);
        else {
            av_log(s->avctx, AV_LOG_DEBUG, "Skipped PREFIX SEI %d\n", payload_type);
            skip_bits(gb, 8*payload_size);
        }
    } else { /* nal_unit_type == NAL_SEI_SUFFIX */
        if (payload_type == 132 /* && s->decode_checksum_sei */)
            decode_nal_sei_decoded_picture_hash(s, payload_size);
        else {
            av_log(s->avctx, AV_LOG_DEBUG, "Skipped SUFFIX SEI %d\n", payload_type);
            skip_bits(gb, 8*payload_size);
        }
    }
    return 0;
}

static int more_rbsp_data(GetBitContext *gb)
{
    return get_bits_left(gb) > 0 && show_bits(gb, 8) != 0x80;
}

int ff_hevc_decode_nal_sei(HEVCContext *s)
{
    do {
        decode_nal_sei_message(s);
    } while (more_rbsp_data(&s->HEVClc->gb));
    return 0;
}
