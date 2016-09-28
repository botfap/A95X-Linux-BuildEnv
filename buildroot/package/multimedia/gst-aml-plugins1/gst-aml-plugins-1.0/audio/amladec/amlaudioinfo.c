#include "amlaudioinfo.h"

gint amlAudioInfoInit(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	GValue *extra_data_buf = NULL;
	GValue *headers = NULL;
	guint i, num;
	AmlAudioInfo *audio = AML_AUDIOINFO_BASE(info);

	if (gst_structure_has_field(structure, "rate"))
		gst_structure_get_int(structure, "rate", &audio->sample_rate);
	if (gst_structure_has_field(structure, "channels"))
		gst_structure_get_int(structure, "channels", &audio->channels);

	pcodec->audio_info.sample_rate = audio->sample_rate;
	pcodec->audio_info.channels = audio->channels;
     aml_dump_structure(structure);
	if (gst_structure_has_field(structure, "codec_data")) {
		extra_data_buf = (GValue *) gst_structure_get_value(structure, "codec_data");
		if (extra_data_buf) {
			info->configdata = gst_buffer_copy(gst_value_get_buffer(extra_data_buf));
			AML_DUMP_BUFFER(info->configdata, "Audio Codec Data");
		}
	}
	
	if (gst_structure_has_field(structure, "streamheader")) {
		 headers = gst_structure_get_value (structure, "streamheader");
          if (headers == NULL || !GST_VALUE_HOLDS_ARRAY (headers)) {
            GST_WARNING ( "no 'streamheader' field in input caps,\n");
            return FALSE;
          }
        num = gst_value_array_get_size (headers);
	   GST_WARNING ( "num=%d\n",num);	
    

    for (i = 0; i < num; ++i) {
        const GValue *header_val;
        GstBuffer *header_buf;

        header_val = gst_value_array_get_value (headers, i);
        if (header_val == NULL || !GST_VALUE_HOLDS_BUFFER (header_val))
          return FALSE;
	   if(i==0){
	       info->configdata = gst_buffer_copy(gst_value_get_buffer(header_val));
		  if(pcodec->audio_type == AFORMAT_VORBIS)
		   	pcodec->audio_info.extradata[1] = gst_buffer_get_size (info->configdata );
	   } else {
	   	   header_buf = g_value_dup_boxed (header_val);
		   if(pcodec->audio_type ==AFORMAT_VORBIS && i==1)
		   	pcodec->audio_info.extradata[2] = gst_buffer_get_size (header_buf);
                GST_WARNING ("pushing header buffer of %" G_GSIZE_FORMAT " bytes "
              " into adapter", gst_buffer_get_size (header_buf));
		   if(header_buf) {
	               gst_buffer_copy_into(info->configdata,header_buf,GST_BUFFER_COPY_MEMORY,0,-1);
		       gst_buffer_unref(header_buf);
		   }
		}
	  } 
    }
	 GST_WARNING("Audio: samplerate=%d channels=%d", audio->sample_rate, audio->channels);
	 if(0 == pcodec->audio_info.channels)
	 	pcodec->audio_info.channels = 2;
	 if(0 == pcodec->audio_info.sample_rate)
	 	pcodec->audio_info.sample_rate =48000;
	 pcodec->audio_samplerate = pcodec->audio_info.sample_rate;
	 pcodec->audio_channels = pcodec->audio_info.channels;
	return 0;
}

void amlAudioInfoFinalize(AmlStreamInfo* info)
{
    amlStreamInfoFinalize(info);
    return;
}

 AmlStreamInfo *createAudioInfo(gint size)
{
    AmlStreamInfo *info = createStreamInfo(size);
    AmlAudioInfo *audio = AML_AUDIOINFO_BASE(info);
    audio->sample_rate = 0;         ///< audio stream sample rate
    audio->channels = 0;            ///< audio stream channels
    info->init = amlAudioInfoInit;
    info->finalize = amlAudioInfoFinalize;
     GST_WARNING("[%s:]Audio info construct", __FUNCTION__);
    return info;
}

//copy from ffmpeg avcodec.h,some format dsp need codec_id to call audio decoder

