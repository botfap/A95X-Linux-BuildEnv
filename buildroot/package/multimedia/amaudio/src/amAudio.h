// Callback prototype
typedef void (*newDataCallback)(unsigned char *buf, unsigned len);

// Audio capture API
extern int amAudio_Init(newDataCallback callback);
extern int amAudio_Finish(void);
