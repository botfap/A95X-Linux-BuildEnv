#ifndef HLSPROTO_HEADER___
#define HLSPROTO_HEADER___

#define EXTM3U						"#EXTM3U"
#define EXTINF						"#EXTINF"
#define EXT_X_TARGETDURATION		"#EXT-X-TARGETDURATION"

#define EXT_X_MEDIA_SEQUENCE		"#EXT-X-MEDIA-SEQUENCE"
#define EXT_X_KEY					"#EXT-X-KEY"
#define EXT_X_PROGRAM_DATE_TIME	"#EXT-X-PROGRAM-DATE-TIME"
#define EXT_X_ALLOW_CACHE			"#EXT-X-ALLOW-CACHE"
#define EXT_X_ENDLIST				"#EXT-X-ENDLIST"
#define EXT_X_STREAM_INF			"#EXT-X-STREAM-INF"

#define EXT_X_DISCONTINUITY		"#EXT-X-DISCONTINUITY"

int  hlsproto_probe(ByteIOContext *s,const char *file);

#endif