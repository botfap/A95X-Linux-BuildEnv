/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86cmap.h"
#include <xf86drm.h>
#include "xf86xv.h"
#include "xf86Crtc.h"
#include "micmap.h"

#include "mali_def.h"
#include "mali_fbdev.h"
#include "mali_exa.h"
#include "mali_dri.h"
#include "mali_lcd.h"

#include "compat-api.h"

/*for amlogic hwc support*/
#include "xf86Cursor.h"
#include "cursorstr.h"
#include <inttypes.h>
#include <linux/fb.h>
#include <string.h>
#include <stdlib.h>

#define MALI_VERSION        4000
#define MALI_NAME           "MALI"
#define MALI_DRIVER_NAME    "mali"

#define PAGE_MASK    (~(getpagesize() - 1))

#define DEFAULT_FG 0xff000000
#define DEFAULT_BG 0xffffffff

static uint32_t *cursor_image = 0;
static const OptionInfoRec *MaliAvailableOptions(int chipid, int busid);
static void MaliIdentify(int flags);
static Bool MaliProbe(DriverPtr drv, int flags);
static Bool MaliPreInit(ScrnInfoPtr pScrn, int flags);
static Bool MaliScreenInit(SCREEN_INIT_ARGS_DECL);
static Bool MaliCloseScreen(CLOSE_SCREEN_ARGS_DECL);

static Bool MaliHWSwitchMode(SWITCH_MODE_ARGS_DECL);
static void MaliHWAdjustFrame(ADJUST_FRAME_ARGS_DECL);
static Bool MaliHWEnterVT(VT_FUNC_ARGS_DECL);
static void MaliHWLeaveVT(VT_FUNC_ARGS_DECL);
static ModeStatus MaliHWValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags);

static int pix24bpp = 0;
static int malihwPrivateIndex = -1;
static int global_drm_fd = -1;

_X_EXPORT DriverRec MALI =
{
	MALI_VERSION,
	MALI_DRIVER_NAME,
	MaliIdentify,
	MaliProbe,
	MaliAvailableOptions,
	NULL,
	0,
	NULL,
	NULL,
	NULL,
};

/* Supported "chipsets" */
static SymTabRec MaliChipsets[] =
{
	{ 0, "mali" },
	{ -1, NULL }
};

/* Supported options */
typedef enum
{
	OPTION_SW_CURSOR,
	OPTION_HW_CURSOR,
	OPTION_DRI2,
	OPTION_DRI2_PAGE_FLIP,
	OPTION_DRI2_WAIT_VSYNC,
	OPTION_UMP_CACHED,
	OPTION_UMP_LOCK,
} FBDevOpts;

static const OptionInfoRec MaliOptions[] =
{
	{ OPTION_HW_CURSOR,	"HWCursor",	OPTV_BOOLEAN, {0},	TRUE  },
	{ OPTION_SW_CURSOR,	"SWCursor",	OPTV_BOOLEAN, {0},	FALSE },
	{ OPTION_DRI2,             "DRI2",            OPTV_BOOLEAN, {0}, TRUE  },
	{ OPTION_DRI2_PAGE_FLIP,   "DRI2_PAGE_FLIP",  OPTV_BOOLEAN, {0}, FALSE },
	{ OPTION_DRI2_WAIT_VSYNC,  "DRI2_WAIT_VSYNC", OPTV_BOOLEAN, {0}, FALSE },
	{ OPTION_UMP_CACHED,       "UMP_CACHED",      OPTV_BOOLEAN, {0}, FALSE },
	{ OPTION_UMP_LOCK,         "UMP_LOCK",        OPTV_BOOLEAN, {0}, FALSE },
	{ -1,                      NULL,              OPTV_NONE,    {0}, FALSE }
};

#ifdef XFree86LOADER

#ifndef PACKAGE_VERSION_MAJOR
#define PACKAGE_VERSION_MAJOR 0
#endif
#ifndef PACKAGE_VERSION_MINOR
#define PACKAGE_VERSION_MINOR 1
#endif
#ifndef PACKAGE_VERSION_PATCHLEVEL
#define PACKAGE_VERSION_PATCHLEVEL 1
#endif

MODULESETUPPROTO(MaliSetup);

