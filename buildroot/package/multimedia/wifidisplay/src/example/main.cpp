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

#define LOG_TAG "wfd"
#include <utils/Log.h>

#include <wifidisplay/WifiDisplaySink.h>

#include <foundation/AString.h>

#include <fcntl.h>

static void usage(const char *me) {
    fprintf(stderr,
            "usage:\n"
            "           %s -c host[:port]\tconnect to wifi source\n"
            "               -u uri        \tconnect to an rtsp uri\n"
            "               -l ip[:port] \tlisten on the specified port "
            "(create a sink)\n",
            me);
}

int osd_blank(const char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

    if(fd>=0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

using namespace android;

int main(int argc, char **argv) {

    AString connectToHost;
    int32_t connectToPort = -1;

    char cmd[128];

    int res;
    
    while ((res = getopt(argc, argv, "hc:l:u:i:")) >= 0) {
        switch (res) {
            case 'c':
            {
                const char *colonPos = strrchr(optarg, ':');

		{
                    connectToHost.setTo(optarg, colonPos - optarg);

                    char *end;
                    connectToPort = strtol(colonPos + 1, &end, 10);

                    if (*end != '\0' || end == colonPos + 1
                            || connectToPort < 1 || connectToPort > 65535) {
                        fprintf(stderr, "Illegal port specified.\n");
                        exit(1);
                    }
                }
                break;
            }

            case '?':
            case 'h':
		break;
            default:
                usage(argv[0]);
                exit(1);
		break;
        }
    }

    if (connectToPort < 0) {
        fprintf(stderr,
                "You need to select either source host or uri.\n");
        exit(1);
    }

    sp<ANetworkSession> session = new ANetworkSession;
    session->start();

    sp<ALooper> looper = new ALooper;
    sp<WifiDisplaySink> sink;

    {
        osd_blank("/sys/class/graphics/fb0/blank",1);//clear all osd0 ,don't need it on APK
        osd_blank("/sys/class/graphics/fb1/blank",1);//clear all osd1 ,don't need it on APK

        sink = new WifiDisplaySink(session);
        ALOGI("***************register sink\n");
        looper->registerHandler(sink);

        if (connectToPort >= 0) {
            sink->start(connectToHost.c_str(), connectToPort);
        }

        looper->start(false /* runOnCallingThread */);
    }
    
    fprintf(stderr, "input q/Q to quit process\n");
    
    do {
   	    memset(cmd,0,sizeof(cmd) );
   	    scanf ("%s",cmd);
        	    fprintf(stderr, "cmd=%s\n", cmd);
        
            if ((strcmp(cmd,"q")==0) || (strcmp(cmd,"Q")==0))
                break;
        
	    usleep(200000);
    }while(1);

    fprintf(stderr, "release resource\n");
    
    {
        osd_blank("/sys/class/graphics/fb0/blank",0);//clear all osd0 ,don't need it on APK
        osd_blank("/sys/class/graphics/fb1/blank",0);//clear all osd1 ,don't need it on APK
    }
    
    looper->stop();
    session->stop();
    fprintf(stderr, "release resource done\n");

    return 0;
}
