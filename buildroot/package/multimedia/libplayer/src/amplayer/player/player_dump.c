#include "player_priv.h"
#include "log_print.h"

int player_dump_playinfo(int pid, int fd)
{
   log_print("player_dump_playinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    
   #define SIZE     256
   char buffer[SIZE];
   if(fd < 0){
        log_error("[%s]Invalid handle fd\n", __FUNCTION__);
        return -1;
    }
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_playinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    snprintf(buffer, SIZE, "  cur_state:%s  last_state:%s\n", player_value2str("status", 
        player_para->state.status), player_value2str("status", player_para->state.last_sta));
    write(fd, buffer, strlen(buffer));

    snprintf(buffer, SIZE, "  cur_time:%d s (%lld ms) last_time:%d s fulltime:%d s (%lld ms) buffed_time: %d s\n", 
        player_para->state.current_time, player_para->state.current_ms, 
        player_para->state.last_time, player_para->state.full_time, 
        player_para->state.full_time_ms, player_para->state.bufed_time);
    write(fd, buffer, strlen(buffer));
    player_close_pid_data(pid);
    return 0;
}

int player_dump_bufferinfo(int pid, int fd)
{
   log_print("player_dump_bufferinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    
   decbuf_status_t adecbuf, vdecbuf;
   #define SIZE     256
   char buffer[SIZE];
   if(fd < 0){
        log_error("[%s]Invalid handle fd\n", __FUNCTION__);
        return -1;
    }
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_bufferinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    adecbuf = player_para->abuffer;
    vdecbuf = player_para->vbuffer;
    snprintf(buffer, SIZE, "  audio buffer size %x datalen=%x rp=%x\n", \
        adecbuf.buffer_size, adecbuf.data_level, adecbuf.buffer_rp);
    write(fd, buffer, strlen(buffer));
    snprintf(buffer, SIZE, "  video buffer size %x datalen=%x rp=%x\n", \
        vdecbuf.buffer_size, vdecbuf.data_level, vdecbuf.buffer_rp);
    write(fd, buffer, strlen(buffer));
    player_close_pid_data(pid);
    return 0;
}

int player_dump_tsyncinfo(int pid, int fd)
{
   #define SIZE     256
   char buffer[SIZE];
   log_print("player_dump_tsyncinfo pid=%d fd=%d\n", pid, fd);
   play_para_t *player_para;    
   if(fd < 0){
        log_error("[%s]Invalid handle fd\n", __FUNCTION__);
        return -1;
    }
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_error("player_dump_tsyncinfo error: can't find match pid[%d] player\n", pid);
        return PLAYER_NOT_VALID_PID;
    }
    snprintf(buffer, SIZE, "  sync enable:%d pts_pcrscr=%x pts_video=%x pts_audio=%x\n",
        player_para->playctrl_info.avsync_enable, get_pts_pcrscr(player_para), get_pts_video(player_para), get_pts_audio(player_para));
    write(fd, buffer, strlen(buffer));
    player_close_pid_data(pid);
    return 0;
}