static XF86ModuleVersionInfo MaliVersRec =
{
	"mali",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData maliModuleData =
{
	&MaliVersRec,
	MaliSetup,
	NULL
};

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

int aml_hw_cursor_set_position(aml_disp_t *ctx, int x, int y)
{
    int result = 1;
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

int aml_hw_cursor_load_image(aml_disp_t *ctx, uint32_t pixeldata[1024])
{
    uint32_t tmp;
    aml_hwc_addr_t hwc;
    hwc.addr = (uintptr_t)&pixeldata[0];
    tmp = (uintptr_t)&hwc;
    ERROR_STR("aml_hw_cursor_load_image hwc.add is 0x%x, tmp is 0x%x\n", hwc.addr, tmp);
    return ioctl(ctx->fd_disp, FBIOPUT_OSD2_CURSOR_DATA, &tmp);
}

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

static void SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    ERROR_STR("SetCurosrColors bg is 0x%x, fg is 0x%x\n",bg, fg);
    aml_disp_t *disp =MALI_DISP(pScrn);
    bg |= 0xff000000;
    fg |= 0xff000000;
    if (cursor_image) {
        int x, y;
        int image_width = 32;
        int image_height = 32;
        uint32_t argb = 0x0;
        /* update color */
        for (y = 0; y < image_height; y++) {
            for (x = 0; x < image_width; x++) {
                argb = 0x0;
                if (cursor_image[y * image_width + x] == DEFAULT_FG) {
                    argb = fg;
                } else if (cursor_image[y * image_width + x] == DEFAULT_BG) {
                    argb = bg;
                }
                cursor_image[y * image_width + x] = argb;
            }
        }
        aml_hw_cursor_load_image(disp, cursor_image);
        free(cursor_image);
        cursor_image = 0;
    }
}

static void LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits)
{
    ERROR_STR("LoadCursorImage\n");
    aml_disp_t *disp = MALI_DISP(pScrn);
    int image_width = 32;
    int image_height = 32;
    int x, y;
    uint32_t argb = 0x0;
    
    uint32_t i = 0;
    uint32_t bitdata;
    uint32_t maskdata;
    if (!cursor_image)
        cursor_image = calloc(32 * 32, 4);
    {
        /* Generate the color_images */
       for (y = 0; y < image_height; y++) {
           bitdata =  bits[3] << 24
               | bits[2] << 16
               | bits[1] << 8
               | bits[0];
           maskdata =  bits[32/8*32+3] << 24
               | bits[32/8*32+2] << 16
               | bits[32/8*32+1] << 8
               | bits[32/8*32+0];
 
           for (x = 0; x < image_width; x++) {
               argb = 0x0;
               if (bitdata & 0x1) {
                   argb =  DEFAULT_FG;
               } else if(maskdata & 0x1) {
                   argb =  DEFAULT_BG;
               }
               cursor_image[y * image_width + x] = argb;
               bitdata >>= 1;
               maskdata >>= 1;
           }
           bits += 4;
       }
    }
}

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
    int source_width  = pCurs->bits->width;
    int source_height = pCurs->bits->height;
    int image_width = 32;
    int image_height = 32;
    int x, y;
	
	if (!cursor_image){
        cursor_image = calloc(32 * 32, 4);
	}

    ERROR_STR("LoadCursorARGB width is %d, height is %d\n", pCurs->bits->width, pCurs->bits->height);
    {
        uint32_t *argb = (uint32_t *)pCurs->bits->argb;

        /* Generate the color_images */
        for (y = 0; y < image_height; y++) {
            for (x = 0; x < image_width; x++) {
	            if(x < source_width && y < source_height){
	                cursor_image[y * image_width + x] = *argb++;
	            }else{
	                cursor_image[y * image_width + x] = 0x0;
	            }
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
    ERROR_STR("aml_hwcursor_init\n");
    if (!disp)
        return NULL;

    ERROR_STR("aml_hwcursor_init\n");
    InfoPtr = xf86CreateCursorInfoRec();
    if(!InfoPtr)
    {
        ERROR_STR("aml_hwcursor_init: xf86CreateCursorInfoRec() failed\n");
        return NULL;
    }

    InfoPtr->ShowCursor = ShowCursor;
    InfoPtr->HideCursor = HideCursor;
    InfoPtr->SetCursorPosition = SetCursorPosition;
    InfoPtr->SetCursorColors = SetCursorColors;
    InfoPtr->LoadCursorImage = LoadCursorImage;
    InfoPtr->MaxWidth = InfoPtr->MaxHeight = 32;
    InfoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP | 
						HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
						HARDWARE_CURSOR_ARGB;

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

aml_disp_t *aml_disp_init(void)
{
    ERROR_STR("enter to aml_disp_init\n");
    aml_disp_t *ctx = calloc(sizeof(aml_disp_t), 1);

    ctx->fd_disp = open("/dev/fb1", O_RDWR);

    /* maybe it's even not a aml hardware */
    if (ctx->fd_disp < 0) {
        ERROR_STR("failed to aml_disp_init fd_disp < 0\n");
        free(ctx);
        return NULL;
    }

    ctx->cursor_enabled = 0;
    ctx->cursor_x = -1;
    ctx->cursor_y = -1;
    ERROR_STR("success to aml_disp_init\n");
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

pointer MaliSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	ERROR_STR("MaliSetup");

	IGNORE(opts);
	IGNORE(errmin);

	if (!setupDone)
	{
		setupDone = TRUE;
		xf86AddDriver(&MALI, module, HaveDriverFuncs);
		return (pointer)1;
	}
	else
	{
		if (errmaj)
		{
			*errmaj = LDR_ONCEONLY;
		}

		return NULL;
	}
}

#endif /* XFree86LOADER */

static Bool MaliGetRec(ScrnInfoPtr pScrn)
{
	TRACE_ENTER();

	if (pScrn->driverPrivate != NULL)
	{
		return TRUE;
	}

	pScrn->driverPrivate = xnfcalloc(sizeof(MaliRec), 1);

	return TRUE;
}

static void MaliFreeRec(ScrnInfoPtr pScrn)
{
	TRACE_ENTER();

	if (pScrn->driverPrivate == NULL)
	{
		return;
	}

	free(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

Bool MaliHWGetRec(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr;

	if (malihwPrivateIndex < 0)
	{
		malihwPrivateIndex = xf86AllocateScrnInfoPrivateIndex();
	}

	if (MALIHWPTR(pScrn) != NULL)
	{
		return TRUE;
	}

	fPtr = MALIHWPTRLVAL(pScrn) = xnfcalloc(sizeof(MaliHWRec), 1);

	return TRUE;
}

void MaliHWFreeRec(ScrnInfoPtr pScrn)
{
	if (malihwPrivateIndex < 0)
	{
		return;
	}

	if (MALIHWPTR(pScrn) == NULL)
	{
		return;
	}

	free(MALIHWPTR(pScrn));
	MALIHWPTRLVAL(pScrn) = NULL;
}

static const OptionInfoRec *MaliAvailableOptions(int chipid, int busid)
{
	ERROR_STR("MaliAvailableOptions");

	IGNORE(chipid);
	IGNORE(busid);

	return MaliOptions;
}

static void MaliIdentify(int flags)
{
	ERROR_STR("MaliIdentify");

	IGNORE(flags);

	xf86PrintChipsets(MALI_NAME, "driver for Mali Framebuffer", MaliChipsets);
}

static Bool fbdev_crtc_config_resize(ScrnInfoPtr pScrn, int width, int height)
{
	MaliPtr fPtr = MALIPTR(pScrn);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	int pitch, i;

	INFO_MSG("%s: width = %d height = %d\n", __FUNCTION__, width, height);

	/* we currently need EXA for this to work */
	if (fPtr->exa == NULL)
	{
		return TRUE;
	}

	/* calculate new pitch, align to any HW requirements if needed */
	pitch = width * (pScrn->bitsPerPixel / 8);

	pScrn->virtualX = width;
	pScrn->virtualY = height;

	/* update pitch setting in EXA */
#if 1
	PixmapPtr frontPixmap = (*pScrn->pScreen->GetScreenPixmap)(pScrn->pScreen);
	PixmapPtr backPixmap  = ((PrivPixmap *)exaGetPixmapDriverPrivate(frontPixmap))->other_buffer;

	backPixmap->devKind = frontPixmap->devKind = pitch;
	backPixmap->drawable.width = frontPixmap->drawable.width = width;
	backPixmap->drawable.height = frontPixmap->drawable.width = height;
#endif
	pScrn->displayWidth = pitch / (pScrn->bitsPerPixel / 8);

	/* reinitialize the crtc to get the new setting */
	for (i = 0; i < xf86_config->num_crtc; i++)
	{
		xf86CrtcPtr crtc = xf86_config->crtc[i];

		if (crtc->enabled)
		{
			xf86CrtcSetMode(crtc, &crtc->mode, crtc->rotation, crtc->x, crtc->y);
		}
	}

	return TRUE;
}

static void mali_check_dri_options(ScrnInfoPtr pScrn)
{
	MaliPtr fPtr = MALIPTR(pScrn);

	fPtr->dri_render = DRI_NONE;

	if (!xf86ReturnOptValBool(fPtr->Options, OPTION_DRI2, TRUE))
	{
		CONFIG_MSG("DRI disabled. No support in config file");
		fPtr->dri_render = DRI_DISABLED;
	}

	if (xf86ReturnOptValBool(fPtr->Options, OPTION_DRI2_PAGE_FLIP, FALSE))
	{
		CONFIG_MSG("DRI Fullscreen page flip enabled");
		fPtr->use_pageflipping = TRUE;
	}
	else
	{
		CONFIG_MSG("DRI Fullscreen page flip disabled. No support in config file");
	}

	if (xf86ReturnOptValBool(fPtr->Options, OPTION_DRI2_WAIT_VSYNC, FALSE))
	{
		CONFIG_MSG("DRI Fullscreen page flip VSYNC'd");
		fPtr->use_pageflipping_vsync = TRUE;
	}
	else
	{
		CONFIG_MSG("DRI Fullscreen page flip VSYNC disabled");
	}

	if (xf86ReturnOptValBool(fPtr->Options, OPTION_UMP_CACHED, FALSE))
	{
		CONFIG_MSG("Use cached UMP memory");
		fPtr->use_cached_ump = TRUE;
	}
	else
	{
		CONFIG_MSG("Use uncached UMP memory");
	}

	if (xf86ReturnOptValBool(fPtr->Options, OPTION_UMP_LOCK, FALSE))
	{
		CONFIG_MSG("Use umplock across processes");
		fPtr->use_umplock = TRUE;
	}
	else
	{
		CONFIG_MSG("Don't use umplock across processes");
	}

	if (pScrn->depth != 16 && pScrn->depth != 24)
	{
		CONFIG_MSG("DRI is disabled since display does not run at 16bpp or 24bpp");
		fPtr->dri_render = DRI_DISABLED;
	}
}

static void mali_check_exa_options(ScrnInfoPtr pScrn)
{
	MaliPtr fPtr = MALIPTR(pScrn);

	IGNORE(fPtr);

	/* EXA specific options checked here */
}

static const xf86CrtcConfigFuncsRec fbdev_crtc_config_funcs =
{
	.resize = fbdev_crtc_config_resize,
};

static void FBDev_crtc_config(ScrnInfoPtr pScrn)
{
	xf86CrtcConfigPtr xf86_config;
	int max_width, max_height;
	TRACE_ENTER();

	/* Allocate an xf86CrtcConfig */
	xf86CrtcConfigInit(pScrn, &fbdev_crtc_config_funcs);
	xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

	max_width = 2048;
	max_height = 2048;

	xf86CrtcSetSizeRange(pScrn, 640, 480, max_width, max_height);
	TRACE_EXIT();
}

static int mali_open(int scrnIndex, const char *device, char **namep)
{
	int fd;
	struct fb_fix_screeninfo fix;


	if (device)
	{
		fd = open(device, O_RDWR, 0);
	}
	else
	{
		device = getenv("FRAMEBUFFER");

		if ((NULL == device) || ((fd = open(device, O_RDWR, 0)) == -1))
		{
			device = "/dev/fb0";
			fd = open(device, O_RDWR, 0);
		}
	}

	if (fd == -1)
	{
		ERROR_STR("open %s: %s", device, strerror(errno));
		return -1;
	}

	if (namep)
	{
		if (-1 == ioctl(fd, FBIOGET_FSCREENINFO, (void *)(&fix)))
		{
			*namep = NULL;
			ERROR_STR("FBIOGET_FSCREENINFO: %s", strerror(errno));
			return -1;
		}
		else
		{
			*namep = xnfalloc(16);
			strncpy(*namep, fix.id, 16);
		}
	}

	return fd;
}

static void calculateFbmem_len(MaliHWPtr fPtr)
{
	fPtr->fboff = (unsigned long) fPtr->fix.smem_start & ~PAGE_MASK;
	fPtr->fbmem_len = (fPtr->fboff + fPtr->fix.smem_len + ~PAGE_MASK) & PAGE_MASK;
}

int MaliHWGetLineLength(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
	TRACE_ENTER();

	if (fPtr->fix.line_length)
	{
		return fPtr->fix.line_length;
	}

	return fPtr->var.xres_virtual * fPtr->var.bits_per_pixel / 8;
}

void *MaliHWMapVidmem(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (NULL == fPtr->fbmem)
	{
		calculateFbmem_len(fPtr);
		fPtr->fbmem = mmap(NULL, fPtr->fbmem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fPtr->fd, 0);

		if (-1 == (long)fPtr->fbmem)
		{
			ERROR_MSG("mmap fbmem: %s\n", strerror(errno));
			fPtr->fbmem = NULL;
		}
		else
		{
		}
	}

	pScrn->memPhysBase = (unsigned long)fPtr->fix.smem_start & (unsigned long)(PAGE_MASK);
	pScrn->fbOffset = (unsigned long)fPtr->fix.smem_start & (unsigned long)(~PAGE_MASK);

	return fPtr->fbmem;
}

Bool MaliHWSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool check)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	IGNORE(fPtr);
	IGNORE(mode);
	IGNORE(check);

	return TRUE;
}

int MaliHWLinearOffset(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	return fPtr->fboff;
}

Bool MaliHWUnmapVidmem(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (NULL != fPtr->fbmem)
	{
		if (-1 == munmap(fPtr->fbmem, fPtr->fbmem_len))
		{
			ERROR_MSG("munmap fbmem: %s\n", strerror(errno));
		}

		fPtr->fbmem = NULL;
	}

	return TRUE;
}

Bool MaliHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	pScrn->vtSema = TRUE;

	if (!MaliHWSetMode(pScrn, mode, FALSE))
	{
		return FALSE;
	}

	if (0 != ioctl(fPtr->fd, FBIOGET_FSCREENINFO, (void *)(&fPtr->fix)))
	{
		ERROR_MSG("FBIOGET_FSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}

	if (0 != ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *)(&fPtr->var)))
	{
		ERROR_MSG("FBIOGET_VSCREENINFO: %s\n", strerror(errno));
		return FALSE;
	}

	if (pScrn->defaultVisual == TrueColor || pScrn->defaultVisual == DirectColor)
	{
		pScrn->offset.red   = fPtr->var.red.offset;
		pScrn->offset.green = fPtr->var.green.offset;
		pScrn->offset.blue  = fPtr->var.blue.offset;
		pScrn->mask.red     = ((1 << fPtr->var.red.length) - 1) << fPtr->var.red.offset;
		pScrn->mask.green   = ((1 << fPtr->var.green.length) - 1) << fPtr->var.green.offset;
		pScrn->mask.blue    = ((1 << fPtr->var.blue.length) - 1) << fPtr->var.blue.offset;
	}

	return TRUE;
}

void MaliHWSave(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (0 != ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *)(&fPtr->saved_var)))
	{
		ERROR_MSG("FBIOGET_VSCREENINFO: %s\n", strerror(errno));
	}
}

