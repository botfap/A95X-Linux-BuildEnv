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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86Cursor.h"
#include "cursorstr.h"

#include "mali_disp.h"
#include "mali_disp_hwcursor.h"

static void ShowCursor(ScrnInfoPtr pScrn)
{
    aml_disp_t *disp = MALI_DISP(pScrn);
    aml_hw_cursor_show(disp);
}

static void HideCursor(ScrnInfoPtr pScrn)
{
    aml_disp_t *disp = MALI_DISP(pScrn);
    aml_hw_cursor_hide(disp);
}

static void SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    aml_disp_t *disp = MALI_DISP(pScrn);
    aml_hw_cursor_set_position(disp, x, y);
}

#if 0
static void SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    aml_disp_t *disp = MALI_DISP(pScrn);
    uint32_t palette[4] = { 0, 0, bg | 0xFF000000, fg | 0xFF000000 };
    aml_hw_cursor_load_palette(disp, &palette[0], 4);
}

static void LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits)
{
    AmlDispHardwareCursor *private = MALI_DISP_HWC(pScrn);
    aml_disp_t *disp = MALI_DISP(pScrn);
    aml_hw_cursor_load_image(disp, bits);
    if (private->EnableHWCursor)
        (*private->EnableHWCursor) (pScrn);
}
#endif

static Bool UseHWCursorARGB(ScreenPtr pScreen, CursorPtr pCurs)
{
    /* We support ARGB cursors up to 32x32 */
    if (pCurs->bits->height <= 32 && pCurs->bits->width <= 32) {
        return TRUE;
    }

    return FALSE;
}

static void LoadCursorARGB(ScrnInfoPtr pScrn, CursorPtr pCurs)
{
    aml_disp_t *disp = MALI_DISP(pScrn);
    int           width  = pCurs->bits->width;
    int           height = pCurs->bits->height;
    uint32_t      *cursor_image = calloc(32 * 32, 4);

    {
        int               x, y;
        uint32_t     *argb = (uint32_t *)pCurs->bits->argb;

        /* Generate the color_images */
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                uint32_t color = *argb++;
	       cursor_image[y * 32 + x] = color;
            }
        }
    }

    aml_hw_cursor_load_image(disp, cursor_image);
    free(cursor_image);
}

aml_hwcursor_t *aml_hwcursor_init(ScreenPtr pScreen)
{
    xf86CursorInfoPtr InfoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    aml_disp_t *disp = MALI_DISP(pScrn);
    aml_hwcursor_t *private;

    if (!disp)
        return NULL;

    if (!(InfoPtr = xf86CreateCursorInfoRec())) {
        ERROR_STR("aml_hwcursor_init: xf86CreateCursorInfoRec() failed\n");
        return NULL;
    }

    InfoPtr->ShowCursor = ShowCursor;
    InfoPtr->HideCursor = HideCursor;
    InfoPtr->SetCursorPosition = SetCursorPosition;
    InfoPtr->SetCursorColors = NULL;
    InfoPtr->LoadCursorImage = NULL;
    InfoPtr->MaxWidth = InfoPtr->MaxHeight = 32;
    InfoPtr->Flags = HARDWARE_CURSOR_ARGB;

    InfoPtr->UseHWCursorARGB = UseHWCursorARGB;
    InfoPtr->LoadCursorARGB = LoadCursorARGB;

    if (!xf86InitCursor(pScreen, InfoPtr)) {
        ERROR_STR("aml_hwcursor_init: xf86InitCursor(pScreen, InfoPtr) failed\n");
        xf86DestroyCursorInfoRec(InfoPtr);
        return NULL;
    }

    private = calloc(1, sizeof(aml_hwcursor_t));
    if (!private) {
        ERROR_STR("aml_hwcursor_init: calloc failed\n");
        xf86DestroyCursorInfoRec(InfoPtr);
        return NULL;
    }

    private->hwcursor = InfoPtr;
    return private;
}

void aml_hwcursor_close(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    aml_hwcursor_t *private = MALI_DISP_HWC(pScrn);
    if (private) {
        xf86DestroyCursorInfoRec(private->hwcursor);
    }
}

