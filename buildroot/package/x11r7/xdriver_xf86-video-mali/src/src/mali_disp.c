/*
 * Copyright © 2013 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <inttypes.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "mali_disp.h"

/*****************************************************************************/
aml_disp_t *aml_disp_init(void)
{
    aml_disp_t *ctx = calloc(sizeof(aml_disp_t), 1);

    ctx->fd_disp = open("/dev/fb1", O_RDWR);

    /* maybe it's even not a aml hardware */
    if (ctx->fd_disp < 0) {
        free(ctx);
        return NULL;
    }

    ctx->cursor_enabled = 0;
    ctx->cursor_x = -1;
    ctx->cursor_y = -1;

    return ctx;
}

int aml_disp_close(aml_disp_t *ctx)
{
    if (ctx->fd_disp >= 0) {
        /* disable cursor */
        if (ctx->cursor_enabled)
            aml_hw_cursor_hide(ctx);

        close(ctx->fd_disp);
        ctx->fd_disp = -1;
        free(ctx);
    }
    return 0;
}

int aml_hw_cursor_set_position(aml_disp_t *ctx, int x, int y)
{
    int result;
    struct fb_cursor cinfo;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    cinfo.hot.x = x;
    cinfo.hot.y = y;

    result = ioctl(ctx->fd_disp, FBIO_CURSOR, &cinfo);
    if (result >= 0) {
        ctx->cursor_x = x;
        ctx->cursor_y = y;
    }
    return result;
}

#if 0
int aml_hw_cursor_load_palette(aml_disp_t *ctx, uint32_t *palette, int n)
{
    uint32_t tmp[4];
    tmp[0] = ctx->fb_id;
    tmp[1] = (uintptr_t)palette;
    tmp[2] = 0;
    tmp[3] = n * sizeof(uint32_t);
    return ioctl(ctx->fd_disp, DISP_CMD_HWC_SET_PALETTE_TABLE, &tmp);
}
#endif

int aml_hw_cursor_load_image(aml_disp_t *ctx, uint32_t pixeldata[1024])
{
    uint32_t tmp;
    __disp_hwc_pattern_t hwc;
    hwc.addr = (uintptr_t)&pixeldata[0];
    tmp = (uintptr_t)&hwc;
    //return ioctl(ctx->fd_disp, DISP_CMD_HWC_SET_FB, &tmp);
    return 1;
}

int aml_hw_cursor_show(aml_disp_t *ctx)
{
    int result;
    result = ioctl(ctx->fd_disp, FBIOBLANK, 0);
    if (result >= 0)
        ctx->cursor_enabled = 1;
    return result;
}

int aml_hw_cursor_hide(aml_disp_t *ctx)
{
    int result;
    result = ioctl(ctx->fd_disp, FBIOBLANK, 1);
    if (result >= 0)
        ctx->cursor_enabled = 0;
    return result;
}