/**********************************************************************
0: syncword 12 always: '111111111111'
12: ID 1 0: MPEG-4, 1: MPEG-2
13: layer 2 always: '00'
15: protection_absent 1
16: profile 2
18: sampling_frequency_index 4
22: private_bit 1
23: channel_configuration 3
26: original/copy 1
27: home 1
28: emphasis 2 only if ID == 0

ADTS Variable header: these can change from frame to frame
28: copyright_identification_bit 1
29: copyright_identification_start 1
30: aac_frame_length 13 length of the frame including header (in bytes)
43: adts_buffer_fullness 11 0x7FF indicates VBR
54: no_raw_data_blocks_in_frame 2
ADTS Error check
crc_check 16 only if protection_absent == 0
}

**************************************************************************/
int extract_adts_header_info(AmlStreamInfo *info,codec_para_t *pcodec, GstBuffer *buffer)
{
	adts_header_t hdr;
	guint8 *p = NULL;
	guint8 *buf;
	GstMapInfo map;
	if (info->configdata) {
		gst_buffer_map(info->configdata, &map, GST_MAP_READ);
		p = map.data;
		hdr.profile = (*p >> 3) - 1;                    // 5 bits
		hdr.sample_freq_idx = (*p & 0x7) << 1 | (*(p + 1) >> 7);                    // 4 bits
		hdr.channel_configuration = (*(p + 1) & 0x7f) >> 3;                    // 4 bits
		if ((*p >> 3) == 5/*AOT_SBR*/|| ((*p >> 3) == 29/*AOT_PS*/&&
		// check for W6132 Annex YYYY draft MP3onMP4
				!((*(p + 1) & 0x7) & 0x03 && !(((*(p + 1) & 0x7) << 6 | (*(p + 2) >> 2)) & 0x3F)))) {
			//skip 4  bits for ex-sample-freq_idx
			hdr.profile = ((*(p + 2) & 0x7f) >> 2) - 1;                    // 5 bits

		}
		GST_INFO("extract aac insert adts header:profile %d,sr_index %d,ch_config %d", hdr.profile,
				hdr.sample_freq_idx, hdr.channel_configuration);
		gst_buffer_unmap(info->configdata, &map);
	} else {
		return 0;
	}

	hdr.syncword = 0xfff;
	hdr.id = 0;
	hdr.layer = 0;
	hdr.protection_absent = 1;
	hdr.private_bit = 0;
	hdr.original_copy = 0;
	hdr.home = 0;
	hdr.copyright_identification_bit = 0;
	hdr.copyright_identification_start = 0;
	hdr.aac_frame_length = 0;
	hdr.adts_buffer_fullness = 0x7ff;
	hdr.number_of_raw_data_blocks_in_frame = 0;
	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	buf = map.data;
	if (buf) {
		buf[0] = (char) (hdr.syncword >> 4);
		buf[1] = (char) ((hdr.syncword & 0xf << 4) | (hdr.id << 3) | (hdr.layer << 1) | hdr.protection_absent);
		buf[2] = (char) ((hdr.profile << 6) | (hdr.sample_freq_idx << 2) | (hdr.private_bit << 1)
				| (hdr.channel_configuration >> 2));
		buf[3] = (char) (((hdr.channel_configuration & 0x3) << 6) | (hdr.original_copy << 5) | (hdr.home << 4)
				| (hdr.copyright_identification_bit << 3) | (hdr.copyright_identification_start << 2)
				| (hdr.aac_frame_length) >> 11);
		buf[4] = (char) ((hdr.aac_frame_length >> 3) & 0xff);
		buf[5] = (char) (((hdr.aac_frame_length & 0x7) << 5) | (hdr.adts_buffer_fullness >> 6));
		buf[6] = (char) (((hdr.adts_buffer_fullness & 0x3f) << 2) | hdr.number_of_raw_data_blocks_in_frame);
		gst_buffer_unmap(buffer, &map);

		gst_buffer_unref(info->configdata);
		info->configdata = gst_buffer_copy(buffer);

//        gst_buffer_make_writable(info->configdata);
//        gst_buffer_copy_into(info->configdata, buffer, GST_BUFFER_COPY_DEEP, 0, ADTS_HEADER_SIZE);
		gst_buffer_set_size(info->configdata, ADTS_HEADER_SIZE);
//        gst_buffer_map(info->configdata, &map, GST_MAP_READ);
//        GST_MEMDUMP2("info->configdata", map.data, map.size);
//        gst_buffer_unmap(info->configdata, &map);

	} else {
		GST_ERROR("[%s:%d]no memory for extract adts header!", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

gint adts_add_startcode(AmlStreamInfo* info, codec_para_t *pcodec, GstBuffer *buffer)
{
    gint8 *buf ;
    int i;
    int size = ADTS_HEADER_SIZE + gst_buffer_get_size(buffer);   // 13bit valid
    size &= 0x1fff;
    guint8 *adts_header=NULL;
    GstBuffer *buffer1;
    GstMapInfo map;
    if (!info->configdata) {
    	return 0;
    }
//Some aac es stream already has adts header,need check the first ADTS_HEADER_SIZE bytes
    while (buffer&&gst_buffer_get_size(buffer)>=ADTS_HEADER_SIZE) {
        buffer1 = gst_buffer_new_and_alloc(ADTS_HEADER_SIZE);
        if (buffer1) {
        	gst_buffer_copy_into(buffer1, buffer, GST_BUFFER_COPY_DEEP, 0, ADTS_HEADER_SIZE);
        	gst_buffer_map(buffer1, &map, GST_MAP_READ);
        	adts_header=map.data;
        	//gst_buffer_unmap(buffer1, &map);
			
		if (((adts_header[0] << 4) | (adts_header[1] & 0xF0) >> 4) != 0xFFF)                    //sync code
			break;
		if ((((*(adts_header + 3) & 0x2) << 11) | ((*(adts_header + 4) & 0xFF) << 3)
				| ((*(adts_header + 5) & 0xE0) >> 5)) != gst_buffer_get_size(buffer))           //frame length
			break;
		}
		else  break;
        GST_WARNING(" AAC es has adts header,don't add again");
        gst_buffer_unmap(buffer1, &map);
        if(adts_header){
            gst_buffer_unref(buffer1);
             buffer1=NULL;
        }		
        return 0; //T
    }


    if(adts_header){
        gst_buffer_unmap(buffer1, &map);	
        gst_buffer_unref(buffer1);
        buffer1=NULL;
    }
	
    gst_buffer_map(info->configdata, &map, GST_MAP_WRITE);
	buf = map.data;
    if (buf!=NULL) {
        buf[3] = (buf[3] & 0xfc) | (size >> 11);
        buf[4] = (size >> 3) & 0xff;
        buf[5] = (buf[5] & 0x1f) | ((size & 0x7) << 5);
        if ( gst_buffer_get_size(info->configdata) == ADTS_HEADER_SIZE) {
            codec_write(pcodec,buf,gst_buffer_get_size(info->configdata));
	 // buffer = gst_buffer_merge(info->configdata,buffer);
        }
    }
	gst_buffer_unmap(info->configdata, &map);

    return 0;
} 
void * aac_finalize(AmlStreamInfo* info)
{    
    AmlAinfoMpeg *audio = (AmlAinfoMpeg*)info;
    if(audio->headerbuf) {
        gst_buffer_unref(audio->headerbuf);
        audio->headerbuf=NULL;
    }
    amlStreamInfoFinalize(info);
    GST_WARNING("[%s:%d]", __FUNCTION__, __LINE__);
    return 0; //T
}
gint amlInitAmpeg(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	AmlAinfoMpeg *audio = (AmlAinfoMpeg*) info;
	gst_structure_get_int(structure, "mpegversion", &audio->version);
	GST_ERROR("Audio: mpegversion = %d", audio->version);
	switch (audio->version) {
	case 1:
		pcodec->audio_type = AFORMAT_MPEG;
		amlAudioInfoInit(info, pcodec, structure);
		break;
	case 2:
	case 4:
		pcodec->audio_type = AFORMAT_AAC;
		amlAudioInfoInit(info, pcodec, structure);
		audio->headerbuf = gst_buffer_new_and_alloc(ADTS_HEADER_SIZE);
		extract_adts_header_info(info, pcodec, audio->headerbuf);
		info->writeheader = NULL;
		info->add_startcode = adts_add_startcode;
		info->finalize = aac_finalize;
		break;
	default:
		break;
	}
	return 0;
}

AmlStreamInfo* newAmlAinfoMpeg()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoMpeg));
    info->init = amlInitAmpeg;
    info->writeheader = NULL;
    info->add_startcode = NULL;
    return info;

}

