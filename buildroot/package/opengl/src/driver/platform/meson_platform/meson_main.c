/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for:
 * - Realview Versatile platforms with ARM11 Mpcore and virtex 5.
 * - Versatile Express platforms with ARM Cortex-A9 and virtex 6.
 */
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#include <linux/module.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <asm/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"

#include <linux/kernel.h>
#include <asm/io.h>
#include <mach/am_regs.h>
#include <linux/module.h>
#include "mali_platform.h"


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
extern struct platform_device meson_device_pd[];
#else
extern struct platform_device meson_device_pd[];
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0) */

static void mali_platform_device_release(struct device *device);
static int mali_os_suspend(struct device *device);
static int mali_os_resume(struct device *device);
static int mali_os_freeze(struct device *device);
static int mali_os_thaw(struct device *device);
#ifdef CONFIG_PM_RUNTIME
static int mali_runtime_suspend(struct device *device);
static int mali_runtime_resume(struct device *device);
static int mali_runtime_idle(struct device *device);
#endif

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6TV

#define INT_MALI_GP      (48+32)
#define INT_MALI_GP_MMU  (49+32)
#define INT_MALI_PP      (50+32)
#define INT_MALI_PP2     (58+32)
#define INT_MALI_PP3     (60+32)
#define INT_MALI_PP4     (62+32)
#define INT_MALI_PP_MMU  (51+32)
#define INT_MALI_PP2_MMU (59+32)
#define INT_MALI_PP3_MMU (61+32)
#define INT_MALI_PP4_MMU (63+32)

#ifndef CONFIG_MALI400_4_PP
static struct resource meson_mali_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP2(0xd0060000, 
			INT_MALI_GP, INT_MALI_GP_MMU, 
			INT_MALI_PP, INT_MALI_PP_MMU, 
			INT_MALI_PP2, INT_MALI_PP2_MMU)
};
#else
static struct resource meson_mali_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP4(0xd0060000, 
			INT_MALI_GP, INT_MALI_GP_MMU, 
			INT_MALI_PP, INT_MALI_PP_MMU, 
			INT_MALI_PP2, INT_MALI_PP2_MMU,
			INT_MALI_PP3, INT_MALI_PP3_MMU,
			INT_MALI_PP4, INT_MALI_PP4_MMU
			)
};
#endif

#elif MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6

int static_pp_mmu_cnt;

#define INT_MALI_GP      (48+32)
#define INT_MALI_GP_MMU  (49+32)
#define INT_MALI_PP      (50+32)
#define INT_MALI_PP_MMU  (51+32)
#define INT_MALI_PP2_MMU ( 6+32)

static struct resource meson_mali_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP2(0xd0060000, 
			INT_MALI_GP, INT_MALI_GP_MMU, 
			INT_MALI_PP, INT_MALI_PP2_MMU, 
			INT_MALI_PP_MMU, INT_MALI_PP2_MMU)
};

#else

#define INT_MALI_GP	48
#define INT_MALI_GP_MMU 49
#define INT_MALI_PP	50
#define INT_MALI_PP_MMU 51

static struct resource meson_mali_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP1(0xd0060000, 
			INT_MALI_GP, INT_MALI_GP_MMU, INT_MALI_PP, INT_MALI_PP_MMU)
};
#endif

static struct dev_pm_ops mali_gpu_device_type_pm_ops =
{
	.suspend = mali_os_suspend,
	.resume = mali_os_resume,
	.freeze = mali_os_freeze,
	.thaw = mali_os_thaw,
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = mali_runtime_suspend,
	.runtime_resume = mali_runtime_resume,
	.runtime_idle = mali_runtime_idle,
#endif
};

static struct device_type mali_gpu_device_device_type =
{
	.pm = &mali_gpu_device_type_pm_ops,
};

static struct platform_device mali_gpu_device =
{
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,

	.dev.parent = NULL,

	.dev.release = mali_platform_device_release,
	/*
	 * We temporarily make use of a device type so that we can control the Mali power
	 * from within the mali.ko (since the default platform bus implementation will not do that).
	 * Ideally .dev.pm_domain should be used instead, as this is the new framework designed
	 * to control the power of devices.
	 */
	.dev.type = &mali_gpu_device_device_type, /* We should probably use the pm_domain instead of type on newer kernels */
};

static struct mali_gpu_device_data mali_gpu_data =
{
	.shared_mem_size =CONFIG_MALI400_OS_MEMORY_SIZE * 1024 * 1024, /* 256MB */
	.fb_start = 0x84000000,
	.fb_size = 0x06000000,
};

int mali_platform_device_register(void)
{
	int err = -1;

#	if MESON_CPU_TYPE == MESON_CPU_TYPE_MESON6
	static_pp_mmu_cnt = 1;
#	endif

	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

	/* Detect present Mali GPU and connect the correct resources to the device */
	
	MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP8 device\n"));
	err = platform_device_add_resources(&mali_gpu_device, meson_mali_resources, sizeof(meson_mali_resources) / sizeof(meson_mali_resources[0]));

	if (0 == err)
	{
		err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
		if (0 == err)
		{
			/* Register the platform device */
			err = platform_device_register(&mali_gpu_device);
			if (0 == err)
			{
				mali_platform_init();
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif

				return 0;
			}
		}

		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

	mali_platform_deinit();

	platform_device_unregister(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

static int mali_os_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_suspend() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->suspend(device);
	}

	mali_platform_power_mode_change(MALI_POWER_MODE_DEEP_SLEEP);

	return ret;
}

static int mali_os_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_resume() called\n"));

	mali_platform_power_mode_change(MALI_POWER_MODE_ON);

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->resume(device);
	}

	return ret;
}

static int mali_os_freeze(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_freeze() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->freeze)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->freeze(device);
	}

	return ret;
}

static int mali_os_thaw(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_thaw() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->thaw)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->thaw(device);
	}

	return ret;
}

#ifdef CONFIG_PM_RUNTIME
static int mali_runtime_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_suspend() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_suspend(device);
	}

	mali_platform_power_mode_change(MALI_POWER_MODE_LIGHT_SLEEP);

	return ret;
}

static int mali_runtime_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_resume() called\n"));

	mali_platform_power_mode_change(MALI_POWER_MODE_ON);

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_resume(device);
	}

	return ret;
}

static int mali_runtime_idle(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_runtime_idle() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_idle)
	{
		/* Need to notify Mali driver about this event */
		int ret = device->driver->pm->runtime_idle(device);
		if (0 != ret)
		{
			return ret;
		}
	}

	pm_runtime_suspend(device);

	return 0;
}
#endif
