#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "amAudio.h"

#define LOG_TAG "amAudio_Test"

#undef LOGI
#define LOGI(fmt, ...) printf("I/%s: ", LOG_TAG);printf(fmt, ##__VA_ARGS__);
#undef LOGE
#define LOGE(fmt, ...) printf("E/%s: ", LOG_TAG);printf(fmt, ##__VA_ARGS__);
#undef LOGD
#define LOGD(fmt, ...) printf("D/%s: ", LOG_TAG);printf(fmt, ##__VA_ARGS__);

//#define WRITE_AUDIO_OUT_TO_FILE
#define SELF_TEST

#define AUDIO_IN									"/dev/amaudio_in"
#define AUDIO_OUT									"/dev/amaudio_out"

#define AMAUDIO_IOC_MAGIC							'A'
#define AMAUDIO_IOC_GET_I2S_OUT_SIZE				_IOW(AMAUDIO_IOC_MAGIC, 0x00, int)
#define AMAUDIO_IOC_GET_I2S_OUT_PTR					_IOW(AMAUDIO_IOC_MAGIC, 0x01, int)
#define AMAUDIO_IOC_SET_I2S_OUT_RD_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x02, int)
#define AMAUDIO_IOC_GET_I2S_IN_SIZE					_IOW(AMAUDIO_IOC_MAGIC, 0x03, int)
#define AMAUDIO_IOC_GET_I2S_IN_PTR					_IOW(AMAUDIO_IOC_MAGIC, 0x04, int)
#define AMAUDIO_IOC_SET_I2S_IN_RD_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x05, int)
#define AMAUDIO_IOC_SET_I2S_IN_MODE					_IOW(AMAUDIO_IOC_MAGIC, 0x06, int)
#define AMAUDIO_IOC_SET_I2S_OUT_MODE				_IOW(AMAUDIO_IOC_MAGIC, 0x07, int)
#define AMAUDIO_IOC_GET_I2S_IN_RD_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x08, int)
#define AMAUDIO_IOC_GET_I2S_OUT_RD_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x09, int)
#define AMAUDIO_IOC_SET_I2S_IN_WR_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x0a, int)
#define AMAUDIO_IOC_SET_I2S_OUT_WR_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x0b, int)
#define AMAUDIO_IOC_GET_I2S_IN_WR_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x0c, int)
#define AMAUDIO_IOC_GET_I2S_OUT_WR_PTR				_IOW(AMAUDIO_IOC_MAGIC, 0x0d, int)
#define AMAUDIO_IOC_SET_LEFT_MONO					_IOW(AMAUDIO_IOC_MAGIC, 0x0e, int)
#define AMAUDIO_IOC_SET_RIGHT_MONO					_IOW(AMAUDIO_IOC_MAGIC, 0x0f, int)
#define AMAUDIO_IOC_SET_STEREO						_IOW(AMAUDIO_IOC_MAGIC, 0x10, int)
#define AMAUDIO_IOC_SET_CHANNEL_SWAP				_IOW(AMAUDIO_IOC_MAGIC, 0x11, int)
#define AMAUDIO_IOC_DIRECT_AUDIO					_IOW(AMAUDIO_IOC_MAGIC, 0x12, int)
#define AMAUDIO_IOC_DIRECT_LEFT_GAIN  				_IOW(AMAUDIO_IOC_MAGIC, 0x13, int)
#define AMAUDIO_IOC_DIRECT_RIGHT_GAIN 				_IOW(AMAUDIO_IOC_MAGIC, 0x14, int)

#define M_AUDIO_THREAD_DELAY_TIME					4000
#define AUDIO_BUFFER_SIZE							19200

static int amAudio_InHandle = -1;
static int amAudio_OutHandle = -1;
static int amAudio_InSize = -1;
static int amAudio_OutSize = -1;
static unsigned char *amAudio_In_BufAddr = NULL;
static unsigned char *amAudio_Out_BufAddr = NULL;
unsigned char *inbuffer_current_position = NULL;
unsigned char *outbuffer_current_position = NULL;

