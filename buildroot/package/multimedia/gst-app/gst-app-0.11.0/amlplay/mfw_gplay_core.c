/*
 * Copyright (c) 2009-2012, Freescale Semiconductor, Inc. All rights reserved. 
 *
 */
 
/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gst/gst.h>

// for fullscreen setting
#include <fcntl.h>
#include <string.h>
#ifndef _MX233
//#include <../../inc/misc/linux/mxcfb.h>
#include <linux/fb.h>
#endif

#include "mfw_gplay_core.h"
#include "gst_snapshot.h"

#define DEFAULT_PLAYBACK_RATE 1.0
#define DEFAULT_VOLUME 1.0
#define DEFAULT_OFFSET_X 0
#define DEFAULT_OFFSET_Y 0
#define DEFAULT_DISPLAY_WIDTH 320
#define DEFAULT_DISPLAY_HEIGHT 240

#define MILLISECOND2NANOSECOND 1000000

#define FADE_TIMEOUT_US (700000)

#define CALLBACK_QUEUE_LENTH 256 //16

// internal structure
typedef struct
{
    GstElement *video_sink;
    GstElement *audio_sink;
    GstElement *visual;
    GstElement* playbin;
    GMainLoop* g_main_loop;
    GThread* g_main_loop_thread;
    fsl_player_s32 watchid;
    fsl_player_config config;

    fsl_player_u64 duration;
    fsl_player_u64 elapsed;
    fsl_player_u64 elapsed_video;
    fsl_player_u64 elapsed_audio;

    fsl_player_state player_state;
    double playback_rate;
    gdouble volume;
    fsl_player_bool bmute;
    fsl_player_bool bfullscreen;
    fsl_player_bool btv_enable;
    fsl_player_s32 tv_mode;
    fsl_player_display_parameter display_parameter;
    fsl_player_rotation rotate_value;

    fsl_player_metadata metadata;

    fsl_player_mutex_t status_switching_mutex;
    fsl_player_queue queue;
    fsl_player_sema_t eos_semaphore;
    fsl_player_sema_t stop_semaphore;
    fsl_player_s32    total_frames;

    fsl_player_s32 verbose;
    fsl_player_bool auto_buffering; 
    fsl_player_bool auto_redirect;
    fsl_player_bool fade;
    fsl_player_bool mute_status;
    fsl_player_bool has_mute;

    fsl_player_s32 timeout;

		fsl_player_bool bchoose;
		fsl_player_bool bts;
    fsl_player_bool bape;
    
} fsl_player_property;


fsl_player_ret_val fsl_player_set_media_location(fsl_player_handle handle, fsl_player_s8* filename, fsl_player_drm_format* drm_format);
fsl_player_ret_val fsl_player_play(fsl_player_handle handle);
fsl_player_ret_val fsl_player_pause(fsl_player_handle handle);
//fsl_player_ret_val fsl_player_resume(fsl_player_handle handle);
fsl_player_ret_val fsl_player_stop(fsl_player_handle handle);
fsl_player_ret_val fsl_player_seek(fsl_player_handle handle, fsl_player_u32 time_ms, fsl_player_u32 flags);
fsl_player_ret_val fsl_player_set_playback_rate(fsl_player_handle handle, double playback_rate);

fsl_player_ret_val fsl_player_set_volume(fsl_player_handle handle, double volume);
fsl_player_ret_val fsl_player_mute(fsl_player_handle handle);
fsl_player_ret_val fsl_player_snapshot(fsl_player_handle handle);
fsl_player_ret_val fsl_player_set_video_output(fsl_player_handle handle, fsl_player_video_output_mode mode);
fsl_player_ret_val fsl_player_select_audio_track(fsl_player_handle handle, fsl_player_s32 track_no);
fsl_player_ret_val fsl_player_select_subtitle(fsl_player_handle handle, fsl_player_s32 subtitle_no);
fsl_player_ret_val fsl_player_full_screen(fsl_player_handle handle);
fsl_player_ret_val fsl_player_display_screen_mode(fsl_player_handle handle, fsl_player_s32 mode);
fsl_player_ret_val fsl_player_resize(fsl_player_handle handle, fsl_player_display_parameter display_parameter);
fsl_player_ret_val fsl_player_rotate(fsl_player_handle handle, fsl_player_rotation rotate_value);

fsl_player_ret_val fsl_player_get_property(fsl_player_handle handle, fsl_player_property_id property_id, void* pstructure);
fsl_player_ret_val fsl_player_set_property(fsl_player_handle handle, fsl_player_property_id property_id, void* pstructure);

fsl_player_ret_val fsl_player_wait_message(fsl_player_handle handle, fsl_player_ui_msg** msg, fsl_player_s32 timeout);
fsl_player_ret_val fsl_player_send_message_exit(fsl_player_handle handle);
fsl_player_ret_val fsl_player_exit_message_loop(fsl_player_handle handle);

fsl_player_ret_val fsl_player_post_eos_semaphore(fsl_player_handle handle);

fsl_player_ret_val fsl_player_property_process(fsl_player_handle handle, char *prop_name);
GstElement * getAudioDecElement(fsl_player_property* pproperty ,fsl_player_s32 i);
static gint64 getcurpos(fsl_player_property* pproperty);

static fsl_player_bool poll_for_state_change(GstState sRecState, GstElement * elem, fsl_player_u32 timeout)
{
    GTimeVal tfthen, tfnow;
    GstClockTimeDiff diff;
    GstStateChangeReturn result = GST_STATE_CHANGE_FAILURE;
    GstState current;
    guint32 timeescap = 0;
    gchar *ele_name = gst_element_get_name(elem);

    g_get_current_time(&tfthen);
    result = gst_element_set_state(elem, sRecState);
    if(result == GST_STATE_CHANGE_FAILURE)
    {
        if( NULL != ele_name )
        {
            g_free(ele_name);
        }
        return FSL_PLAYER_FALSE;
    }

    while(1)
    {
        if (gst_element_get_state(elem, &current, NULL, GST_SECOND)!=GST_STATE_CHANGE_FAILURE){
        g_get_current_time(&tfnow);
        diff = GST_TIMEVAL_TO_TIME(tfnow) - GST_TIMEVAL_TO_TIME(tfthen);
        diff /= (1000 * 1000);
        timeescap = (unsigned int) diff;

        if( sRecState == current )
        {
            break;
        }
        else
        {
            if ((timeout) && (timeescap > (timeout * 1000)))	
            {
                FSL_PLAYER_PRINT( "\n%s(): Element %s time out in state transferring from %s to %s\n", 
                    __FUNCTION__, ele_name, 
                    gst_element_state_get_name (current),
                    gst_element_state_get_name (sRecState) );
                if( NULL != ele_name )
                {
                    g_free(ele_name);
                }
                return FSL_PLAYER_FALSE;
            }
        }
        usleep(1000000);
        g_print("Wait status change from %d to %d \n", current, sRecState);
        }

        else{
          g_print("state change failed from %d to %d \n", current, sRecState);
          return FSL_PLAYER_FALSE;
        }
        //FSL_PLAYER_PRINT( "\ntimeescap=%d.Element %s time out in state transferring from %s to %s\n", 
        //  ele_name, timeescap, 
        //  gst_element_state_get_name (current),
        //  gst_element_state_get_name (sRecState) );
    }

    if( NULL != ele_name )
    {
        g_free(ele_name);
    }

    return FSL_PLAYER_TRUE;
}

void g_main_loop_thread_fun(gpointer data)
{
    if(data)
    {
        fsl_player* pplayer = (fsl_player*)data;
        fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
        if(pproperty->g_main_loop)
        {   
            GMainLoop *loop =  g_main_loop_ref(pproperty->g_main_loop);
            GMainContext *context = g_main_loop_get_context(loop);
            while (g_main_loop_is_running(loop))
            {
                while (g_main_context_iteration (context, FALSE));
                usleep(10000);
            }
            g_main_loop_unref(loop);
        }
    }
	FSL_PLAYER_PRINT("g_main_loop_thread_fun() quit!\n");
}

/* Get metadata information. */
static void get_metadata_tag(const GstTagList * list, const gchar * tag,
		      gpointer data)
{
    fsl_player* pplayer = (fsl_player*)data;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_s32 count = gst_tag_list_get_tag_size(list, tag);
    fsl_player_s32 i = 0;

    for (i = 0; i < count; i++)
    {
        gchar *str = NULL;

        if( gst_tag_get_type(tag) == G_TYPE_STRING )
        {
            if( !gst_tag_list_get_string_index(list, tag, i, &str) )
            {
                g_assert_not_reached();
            }
            /* Need to judge whether return str address is NULL, engr56736 */
            if( NULL == str )
            {
                FSL_PLAYER_PRINT(" get string pointer return NULL\n");
                return;
            }
        }
        else if( gst_tag_get_type(tag) == GST_TYPE_BUFFER )
        {
            GstBuffer *img;

            img = gst_value_get_buffer(gst_tag_list_get_value_index(list, tag, i));
            if( img )
            {
                gchar *caps_str;

                caps_str = GST_BUFFER_CAPS(img) ? gst_caps_to_string(GST_BUFFER_CAPS(img)) : g_strdup("unknown");
                str = g_strdup_printf("buffer of %u bytes, type: %s", GST_BUFFER_SIZE(img), caps_str);
                if( NULL != caps_str )
                {
                    g_free(caps_str);
                }
            }
            else
            {
                str = g_strdup("NULL buffer");
            }
        }
        else
        {
            str = g_strdup_value_contents(gst_tag_list_get_value_index(list, tag, i));
        }

        if( 0 == i )
        {
            FSL_PLAYER_PRINT("%15s: %s\n", gst_tag_get_nick(tag), str);

            if( strncmp(gst_tag_get_nick(tag), "location", 8) == 0 )
            {
                strncpy(pproperty->metadata.currentfilename, str, sizeof(pproperty->metadata.currentfilename));
                pproperty->metadata.currentfilename[sizeof(pproperty->metadata.currentfilename)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "title", 5) == 0 )
            {
                strncpy(pproperty->metadata.title, str, sizeof(pproperty->metadata.title));
                pproperty->metadata.title[sizeof(pproperty->metadata.title)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "artist", 6) == 0 )
            {
                strncpy(pproperty->metadata.artist, str, sizeof(pproperty->metadata.artist));
                pproperty->metadata.artist[sizeof(pproperty->metadata.artist)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "album", 5) == 0 )
            {
                strncpy(pproperty->metadata.album, str, sizeof(pproperty->metadata.album));
                pproperty->metadata.album, str[sizeof(pproperty->metadata.album, str)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "date", 4) == 0 )
            {
                strncpy(pproperty->metadata.year, str, sizeof(pproperty->metadata.year));
                pproperty->metadata.year[sizeof(pproperty->metadata.year)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "genre", 5) == 0 )
            {
                strncpy(pproperty->metadata.genre, str, sizeof(pproperty->metadata.genre));
                pproperty->metadata.genre[sizeof(pproperty->metadata.genre)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "audio codec", 11) == 0 )
            {
                strncpy(pproperty->metadata.audiocodec, str, sizeof(pproperty->metadata.audiocodec));
                pproperty->metadata.audiocodec[sizeof(pproperty->metadata.audiocodec)-1]= '\0';

             // printf("audio codec [%s]\n", pproperty->metadata.audiocodec);
            }
            if( strncmp(gst_tag_get_nick(tag), "bitrate", 7) == 0 )
            {
                pproperty->metadata.audiobitrate = atoi(str);
            }
            if( strncmp(gst_tag_get_nick(tag), "video codec", 11) == 0 )
            {
                strncpy(pproperty->metadata.videocodec, str, sizeof(pproperty->metadata.videocodec));
                pproperty->metadata.videocodec[sizeof(pproperty->metadata.videocodec)-1] = '\0';
            }
            if( strncmp(gst_tag_get_nick(tag), "image width", 11) == 0 )
            {
                pproperty->metadata.width = atoi(str);
            }
            if( strncmp(gst_tag_get_nick(tag), "image height", 12) == 0 )
            {
                pproperty->metadata.height = atoi(str);
            }
            if( strncmp(gst_tag_get_nick(tag), "number of channels", 18) == 0 )
            {
                pproperty->metadata.channels = atoi(str);
            }
            if (strncmp(gst_tag_get_nick(tag), "sampling frequency (Hz)", 23) == 0)
            {
                pproperty->metadata.samplerate = atoi(str);
            }

        }
        else
        {
            if(str)
            {
                FSL_PLAYER_PRINT("               : %s\n", str);
            }
        }
ignore:
    if (str)
        g_free(str);
    }
}

