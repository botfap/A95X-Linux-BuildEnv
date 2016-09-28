#define LOG_TAG "AmAvutls"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <sys/ioctl.h>
#include "include/amutils_common.h"
#include "include/amlog.h"

int set_sys_int(const char *path,int val)
{
    	int fd;
        char  bcmd[16];
        fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
        if(fd>=0)
        {
        	sprintf(bcmd,"%d",val);
        	write(fd,bcmd,strlen(bcmd));
        	close(fd);
        	LOGI("\n switch %s clk81 freq\n", val?"high":"low");
        	return 0;
        }

        return -1;
}

int get_sys_int(const char *path)
{
    int fd;
    int val = 0;
    char  bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
    }
    return val;
}