unsigned char *amAudio_In_BufAddr_half = NULL;
unsigned char *amAudio_Out_BufAddr_half = NULL;

static pthread_t amAudio_ThreadID = 0;
static int amAudio_ThreadExitFlag = 0;

#define NODATA  0
#define BUFFER1_READY 1
#define BUFFER2_READY 2

typedef struct Buffer_status {
	int status;
	unsigned char *user_read;
} Buffer_status;

Buffer_status Output_buffer_status;

#ifdef WRITE_AUDIO_OUT_TO_FILE
static FILE *FileIn = NULL;
static FILE *FileOut = NULL;

#endif

static int amAudio_InInit(void)
{
	unsigned int hwptr;
	
	hwptr = ioctl (amAudio_InHandle, AMAUDIO_IOC_GET_I2S_IN_PTR);
	if (hwptr == 0xFFFFFFFF)
	{
		LOGI("amAudio_InInit Error!\n");
		return -1;
	}
    ioctl (amAudio_InHandle, AMAUDIO_IOC_SET_I2S_IN_RD_PTR, hwptr);
	inbuffer_current_position = amAudio_In_BufAddr;
    return 0;
}

static int amAudio_OutInit(void)
{
	unsigned int hwptr;

	hwptr= ioctl (amAudio_OutHandle, AMAUDIO_IOC_GET_I2S_OUT_PTR);
	if (hwptr == 0xFFFFFFFF)
	{
		LOGI("amAudio_OutInit Error!\n");
		return -1;
	}
	ioctl (amAudio_OutHandle, AMAUDIO_IOC_SET_I2S_OUT_RD_PTR, hwptr);
	outbuffer_current_position = amAudio_Out_BufAddr;
	return 0;
}

