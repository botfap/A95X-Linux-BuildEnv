/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief OSD驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/

#ifndef _AM_OSD_H
#define _AM_OSD_H

#include "am_types.h"
#include "am_evt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief OSD模块错误代码*/
enum AM_OSD_ErrorCode
{
	AM_OSD_ERROR_BASE=AM_ERROR_BASE(AM_MOD_OSD),
	AM_OSD_ERR_NO_MEM,                   /**< 内存不足*/
	AM_OSD_ERR_BUSY,                     /**< 设备已经打开*/
	AM_OSD_ERR_INVALID_DEV_NO,           /**< 无效的设备号*/
	AM_OSD_ERR_NOT_OPENNED,              /**< 设备还没有打开*/
	AM_OSD_ERR_CANNOT_CREATE_THREAD,     /**< 无法创建线程*/
	AM_OSD_ERR_NOT_SUPPORTED,            /**< 设备不支持此功能*/
	AM_OSD_ERR_CANNOT_OPEN,              /**< 无法打开设备*/
	AM_OSD_ERR_ILLEGAL_PIXEL,            /**< 无效的像素值*/
	AM_OSD_ERR_SYS,                      /**< 系统错误*/
	AM_OSD_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 图形模式类型*/
typedef enum
{
	AM_OSD_FMT_PALETTE_256,  /**< 256色调色板模式*/
	AM_OSD_FMT_COLOR_4444,   /**< 真彩a:4 r:4 g:4 b:4*/
	AM_OSD_FMT_COLOR_1555,   /**< 真彩a:1 r:5 g:5 b:5*/
	AM_OSD_FMT_COLOR_565,    /**< 真彩r:5 g:6 b:5*/
	AM_OSD_FMT_COLOR_888,    /**< 真彩r:8 g:8 b:8*/
	AM_OSD_FMT_COLOR_8888,   /**< 真彩a:8 r:8 g:8 b:8*/
	AM_OSD_FMT_YUV_420,      /**< YUV4:2:0模式，此模式在JPEG解码过程中使用*/
	AM_OSD_FMT_COUNT
} AM_OSD_PixelFormatType_t;

/**\brief 图形模式是否为调色板方式*/
#define AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(t)    ((t)<=AM_OSD_FMT_PALETTE_256)

/**\brief 图形模式是否为YUV模式*/
#define AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(t)        ((t)>=AM_OSD_FMT_YUV_420)

/**\brief 颜色*/
typedef struct
{
	uint8_t r; /**< 红*/
	uint8_t g; /**< 绿*/
	uint8_t b; /**< 蓝*/
	uint8_t a; /**< alpah*/
} AM_OSD_Color_t;

/**\brief 调色板*/
typedef struct
{
	int             color_count; /**< 调色板中的颜色数*/
	AM_OSD_Color_t *colors;      /**< 颜色缓冲区*/
} AM_OSD_Palette_t;
 
/**\brief 图形模式*/
typedef struct
{
	AM_OSD_PixelFormatType_t type;            /**< 显示模式类型*/
	int                      bits_per_pixel;  /**< 每个像素占用的位数*/
	int                      bytes_per_pixel; /**< 每个像素占用的字节数*/
	int                      planes;          /**< 像素缓冲区数目*/
	uint32_t                 a_mask;          /**< 像素中alpha值掩码*/
	uint32_t                 r_mask;          /**< 像素中红色值掩码*/
	uint32_t                 g_mask;          /**< 像素中绿色值掩码*/
	uint32_t                 b_mask;          /**< 像素中蓝色值掩码*/
	uint8_t                  a_shift;         /**< 像素中alpha值的偏移*/
	uint8_t                  r_shift;         /**< 像素中红色值的偏移*/
	uint8_t                  g_shift;         /**< 像素中绿色值的偏移*/
	uint8_t                  b_shift;         /**< 像素中蓝色值的偏移*/
	AM_OSD_Palette_t         palette;         /**< 调色板*/
} AM_OSD_PixelFormat_t;

/**\brief OSD设备开启参数*/
typedef struct
{
	AM_OSD_PixelFormatType_t format;          /**< 显示模式*/
	int                      width;           /**< 宽度像素数*/
	int                      height;          /**< 高度像素数*/
	int                      output_width;    /**< 视频输出宽度*/
	int                      output_height;   /**< 视频输出高度*/
	AM_Bool_t                enable_double_buffer; /**< 是否支持双缓冲*/
	int                      vout_dev_no;     /**< OSD对应的视频输出设备ID*/
} AM_OSD_OpenPara_t;

/**\brief 矩形*/
typedef struct
{
	int x;  /**< X坐标*/
	int y;  /**< Y坐标*/
	int w;  /**< 宽度*/
	int h;  /**< 高度*/
} AM_OSD_Rect_t;

/**\brief 绘图表面*/
typedef struct
{
	uint32_t   flags;       /**< 绘图表面功能标志*/
	AM_OSD_PixelFormat_t    *format; /**< 图形模式*/
	AM_OSD_Rect_t            clip;   /**< 剪切区域*/
	int        width;       /**< 宽度*/
	int        height;      /**< 高度*/
	int        pitch;       /**< 每行所占字节数*/
	uint8_t   *buffer;      /**< 像素缓冲区*/
	void      *drv_data;    /**< 驱动私有数据*/
	void      *hw_data;     /**< 硬件绘图相关数据*/
} AM_OSD_Surface_t;

/**\brief Blit 操作参数*/
typedef struct
{
	AM_Bool_t  enable_alpha;/**< 是否进行alpha blending*/
	AM_Bool_t  enable_key;  /**< 是否支持color key*/
	uint32_t   key;         /**< Color Key值*/
} AM_OSD_BlitPara_t;

/**\brief 绘图表面支持硬件加速*/
#define AM_OSD_SURFACE_FL_HW     (1)

/**\brief 绘图表面支持openGL*/
#define AM_OSD_SURFACE_FL_OPENGL (2)

/**\brief 绘图表面为OSD*/
#define AM_OSD_SURFACE_FL_OSD    (4)

/**\brief 绘图表面为OSD的双缓冲*/
#define AM_OSD_SURFACE_FL_DBUF   (8)

/**\brief 外部分配的缓冲区，AM_OSD_DestroySurface时不释放*/
#define AM_OSD_SURFACE_FL_EXTERNAL (16)

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开一个OSD设备
 * \param dev_no OSD设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Open(int dev_no, AM_OSD_OpenPara_t *para);

/**\brief 关闭一个OSD设备
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Close(int dev_no);

/**\brief 取得OSD设备的绘图表面
 * \param dev_no OSD设备号
 * \param[out] s 返回OSD绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_GetSurface(int dev_no, AM_OSD_Surface_t **s);

/**\brief 取得OSD设备的显示绘图表面，如果OSD没有使用双缓冲，则返回的Surface和AM_OSD_GetSurface的返回值相同
 * \param dev_no OSD设备号
 * \param[out] s 返回OSD绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_GetDisplaySurface(int dev_no, AM_OSD_Surface_t **s);

/**\brief 显示OSD层
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Enable(int dev_no);

/**\brief 不显示OSD层
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Disable(int dev_no);

/**\brief 双缓冲模式下，将缓冲区中的数据显示到OSD中
 * \param dev_no OSD设备号
 * \param[in] rect 要刷新的区域，NULL表示整个OSD
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Update(int dev_no, AM_OSD_Rect_t *rect);

/**\brief 设定OSD的全局Alpha值
 * \param dev_no OSD设备号
 * \param alpha Alpha值(0~255)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_SetGlobalAlpha(int dev_no, uint8_t alpha);

/**\brief 取得图形模式相关参数
 * \param type 图形模式类型
 * \param[out] fmt 返回图形模式相关参数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_GetFormatPara(AM_OSD_PixelFormatType_t type, AM_OSD_PixelFormat_t **fmt);

/**\brief 将颜色映射为像素值
 * \param[in] fmt 图形模式 
 * \param[in] col 颜色值
 * \param[out] pix 返回映射的像素值 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_MapColor(AM_OSD_PixelFormat_t *fmt, AM_OSD_Color_t *col, uint32_t *pix);

/**\brief 将像素映射为颜色
 * \param[in] fmt 图形模式 
 * \param[in] pix 像素值
 * \param[out] col 返回映射的颜色值 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_MapPixel(AM_OSD_PixelFormat_t *fmt, uint32_t pix, AM_OSD_Color_t *col);

/**\brief 创建一个绘图表面
 * \param type 图形模式 
 * \param w 绘图表面宽度
 * \param h 绘图表面高度
 * \param flags 绘图表面的属性标志
 * \param[out] s 返回创建的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_CreateSurface(AM_OSD_PixelFormatType_t type, int w, int h, uint32_t flags, AM_OSD_Surface_t **s);

/**\brief 从一个缓冲区创建一个绘图表面
 * \param type 图形模式 
 * \param w 绘图表面宽度
 * \param h 绘图表面高度
 * \param pitch 两行之间的字节数
 * \param buffer 缓冲区指针
 * \param flags 绘图表面的属性标志
 * \param[out] s 返回创建的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_CreateSurfaceFromBuffer(AM_OSD_PixelFormatType_t type, int w, int h, int pitch,
		uint8_t *buffer, uint32_t flags, AM_OSD_Surface_t **s);

/**\brief 销毁一个绘图表面
 * \param[in,out] s 要销毁的绘图表面 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DestroySurface(AM_OSD_Surface_t *s);

/**\brief 设定绘图剪切区，绘图只对剪切区内部的像素生效
 * \param[in] surf 绘图表面
 * \param[in] clip 剪切区矩形 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_SetClip(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *clip);

/**\brief 设定调色板，此操作只对调色板模式的表面有效
 * \param[in] surf 绘图表面
 * \param[in] pal 设定颜色数组
 * \param start 要设定的调色板起始项
 * \param count 要设定的颜色数目
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_SetPalette(AM_OSD_Surface_t *surf, AM_OSD_Color_t *pal, int start, int count);

/**\brief 在绘图表面上画点
 * \param[in] surf 绘图表面
 * \param x 点的X坐标
 * \param y 点的Y坐标
 * \param pix 点的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DrawPixel(AM_OSD_Surface_t *surf, int x, int y, uint32_t pix);

/**\brief 在绘图表面上画水平线
 * \param[in] surf 绘图表面
 * \param x 左顶点的X坐标
 * \param y 左顶点的Y坐标
 * \param w 水平线宽度
 * \param pix 直线的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DrawHLine(AM_OSD_Surface_t *surf, int x, int y, int w, uint32_t pix);

/**\brief 在绘图表面上画垂直线
 * \param[in] surf 绘图表面
 * \param x 上顶点的X坐标
 * \param y 上顶点的Y坐标
 * \param h 垂直线高度
 * \param pix 直线的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DrawVLine(AM_OSD_Surface_t *surf, int x, int y, int h, uint32_t pix);

/**\brief 在绘图表面上画矩形
 * \param[in] surf 绘图表面
 * \param[in] rect 矩形区 
 * \param pix 矩形边的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DrawRect(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *rect, uint32_t pix);

/**\brief 在绘图表面上画填充矩形
 * \param[in] surf 绘图表面
 * \param[in] rect 矩形区 
 * \param pix 填充像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_DrawFilledRect(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *rect, uint32_t pix);

/**\brief Blit操作
 * \param[in] src 源绘图表面
 * \param[in] sr 源矩形区 
 * \param[in] dst 目标绘图表面
 * \param[in] dr 目标矩形区
 * \param[in] para Blit参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
extern AM_ErrorCode_t AM_OSD_Blit(AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para);

#ifdef __cplusplus
}
#endif

#endif

