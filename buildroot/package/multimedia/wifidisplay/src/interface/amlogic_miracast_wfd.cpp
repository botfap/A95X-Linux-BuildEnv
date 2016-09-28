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

#define LOG_NDEBUG 0
#define LOG_TAG "amlMiracast-API"

#include <utils/Log.h>

#include <wifidisplay/WifiDisplaySink.h>

#include <foundation/ADebug.h>

using namespace android;

sp<ALooper> mSinkLooper = new ALooper;

sp<ANetworkSession> mSession = new ANetworkSession;
sp<WifiDisplaySink> mSink;
bool mStart = false;
bool mInit = false;

int connect_to_wifi_source(const char *sourceHost, int32_t sourcePort) {
    mSession->start();

    if(!mInit){
        mInit = true;
        mSink = new WifiDisplaySink(mSession);
        mSinkLooper->registerHandler(mSink);
     }

    if (sourcePort >= 0) {
        mSink->start(sourceHost, sourcePort);
    } else {
        mSink->start(sourceHost);
    }

    mStart = true;
    mSinkLooper->start(false /* runOnCallingThread */);

    ALOGI("connected\n");
    return 0;
}

void disconnectSink(void) {
    ALOGI("disconnect sink mStart:%d\n", mStart);

    if(mStart){
        mSink->stop();
        mSession->stop();
        mSinkLooper->unregisterHandler(mSink->id());
        //mSinkLooper->stop();
        mStart = false;
    }
}

