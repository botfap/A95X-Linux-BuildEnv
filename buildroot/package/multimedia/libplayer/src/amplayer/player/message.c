#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <player.h>

#include "player_priv.h"
#include "player_update.h"
#include "thread_mgt.h"


int message_pool_init(play_para_t *para)
{
    message_pool_t *pool = &para->message_pool;
    pool->message_num = 0;
    pool->message_in_index = 0;
    pool->message_out_index = 0;
    MEMSET(pool->message_list, 0, sizeof(pool->message_list));
    pthread_mutex_init(&pool->msg_mutex, NULL);
    return 0;
}
player_cmd_t * message_alloc(void)
{
    player_cmd_t *cmd;
    cmd = MALLOC(sizeof(player_cmd_t));
    if (cmd) {
        MEMSET(cmd, 0, sizeof(player_cmd_t));
    }
    return cmd;
}

int message_free(player_cmd_t * cmd)
{
    if (cmd) {
        FREE(cmd);
    }
    cmd = NULL;
    return 0;
}
int send_message_update(play_para_t *para, player_cmd_t *cmd)
{
    int i, j;
    player_cmd_t *oldcmd;
    int updated = 0;

    message_pool_t *pool = &para->message_pool;
    //log_print("[send_message:%d]num=%d in_idx=%d out_idx=%d\n",__LINE__,pool->message_num,pool->message_in_index,pool->message_out_index);
    pthread_mutex_lock(&pool->msg_mutex);
    j = pool->message_out_index;
    for (i = 0; i < pool->message_num; i++) {
        oldcmd = pool->message_list[j];
        if (oldcmd && (oldcmd->ctrl_cmd == cmd->ctrl_cmd)) { /*same cmd*/
            *oldcmd = *cmd; /*update old one*/
            log_print("found old message update old one.\n");
            updated = 1;
            break;
        }
    }
    pthread_mutex_unlock(&pool->msg_mutex);
    if (!updated) {
        return send_message(para, cmd);
    } else {
        message_free(cmd);
    }
    return 0;
}

int send_message(play_para_t *para, player_cmd_t *cmd)
{
    int ret = -1;
    message_pool_t *pool = &para->message_pool;
    //log_print("[send_message:%d]num=%d in_idx=%d out_idx=%d\n",__LINE__,pool->message_num,pool->message_in_index,pool->message_out_index);
    pthread_mutex_lock(&pool->msg_mutex);
    if (pool->message_num < MESSAGE_MAX) {
        pool->message_list[pool->message_in_index] = cmd;
        pool->message_in_index = (pool->message_in_index + 1) % MESSAGE_MAX;
        pool->message_num++;
        wakeup_player_thread(para);
        ret = 0;
    } else {
        /*message_num is full*/
        player_cmd_t *oldestcmd;
        oldestcmd = pool->message_list[pool->message_in_index];
        FREE(oldestcmd);
        pool->message_out_index = (pool->message_out_index + 1) % MESSAGE_MAX; /*del the oldest command*/
        pool->message_list[pool->message_in_index] = cmd;
        pool->message_in_index = (pool->message_in_index + 1) % MESSAGE_MAX;
        wakeup_player_thread(para);
        ret = 0;
    }
    log_debug1("[send_message:%d]num=%d in_idx=%d out_idx=%d cmd=%x mode=%d\n", __LINE__, pool->message_num, pool->message_in_index, pool->message_out_index, cmd->ctrl_cmd, cmd->set_mode);
    pthread_mutex_unlock(&pool->msg_mutex);
    return ret;

}

int send_message_by_pid(int pid, player_cmd_t *cmd)
{
    int ret;
    play_para_t *player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }
    if (cmd->ctrl_cmd == CMD_SEARCH) {
        ret = send_message_update(player_para, cmd);
    } else {
        ret = send_message(player_para, cmd);
    }
    player_close_pid_data(pid);
    return ret;
}
player_cmd_t * get_message_locked(play_para_t *para)
{
    player_cmd_t *cmd = NULL;
    message_pool_t *pool = &para->message_pool;
    if (pool == NULL) {
        log_error("[get_message_locked]pool is null!\n");
        return NULL;
    }
    if (pool->message_num > 0) {

        cmd = pool->message_list[pool->message_out_index];
        pool->message_out_index = (pool->message_out_index + 1) % MESSAGE_MAX;
        pool->message_num--;
        log_print("[get_message_locked:%d]num=%d in_idx=%d out_idx=%d cmd=%x\n", __LINE__, pool->message_num, pool->message_in_index, pool->message_out_index, cmd->ctrl_cmd);
    }
    return cmd;
}
player_cmd_t * get_message(play_para_t *para)
{
    player_cmd_t *cmd = NULL;
    message_pool_t *pool = &para->message_pool;
    if (pool == NULL) {
        log_error("[get_message]pool is null!\n");
        return NULL;
    }
    pthread_mutex_lock(&pool->msg_mutex);
    //log_print("[get_message]pool=%p msg_num=%d\n",pool,pool->message_num);
    if (pool->message_num > 0) {

        cmd = pool->message_list[pool->message_out_index];
        pool->message_out_index = (pool->message_out_index + 1) % MESSAGE_MAX;
        pool->message_num--;
        log_print("[get_message:%d]num=%d in_idx=%d out_idx=%d cmd=%x\n", __LINE__, pool->message_num, pool->message_in_index, pool->message_out_index, cmd->ctrl_cmd);
    }
    pthread_mutex_unlock(&pool->msg_mutex);
    return cmd;
}
player_cmd_t * peek_message_locked(play_para_t *para)
{
    player_cmd_t *cmd = NULL;
    message_pool_t *pool = &para->message_pool;
    if (pool == NULL) {
        log_error("[peek_message_locked]pool is null!\n");
        return NULL;
    }
    //log_print("[get_message]pool=%p msg_num=%d\n",pool,pool->message_num);
    if (pool->message_num > 0) {

        cmd = pool->message_list[pool->message_out_index];
        log_print("[peek_message_locked:%d]num=%d in_idx=%d out_idx=%d cmd=%x\n", __LINE__, pool->message_num, pool->message_in_index, pool->message_out_index, cmd->ctrl_cmd);
    }
    return cmd;
}


