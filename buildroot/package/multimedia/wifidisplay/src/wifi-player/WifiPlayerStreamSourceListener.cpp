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

#define LOG_NDEBUG 1
#define LOG_TAG "WifiPlayerStreamSourceListener"
#include <utils/Log.h>

#include <stagefright/MediaErrors.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <foundation/ABuffer.h>

#include <wifiplayer/WifiPlayerStreamSourceListener.h>

#define kWhatMoreDataQueued             = 'more'
namespace android
{

WifiPlayerStreamSourceListener::WifiPlayerStreamSourceListener(
    const sp<IStreamSource> &source)
    : mSource(source),
      mEOS(false)
{
    mSource->setListener(this);
    for (size_t i = 0; i < kNumBuffers; ++i) {
        sp<ABuffer> buffer = new ABuffer(kBufferSize);
        CHECK(buffer != NULL);

        mBuffers.push(buffer);
    }
    valid_data_buf_num = 0;
    mSource->setBuffers(mBuffers);
}

WifiPlayerStreamSourceListener::~WifiPlayerStreamSourceListener() {
    mSource.clear();
}

void WifiPlayerStreamSourceListener::start()
{
    for (size_t i = 0; i < kNumBuffers; ++i) {
        sendBufferAvailable(i);
    }
}

void WifiPlayerStreamSourceListener::queueBuffer(size_t index, size_t size)
{
    QueueEntry entry;
    entry.mIsCommand = false;
    entry.mIndex = index;
    entry.mSize = size;
    entry.mOffset = 0;

    Mutex::Autolock autoLock(mLock);

    mQueue.push_back(entry);
    valid_data_buf_num++;
    ALOGV("queueBuffer=%d,size=%d,valid_data_buf_num=%d", index, size, valid_data_buf_num);
}

void WifiPlayerStreamSourceListener::sendBufferAvailable(size_t index)
{
    sp<AMessage> bufferAvailableMsg =
            new AMessage(IStreamSource::kWhatBufferAvailable, mSource->id());
    bufferAvailableMsg->setInt32("bufferidx", index);
    bufferAvailableMsg->post();
}

void WifiPlayerStreamSourceListener::issueCommand(
    Command cmd, bool synchronous, const sp<AMessage> &extra)
{
    CHECK(!synchronous);

    QueueEntry entry;
    entry.mIsCommand = true;
    entry.mCommand = cmd;
    entry.mExtra = extra;

    Mutex::Autolock autoLock(mLock);
    mQueue.push_back(entry);
}

ssize_t WifiPlayerStreamSourceListener::read(
    void *data, size_t size, sp<AMessage> *extra)
{
    CHECK_GT(size, 0u);

    extra->clear();
    Mutex::Autolock autoLock(mLock);
    
    if (mEOS) {
        return 0;
    }

    if (mQueue.empty()) {
        sendBufferAvailable(0x80000001);
        return -EWOULDBLOCK;
    }

    QueueEntry *entry = &*mQueue.begin();

    if (entry->mIsCommand) {
        switch (entry->mCommand) {
        case EOS: {
            mQueue.erase(mQueue.begin());
            entry = NULL;

            mEOS = true;
            return 0;
        }

        case DISCONTINUITY: {
            *extra = entry->mExtra;

            mQueue.erase(mQueue.begin());
            entry = NULL;

            return INFO_DISCONTINUITY;
        }

        default:
            TRESPASS();
            break;
        }
    }

    size_t copy = entry->mSize;
    if (copy > size) {
        copy = size;
    }

    memcpy(data,
           (const uint8_t *)mBuffers.editItemAt(entry->mIndex)->data()
           + entry->mOffset,
           copy);

    entry->mOffset += copy;
    entry->mSize -= copy;

    if (entry->mSize == 0) {
        valid_data_buf_num--;
        sendBufferAvailable(entry->mIndex);
        ALOGV("free Index=%d,valid_data_buf_num=%d", entry->mIndex, valid_data_buf_num);
        mQueue.erase(mQueue.begin());
        entry = NULL;
    }
    return copy;
}

}  // namespace android