void MaliHWRestore(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (0 != ioctl(fPtr->fd, FBIOPUT_VSCREENINFO, (void *)(&fPtr->saved_var)))
	{
		ERROR_MSG("FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
	}
}

Bool MaliHWProbe(char *device, char **namep)
{
	int fd;

	ERROR_STR("MaliHWProbe");

	if ((fd = mali_open(-1, device, namep)) == -1)
	{
		return FALSE;
	}

	close(fd);

	return TRUE;
}

void MaliHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors, VisualPtr pVisual)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
	struct fb_cmap cmap;
	unsigned short red, green, blue;
	int i;

	TRACE_ENTER();
	IGNORE(pVisual);

	cmap.len   = 1;
	cmap.red   = &red;
	cmap.green = &green;
	cmap.blue  = &blue;
	cmap.transp = NULL;

	for (i = 0; i < numColors; i++)
	{
		cmap.start = indices[i];
		red   = (colors[indices[i]].red   << 8) | colors[indices[i]].red;
		green = (colors[indices[i]].green << 8) | colors[indices[i]].green;
		blue  = (colors[indices[i]].blue  << 8) | colors[indices[i]].blue;

		if (-1 == ioctl(fPtr->fd, FBIOPUTCMAP, (void *)&cmap))
		{
			ERROR_MSG("FBIOPUTCMAP: %s\n", strerror(errno));
		}
	}
}

