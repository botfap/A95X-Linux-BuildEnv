
#include "amlvideoinfo.h"

//media stream info class ,in clude audio and video
static int check_size_in_buffer(unsigned char *p, int len)
{
    unsigned int size;
    unsigned char *q = p;
    while ((q + 4) < (p + len)) {
        size = (*q << 24) | (*(q + 1) << 16) | (*(q + 2) << 8) | (*(q + 3));
        if (size & 0xff000000) {
            return 0;
        }

        if (q + size + 4 == p + len) {
            return 1;
        }

        q += size + 4;
    }
    return 0;
}

static int check_size_in_buffer3(unsigned char *p, int len)
{
    unsigned int size;
    unsigned char *q = p;
    while ((q + 3) < (p + len)) {
        size = (*q << 16) | (*(q + 1) << 8) | (*(q + 2));

        if (q + size + 3 == p + len) {
            return 1;
        }

        q += size + 3;
    }
    return 0;
}

static int check_size_in_buffer2(unsigned char *p, int len)
{
    unsigned int size;
    unsigned char *q = p;
    while ((q + 2) < (p + len)) {
        size = (*q << 8) | (*(q + 1));

        if (q + size + 2 == p + len) {
            return 1;
        }

        q += size + 2;
    }
    return 0;
}

gint amlVideoInfoInit(AmlStreamInfo *info, codec_para_t *pcodec, GstStructure  *structure)
{
    AmlVideoInfo *video = AML_VIDEOINFO_BASE(info);
    gint64 numerator=0;
    gint64 denominator=0;
    GValue * data_buf = NULL;
    gst_structure_get_int(structure, "width",&video->width);
    gst_structure_get_int(structure, "height",&video->height);
    gst_structure_get_fraction(structure, "framerate",&numerator,&denominator);
    
    if(numerator > 0){
        video->framerate = 96000 * denominator / numerator;
    }
   
    data_buf = (GValue *) gst_structure_get_value(structure, "codec_data"); 
    if(data_buf){
        info->configdata = gst_value_get_buffer(data_buf);
    }
    pcodec->am_sysinfo.height = video->height;
    pcodec->am_sysinfo.width = video->width;
    pcodec->am_sysinfo.rate = video->framerate;
    GST_WARNING("[%s:%d]width=%d height=%d framerate=%d codec_data=%p\n", 
        __FUNCTION__, __LINE__, video->width, video->height, video->framerate, data_buf);
    return 0;
}

void amlVdeoInfoFinalize(AmlStreamInfo *info)
{
    //do somethings it's self
    //TODO:
    amlStreamInfoFinalize(info);
    return;
}

AmlStreamInfo *createVideoInfo(gint size)
{
    AmlStreamInfo *info = createStreamInfo(size);
    AmlVideoInfo *video = AML_VIDEOINFO_BASE(info);
    video->width = 0;
    video->height = 0;
    video->framerate = 0;
    info->init = amlVideoInfoInit;
    info->finalize = amlVdeoInfoFinalize;
    GST_WARNING("[%s:]video info construct\n", __FUNCTION__);
    return info;
}