static gboolean my_bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
    fsl_player* pplayer = (fsl_player*)data;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_ui_msg* pui_msg = NULL;

    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            FSL_PLAYER_PRINT("EOS Found!\n");

            // Reset mute to none-mute. for example, play over when in FF/SF/FB status
            if( pproperty->bmute )
            {
                fsl_player_mute(pplayer);
            }
            pui_msg = fsl_player_ui_msg_new_empty(FSL_PLAYER_UI_MSG_EOS);
            if( NULL == pui_msg )
            {
                FSL_PLAYER_PRINT("EOS Message is not sending out.\n");
                break;
            }
            pproperty->queue.klass->put( &(pproperty->queue), pui_msg, FSL_PLAYER_FOREVER, FSL_PLAYER_UI_MSG_PUT_NEW);
            // Wait until eos message has been processed completely. Fix the bug of play/stop hang problem.
            FSL_PLAYER_SEMA_WAIT( &(pproperty->eos_semaphore) );
            break;
	    }

        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *err;
            gst_message_parse_error(msg, &err, &debug);
            FSL_PLAYER_PRINT("Debug: %s\n", debug);
            g_free(debug);
            if(err)
            {
                FSL_PLAYER_PRINT("Error: %s\n", err->message);
            }
            g_error_free(err);


            fsl_player_stop(pplayer);

            pui_msg = fsl_player_ui_msg_new_empty(FSL_PLAYER_UI_MSG_INTERNAL_ERROR);
            if( NULL == pui_msg )
            {
                FSL_PLAYER_PRINT("FSL_PLAYER_UI_MSG_INTERNAL_ERROR Message is not sending out.\n");
                break;
            }
            pproperty->queue.klass->put( &(pproperty->queue), (void*)/*&*/pui_msg, FSL_PLAYER_FOREVER, FSL_PLAYER_UI_MSG_PUT_NEW );

            break;
        }
    
        case GST_MESSAGE_TAG:
        {
#if 1
            GstTagList *tags;
            gst_message_parse_tag(msg, &tags);
            FSL_PLAYER_PRINT("FOUND GST_MESSAGE_TAG!\n");
            gst_tag_list_foreach(tags, get_metadata_tag, data);
            gst_tag_list_free(tags);
#endif
            break;
        }

        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState old_state, new_state;
            gchar *src_name;

            gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);

            if(old_state == new_state)
                break;

            if( GST_MESSAGE_SRC (msg) == GST_OBJECT (pproperty->playbin) )
            {
                if( (GST_STATE_PAUSED==old_state && GST_STATE_READY==new_state) ||
                    (GST_STATE_READY==old_state && GST_STATE_NULL==new_state) )
                {
                }
            }

            /* we only care about playbin (pipeline) state changes */
            src_name = gst_object_get_name (msg->src);

            /* now do stuff */
            if( new_state <= GST_STATE_PAUSED )
            {
            }
            else if( new_state > GST_STATE_PAUSED )
            {
            }
            //DBG_PRINT("%s(): GST_MESSAGE_STATE_CHANGED from %d to %d\n", src_name, old_state, new_state);

            
            g_free (src_name);
	        break;
        }

        case GST_MESSAGE_ELEMENT:
            {
                gchar * msgstr;
                if (msgstr = gst_structure_to_string(msg->structure)){
                  g_print("get GST_MESSAGE_ELEMENT %s\n", msgstr);
                  g_free(msgstr);
                }
            }
            break;
        
        case GST_MESSAGE_BUFFERING:
            {
                gint percent = 0;
                fsl_player_ret_val ret;
                GstState current = GST_STATE_NULL;
                GstStateChangeReturn stateret;
                gst_message_parse_buffering (msg, &percent);

                pui_msg = fsl_player_ui_msg_new_empty(FSL_PLAYER_UI_MSG_BUFFERING);
                fsl_player_ui_msg_body_buffering * pui_body = malloc(sizeof(fsl_player_ui_msg_body_buffering));
                if( (NULL == pui_msg ) || (pui_body==NULL))
                {
                    if (pui_msg) fsl_player_ui_msg_free(pui_msg);
                    if (pui_body) free(pui_body);
                    FSL_PLAYER_PRINT("FSL_PLAYER_UI_MSG_INTERNAL_ERROR Message is not sending out.\n");
                    break;
                }
                pui_msg->msg_body = pui_body;
                pui_body->percent = percent;
                pproperty->queue.klass->put( &(pproperty->queue), (void*)/*&*/pui_msg, FSL_PLAYER_FOREVER, FSL_PLAYER_UI_MSG_REPLACE);

                if (pproperty->auto_buffering){
                    if (percent==0){
                        FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );   
                        if (pproperty->player_state==FSL_PLAYER_STATUS_PLAYING){
                            
                            stateret = gst_element_get_state(pproperty->playbin, &current, NULL, GST_SECOND);
                            
                            if( current == GST_STATE_PLAYING){
                                g_print("switch to PAUSE\n");
                                 gst_element_set_state((pproperty->playbin), GST_STATE_PAUSED);
                                //ret = poll_for_state_change(GST_STATE_PAUSED, (pproperty->playbin));
                                g_print("done\n");
                                
                            }
                        }
                        FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
                    }else if (percent>=100){

                        FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );   

                        if (pproperty->player_state==FSL_PLAYER_STATUS_PLAYING){
                            
                            stateret = gst_element_get_state(pproperty->playbin, &current, NULL, GST_SECOND);
                            if (1){//( current == GST_STATE_PAUSED){
                                g_print("switch to PLAYING\n");
                                //gst_element_set_state((pproperty->playbin), GST_STATE_PLAYING);
                                ret = poll_for_state_change(GST_STATE_PLAYING, (pproperty->playbin), pproperty->timeout);
                                g_print("done\n");
                            }
                            
                        }
                        FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
                    }
                }
            }
            break;

        default:
            break;
    }

    return FSL_PLAYER_TRUE;
}

fsl_player_ret_val fsl_player_class_init (fsl_player_class * klass)
{
    klass->set_media_location = &fsl_player_set_media_location;
    klass->play = &fsl_player_play;
    klass->pause = &fsl_player_pause;
    klass->stop = &fsl_player_stop;
    klass->seek = &fsl_player_seek;
    klass->set_playback_rate = &fsl_player_set_playback_rate;
    klass->set_volume = &fsl_player_set_volume;
    klass->mute = &fsl_player_mute;
    klass->select_audio_track = &fsl_player_select_audio_track;
    klass->select_subtitle = &fsl_player_select_subtitle;
    klass->get_property = &fsl_player_get_property;
    klass->set_property = &fsl_player_set_property;
    klass->wait_message = &fsl_player_wait_message;
    klass->send_message_exit = &fsl_player_send_message_exit;
    klass->exit_message_loop = &fsl_player_exit_message_loop;
    klass->post_eos_semaphore = &fsl_player_post_eos_semaphore;
    klass->property = &fsl_player_property_process;
        
    return FSL_PLAYER_SUCCESS;
}

fsl_player_handle fsl_player_initwav(fsl_player_config * config, fsl_player_s8* name)
{ 

    GstBus* bus = NULL;
    fsl_player* pplayer = NULL;
    fsl_player_property* pproperty = NULL;
    fsl_player_element_property * ele_property;
    fsl_player_element_signal_handler * ele_sighandle;
    gchar *l;
    GError *err = NULL;

 /* Specify the log file. */
    if (fsl_player_logfile == NULL)
        fsl_player_logfile = stdout;

    if (config->api_version!=GPLAYCORE_API_VERSION){
        g_print("Wrong API version %d, expect %d!!\n", config->api_version, GPLAYCORE_API_VERSION);
        goto init_failed;
    }

    pplayer = (fsl_player*)malloc(sizeof(fsl_player));
    if( NULL == pplayer )
    {
        FSL_PLAYER_PRINT("%s(): Failed in g_malloc(fsl_player)!\n", __FUNCTION__);
        goto init_failed;
    }
    memset(pplayer, 0, sizeof(fsl_player));
       
    pplayer->property_handle = malloc(sizeof(fsl_player_property));
    if( NULL == pplayer->property_handle )
    {
        FSL_PLAYER_PRINT("%s(): Failed in malloc(fsl_player_property)!\n", __FUNCTION__);
        goto init_failed;
    }
    pproperty = (fsl_player_property*)pplayer->property_handle;
    memset(pproperty, 0, sizeof(fsl_player_property));

    
    pproperty->config.playbin_version = config->playbin_version;
    pproperty->verbose = config->verbose;

    pproperty->timeout = config->timeout_second;
    
    if (config->features & GPLAYCORE_FEATURE_AUTO_BUFFERING){
        pproperty->auto_buffering = 1;
    }
    if (config->features & GPLAYCORE_FEATURE_AUTO_REDIRECT_URI){
        pproperty->auto_redirect = 1;
    }
    if (config->features & GPLAYCORE_FEATURE_AUDIO_FADE){
        pproperty->fade = TRUE;
    }

    gst_init(NULL, NULL);
    pproperty->g_main_loop = g_main_loop_new(NULL, TRUE);
	
    l = g_strdup_printf (" filesrc location=\"%s\" !wavparse !amladec !amlasink",name);
    pproperty->playbin = gst_parse_launch (l, &err);
    if (pproperty->playbin== NULL || err != NULL) {
      g_printerr ("Cannot build pipeline: %s\n", err->message);
      g_error_free (err);
      g_free (l);
      if (pproperty->playbin)
        gst_object_unref (pproperty->playbin);
      return -1;
    }
    g_free (l);

    bus = gst_pipeline_get_bus(GST_PIPELINE(pproperty->playbin));
    if( NULL == bus )
    {
        FSL_PLAYER_PRINT("%s(): Failed in gst_pipeline_get_bus()!\n", __FUNCTION__);
        goto init_failed;
    }
    pproperty->watchid = gst_bus_add_watch(bus, my_bus_callback, pplayer);
    gst_object_unref(bus);

    if( NULL == pproperty->g_main_loop_thread )
    {
		pproperty->g_main_loop_thread = g_thread_create((GThreadFunc)g_main_loop_thread_fun, pplayer, FSL_PLAYER_TRUE, NULL);
    }

    pproperty->player_state = FSL_PLAYER_STATUS_STOPPED;
    pproperty->playback_rate = DEFAULT_PLAYBACK_RATE;
    pproperty->volume = DEFAULT_VOLUME;
    pproperty->bmute = 0;
    pproperty->display_parameter.offsetx = DEFAULT_OFFSET_X;
    pproperty->display_parameter.offsety = DEFAULT_OFFSET_Y;
    pproperty->display_parameter.disp_width = DEFAULT_DISPLAY_WIDTH;
    pproperty->display_parameter.disp_height = DEFAULT_DISPLAY_HEIGHT;
    pproperty->rotate_value = FSL_PLAYER_ROTATION_NORMAL;
    pproperty->bfullscreen = 0;
    pproperty->btv_enable = 0;
    pproperty->tv_mode = FSL_PLAYER_VIDEO_OUTPUT_LCD;

    pproperty->duration = 0;
    pproperty->elapsed = 0;

    FSL_PLAYER_MUTEX_INIT( &(pproperty->status_switching_mutex) );

    // Initialize queue.
    fsl_player_queue_inst_init( &(pproperty->queue) );
    //fsl_player_queue_class_init( pproperty->queue.klass );
    //pproperty->queue.klass->config( &(pproperty->queue), CALLBACK_QUEUE_LENTH );

    FSL_PLAYER_SEMA_INIT( &(pproperty->eos_semaphore), 0 );
    FSL_PLAYER_SEMA_INIT( &(pproperty->stop_semaphore), 0 );

    // assign the function pointers.
    pplayer->klass = (fsl_player_class*)malloc( sizeof(fsl_player_class) );
    if( NULL == pplayer->klass )
    {
        FSL_PLAYER_PRINT("klass: fail to init klass.\n");
        goto init_failed;
    }
    fsl_player_class_init( pplayer->klass );

    FSL_PLAYER_PRINT("%s(): Successfully initialize!\n", __FUNCTION__);

    return pplayer;

init_failed:
    if(pproperty)
    {
        free(pproperty);
        pproperty = NULL;
    }
    if(pplayer)
    {
        free(pplayer);
        pplayer = NULL;
    }
    FSL_PLAYER_PRINT("%s(): Failed initialization!\n", __FUNCTION__);
    return pplayer;
} 