Bool MaliHWSaveScreen(ScreenPtr pScreen, int mode)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
	unsigned long unblank;

	TRACE_ENTER();

	if (!pScrn->vtSema)
	{
		return TRUE;
	}

	unblank = xf86IsUnblank(mode);

	if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *)(1 - unblank)))
	{
		ERROR_MSG("FBIOBLANK: %s\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

static ModeStatus MaliHWValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags)
{
	SCRN_INFO_PTR(arg);

	TRACE_ENTER();

	IGNORE(verbose);
	IGNORE(flags);

	if (!MaliHWSetMode(pScrn, mode, TRUE))
	{
		return MODE_BAD;
	}

	return MODE_OK;
}

static Bool MaliHWSwitchMode(SWITCH_MODE_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);

	TRACE_ENTER();

	if (!MaliHWSetMode(pScrn, mode, FALSE))
	{
		return FALSE;
	}

	return TRUE;
}

static void MaliHWAdjustFrame(ADJUST_FRAME_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (x < 0 || x + fPtr->var.xres > fPtr->var.xres_virtual || y < 0 || y + fPtr->var.yres > fPtr->var.yres_virtual)
	{
		return;
	}

	fPtr->var.xoffset = x;
	fPtr->var.yoffset = y;

	if (-1 == ioctl(fPtr->fd, FBIOPAN_DISPLAY, (void *)&fPtr->var))
	{
		xf86DrvMsgVerb(pScrn->scrnIndex, X_WARNING, 5, "FBIOPAN_DISPLAY: %s\n", strerror(errno));
	}
}

