/**
 * \file adec-pts-mgt.h
 * \brief  Function prototypes of Pts manage.
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_PTS_H
#define ADEC_PTS_H

#include <audio-dec.h>

ADEC_BEGIN_DECLS

#define TSYNC_PCRSCR    "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT     "/sys/class/tsync/event"
#define TSYNC_APTS      "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS      "/sys/class/tsync/pts_video"
#define TSYNC_ENABLE  "/sys/class/tsync/enable"
#define TSYNC_LAST_CHECKIN_APTS "/sys/class/tsync/last_checkin_apts"

#define SYSTIME_CORRECTION_THRESHOLD        (90000*15/100)
#define APTS_DISCONTINUE_THRESHOLD          (90000*3)
#define REFRESH_PTS_TIME_MS                 (1000/10)

#define abs(x) ({                               \
                long __x = (x);                 \
                (__x < 0) ? -__x : __x;         \
                })


/**********************************************************************/

unsigned long adec_calc_pts(aml_audio_dec_t *audec);
int adec_pts_start(aml_audio_dec_t *audec);
int adec_pts_pause(void);
int adec_pts_resume(void);
int adec_refresh_pts(aml_audio_dec_t *audec);
int avsync_en(int e);
int track_switch_pts(aml_audio_dec_t *audec);

ADEC_END_DECLS

#endif
