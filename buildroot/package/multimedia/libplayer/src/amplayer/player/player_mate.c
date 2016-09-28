/********************************************
 * name : player_control.c
 * function: player's mate thread
 * date     : 2011.10.11
 * Please don't call any ffmpeg function in this thread.
 * recevice upper level cmd and get lowlevel status.
 * Send player infos to uperlevel;
 ********************************************/
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <player.h>

#include "player_priv.h"
#include "thread_mgt.h"
#include <semaphore.h>

#define mate_print(fmt,args...)     log_debug(fmt,##args)
//#define mate_print(fmt...)

struct player_mate {
    pthread_mutex_t  pthread_mutex;
    pthread_cond_t   pthread_cond;
    pthread_t        pthread_id;
    player_status    player_state;
    sem_t            mate_sem;
    int              work_intervals;
    int              wake_delay;
    int              mate_isrunng;
    int              mate_should_sleep;
    int              mate_should_exit;
};
static int player_mate_thread_run(play_para_t *player);


void *player_mate_init(play_para_t *player, int intervals)
{
    pthread_t       tid;
    pthread_attr_t pthread_attr;
    struct player_mate *mate;
    int ret;
    player->player_mate = NULL;
    mate = MALLOC(sizeof(struct player_mate));
    if (!mate) {
        return NULL;
    }
    pthread_attr_init(&pthread_attr);
    pthread_attr_setstacksize(&pthread_attr, 409600);   //default joinable
    pthread_mutex_init(&mate->pthread_mutex, NULL);
    pthread_cond_init(&mate->pthread_cond, NULL);
    ret = sem_init(&mate->mate_sem,   0,   0);
    mate->mate_isrunng = 0;
    mate->mate_should_sleep = 0;
    mate->mate_should_exit = 0;
    mate->work_intervals = intervals;
    log_print("player mate init ok mate=%x(%d)\n", mate, sizeof(struct player_mate));
    player->player_mate = mate;
    ret = pthread_create(&tid, &pthread_attr, (void*)&player_mate_thread_run, (void*)player);
    //pthread_setname_np(tid,"AmplayerMate");
    mate->pthread_id = tid;
    return (void *)mate;
}

int player_mate_wake(play_para_t *player, int delay)
{
    int ret;
    struct player_mate *mate = (struct player_mate *)(player->player_mate);
    if (!mate) {
        return -1;
    }
    if (delay > 0) {
        mate->wake_delay = delay;
    } else {
        mate->wake_delay = 0;
    }
    mate->mate_should_sleep = 0;
    ret = sem_post(&mate->mate_sem);/*wake wate thread*/
    return ret;
}
int player_mate_sleep(play_para_t *player)
{
    struct player_mate *mate = (struct player_mate *)(player->player_mate);
    int ret = 0;
    if (!mate) {
        return -1;
    }
    if(player->playctrl_info.temp_interrupt_ffmpeg){
	     player->playctrl_info.temp_interrupt_ffmpeg=0;	
	     log_print("ffmpeg_uninterrupt tmped by player mate!\n");	 
            ffmpeg_uninterrupt(player->thread_mgt.pthread_id);
    }
    mate->mate_should_sleep = 1;	 
    while (mate->mate_isrunng) {
        pthread_mutex_lock(&mate->pthread_mutex);
        ret = pthread_cond_signal(&mate->pthread_cond);
        pthread_mutex_unlock(&mate->pthread_mutex);
        if (mate->mate_isrunng) {
            player_thread_wait(player, 1000);
        }
    }
    return ret;
}
int player_mate_release(play_para_t *player)
{
    struct player_mate *mate = (struct player_mate *)(player->player_mate);
    int ret;
    if (!mate) {
        return -1;
    }
    mate_print("try release mate thread now\n");
    pthread_mutex_lock(&mate->pthread_mutex);
    mate->mate_should_sleep = 1;
    mate->mate_should_exit = 1;
    mate->wake_delay = 0;
    ret = sem_post(&mate->mate_sem);/*wake mate thread*/
    ret = sem_post(&mate->mate_sem);/*wake mate thread*/
    ret = sem_post(&mate->mate_sem);/*wake mate thread*/
    ret = pthread_cond_signal(&mate->pthread_cond);
    pthread_mutex_unlock(&mate->pthread_mutex);
    mate_print("wait mate thread exit\n");
    ret = pthread_join(mate->pthread_id, NULL);
    mate_print("mate thread exited\n");
    FREE(player->player_mate);
    player->player_mate = NULL;
    return ret;
}