static Bool MaliHWEnterVT(VT_FUNC_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);

	TRACE_ENTER();

	if (!MaliHWModeInit(pScrn, pScrn->currentMode))
	{
		return FALSE;
	}

	MaliHWAdjustFrame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));

	return TRUE;
}

static void MaliHWLeaveVT(VT_FUNC_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);

	TRACE_ENTER();

	MaliHWRestore(pScrn);
}

void MaliHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
    aml_disp_t *disp = MALI_DISP(pScrn);
	unsigned long fbmode;

	TRACE_ENTER();
	IGNORE(flags);

	if (!pScrn->vtSema)
	{
		return;
	}

	switch (mode)
	{
		case DPMSModeOn:
			fbmode = 0;
			break;

		case DPMSModeStandby:
			fbmode = 2;
			break;

		case DPMSModeSuspend:
			fbmode = 3;
			break;

		case DPMSModeOff:
			fbmode = 4;
			break;

		default:
			return;
	}

	if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *)fbmode))
	{
		ERROR_MSG("FBIOBLANK: %s\n", strerror(errno));
	}
    if (fbmode == 0)
        aml_hw_cursor_show(disp);
    else 
        aml_hw_cursor_hide(disp);

}

static Bool MaliProbe(DriverPtr drv, int flags)
{
	int i, numDevSections, entity;
	Bool foundScreen = FALSE;
	ScrnInfoPtr pScrn;
	GDevPtr *devSections;

	ERROR_STR("MaliProbe");

	if (flags & PROBE_DETECT)
	{
		return FALSE;
	}

	if ((numDevSections = xf86MatchDevice(MALI_DRIVER_NAME, &devSections)) <= 0)
	{
		return FALSE;
	}

	for (i = 0; i < numDevSections; i++)
	{
		char *dev = (char *)xf86FindOptionValue(devSections[i]->options, "fbdev");

		if (MaliHWProbe(dev, NULL))
		{
			pScrn = NULL;
			entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
			pScrn = xf86ConfigFbEntity(pScrn, 0, entity, NULL, NULL, NULL, NULL);

			if (pScrn)
			{
				foundScreen = TRUE;

				pScrn->driverVersion = MALI_VERSION;
				pScrn->driverName    = MALI_DRIVER_NAME;
				pScrn->name          = MALI_NAME;
				pScrn->Probe         = MaliProbe;
				pScrn->PreInit       = MaliPreInit;
				pScrn->ScreenInit    = MaliScreenInit;

				pScrn->SwitchMode    = MaliHWSwitchMode;
				pScrn->AdjustFrame   = MaliHWAdjustFrame;
				pScrn->EnterVT       = MaliHWEnterVT;
				pScrn->LeaveVT       = MaliHWLeaveVT;
				pScrn->ValidMode     = MaliHWValidMode;

				INFO_MSG("using %s\n", dev ? dev : "default device");
			}
		}
	}

	free(devSections);

	return foundScreen;
}

Bool MaliHWInit(ScrnInfoPtr pScrn, const char *device)
{
	MaliHWPtr fPtr;

	TRACE_ENTER();

	MaliHWGetRec(pScrn);
	fPtr = MALIHWPTR(pScrn);
	fPtr->fd = mali_open(pScrn->scrnIndex, device, NULL);

	if (-1 == fPtr->fd)
	{
		ERROR_MSG("Failed to open framebuffer!");
		return FALSE;
	}

	if (-1 == ioctl(fPtr->fd, FBIOGET_FSCREENINFO, (void *)(&fPtr->fix)))
	{
		ERROR_MSG("Failed to get fixed info!");
		return FALSE;
	}

	if (-1 == ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *)(&fPtr->var)))
	{
		ERROR_MSG("Failed to get var info!");
		return FALSE;
	}

	return TRUE;

}

int MaliHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();

	if (fbbpp)
	{
		*fbbpp = fPtr->var.bits_per_pixel;
	}

	if (fPtr->fix.visual == FB_VISUAL_TRUECOLOR || fPtr->fix.visual == FB_VISUAL_DIRECTCOLOR)
	{
		return fPtr->var.red.length + fPtr->var.green.length + fPtr->var.blue.length;
	}

	return fPtr->var.bits_per_pixel;
}

int MaliHWGetVidmem(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
	TRACE_ENTER();

	return fPtr->fix.smem_len;
}

char *MaliHWGetName(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);
	TRACE_ENTER();

	return fPtr->fix.id;
}

int MaliHWGetFD(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr;
	TRACE_ENTER();

	MaliHWGetRec(pScrn);
	fPtr = MALIHWPTR(pScrn);

	return fPtr->fd;
}