fsl_player_handle fsl_player_init(fsl_player_config * config)
{
    GstBus* bus = NULL;
    fsl_player* pplayer = NULL;
    fsl_player_property* pproperty = NULL;
    fsl_player_element_property * ele_property;
    fsl_player_element_signal_handler * ele_sighandle;

    /* Specify the log file. */
    if (fsl_player_logfile == NULL)
        fsl_player_logfile = stdout;

    if (config->api_version!=GPLAYCORE_API_VERSION){
        g_print("Wrong API version %d, expect %d!!\n", config->api_version, GPLAYCORE_API_VERSION);
        goto init_failed;
    }

    pplayer = (fsl_player*)malloc(sizeof(fsl_player));
    if( NULL == pplayer )
    {
        FSL_PLAYER_PRINT("%s(): Failed in g_malloc(fsl_player)!\n", __FUNCTION__);
        goto init_failed;
    }
    memset(pplayer, 0, sizeof(fsl_player));
       
    pplayer->property_handle = malloc(sizeof(fsl_player_property));
    if( NULL == pplayer->property_handle )
    {
        FSL_PLAYER_PRINT("%s(): Failed in malloc(fsl_player_property)!\n", __FUNCTION__);
        goto init_failed;
    }
    pproperty = (fsl_player_property*)pplayer->property_handle;
    memset(pproperty, 0, sizeof(fsl_player_property));

    
    pproperty->config.playbin_version = config->playbin_version;
    pproperty->verbose = config->verbose;

    pproperty->timeout = config->timeout_second;
    
    if (config->features & GPLAYCORE_FEATURE_AUTO_BUFFERING){
        pproperty->auto_buffering = 1;
    }
    if (config->features & GPLAYCORE_FEATURE_AUTO_REDIRECT_URI){
        pproperty->auto_redirect = 1;
    }
    if (config->features & GPLAYCORE_FEATURE_AUDIO_FADE){
        pproperty->fade = TRUE;
    }

    gst_init(NULL, NULL);
    pproperty->g_main_loop = g_main_loop_new(NULL, TRUE);
    if( NULL == pproperty->g_main_loop )
    {
        FSL_PLAYER_PRINT("%s(): Failed in g_main_loop_new()!\n", __FUNCTION__);
        goto init_failed;
    }

    if( 2 == pproperty->config.playbin_version )
    {
        pproperty->playbin = gst_element_factory_make("playbin2", "playbin0");
        if (pproperty->fade==FALSE){
          pproperty->has_mute = TRUE;
        }
        FSL_PLAYER_PRINT("playbin2 is employed!\n");
    }
    else if( 1 == pproperty->config.playbin_version )
    {
        GstElement *auto_video_sink;
        pproperty->playbin = gst_element_factory_make("playbin", "playbin0");
        pproperty->has_mute = FALSE;
        FSL_PLAYER_PRINT("playbin is employed!\n");
        // There is different in playbin->video_sink between gstreamer10.22 and gstreamer10.25. autovideosink should be set into playbin
       
    }else{
        goto init_failed;
    }

    if( NULL == pproperty->playbin )
    {
        FSL_PLAYER_PRINT("%s(): Failed in gst_element_factory_make()!\n", __FUNCTION__);
        goto init_failed;
    }


    if (config->video_sink_name){
        g_print("Generate VideoSink %s\n", config->video_sink_name);
        pproperty->video_sink = gst_element_factory_make(config->video_sink_name, "vsink");
    }

    if (config->audio_sink_name){
        g_print("Generate AudioSink %s\n", config->audio_sink_name);
        pproperty->audio_sink = gst_element_factory_make(config->audio_sink_name, "asink");
    }

    
    if (config->visual_name){
        g_print("Generate visualization %s\n", config->visual_name);
        pproperty->visual = gst_element_factory_make(config->visual_name, "visual");
    }


    ele_sighandle = config->ele_signal_handlers;

    while(ele_sighandle){
        if ((ele_sighandle->handler) &&(ele_sighandle->signal_name)){
            if (ele_sighandle->type == ELEMENT_TYPE_VIDEOSINK){
                if (pproperty->video_sink)
                    g_signal_connect (G_OBJECT (pproperty->video_sink),
                                   ele_sighandle->signal_name, G_CALLBACK (ele_sighandle->handler), NULL);
            }
        }
        ele_sighandle = ele_sighandle->next;
    }

    
    ele_property = config->ele_properties;

    while(ele_property){
        void * object = NULL;
        if (ele_property->property_name){
            if (ele_property->type == ELEMENT_TYPE_VIDEOSINK){
                object = (void *)pproperty->video_sink;
            }else if (ele_property->type == ELEMENT_TYPE_PLAYBIN){
                object = (void *)pproperty->playbin;
            }

            if (object){

                switch(ele_property->property_type){
                    case ELEMENT_PROPERTY_TYPE_INT:
                        g_object_set(G_OBJECT (object),ele_property->property_name, ele_property->value_int, NULL);
                        break;
                    case ELEMENT_PROPERTY_TYPE_STRING:
                        g_object_set(G_OBJECT (object),ele_property->property_name, ele_property->value_string, NULL);
                        break;
                    case ELEMENT_PROPERTY_TYPE_INT64:
                        g_object_set(G_OBJECT (object),ele_property->property_name, ele_property->value_int64, NULL);
                        break;    
                    default:
                        break;
                }
            }
        }
        ele_property = ele_property->next;
    }



    if (pproperty->video_sink){
        g_object_set(pproperty->playbin, "video-sink", pproperty->video_sink, NULL);
    }

    if (pproperty->audio_sink){
        g_object_set(pproperty->playbin, "audio-sink", pproperty->audio_sink, NULL);
    }
    
    if (pproperty->visual){
        g_object_set(pproperty->playbin, "vis-plugin", pproperty->visual, NULL);
    }


    bus = gst_pipeline_get_bus(GST_PIPELINE(pproperty->playbin));
    if( NULL == bus )
    {
        FSL_PLAYER_PRINT("%s(): Failed in gst_pipeline_get_bus()!\n", __FUNCTION__);
        goto init_failed;
    }
    pproperty->watchid = gst_bus_add_watch(bus, my_bus_callback, pplayer);
    gst_object_unref(bus);

    if( NULL == pproperty->g_main_loop_thread )
    {
		pproperty->g_main_loop_thread = g_thread_create((GThreadFunc)g_main_loop_thread_fun, pplayer, FSL_PLAYER_TRUE, NULL);
    }

    pproperty->player_state = FSL_PLAYER_STATUS_STOPPED;
    pproperty->playback_rate = DEFAULT_PLAYBACK_RATE;
    pproperty->volume = DEFAULT_VOLUME;
    pproperty->bmute = 0;
    pproperty->display_parameter.offsetx = DEFAULT_OFFSET_X;
    pproperty->display_parameter.offsety = DEFAULT_OFFSET_Y;
    pproperty->display_parameter.disp_width = DEFAULT_DISPLAY_WIDTH;
    pproperty->display_parameter.disp_height = DEFAULT_DISPLAY_HEIGHT;
    pproperty->rotate_value = FSL_PLAYER_ROTATION_NORMAL;
    pproperty->bfullscreen = 0;
    pproperty->btv_enable = 0;
    pproperty->tv_mode = FSL_PLAYER_VIDEO_OUTPUT_LCD;

    pproperty->duration = 0;
    pproperty->elapsed = 0;

    FSL_PLAYER_MUTEX_INIT( &(pproperty->status_switching_mutex) );

    // Initialize queue.
    fsl_player_queue_inst_init( &(pproperty->queue) );
    //fsl_player_queue_class_init( pproperty->queue.klass );
    //pproperty->queue.klass->config( &(pproperty->queue), CALLBACK_QUEUE_LENTH );

    FSL_PLAYER_SEMA_INIT( &(pproperty->eos_semaphore), 0 );
    FSL_PLAYER_SEMA_INIT( &(pproperty->stop_semaphore), 0 );

    // assign the function pointers.
    pplayer->klass = (fsl_player_class*)malloc( sizeof(fsl_player_class) );
    if( NULL == pplayer->klass )
    {
        FSL_PLAYER_PRINT("klass: fail to init klass.\n");
        goto init_failed;
    }
    fsl_player_class_init( pplayer->klass );

    FSL_PLAYER_PRINT("%s(): Successfully initialize!\n", __FUNCTION__);

    return pplayer;

init_failed:
    if(pproperty)
    {
        free(pproperty);
        pproperty = NULL;
    }
    if(pplayer)
    {
        free(pplayer);
        pplayer = NULL;
    }
    FSL_PLAYER_PRINT("%s(): Failed initialization!\n", __FUNCTION__);
    return pplayer;
}

fsl_player_ret_val fsl_player_deinit(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    GstState current; 
    fsl_player_bool ret;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    
    gst_element_get_state(pproperty->playbin, &current, NULL, GST_SECOND);
    if( GST_STATE_NULL != current )
    {
        ret = poll_for_state_change(GST_STATE_NULL, (pproperty->playbin), pproperty->timeout);
    }

    gst_object_unref(pproperty->playbin);

    if( NULL != pproperty->g_main_loop )
    {
        g_main_loop_quit (pproperty->g_main_loop);
        g_main_loop_unref(pproperty->g_main_loop );
        pproperty->g_main_loop = NULL;
    }
    if( pproperty->g_main_loop_thread )
    {
        g_thread_join(pproperty->g_main_loop_thread);
        pproperty->g_main_loop_thread = NULL;
    }
	if( pproperty->watchid )
	{
        g_source_remove(pproperty->watchid);
        pproperty->watchid = 0;
	}	

    FSL_PLAYER_MUTEX_DESTROY( &(pproperty->status_switching_mutex) );

    // Deinitialize queue. No need to flush queue because flush operation is in deinit.
    //pproperty->queue.klass->flush( &(pproperty->queue) );
    fsl_player_queue_inst_deinit( &(pproperty->queue) );

    FSL_PLAYER_SEMA_DESTROY( &(pproperty->eos_semaphore) );
    FSL_PLAYER_SEMA_DESTROY( &(pproperty->stop_semaphore) );

    free(pplayer->klass);
    free(pproperty);
    free(pplayer);

    /* Close the log file. */    
    if (fsl_player_logfile != NULL && fsl_player_logfile != stdout)
    {
        //FSL_PLAYER_PRINT("%s(): log file closed.\n", __FUNCTION__);
        fclose (fsl_player_logfile);
        fsl_player_logfile = NULL;
    }

    FSL_PLAYER_PRINT("%s\n", __FUNCTION__);
    return FSL_PLAYER_SUCCESS;
}

fsl_player_s8* filename2uri(fsl_player_s8* uri, fsl_player_s8* fn)
{
    char * tmp;
    if (strstr(fn, "://")){
        sprintf(uri, "%s", fn);
    }else if( fn[0] == '/' ){
        sprintf(uri, "file://%s", fn);
    }
    else
    {
        fsl_player_s8* pwd = getenv("PWD");
        sprintf(uri, "file://%s/%s", pwd, fn);
    }
    if (tmp = strstr(uri, "|"))
       *tmp = '\0';
    return uri;
}