static gint amlInitH265(AmlStreamInfo* info, codec_para_t *pcodec, 
GstStructure  *structure)
{   
//    AmlVideoInfo *videoinfo = AML_VIDEOINFO_BASE(info);
    amlVideoInfoInit(info, pcodec, structure);
    pcodec->video_type = VFORMAT_HEVC;
    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
    pcodec->am_sysinfo.param = (void *)( EXTERNAL_PTS);
    return 0;
}
static gint h265_write_header(AmlStreamInfo* info, codec_para_t *pcodec)
{
    guint8 nal_start_code[] = {0x0, 0x0, 0x0, 0x1};
    int nal_len_size, nal_size;
    int i, j;
    guint config_size = 0;
    guint8 *config    = NULL;
    guint8 *hevc_data = NULL;
    GstBuffer *spps   = NULL;
    guint8 *sppsData  = NULL;  
    gint sppsLen = 0;
    int num_arrays = 0;
    
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] no codec data\n", __FUNCTION__, __LINE__);
        return 0;
    }
    config_size = GST_BUFFER_SIZE(info->configdata);	
    config = GST_BUFFER_DATA(info->configdata);
    hevc_data = config;
    GST_WARNING("add 265 header in stream\n");
    if (((hevc_data[0] == 0 && hevc_data[1] == 0 && hevc_data[2] == 0 && hevc_data[3] == 1)
        ||(hevc_data[0] == 0 && hevc_data[1] == 0 && hevc_data[2] == 1 )) && config_size < 1024) {
        g_print("add 265 header in stream before header len=%d", config_size);
        codec_write(pcodec, config, config_size);
        return 0;
    }
    
    if (config_size < 3) {
        return -1;
    }

    if (config_size < 10) {
        GST_WARNING("hvcC too short\n");
        return -1;
    }

    if (*hevc_data != 1) {
        GST_WARNING(" Unkonwn hvcC version %d\n", *hevc_data);
        return -1;
    }
    spps = gst_buffer_new_and_alloc(config_size);
    sppsData = GST_BUFFER_DATA(spps);
    
    /*skip 21 bytes*/
    hevc_data += 21;
   
    nal_len_size = (*hevc_data) & 3 + 1;
    hevc_data++;
    num_arrays = *hevc_data;
    hevc_data++;
    GST_WARNING("num_arrays:%d\n", num_arrays);

    /* Decode nal units from hvcC. */
    for(i = 0; i < num_arrays; i++) {
      int type = (*hevc_data) & 0x3F;
      hevc_data++;
      int cnt  = (*hevc_data << 8) | (*(hevc_data + 1));
      hevc_data += 2;
      for(j = 0; j < cnt; j++) {
        nal_size = (*hevc_data << 8) | (*(hevc_data + 1));
        memcpy(&(sppsData[sppsLen]), nal_start_code, sizeof(nal_start_code));
        sppsLen += 4;
        memcpy(&(sppsData[sppsLen]), hevc_data + 2, nal_size);
        sppsLen += nal_size;
        hevc_data += (nal_size + 2);
      }
    }

    GST_BUFFER_SIZE(spps) = sppsLen;
    codec_write(pcodec, sppsData, sppsLen);
    
    if(spps)
        gst_buffer_unref(spps);
    
    return 0;
}


gint amlInitH264(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{   
//    AmlVideoInfo *videoinfo = AML_VIDEOINFO_BASE(info);
    amlVideoInfoInit(info, pcodec, structure);
    pcodec->video_type = VFORMAT_H264;
    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
    pcodec->am_sysinfo.param = (void *)( EXTERNAL_PTS);
    return 0;
}

static gint h264_write_header(AmlStreamInfo* info, codec_para_t *pcodec)
{
    guint8 nal_start_code[] = {0x0, 0x0, 0x0, 0x1};
    int nalsize;
    int i;
    guint config_size = 0;
    guint8 * config = NULL;
    guint8 *p = NULL;
    GstBuffer *spps=NULL;
    guint8 *sppsData = NULL;  
    gint sppsLen = 0;
    int cnt = 0;
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] no codec data\n", __FUNCTION__, __LINE__);
        return 0;
    }
    config_size = GST_BUFFER_SIZE(info->configdata);	
    config = GST_BUFFER_DATA(info->configdata);
    p = config;
    GST_WARNING("add 264 header in stream\n");
    if (((p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
        ||(p[0] == 0 && p[1] == 0 && p[2] == 1 )) && config_size < 1024) {
        g_print("add 264 header in stream before header len=%d", config_size);
        codec_write(pcodec, config, config_size);
        return 0;
    }
    if (config_size < 4) {
        return -1;
    }

    if (config_size < 10) {
        GST_WARNING("avcC too short\n");
        return -1;
    }

    if (*p != 1) {
        GST_WARNING(" Unkonwn avcC version %d\n", *p);
        return -1;
    }
    spps = gst_buffer_new_and_alloc(config_size);
    sppsData = GST_BUFFER_DATA(spps);
    cnt = *(p + 5) & 0x1f; //number of sps
    GST_WARNING("number of sps :%d\n", cnt);
    p += 6;
    for (i = 0; i < cnt; i++) {
        nalsize = (*p << 8) | (*(p + 1));
        memcpy(&(sppsData[sppsLen]), nal_start_code, sizeof(nal_start_code));
        sppsLen += 4;
        memcpy(&(sppsData[sppsLen]), p + 2, nalsize);
        sppsLen += nalsize;
        p += (nalsize + 2);
    }

    cnt = *(p++); //Number of pps
    GST_WARNING("number of pps :%d\n", cnt);
    for (i = 0; i < cnt; i++) {
        nalsize = (*p << 8) | (*(p + 1));
        memcpy(&(sppsData[sppsLen]), nal_start_code, sizeof(nal_start_code));
        sppsLen += 4;
        memcpy(&(sppsData[sppsLen]), p + 2, nalsize);
        sppsLen += nalsize;
        p += (nalsize + 2);
    }
    if (sppsLen >= 1024) {
        GST_ERROR("header_len %d is larger than max length\n", sppsLen);
        return -1;
    }
    GST_BUFFER_SIZE(spps) = sppsLen;
    codec_write(pcodec, sppsData, sppsLen);
    
    if(spps)
        gst_buffer_unref(spps);
    return 0;
}