gint amlInitAc3(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{ 
    pcodec->audio_type = AFORMAT_AC3;
    pcodec->audio_info.valid = 1;
    amlAudioInfoInit(info,pcodec,structure);
    return 0;  
}

AmlStreamInfo * newAmlAinfoAc3()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoAc3));
    info->init = amlInitAc3;
    info->writeheader = NULL;
    info->add_startcode = NULL;
    return info;
}

gint amlInitEac3(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{ 
    pcodec->audio_type = AFORMAT_EAC3;
    pcodec->audio_info.valid = 1;
    amlAudioInfoInit(info,pcodec,structure);
    return 0;  
}

AmlStreamInfo * newAmlAinfoEac3()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoEac3));
    info->init = amlInitEac3;
    info->writeheader = NULL;
    info->add_startcode = NULL;
    return info;
}

gint amlInitAdpcm(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	AmlAudioInfo *audio = AML_AUDIOINFO_BASE(info);
	const gchar *getlayout;
	gint getblock_align;
	GST_WARNING("[%s:%d] ", __FUNCTION__, __LINE__);
	getlayout = gst_structure_get_string(structure, "layout");
	gst_structure_get_int(structure, "block_align", &getblock_align);
	pcodec->audio_type = AFORMAT_ADPCM;
	if (g_str_equal(getlayout, "microsoft"))
		pcodec->audio_info.codec_id = CODEC_ID_ADPCM_MS;
	pcodec->audio_info.block_align = getblock_align;
	amlAudioInfoInit(info, pcodec, structure);
	pcodec->audio_info.valid = 1;

	GST_WARNING("[%s:%d] layout%s align%d ", __FUNCTION__, __LINE__, getlayout, getblock_align);
	return 0;
}