fsl_player_ret_val fsl_player_set_media_location(fsl_player_handle handle, fsl_player_s8* filename, fsl_player_drm_format* drm_format)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_s8 uri_buffer[512];
    char *p;
	
    p = strrchr(filename, '.');
    FSL_PLAYER_PRINT("p=%s\n",p);	
    if(p&&!strncmp(p,".wav",4))
        goto Done;
    filename2uri(uri_buffer,filename);

    
    g_object_set(G_OBJECT(pproperty->playbin), "uri", (gchar*)uri_buffer, NULL);

		if(0 == strncmp(p, ".ts", 3))
			pproperty->bts = TRUE;
		
		if(0 == strncmp(p, ".ape", 4))
			pproperty->bape = TRUE;

#if 0
    if (strncmp("file://", uri_buffer, 7)){
        //g_object_set(G_OBJECT(pproperty->playbin), "flags", 0x100, NULL);
        g_object_set(G_OBJECT(pproperty->playbin), "buffer-size", (fsl_player_s64)2000000, NULL);
    }
#endif    
Done:    
    FSL_PLAYER_PRINT("%s(): filename=%s\n", __FUNCTION__, filename);
    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_play(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_bool ret;

    FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );

    ret = poll_for_state_change(GST_STATE_PLAYING, (pproperty->playbin), pproperty->timeout);
    if( FSL_PLAYER_FALSE == ret)
    {
        FSL_PLAYER_PRINT("try to play failed\n");
        poll_for_state_change(GST_STATE_NULL, (pproperty->playbin), pproperty->timeout);
        FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
        return FSL_PLAYER_FAILURE;
    }

    if ((pproperty->fade) && (!pproperty->bmute)){
      _player_mute(handle, FALSE);
    }

    
    pproperty->player_state = FSL_PLAYER_STATUS_PLAYING;
    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);

    FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );

    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_pause(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_bool ret;
    GstState current;

    FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );

    //gst_element_get_state(pproperty->playbin, &current, NULL, GST_SECOND);
    if (pproperty->player_state == FSL_PLAYER_STATUS_PAUSED)
    //if( current == GST_STATE_PAUSED )
    {

        ret = poll_for_state_change(GST_STATE_PLAYING, (pproperty->playbin), pproperty->timeout);
        if( FSL_PLAYER_FALSE == ret )
        {
            FSL_PLAYER_PRINT("try to resume failed\n");
            FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
            return FSL_PLAYER_FAILURE;
        }
        else
        {
            FSL_PLAYER_PRINT("try to resume successfully\n");
        }
        if ((pproperty->fade) && (!pproperty->bmute)){
          _player_mute(handle, FALSE);
          
        }
        pproperty->player_state = FSL_PLAYER_STATUS_PLAYING;
    }
    else
    {
        if ((pproperty->fade) && (!pproperty->bmute)){
          _player_mute(handle, TRUE);
          usleep(FADE_TIMEOUT_US);
        }
        ret = poll_for_state_change(GST_STATE_PAUSED, (pproperty->playbin), pproperty->timeout);
        if( FSL_PLAYER_FALSE == ret )
        {
            FSL_PLAYER_PRINT("try to pause failed\n");
            FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
            return FSL_PLAYER_FAILURE;
        }
        else
        {
            FSL_PLAYER_PRINT("try to pause successfully\n");
        }
        pproperty->player_state = FSL_PLAYER_STATUS_PAUSED;
    }
    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);

    FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );

    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_seek(fsl_player_handle handle, fsl_player_u32 time_ms, fsl_player_u32 flags)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstEvent *seek_event = NULL;
    fsl_player_u64 seekpos;
    GstSeekFlags seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
    GstFormat fmt;
		GstElement *element = NULL;

    if( 0 == pproperty->duration )
    {
        fmt = GST_FORMAT_TIME;
        gst_element_query_duration(pproperty->playbin, &fmt, (fsl_player_s64 *)&(pproperty->duration));
    }

    if( (fsl_player_s64)(pproperty->duration) <= 0 )
    {
        FSL_PLAYER_PRINT("Seek Failed: pplayer->duration is %lld\n", pproperty->duration);
	    return FSL_PLAYER_FAILURE;
    }

    seekpos = (fsl_player_u64)time_ms * MILLISECOND2NANOSECOND;
    if( seekpos > pproperty->duration || seekpos < 0 )
    {
        FSL_PLAYER_PRINT("Seek Failed: Invalid seek point=%u(ms)!\n", time_ms);
	    return FSL_PLAYER_ERROR_BAD_PARAM;
    }

    if (flags & FSL_PLAYER_FLAG_SEEK_ACCURATE){
        seek_flags |= GST_SEEK_FLAG_ACCURATE;
    }
    
    g_print("seeking: %"GST_TIME_FORMAT"/%"GST_TIME_FORMAT"\n", GST_TIME_ARGS(seekpos), GST_TIME_ARGS(pproperty->duration));
    element = gst_bin_get_by_name(GST_BIN(pproperty->playbin), "flutsdemux0");
		if(NULL != element) {
    	g_object_set (element, "duration", pproperty->duration, NULL);
		}
    seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
                    seek_flags,
                    GST_SEEK_TYPE_SET, seekpos,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

    if( gst_element_send_event(pproperty->playbin, seek_event) == FSL_PLAYER_FALSE )
    {
        FSL_PLAYER_PRINT("Seek Failed: send seek event failed!\n");
        return FSL_PLAYER_FAILURE;
    }
    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_set_playback_rate(fsl_player_handle handle, double playback_rate)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstEvent *set_playback_rate_event = NULL;
    GstFormat fmt = GST_FORMAT_TIME;

    if( playback_rate > 8.0 || playback_rate < -8.0 /*|| (playback_rate>-2.0 && playback_rate<0.0)*/ )
    {
        FSL_PLAYER_PRINT("Set playback rate failed: Invalid playback_rate=%f which should be between [-8.0, 8.0]!\n" ,playback_rate);
        return FSL_PLAYER_ERROR_BAD_PARAM;
    }

    if( FSL_PLAYER_STATUS_PLAYING == pproperty->player_state )
    {
        fsl_player_pause(pplayer);
    }
    do {
        #if 0
        gint64 current_position;
        GstQuery* query;
        gboolean res;
        query = gst_query_new_position(GST_FORMAT_TIME);
        res = gst_element_query(pproperty->playbin, query);
        if( res )
        {
            gst_query_parse_position(query,NULL,&current_position);
            g_print("current_position = %"GST_TIME_FORMAT, GST_TIME_ARGS (current_position));
            g_print("\n");
        }
        else
        {
            g_print ("current_postion query failed...\n");
        }
        #endif
        gint64 current_position;
        GstElement* element;
        current_position = getcurpos(pproperty);
        element = gst_bin_get_by_name(GST_BIN(pproperty->playbin), "flutsdemux0");
        if(NULL != element) {
          g_object_set (element, "duration", pproperty->duration, NULL);
        }

        if( playback_rate >= 0.0 )
        {
            set_playback_rate_event =
                gst_event_new_seek(playback_rate, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                    GST_SEEK_TYPE_SET, current_position,
                    GST_SEEK_TYPE_SET, pproperty->duration);
        }
        else
        {
            set_playback_rate_event =
                gst_event_new_seek(playback_rate, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                    GST_SEEK_TYPE_SET, 0,
                    GST_SEEK_TYPE_SET, current_position);
        }
    } while(0);
    fsl_player_play(pplayer);

    if(gst_element_send_event(pproperty->playbin, set_playback_rate_event) == FSL_PLAYER_FALSE)
    {
        FSL_PLAYER_PRINT("Set playback rate failed: send setting playback rate event failed!\n");
        return FSL_PLAYER_FAILURE;
    }

    if( playback_rate > 1.0 && playback_rate <= 8.0 )
    {
        if( !pproperty->bmute )
        {
            fsl_player_mute(pplayer);
        }
        pproperty->player_state = FSL_PLAYER_STATUS_FASTFORWARD;
    }
    else if( 1.0 == playback_rate )
    {
        if( pproperty->bmute )
        {
            fsl_player_mute(pplayer);
        }
        pproperty->player_state = FSL_PLAYER_STATUS_PLAYING;
    }
    if( playback_rate > 0.0 && playback_rate < 1.0 )
    {
        if( !pproperty->bmute )
        {
            fsl_player_mute(pplayer);
        }
        pproperty->player_state = FSL_PLAYER_STATUS_SLOWFORWARD;
    }
    else if( 0.0 == playback_rate )
    {
        pproperty->player_state = FSL_PLAYER_STATUS_PAUSED;
    }
    else if( playback_rate >= -8.0 && playback_rate <= -0.0/*-1.0*/ )
    {
        if( !pproperty->bmute )
        {
            fsl_player_mute(pplayer);
        }
        pproperty->player_state = FSL_PLAYER_STATUS_FASTBACKWARD;
    }
    pproperty->playback_rate = playback_rate;
    
/*    if( (!pproperty->bmute) && (playback_rate!=1.0 || playback_rate!=0.0) )
    {
        fsl_player_mute(pplayer);
    }*/

    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_stop(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
#if 1
    fsl_player_bool ret;

    FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );

    if ((pproperty->fade) && (!pproperty->bmute)){
      _player_mute(handle, TRUE);
      usleep(FADE_TIMEOUT_US);
    }

    ret = poll_for_state_change(GST_STATE_READY/*GST_STATE_NULL*/, (pproperty->playbin), pproperty->timeout);
    if( FSL_PLAYER_FALSE == ret )
    {
        FSL_PLAYER_PRINT("try to stop failed\n");
        FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );
        return FSL_PLAYER_FAILURE;
    }
    pproperty->player_state = FSL_PLAYER_STATUS_STOPPED;
#else
    GstMessage *msg;
    GstBus *bus;

    GST_DEBUG ("stopping");

    FSL_PLAYER_MUTEX_LOCK( &(pproperty->status_switching_mutex) );

    gst_element_set_state (pproperty->playbin, GST_STATE_READY);

    /* process all remaining state-change messages so everything gets
     * cleaned up properly (before the state change to NULL flushes them) */
    GST_DEBUG ("processing pending state-change messages");
    bus = gst_element_get_bus(pproperty->playbin);
    while( (msg = gst_bus_poll (bus, GST_MESSAGE_STATE_CHANGED, 0)) )
    {
        gst_bus_async_signal_func (bus, msg, NULL);
        gst_message_unref (msg);
    }
    gst_object_unref (bus);
    pproperty->player_state = FSL_PLAYER_STATUS_STOPPED;
#endif
    //FSL_PLAYER_PRINT("fsl_player_stop Before:GGGGGGGGGGGGGGGGGGGGGGGGGGGG\n");
    //FSL_PLAYER_SEMA_POST( &(pproperty->stop_semaphore) );
    //FSL_PLAYER_PRINT("fsl_player_stop Before:GGGGGGGGGGGGGGGGGGGGGGGGGGGG\n");
    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);

    FSL_PLAYER_MUTEX_UNLOCK( &(pproperty->status_switching_mutex) );

    return FSL_PLAYER_SUCCESS;
}