static gint h264_add_startcode(AmlStreamInfo* info, codec_para_t *pcodec, GstBuffer *buf)
{
    int nalsize, size = GST_BUFFER_SIZE(buf);
    unsigned char *data = GST_BUFFER_DATA(buf);
    unsigned char *p = data; 
    GstBuffer *hdrBuf;
    //g_print("h264_update_frame_header\n");
    if (p != NULL) {
        if (check_size_in_buffer(p, size)) {
            while ((p + 4) < (data + size)) {
                nalsize = (*p << 24) | (*(p + 1) << 16) | (*(p + 2) << 8) | (*(p + 3));
                *p = 0;
                *(p + 1) = 0;
                *(p + 2) = 0;
                *(p + 3) = 1;
                p += (nalsize + 4);
            }
            return 0;
        } else if (check_size_in_buffer3(p, size)) {
            while ((p + 3) < (data + size)) {
                nalsize = (*p << 16) | (*(p + 1) << 8) | (*(p + 2));
                *p = 0;
                *(p + 1) = 0;
                *(p + 2) = 1;
                p += (nalsize + 3);
            }
            return 0;
        } else if (check_size_in_buffer2(p, size)) {
            unsigned char *new_data;
            int new_len = 0;
            hdrBuf = gst_buffer_new_and_alloc(size + 2 * 1024);  
            //bufout = GST_BUFFER_DATA(hdrBuf);
            new_data = GST_BUFFER_DATA(hdrBuf); 
            if (!new_data) {
                return -1;
            }

            while ((p + 2) < (data + size)) {
                nalsize = (*p << 8) | (*(p + 1));
                *(new_data + new_len) = 0;
                *(new_data + new_len + 1) = 0;
                *(new_data + new_len + 2) = 0;
                *(new_data + new_len + 3) = 1;
                memcpy(new_data + new_len + 4, p + 2, nalsize);
                p += (nalsize + 2);
                new_len += nalsize + 4;
            }
            gst_buffer_set_data(buf, new_data, new_len);//size + 2 * 1024);
            //GST_BUFFER_SIZE(buf)= new_len;
            /*fix Bug 93125 do not free */
            //if(hdrBuf)	 
              //gst_buffer_unref(hdrBuf); 
        }
    } else {
        GST_ERROR("[%s]invalid pointer!\n", __FUNCTION__);
        return -1;
    }
   return 0; 
}

void amlH264Finalize(AmlStreamInfo *info)
{
    AmlStreamInfo *baseinfo = AML_STREAMINFO_BASE(info);
    //do somethings it's self
    //TODO:
    amlVdeoInfoFinalize(baseinfo);
    return;
}

void amlH265Finalize(AmlStreamInfo *info)
{
    AmlStreamInfo *baseinfo = AML_STREAMINFO_BASE(info);
    //do somethings it's self
    //TODO:
    amlVdeoInfoFinalize(baseinfo);
    return;
}

static AmlStreamInfo *newAmlInfoH265()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoH265));
    
    info->init = amlInitH265;
    info->writeheader = h265_write_header;
    info->add_startcode = h264_add_startcode;
    info->finalize = amlH265Finalize;
    return info;
}

AmlStreamInfo *newAmlInfoH264()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoH264));
    
    info->init = amlInitH264;
    info->writeheader = h264_write_header;
    info->add_startcode = h264_add_startcode;
    info->finalize = amlH264Finalize;
    return info;
}