AmlStreamInfo * newAmlAinfoAdpcm()
{
	AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoAdpcm));
	info->init = amlInitAdpcm;
	info->writeheader = NULL;
	info->add_startcode = NULL;
	return info;
}

gint amlInitFlac(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_FLAC;
	pcodec->audio_info.valid = 1;
	 aml_dump_structure(structure);
	return 0;
}

AmlStreamInfo * newAmlAinfoFlac()
{
	AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoFlac));
	info->init = amlInitFlac;
	info->writeheader = NULL;
	info->add_startcode = NULL;
	return info;
}

gint vorbis_writeheader(AmlStreamInfo* info, codec_para_t *pcodec)
{
	GstMapInfo map;
    if(info->configdata){
    	gst_buffer_map(info->configdata, &map, GST_MAP_READ);
        pcodec->audio_info.extradata_size = map.size;
	   GST_WARNING("size=%d\n",pcodec->audio_info.extradata_size);
        if (pcodec->audio_info.extradata_size > 0) {
            if (pcodec->audio_info.extradata_size >  AUDIO_EXTRA_DATA_SIZE) {
                GST_WARNING("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
                pcodec->audio_info.extradata_size =  AUDIO_EXTRA_DATA_SIZE;
            }
		 pcodec->audio_info.extradata[0] = 0x02;
            memcpy((char*)pcodec->audio_info.extradata+3, map.data, pcodec->audio_info.extradata_size);
        }
        gst_buffer_unmap(info->configdata, &map);
    }
	pcodec->audio_info.extradata_size= pcodec->audio_info.extradata_size +3;
    pcodec->audio_info.valid = 1;
    return 0;	
}
gint vorbis_startcode(AmlStreamInfo* info, codec_para_t *pcodec, GstBuffer *buffer)
{
	gint32 buf_size;
    char head[] = "HEAD";

    codec_write(pcodec, head, 4);
    buf_size = gst_buffer_get_size(buffer);
    codec_write(pcodec, &buf_size, 4);

    return 0;

}
gint amlInitVorbis(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_VORBIS;
	pcodec->audio_info.codec_id = CODEC_ID_VORBIS;
	amlAudioInfoInit(info, pcodec, structure);
	return 0;
}

AmlStreamInfo * newAmlAinfoVorbis()
{
	AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoVorbis));
	info->init = amlInitVorbis;
	info->writeheader = vorbis_writeheader;
	info->add_startcode = vorbis_startcode;
	return info;
}

