
#include "amlstreaminfo.h"
#include "amlvideoinfo.h"
#include "amlaudioinfo.h"

//media stream info class ,in clude audio and video
//typedef int (*AmlCallBackFunc)(void *data);

int amlStreamInfoWriteHeader(AmlStreamInfo *info, codec_para_t *pcodec)
{
	GstMapInfo map;
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] configdata is null", __FUNCTION__, __LINE__);
        return 0;
    }
    gst_buffer_map(info->configdata, &map, GST_MAP_READ);
    guint8 *configbuf = map.data;
    gint configsize = map.size;
    if(configbuf && (configsize > 0)){
        codec_write(pcodec, configbuf, configsize);
    }
    gst_buffer_unmap(info->configdata, &map);
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
//    GST_WARNING("[%s:%d]", __FUNCTION__, __LINE__);
    return info;
}

void amlStreamInfoFinalize(AmlStreamInfo *info)
{
    if (info->configdata)
        gst_buffer_unref(info->configdata);
	
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
