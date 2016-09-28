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
#define LOG_TAG "WifiDisplaySink"
#include <utils/Log.h>

#include <wifidisplay/ParsedMessage.h>
#include <wifidisplay/Parameters.h>
#include <wifidisplay/RTPSink.h>

#include <wifidisplay/WifiDisplaySink.h>
#include <foundation/ABuffer.h>
#include <foundation/ADebug.h>
#include <foundation/AMessage.h>
#include <stagefright/MediaErrors.h>

#include <fcntl.h>

#define FILE_PATCH "/etc/wfdrc"

namespace android {

WifiDisplaySink::WifiDisplaySink(
        const sp<ANetworkSession> &netSession)
    : mState(UNDEFINED),
      mDumpEnable(false),
      mNetSession(netSession),
      mSessionID(0),
      mNextCSeq(1) {

    int fd = -1;
    size_t amt = 0;
    size_t len = 100;
    
    fd = open(FILE_PATCH, O_RDONLY);
    if (fd == -1) {
        mDumpEnable = false;
        ALOGI("open file /etc/wfdrc to get property error.");
    }else{
        char* contents = (char*)malloc(len);
        if (contents == NULL) {
	    mDumpEnable = false;
             ALOGI("malloc memory for set the content of file");
        }else{
            amt = lseek(fd, 0, SEEK_END);
            if (amt != len) {
                ALOGI("compare_file file length should not be %d, was %d\n", len, amt);
            }
            lseek(fd, 0, SEEK_SET);

            size_t readLen = amt < len ? amt : len;
            amt = read(fd, contents, readLen);
            if (amt == 0) {
                mDumpEnable = false;
                ALOGI("compare_file read expected %d bytes but got %d\n", len, amt);
            }else{
                sp<Parameters> params =
                    Parameters::Parse(contents, readLen);

                AString value;

                if (!params->findParameter("sys.wfddump", &value)) {
                    ALOGE("The file of wfdrc don't report wfddump state.");
                }else{
                    if(strncmp(value.c_str(), "true", 4) == 0){
                        mDumpEnable = true;
                    }else{
                        mDumpEnable = false;
                    }
                    ALOGI("start wifidisplay sys.wfddump is %s\n", value.c_str());
                }
            }
        }
    }
}

WifiDisplaySink::~WifiDisplaySink() {
}

void WifiDisplaySink::start(const char *sourceHost, int32_t sourcePort) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("sourceHost", sourceHost);
    msg->setInt32("sourcePort", sourcePort);
    ALOGI("post msg kWhatStart.");
    msg->post();
}

void WifiDisplaySink::start(const char *uri) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("setupURI", uri);
    msg->post();
}

void WifiDisplaySink::stop(void) {
    sp<AMessage> msg = new AMessage(kWhatStop, id());
    msg->post();
}

// static
bool WifiDisplaySink::ParseURL(
        const char *url, AString *host, int32_t *port, AString *path,
        AString *user, AString *pass) {
    host->clear();
    *port = 0;
    path->clear();
    user->clear();
    pass->clear();

    if (strncasecmp("rtsp://", url, 7)) {
        return false;
    }

    const char *slashPos = strchr(&url[7], '/');

    if (slashPos == NULL) {
        host->setTo(&url[7]);
        path->setTo("/");
    } else {
        host->setTo(&url[7], slashPos - &url[7]);
        path->setTo(slashPos);
    }

    ssize_t atPos = host->find("@");

    if (atPos >= 0) {
        // Split of user:pass@ from hostname.

        AString userPass(*host, 0, atPos);
        host->erase(0, atPos + 1);

        ssize_t colonPos = userPass.find(":");

        if (colonPos < 0) {
            *user = userPass;
        } else {
            user->setTo(userPass, 0, colonPos);
            pass->setTo(userPass, colonPos + 1, userPass.size() - colonPos - 1);
        }
    }

    const char *colonPos = strchr(host->c_str(), ':');

    if (colonPos != NULL) {
        char *end;
        unsigned long x = strtoul(colonPos + 1, &end, 10);

        if (end == colonPos + 1 || *end != '\0' || x >= 65536) {
            return false;
        }

        *port = x;

        size_t colonOffset = colonPos - host->c_str();
        size_t trailing = host->size() - colonOffset;
        host->erase(colonOffset, trailing);
    } else {
        *port = 554;
    }

    return true;
}

