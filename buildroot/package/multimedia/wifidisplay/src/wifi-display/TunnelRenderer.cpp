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
#define LOG_TAG "TunnelRenderer"
#include <utils/Log.h>

#include <wifidisplay/TunnelRenderer.h>

#include <foundation/ABuffer.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <wifiplayer/IStreamSource.h>
namespace android {

struct TunnelRenderer::StreamSource : public IStreamSource {
    StreamSource(TunnelRenderer *owner);

    virtual void setListener(const sp<IStreamListener> &listener);
    virtual void setBuffers(const Vector<sp<ABuffer> > &buffers);

    virtual void onBufferAvailable(size_t index);

    virtual uint32_t flags() const;

    void doSomeWork();
    void clearListerner(void);
protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
    virtual ~StreamSource();

private:
    mutable Mutex mLock;

    TunnelRenderer *mOwner;

    sp<IStreamListener> mListener;

    Vector<sp<ABuffer> > mBuffers;
    List<size_t> mIndicesAvailable;

    size_t mNumDeqeued;

    DISALLOW_EVIL_CONSTRUCTORS(StreamSource);
};

////////////////////////////////////////////////////////////////////////////////

TunnelRenderer::StreamSource::StreamSource(TunnelRenderer *owner)
    : mOwner(owner),
      mNumDeqeued(0) {
}

TunnelRenderer::StreamSource::~StreamSource() {
}

void TunnelRenderer::StreamSource::setListener(
        const sp<IStreamListener> &listener) {
    mListener = listener;
}

void TunnelRenderer::StreamSource::setBuffers(
        const Vector<sp<ABuffer> > &buffers) {
    mBuffers = buffers;
}

void TunnelRenderer::StreamSource::onBufferAvailable(size_t index) {
    if(index>=0x80000000)
    {
	doSomeWork();
	return ;/*require buffer command,for amplayer only. ignore.now*/
    }	
    CHECK_LT(index, mBuffers.size());
    {
        Mutex::Autolock autoLock(mLock);
        mIndicesAvailable.push_back(index);
    }

    doSomeWork();
}

uint32_t TunnelRenderer::StreamSource::flags() const {
    return kFlagAlignedVideoData;
}

void TunnelRenderer::StreamSource::doSomeWork() {
    Mutex::Autolock autoLock(mLock);

    while (!mIndicesAvailable.empty()) {
        sp<ABuffer> srcBuffer = mOwner->dequeueBuffer();
        if (srcBuffer == NULL) {
            break;
        }

        ++mNumDeqeued;

        if (mNumDeqeued == 1) {
            ALOGI("fixing real time now.");

            sp<AMessage> extra = new AMessage;
#if 0
            extra->setInt32(
                    IStreamListener::kKeyDiscontinuityMask,
                    ATSParser::DISCONTINUITY_ABSOLUTE_TIME);
#endif
            extra->setInt64("timeUs", ALooper::GetNowUs());

            mListener->issueCommand(
                    IStreamListener::DISCONTINUITY,
                    false /* synchronous */,
                    extra);
        }

        ALOGI("dequeue TS packet of size %d", srcBuffer->size());

        size_t index = *mIndicesAvailable.begin();
        mIndicesAvailable.erase(mIndicesAvailable.begin());

        sp<ABuffer> mem = mBuffers.itemAt(index);
        CHECK_LE(srcBuffer->size(), mem->size());
        CHECK_EQ((srcBuffer->size() % 188), 0u);

        memcpy(mem->data(), srcBuffer->data(), srcBuffer->size());
        mListener->queueBuffer(index, srcBuffer->size());
    }
}

void TunnelRenderer::StreamSource::clearListerner(void) {
    mListener.clear();
}

void TunnelRenderer::StreamSource::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatBufferAvailable:
        {
                int value = 0;
                CHECK(msg->findInt32("bufferidx", &value));
                onBufferAvailable(value);
            break;
        }
        default:
            TRESPASS();
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

TunnelRenderer::TunnelRenderer(
        const sp<AMessage> &notifyLost)
    : mNotifyLost(notifyLost),
      mTotalBytesQueued(0ll),
      mLastDequeuedExtSeqNo(-1),
      mFirstFailedAttemptUs(-1ll),
      mPackageSuccess(0),
      mPackageFailed(0),
      mPackageRequest(0),
      mRequestedRetry(false),
      mRequestedRetransmission(false) {
}

TunnelRenderer::~TunnelRenderer() {
    destroyPlayer();
}

void TunnelRenderer::queueBuffer(const sp<ABuffer> &buffer) {
    Mutex::Autolock autoLock(mLock);

    mTotalBytesQueued += buffer->size();

    if (mPackets.empty()) {
        mPackets.push_back(buffer);
        return;
    }

    int32_t newExtendedSeqNo = buffer->int32Data();

    List<sp<ABuffer> >::iterator firstIt = mPackets.begin();
    List<sp<ABuffer> >::iterator it = --mPackets.end();
    for (;;) {
        int32_t extendedSeqNo = (*it)->int32Data();

        if (extendedSeqNo == newExtendedSeqNo) {
            // Duplicate packet.
            return;
        }

        if (extendedSeqNo < newExtendedSeqNo) {
            // Insert new packet after the one at "it".
            mPackets.insert(++it, buffer);
            return;
        }

        if (it == firstIt) {
            // Insert new packet before the first existing one.
            mPackets.insert(it, buffer);
            return;
        }

        --it;
    }
}

sp<ABuffer> TunnelRenderer::dequeueBuffer() {
    Mutex::Autolock autoLock(mLock);

    sp<ABuffer> buffer;
    int32_t extSeqNo;

    while (!mPackets.empty()) {
        buffer = *mPackets.begin();
        extSeqNo = buffer->int32Data();

        if (mLastDequeuedExtSeqNo < 0 || extSeqNo > mLastDequeuedExtSeqNo) {
            break;
        }

        // This is a retransmission of a packet we've already returned.

        mTotalBytesQueued -= buffer->size();
        buffer.clear();
        extSeqNo = -1;

        mPackets.erase(mPackets.begin());
    }

    if (mPackets.empty()) {
        if (mFirstFailedAttemptUs < 0ll) {
            mFirstFailedAttemptUs = ALooper::GetNowUs();
            mRequestedRetry = false;
            mRequestedRetransmission = false;
        } else {
            ALOGI("no packets available for %.2f secs",
                    (ALooper::GetNowUs() - mFirstFailedAttemptUs) / 1E6);
        }

        return NULL;
    }

    if (mLastDequeuedExtSeqNo < 0 || extSeqNo == mLastDequeuedExtSeqNo + 1) {
        if (mRequestedRetransmission) {
            ALOGI("Recovered after requesting retransmission of %d",
                  extSeqNo);
        }

        if (!mRequestedRetry){
            mPackageSuccess++;
        }else{
            mRequestedRetry = false;
            mPackageRequest++;
        }

        mLastDequeuedExtSeqNo = extSeqNo;
        mFirstFailedAttemptUs = -1ll;
        mRequestedRetransmission = false;

        mPackets.erase(mPackets.begin());

        mTotalBytesQueued -= buffer->size();

        return buffer;
    }

    if (mFirstFailedAttemptUs < 0ll) {
        mFirstFailedAttemptUs = ALooper::GetNowUs();

        ALOGI("failed to get the correct packet the first time.");
        return NULL;
    }

    if (mFirstFailedAttemptUs + 50000ll > ALooper::GetNowUs()) {
        // We're willing to wait a little while to get the right packet.

        if (!mRequestedRetransmission) {
            ALOGI("requesting retransmission of seqNo %d",
                  (mLastDequeuedExtSeqNo + 1) & 0xffff);

            sp<AMessage> notify = mNotifyLost->dup();
            notify->setInt32("seqNo", (mLastDequeuedExtSeqNo + 1) & 0xffff);
            notify->post();

            mRequestedRetry = true;
            mRequestedRetransmission = true;
        } else {
            ALOGI("still waiting for the correct packet to arrive.");
        }

        return NULL;
    }

    ALOGI("dropping packet. extSeqNo %d didn't arrive in time",
            mLastDequeuedExtSeqNo + 1);

    // Permanent failure, we never received the packet.
    mPackageFailed++;

    mLastDequeuedExtSeqNo = extSeqNo;
    mFirstFailedAttemptUs = -1ll;
    mRequestedRetransmission = false;
    mRequestedRetry = false;

    mTotalBytesQueued -= buffer->size();

    mPackets.erase(mPackets.begin());

    return buffer;
}

void TunnelRenderer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatQueueBuffer:
        {
            sp<ABuffer> buffer;
            CHECK(msg->findBuffer("buffer", &buffer));
            queueBuffer(buffer);

            if (mStreamSource == NULL) {
                if (mTotalBytesQueued > 0ll) {
                    initPlayer();
                } else {
                    ALOGI("Have %lld bytes queued...", mTotalBytesQueued);
                }
            } else {
                mStreamSource->doSomeWork();
            }
            break;
        }
        default:
            TRESPASS();
    }
}

void TunnelRenderer::initPlayer() {
    mStreamSource = new StreamSource(this);
    looper()->registerHandler(mStreamSource);

    mPlayer = new WifiPlayer();
    CHECK(mPlayer != NULL);
    CHECK_EQ(mPlayer->setDataSource(mStreamSource),  (status_t)OK);

    mPlayer->prepareAsync();
    mPlayer->start();
}

void TunnelRenderer::destroyPlayer() {
    if(mStreamSource != NULL){
        looper()->unregisterHandler(mStreamSource->id());
    }
    mStreamSource->clearListerner();
    mStreamSource.clear();

    mPlayer->stop();
    mPlayer.clear();
}

}  // namespace android

