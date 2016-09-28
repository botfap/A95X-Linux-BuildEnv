/********************************************
 * name : player_control.c
 * function: thread manage
 * date     : 2010.2.22
 ********************************************/
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <player.h>

//private
#include "player_priv.h"
#include "player_para.h"
#include "player_av.h"
#include "player_update.h"
#include "thread_mgt.h"
#include "stream_decoder.h"

static pthread_mutex_t priv_pid_mutex;
static unsigned long priv_pid_pool = 0; /*player_id*/
static void * priv_pid_data[MAX_PLAYER_THREADS];/*player_id*/
static unsigned long priv_pid_used[MAX_PLAYER_THREADS];/*player_id*/
#define PID_isVALID(pid) (pid>=0 && pid<32 && (priv_pid_pool &(1 <<pid)))

#define PID_PRINT(fmt,args...)  log_print(fmt,##args)


int player_id_pool_init(void)
{
    priv_pid_pool = 0;
    MEMSET(priv_pid_data, 0, sizeof(priv_pid_data));
    MEMSET(priv_pid_used, 0, sizeof(priv_pid_used));
    pthread_mutex_init(&priv_pid_mutex, NULL);
    return 0;
}
int player_request_pid(void)
{
    int i,j;
    int pid = -1;
    static int last = -1;
    pthread_mutex_lock(&priv_pid_mutex);
    log_debug1("[player_request_pid:%d]last=%d\n", __LINE__, last);
    i = last+1;  
    if (i >= (MAX_PLAYER_THREADS)) {
         i = 0;
    } 	
    for (j=0;j < MAX_PLAYER_THREADS; j++) {
        if (!(priv_pid_pool & (1 << i))) {
            priv_pid_pool |= (1 << i);
            priv_pid_data[i] = NULL;
            priv_pid_used[i] = 0;
            pid = i;
	     last=i;
            log_debug1("[player_request_pid:%d]last=%d pid=%d\n", __LINE__, last, pid);
            break;
        }
	 i = i + 1;
        if (i >= (MAX_PLAYER_THREADS)) {
        	 i = 0;
	 }
    }
    pthread_mutex_unlock(&priv_pid_mutex);
    return pid;
}
int player_release_pid(int pid)
{
    int ret = PLAYER_NOT_VALID_PID;
    pthread_mutex_lock(&priv_pid_mutex);
    if (PID_isVALID(pid)) {
        if (priv_pid_used[pid] != 0) {
            PID_PRINT(":WARING!%s:PID is in using!,pid=%d,used=%ld\n", __FUNCTION__, pid, priv_pid_used[pid]);
        }
        priv_pid_used[pid] = 0;
        priv_pid_data[pid] = NULL;
        priv_pid_pool &= ~(1 << pid);
        log_print("[player_release_pid:%d]release pid=%d\n", __LINE__, pid);
        ret = PLAYER_SUCCESS;
    } else {

        PID_PRINT("%s:pid is not valid,pid=%d,priv_pid_pool=%lx\n", __FUNCTION__, pid, priv_pid_pool);
    }
    pthread_mutex_unlock(&priv_pid_mutex);
    return ret;
}



int player_init_pid_data(int pid, void * data)
{
    int ret;
    pthread_mutex_lock(&priv_pid_mutex);
    if (PID_isVALID(pid)) {
        priv_pid_data[pid] = data;
        ret = PLAYER_SUCCESS;
    } else {
        ret = PLAYER_NOT_VALID_PID;
    }
    pthread_mutex_unlock(&priv_pid_mutex);
    return ret;
}

void * player_open_pid_data(int pid)
{
    void * pid_data;
    pthread_mutex_lock(&priv_pid_mutex);
    if (PID_isVALID(pid)) {
        pid_data = priv_pid_data[pid];
        priv_pid_used[pid]++;
    } else {
        pid_data = NULL; /*0 is no data!NULL*/
        //PID_PRINT("%s:pid is not valid,pid=%d,priv_pid_pool=%lx\n",__FUNCTION__,pid,priv_pid_pool);
    }
    pthread_mutex_unlock(&priv_pid_mutex);

    return pid_data;
}
int player_close_pid_data(int pid)
{
    int ret = PLAYER_NOT_VALID_PID;
    pthread_mutex_lock(&priv_pid_mutex);
    if (PID_isVALID(pid)) {
        if (priv_pid_used[pid] > 0) {
            priv_pid_used[pid]--;
            ret = PLAYER_SUCCESS;
        } else {
            PID_PRINT("PID data release too much time:pid=%d!\n", pid);
        }
    } else {
        PID_PRINT("%s:pid is not valid,pid=%d,priv_pid_pool=%lx\n", __FUNCTION__, pid, priv_pid_pool);
    }
    pthread_mutex_unlock(&priv_pid_mutex);

    return ret;
}

int player_list_pid(char id[], int size)
{
    int i, ids;
    ids = 0;
    for (i = 0; i < MAX_PLAYER_THREADS && i < size; i++) {
        if (priv_pid_pool & (1 << i)) {
            id[ids++] = i;
        }
    }
    return ids;
}

int check_pid_valid(int pid)
{
    return PID_isVALID(pid);
}