int lock_message_pool(play_para_t *para)
{
    message_pool_t *pool = &para->message_pool;
    pthread_mutex_lock(&pool->msg_mutex);
    return 0;
}
int unlock_message_pool(play_para_t *para)
{
    message_pool_t *pool = &para->message_pool;
    pthread_mutex_unlock(&pool->msg_mutex);
    return 0;
}
player_cmd_t * peek_message(play_para_t *para)
{
    player_cmd_t *cmd;
    message_pool_t *pool = &para->message_pool;
    if (pool == NULL) {
        log_error("[peek_message]pool is null!\n");
        return NULL;
    }
    pthread_mutex_lock(&pool->msg_mutex);
    cmd = peek_message_locked(para);
    pthread_mutex_unlock(&pool->msg_mutex);
    return cmd;
}

void clear_all_message(play_para_t *para)
{
    player_cmd_t *cmd;
    while ((cmd = get_message(para)) != NULL) {
        message_free(cmd);
    }
}
int register_update_callback(callback_t *cb, update_state_fun_t up_fn, int interval_s)
{
    if (up_fn != NULL) {
        cb->update_statue_callback = up_fn;
    } else {
        return PLAYER_ERROR_CALLBACK;
    }
    if (interval_s > 0) {
        cb->update_interval = interval_s;
    }
    return PLAYER_SUCCESS;
}

int update_player_states(play_para_t *para, int force)
{
    callback_t *cb = &para->update_state;
    update_state_fun_t fn;
    para->state.last_sta = para->state.status;
    para->state.status = get_player_state(para);

    if (check_time_interrupt(&cb->callback_old_time, cb->update_interval) || force) {
        player_info_t state;
        MEMCPY(&state, &para->state, sizeof(state));
        //if(force == 1)
        log_print("**[update_state]pid:%d status=%s(tttlast:%s) err=0x%x curtime=%d (ms:%d) fulltime=%d lsttime=%d\n",
                  para->player_id,
                  player_status2str(state.status),
                  player_status2str(state.last_sta),
                  (-state.error_no),
                  state.current_time,
                  state.current_ms,
                  state.full_time,
                  state.last_time);		
        log_print("**[update_state]abuflevel=%.08f vbublevel=%.08f abufrp=%x vbufrp=%x read_end=%d\n",
                  state.audio_bufferlevel,
                  state.video_bufferlevel,
                  para->abuffer.buffer_rp,
                  para->vbuffer.buffer_rp,
                  para->playctrl_info.read_end_flag);
        fn = cb->update_statue_callback;

        if (fn) {
            fn(para->player_id, &state);
        }
        send_event(para, PLAYER_EVENTS_PLAYER_INFO, &state, 0);
        para->state.error_no = 0;
        player_hwbuflevel_update(para);
    }
	return 0;
}