/* accept 0-1000, corresponding as 0-10.0*/
fsl_player_ret_val fsl_player_set_volume(fsl_player_handle handle, double volume)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GValue value  = {0};

    if( volume < 0.0 )
    {
        FSL_PLAYER_PRINT("Set volume failed: Invalid volume=%lf which should be between [0, 1000]!\n" ,volume);
        return FSL_PLAYER_ERROR_BAD_PARAM;
    }
    pproperty->volume = (double)volume;
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_set_double(&value, pproperty->volume);
    g_object_set_property(G_OBJECT(pproperty->playbin), "volume", &value);
    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val _player_mute(fsl_player_handle handle, fsl_player_bool tomute)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GValue value  = {0};

    if (pproperty->has_mute){
      g_value_init(&value, G_TYPE_BOOLEAN);
      if( tomute )
      {
          pproperty->mute_status = TRUE;
          g_value_set_boolean(&value, TRUE);
      }
      else
      {
          pproperty->mute_status = FALSE;
          g_value_set_boolean(&value, FALSE);
      }
      g_object_set_property(G_OBJECT(pproperty->playbin), "mute", &value);
    }else{
      g_value_init(&value, G_TYPE_DOUBLE);
      if( tomute )
      {
          pproperty->mute_status = TRUE;
          g_value_set_double(&value, 0.0);
      }
      else
      {
          pproperty->mute_status = FALSE;
          g_value_set_double(&value, pproperty->volume);
      }
      g_object_set_property(G_OBJECT(pproperty->playbin), "volume", &value);
    }
}




fsl_player_ret_val fsl_player_mute(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    if( pproperty->bmute )
    {
        _player_mute(handle, FALSE);
        pproperty->bmute = FSL_PLAYER_FALSE;
    }
    else
    {
        _player_mute(handle, TRUE);
        pproperty->bmute = FSL_PLAYER_TRUE;
    }
    return FSL_PLAYER_SUCCESS;
}
#if 0
//#include "/usr/include/gtk-2.0/gdk-pixbuf/gdk-pixbuf.h"
//#include "gdk-pixbuf/gdk-pixbuf.h"
fsl_player_ret_val fsl_player_snapshot(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstStructure *s;
    /*GdkPixbuf **/ char* pixbuf;
    GstBuffer* buf = NULL;
    GstCaps* to_caps;
    fsl_player_s32 outwidth = 0;
    fsl_player_s32 outheight = 0;
    fsl_player_s32 red_mask = 0xff0000;
    fsl_player_s32 green_mask = 0x00ff00;
    fsl_player_s32 blue_mask = 0x0000ff;
    fsl_player_s8 snapshort_filename[256];
    static fsl_player_u32 snapshort_number = 0;

    g_object_get( pproperty->playbin, "frame", &buf, NULL );
    if( NULL == buf )
    {
        FSL_PLAYER_PRINT("Could not take snapshot: no last video frame!\n");
        return FSL_PLAYER_FAILURE;
    }
    if( NULL == GST_BUFFER_CAPS(buf) )
    {
        FSL_PLAYER_PRINT("Could not take snapshot: no caps on buffer!\n");
        return FSL_PLAYER_FAILURE;
    }

  /* convert to our desired format (RGB24) */
  to_caps = gst_caps_new_simple ("video/x-raw-rgb",
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24,
      /* Note: we don't ask for a specific width/height here, so that
       * videoscale can adjust dimensions from a non-1/1 pixel aspect
       * ratio to a 1/1 pixel-aspect-ratio */
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      "endianness", G_TYPE_INT, G_BIG_ENDIAN,
      "red_mask", G_TYPE_INT, red_mask,
      "green_mask", G_TYPE_INT, green_mask,
      "blue_mask", G_TYPE_INT, blue_mask,
      NULL);

  GST_DEBUG ("frame caps: %" GST_PTR_FORMAT, GST_BUFFER_CAPS (buf));
  GST_DEBUG ("pixbuf caps: %" GST_PTR_FORMAT, to_caps);

  /* bvw_frame_conv_convert () takes ownership of the buffer passed */
  //buf = bvw_frame_conv_convert (buf, to_caps);
  buf = gst_frame_convert(buf, to_caps);

  gst_caps_unref (to_caps);

  if (!buf) {
    /*GST_DEBUG*/g_print ("Could not take screenshot: %s", "conversion failed");
    g_warning ("Could not take screenshot: %s", "conversion failed");
    return FSL_PLAYER_FAILURE;
  }

  if (!GST_BUFFER_CAPS (buf)) {
    /*GST_DEBUG*/g_print ("Could not take screenshot: %s", "no caps on output buffer");
    g_warning ("Could not take screenshot: %s", "no caps on output buffer");
    return FSL_PLAYER_FAILURE;
  }

  s = gst_caps_get_structure (GST_BUFFER_CAPS (buf), 0);
  gst_structure_get_int (s, "width", &outwidth);
  gst_structure_get_int (s, "height", &outheight);
  g_return_val_if_fail (outwidth > 0 && outheight > 0, FSL_PLAYER_FAILURE);

  /* create pixbuf from that - use our own destroy function */
  /*pixbuf = gdk_pixbuf_new_from_data (GST_BUFFER_DATA (buf),
      GDK_COLORSPACE_RGB, FALSE, 8, outwidth, outheight,
      GST_ROUND_UP_4 (outwidth * 3), destroy_pixbuf, buf);*/
  /*if (!pixbuf) {
    GST_DEBUG ("Could not take screenshot: %s", "could not create pixbuf");
    g_warning ("Could not take screenshot: %s", "could not create pixbuf");
    gst_buffer_unref (buf);
  }*/

    sprintf( snapshort_filename, "/tmp/snapshot_%d.bmp", snapshort_number++ );
    FSL_PLAYER_PRINT("\tGST_BUFFER_SIZE(buf)%d\n", GST_BUFFER_SIZE(buf));
    gst_save_bmp( GST_BUFFER_DATA(buf), outwidth, outheight, red_mask, green_mask, blue_mask, snapshort_filename );
    gst_buffer_unref (buf);

    FSL_PLAYER_PRINT("Saving image file Done.\n");

    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);
    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_set_video_output(fsl_player_handle handle, fsl_player_video_output_mode mode)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);
    return FSL_PLAYER_SUCCESS;
}
#endif

static GstElement * 
getVideoDecElement(fsl_player_property* pproperty ,fsl_player_s32 i)
{
  GstPad *pad = NULL;
  GstPad *dec_pad = NULL;
  GstElement *e = NULL;

  if (!pproperty->playbin)
    return NULL;

  g_signal_emit_by_name(pproperty->playbin, "get-video-pad", i, &pad);
  if (pad) {
    dec_pad = gst_pad_get_peer(pad);
    while (dec_pad && GST_IS_GHOST_PAD(dec_pad)) {
      gst_object_unref(dec_pad);
      dec_pad = gst_ghost_pad_get_target(GST_GHOST_PAD(dec_pad));
    }
    if (dec_pad) {
      e = gst_pad_get_parent_element(dec_pad);
      gst_object_unref(dec_pad);
    }
    gst_object_unref(pad);
  }

  if (!e)
    FSL_PLAYER_PRINT("no VideoDecElement\n");

  return e;
}

GstElement * getAudioDecElement(fsl_player_property* pproperty ,fsl_player_s32 i)
{
  GstPad *pad = NULL;
  GstPad *dec_pad = NULL;
  GstElement *e = NULL;

  if (!pproperty->playbin)
    return NULL;

  g_signal_emit_by_name(pproperty->playbin, "get-audio-pad", i, &pad);
  if (pad) {
    dec_pad = gst_pad_get_peer(pad);
    while (dec_pad && GST_IS_GHOST_PAD(dec_pad)) {
      gst_object_unref(dec_pad);
      dec_pad = gst_ghost_pad_get_target(GST_GHOST_PAD(dec_pad));
    }
    if (dec_pad) {
      e = gst_pad_get_parent_element(dec_pad);
      gst_object_unref(dec_pad);
    }
    gst_object_unref(pad);
  }

  if (!e)
    FSL_PLAYER_PRINT("no audioDecElement\n");

  return e;
} 
static gint64 
fsl_player_prop_currentposition(fsl_player_property* pproperty)
{
	static GstElement* element = NULL;
	gint64 currentposition = 0;
  gint32 audionum = 0, videonum = 0;
  g_object_get(pproperty->playbin, "current-audio", &audionum, NULL);
  g_object_get(pproperty->playbin, "current-video", &videonum, NULL);

	if(FALSE == pproperty->bchoose){
	  element = getAudioDecElement(pproperty ,audionum);
		if( NULL == element ){
				element = getVideoDecElement(pproperty, videonum);
				if(NULL == element) {
					FSL_PLAYER_PRINT("%s(): Can not find element to get current position\n", __FUNCTION__); 
					return -1;
				}
    }
		pproperty->bchoose= TRUE;
	}
	g_object_get(element, "position", &currentposition, NULL);
  /* Nanosecond*/
	return currentposition;
}

static gint64 getcurpos(fsl_player_property* pproperty)
{
  #if 0
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position = 0;
  gst_element_query_position (pproperty->playbin, &fmt, &position);
	#else
	gint64 position = fsl_player_prop_currentposition(pproperty);
	#endif
  return position;
}

static gint
get_n_audio (GstElement *playbin)
{
  gint i;
  g_object_get (playbin, "n-audio", &i, NULL);
  return i;
}

set_passthrough(fsl_player_handle handle,fsl_player_s32 index)
{
    gint64 position = 0;
    GstElement * adec = NULL, *vdec = NULL;
    gint n_audio; 
    gint i;
    gint32 videonum = 0;
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    n_audio=get_n_audio (pproperty->playbin);
    for (i = 0; i < n_audio; i++) {
        adec = getAudioDecElement(pproperty,i);
        if (adec) {
          g_object_set(G_OBJECT(adec), "pass-through", TRUE, NULL);
          gst_object_unref(adec);
        }
    }
    adec = getAudioDecElement(pproperty,index);
    if (adec) {
        g_object_set(G_OBJECT(adec), "pass-through", FALSE, NULL);
        gst_object_unref(adec);
    }
    
    g_object_get(pproperty->playbin, "current-video", &videonum, NULL);
    vdec = getVideoDecElement(pproperty, videonum);
    if(vdec) {
      g_object_set(G_OBJECT(vdec), "pass-through", TRUE, NULL);
    }
    position=getcurpos(pproperty);
    fsl_player_seek(handle,position/MILLISECOND2NANOSECOND,1); 
}
fsl_player_ret_val fsl_player_select_audio_track(fsl_player_handle handle, fsl_player_s32 track_no)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_s32 current_audio_index = -1;
    fsl_player_s32 total_audio_number = 0;
    fsl_player_s32 total_stream_number = 0;
    GstTagList *tags;

    if( 2 == pproperty->config.playbin_version )
    {
        // playbin2, refer totem2.27 or later
        g_object_get( G_OBJECT(pproperty->playbin), "n-audio", &total_audio_number, NULL);
        FSL_PLAYER_PRINT("playbin2 is employed!\n");
    }
    else if( 1 == pproperty->config.playbin_version )
    {
        //playbin, refer totem2.26 or earlier
        g_object_get( G_OBJECT(pproperty->playbin), "nstreams", &total_stream_number, NULL);
        FSL_PLAYER_PRINT("playbin is employed!\n");
    }
    g_object_get( G_OBJECT(pproperty->playbin), "current-audio", &current_audio_index, NULL );
    FSL_PLAYER_PRINT( "total_stream_number = %d, total_audio_number = %d, current_audio_index = %d.\n", total_stream_number, total_audio_number, current_audio_index );

    if( track_no != current_audio_index )
    {
        g_object_set( pproperty->playbin, "current-audio", track_no, NULL );
        set_passthrough(handle,track_no);		
        g_object_get( G_OBJECT(pproperty->playbin), "current-audio", &current_audio_index, NULL );
        FSL_PLAYER_PRINT( "Current audio_index is %d after set current-audio.\n", current_audio_index );
        //g_signal_emit_by_name( G_OBJECT(pproperty->playbin), "get-audio-tags", track_no, &tags);
    }

    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);
    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_select_subtitle(fsl_player_handle handle, fsl_player_s32 subtitle_no)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_s32 current_subtitle_index = -1;
    fsl_player_s32 total_subtitle_number = 0;
    fsl_player_s32 total_stream_number = 0;
    GstTagList *tags;

    if( 2 == pproperty->config.playbin_version )
    {
        // playbin2, refer totem2.27 or later
        g_object_get( G_OBJECT(pproperty->playbin), "n-text", &total_subtitle_number, NULL);
        FSL_PLAYER_PRINT("playbin2 is employed!\n");
    }
    else if ( 1 == pproperty->config.playbin_version )
    {
        //playbin, refer totem2.26 or earlier
        g_object_get( G_OBJECT(pproperty->playbin), "nstreams", &total_stream_number, NULL);
        FSL_PLAYER_PRINT("playbin is employed!\n");
    }
    g_object_get( G_OBJECT(pproperty->playbin), "current-text", &current_subtitle_index, NULL);
    FSL_PLAYER_PRINT( "total_subtitle_number = %d, current_subtitle_index = %d.\n", total_subtitle_number, current_subtitle_index );

    if( subtitle_no != current_subtitle_index )
    {
        g_object_set( pproperty->playbin, "current-audio", subtitle_no, NULL );
        g_object_get( G_OBJECT(pproperty->playbin), "current-text", &current_subtitle_index, NULL );
        FSL_PLAYER_PRINT( "current_subtitle_index is %d after set current-text.\n", current_subtitle_index );
        //g_signal_emit_by_name( G_OBJECT(pproperty->playbin), "get-audio-tags", subtitle_no, &tags);
    }

    FSL_PLAYER_PRINT("%s()\n", __FUNCTION__);
    return FSL_PLAYER_SUCCESS;
}