void WifiDisplaySink::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatStart:
        {
            int32_t sourcePort;

            ALOGI("Received msg kWhatStart.");

            if (msg->findString("setupURI", &mSetupURI)) {
                AString path, user, pass;
                CHECK(ParseURL(
                            mSetupURI.c_str(),
                            &mRTSPHost, &sourcePort, &path, &user, &pass)
                        && user.empty() && pass.empty());
            } else {
                CHECK(msg->findString("sourceHost", &mRTSPHost));
                CHECK(msg->findInt32("sourcePort", &sourcePort));
            }

            sp<AMessage> notify = new AMessage(kWhatRTSPNotify, id());

            ALOGI("Create RTSPClient.");
            status_t err = mNetSession->createRTSPClient(
                    mRTSPHost.c_str(), sourcePort, notify, &mSessionID);
            CHECK_EQ(err, (status_t)OK);

            mState = CONNECTING;
            break;
        }

        case kWhatRTSPNotify:
        {
            int32_t reason;
            CHECK(msg->findInt32("reason", &reason));

            ALOGI("Received msg kWhatRTSPNotify.");

            switch (reason) {
                case ANetworkSession::kWhatError:
                {
                    int32_t sessionID;
                    CHECK(msg->findInt32("sessionID", &sessionID));
                    ALOGI("reason: kWhatError.");

                    int32_t err;
                    CHECK(msg->findInt32("err", &err));

                    AString detail;
                    CHECK(msg->findString("detail", &detail));

                    ALOGE("An error occurred in session %d (%d, '%s/%s').",
                          sessionID,
                          err,
                          detail.c_str(),
                          strerror(-err));

                    if (sessionID == mSessionID) {
                        ALOGI("Lost control connection.");

                        // The control connection is dead now.
                        mNetSession->destroySession(mSessionID);
                        mSessionID = 0;
			if(mRTPSink!=NULL)
            		{
                		looper()->unregisterHandler(mRTPSink->id());
                		mRTPSink.clear();
            		}
                        //looper()->stop();
                    }
                    break;
                }

                case ANetworkSession::kWhatConnected:
                {
                    ALOGI("reason: kWhatConnected.");
                    ALOGI("We're now connected.");
                    mState = CONNECTED;

                    if (!mSetupURI.empty()) {
                        ALOGI("sendDescribe.[uri]");

                        status_t err =
                            sendDescribe(mSessionID, mSetupURI.c_str());

                        CHECK_EQ(err, (status_t)OK);
		    }
			
                    break;
                }

                case ANetworkSession::kWhatData:
                {
                    ALOGI("reason: kWhatData.");
                    onReceiveClientData(msg);
                    break;
                }

                case ANetworkSession::kWhatBinaryData:
                {
                    CHECK(sUseTCPInterleaving);

                    ALOGI("reason: kWhatBinaryData.");

                    int32_t channel;
                    CHECK(msg->findInt32("channel", &channel));

                    sp<ABuffer> data;
                    CHECK(msg->findBuffer("data", &data));

                    mRTPSink->injectPacket(channel == 0 /* isRTP */, data);
                    break;
                }

                default:
                    TRESPASS();
            }
            break;
        }

        case kWhatStop:
        {
            ALOGI("Received msg stop.");
	    if(mSessionID!=0)
	    {
		mNetSession->destroySession(mSessionID);
		mSessionID=0;	
	    }
	    if(mRTPSink!=NULL)
	    {
	    	looper()->unregisterHandler(mRTPSink->id());
            	mRTPSink.clear();
	    }
	    //looper()->stop();
	    
            break;
        }

        default:
            TRESPASS();
    }
}

void WifiDisplaySink::registerResponseHandler(
        int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func) {
    ResponseID id;
    id.mSessionID = sessionID;
    id.mCSeq = cseq;

    mResponseHandlers.add(id, func);
}

status_t WifiDisplaySink::sendM2(int32_t sessionID) {
    AString request = "OPTIONS * RTSP/1.0\r\n";
    AppendCommonResponse(&request, mNextCSeq);

    request.append(
            "Require: org.wfa.wfd1.0\r\n"
            "\r\n");

    ALOGI("\nSend Request:\n%s", request.c_str());
    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    ALOGI("registerResponseHandler:id = %d, onReceiveM2Response", mNextCSeq);

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveM2Response);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::onReceiveM2Response(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;

    ALOGI("%s", __FUNCTION__);

    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return OK;
}

status_t WifiDisplaySink::onReceiveDescribeResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    ALOGI("%s", __FUNCTION__);

    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return sendSetup(sessionID, mSetupURI.c_str());
}

status_t WifiDisplaySink::onReceiveSetupResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;

    ALOGI("%s", __FUNCTION__);
    
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    if (!msg->findString("session", &mPlaybackSessionID)) {
        return ERROR_MALFORMED;
    }

    if (!ParsedMessage::GetInt32Attribute(
                mPlaybackSessionID.c_str(),
                "timeout",
                &mPlaybackSessionTimeoutSecs)) {
        mPlaybackSessionTimeoutSecs = -1;
    }

    ssize_t colonPos = mPlaybackSessionID.find(";");
    if (colonPos >= 0) {
        // Strip any options from the returned session id.
        mPlaybackSessionID.erase(
                colonPos, mPlaybackSessionID.size() - colonPos);
    }

    status_t err = configureTransport(msg);

    if (err != OK) {
        return err;
    }

    mState = PAUSED;

    AString url = StringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());

    ALOGI("%s: send PLAY Request", __FUNCTION__);

    return sendPlay(
            sessionID,
            !mSetupURI.empty()
                ? mSetupURI.c_str() : url.c_str());
}

