// Callback prototype
typedef void (*newDataCallback)(unsigned char *buf, unsigned len, int state_change);

#define AUDIO_CAPTURE_NEW_DATA	1
#define AUDIO_CAPTURE_NO_DATA	2

// Audio capture API
extern int amAudio_Init(newDataCallback callback);
extern int amAudio_Finish(void);
extern int amAudio_Get_Playback_Samplerate(void);
extern int test_capture(char *filename);
