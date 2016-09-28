
#ifndef __AML_VIDEOINFO_H__
#define __AML_VIDEOINFO_H__

#include "amlstreaminfo.h"

#define AML_VIDEOINFO_BASE(x) ((AmlVideoInfo *)(x))


#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)
#define UNIT_FREQ       96000
#define PTS_FREQ        90000
#define AV_SYNC_THRESH    PTS_FREQ*30

typedef struct{
    AmlStreamInfo streaminfo;
//protected:
    gint height;
    gint width;
    gint framerate;
}AmlVideoInfo;

typedef struct{
    AmlVideoInfo videoinfo;
}AmlInfoH264;

typedef struct{
    AmlVideoInfo videoinfo;
}AmlInfoH265;

typedef struct{
    AmlVideoInfo videoinfo;
    gint version;
}AmlInfoMpeg;

typedef struct{
    AmlVideoInfo videoinfo;
    gint version;
}AmlInfoMsmpeg;

typedef struct{
    AmlVideoInfo videoinfo;
}AmlInfoH263;

typedef struct{
    AmlVideoInfo videoinfo;
}AmlInfoJpeg;

typedef struct{
    AmlVideoInfo videoinfo;
    gint version;
}AmlInfoWmv;

typedef struct{
    AmlVideoInfo videoinfo;
    gint version;
}AmlInfoDivx;

typedef struct{
    AmlVideoInfo videoinfo;
}AmlInfoXvid;

AmlStreamInfo *newAmlInfoH264();
AmlStreamInfo *newAmlInfoMpeg();
AmlStreamInfo *newAmlInfoMsmpeg();
AmlStreamInfo *newAmlInfoH263();
AmlStreamInfo *newAmlInfoJpeg();
AmlStreamInfo *newAmlInfoWmv();
AmlStreamInfo *newAmlInfoDivx();
AmlStreamInfo *newAmlInfoXvid();
AmlStreamInfo *createVideoInfo(gint size);
AmlStreamInfo *amlVstreamInfoInterface(gchar *format);
#endif

