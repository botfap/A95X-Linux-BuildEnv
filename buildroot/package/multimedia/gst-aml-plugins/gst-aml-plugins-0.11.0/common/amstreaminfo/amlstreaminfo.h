
#ifndef __AML_STREAMINFO_H__
#define __AML_STREAMINFO_H__
#include <gst/gst.h>
#include  "../include/codec.h"

#define AML_STREAMINFO_BASE(x) ((AmlStreamInfo *)(x))

typedef struct stAmlStreamInfo AmlStreamInfo;
struct stAmlStreamInfo{
//public:
    gchar *format;
    gint (*init)(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure); //pure virtual function
    gint (*writeheader)(AmlStreamInfo* info, codec_para_t *pcodec);
    gint (*add_startcode)(AmlStreamInfo* info, codec_para_t *pcodec, GstBuffer *buf); //pure virtual function
    void (*finalize)(AmlStreamInfo* info);
//protected:
    GstBuffer *configdata;
//private:
};

typedef struct{
    const gchar *name;
    AmlStreamInfo *(*newStreamInfo)();
}AmlStreamInfoPool;

AmlStreamInfo *amlStreamInfoInterface(gchar *format, AmlStreamInfoPool *amlStreamInfoPool);
AmlStreamInfo *createStreamInfo(gint size);
void amlStreamInfoFinalize(AmlStreamInfo *info);

#endif