static fsl_player_ret_val update_mfw_v4lsink_parameter(GstElement* mfw_v4lsink)
{
    fsl_player_s32 cnt = 0;
    fsl_player_s32 paraset;

    g_object_set(G_OBJECT(mfw_v4lsink), "setpara", 1, NULL);

  	g_object_get(G_OBJECT(mfw_v4lsink), "setpara", &paraset, NULL);
    while( (paraset) && cnt<100 )
    {
        usleep(20000);
        cnt++;
        g_object_get(G_OBJECT(mfw_v4lsink), "setpara", &paraset, NULL);
    }
    if( cnt >= 100 )
    {
        FSL_PLAYER_PRINT("%s(): Can not set separa to v4lsink!\n", __FUNCTION__);
        return FSL_PLAYER_FAILURE;
    }
    else
    {
        FSL_PLAYER_PRINT("%s(): Update mfw_v4lsink successfully!\n", __FUNCTION__);
    }
    return FSL_PLAYER_SUCCESS;
}
#if 1 
fsl_player_ret_val get_mfw_v4lsink(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
     //   FSL_PLAYER_PRINT("%s(): Can not find auto_video-sink\n", __FUNCTION__);    
        return FSL_PLAYER_FAILURE;
    }

    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
     //   FSL_PLAYER_PRINT("%s(): Can not find actual_video-sink\n", __FUNCTION__);
        return FSL_PLAYER_FAILURE;
    }
    FSL_PLAYER_PRINT("%s(): AutoVideoSink = %s : ActualVideoSink = %s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));
    pproperty->video_sink = actual_video_sink;

    g_object_unref (actual_video_sink);
    g_object_unref (auto_video_sink);

    return FSL_PLAYER_SUCCESS;
}
#endif
fsl_player_bool fullscreen_fb0_open(fsl_player_s32 *pfb)
{
#define FB_DEIVCE "/dev/fb0"
    fsl_player_bool retval = FSL_PLAYER_TRUE;
    fsl_player_s8 fb_device[100] = FB_DEIVCE;

	if((*pfb = open(fb_device, O_RDWR, 0)) < 0)
    {
	    FSL_PLAYER_PRINT("Unable to open %s %d\n", fb_device, *pfb);
        *pfb = 0;
	    retval = FSL_PLAYER_FALSE;
    }
    return retval;
}

fsl_player_bool fullscreen_fb0_close(fsl_player_s32 *pfb)
{
    fsl_player_bool retval = FSL_PLAYER_TRUE;
    if(*pfb)
    {
        close(*pfb);
        *pfb = 0;
    }
    return retval;
}

void fullscreen_fb0_get_width_height(fsl_player_s32 fb, fsl_player_s32* pfullscreen_width, fsl_player_s32* pfullscreen_height)
{
    struct fb_var_screeninfo scrinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &scrinfo) < 0)
    {
        FSL_PLAYER_PRINT("Get var of fb0 failed\n");
        return;
    }
    *pfullscreen_width = scrinfo.xres;
    *pfullscreen_height = scrinfo.yres;
    return;
}
#if 0
fsl_player_ret_val fsl_player_full_screen(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    fsl_player_s32 fullscreen_width = 0;
    fsl_player_s32 fullscreen_height = 0;
    fsl_player_s32 fb = 0;

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);    
        return FSL_PLAYER_FAILURE;
    }
 #if 1	
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return FSL_PLAYER_FAILURE;
    }
#endif 	
    FSL_PLAYER_PRINT("%s(): AutoVideoSink=%s : ActualVideoSink=%s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));    

    if( pproperty->bfullscreen )
    {
        g_object_set(G_OBJECT(actual_video_sink), "axis-left", pproperty->display_parameter.offsetx, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "axis-top", pproperty->display_parameter.offsety, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "disp-width", pproperty->display_parameter.disp_width, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "disp-height", pproperty->display_parameter.disp_height, NULL);
        pproperty->bfullscreen = 0;
    }
    else
    {
        g_object_get(G_OBJECT(actual_video_sink), "axis-left", &(pproperty->display_parameter.offsetx), NULL);
        g_object_get(G_OBJECT(actual_video_sink), "axis-top", &(pproperty->display_parameter.offsety), NULL);
        g_object_get(G_OBJECT(actual_video_sink), "disp-width", &(pproperty->display_parameter.disp_width), NULL);
        g_object_get(G_OBJECT(actual_video_sink), "disp-height", &(pproperty->display_parameter.disp_height), NULL);

        
        //g_object_get(G_OBJECT(actual_video_sink), "fullscreen-width", &(fullscreen_width), NULL);
        //g_object_get(G_OBJECT(actual_video_sink), "fullscreen-height", &(fullscreen_height), NULL);
        fullscreen_fb0_open( &fb );
        fullscreen_fb0_get_width_height( fb, &(fullscreen_width),  &(fullscreen_height) );
        fullscreen_fb0_close( &fb );

        g_object_set(G_OBJECT(actual_video_sink), "axis-left", 0, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "axis-top", 0, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "disp-width", fullscreen_width, NULL);
        g_object_set(G_OBJECT(actual_video_sink), "disp-height", fullscreen_height, NULL);
        pproperty->bfullscreen = 1;
    }

    update_mfw_v4lsink_parameter(actual_video_sink);

    g_object_unref (actual_video_sink);
    g_object_unref (auto_video_sink);
    return FSL_PLAYER_SUCCESS;
}
#endif
#if 0
fsl_player_ret_val fsl_player_display_screen_mode(fsl_player_handle handle, fsl_player_s32 mode)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    // TODO. Implement this module later.

    return FSL_PLAYER_SUCCESS;
}
#endif

#if 0
fsl_player_ret_val fsl_player_resize(fsl_player_handle handle, fsl_player_display_parameter display_parameter)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return FSL_PLAYER_FAILURE;
    }
#if 1	
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return FSL_PLAYER_FAILURE;
    }
#endif 	
    FSL_PLAYER_PRINT("%s(): AutoVideoSink=%s : ActualVideoSink=%s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));

    pproperty->display_parameter.offsetx = display_parameter.offsetx;
    pproperty->display_parameter.offsety = display_parameter.offsety;
    pproperty->display_parameter.disp_width = display_parameter.disp_width;
    pproperty->display_parameter.disp_height = display_parameter.disp_height;
    g_object_set(G_OBJECT(actual_video_sink), "axis-left", pproperty->display_parameter.offsetx, NULL);
    g_object_set(G_OBJECT(actual_video_sink), "axis-top", pproperty->display_parameter.offsety, NULL);
    g_object_set(G_OBJECT(actual_video_sink), "disp-width", pproperty->display_parameter.disp_width, NULL);
    g_object_set(G_OBJECT(actual_video_sink), "disp-height", pproperty->display_parameter.disp_height, NULL);

    update_mfw_v4lsink_parameter(actual_video_sink);
    
    g_object_unref (actual_video_sink);
    g_object_unref (auto_video_sink);

    return FSL_PLAYER_SUCCESS;
}
#endif
#if 0
fsl_player_ret_val fsl_player_rotate(fsl_player_handle handle, fsl_player_rotation rotate_value)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    GList* list = NULL;
    GList* pfirst = NULL;

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return FSL_PLAYER_FAILURE;
    }

/*    //g_object_get(auto_video_sink, "videosink-actual-sink-mfw_v4l", &actual_video_sink, NULL);
    FSL_PLAYER_PRINT("The number of children in auto_video_sink is %d\n", GST_BIN_NUMCHILDREN((GstBin*)auto_video_sink));
    list = GST_BIN_CHILDREN((GstBin*)auto_video_sink);
    //list = gst_bin_get_list((GstBin*)auto_video_sink);
    pfirst = g_list_first(list);
    actual_video_sink = (GstElement*)(pfirst->data);
    FSL_PLAYER_PRINT(" CHILDREN Name:%s\n", gst_element_get_name((GstElement*)(pfirst->data)));*/
  #if 1 
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);
        return FSL_PLAYER_FAILURE;
    }
#endif 	
    FSL_PLAYER_PRINT("%s(): AutVideoSinko=%s : ActualVideoSink=%s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));

    FSL_PLAYER_PRINT("rotate_value = %d\n", rotate_value);
    g_object_set(G_OBJECT(actual_video_sink), "rotate", (int)rotate_value, NULL);

    update_mfw_v4lsink_parameter(actual_video_sink);

    pproperty->rotate_value = rotate_value;

    g_object_get(G_OBJECT(actual_video_sink), "rotate", &(rotate_value), NULL);
    FSL_PLAYER_PRINT("%s(): After rotate_value == %d\n", __FUNCTION__, rotate_value);

    g_object_unref (actual_video_sink);
    g_object_unref (auto_video_sink);

    return FSL_PLAYER_SUCCESS;
}
#endif