gint amlInitMpeg(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{   
    AmlInfoMpeg *mpeg = (AmlInfoMpeg *)info;
    
    gint version; 
    gst_structure_get_int (structure, "mpegversion", &version);
    mpeg->version = version;
    GST_WARNING("[%s:%d]version=%d\n", __FUNCTION__, __LINE__, version);
    switch(version){
        case 1:
        case 2:
            pcodec->video_type = VFORMAT_MPEG12;
            pcodec->am_sysinfo.format = 0;
            info->writeheader = NULL;
            break;
        case 4:
            pcodec->video_type = VFORMAT_MPEG4;
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
            break;
        default:break;
    }
    amlVideoInfoInit(info, pcodec, structure);
    return 0;
}

AmlStreamInfo *newAmlInfoMpeg()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoMpeg));
    info->init = amlInitMpeg;
    
    return info;
}

gint amlInitMsmpeg(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{   
    AmlInfoMsmpeg *msmpeg = (AmlInfoMsmpeg *)info;
    
    gint version; 
    gst_structure_get_int (structure, "msmpegversion", &version);
    msmpeg->version = version;
    switch(version){
        case 43:
            pcodec->video_type = VFORMAT_MPEG4;
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
            
            break;
        default:break;
    }
    return 0;
}

AmlStreamInfo *newAmlInfoMsmpeg()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoMsmpeg));
    info->init = amlInitMsmpeg;
    info->writeheader = NULL;
    return info;
}

gint amlInitH263(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{    
    pcodec->video_type = VFORMAT_MPEG4;
    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H263;     
    return 0;
}

AmlStreamInfo *newAmlInfoH263()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoH263));
    info->init = amlInitH263;
    info->writeheader = NULL;
    return info;
}

gint amlInitJpeg(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{      
    pcodec->video_type = VFORMAT_MJPEG;
    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MJPEG;     
    return 0;
}

static int mjpeg_write_header(AmlStreamInfo* info, codec_para_t *pcodec)
{
    const guint8 mjpeg_addon_data[] = {
        0xff, 0xd8, 0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01, 0x00, 0x03, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10,
        0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
        0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
        0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1,
        0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
        0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64,
        0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95,
        0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
        0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
        0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
        0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
        0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0x11, 0x00, 0x02, 0x01,
        0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51,
        0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1,
        0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24,
        0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a,
        0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82,
        0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
        0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
        0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4,
        0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
    };
    codec_write(pcodec,&mjpeg_addon_data,sizeof(mjpeg_addon_data));
    return 0;
}

AmlStreamInfo *newAmlInfoJpeg()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoJpeg));
    info->init = amlInitJpeg;
    info->writeheader= mjpeg_write_header;
    return info;
}

