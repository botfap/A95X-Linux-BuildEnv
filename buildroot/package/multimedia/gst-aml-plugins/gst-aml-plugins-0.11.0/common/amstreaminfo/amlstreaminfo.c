
#include "amlstreaminfo.h"
#include "amlvideoinfo.h"
#include "amlaudioinfo.h"

//media stream info class ,in clude audio and video
//typedef int (*AmlCallBackFunc)(void *data);

int amlStreamInfoWriteHeader(AmlStreamInfo *info, codec_para_t *pcodec)
{
    GST_WARNING("[%s:%d]\n", __FUNCTION__, __LINE__);
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] configdata is null\n", __FUNCTION__, __LINE__);
        return 0;
    }
    guint8 *configbuf = GST_BUFFER_DATA(info->configdata);
    gint configsize = GST_BUFFER_SIZE(info->configdata);
    if(configbuf && (configsize > 0)){
        codec_write(pcodec, configbuf, configsize);
    }
    return 0;
}

AmlStreamInfo *createStreamInfo(gint size)
{
    AmlStreamInfo *info = g_malloc(size);
    info->init = NULL;
    info->writeheader = amlStreamInfoWriteHeader;
    info->add_startcode = NULL;
    info->finalize = amlStreamInfoFinalize;
    info->configdata = NULL;
    GST_WARNING("[%s:%d]\n", __FUNCTION__, __LINE__);
    return info;
}

void amlStreamInfoFinalize(AmlStreamInfo *info)
{
    if(info){
        g_free(info);
    }
}
AmlStreamInfo *amlStreamInfoInterface(gchar *format, AmlStreamInfoPool *amlStreamInfoPool)
{
    AmlStreamInfoPool *p  = amlStreamInfoPool; 
    AmlStreamInfo *info = NULL;
    int i = 0; 		
    while(p&&p->name){
        if(!strcmp(p->name, format)){
            if(p->newStreamInfo){
                info = p->newStreamInfo();
            }
            break;
        }
        p++;
    }
    return info;
}


