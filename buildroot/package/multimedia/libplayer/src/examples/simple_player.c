
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <player.h>
#include <log_print.h>
//#include <version.h>

int main(int argc,char ** argv)
{
		play_control_t ctrl;
		int pid;
		
		if(argc<2)
		{
			printf("USAG:%s file\n",argv[0]);
			return 0;
		}
		player_init();
		memset(&ctrl,0,sizeof(ctrl));
		ctrl.file_name=argv[1];
		ctrl.video_index=-1;
		ctrl.audio_index=-1;
		ctrl.sub_index=-1;
		ctrl.t_pos=-1;
		pid=player_start(&ctrl,0);
		if(pid<0)
			{
			printf("play failed=%d\n",pid);
			return -1;
			}
		while(!PLAYER_THREAD_IS_STOPPED(player_get_state(pid)))
		usleep(10000);
		player_stop(pid);
		player_exit(pid);
		printf("play end=%d\n",pid);
		return 0;
}
