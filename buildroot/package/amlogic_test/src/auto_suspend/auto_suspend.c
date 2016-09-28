#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>
//#include <linux/android_alarm.h>
#include "android_alarm.h"

int main(int argc, char *argv[])
{
	int fd = 0;
	int fd1 = 0;
	unsigned int cmd = 0;
	int result = 0;
	int alarmtype = 0;
	int waitalarmmask = 0;
	struct timespec ts;  
	struct timeval tv;
	const char* on="on";
	const char* mem="mem";
	unsigned int time_on=0;
	unsigned int time_suspend=0;
	unsigned int repeat=0;
	int cnt=0;

	gettimeofday(&tv, NULL);  
	ts.tv_sec = tv.tv_sec;  
	ts.tv_nsec = tv.tv_usec*1000; 

	printf(" +======+ AutoSuspend Start. +======+\n");
	if(argc <5)
	{
		printf("\n Usage: %s 0 time_on  time_suspend  repeat. \n", argv[0]);
		return 0;	
	}

	fd = open("/dev/alarm", O_RDWR);
	fd1 = open("/sys/power/state", O_RDWR);

	if (fd < 0 || fd1 <0 )
	{
		printf("+======+ Unable to open device. /dev/alarm: %d.  /sys/power/state:%d   \n", fd,fd1);
		return -1;
	}
	alarmtype = atoi(argv[1]);
	time_suspend = strtol(argv[2], NULL, 0);
	time_on = strtol(argv[3], NULL, 0);
	repeat= strtol(argv[4], NULL, 0);
	printf("\n +============+ on %d secs. suspend:%d secs repeat_%d +============+\n", time_on, time_suspend,repeat);

	for(cnt=0;cnt<repeat;cnt++)
	{
		printf("+============+ Auto Suspend Test Cnt_%d +============+ \n",cnt);
		result = ioctl(fd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_RTC_WAKEUP), &ts);
		if (result < 0)  
		{  
			printf(" +============+. Unable to get time  %d +============+\n", result);  
			close(fd);
			close(fd1);
			return -1;
		}
		ts.tv_sec += time_suspend;
		cmd = ANDROID_ALARM_SET(alarmtype);
		waitalarmmask |= 1U << alarmtype;

		result = ioctl(fd, cmd, &ts);  
		if (result < 0)  
		{  
			printf("+============+ Unable to set alarm %d. +============+\n", result);  
			close(fd);
			close(fd1);
			return -1;
		}
		lseek(fd1,0,SEEK_SET);
		result=write(fd1,mem,3);
		printf(" +============+ Set alarm and suspend %d +============+ \n",result);


		while(waitalarmmask)
		{
			printf(" +============+ wait for alarm %x +============+\n", waitalarmmask);
			result = ioctl(fd, ANDROID_ALARM_WAIT);
			if(result < 0) 
			{
				printf("alarm wait failed\n");
			}
			lseek(fd1,0,SEEK_SET);
			write(fd1,on,2);
			printf(" +============+. wakeup %x +============+\n", result);
			waitalarmmask &= ~result;
			sleep(1);
		}
		sleep(time_on);
	}
	printf(" +============+. AutoSuspend Test Finished. Total:%d times +============+\n", cnt);	
	close(fd);
	close(fd1);
	return 0;
}
