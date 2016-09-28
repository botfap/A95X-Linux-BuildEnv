// Copyright 2013 Yahoo! -- All rights reserved.
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "yctv_media.h"
#include "Hash.h"
#include "cJSON.h"
#include "MediaSupport.h"

#include "player.h"
#include "player_type.h"
#include "streamsource.h"

static int player_pid = -1;
static play_control_t *player_ctrl = NULL;
static int player_inited = 0;
static char play_uri[512];
static pthread_t play_thread;

static char *parse_uri(char *uri)
{
	strcpy(play_uri, uri);
	return play_uri;
}

static void signal_handler(int signum)
{
        printf("player get signum=%x\n",signum);
        player_progress_exit();
        signal(signum, SIG_DFL);
        raise (signum);
}

static void *amplayer_signal_thread(void *data)
{
        while ((player_pid>=0) && (!PLAYER_THREAD_IS_STOPPED(player_get_state(player_pid)))){
                usleep(200*1000);
                signal(SIGCHLD, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
                signal(SIGTTOU, SIG_IGN);
                signal(SIGTTIN, SIG_IGN);
                signal(SIGHUP, signal_handler);
                signal(SIGTERM, signal_handler);
                signal(SIGSEGV, signal_handler);
                signal(SIGINT, signal_handler);
                signal(SIGQUIT, signal_handler);
        };
}

static play_control_t *amplayer_init(char *uri)
{
	if (!player_inited){
        	player_init();
        	streamsource_init();
		player_inited = 1;
	}

	play_control_t *pCtrl = (play_control_t*)malloc(sizeof(play_control_t));  
	memset(pCtrl, 0, sizeof(play_control_t));
	if (pCtrl){
		pCtrl->video_index = -1;
		pCtrl->audio_index = -1;
		pCtrl->sub_index = -1; 
		pCtrl->hassub = 0;
		pCtrl->t_pos = -1;	// start position, if live streaming, need set to -1
		pCtrl->need_start = 0; // if 0,you can omit player_start_play API.just play video/audio immediately. if 1,need call "player_start_play" API;
		pCtrl->loop_mode = 0;
	    if (uri){
			pCtrl->file_name = parse_uri(uri); 
			printf("player init with URI:%s\n", pCtrl->file_name);
		}
		return pCtrl;
	}
	return NULL;
}

static int amplayer_start(play_control_t *pCtrl)
{
	int pid=player_start(pCtrl, 0);
	if(pid<0)
	{
		printf("player start failed!error=%d\n",pid);
		return -1;
	}	
        pthread_create(&play_thread, NULL, amplayer_signal_thread, NULL);
        pthread_join(play_thread, NULL);
	return pid;
}

static player_status amplayer_status(int pid)
{
	return player_get_state(pid);
}

static void amplayer_play(int pid)
{
	player_start_play(pid);
}

static void amplayer_stop(int pid)
{
	player_stop(pid);
}

static void amplayer_pause(int pid)
{
	player_pause(pid);
}

static void amplayer_resume(int pid)
{
	player_resume(pid);
}

static void amplayer_timesearch(int pid, int time)
{
    player_timesearch(pid, time);
}

static void amplayer_exit(int pid)
{
	player_exit(pid);
}

// Global decoders (config_media.json specifies 2 decoders)
#define  NUM_OF_DECODERS   2
SampleDecoder *gDecoders[NUM_OF_DECODERS];
void init_decoders( const int index, const int id, const char* name )
{
	if( !gDecoders[index] && index < NUM_OF_DECODERS )
	{
		gDecoders[index] = new SampleDecoder;
	}

	gDecoders[index]->Name = std::string(name);
	gDecoders[index]->DecoderID = id;
	gDecoders[index]->GlobalMute = 0.5;
	gDecoders[index]->EnableClosedCaption = false;
	gDecoders[index]->ClosedCaptionChannel = 0;
	gDecoders[index]->EnableAnalogClosedCaption = false;
	gDecoders[index]->AnalogClosedCaptionChannel = 0;
	gDecoders[index]->PlayCount = 0;
	gDecoders[index]->BufferPercentage = -1;
}

void remove_decoders()
{
	int i;
	for(i = 0; i < NUM_OF_DECODERS; i++)
	{
		if( gDecoders[i] )
		{
			delete gDecoders[i];
		}
	}
}

// Helper functions
cJSON* get_obj( cJSON* root, const char* key )
{
	if( cJSON* v = cJSON_GetObjectItem(root, key) )
		return v;
	assert(false);
	return 0;
}

bool get_bool( cJSON* root, const char* key )
{
	if( cJSON* v = cJSON_GetObjectItem(root, key) )
		return (bool)v->valueint;
	assert( false );
	return false;
}

int json_int( cJSON* root, const char* key )
{
	if( cJSON* v = cJSON_GetObjectItem(root, key) )
		return v->valueint;
	assert( false );
	return 0;
}

float json_float( cJSON* root, const char* key )
{
	if( cJSON* v = cJSON_GetObjectItem(root, key) )
		return (float)v->valuedouble;
	assert( false );
	return 0.0f;
}

std::string json_string( cJSON* root, const char* key )
{
	if( cJSON* v = cJSON_GetObjectItem(root, key) )
		return std::string(v->valuestring);
	assert( false );
	return "";
}

SampleDecoder* get_decoder( cJSON* root )
{
	int id = json_int(root, "DecoderID");
	for( int i = 0; i < NUM_OF_DECODERS; ++i )
	{
		if( gDecoders[i]->DecoderID == id )
			return gDecoders[i];
	}
	return 0;
}

void get_display_config( SampleDisplay& display, cJSON* root )
{
	if( root )
	{
		display.SessionID = json_int(root,   "SessionID");
		display.X         = json_float(root, "X");
		display.Y         = json_float(root, "Y");
		display.Width     = json_float(root, "Width");
		display.Height    = json_float(root, "Height");
	}
}

void get_sound_config( SampleSound& sound, cJSON* root )
{
	if( root )
	{
		sound.SessionID = json_int(root,   "SessionID");
		sound.Volume    = json_float(root, "Volume");
	}
}

void get_input_config( SampleInput& in, cJSON* root )
{
	if( root )
	{
		in.Name      = json_string(root, "Name");
		in.InputID   = json_int(root,    "InputID");
		in.SessionID = json_int(root,    "SessionID");
		in.URI       = json_string(root, "URI");
		in.DecoderID = json_int(root,    "DecoderID");

		get_display_config(in.Display, get_obj(root, "Display"));
		get_sound_config(in.Sound, get_obj(root, "Sound"));
	}
}

void get_dec_config( SampleDecoder* dec, cJSON* root )
{
	if( root )
	{
		dec->Name                       = json_string(root, "Name");
		dec->DecoderID                  = json_int(root,    "DecoderID");
		dec->SessionID                  = json_int(root,    "SessionID");
		dec->GlobalMute                 = json_float(root,  "GlobalMute");
		dec->EnableClosedCaption        = get_bool(root,    "EnableClosedCaption");
		dec->ClosedCaptionChannel       = json_int(root,    "ClosedCaptionChannel");
		dec->EnableAnalogClosedCaption  = get_bool(root,    "EnableAnalogClosedCaption");
		dec->AnalogClosedCaptionChannel = json_int(root,    "AnalogClosedCaptionChannel");
		dec->InputID                    = json_int(root,    "InputID");
	}
}

// Event processing
void receive_event_from_yahoo( const char** json, const int stream_count )
{
	for (int idx = 0; idx < stream_count; idx++)
	{
		if( cJSON* root = cJSON_Parse(json[idx]) )
		{
			const char* EventName = cJSON_GetObjectItem(root, "EventName")->valuestring;
			uint32_t hash = SDK::Hash(EventName, strlen(EventName));

			switch( hash )
			{
				case YCTV_HASH_Buffer:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						fprintf(stderr, "BUFFER %s\n", dec->Name.c_str());

						YCTVMediaUpdateDecoderState(dec->SessionID,
													dec->InputID,
													YCTVMedia::StateBuffering,
													"Buffering");
					}
					break;

				case YCTV_HASH_ChangeChannel:
					fprintf(stderr, "YCTV_HASH_ChangeChannel not implemented.\n");
					break;

				case YCTV_HASH_ChangeVolume:
					fprintf(stderr, "YCTV_HASH_ChangeVolume not implemented.\n");
					break;

				case YCTV_HASH_ConfigureDecoder:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						get_dec_config(dec, get_obj(root, "Decoder"));
						get_input_config(dec->Input, get_obj(root, "Input"));
						fprintf(stderr,
								"CONFIG %s - %s\n",
								dec->Name.c_str(),
								dec->Input.Name.c_str());
					}
					break;

				case YCTV_HASH_ConfigureInput:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						get_input_config(dec->Input, get_obj(root, "Input"));
						fprintf(stderr, "CONFIG %s\n", dec->Input.Name.c_str());
					}
					break;

				case YCTV_HASH_DeviceHotPlug:
					fprintf(stderr, "YCTV_HASH_DeviceHotPlug not implemented.\n");
					break;

				case YCTV_HASH_GetCapabilities:
					fprintf(stderr, "YCTV_HASH_GetCapabilities not implemented.\n");
					break;

				case YCTV_HASH_GetProgramInfo:
					fprintf(stderr, "YCTV_HASH_GetProgramInfo not implemented.\n");
					break;

				case YCTV_HASH_ListDirectory:
					fprintf(stderr, "YCTV_HASH_ListDirectory not implemented.\n");
					break;

				case YCTV_HASH_Pause:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						fprintf(stderr, "PAUSE %s\n", dec->Name.c_str());
						if (player_pid>=0)
							amplayer_pause(player_pid);
					}
					break;

				case YCTV_HASH_Play:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						dec->PlayCount = json_int(root, "PlaybackCount");
						fprintf(stderr, "PLAY %s SPEED %f REPEAT %d\n", dec->Name.c_str(), json_float(root, "Speed"), dec->PlayCount);

						if (player_pid>=0){
							if (amplayer_status(player_pid)==PLAYER_PAUSE)
								amplayer_resume(player_pid);
						}
						else{
							if (player_pid>=0){
							    if (PLAYER_THREAD_IS_RUNNING(amplayer_status(player_pid)))
								    amplayer_stop(player_pid);
							    amplayer_exit(player_pid);
							    if (player_ctrl){
								    free(player_ctrl);
							    }
							    player_pid = -1;
							}
							player_ctrl = amplayer_init((char*)dec->Input.URI.c_str());
							if (dec->PlayCount<0)
								player_ctrl->loop_mode = 0xffff;
							else
								player_ctrl->loop_mode = dec->PlayCount;
							player_pid = amplayer_start(player_ctrl);
                            
						}							
						YCTVMediaUpdateDecoderState(dec->SessionID,
													dec->InputID,
													YCTVMedia::StatePlaying,
													"Playing");
					}
					break;

				case YCTV_HASH_Seek:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						fprintf(stderr, "SEEK %s POSITION %d\n", dec->Name.c_str(), json_int(root, "TimeIndex"));
                        if ((player_pid>=0)&&(PLAYER_THREAD_IS_RUNNING(amplayer_status(player_pid))))
                            amplayer_timesearch(player_pid, json_int(root, "TimeIndex"));
						YCTVMediaUpdateDecoderState(dec->SessionID,
													dec->InputID,
													YCTVMedia::StatePlaying,
													"Playing");
					}
					break;

				case YCTV_HASH_SendEventToPeripheral:
					fprintf(stderr, "YCTV_HASH_SendEventToPeripheral not implemented.\n");
					break;

				case YCTV_HASH_StartChannelScan:
					fprintf(stderr, "YCTV_HASH_StartChannelScan not implemented.\n");
					break;

				case YCTV_HASH_Stop:
					if( SampleDecoder* dec = get_decoder(root) )
					{
						fprintf(stderr, "STOP %s\n", dec->Name.c_str());
						if (player_pid>=0){
							amplayer_stop(player_pid);
							amplayer_exit(player_pid);
							if (player_ctrl){
								free(player_ctrl);
							}
							player_pid = -1;
						}
						YCTVMediaUpdateDecoderState(dec->SessionID,
													dec->InputID,
													YCTVMedia::StateStopped,
													"Stopped");
					}
					break;

				default:
					fprintf(stderr, "Unsupported event!\n [%s]\n", json[idx]);
					break;
			}
			cJSON_Delete(root);
		}
	}
}