gint amlInitMulaw(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_ADPCM;                    //mulaw & alaw use adpcm now
	amlAudioInfoInit(info, pcodec, structure);
	pcodec->audio_info.valid = 1;
	pcodec->audio_info.codec_id = CODEC_ID_PCM_MULAW;
	return 0;
}

AmlStreamInfo * newAmlAinfoMulaw()
{
	AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoMulaw));
	info->init = amlInitMulaw;
	info->writeheader = NULL;
	info->add_startcode = NULL;
	return info;
}

gint wma_writeheader(AmlStreamInfo* info, codec_para_t *pcodec)
{
	GstMapInfo map;
    if(info->configdata){
    	gst_buffer_map(info->configdata, &map, GST_MAP_READ);
        pcodec->audio_info.extradata_size = map.size;
        if (pcodec->audio_info.extradata_size > 0) {
            if (pcodec->audio_info.extradata_size >  AUDIO_EXTRA_DATA_SIZE) {
                GST_WARNING("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
                pcodec->audio_info.extradata_size =  AUDIO_EXTRA_DATA_SIZE;
            }
            memcpy((char*)pcodec->audio_info.extradata, map.data, pcodec->audio_info.extradata_size);
        }
        gst_buffer_unmap(info->configdata, &map);
    }
    pcodec->audio_info.valid = 1;
    return 0;	
}


gint cook_writeheader(AmlStreamInfo* info, codec_para_t *pcodec)
{
	GstMapInfo map;
    if(info->configdata){
    	gst_buffer_map(info->configdata, &map, GST_MAP_READ);
        pcodec->audio_info.extradata_size = map.size;
        if (pcodec->audio_info.extradata_size > 0) {
            if (pcodec->audio_info.extradata_size >  AUDIO_EXTRA_DATA_SIZE) {
                GST_WARNING("[%s:%d],extra data size exceed max  extra data buffer,cut it to max buffer size ", __FUNCTION__, __LINE__);
                pcodec->audio_info.extradata_size =  AUDIO_EXTRA_DATA_SIZE;
            }
            memcpy((char*)pcodec->audio_info.extradata, map.data, pcodec->audio_info.extradata_size);
        }
        gst_buffer_unmap(info->configdata, &map);
    }
    pcodec->audio_info.valid = 1;
    return 0;	
}
gint amlInitWma(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{ 
    AmlAinfoWma *audio = (AmlAinfoWma*) info;
    gst_structure_get_int (structure, "wmaversion", &audio->version);
    gst_structure_get_int (structure, "block_align", &audio->block_align);
    gst_structure_get_int (structure, "bitrate", &audio->bitrate);
    if(audio->bitrate){
        pcodec->audio_info.bitrate = audio->bitrate;
    }
    pcodec->audio_info.block_align = audio->block_align;
    switch (audio->version) {
        case 1:
            pcodec->audio_info.codec_id  = CODEC_ID_WMAV1;
            pcodec->audio_type = AFORMAT_WMA;  
            break;
        case 2:
            pcodec->audio_info.codec_id  = CODEC_ID_WMAV2;
            pcodec->audio_type = AFORMAT_WMA;  
            break;
        case 3:
            pcodec->audio_info.codec_id  = CODEC_ID_WMAPRO;
            pcodec->audio_type = AFORMAT_WMAPRO;
            break;
        default:
            break;
    }	
    amlAudioInfoInit(info,pcodec,structure);
    return 0;  
}

AmlStreamInfo * newAmlAinfoWma()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoWma));
    info->init = amlInitWma;
    info->writeheader = wma_writeheader;
    info->add_startcode = NULL;
    return info;
}

gint amlInitPcm(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{ 
    AmlAudioInfo *audio = AML_AUDIOINFO_BASE(info);
	gchar * format;
	gint  channels,rate;
	format = gst_structure_get_string(structure, "format");
	if (!strcmp(format, "S16LE")) {
		pcodec->audio_type = AFORMAT_PCM_S16LE;
		pcodec->audio_info.codec_id = CODEC_ID_PCM_S16LE;
	} else {
		GST_ERROR("%s", format);
	}

    pcodec->audio_info.valid = 1;
    aml_dump_structure(structure);
    amlAudioInfoInit(info,pcodec,structure);
    return 0;  
}
AmlStreamInfo * newAmlAinfoPcm()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoPcm));
    info->init = amlInitPcm;
    info->writeheader = NULL;
    info->add_startcode = NULL;
    return info;
}