static int wmv3_add_startcode(AmlStreamInfo* info, codec_para_t *vpcodec, GstBuffer *buf)
{
    unsigned i, check_sum = 0;
    guint32 data_len = GST_BUFFER_SIZE(info->configdata) + 4;
    GstBuffer *hdrBuf=NULL;
    char *bufout = NULL;
    int data_size = GST_BUFFER_SIZE(buf);
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] no codec data\n", __FUNCTION__, __LINE__);
        return 0;
    }
    hdrBuf = gst_buffer_new_and_alloc(26);
    bufout = GST_BUFFER_DATA(hdrBuf);
    bufout[0] = 0;
    bufout[1] = 0;
    bufout[2] = 1;
    bufout[3] = 0x10;

    bufout[4] = 0;
    bufout[5] = (data_len >> 16) & 0xff;
    bufout[6] = 0x88;
    bufout[7] = (data_len >> 8) & 0xff;
    bufout[8] = data_len & 0xff;
    bufout[9] = 0x88;

    bufout[10] = 0xff;
    bufout[11] = 0xff;
    bufout[12] = 0x88;
    bufout[13] = 0xff;
    bufout[14] = 0xff;
    bufout[15] = 0x88;

    for (i = 4 ; i < 16 ; i++) {
        check_sum += bufout[i];
    }

    bufout[16] = (check_sum >> 8) & 0xff;
    bufout[17] = check_sum & 0xff;
    bufout[18] = 0x88;
    bufout[19] = (check_sum >> 8) & 0xff;
    bufout[20] = check_sum & 0xff;
    bufout[21] = 0x88;

    bufout[22] = (vpcodec->am_sysinfo.width >> 8) & 0xff;
    bufout[23] = vpcodec->am_sysinfo.width & 0xff;
    bufout[24] = (vpcodec->am_sysinfo.height >> 8) & 0xff;
    bufout[25] = vpcodec->am_sysinfo.height & 0xff;  
	
    hdrBuf = gst_buffer_merge(hdrBuf,info->configdata);	 
    GST_BUFFER_SIZE(hdrBuf) = data_len + 26-4;
    codec_write(vpcodec,GST_BUFFER_DATA(hdrBuf),GST_BUFFER_SIZE(hdrBuf));

    if(hdrBuf)	
        gst_buffer_unref(hdrBuf);
    check_sum = 0;
    hdrBuf = gst_buffer_new_and_alloc(22);
    bufout = GST_BUFFER_DATA(hdrBuf);	
    bufout[0] = 0;
    bufout[1] = 0;
    bufout[2] = 1;
    bufout[3] = 0xd;

    bufout[4] = 0;
    bufout[5] = (data_size >> 16) & 0xff;
    bufout[6] = 0x88;
    bufout[7] = (data_size >> 8) & 0xff;
    bufout[8] = data_size & 0xff;
    bufout[9] = 0x88;

    bufout[10] = 0xff;
    bufout[11] = 0xff;
    bufout[12] = 0x88;
    bufout[13] = 0xff;
    bufout[14] = 0xff;
    bufout[15] = 0x88;

    for (i = 4 ; i <  16 ; i++) {
        check_sum +=  bufout[i];
    }

    bufout[16] = (check_sum >> 8) & 0xff;
    bufout[17] = check_sum & 0xff;
    bufout[18] = 0x88;
    bufout[19] = (check_sum >> 8) & 0xff;
    bufout[20] = check_sum & 0xff;
    bufout[21] = 0x88;
    codec_write(vpcodec,GST_BUFFER_DATA(hdrBuf),22);

    if(hdrBuf)	
        gst_buffer_unref(hdrBuf);
    bufout = NULL;

}
static int wmv3_write_header(AmlStreamInfo* info, codec_para_t *vpcodec)
{
    unsigned i, check_sum = 0;
    guint32 data_len = GST_BUFFER_SIZE(info->configdata) + 4;
    GstBuffer *hdrBuf=NULL;
    unsigned char *bufout = NULL;  
    if(NULL == info->configdata){
        GST_WARNING("[%s:%d] no codec data\n", __FUNCTION__, __LINE__);
        return 0;
    }

    hdrBuf = gst_buffer_new_and_alloc(26);
    bufout = GST_BUFFER_DATA(hdrBuf);

    bufout[0] = 0;
    bufout[1] = 0;
    bufout[2] = 1;
    bufout[3] = 0x10;

    bufout[4] = 0;
    bufout[5] = (data_len >> 16) & 0xff;
    bufout[6] = 0x88;
    bufout[7] = (data_len >> 8) & 0xff;
    bufout[8] = data_len & 0xff;
    bufout[9] = 0x88;

    bufout[10] = 0xff;
    bufout[11] = 0xff;
    bufout[12] = 0x88;
    bufout[13] = 0xff;
    bufout[14] = 0xff;
    bufout[15] = 0x88;

    for (i = 4 ; i < 16 ; i++) {
        check_sum += bufout[i];
    }

    bufout[16] = (check_sum >> 8) & 0xff;
    bufout[17] = check_sum & 0xff;
    bufout[18] = 0x88;
    bufout[19] = (check_sum >> 8) & 0xff;
    bufout[20] = check_sum & 0xff;
    bufout[21] = 0x88;

    bufout[22] = (vpcodec->am_sysinfo.width >> 8) & 0xff;
    bufout[23] = vpcodec->am_sysinfo.width & 0xff;
    bufout[24] = (vpcodec->am_sysinfo.height >> 8) & 0xff;
    bufout[25] = vpcodec->am_sysinfo.height & 0xff;

    hdrBuf = gst_buffer_merge(hdrBuf,info->configdata);	 
    GST_BUFFER_SIZE(hdrBuf) = data_len + 26-4;
	
    codec_write(vpcodec,GST_BUFFER_DATA(hdrBuf),GST_BUFFER_SIZE(hdrBuf));

    if(hdrBuf)	
        gst_buffer_unref(hdrBuf);
    return 0;
}