status_t WifiDisplaySink::configureTransport(const sp<ParsedMessage> &msg) {
    if (sUseTCPInterleaving) {
        return OK;
    }

    AString transport;
    if (!msg->findString("transport", &transport)) {
        ALOGE("Missing 'transport' field in SETUP response.");
        return ERROR_MALFORMED;
    }

    AString sourceHost;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "source", &sourceHost)) {
        sourceHost = mRTSPHost;
    }

    AString serverPortStr;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "server_port", &serverPortStr)) {
    }

    int rtpPort = 0, rtcpPort=0;
    int ret;
    ret = sscanf(serverPortStr.c_str(), "%d-%d", &rtpPort, &rtcpPort);

    if (ret != 2
            || rtpPort <= 0 || rtpPort > 65535
            || rtcpPort <=0 || rtcpPort > 65535
            || rtcpPort != rtpPort + 1) {
        ALOGE("!!! FIXME: Invalid server_port description '%s'.",
                serverPortStr.c_str());

        ALOGE("ret=%d, rtpPort=%d, rtcpPort=%d",
                ret, rtpPort, rtcpPort);

        //return ERROR_MALFORMED;
    }

    if (rtpPort & 1) {
        ALOGW("Server picked an odd numbered RTP port.");
    }

    return mRTPSink->connect(sourceHost.c_str(), rtpPort, rtcpPort);
    return OK;
}

status_t WifiDisplaySink::onReceivePlayResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;

    ALOGI("%s", __FUNCTION__);
    
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    mState = PLAYING;

    return OK;
}

void WifiDisplaySink::onReceiveClientData(const sp<AMessage> &msg) {
    int32_t sessionID;
    CHECK(msg->findInt32("sessionID", &sessionID));

    sp<RefBase> obj;
    CHECK(msg->findObject("data", &obj));

    sp<ParsedMessage> data =
        static_cast<ParsedMessage *>(obj.get());

    AString method;
    AString uri;
    data->getRequestField(0, &method);

    int32_t cseq;
    if (!data->findInt32("cseq", &cseq)) {
        ALOGI("\nReceived invalid packet:\n%s",
              data->debugString().c_str());
        sendErrorResponse(sessionID, "400 Bad Request", -1 /* cseq */);
        return;
    }

    if (method.startsWith("RTSP/")) {
        ALOGI("\nReceived Response:\n%s",
              data->debugString().c_str());
        // This is a response.

        ResponseID id;
        id.mSessionID = sessionID;
        id.mCSeq = cseq;

        ssize_t index = mResponseHandlers.indexOfKey(id);

        if (index < 0) {
            ALOGW("Received unsolicited server response, cseq %d", cseq);
            return;
        }

        HandleRTSPResponseFunc func = mResponseHandlers.valueAt(index);
        mResponseHandlers.removeItemsAt(index);

        status_t err = (this->*func)(sessionID, data);
        CHECK_EQ(err, (status_t)OK);
    } else {
        AString version;

        ALOGI("\nReceived Request:\n%s",
              data->debugString().c_str());

        data->getRequestField(2, &version);
        if (!(version == AString("RTSP/1.0"))) {
            sendErrorResponse(sessionID, "505 RTSP Version not supported", cseq);
            return;
        }

        if (method == "OPTIONS") {
            onOptionsRequest(sessionID, cseq, data);
        } else if (method == "GET_PARAMETER") {
            onGetParameterRequest(sessionID, cseq, data);
        } else if (method == "SET_PARAMETER") {
            onSetParameterRequest(sessionID, cseq, data);
        } else {
            sendErrorResponse(sessionID, "405 Method Not Allowed", cseq);
        }
    }
}

void WifiDisplaySink::onOptionsRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER\r\n");
    response.append("\r\n");

    ALOGI("\nSend Response:\n%s", response.c_str());
    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);

    err = sendM2(sessionID);
    CHECK_EQ(err, (status_t)OK);
}

