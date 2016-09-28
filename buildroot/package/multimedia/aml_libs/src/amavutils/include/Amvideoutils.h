#ifndef AMVIDEO_UTILS_H
#define AMVIDEO_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif

#define HDMI_HDCP_PASS           (1)
#define HDMI_HDCP_FAILED      (0)
#define HDMI_NOCONNECT        (-1)

int     amvideo_utils_get_freescale_enable(void);
int     amvideo_utils_get_global_offset();
int     amvideo_utils_set_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation);
int     amvideo_utils_set_virtual_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation);
int     amvideo_utils_set_absolute_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation);
int     amvideo_utils_get_position(int32_t *x, int32_t *y, int32_t *w, int32_t *h);
int     amvideo_utils_get_screen_mode(int *mode);
int     amvideo_utils_set_screen_mode(int mode);
int     amvideo_utils_get_video_angle(int *angle);
int     amvideo_utils_get_hdmi_authenticate(void);


#ifdef  __cplusplus
}
#endif

#endif