gint amlInitWmv(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{   
    AmlInfoWmv *wmv = (AmlInfoWmv *)info;
    gint version;
    guint32 format;        
    gst_structure_get_int(structure, "wmvversion", &version);
    wmv->version = version;
    gst_structure_get_fourcc(structure, "format", &format);
    pcodec->video_type = VFORMAT_VC1;
    switch(format){
        case GST_MAKE_FOURCC ('W', 'V', 'C', '1'):
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
            break;
        case GST_MAKE_FOURCC ('W', 'V', 'C', '3'):
        case GST_MAKE_FOURCC ('W', 'M', 'V', '3'):
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_WMV3;
            info->add_startcode = wmv3_add_startcode;
            break;
        default:break;
    }
    amlVideoInfoInit(info, pcodec, structure);

    return 0;
}
AmlStreamInfo *newAmlInfoWmv()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoWmv));
    info->init = amlInitWmv;
    info->writeheader = NULL;
    return info;
}
static int divx3_write_header(AmlStreamInfo* info, codec_para_t *pcodec)
{
   unsigned i = (pcodec->am_sysinfo.width<< 12) | (pcodec->am_sysinfo.height & 0xfff);
    unsigned char divx311_add[10] = {
        0x00, 0x00, 0x00, 0x01,
        0x20, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    divx311_add[5] = (i >> 16) & 0xff;
    divx311_add[6] = (i >> 8) & 0xff;
    divx311_add[7] = i & 0xff;
    codec_write(pcodec, divx311_add, sizeof(divx311_add));
    return 0;
}

static gint divx3_add_startcode(AmlStreamInfo* info, codec_para_t *pcodec, GstBuffer *buf)
{
#define DIVX311_CHUNK_HEAD_SIZE 13
    gint data_size = GST_BUFFER_SIZE(buf);
    guint8 prefix[DIVX311_CHUNK_HEAD_SIZE + 4] = {
        0x00, 0x00, 0x00, 0x01, 0xb6, 'D', 'I', 'V', 'X', '3', '.', '1', '1',
    };

    prefix[DIVX311_CHUNK_HEAD_SIZE + 0] = (data_size >> 24) & 0xff;
    prefix[DIVX311_CHUNK_HEAD_SIZE + 1] = (data_size >> 16) & 0xff;
    prefix[DIVX311_CHUNK_HEAD_SIZE + 2] = (data_size >>  8) & 0xff;
    prefix[DIVX311_CHUNK_HEAD_SIZE + 3] = data_size & 0xff;
    codec_write(pcodec, prefix, sizeof(prefix));
    return 0;
}

gint amlInitDivx(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{   
    AmlInfoDivx *divx = (AmlInfoDivx *)info;
    gint version;
    pcodec->video_type = VFORMAT_MPEG4;	
    
    gst_structure_get_int(structure, "divxversion", &version);
    divx->version = version;

    switch(version){
        case 3:
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
            info->writeheader = divx3_write_header;
            info->add_startcode = divx3_add_startcode;
            break;
        case 4:
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_4;
            break;
        case 5:
            pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
            break;
        default:break;
    }
    amlVideoInfoInit(info, pcodec, structure);

    return 0;
}

AmlStreamInfo *newAmlInfoDivx()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoDivx));
    info->init = amlInitDivx;
    info->writeheader = NULL;
    info->add_startcode = NULL;

    return info;
}

gint amlInitXvid(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{       
    pcodec->video_type = VFORMAT_MPEG4;
    pcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;     
    amlVideoInfoInit(info, pcodec, structure);
    return 0;
}

AmlStreamInfo *newAmlInfoXvid()
{
    AmlStreamInfo *info = createVideoInfo(sizeof(AmlInfoXvid));
    info->init = amlInitXvid;
    
    return info;
}

static const AmlStreamInfoPool amlVstreamInfoPool[] = {
    /*******video format information*******/
    {"video/x-h265", newAmlInfoH265},
    {"video/x-h264", newAmlInfoH264},
    {"video/mpeg", newAmlInfoMpeg},
    {"video/x-msmpeg", newAmlInfoMsmpeg},
    {"video/x-h263", newAmlInfoH263},
    {"video/x-jpeg", newAmlInfoJpeg},
    {"video/x-wmv", newAmlInfoWmv},
    {"video/x-divx", newAmlInfoDivx},
    {"video/x-xvid", newAmlInfoXvid},
    {NULL, NULL}
};
AmlStreamInfo *amlVstreamInfoInterface(gchar *format)
{
    AmlStreamInfoPool *p  = amlVstreamInfoPool;
    AmlStreamInfo *info = NULL;
    info =amlStreamInfoInterface(format,p);
    return info;	
}