int test_uri(char *uri)
{
	callback_t callback_fn;
	char* file_name = strdup(uri);
	printf("filename %s\n", file_name);

	player_ctrl = amplayer_init(file_name);
 	player_ctrl->loop_mode = 3; //repeat 3 times
        player_pid = amplayer_start(player_ctrl);
	while(!PLAYER_THREAD_IS_STOPPED(player_get_state(player_pid))){
		usleep(100*1000);
        signal(SIGCHLD, SIG_IGN);        
		signal(SIGTSTP, SIG_IGN);        
		signal(SIGTTOU, SIG_IGN);        
		signal(SIGTTIN, SIG_IGN);        
		signal(SIGHUP, signal_handler);        
		signal(SIGTERM, signal_handler);        
		signal(SIGSEGV, signal_handler);        
		signal(SIGINT, signal_handler);        
		signal(SIGQUIT, signal_handler);	
	};
	return 0;
}


int main( int argc, char**argv)
{
        if (argc > 1){
                return test_uri(argv[1]);
	}
        else
        {
        // Sets up sample data structures
        init_decoders(0, 1, "Decoder 1");
        init_decoders(1, 2, "Decoder 2");

        atexit(remove_decoders);

        // initialize media extension
        YCTVMedia media;
        media.OnReceiveEvent = receive_event_from_yahoo;
        media.ConfigFilePath = "./config_media.json";

        YCTVMediaInit(media, true);

        // wait for events...
        YCTVMediaWaitForEvents();

        // shutdown
        YCTVMediaShutdown();
        }
        return 0;
}