fsl_player_ret_val fsl_player_get_property(fsl_player_handle handle, fsl_player_property_id property_id, void* pstructure)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_ret_val ret_val = FSL_PLAYER_SUCCESS;

    switch( property_id )
    {
        case FSL_PLAYER_PROPERTY_DURATION:
        {
            if(TRUE == pproperty->bts) {
							GstElement* filesource = NULL;
	            guint64 duration;
	            filesource = gst_bin_get_by_name((GstBin*)pproperty->playbin, "source");
	            if(NULL != filesource) {
	              g_object_get(filesource, "VBRduration", &duration, NULL);
	              *((fsl_player_u64*)pstructure) = duration;
								pproperty->duration = duration;
	            }
	            else {
	              ret_val = FSL_PLAYER_FAILURE;
	            }
						}
						else {
							if(TRUE == pproperty->bape) {
								GstElement* element = NULL;
								gint64 duration;
								element = getAudioDecElement(pproperty ,0);
								if(NULL != element) {
									g_object_get(element, "apeduration", &duration, NULL);
		              *((fsl_player_u64*)pstructure) = duration;
									pproperty->duration = duration;
								}
								else {
		              ret_val = FSL_PLAYER_FAILURE;
		            }
							}
							else {
								gboolean bquery;
		            GstFormat fmt = GST_FORMAT_TIME;
		            bquery = gst_element_query_duration(pproperty->playbin, &fmt, (gint64 *)&(pproperty->duration));
		            if( FSL_PLAYER_TRUE == bquery )
		            {
		                *((fsl_player_u64*)pstructure) = pproperty->duration;
		            }
		            else
		            {
		                *((fsl_player_u64*)pstructure) = 0;
		                ret_val = FSL_PLAYER_FAILURE;
		            }
							}
						}
				
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED:
        {
            #if 0
            gboolean bquery;
            GstFormat fmt = GST_FORMAT_TIME;
            bquery = gst_element_query_position(pproperty->playbin, &fmt, (gint64 *)&(pproperty->elapsed));
            if (bquery==TRUE){
                *((fsl_player_u64*)pstructure) = pproperty->elapsed;
            }else{
                ret_val = FSL_PLAYER_FAILURE;
            }
						#else
						gint64 currentposition = fsl_player_prop_currentposition(pproperty);
						if(-1 == currentposition) {
							ret_val = FSL_PLAYER_FAILURE;
						}
						else {
							*((fsl_player_u64*)pstructure) = currentposition;
							pproperty->elapsed = currentposition;
						}
						#endif
            break;
        }
        case FSL_PLAYER_PROPERTY_PLAYER_STATE:
        {
            *((fsl_player_state*)pstructure) = pproperty->player_state;
            break;
        }
        case FSL_PLAYER_PROPERTY_PLAYBACK_RATE:
        {
            *((double*)pstructure) = pproperty->playback_rate;
            break;
        }
        case FSL_PLAYER_PROPERTY_MUTE:
        {
            *((fsl_player_bool*)pstructure) = pproperty->bmute;
            break;
        }
        case FSL_PLAYER_PROPERTY_VOLUME:
        {
            *((double*)pstructure) = pproperty->volume;
            break;
        }
        case FSL_PLAYER_PROPERTY_METADATA:
        {
            memcpy((fsl_player_s8*)pstructure, (fsl_player_s8*)(&(pproperty->metadata)), sizeof(fsl_player_metadata));
            break;
        }
        case FSL_PLAYER_PROPERTY_VERSION:
        {
            *((fsl_player_s8**)pstructure) = (fsl_player_s8*)FSL_PLAYER_VERSION_STR;
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_VIDEO_NO:
        {
            if (pproperty->config.playbin_version==2)
                g_object_get( G_OBJECT(pproperty->playbin), "n-video", (fsl_player_s32*)pstructure, NULL);
            else
                ret_val = FSL_PLAYER_ERROR_NOT_SUPPORT;
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_AUDIO_NO:
        {
            if (pproperty->config.playbin_version==2)
                g_object_get( G_OBJECT(pproperty->playbin), "n-audio", (fsl_player_s32*)pstructure, NULL);
            else
                ret_val = FSL_PLAYER_ERROR_NOT_SUPPORT;
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_SUBTITLE_NO:
        {
            if (pproperty->config.playbin_version==2)
                g_object_get( G_OBJECT(pproperty->playbin), "n-text", (fsl_player_s32*)pstructure, NULL);
            else
                ret_val = FSL_PLAYER_ERROR_NOT_SUPPORT;
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED_VIDEO:
        {
            GstElement* auto_video_sink = NULL;
            GstFormat fmt = GST_FORMAT_TIME;
            g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
            if( NULL == auto_video_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
                return FSL_PLAYER_FAILURE;
            }
            gst_element_query_position(auto_video_sink, &fmt, (gint64 *)&(pproperty->elapsed_video));
            *((fsl_player_u64*)pstructure) = pproperty->elapsed_video;
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED_AUDIO:
        {
            GstElement* auto_audio_sink = NULL;
            GstFormat fmt = GST_FORMAT_TIME;
            g_object_get(pproperty->playbin, "audio-sink", &auto_audio_sink, NULL);
            if( NULL == auto_audio_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find auto_audio_sink\n", __FUNCTION__);
                return FSL_PLAYER_FAILURE;
            }
            gst_element_query_position(auto_audio_sink, &fmt, (gint64 *)&(pproperty->elapsed_audio));
            *((fsl_player_u64*)pstructure) = pproperty->elapsed_audio;

            if (auto_audio_sink){
              g_object_unref(auto_audio_sink);
            }
            break;
        }
        case FSL_PLAYER_PROPERTY_DISP_PARA:
        {
            GstElement* auto_video_sink = NULL;
            GstElement* actual_video_sink = NULL;

            g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
            if( NULL == auto_video_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
                return FSL_PLAYER_FAILURE;
            }
            actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
            if( NULL == actual_video_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
                return FSL_PLAYER_FAILURE;
            }
            FSL_PLAYER_PRINT("%s(): AutoVideoSink=%s : ActualVideoSink=%s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));
            g_object_get(G_OBJECT(actual_video_sink), "axis-left", &(pproperty->display_parameter.offsetx), NULL);
            g_object_get(G_OBJECT(actual_video_sink), "axis-top", &(pproperty->display_parameter.offsety), NULL);
            g_object_get(G_OBJECT(actual_video_sink), "disp-width", &(pproperty->display_parameter.disp_width), NULL);
            g_object_get(G_OBJECT(actual_video_sink), "disp-height", &(pproperty->display_parameter.disp_height), NULL);
            
            g_object_unref (actual_video_sink);
            g_object_unref (auto_video_sink);

            ((fsl_player_display_parameter*)pstructure)->offsetx = pproperty->display_parameter.offsetx;
            ((fsl_player_display_parameter*)pstructure)->offsety = pproperty->display_parameter.offsety;
            ((fsl_player_display_parameter*)pstructure)->disp_width = pproperty->display_parameter.disp_width;
            ((fsl_player_display_parameter*)pstructure)->disp_height = pproperty->display_parameter.disp_height;
            
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_FRAMES:
        {
            GstElement* auto_video_sink = NULL;
            GstElement* actual_video_sink = NULL;

            g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
            if( NULL == auto_video_sink )
            {
            //    FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
                return FSL_PLAYER_FAILURE;
            }
            if (GST_IS_BIN(auto_video_sink)){
              actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
              if( NULL == actual_video_sink )
              {
              //    FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
                  return FSL_PLAYER_FAILURE;
              }
            }else{
              actual_video_sink = auto_video_sink;
              auto_video_sink = NULL;
            }

            if(g_object_class_find_property(G_OBJECT_GET_CLASS(actual_video_sink), "rendered"))
              g_object_get(G_OBJECT(actual_video_sink), "rendered", &(pproperty->total_frames), NULL);
             *((fsl_player_u32*)pstructure) = pproperty->total_frames;

            if (actual_video_sink){
              g_object_unref(actual_video_sink);
            }

            if (auto_video_sink){
              g_object_unref(auto_video_sink);
            }
            
            break;
        }
        
        case FSL_PLAYER_PROPERTY_SEEKABLE:
        {
            GstQuery *query;
            gboolean res;
            *((fsl_player_bool *)pstructure) = FALSE;

            query = gst_query_new_seeking (GST_FORMAT_TIME);
            if (gst_element_query (pproperty->playbin, query)) {
              gst_query_parse_seeking (query, NULL, &res, NULL, NULL);
              if (res){
                  *((fsl_player_bool *)pstructure) = TRUE;  
              }
            }
            gst_query_unref (query);
            break;
        }
        default:
        {
            ret_val = FSL_PLAYER_ERROR_NOT_SUPPORT;
            break;
        }
    }
    return ret_val;
}

fsl_player_ret_val fsl_player_set_property(fsl_player_handle handle, fsl_player_property_id property_id, void* pstructure)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_ret_val ret_val = FSL_PLAYER_SUCCESS;

    switch( property_id )
    {
        case FSL_PLAYER_PROPERTY_DURATION:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_PLAYER_STATE:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_PLAYBACK_RATE:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_MUTE:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_VOLUME:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_METADATA:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_VERSION:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_VIDEO_NO:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_AUDIO_NO:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_TOTAL_SUBTITLE_NO:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED_VIDEO:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_ELAPSED_AUDIO:
        {
            break;
        }
        case FSL_PLAYER_PROPERTY_DISP_PARA:
        {
            GstElement* auto_video_sink = NULL;
            GstElement* actual_video_sink = NULL;

            g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
            if( NULL == auto_video_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
                return FSL_PLAYER_FAILURE;
            }
            actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
            if( NULL == actual_video_sink )
            {
                FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
                return FSL_PLAYER_FAILURE;
            }
            FSL_PLAYER_PRINT("%s(): AutoVideoSink=%s : ActualVideoSink=%s\n", __FUNCTION__, GST_OBJECT_NAME(auto_video_sink), GST_OBJECT_NAME(actual_video_sink));
            g_object_set(G_OBJECT(actual_video_sink), "axis-left", ((fsl_player_display_parameter*)pstructure)->offsetx, NULL);
            g_object_set(G_OBJECT(actual_video_sink), "axis-top", ((fsl_player_display_parameter*)pstructure)->offsety, NULL);
            g_object_set(G_OBJECT(actual_video_sink), "disp-width", ((fsl_player_display_parameter*)pstructure)->disp_width, NULL);
            g_object_set(G_OBJECT(actual_video_sink), "disp-height", ((fsl_player_display_parameter*)pstructure)->disp_height, NULL);
            
            g_object_unref (actual_video_sink);
            g_object_unref (auto_video_sink);

            break;
        }
        default:
        {
            ret_val = FSL_PLAYER_ERROR_NOT_SUPPORT;
            break;
        }
    }

    return ret_val;
}

fsl_player_ret_val fsl_player_wait_message(fsl_player_handle handle, fsl_player_ui_msg** msg, fsl_player_s32 timeout)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_ret_val ret_val = FSL_PLAYER_SUCCESS;
    fsl_player_ui_msg *ui_msg = NULL;

    if( NULL == msg )
    {
        return FSL_PLAYER_FAILURE;
    }

    ret_val = pproperty->queue.klass->get( &(pproperty->queue), msg, timeout );
    if( FSL_PLAYER_SUCCESS == ret_val )
    {

    }

    return ret_val;
}

fsl_player_ret_val fsl_player_send_message_exit(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    fsl_player_ui_msg* pui_msg = NULL;

    pui_msg = fsl_player_ui_msg_new_empty(FSL_PLAYER_UI_MSG_EXIT);
    if( NULL == pui_msg )
    {
        FSL_PLAYER_PRINT("EOS Message is not sending out.\n");
        return FSL_PLAYER_FAILURE;
    }
    pproperty->queue.klass->put( &(pproperty->queue), (void*)/*&*/pui_msg, FSL_PLAYER_FOREVER, FSL_PLAYER_UI_MSG_PUT_NEW);

    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_exit_message_loop(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    pproperty->queue.klass->flush( &(pproperty->queue) );

    return FSL_PLAYER_SUCCESS;
}

fsl_player_ret_val fsl_player_post_eos_semaphore(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    FSL_PLAYER_SEMA_POST( &(pproperty->eos_semaphore) );

    return FSL_PLAYER_SUCCESS;
}

fsl_player_bool fsl_player_prop_options()
{
    char option[64] = {0};
    FSL_PLAYER_PRINT("Select options:['get' or 'set'] (default option is get):");
    scanf("%s", &option);
    option[63] = '\0';
    if(!strncmp(option, "set", strlen("set"))){
        return TRUE;
    }
    return FALSE;
}

static int fsl_player_prop_trickrate(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    GValue value = {0,};
    gdouble rate = 1.0;
    fsl_player_bool bset = FALSE;
    
    bset = fsl_player_prop_options();
    
    element = gst_bin_get_by_name_recurse_up(GST_BIN(pproperty->playbin), "amlvdec0");
    g_print("get amlvdec point=%p\n", element);
    g_value_init (&value, G_TYPE_DOUBLE);
    if(bset){
        FSL_PLAYER_PRINT("Input rate:");
        scanf("%lf", &rate);
        g_value_set_double (&value, rate);
        g_object_set_property(G_OBJECT(element), "trickrate", &value);
    }
    else{
        g_object_get_property(G_OBJECT(element), "trickrate", &value);
        rate = g_value_get_double (&value);
        FSL_PLAYER_PRINT("current trickrate is:%lf\n", rate);
    }
    g_value_unset (&value);

    return 0;
}

static int fsl_player_prop_interlaced(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    GValue value = {0,};
    gboolean interlaced = FALSE;

    element = gst_bin_get_by_name(GST_BIN(pproperty->playbin), "amlvdec0");
    g_value_init (&value, G_TYPE_BOOLEAN);
    g_object_get_property(G_OBJECT(element), "interlaced", &value);
    interlaced = g_value_get_boolean(&value);
    FSL_PLAYER_PRINT("frame interlaced is:%d\n", interlaced);
    g_value_unset (&value);

    return 0;
}

static int fsl_player_prop_currentPTS(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    GstElement * element = NULL;
    GValue value = {0,};
    gint64 currentPTS = 0;
    

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return -1;
    }
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return -1;
    }

    //element = gst_bin_get_by_name_recurse_up(GST_BIN(pproperty->playbin), "amlvsink0");
    //g_print("get amlvdec point=%p\n", element);
    g_value_init (&value, G_TYPE_INT64);
    g_object_get_property(G_OBJECT(actual_video_sink), "currentPTS", &value);
    currentPTS = g_value_get_int64 (&value);
    g_value_unset (&value);
    FSL_PLAYER_PRINT("current PTS is:%d\n", currentPTS);

    return 0;
}
static int fsl_player_prop_contentframerate(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    static GstElement* element = NULL;
    gint32 videonum = 0;
    gint32 fps;
    g_object_get(pproperty->playbin, "current-video", &videonum, NULL);
    element = getVideoDecElement(pproperty, videonum);
    if(NULL == element) {
	FSL_PLAYER_PRINT("%s(): Can not find element to get current position\n", __FUNCTION__); 
	return -1;
    }

    g_object_get(element, "contentFrameRate", &fps, NULL);
    printf("content framerate:%dfps\n", fps);
    return 0;
}

static int fsl_player_prop_flushRepeatFrame(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    GValue value = {0,};
    gboolean isRepeatFrame = FALSE;
    fsl_player_bool bset = FALSE;
    
    bset = fsl_player_prop_options();
    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return -1;
    }
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return -1;
    }
    g_value_init (&value, G_TYPE_BOOLEAN);
    if(bset){
        FSL_PLAYER_PRINT("Input flushRepeatFrame flag:");
        scanf("%d", &isRepeatFrame);
        g_value_set_boolean (&value, isRepeatFrame);
        g_object_set_property(G_OBJECT(actual_video_sink), "flushRepeatFrame", &value);
    }
    else{
        char *flag = "TRUE";
        g_object_get_property(G_OBJECT(actual_video_sink), "flushRepeatFrame", &value);
        isRepeatFrame = g_value_get_boolean (&value);
        flag = isRepeatFrame ? "TRUE" : "FALSE";
        FSL_PLAYER_PRINT("RepeatFrame is:%s\n", flag);
    }
    
    g_value_unset (&value);
    
    return 0;
}

static int fsl_player_prop_pmt_info(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    gpointer pmtinfo;
    guint16 program = 0;
    guint8 version = 0;
    guint16 pcr_pid = 0;
    GValueArray *streaminfos;
    GValueArray *descriptors;
    int i = 0;
    
    element = gst_bin_get_by_name(GST_BIN(pproperty->playbin), "flutsdemux0");
    //g_object_get(G_OBJECT(pproperty->playbin), "pmt-info", &pmtinfo, NULL);
    g_object_get (element, "pmt-info", &pmtinfo, NULL);
    g_object_get (pmtinfo, "program-number", &program, NULL);
    g_object_get (pmtinfo, "version-number", &version, NULL);
    g_object_get (pmtinfo, "pcr-pid", &pcr_pid, NULL);

    g_object_get (pmtinfo, "stream-info", &streaminfos, NULL);
    g_object_get (pmtinfo, "descriptors", &descriptors, NULL);

    FSL_PLAYER_PRINT ("PMT: program: %04x" \
        " version: %d " \
        "pcr: %04x "    \
        "streams: %d "  \
        "descriptors: %d\n",
        (guint16)program, 
        version, 
        (guint16)pcr_pid, 
        streaminfos->n_values,
        descriptors->n_values);
    
    //dump_descriptors (descriptors);
    
    for (i = 0 ; i < streaminfos->n_values; i++) {
        GValue *value;
        guint16 es_pid;
        guint8 es_type;
        GValueArray *languages;
        GValueArray *streaminfo;
        GValueArray *descriptor;
        value = g_value_array_get_nth (streaminfos, i);
        streaminfo = (GObject*) g_value_get_object (value);
        g_object_get (streaminfo, "pid", &es_pid, NULL);
        g_object_get (streaminfo, "stream-type", &es_type, NULL);
        g_object_get (streaminfo, "languages", &languages, NULL);
        g_object_get (streaminfo, "descriptors", &descriptor, NULL);
        FSL_PLAYER_PRINT ("pid: %04x type: %x languages: %d descriptors: %d\n",
            (guint16)es_pid, (guint8) es_type, languages->n_values,
            descriptor->n_values);
        
        //dump_languages (languages);
        //dump_descriptors (descriptor);
    } 

    return 0;
}
#define MAX_BIT 5
gint parse_str2int(const gchar *input, const char boundary, guint *first, ...)
{
    va_list var_args;
    guint *name = NULL;
    const gchar *p = input;
    gint i = 0;
    gint index = 0;
    gint size = 0;
    gchar value[MAX_BIT]= {0};

    if(NULL == p){
        FSL_PLAYER_PRINT("[%s:%d] null\n", __FUNCTION__, __LINE__);
        return -1;
    }
    size = strlen(p);
    va_start (var_args, first);
    name = first;
    while(name && (i <= size)){
        value[index] = p[i];
        if(index++ >= MAX_BIT){
            va_end (var_args);
             FSL_PLAYER_PRINT("[%s:%d] error\n", __FUNCTION__, __LINE__);
            return -1;
        }
        if(p[i+1] == boundary){
            index = 0;
            *name = atoi(value);
            memset(value, 0, sizeof(value));
            i++;
            name = va_arg (var_args, guint*);
        }
        else if(p[i+1] == '\0'){
            *name = atoi(value);
            break;
        }
        i++;
        
    }
    va_end (var_args);
    return 0;
}

static int fsl_player_prop_rectangle_info(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    GValue value = {0,};
    gpointer rectangleinfo;
    guint x = 0;
    guint y = 0;
    guint width = 0;
    guint height = 0;
    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    fsl_player_bool bset = FALSE;
    
    bset = fsl_player_prop_options();

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return -1;
    }
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return -1;
    }

    g_object_get (G_OBJECT(actual_video_sink), "rectangle", &rectangleinfo, NULL);
    if(!bset){
        g_value_init (&value, G_TYPE_UINT);
        g_object_get_property (rectangleinfo, "x", &value);
        x = g_value_get_uint (&value);
        g_value_unset (&value);
        
        g_value_init (&value, G_TYPE_UINT);
        g_object_get_property (rectangleinfo, "y", &value);
        y = g_value_get_uint (&value);
        g_value_unset (&value);

        g_value_init (&value, G_TYPE_UINT);
        g_object_get_property (rectangleinfo, "width", &value);
        width = g_value_get_uint (&value);
        g_value_unset (&value);

        g_value_init (&value, G_TYPE_UINT);
        g_object_get_property (rectangleinfo, "height", &value);
        height = g_value_get_uint (&value);
        g_value_unset (&value);
    }
    else{
        char param[64] = {0};
        FSL_PLAYER_PRINT("input parameters:");
        scanf("%s", &param);
        param[63] = '\0';

        if(parse_str2int(param, ',', &x, &y, &width, &height) < 0){
            return -1;
        }
        g_value_init (&value, G_TYPE_UINT);
        g_value_set_uint(&value, x);
        g_object_set_property (G_OBJECT(rectangleinfo), "x", &value);
        g_value_unset (&value);
        
        g_value_init (&value, G_TYPE_UINT);
        g_value_set_uint(&value, x);
        g_object_set_property (G_OBJECT(rectangleinfo), "y", &value);
        g_value_unset (&value);

        g_value_init (&value, G_TYPE_UINT);
        g_value_set_uint(&value, width);
        g_object_set_property (G_OBJECT(rectangleinfo), "width", &value);
        g_value_unset (&value);

        g_value_init (&value, G_TYPE_UINT);
        g_value_set_uint(&value, height);
        g_object_set_property (G_OBJECT(rectangleinfo), "height", &value);
        g_value_unset (&value);

        g_object_set (G_OBJECT(actual_video_sink), "rectangle", G_OBJECT(rectangleinfo), NULL);
    }

    FSL_PLAYER_PRINT("Rectangle: x: %d y: %d width: %d height: %d\n", x, y, width, height);
    return 0;
}