gint amlInitLPcm(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
    AmlAudioInfo *audio = AML_AUDIOINFO_BASE(info);
    gint endianness,depth;
    gboolean getsigned;
    gst_structure_get_int (structure, "endianness", &endianness);
    gst_structure_get_int (structure, "depth", &depth);
    gst_structure_get_boolean (structure, "signed", &getsigned);
    GST_WARNING("[%s:%d],heredepth=%d",__FUNCTION__, __LINE__,depth);
    amlAudioInfoInit(info,pcodec,structure);
    pcodec->audio_info.valid = 1;
    return 0;
}

AmlStreamInfo * newAmlAinfoLPcm()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoPcm));
    info->init = amlInitPcm;
    info->writeheader = NULL;
    info->add_startcode = NULL;
    return info;
}

gint amlInitApe(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_APE;	
	pcodec->audio_info.codec_id = CODEC_ID_APE;
	pcodec->audio_info.valid = 1;

	GST_WARNING("[%s:%d]", __FUNCTION__, __LINE__);

  return 0; 
}

AmlStreamInfo * newAmlAinfoApe()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoApe));
    info->init = amlInitApe;
    info->writeheader = NULL;
    info->add_startcode = NULL;
		
    return info;
}

gint amlInitDTS(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_DTS;	
	pcodec->audio_info.codec_id = CODEC_ID_DTS;
	//pcodec->audio_info.valid = 1;
	amlAudioInfoInit(info, pcodec, structure);
	GST_WARNING("[%s:%d]", __FUNCTION__, __LINE__);

  return 0; 
}
AmlStreamInfo * newAmlAinfoDts()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoDts));
    info->init = amlInitDTS;
    info->writeheader = NULL;
    info->add_startcode = NULL;
		
    return info;
}

gint amlInitCook(AmlStreamInfo* info, codec_para_t *pcodec, GstStructure  *structure)
{
	pcodec->audio_type = AFORMAT_COOK;	
	pcodec->audio_info.codec_id = CODEC_ID_COOK;
	//pcodec->audio_info.valid = 1;
	amlAudioInfoInit(info, pcodec, structure);
	GST_WARNING("[%s:%d]", __FUNCTION__, __LINE__);

  return 0; 
}
AmlStreamInfo * newAmlAinfoCook()
{
    AmlStreamInfo *info = createAudioInfo(sizeof(AmlAinfoCook));
    info->init = amlInitCook;
    info->writeheader = cook_writeheader;
    info->add_startcode = NULL;
		
    return info;
}

static const AmlStreamInfoPool amlAstreamInfoPool[] = {
	
    {"audio/mpeg", newAmlAinfoMpeg},
    {"audio/x-ac3", newAmlAinfoAc3},
    {"audio/x-eac3", newAmlAinfoEac3},
    {"audio/x-adpcm", newAmlAinfoAdpcm},
    {"audio/x-flac", newAmlAinfoFlac},
    {"audio/x-wma", newAmlAinfoWma},
    {"audio/x-vorbis", newAmlAinfoVorbis},
    {"audio/x-mulaw", newAmlAinfoMulaw},
    {"audio/x-lpcm", newAmlAinfoLPcm},
    {"audio/x-raw-int", newAmlAinfoPcm},
    {"audio/x-raw", newAmlAinfoPcm},
    {"application/x-ape", newAmlAinfoApe},
    {"audio/x-private1-dts", newAmlAinfoDts},
    {"audio/x-dts", newAmlAinfoDts},
    {"audio/x-pn-realaudio", newAmlAinfoCook},
    {NULL, NULL}
};
AmlStreamInfo *amlAstreamInfoInterface(gchar *format)
{
    AmlStreamInfoPool *p  = amlAstreamInfoPool; 
    AmlStreamInfo *info = NULL;
    info =amlStreamInfoInterface(format,p);
    return info;
}