static int amAudio_InProc(void)
{
	int Ret;
	int hwptr;
	int rdptr;
	int Bytes;
	int PosBytes;
	int left_size = AUDIO_BUFFER_SIZE - (inbuffer_current_position - amAudio_In_BufAddr);

	if(amAudio_InHandle == -1){
		LOGE("amAudio_InHandle error: %d\n", -1);
	}
		
	hwptr = ioctl (amAudio_InHandle, AMAUDIO_IOC_GET_I2S_IN_PTR);
	if (hwptr == 0xFFFFFFFF)
		return -1;
	
	rdptr = ioctl (amAudio_InHandle, AMAUDIO_IOC_GET_I2S_IN_RD_PTR);
	if (rdptr == 0xFFFFFFFF)
		return -1;

	Bytes = (hwptr - rdptr + amAudio_InSize) % amAudio_InSize;
	Bytes = Bytes >> 2 << 2;

	//LOGD("amAudio in hwptr: %x, rdptr: %x, Bytes: %x, left_size = %x\n", hwptr, rdptr, Bytes,left_size);
	
	if (Bytes == 0)
		return 0;

	if(hwptr > rdptr){
		PosBytes = Bytes;
		if(left_size >= PosBytes){
			Ret = read (amAudio_InHandle, inbuffer_current_position, PosBytes);
			if(Ret != PosBytes){
				LOGE("amAudio_in fail to read! Ret = %d, PosBytes = %d \n",Ret, PosBytes);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, PosBytes, FileIn);
#endif
			inbuffer_current_position += PosBytes;
			if(left_size == PosBytes){
				inbuffer_current_position = amAudio_In_BufAddr;
			}
		}else{
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr;
			left_size = PosBytes - left_size;
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr + left_size;
		}
	}else{
	
		PosBytes = amAudio_InSize - rdptr;
		
		if(left_size >= PosBytes){
			Ret = read (amAudio_InHandle, inbuffer_current_position, PosBytes);
			if(Ret != PosBytes){
				LOGE("amAudio_in fail to read! Ret = %d, PosBytes = %d \n",Ret, PosBytes);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, PosBytes, FileIn);
#endif
			inbuffer_current_position += PosBytes;
			if(left_size == PosBytes)
				inbuffer_current_position = amAudio_In_BufAddr;
		}else{
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr;
			left_size = PosBytes - left_size;
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr + left_size;
		}

		PosBytes = Bytes - PosBytes;
		
		left_size = AUDIO_BUFFER_SIZE - (inbuffer_current_position - amAudio_In_BufAddr);
		
		if(left_size >= PosBytes){
			Ret = read (amAudio_InHandle, inbuffer_current_position, PosBytes);
			if(Ret != PosBytes){
				LOGE("amAudio_in fail to read! Ret = %d, PosBytes = %d \n",Ret, PosBytes);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, PosBytes, FileIn);
#endif
			inbuffer_current_position += PosBytes;
			if(left_size == PosBytes)
				inbuffer_current_position = amAudio_In_BufAddr;
		}else{
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr;
			left_size = PosBytes - left_size;
			Ret = read (amAudio_InHandle, inbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_in fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(inbuffer_current_position, 1, left_size, FileIn);
#endif
			inbuffer_current_position = amAudio_In_BufAddr + left_size;
		}
		
	}
	return 0;
}

static int amAudio_OutProc(void)
{
	int hwptr;
	int rdptr;
	int Bytes;
	int Ret;
	int PosBytes;
	int left_size = AUDIO_BUFFER_SIZE - (outbuffer_current_position - amAudio_Out_BufAddr);
	
	hwptr = ioctl (amAudio_OutHandle, AMAUDIO_IOC_GET_I2S_OUT_PTR);
	if (hwptr == 0xFFFFFFFF)
		return -1;
	
	rdptr = ioctl (amAudio_OutHandle, AMAUDIO_IOC_GET_I2S_OUT_RD_PTR);
	if (rdptr == 0xFFFFFFFF)
		return -1;
	
	Bytes = (hwptr - rdptr + amAudio_OutSize) % amAudio_OutSize;
	Bytes = Bytes >> 2 << 2;

	//LOGD("amAudio out hwptr: %x, rdptr: %x, Bytes: %x, left_size = %x, status = %d\n", hwptr, rdptr, Bytes,left_size,(int)Output_buffer_status.status);
	
	if (Bytes == 0)
		return 0;

	if(hwptr > rdptr)
		PosBytes = Bytes;
	else
		PosBytes = amAudio_OutSize - rdptr;
	{			
		if(left_size >= PosBytes){
			Ret = read (amAudio_OutHandle, outbuffer_current_position, PosBytes);
			if(Ret != PosBytes){
				LOGE("amAudio_out fail to read! Ret = %d, PosBytes = %d \n",Ret, PosBytes);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(outbuffer_current_position, 1, PosBytes, FileOut);
#endif
			outbuffer_current_position += PosBytes;

			if (outbuffer_current_position - PosBytes < amAudio_Out_BufAddr_half){
				if (outbuffer_current_position >= amAudio_Out_BufAddr_half)
					Output_buffer_status.status = BUFFER1_READY;
				Output_buffer_status.user_read = amAudio_Out_BufAddr;
			}
			else{
				Output_buffer_status.user_read = amAudio_Out_BufAddr_half;
			}
			if(left_size == PosBytes){
				outbuffer_current_position = amAudio_Out_BufAddr;
				Output_buffer_status.status |= BUFFER2_READY;
			}
		}else{
			Ret = read (amAudio_OutHandle, outbuffer_current_position, left_size);
			if(Ret != left_size){
				LOGE("amAudio_out fail to read! Ret = %d, left_size = %d \n",Ret, left_size);
				return -1;
			}
#ifdef WRITE_AUDIO_OUT_TO_FILE
			fwrite(outbuffer_current_position, 1, left_size, FileOut);
#endif
                        if (outbuffer_current_position < amAudio_Out_BufAddr_half){
                                Output_buffer_status.status = BUFFER1_READY|BUFFER2_READY;
				Output_buffer_status.user_read = amAudio_Out_BufAddr;
			}
			else{ 
                                Output_buffer_status.status = BUFFER2_READY;
				Output_buffer_status.user_read = amAudio_Out_BufAddr_half;
			}
			outbuffer_current_position = amAudio_Out_BufAddr;
		}
	}
	return 0;
}

static void *amAudio_Thread(void *data)
{
	int Ret = 0;
	int Status = 0;
	newDataCallback callback = (newDataCallback)data;
	while (1)
	{
		if (amAudio_ThreadExitFlag == 1)
		{
			amAudio_ThreadExitFlag = 0;
			break;
		}

		switch (Status)
		{
			case 0:
				//Ret = amAudio_InInit();
				if (Ret == 0)
				{
					Status = 1;
				}
				break;
			case 1:
				Ret = amAudio_OutInit();
				if (Ret == 0)
				{
					Status = 2;
				}
				break;
			case 2:
				//Ret = amAudio_InProc();
				if (Ret == 0)
				{
					Status = 3;
				}else{
					Status = 0;
				}
				break;
			case 3:
				Ret = amAudio_OutProc();
				if (Ret == 0)
				{
					Status = 2;
				}else{
					Status = 1;
				}
				if ((Output_buffer_status.status & BUFFER1_READY)
					||(Output_buffer_status.status & BUFFER2_READY)){
					if ((Output_buffer_status.status & BUFFER1_READY)
	                                	&&(Output_buffer_status.status & BUFFER2_READY))
						(*callback)(Output_buffer_status.user_read, AUDIO_BUFFER_SIZE);
					else
						(*callback)(Output_buffer_status.user_read, AUDIO_BUFFER_SIZE/2);
					Output_buffer_status.status = NODATA;
				}
				break;
			if(Status == 0 && Status == 1)
				LOGE("error: new reset!\n");
		}

		usleep (M_AUDIO_THREAD_DELAY_TIME);
	}

	pthread_exit (0);
	
	return ((void*)0);
}

static int amAudio_AmlogicInit (newDataCallback callback)
{
	pthread_attr_t		attr;
	struct sched_param	param;
	int					Ret;
	
	/*amAudio_InHandle = open (AUDIO_IN, O_RDWR);
	if (amAudio_InHandle < 0)
	{
		LOGE("The device amaudio_in cant't be opened!\n");
		return -1;
	}*/

	amAudio_OutHandle = -1;
	while (amAudio_OutHandle < 0){
		amAudio_OutHandle = open (AUDIO_OUT, O_RDWR);
		if (amAudio_OutHandle < 0){
			LOGE("The device amaudio_out cant't be opened!\n");
			sleep(5);
		}
		else{
			break;
		}
		if (amAudio_ThreadExitFlag == 1){
			amAudio_ThreadExitFlag = 0;
			return -1;
		}	
	}

	//ioctl (amAudio_InHandle, AMAUDIO_IOC_SET_I2S_IN_MODE, 1);
	ioctl (amAudio_OutHandle, AMAUDIO_IOC_SET_I2S_OUT_MODE, 1);

	//ioctl (amAudio_InHandle, AMAUDIO_IOC_DIRECT_LEFT_GAIN, 0);
	//ioctl (amAudio_InHandle, AMAUDIO_IOC_DIRECT_RIGHT_GAIN, 0);

	/*amAudio_InSize = ioctl (amAudio_InHandle, AMAUDIO_IOC_GET_I2S_IN_SIZE, 0);
	LOGI("amAudio_InSize = 0x%x !\n",amAudio_InSize);
	if (amAudio_InSize == -1)
	{
		close (amAudio_OutHandle);
		amAudio_OutHandle = -1;
		close (amAudio_InHandle);
		amAudio_InHandle = -1;
		return -1;
	}*/
	
	amAudio_OutSize = ioctl (amAudio_OutHandle, AMAUDIO_IOC_GET_I2S_OUT_SIZE, 0);
	LOGI("amAudio_OutSize = 0x%x !\n",amAudio_OutSize);
	if (amAudio_OutSize == -1)
	{
		close (amAudio_OutHandle);
		amAudio_OutHandle = -1;
		//close (amAudio_InHandle);
		//amAudio_InHandle = -1;
		return -1;
	}
	
	amAudio_In_BufAddr = (unsigned char *)malloc (AUDIO_BUFFER_SIZE);
	amAudio_Out_BufAddr = (unsigned char *)malloc (AUDIO_BUFFER_SIZE);
	
	/*if (amAudio_In_BufAddr == NULL || amAudio_Out_BufAddr == NULL)
	{
		LOGE("Fail to malloc amaudio buffer!\n");
		close (amAudio_OutHandle);
		amAudio_OutHandle = -1;
		close (amAudio_InHandle);
		amAudio_InHandle = -1;
		return -1;
	}*/
	
	amAudio_In_BufAddr_half  = amAudio_In_BufAddr + AUDIO_BUFFER_SIZE/2;
	amAudio_Out_BufAddr_half = amAudio_Out_BufAddr + AUDIO_BUFFER_SIZE/2;
	Output_buffer_status.status = NODATA;
	Output_buffer_status.user_read = NULL;
	
	pthread_attr_init (&attr);
	pthread_attr_setschedpolicy (&attr, SCHED_RR);
	param.sched_priority = sched_get_priority_max (SCHED_RR);
	pthread_attr_setschedparam (&attr, &param);
	amAudio_ThreadExitFlag = 0;
	Ret = pthread_create (&amAudio_ThreadID, &attr, &amAudio_Thread, (void*)callback);
	pthread_attr_destroy (&attr);

	if (Ret != 0)
	{
		LOGE("Fail to create thread!\n");
		free(amAudio_In_BufAddr);
		amAudio_In_BufAddr = NULL;
		free(amAudio_Out_BufAddr);
		amAudio_Out_BufAddr = NULL;
		
		close (amAudio_OutHandle);
		amAudio_OutHandle = -1;
		//close (amAudio_InHandle);
		//amAudio_InHandle = -1;
		return -1;
	}

	return 0;
}

static int amAudio_AmlogicFinish (void)
{
	amAudio_ThreadExitFlag = 1;
	pthread_join(amAudio_ThreadID, NULL);
	
	free(amAudio_In_BufAddr);
	amAudio_In_BufAddr = NULL;
	
	free(amAudio_Out_BufAddr);
	amAudio_Out_BufAddr = NULL;
		
	close (amAudio_OutHandle);
	amAudio_OutHandle = -1;
	
	//close (amAudio_InHandle);
	//amAudio_InHandle = -1;

	return 0;
}

int amAudio_Init(newDataCallback callback)
{
	int Ret;

	LOGI("Audio capture start!\n");

#ifdef WRITE_AUDIO_OUT_TO_FILE
	FileIn = fopen ("/mnt/test_in.pcm", "wb");
	FileOut = fopen ("/mnt/test_out.pcm", "wb");
	if(FileOut == NULL || FileIn == NULL)
		LOGE("The file can't be opened!\n");
#endif
	
	Ret = amAudio_AmlogicInit(callback);
	if (Ret == 0){
		return 0;
	}else{
		amAudio_AmlogicFinish();
		return -1;
	}
}

int amAudio_Finish (void)
{
	amAudio_AmlogicFinish ();
	
#ifdef WRITE_AUDIO_OUT_TO_FILE
	if (FileOut != NULL)
	{
		fclose (FileOut);
		FileOut = NULL;
	}
#endif
	LOGI("Audio capture finish!\n");
	return 0;
}

#ifdef SELF_TEST
static FILE *pcm_dumpfile = NULL;
void amAudio_Callback(unsigned char *buf, unsigned len)
{
	if (pcm_dumpfile){
		LOGD("write %x %x to file\n", buf, len);
		fwrite(buf, len, 1, pcm_dumpfile);
	}
}

int main (void)
{
	pcm_dumpfile = fopen("output.pcm","wb");
	amAudio_Init(&amAudio_Callback);
	//while (1);
	sleep(10);
	amAudio_Finish();
	if (pcm_dumpfile)
		fclose(pcm_dumpfile);
	return 0;
}
#endif

