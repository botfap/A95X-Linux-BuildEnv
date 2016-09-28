#ifndef _PLAYER_PARA_H_
#define _PLAYER_PARA_H_

#include <libavformat/avformat.h>
#include <stream_format.h>
#define ASTREAM_MAX_NUM 20
struct play_para;

//#define DEBUG_VARIABLE_DUR

typedef enum {
    STREAM_UNKNOWN = 0,
    STREAM_TS,
    STREAM_PS,
    STREAM_ES,
    STREAM_RM,
    STREAM_AUDIO,
    STREAM_VIDEO,
} pstream_type;


typedef struct {
    int             has_video;
    vformat_t       video_format;
    signed short    video_index;
    unsigned short  video_pid;
    unsigned int    video_width;
    unsigned int    video_height;
    unsigned int    video_ratio;
	uint64_t        video_ratio64;
    int             check_first_pts;
    int             flv_flag;
    int             h263_decodable;
	int				discard_pkt;
    int64_t         start_time;
    float           video_duration;
    float           video_pts;
    unsigned int    video_rate;
    unsigned int    video_rotation_degree;
    unsigned int    video_codec_rate;
    vdec_type_t     video_codec_type;
    int             extradata_size;
    uint8_t         *extradata;
    int             codec_id;
} v_stream_info_t;

typedef struct {
    int             has_audio;
    int             resume_audio;
    aformat_t       audio_format;
    signed short    audio_index;
    signed short    audio_index_tab[ASTREAM_MAX_NUM]; // some audio stream is unvalid, convert the stream.index to display num.
    unsigned short  audio_pid;
    int             audio_channel;
    int             audio_samplerate;
    int             check_first_pts;
    int64_t         start_time;
    float           audio_duration;
    int             extradata_size;
    uint8_t         *extradata;
} a_stream_info_t;

typedef struct {
    int             has_sub;
    signed short    sub_index;
    unsigned short  sub_pid;
    unsigned int    sub_type;
    int64_t         start_time;
    float           sub_duration;
    float           sub_pts;
    int             check_first_pts;
    int             cur_subindex; //for change subtitle
    int             sub_has_found;
    int             sub_stream;
    char            *sub_buf[9];
} s_stream_info_t;

typedef  struct {
    unsigned int search_flag;
    unsigned int read_end_flag;
    unsigned int video_end_flag;
    unsigned int video_low_buffer;
    unsigned int audio_end_flag;
    unsigned int audio_low_buffer;
    unsigned int end_flag;
    unsigned int request_end_flag;//stop request by user.	
    unsigned int pts_valid;
    unsigned int sync_flag;
    unsigned int reset_flag;
    unsigned int no_audio_flag;
    unsigned int no_video_flag;
    unsigned int has_sub_flag;
    unsigned int loop_flag;
    unsigned int black_out;
    unsigned int raw_mode;
    unsigned int pause_flag;
    unsigned int fast_forward;
    unsigned int fast_backward;
    unsigned int init_ff_fr;
    unsigned int seek_base_audio;
    unsigned int audio_mute;   
	unsigned int avsync_enable;
	#ifdef DEBUG_VARIABLE_DUR
	unsigned int info_variable;
	#endif
    unsigned int audio_switch_vmatch;
    unsigned int audio_switch_smatch;
    unsigned int switch_audio_id;
    unsigned int switch_sub_id;
    unsigned int is_playlist;	
    unsigned int lowbuffermode_flag;
    unsigned int ignore_ffmpeg_errors;
    unsigned int temp_interrupt_ffmpeg;	
    float time_point;
    int f_step;
    int read_max_retry_cnt;
	int audio_ready;		
    int check_lowlevel_eagain_cnt;
    int check_audio_ready_ms;
    int last_seek_time_point;
    int64_t last_seek_offset;
    int seek_offset_same;
	int seek_frame_fail;
    long avsync_check_old_time;
    long vbuf_rpchanged_Old_time;
    long avdiff_check_old_time;
    int avdiff_next_reset_timepoint;
    int pts_discontinue_check_time;

	int buf_limited_time_ms;/*low buffering mode,if data> ms,we do wait write.*/
	int reset_drop_buffered_data;/*droped buffered data.*/

    int iponly_flag;
    int freerun_mode;
    int no_dec_ref_buf;
    int vsync_upint;
    int no_error_recovery;
} p_ctrl_info_t;

int player_dec_init(struct play_para *p_para);
int player_decoder_init(struct play_para *p_para);
void player_para_reset(struct play_para *para);
int player_dec_reset(struct play_para *p_para);
void player_clear_ctrl_flags(p_ctrl_info_t *cflag);
int player_offset_init(struct play_para *p_para);
#endif
