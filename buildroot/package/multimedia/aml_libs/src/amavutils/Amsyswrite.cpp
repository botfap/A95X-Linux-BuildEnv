#define LOG_TAG "amSystemWrite"

#include <systemwrite/ISystemWriteService.h>

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <Amsyswrite.h>
#include <unistd.h>


using namespace android;

class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier() {
        }

        void binderDied(const wp<IBinder>& who) {
            ALOGW("system_write died!");
        }
};

static sp<ISystemWriteService> amSystemWriteService;
static sp<DeathNotifier> amDeathNotifier;
static  Mutex            amLock;
static  Mutex            amgLock;

const sp<ISystemWriteService>& getSystemWriteService()
{
    Mutex::Autolock _l(amgLock);
    if (amSystemWriteService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("system_write"));
            if (binder != 0)
                break;
            ALOGW("SystemWriteService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (amDeathNotifier == NULL) {
            amDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(amDeathNotifier);
        amSystemWriteService = interface_cast<ISystemWriteService>(binder);
    }
    ALOGE_IF(amSystemWriteService==0, "no CameraService!?");
    return amSystemWriteService;
}

int amSystemWriteGetProperty(const char* key, char* value)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0){
        String16 v;
        if(sws->getProperty(String16(key), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }
    return -1;

}

int amSystemWriteGetPropertyStr(const char* key, char* def, char* value)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0){
        String16 v;
        String16 d(def);
        sws->getPropertyString(String16(key), d, v);
        strcpy(value, String8(v).string());
    }
    strcpy(value, def);
    return -1;
}

int amSystemWriteGetPropertyInt(const char* key, int def)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0) {
        return sws->getPropertyInt(String16(key), def);
    }
    return def;
}


long amSystemWriteGetPropertyLong(const char* key, long def)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0) {
        return	sws->getPropertyLong(String16(key), def);
    }
    return def;
}


int amSystemWriteGetPropertyBool(const char* key, int def)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0) { 
        if(sws->getPropertyBoolean(String16(key), def)) {
            return 1;
        } else {
            return 0;
        }
    }
    return def;

}

void amSystemWriteSetProperty(const char* key, const char* value)
{
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0){
        sws->setProperty(String16(key), String16(value));
    }  
}

int amSystemWriteReadSysfs(const char* path, char* value)
{
    //ALOGD("amSystemWriteReadNumSysfs:%s",path);
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0){
        String16 v;
        if(sws->readSysfs(String16(path), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }
    return -1;
}

int amSystemWriteReadNumSysfs(const char* path, char* value, int size)
{
    //ALOGD("amSystemWriteReadNumSysfs:%s",path);

    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0 && value != NULL && access(path, 0) != -1){
        String16 v;
        if(sws->readSysfs(String16(path), v)) {
            if(v.size() != 0) {
                //ALOGD("readSysfs ok:%s,%s,%d", path, String8(v).string(), String8(v).size());
                memset(value, 0, size);
                if(size <= String8(v).size() + 1) {
                    memcpy(value, String8(v).string(), size - 1);
                    value[strlen(value)] = '\0';

                } else {
                    strcpy(value, String8(v).string());

                }
                return 0;
            }
        }

    }
    //ALOGD("[false]amSystemWriteReadNumSysfs%s,",path);
    return -1;
}

int amSystemWriteWriteSysfs(const char* path, char* value)
{
    //ALOGD("amSystemWriteWriteSysfs:%s",path);
    const sp<ISystemWriteService>& sws = getSystemWriteService();
    if(sws != 0){
        String16 v(value);
        if(sws->writeSysfs(String16(path), v)){
            //ALOGD("writeSysfs ok");
            return 0;
        }
    }
    //ALOGD("[false]amSystemWriteWriteSysfs%s,",path);
    return -1;
}