int cmd2str(player_cmd_t *cmd, char *buf)
{
    int len = 0;

    if (!cmd) {
        return -1;
    }

    if (cmd->ctrl_cmd > 0) {
        switch (cmd->ctrl_cmd) {
        case CMD_EXIT:
            len = sprintf(buf, "%s", "PLAYER_EXIT");
            break;

        case CMD_PLAY:
            len = sprintf(buf, "%s:%s", "PLAYER_PLAY", cmd->filename);
            break;

        case CMD_PLAY_START:
            len = sprintf(buf, "%s:%s", "PLAYER_PLAY_START", cmd->filename);
            break;

        case CMD_STOP:
            len = sprintf(buf, "%s", "PLAYER_STOP");
            break;

        case CMD_START:
            len = sprintf(buf, "%s", "PLAYER_START");
            break;

        case CMD_NEXT:
            len = sprintf(buf, "%s", "PLAYER_PLAY_NEXT");
            break;

        case CMD_PREV:
            len = sprintf(buf, "%s", "PLAYER_PLAY_PREV");
            break;

        case CMD_PAUSE:
            len = sprintf(buf, "%s", "PLAYER_PAUSE");
            break;

        case CMD_RESUME:
            len = sprintf(buf, "%s", "PLAYER_RESUME");
            break;

        case CMD_SEARCH:
            len = sprintf(buf, "%s:%f", "PLAYER_SEEK", cmd->f_param);
            break;

        case CMD_FF:
            len = sprintf(buf, "%s:%d", "PLAYER_FF", cmd->param);
            break;

        case CMD_FB:
            len = sprintf(buf, "%s:%d", "PLAYER_FR", cmd->param);
            break;

        case CMD_SWITCH_AID:
            len = sprintf(buf, "%s:%d", "SWITCH_AUDIO", cmd->param);
            break;

        case CMD_SWITCH_SID:
            len = sprintf(buf, "%s:%d", "SWITCH_SUBTITLE", cmd->param);
            break;

        default:
            len = sprintf(buf, "%s", "UNKNOW_PLAYER_CTRL_COMMAND");
            break;
        }
    }

    if (cmd->set_mode > 0) {
        switch (cmd->set_mode) {
        case CMD_LOOP:
            len = sprintf(buf, "%s", "SET_LOOP_PLAY");
            break;

        case CMD_NOLOOP:
            len = sprintf(buf, "%s", "CANCLE_LOOP");
            break;

        case CMD_BLACKOUT:
            len = sprintf(buf, "%s", "SET_BLACK_OUT");
            break;

        case CMD_NOBLACK:
            len = sprintf(buf, "%s", "CANCLE_BLACK");
            break;

        case CMD_NOAUDIO:
            len = sprintf(buf, "%s", "CLOSE_AUDIO_PLAY");
            break;

        case CMD_NOVIDEO:
            len = sprintf(buf, "%s", "CLOSE_VIDEO_PLAY");
            break;

        case CMD_MUTE:
            len = sprintf(buf, "%s", "SET_VOLUME_MUTE");
            break;

        case CMD_UNMUTE:
            len = sprintf(buf, "%s", "CANCLE_VOLUME_MUTE");
            break;

        case CMD_SET_VOLUME:
            len = sprintf(buf, "%s:%d", "SET_VOLUME", cmd->param);
            break;

        case CMD_SPECTRUM_SWITCH:
            len = sprintf(buf, "%s", "SPECTRUM_SWITCH");
            break;

        case CMD_SET_BALANCE:
            len = sprintf(buf, "%s", "SET_BALANCE");
            break;

        case CMD_SWAP_LR:
            len = sprintf(buf, "%s", "SWAP_CHANNEL");
            break;

        case CMD_LEFT_MONO:
            len = sprintf(buf, "%s", "LEFT_MONO");
            break;

        case CMD_RIGHT_MONO:
            len = sprintf(buf, "%s", "RIGHT_MONO");
            break;

        case CMD_SET_STEREO:
            len = sprintf(buf, "%s", "SET_STEREO");
            break;
        case CMD_EN_AUTOBUF:
            len = sprintf(buf, "%s", "ENABLE_AUTOBUF");
            break;

        case CMD_SET_AUTOBUF_LEV:
            len = sprintf(buf, "%s:%.03f:%.03f:%.03f", "SET_AUTOBUF_LEVEL", cmd->f_param, cmd->f_param1, cmd->f_param2);
            break;

        default:
            len = sprintf(buf, "%s", "UNKNOW_SETMODE_COMMAND");
            break;
        }
    }
    if (cmd->info_cmd > 0) {
        switch (cmd->info_cmd) {
        case CMD_GET_VOLUME:
            len = sprintf(buf, "%s", "GET_CURRENT_VOLUME");
            break;

        case CMD_GET_VOL_RANGE:
            len = sprintf(buf, "%s", "GET_VOLUME_RANGE");
            break;

        case CMD_GET_PLAY_STA:
            len = sprintf(buf, "%s", "GET_PLAYER_STATUS");
            break;

        case CMD_GET_CURTIME:
            len = sprintf(buf, "%s", "GET_CURRENT_PLAYTIME");
            break;

        case CMD_GET_DURATION:
            len = sprintf(buf, "%s", "GET_PLAY_DURATION");
            break;

        case CMD_GET_MEDIA_INFO:
            len = sprintf(buf, "%s", "GET_MEDIA_INFORMATION");
            break;

        case CMD_LIST_PID:
            len = sprintf(buf, "%s", "LIST_ALIVED_PID");
            break;

        default:
            len = sprintf(buf, "%s", "UNKNOW_GETINFO_COMMAND");
        }
    }
    buf[len + 1] = '\0';

    return 0;
}
int send_event(play_para_t *para, int msg, unsigned long ext1, unsigned long ext2)
{
    callback_t *cb = &para->update_state;
    if (cb->notify_fn) {
        cb->notify_fn(para->player_id, msg, ext1, ext2);
    }
    return 0;
}            