void MaliHWSetVideoModes(ScrnInfoPtr pScrn)
{
	char **modename;
	DisplayModePtr mode, this, last = pScrn->modes;

	TRACE_ENTER();

	if (NULL == pScrn->display->modes)
	{
		return;
	}

	pScrn->virtualX = pScrn->display->virtualX;
	pScrn->virtualY = pScrn->display->virtualY;

	for (modename = pScrn->display->modes; *modename != NULL; modename++)
	{
		for (mode = pScrn->monitor->Modes; mode != NULL; mode = mode->next)
			if (0 == strcmp(mode->name, *modename))
			{
				break;
			}

		if (NULL == mode)
		{
			INFO_MSG("\tmode \"%s\" not found\n", *modename);
			continue;
		}

		INFO_MSG("\tmodename \"%s\" mode->name \"%s\"\n", *modename, mode->name);

		if (!MaliHWSetMode(pScrn, mode, TRUE))
		{
			INFO_MSG("\tmode \"%s\" test failed\n", *modename);
			continue;
		}

		INFO_MSG("\tmode \"%s\" ok\n", *modename);

		if (pScrn->virtualX < mode->HDisplay)
		{
			pScrn->virtualX = mode->HDisplay;
		}

		if (pScrn->virtualY < mode->VDisplay)
		{
			pScrn->virtualY = mode->VDisplay;
		}

		if (NULL == pScrn->modes)
		{
			this = pScrn->modes = xf86DuplicateMode(mode);
			this->next = this;
			this->prev = this;
		}
		else
		{
			this = xf86DuplicateMode(mode);
			this->next = pScrn->modes;
			this->prev = last;
			last->next = this;
			pScrn->modes->prev = this;
		}

		last = this;
	}
}

void MaliHWUseBuildinMode(ScrnInfoPtr pScrn)
{
	MaliHWPtr fPtr = MALIHWPTR(pScrn);

	TRACE_ENTER();
	pScrn->modes    = &fPtr->buildin;
	pScrn->virtualX = pScrn->display->virtualX;
	pScrn->virtualY = pScrn->display->virtualY;

	if (pScrn->virtualX < fPtr->buildin.HDisplay)
	{
		pScrn->virtualX = fPtr->buildin.HDisplay;
	}

	if (pScrn->virtualY < fPtr->buildin.VDisplay)
	{
		pScrn->virtualY = fPtr->buildin.VDisplay;
	}
}

static void mali_drm_close_master(ScrnInfoPtr pScrn)
{
	MaliPtr fPtr = MALIPTR(pScrn);

    if (fPtr && fPtr->drm_fd > 0)
    {
        INFO_MSG("closing DRM device");
        drmClose(fPtr->drm_fd);
        fPtr->drm_fd = -1;
        global_drm_fd = -1;
    }
}

static Bool mali_drm_open_master(ScrnInfoPtr pScrn)
{
	MaliPtr fPtr = MALIPTR(pScrn);
	drmSetVersion sv;
	int err;

	if (global_drm_fd == -1)
	{
		global_drm_fd = drmOpen("mali_drm", NULL);

		if (global_drm_fd == -1)
		{
			ERROR_MSG("%s Unable to open DRM: %s\n", __func__, strerror(errno));
			return FALSE;
		}
	}

	fPtr->drm_fd = global_drm_fd;
	ERROR_MSG("%s DRM OPEN (fd: 0x%x)\n", __func__, fPtr->drm_fd);

	sv.drm_di_major = 1;
	sv.drm_di_minor = 1;
	sv.drm_dd_major = -1;
	sv.drm_dd_minor = -1;

	err = drmSetInterfaceVersion(fPtr->drm_fd, &sv);

	if (err != 0)
	{
		ERROR_MSG("%s DRM failed to interface version\n", __func__);
		drmClose(fPtr->drm_fd);
		fPtr->drm_fd = -1;
		return FALSE;
	}

	return TRUE;
}

