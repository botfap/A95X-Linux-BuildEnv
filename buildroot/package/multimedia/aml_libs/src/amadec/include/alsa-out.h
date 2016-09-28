#ifndef ALSA_OUT_H
#define ALSA_OUT_H

#define PCM_DEVICE_DEFAULT      "default"
#define OUTPUT_BUFFER_SIZE      (8*1024)

typedef struct {
    pthread_t playback_tid;
    pthread_mutex_t playback_mutex;
    pthread_cond_t playback_cond;
    snd_pcm_t *handle;
    snd_pcm_format_t format;
    size_t bits_per_sample;
    size_t bits_per_frame;
    int buffer_size;
    unsigned int channelcount;
    unsigned int rate;
    int oversample;
    int realchanl;
    int flag;
    int stop_flag;
    int pause_flag;
    int wait_flag;
} alsa_param_t;
int dummy_alsa_control(char * id_string , long vol, int rw, long * value);
#endif
