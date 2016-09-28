#ifndef PLAYER_SUB_H
#define PLAYER_SUB_H

#define SUBAPI
#define SUB_MAX_TEXT                10

/* subtitle formats */
#define SUB_INVALID     -1
#define SUB_MICRODVD    0
#define SUB_SUBRIP      1
#define SUB_SUBVIEWER   2
#define SUB_SAMI        3
#define SUB_VPLAYER     4
#define SUB_RT          5
#define SUB_SSA         6
#define SUB_PJS         7
#define SUB_MPSUB       8
#define SUB_AQTITLE     9
#define SUB_SUBVIEWER2  10
#define SUB_SUBRIP09    11
#define SUB_JACOSUB     12
#define SUB_MPL2        13
#define SUB_DIVX        14

/* Maximal length of line of a subtitle */
#define LINE_LEN                    1000

typedef enum {
    SUB_ALIGNMENT_BOTTOMLEFT = 1,
    SUB_ALIGNMENT_BOTTOMCENTER,
    SUB_ALIGNMENT_BOTTOMRIGHT,
    SUB_ALIGNMENT_MIDDLELEFT,
    SUB_ALIGNMENT_MIDDLECENTER,
    SUB_ALIGNMENT_MIDDLERIGHT,
    SUB_ALIGNMENT_TOPLEFT,
    SUB_ALIGNMENT_TOPCENTER,
    SUB_ALIGNMENT_TOPRIGHT
} sub_alignment_t;

/**
 * Subtitle struct unit
 */
typedef struct {
    /// number of subtitle lines
    int lines;

    /// subtitle strings
    char *text[SUB_MAX_TEXT];

    /// alignment of subtitles
    sub_alignment_t alignment;
} subtext_t;

struct subdata_s {
    list_t  list;            /* head node of subtitle_t list */
    list_t  list_temp;

    int     sub_num;
    int     sub_error;
    int     sub_format;
};

struct subtitle_s {
    list_t      list;         /* linked list */
    long int    start;        /* start time */
    long int    end;          /* end time */
    subtext_t   text;         /* subtitle text */
    unsigned char *subdata;   /* data for divx bmp subtitle*/
} ;

typedef struct subtitle_s subtitle_t;
typedef struct subdata_s subdata_t;

typedef struct {
    subtitle_t *(*read)(int fd, subtitle_t *dest);
    void (*post)(subtitle_t *dest);
    const char *name;
} subreader_t;

typedef struct _DivXSubPictColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} DivXSubPictColor;

typedef struct _DivXSubPictHdr
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
    unsigned char *rleData;
} DivXSubPictHdr;

typedef struct _DivXSubPictHdr_HD
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
	unsigned char background_transparency;	//HD profile only
	unsigned char pattern1_transparency;	//HD profile only
	unsigned char pattern2_transparency;	//HD profile only
	unsigned char pattern3_transparency;	//HD profile only
    unsigned char *rleData;
} DivXSubPictHdr_HD;

#define ERR ((void *) -1)
#define sub_ms2pts(x) ((x) * 900)

SUBAPI extern void internal_sub_close(subdata_t *subdata);
SUBAPI extern subdata_t *internal_sub_open(char *filename, unsigned rate);
SUBAPI extern char *internal_sub_filenames(char *filename, unsigned perfect_match);
SUBAPI extern subtitle_t *internal_sub_search(subdata_t *subdata, subtitle_t *ref, int pts);
SUBAPI extern int internal_sub_get_starttime(subtitle_t *subt);
SUBAPI extern int internal_sub_get_endtime(subtitle_t *subt);
SUBAPI extern subtext_t *internal_sub_get_text(subtitle_t *subt);
SUBAPI extern subtitle_t *internal_divx_sub_add(subdata_t *subdata, unsigned char *data);
SUBAPI extern void internal_divx_sub_delete(subdata_t *subdata, int pts);
SUBAPI extern void internal_divx_sub_flush(subdata_t *subdata);

/*The struct use fot notify the ui layer the audio information of the clip */
typedef struct _CLIP_AUDIO_INFO {
    unsigned int audio_info; /* audio codec info */
    unsigned int audio_cur_play_index;   /* audio play current index */
    unsigned int total_audio_stream; /* total audio stream number */
} Clip_Audio_Info;

/*@}*/
#endif /* PLAYER_SUB_H */