static int player_mate_thread_wait(struct player_mate *mate, int microseconds)
{
    struct timespec pthread_ts;
    struct timeval now;
    int ret;

    gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (microseconds + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((microseconds + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&mate->pthread_mutex);
    ret = pthread_cond_timedwait(&mate->pthread_cond, &mate->pthread_mutex, &pthread_ts);
    pthread_mutex_unlock(&mate->pthread_mutex);
    return ret;
}


static int player_mate_thread_cmd_proxy(play_para_t *player, struct player_mate *mate)
{
    player_cmd_t *cmd = NULL;
    int ret;
    play_para_t *p_para=player;
    /*
    check the cmd & do for main thread;
    */
      if (p_para->oldcmd.ctrl_cmd == CMD_SEARCH &&
        nextcmd_is_cmd(p_para, CMD_SEARCH) &&
        ((p_para->oldcmdtime >= player_get_systemtime_ms() - 400)) && /*lastcmd is not too old.*/
        ((p_para->stream_type == STREAM_ES && p_para->vcodec != NULL) || /*ES*/
         (p_para->stream_type != STREAM_ES  && p_para->codec && p_para->vstream_info.has_video))) { /*PS,RM,TS*/
        /*if latest cmd and next cmd are all search,we must wait the frame show.*/
        if (p_para->vcodec) {
            ret = codec_get_cntl_state(p_para->vcodec);    /*es*/
        } else {
            ret = codec_get_cntl_state(p_para->codec);    /*not es*/
        }
        if(p_para->avsynctmpchanged == 0) {
            p_para->oldavsyncstate = get_tsync_enable();
        }
        if (p_para->oldavsyncstate == 1) {
            set_tsync_enable(0);
            p_para->avsynctmpchanged = 1;
        }
        if (ret <= 0) {
            return NONO_FLAG;
        }
    }	
    cmd = get_message(player);
    if (!cmd) {
        return 0;    /*no cmds*/
    }

    p_para->oldcmd = *cmd;
    p_para->oldcmdtime = player_get_systemtime_ms();
    log_print("pid[%d]:: [check_flag:%d]cmd=%x set_mode=%x info=%x param=%d fparam=%f\n", p_para->player_id, __LINE__, cmd->ctrl_cmd, cmd->set_mode, cmd->info_cmd, cmd->param, cmd->f_param);
    if (cmd->ctrl_cmd != CMD_SEARCH && p_para->avsynctmpchanged > 0) {
		/*not search now,resore the sync states...*/
		set_tsync_enable(p_para->oldavsyncstate);
		p_para->avsynctmpchanged = 0;
    }	
    check_msg(player,cmd);
    message_free(cmd);
    if(player->playctrl_info.search_flag){
	/*in mate thread seek,and interrupt the read thread.
	    so we need to ignore the first ffmpeg erros.
	*/
	player->playctrl_info.ignore_ffmpeg_errors=1;	
	player->playctrl_info.temp_interrupt_ffmpeg=1;
	log_print("ffmpeg_interrupt tmped by player mate!\n");
	ffmpeg_interrupt(player->thread_mgt.pthread_id);
	codec_resume(player->codec);  /*auto resume on*/
    }
    if (p_para->playctrl_info.pause_flag) {
        if (get_player_state(p_para) != PLAYER_PAUSE) {
            ret = codec_pause(p_para->codec);
            if (ret != 0) {
                log_error("[%s:%d]pause failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
            }
            set_player_state(p_para, PLAYER_PAUSE);
            update_playing_info(p_para);
            update_player_states(p_para, 1);
        }
        return CONTINUE_FLAG;
    } else {
        if ((get_player_state(p_para) == PLAYER_PAUSE) || (get_player_state(p_para) == PLAYER_SEARCHOK)) {
            ret = codec_resume(p_para->codec);
            if (ret != 0) {
                log_error("[%s:%d]resume failed!ret=%d\n", __FUNCTION__, __LINE__, ret);
            }
            set_player_state(p_para, PLAYER_RUNNING);
            update_playing_info(p_para);
            update_player_states(p_para, 1);
        }
    }
    return 0;
}


static int player_mate_thread_run_per100ms(play_para_t *player, struct player_mate *mate)
{
    mate_print("[MATE]player's mate start do a day work now\n ");
    player_mate_thread_cmd_proxy(player, mate); /**/
    mate_print("[MATE]player's mate finixhed cmd proxy\n ");
    update_playing_info(player);
    mate_print("[MATE]player's mate finished update playing info\n ");
    update_player_states(player, 0);
    mate_print("[MATE]player's mate finished one day work now\n ");
    return 0;
}
static int player_mate_thread_run_ll_everytime(play_para_t *player, struct player_mate *mate)
{
    player_hwbuflevel_update(player);
    return 0;
}
static int player_mate_thread_run_l(play_para_t *player, struct player_mate *mate)
{
    int worktimenum = 0;
    int runll = 101 / (mate->work_intervals / 1000); /*work_intervals is less than 100ms?*/
    int runlldelay = 0;
    runll = (runll > 0 && runll < 100) ? runll : 0;
    runlldelay = runll;
    mate_print("[MATE]player's mate start work now!\n ");
    while (!mate->mate_should_sleep && !mate->mate_should_exit) {
        worktimenum++;
        if (--runlldelay <= 0) { /*run once delay about  100ms...*/
            player_mate_thread_run_per100ms(player, mate);
            runlldelay = runll;
        }
        player_mate_thread_run_ll_everytime(player, mate);
        mate_print("[MATE]player's mate sleep now %d\n ", worktimenum);
        player_mate_thread_wait(mate, mate->work_intervals);
        mate_print("[MATE]player's mate wake and try get next day work\n ");
    }
    mate_print("[MATE]player's mate exit work now!\n ");
    return 0;
}

static int player_mate_thread_run(play_para_t *player)
{
    struct player_mate *mate = player->player_mate;
    while (!mate->mate_should_exit) {
        sem_wait(&mate->mate_sem);/*wait main thread to do heavy works*/
        mate->mate_isrunng = 1;
        if (mate->wake_delay > 0) {
            player_mate_thread_wait(mate, mate->wake_delay);    /*do wait,if main thread may wake fast*/
        }
        mate->wake_delay = 0;
        if (!mate->mate_should_sleep) {
            player_mate_thread_run_l(player, mate);
        }
        wakeup_player_thread(player);
        mate->mate_isrunng = 0;
    }
    return 0;
}