static Bool MaliPreInit(ScrnInfoPtr pScrn, int flags)
{
	MaliPtr fPtr;
	int default_depth, fbbpp;

	TRACE_ENTER();

	if (flags & PROBE_DETECT)
	{
		return FALSE;
	}

	/* Check the number of entities, and fail if it isn't one. */
	if (pScrn->numEntities != 1)
	{
		return FALSE;
	}

	pScrn->monitor = pScrn->confScreen->monitor;

	MaliGetRec(pScrn);
	fPtr = MALIPTR(pScrn);

	fPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
	fPtr->dri_open = FALSE;
	fPtr->dri_render = DRI_NONE;
	fPtr->use_pageflipping = FALSE;
	fPtr->use_pageflipping_vsync = FALSE;
	fPtr->use_cached_ump = FALSE;
	fPtr->use_umplock = FALSE;

	/* open device */
	if (!MaliHWInit(pScrn, xf86FindOptionValue(fPtr->pEnt->device->options, "fbdev")))
	{
		return FALSE;
	}

	default_depth = MaliHWGetDepth(pScrn, &fbbpp);

	if (!xf86SetDepthBpp(pScrn, default_depth, default_depth, fbbpp, Support24bppFb | Support32bppFb | SupportConvert32to24 | SupportConvert24to32))
	{
		return FALSE;
	}

	xf86PrintDepthBpp(pScrn);

	/* Get the depth24 pixmap format */
	if (pScrn->depth == 24 && pix24bpp == 0)
	{
		pix24bpp = xf86GetBppFromDepth(pScrn, 24);
	}

	/* color weight */
	if (pScrn->depth > 8)
	{
		rgb zeros = { 0, 0, 0 };

		if (!xf86SetWeight(pScrn, zeros, zeros))
		{
			return FALSE;
		}
	}

	/* visual init */
	if (!xf86SetDefaultVisual(pScrn, -1))
	{
		return FALSE;
	}

	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor)
	{
		ERROR_MSG("requested default visual (%s) is not supported at depth %d\n", xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
		return FALSE;
	}

	{
		Gamma zeros = {0.0, 0.0, 0.0};

		if (!xf86SetGamma(pScrn, zeros))
		{
			return FALSE;
		}
	}

	pScrn->progClock = TRUE;
	pScrn->rgbBits   = 8;
	pScrn->chipset   = "mali";
	pScrn->videoRam  = MaliHWGetVidmem(pScrn);

	INFO_MSG("hardware: %s (video memory: %dkB)\n", MaliHWGetName(pScrn), pScrn->videoRam / 1024);

	/* handle options */
	xf86CollectOptions(pScrn, NULL);

	if (!(fPtr->Options = malloc(sizeof(MaliOptions))))
	{
		return FALSE;
	}

	memcpy(fPtr->Options, MaliOptions, sizeof(MaliOptions));
	xf86ProcessOptions(pScrn->scrnIndex, fPtr->pEnt->device->options, fPtr->Options);

	mali_check_dri_options(pScrn);

	if (fPtr->dri_render == DRI_NONE && FALSE == mali_drm_open_master(pScrn))
	{
		ERROR_MSG("Failed to be master of DRM!");
		return FALSE;
	}

	mali_check_dri_options(pScrn);
	mali_check_exa_options(pScrn);

	fPtr->fb_lcd_fd = MaliHWGetFD(pScrn);

	if (ioctl(fPtr->fb_lcd_fd, FBIOGET_FSCREENINFO, &fPtr->fb_lcd_fix))
	{
		ERROR_MSG("FBIOGET_FSCREENINFO failed!");
		return FALSE;
	}

	if (ioctl(fPtr->fb_lcd_fd, FBIOGET_VSCREENINFO, &fPtr->fb_lcd_var))
	{
		ERROR_MSG("FBIOGET_VSCREENINFO failed!");
		return FALSE;
	}

	pScrn->frameX0 = 0;
	pScrn->frameY0 = 0;
	pScrn->frameX1 = fPtr->fb_lcd_var.xres;
	pScrn->frameY1 = fPtr->fb_lcd_var.yres;

	FBDev_crtc_config(pScrn);

	if (!FBDEV_lcd_init(pScrn))
	{
		ERROR_MSG("FBDev_lcd_init failed!");
		return FALSE;
	}

	if (!xf86InitialConfiguration(pScrn, TRUE))
	{
		ERROR_MSG("xf86InitialConfiguration failed!");
		return FALSE;
	}

	/* collect the available video modes */
	MaliHWSetVideoModes(pScrn);
	{
		DisplayModePtr mode, first = mode = pScrn->modes;

		if (mode != NULL)
		{
			do
			{
				mode->status = xf86CheckModeForMonitor(mode, pScrn->monitor);
				mode = mode->next;
			}
			while (mode != NULL && mode != first);
		}

		xf86PruneDriverModes(pScrn);
	}

	if (NULL == pScrn->modes)
	{
		ERROR_MSG("Failed to get video modes!");
	}

	pScrn->currentMode = pScrn->modes;


	pScrn->displayWidth = pScrn->virtualX;

	xf86PrintModes(pScrn);
	xf86SetDpi(pScrn, 0, 0);

	if (xf86LoadSubModule(pScrn, "fb") == NULL)
	{
		MaliFreeRec(pScrn);
		return FALSE;
	}

	return TRUE;
}

static Bool MaliScreenInit(SCREEN_INIT_ARGS_DECL)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	MaliPtr fPtr = MALIPTR(pScrn);
	VisualPtr visual;
	int init_picture = 0;
	int ret, flags;

	TRACE_ENTER();
	IGNORE(argc);
	IGNORE(argv);

#if DEBUG
	ERROR_MSG("\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n\tmask: %x,%x,%x, offset: %d,%d,%d\n",
	          pScrn->bitsPerPixel,
	          pScrn->depth,
	          xf86GetVisualName(pScrn->defaultVisual),
	          pScrn->mask.red, pScrn->mask.green, pScrn->mask.blue,
	          pScrn->offset.red, pScrn->offset.green, pScrn->offset.blue);
