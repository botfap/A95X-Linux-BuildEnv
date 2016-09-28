/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WIFIPLAYERSTREAMLISTENER_H_
#define WIFIPLAYERSTREAMLISTENER_H_

#include <utils/RefBase.h>
#include <wifiplayer/IStreamSource.h>
#include <utils/List.h>
#include <utils/Mutex.h>

namespace android
{

struct  WifiPlayerStreamSourceListener : public IStreamListener {
    WifiPlayerStreamSourceListener(
        const sp<IStreamSource> &source);

    virtual void queueBuffer(size_t index, size_t size);

    virtual void issueCommand(
        Command cmd, bool synchronous, const sp<AMessage> &extra);

    void start();
    ssize_t read(void *data, size_t size, sp<AMessage> *extra);

protected:
    virtual ~WifiPlayerStreamSourceListener();
private:
    void sendBufferAvailable(size_t index);

    enum {
        kNumBuffers = 8,
        kBufferSize = 188 * 10
    };

    struct QueueEntry {
        bool mIsCommand;

        size_t mIndex;
        size_t mSize;
        size_t mOffset;

        Command mCommand;
        sp<AMessage> mExtra;
    };

    Mutex mLock;
    int valid_data_buf_num;
    sp<IStreamSource> mSource;
    Vector<sp<ABuffer> > mBuffers;
    List<QueueEntry> mQueue;
    bool mEOS;

};

}  // namespace android

#endif // NUPLAYER_STREAM_LISTENER_H_