static int fsl_player_prop_tvmode(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    GValue value = {0,};

    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    fsl_player_bool bset = FALSE;
    guint mode = 0;
    
    bset = fsl_player_prop_options();

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return -1;
    }
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return -1;
    }

    g_value_init (&value, G_TYPE_UINT);
    if(bset){
        FSL_PLAYER_PRINT("Input TV Mode:");
        scanf("%d", &mode);
        g_value_set_uint (&value, mode);
        g_object_set_property(G_OBJECT(actual_video_sink), "tvmode", &value);
    }
    else{
        const char *wide_str[] = {"normal", "full stretch", "4-3", "16-9", "non-linear", "normal-noscaleup"};
        g_object_get_property(G_OBJECT(actual_video_sink), "tvmode", &value);
        mode = g_value_get_uint (&value);
        if(mode < sizeof(wide_str) / sizeof(wide_str[0]) && mode >= 0){
            FSL_PLAYER_PRINT("current TV Mode is:%d:%s\n", mode, wide_str[mode]);
        }
        else{
            FSL_PLAYER_PRINT("current TV Mode:%d is error\n", mode);
        }
    }
    g_value_unset (&value);
    return 0;
}

static int fsl_player_prop_video_mute(fsl_player_handle handle)
{
    fsl_player* pplayer = (fsl_player*)handle;
    fsl_player_property* pproperty = (fsl_player_property*)pplayer->property_handle;
    GstElement * element = NULL;
    GValue value = {0,};

    GstElement* auto_video_sink = NULL;
    GstElement* actual_video_sink = NULL;
    gboolean mute = FALSE;
    int v = 0;

    g_object_get(pproperty->playbin, "video-sink", &auto_video_sink, NULL);
    if( NULL == auto_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find auto_video_sink\n", __FUNCTION__);
        return -1;
    }
    actual_video_sink = gst_bin_get_by_name((GstBin*)auto_video_sink, "videosink-actual-sink-amlv");
    if( NULL == actual_video_sink )
    {
        FSL_PLAYER_PRINT("%s(): Can not find actual_video_sink\n", __FUNCTION__);    
        return -1;
    }

    g_value_init (&value, G_TYPE_BOOLEAN);
    FSL_PLAYER_PRINT("Input video mute set: {0 is display, 1 is disable display}");
    scanf("%d", &v);
    mute = v ? TRUE : FALSE;
    g_value_set_boolean (&value, mute);
    g_object_set_property(G_OBJECT(actual_video_sink), "mute", &value);
    g_value_unset (&value);
    return 0;
}

static const PropType property_pool[] = {
    {"trickrate", fsl_player_prop_trickrate},
    {"interlaced", fsl_player_prop_interlaced},
    {"currentPTS", fsl_player_prop_currentPTS},
    {"flushRepeatFrame", fsl_player_prop_flushRepeatFrame},
    {"rectangle", fsl_player_prop_rectangle_info},
    {"tvmode", fsl_player_prop_tvmode},
    {"mute", fsl_player_prop_video_mute},
    {"pmt-info", fsl_player_prop_pmt_info},
    {"contentFrameRate", fsl_player_prop_contentframerate},
    {NULL, NULL},
};

fsl_player_ret_val fsl_player_property_process(fsl_player_handle handle, char *prop_name)
{
    fsl_player* pplayer = (fsl_player*)handle;

    PropType * p = property_pool;
    while(p->name){
        if(!strcmp(p->name, prop_name)){
            if(p->property_func(handle) < 0){
                return FSL_PLAYER_ERROR_BAD_PARAM;
            }
        }
        p++;
    }
    return FSL_PLAYER_SUCCESS;
}