void WifiDisplaySink::onGetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    status_t err;

    if (mRTPSink == NULL) {
        mRTPSink = new RTPSink(mNetSession, mDumpEnable);
        ALOGI("*******register mRTPSink\n");
        looper()->registerHandler(mRTPSink);

        err = mRTPSink->init(sUseTCPInterleaving);

        if (err != OK) {
            ALOGE("------onGetParameterRequest RTPSink ERROR");
            looper()->unregisterHandler(mRTPSink->id());
            mRTPSink.clear();
            return;
        }
    }

    ALOGI("!!! FIXME: HARD CODE with video_formats, audio_codecs and client_rtp_ports");

    char body[1024];

    snprintf(body, sizeof(body), 
        "wfd_video_formats: 40 00 02 02 0001DEFF 157C7FFF 00000FFF 00 0000 0000 11 none none, 01 02 0001DEFF 157C7FFF 00000FFF 00 0000 0000 11 none none\r\n"
        "wfd_audio_codecs: AAC 0000000F 00\r\n" //LPCM 00000003 00,
        "wfd_client_rtp_ports: RTP/AVP/UDP;unicast %d 0 mode=play\r\n"
        "wfd_uibc_capability: input_category_list=GENERIC;generic_cap_list=Keyboard, Mouse, SingleTouch;hidc_cap_list=none;port=none\r\n"
        "wfd_standby_resume_capability: supported\r\n",
        mRTPSink->getRTPPort());

    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Content-Type: text/parameters\r\n");
    response.append(StringPrintf("Content-Length: %d\r\n", strlen(body)));
    response.append("\r\n");
    response.append(body);

    ALOGI("\nSend Response:\n%s", response.c_str());
    err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

status_t WifiDisplaySink::sendDescribe(int32_t sessionID, const char *uri) {
    AString request = StringPrintf("DESCRIBE %s RTSP/1.0\r\n", uri);
    AppendCommonResponse(&request, mNextCSeq);

    request.append("Accept: application/sdp\r\n");
    request.append("\r\n");

    ALOGI("\nSend Request:\n%s", request.c_str());

    status_t err = mNetSession->sendRequest(
            sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    ALOGI("registerResponseHandler:id = %d, onReceiveDescribeResponse", mNextCSeq);

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveDescribeResponse);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::sendSetup(int32_t sessionID, const char *uri) {
    status_t err;
    
    if (mRTPSink == NULL) {
        mRTPSink = new RTPSink(mNetSession, mDumpEnable);
        ALOGI("*******register mRTPSink\n");
        looper()->registerHandler(mRTPSink);

        err = mRTPSink->init(sUseTCPInterleaving);

        if (err != OK) {
            ALOGE("------sendSetup RTPSink ERROR");
            looper()->unregisterHandler(mRTPSink->id());
            mRTPSink.clear();
            return err;
        }
    }

    AString request = StringPrintf("SETUP %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    if (sUseTCPInterleaving) {
        request.append("Transport: RTP/AVP/TCP;interleaved=0-1\r\n");
    } else {
        int32_t rtpPort = mRTPSink->getRTPPort();

        request.append(
                StringPrintf(
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                    rtpPort, rtpPort + 1));
    }

    request.append("\r\n");

    ALOGI("\nSend Request:\n%s", request.c_str());

    err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    ALOGI("registerResponseHandler:id = %d, onReceiveSetupResponse", mNextCSeq);
    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveSetupResponse);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::sendPlay(int32_t sessionID, const char *uri) {
    AString request = StringPrintf("PLAY %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    request.append(StringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
    request.append("\r\n");

    ALOGI("\nSend Request:\n%s", request.c_str());

    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    ALOGI("registerResponseHandler:id = %d, onReceivePlayResponse", mNextCSeq);
    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceivePlayResponse);

    ++mNextCSeq;

    return OK;
}

void WifiDisplaySink::onSetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    const char *content = data->getContent();
	
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("\r\n");

    ALOGI("\nSend Response:\n%s", response.c_str());
    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);

    if (strstr(content, "wfd_trigger_method: SETUP\r\n") != NULL) {
        AString url = StringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str());
        status_t err = sendSetup( sessionID, url.c_str());

        CHECK_EQ(err, (status_t)OK);
    }

}

void WifiDisplaySink::sendErrorResponse(
        int32_t sessionID,
        const char *errorDetail,
        int32_t cseq) {
    AString response;
    response.append("RTSP/1.0 ");
    response.append(errorDetail);
    response.append("\r\n");

    AppendCommonResponse(&response, cseq);

    response.append("\r\n");

    ALOGI("\nSend Response:\n%s", response.c_str());
    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

// static
void WifiDisplaySink::AppendCommonResponse(AString *response, int32_t cseq) {
    time_t now = time(NULL);
    struct tm *now2 = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);

    response->append("Date: ");
    response->append(buf);
    response->append("\r\n");

    response->append("User-Agent: stagefright/1.1 (Linux;Android 4.1)\r\n");

    if (cseq >= 0) {
        response->append(StringPrintf("CSeq: %d\r\n", cseq));
    }
}

}  // namespace android
