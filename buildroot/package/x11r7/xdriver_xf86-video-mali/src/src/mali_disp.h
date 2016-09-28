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

#ifndef MALI_DISP_H
#define MALI_DISP_H

#include <inttypes.h>


/*
 * Support for Amlogic M8 display controller features such as layers
 * and hardware cursor
 */

typedef struct {
	unsigned int addr;
} __disp_hwc_pattern_t;

typedef struct {
    int                 fd_disp;

    /* Hardware cursor support */
    int                  cursor_enabled;
    int                  cursor_x, cursor_y;
} aml_disp_t;

aml_disp_t *aml_disp_init(void);
int aml_disp_close(aml_disp_t *ctx);

/*
 * Support for hardware cursor
 * four 32-bit ARGB entries in the palette.
 */
int aml_hw_cursor_set_position(aml_disp_t *ctx, int x, int y);
int aml_hw_cursor_show(aml_disp_t *ctx);
int aml_hw_cursor_hide(aml_disp_t *ctx);

#endif

