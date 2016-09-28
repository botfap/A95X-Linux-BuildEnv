#ifndef PTHREAD_MSG_H
#define PTHREAD_MSG_H
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <player.h>

#include "player_priv.h"


//#define PLAYER_DEBUG
#ifdef PLAYER_DEBUG
void debug_set_player_state(play_para_t *player, player_status status, const char *fn, int line);
#define  set_player_state(player,status) debug_set_player_state(player,status,__func__,__LINE__)
#else
void set_player_state(play_para_t *player, player_status status);

#endif


player_status get_player_state(play_para_t *player);
int    player_thread_wait_exit(play_para_t *player);
int    player_thread_create(play_para_t *player);
int    player_thread_wait(play_para_t *player, int microseconds);
int    wakeup_player_thread(play_para_t *player);


#endif