#endif

	if (fPtr->dri_render == DRI_NONE)
    {
        if (FALSE == mali_drm_open_master(pScrn))
        {
            ERROR_MSG("fail to init mali drm");
        }
        if (TRUE == MaliDRI2ScreenInit(pScreen))
        {
            fPtr->dri_render = DRI_2;
            fPtr->dri_open = TRUE;
        }
        else
        {
            ERROR_MSG("DRI2 initialization failed");
        }
    }

	if (NULL == (fPtr->fbmem = MaliHWMapVidmem(pScrn)))
	{
		ERROR_MSG("mapping of video memory failed");
		return FALSE;
	}

	fPtr->fboff = MaliHWLinearOffset(pScrn);

	MaliHWSave(pScrn);

	if (!MaliHWModeInit(pScrn, pScrn->currentMode))
	{
		ERROR_MSG("mode initialization failed");
		return FALSE;
	}

	MaliHWSaveScreen(pScreen, SCREEN_SAVER_ON);
	MaliHWAdjustFrame(ADJUST_FRAME_ARGS(pScrn, 0, 0));

	/* mi layer */
	miClearVisualTypes();

	if (pScrn->bitsPerPixel > 8)
	{
		if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor))
		{
			ERROR_MSG("visual type setup failed for %d bits per pixel [1]", pScrn->bitsPerPixel);
			return FALSE;
		}
	}
	else
	{
		if (!miSetVisualTypes(pScrn->depth, miGetDefaultVisualMask(pScrn->depth), pScrn->rgbBits, pScrn->defaultVisual))
		{
			ERROR_MSG("visual type setup failed for %d bits per pixel [2]", pScrn->bitsPerPixel);
			return FALSE;
		}
	}

	if (!miSetPixmapDepths())
	{
		ERROR_MSG("pixmap depth setup failed");
		return FALSE;
	}

	fPtr->fbstart = fPtr->fbmem + fPtr->fboff;

	ret = fbScreenInit(pScreen, fPtr->fbstart, pScrn->virtualX,
	                   pScrn->virtualY, pScrn->xDpi,
	                   pScrn->yDpi, pScrn->displayWidth,
	                   pScrn->bitsPerPixel);
	init_picture = 1;

	if (pScrn->bitsPerPixel > 8)
	{
		/* Fixup RGB ordering */
		visual = pScreen->visuals + pScreen->numVisuals;

		while (--visual >= pScreen->visuals)
		{
			if ((visual->class | DynamicClass) == DirectColor)
			{
				visual->offsetRed   = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue  = pScrn->offset.blue;
				visual->redMask     = pScrn->mask.red;
				visual->greenMask   = pScrn->mask.green;
				visual->blueMask    = pScrn->mask.blue;
			}
		}
	}

	/* must be after RGB ordering fixed */
	if (init_picture && !fbPictureInit(pScreen, NULL, 0))
	{
		WARNING_MSG("Render extension initialisation failed");
	}

	xf86SetBlackWhitePixels(pScreen);

	xf86LoadSubModule(pScrn, "exa");
	fPtr->exa = exaDriverAlloc();

	if (maliSetupExa(pScreen, fPtr->exa))
	{
		WARNING_MSG("Initializing EXA Driver!");
		exaDriverInit(pScreen, fPtr->exa);
	}
	else
	{
		WARNING_MSG("Failed allocating EXA Driver!");
		free(fPtr->exa);
		fPtr->exa = NULL;
	}

	xf86SetBackingStore(pScreen);
	xf86SetSilkenMouse(pScreen);

	/* software cursor */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	xf86SetDesiredModes(pScrn);

	if (!xf86CrtcScreenInit(pScreen))
	{
		ERROR_MSG("xf86CrtcScreenInit failed");
		return FALSE;
	}

	if (!miCreateDefColormap(pScreen))
	{
		ERROR_MSG("internal error: miCreateDefColormap failed in FBDevScreenInit()");
		return FALSE;
	}

	flags = CMAP_PALETTED_TRUECOLOR;

	if (!xf86HandleColormaps(pScreen, 256, 8, MaliHWLoadPalette, NULL, flags))
	{
		return FALSE;
	}

	xf86DPMSInit(pScreen, MaliHWDPMSSet, 0);
	pScreen->SaveScreen = MaliHWSaveScreen;

	/* Wrap the current CloseScreen function */
	fPtr->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = MaliCloseScreen;

	{
		XF86VideoAdaptorPtr *ptr;
		int n = xf86XVListGenericAdaptors(pScrn, &ptr);
		if (n)
		{
			xf86XVScreenInit(pScreen, ptr, n);
		}
	}

	fPtr->aml_disp_private = aml_disp_init();

	if (!fPtr->aml_disp_private) {
		ERROR_STR("failed to enable the hw cursor of M8\n");
	}else{
		if (!xf86ReturnOptValBool(fPtr->Options, OPTION_SW_CURSOR, FALSE) &&
		     xf86ReturnOptValBool(fPtr->Options, OPTION_HW_CURSOR, TRUE)) {
		    fPtr->aml_hwcursor_private = aml_hwcursor_init(pScreen);

		    if (fPtr->aml_hwcursor_private)
		    {
				ERROR_STR("using hardware cursor\n");
		    }
		    else
		    {
				ERROR_STR("failed to enable hardware cursor\n");
		    }
		}
	}

	if (fPtr->use_umplock)
	{
		fPtr->fd_umplock = open("/dev/umplock", O_RDWR);

		if (-1 == fPtr->fd_umplock)
		{
			WARNING_MSG("Failed to open umplock device!");
		}
		else
		{
			INFO_MSG("Opened umplock device successfully!");
		}
	}
	else
	{
		fPtr->fd_umplock = -1;
	}

	return TRUE;
}

static Bool MaliCloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	MaliPtr fPtr = MALIPTR(pScrn);

	TRACE_ENTER();

	MaliHWRestore(pScrn);
	MaliHWUnmapVidmem(pScrn);
	pScrn->vtSema = FALSE;

	pScreen->CreateScreenResources = fPtr->CreateScreenResources;
	pScreen->CloseScreen = fPtr->CloseScreen;

	(*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);

	if (fPtr->dri_open && fPtr->dri_render == DRI_2)
	{
		fPtr->dri_open = FALSE;
		MaliDRI2CloseScreen(pScreen);
		mali_drm_close_master(pScrn);
	}

	if (fPtr->fd_umplock > 0)
	{
		close(fPtr->fd_umplock);
		fPtr->fd_umplock = -1;
	}

	if (fPtr->aml_hwcursor_private) {
	    aml_hwcursor_close(pScreen);
	    free(fPtr->aml_hwcursor_private);
	    fPtr->aml_hwcursor_private = NULL;
	}
	
	if (fPtr->aml_disp_private) {
	    aml_disp_close(fPtr->aml_disp_private);
	    fPtr->aml_disp_private = NULL;
	}
	return TRUE;
}

