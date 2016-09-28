#ifndef ANDROID_WIFIPLAYER_H
#define ANDROID_WIFIPLAYER_H

extern "C" {
#include "libavutil/avstring.h"
#include <Amavutils.h>
#include "libavformat/avformat.h"
}
#include <wifiplayer/WifiPlayerStreamSource.h>
#include <player.h>
#include <player_ctrl.h>
#include <utils/RefBase.h>

namespace android
{

// TODO: Determine appropriate return codes
static int ERROR_NOT_OPEN = -1;
static int ERROR_OPEN_FAILED = -2;
static int ERROR_ALLOCATE_FAILED = -4;
static int ERROR_NOT_SUPPORTED = -8;
static int ERROR_NOT_READY = -16;
static int STATE_INIT = 0;
static int STATE_ERROR = 1;
static int STATE_OPEN = 2;
class WifiPlayer:public RefBase
{

public:
    static  status_t    BasicInit();
    WifiPlayer();
    ~WifiPlayer();
    virtual void        onFirstRef();
    virtual status_t    setDataSource(const char *uri);
    virtual status_t    setDataSource(const sp<IStreamSource> &source) ;

    virtual status_t    prepareAsync();
    virtual status_t    start();
    virtual status_t    stop();

    static int          notifyhandle(int pid, int msg, unsigned long ext1, unsigned long ext2);
    int                 NotifyHandle(int pid, int msg, unsigned long ext1, unsigned long ext2);
    int                 UpdateProcess(int pid, player_info_t *info);

private:

    status_t            mState;

    bool                mPaused;
    volatile bool       mRunning;
    bool                mExit;
    play_control_t      mPlay_ctl;
    int                 mPlayer_id;

    //added extend info

    bool                mEnded;
    int                 mSessionID;

    sp<WifiPlayerStreamSource> mStreamSource;
    sp<IStreamSource> mSource;
    int get_axis(const char *para, int para_num, int *result);
    int set_display_axis(int recovery);
};
}; // namespace android

#endif // ANDROID_AMLOGICPLAYER_H

