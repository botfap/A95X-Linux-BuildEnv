/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 1
#define LOG_TAG "AmlogicPlayer"
#include "utils/Log.h"
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils/String8.h>

#include "ammodule.h"
#include "amconfigutils.h"
#include <wifiplayer/WifiPlayer.h>
namespace android
{
WifiPlayer::WifiPlayer() :
    mState(STATE_ERROR),
    mExit(false), mPaused(false), mRunning(false),
    mPlayer_id(-1)
{
    ALOGV("AmlogicPlayer constructor\n");
    memset(&mPlay_ctl, 0, sizeof mPlay_ctl);
    BasicInit();
}

WifiPlayer::~WifiPlayer()
{
    ALOGV("AmlogicPlayer donstructor\n");
}

void WifiPlayer::onFirstRef()
{
    ALOGV("onFirstRef");
    WifiPlayer::BasicInit();
    // create playback thread
    mState = STATE_INIT;
}

int WifiPlayer::BasicInit()
{
    static int have_inited = 0;
    if (!have_inited) {
        player_init();
        WifiPlayerStreamSource::init();
        have_inited++;
    }
    return 0;
}

status_t WifiPlayer:: setDataSource(const sp<IStreamSource> &source)
{
    mStreamSource = new WifiPlayerStreamSource(source);
    return setDataSource(mStreamSource->GetPathString());
}

status_t WifiPlayer::setDataSource(const char *path)
{
    int num;
    char * file = NULL;

    int time;
    file = (char *)malloc(strlen(path) + 10);
    if (file == NULL) {
        return NO_MEMORY;
    }
    num = sprintf(file, "%s", path);
    file[num] = '\0';
    mPlay_ctl.t_pos = -1;
    if (mPlay_ctl.headers) {
        free(mPlay_ctl.headers);
        mPlay_ctl.headers = NULL;
    }
    mPlay_ctl.need_start = 1;
    mPlay_ctl.file_name = file;
    ALOGV("setDataSource url=%s, len=%d\n", mPlay_ctl.file_name, strlen(mPlay_ctl.file_name));
    mState = STATE_OPEN;
    return NO_ERROR;

}

status_t WifiPlayer::prepareAsync()
{
    ALOGV("prepareAsync\n");
    am_setconfig("media.libplayer.wfd", "1");
    am_setconfig("media.libplayer.fastswitch", "2");

    mPlay_ctl.callback_fn.notify_fn = notifyhandle;
    mPlay_ctl.callback_fn.update_interval = 1000;
    mPlay_ctl.audio_index = -1;
    mPlay_ctl.video_index = -1;
    mPlay_ctl.hassub = 1;  //enable subtitle
    mPlay_ctl.is_type_parser = 1;
    mPlay_ctl.auto_buffing_enable = 0;
    mPlay_ctl.enable_rw_on_pause = 0; /**/
    mPlay_ctl.lowbuffermode_flag = 1;
//	mPlay_ctl.noaudio = 1; 
    mPlay_ctl.read_max_cnt = 10000; /*retry num*/

    mPlay_ctl.SessionID = mSessionID;
    
    mPlay_ctl.auto_buffing_enable = 0;
    ALOGV("prepareAsync,file_name=%s\n", mPlay_ctl.file_name);
    mPlayer_id = player_start(&mPlay_ctl, (unsigned long)this);
    if (mPlayer_id >= 0) {
        ALOGV("Start player,pid=%d\n", mPlayer_id);
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}
int WifiPlayer::get_axis(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;

    if (!startp) {
        return 0;
    }

    len = strlen(startp);

    do {
        //filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len) {
            startp++;
            len--;
        }

        if (len == 0) {
            break;
        }

        *out++ = strtol(startp, &endp, 0);

        len -= endp - startp;
        startp = endp;
        count++;

    } while ((endp) && (count < para_num) && (len > 0));

    return count;
}
int WifiPlayer::set_display_axis(int recovery)
{
    int fd;
    char *path = "/sys/class/display/axis";
    char str[128];
    int axis[8] = {0};
    int count, i;
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        if (!recovery) {
            read(fd, str, 128);
            printf("read axis %s, length %d\n", str, strlen(str));
            count = get_axis(str, 8, axis);
        }
        if (recovery) {
            sprintf(str, "%d %d %d %d %d %d %d %d", 
                axis[0],axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        } else {
            sprintf(str, "2048 %d %d %d %d %d %d %d", 
                axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }

    return -1;
}

status_t WifiPlayer::start()
{
    ALOGV("start\n");
    if (mState != STATE_OPEN) {
        return ERROR_NOT_OPEN;
    }

    if (mRunning && !mPaused) {
        return NO_ERROR;
    }
    set_display_axis(0);
#if 1
    player_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.set_mode = CMD_SET_FREERUN_MODE;
    cmd.param = 28;
    player_send_message(mPlayer_id, &cmd);
#endif
    player_start_play(mPlayer_id);

    mRunning = true;
    mEnded = false;
    return NO_ERROR;
}

status_t WifiPlayer::stop()
{
    ALOGV("stop\n");
    if (mState != STATE_OPEN) {
        return ERROR_NOT_OPEN;
    }
    mPaused = true;
    mRunning = false;
    player_stop(mPlayer_id);
    return NO_ERROR;
}

int WifiPlayer::notifyhandle(int pid, int msg, unsigned long ext1, unsigned long ext2)
{
    WifiPlayer *player = (WifiPlayer *)player_get_extern_priv(pid);
    if (player != NULL) {
        return player->NotifyHandle(pid, msg, ext1, ext2);
    } else {
        return -1;
    }
}
int WifiPlayer::NotifyHandle(int pid, int msg, unsigned long ext1, unsigned long ext2)
{
    player_file_type_t *type;
    int ret;
    switch (msg) {
    case PLAYER_EVENTS_PLAYER_INFO:
        return UpdateProcess(pid, (player_info_t *)ext1);
        break;
    case PLAYER_EVENTS_STATE_CHANGED:
    case PLAYER_EVENTS_ERROR:
    case PLAYER_EVENTS_BUFFERING:
        break;
    case PLAYER_EVENTS_FILE_TYPE: {
        break;
    }
    case PLAYER_EVENTS_HTTP_WV: {
        ALOGV("Get http wvm, goto WVM Extractor");
    }
    break;
    default:
        break;
    }
    return 0;
}



int WifiPlayer::UpdateProcess(int pid, player_info_t *info)
{

    ALOGV("update_process pid=%d, current=%d,status=[%s]\n", pid, info->current_time, player_status2str(info->status));
    if (info->status != PLAYER_ERROR) {
        return 0;
    }
#if 0
    LatestPlayerState = info->status;
    mHWvideobuflevel = info->video_bufferlevel;
    mHWaudiobuflevel = info->audio_bufferlevel;
    if (info->status != PLAYER_ERROR && info->error_no != 0) {
        if (info->error_no == PLAYER_NO_VIDEO) {
            sendEvent(MEDIA_INFO, MEDIA_INFO_AMLOGIC_NO_VIDEO);
            LOGW("player no video\n");
        } else if (info->error_no == PLAYER_NO_AUDIO) {
            sendEvent(MEDIA_INFO, MEDIA_INFO_AMLOGIC_NO_AUDIO);
            LOGW("player no audio\n");
        } else if (info->error_no == PLAYER_UNSUPPORT_VCODEC || info->error_no == PLAYER_UNSUPPORT_VIDEO) {
            LOGW("player video not supported\n");
            sendEvent(MEDIA_INFO, MEDIA_INFO_AMLOGIC_VIDEO_NOT_SUPPORT);
        } else if (info->error_no == PLAYER_UNSUPPORT_ACODEC || info->error_no == PLAYER_UNSUPPORT_AUDIO) {
            LOGW("player audio not supported\n");
            sendEvent(MEDIA_INFO, MEDIA_INFO_AMLOGIC_AUDIO_NOT_SUPPORT);
	     if(!mhasVideo)
	         sendEvent(MEDIA_PREPARED);				
        }
    } else if (info->status == PLAYER_BUFFERING) {
        if (mDuration > 0) {
            sendEvent(MEDIA_BUFFERING_UPDATE, mPlayTime * 100 / mDuration);
        }
        if (!mInbuffering) {
            mInbuffering = true;
            if (!mLowLevelBufMode) {
                sendEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_START);
            }
        }
    } else if (info->status == PLAYER_INITOK) {
        updateMediaInfo();
        if (info->full_time_ms != -1) {
            mDuration = info->full_time_ms;
        } else if (info->full_time != -1) {
            mDuration = info->full_time * 1000;
        }
        if (video_rotation_degree == 1 || video_rotation_degree == 3) {
            sendEvent(MEDIA_SET_VIDEO_SIZE, mHeight, mWidth);    // 90du,or 270du
        } else {
            sendEvent(MEDIA_SET_VIDEO_SIZE, mWidth, mHeight);
        }
        if (!fastNotifyMode) { ///fast mode,will send before,do't send again
            sendEvent(MEDIA_PREPARED);
        }
    } else if (info->status == PLAYER_STOPED || info->status == PLAYER_PLAYEND) {
        LOGV("Player status:%s, playback complete", player_status2str(info->status));
        if (mHttpWV == false) {
            if (!mEnded) {
                sendEvent(MEDIA_PLAYBACK_COMPLETE);
            }
        }
        mEnded = true;
    } else if (info->status == PLAYER_EXIT) {
        LOGV("Player status:%s, playback exit", player_status2str(info->status));
        mRunning = false;
        if (mHttpWV == false) {
            if (!mLoop && (mState != STATE_ERROR) && (!mEnded)) { //no errors & no loop^M
                sendEvent(MEDIA_PLAYBACK_COMPLETE);
            }
        }
        mEnded = true;
        if (isHDCPFailed == true) {
            set_sys_int(DISABLE_VIDEO, 2);
            isHDCPFailed = false;
            LOGV("[L%d]:Enable Video", __LINE__);
        }
    } else if (info->status == PLAYER_ERROR) {
        if (mHttpWV == false) {
            sendEvent(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, info->error_no);
            //sendEvent(MEDIA_ERROR,MEDIA_ERROR_UNKNOWN,info->error_no);
            LOGV("Player status:%s, error occur", player_status2str(info->status));
            //sendEvent(MEDIA_ERROR);
            //sendEvent(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, -1);
            mState = STATE_ERROR;
            if (isHDCPFailed == true) {
                set_sys_int(DISABLE_VIDEO, 2);
                isHDCPFailed = false;
                LOGV("[L%d]:Enable Video", __LINE__);
            }
        }
    } else {
        if (info->status == PLAYER_SEARCHING) {
            if (mDuration > 0) {
                sendEvent(MEDIA_BUFFERING_UPDATE, mPlayTime * 100 / mDuration);
            }
            if (!mInbuffering) {
                mInbuffering = true;
                sendEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_START);
            }
        }
        int percent = 0;
        if (mInbuffering && info->status != PLAYER_SEARCHING) {
            mInbuffering = false;
            sendEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
        }
        if (mDuration > 0) {
            percent = (mPlayTime) * 100 / (mDuration);
        } else {
            percent = 0;
        }

        if (info->status == PLAYER_SEARCHOK) {
            ///sendEvent(MEDIA_SEEK_COMPLETE);
        }
        if (info->full_time_ms != -1) {
            mDuration = info->full_time_ms;
        } else if (info->full_time != -1) {
            mDuration = info->full_time * 1000;
        }
        if (info->current_ms >= 100 && mDelayUpdateTime-- <= 0) {
            mPlayTime = info->current_ms;
            mLastPlayTimeUpdateUS = ALooper::GetNowUs();
        }
        if (info->current_pts != 0 && info->current_pts != 0xffffffff) {
            mStreamTime = info->current_pts / 90; /*pts(90000hz)->ms*/
            mLastStreamTimeUpdateUS = ALooper::GetNowUs();
        }


        LOGV("Playing percent =%d,mPlayTime:%d,mStreamTime:%d\n", percent, mPlayTime, mStreamTime);
        if (streaminfo_valied && mDuration > 0 && info->bufed_time > 0) {
            percent = (info->bufed_time * 100 / (mDuration / 1000));
            LOGV("Playing percent on percent=%d,bufed time=%dS,Duration=%dS\n", percent, info->bufed_time, mDuration / 1000);
        } else if (streaminfo_valied && mDuration > 0 && info->bufed_pos > 0 && mStreamInfo.stream_info.file_size > 0) {

            percent = (info->bufed_pos / (mStreamInfo.stream_info.file_size));
            LOGV("Playing percent on percent=%d,bufed pos=%lld,Duration=%lld\n", percent, info->bufed_pos, (mStreamInfo.stream_info.file_size));
        } else if (mDuration > 0 && streaminfo_valied && mStreamInfo.stream_info.file_size > 0) {
            percent += ((long long)4 * 1024 * 1024 * 100 * info->audio_bufferlevel / mStreamInfo.stream_info.file_size);
            percent += ((long long)6 * 1024 * 1024 * 100 * info->video_bufferlevel / mStreamInfo.stream_info.file_size);
            /*we think the lowlevel buffer size is alsways 10M */
            LOGV("Playing buffer percent =%d\n", percent);
        } else {
            //percent+=info->audio_bufferlevel*4;
            //percent+=info->video_bufferlevel*6;
        }
        if (percent > 100) {
            percent = 100;
        } else if (percent < 0) {
            percent = 0;
        }
        if (mDuration > 0 && !mLowLevelBufMode) {
            sendEvent(MEDIA_BUFFERING_UPDATE, percent);
        }

    }
    #endif
    return 0;
}
}  // namespace android

