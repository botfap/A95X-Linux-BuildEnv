
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

#include <dlfcn.h>

#include <controler.h>

#include <adecproc.h>

static global_ctrl_para_t player_para;


int osd_blank(char *path,int cmd)
{
	int fd;
	char  bcmd[16];
	fd=open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);
	if(fd>=0)
		{
		sprintf(bcmd,"%d",cmd);
		write(fd,bcmd,strlen(bcmd));
		close(fd);
		return 0;
		}
	return -1;
}


int load_extern_lib(global_ctrl_para_t *p,const char *libs)
{
	void* fd;
	static int lib_index=0;
	char *ps,*pp,*op,*buf; 
	
	buf=MALLOC(strlen(optarg)+20);
	pp=optarg;
	op=optarg;
	if(buf==NULL)
		{
		printf("extern lib memory problem %s\n",libs);
		return -1;
		}
	
	while(op!=NULL)
		{	
			int len;
			pp=strstr(op,",");
			ps=buf+10;
			if(pp!=NULL)
				{
				len=pp-op;
				memcpy(ps,op,len);
				ps[(int)(len)]='\0';
				op=pp+1;
				}
			else
				{
				strcpy(ps,op);
				len=strlen(ps);
				op=NULL;
				}
			if(lib_index>10)
			{
				printf("too many lib register,10 is max\n");
				goto error;
			}
			if(strstr(ps,".so")==NULL)
			{	
				ps-=3;
				ps[0]='l';
				ps[1]='i';
				ps[2]='b';
				len+=3;
				ps[len++]='.';
				ps[len++]='s';
				ps[len++]='o';
				ps[len++]='\0';
			}
			fd=dlopen(ps,RTLD_NOW);
			if(fd==NULL)
			{
				printf("open extern lib:%s failed\n",ps);
				goto error;
			}
			printf("opened extern lib:%s\n",ps);
			p->externlibfd[lib_index++]=fd;	
		
		}
	return 0;
error:	
	FREE(buf);
	return -1;
}

void release_extern_lib(global_ctrl_para_t *p)
{
	void * fd;
	int i=0;
	while(i<10)
		{
		fd=p->externlibfd[i++];
		if(fd!=NULL)	
			dlclose(fd);
		else
			break;
		}
}



void  print_usage(char *execname)
{
	printf("\r\n");
	printf("usage:\r\n");
	printf("%s  filename <-v vid> <-a aid> <-n> <-c> <-l>\r\n",execname);
	printf("\t-v --videoindex :video index,for ts stream\r\n");
	printf("\t-a --audioindex :auido index,for ts stream\r\n");
	printf("\t-L --list :the file is a playlist\r\n");
	printf("\t-m mode :player control mode\r\n");
	printf("\t\t[shell] shell mode\r\n");
	printf("\t\t[socket] socket mode\r\n");
	printf("\t\t[dbus] dbus mode\r\n");
	printf("\t-e extern<,extern1,extern2> :add extern libraries\r\n");
	printf("\t-l --loop :loop play\r\n");
	printf("\t-n --nosound :play without sound\r\n");
	printf("\t-c --novideo :play without video\r\n");
	printf("\t-b --background :running in the background\r\n");
	printf("\t-o --clearosd :clear osd\r\n");
    printf("\t-k --clearblack :clear blackout\r\n");
    printf("\t-d --setup the second player\r\n");
	printf("\r\n");
}

int parser_option(int argc, char *argv[],global_ctrl_para_t *p)
{
	int c;
	int i;
	
	
const char cshort_options[] = "v:a:m:e:Lblncokhu?";

const struct option clong_options[] = {
	{"videoindex", 1, 0, 'v'},
	{"audioindex", 1, 0, 'a'},
	{"mode", 1, 0, 'm'},
	{"extern", 1, 0, 'e'},
	{"playlist", 0, 0, 'L'},	
	{"background", 0, 0, 'b'},
	{"loop", 0, 0, 'l'},	
	{"nosound", 0, 0, 'n'},	
	{"novideo", 0, 0, 'c'},	
	{"clearosd", 0, 0, 'o'},
	{"clearblack",0, 0,'k'},	
	{"help", 0, 0, 'h'},
    {"subtitle", 0, 0, 'u'},
	{0, 0, 0, 0}
     };

    if(argc<=1)
    {
     printf("not set input file name\n");
        return -1;
    } 
	p->g_play_ctrl_para.video_index = -1;
	p->g_play_ctrl_para.audio_index = -1;
    p->g_play_ctrl_para.sub_index   = -1;
	
	p->control_mode[0]='\0';
    while ((c = getopt_long(argc, argv, cshort_options, clong_options, NULL)) != -1 ) 
	{
	 	switch(c) 
        {
			case 'v':
				sscanf(optarg,"%d",&i);
				if(i>=0)
					p->g_play_ctrl_para.video_index = i;
				else
					{
					  printf("unsupported video type\n");
		                        return -1;
					}
				break;
			case 'a':
				sscanf(optarg,"%d",&i);
				if(i>=0)
					p->g_play_ctrl_para.audio_index = i;
				else
					{
					  printf("unsupported video type\n");
		                       return -1;
					}
				break;
			case 'm':
				MEMCPY(p->control_mode,optarg,MIN(strlen(optarg),16));
				break;
			case 'e':
				if(load_extern_lib(p,optarg)!=0)
					{
					printf("load extern libraries failed,%s\n",optarg);
					return -1;
					}
				break;
			case 'b':
				p->background=1;
				break;
			case 'l':
				p->g_play_ctrl_para.loop_mode = 1;
				break;	
			case 'n':
				p->g_play_ctrl_para.nosound = 1;
				break;
            case 'u':
                p->g_play_ctrl_para.hassub = 1;
                break;
			case 'c':
				p->g_play_ctrl_para.novideo = 1;
				break;
			case 'o':
				osd_blank("/sys/class/graphics/fb0/blank",1);
				osd_blank("/sys/class/graphics/fb1/blank" ,1);
				break;
            case 'k':
                osd_blank("/sys/class/video/blackout_policy", 0);
                break;             
			case 'h':	
				return -1;	
			case ':':	
				  printf("unsupported option\n");
				return -1;
			case '?':
				printf("unsupported option =%s\n",optarg);
				return -1;
				break;
			case 0:
			default:
				printf("unsupported option =%s\n",optarg);
				break;
	 	}
     	}
	 if(optind>=0 && optind<argc)
	 	{
			char * file=argv[optind];
		 	p->g_play_ctrl_para.file_name=MALLOC(strlen(file)+1);
		 	if(p->g_play_ctrl_para.file_name==NULL)
	 			{
				printf("alloc memory for file failed ,file=%s!\n",file);
				return -1;
	 			}
			strcpy(p->g_play_ctrl_para.file_name,file);
	 	}
	 
	 return 0;
}
static void signal_handler(int signum)
{   
	printf("Get signum=%x\n",signum);
	player_progress_exit();	
	signal(signum, SIG_DFL);
	raise (signum);
}
static void daemonize()
{
    pid_t pid, sid;


    if (getppid() == 1)
        return;

    pid = fork();
    if (pid < 0) {
        printf("daemonize failed on fork");
        exit(0);
    }
    if (pid > 0)
        exit(0);


    sid = setsid();
    if (sid < 0)
        exit(-1);

    umask(0);

    if ((chdir("/")) < 0)
        exit(0);


    signal(SIGCHLD, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
}

int main(int argc,char *argv[])
{
  ///  printf("\n*********Amplayer version: %d************\n\n",SVN_VERSION);
	MEMSET(&player_para,0,sizeof(player_para));
	basic_controlers_init();
	if(parser_option(argc,argv,&player_para)<0)
	{
		print_usage(argv[0]);
		return 0;
	}
	player_init();	
	//amadec_thread_init();

	if(start_controler(&player_para)!=0)
	{
		printf("Can't get any controlers ,exit now\n");
		return -1;
	}
	if(player_para.background)
	{
		if(log_open(LOG_FILE)<0)

			{
			printf("open log file failed %s\n",LOG_FILE);
			return -1;
			}
		printf("\n\n start log %s\n",LOG_FILE);
		daemonize();
	}
	else
	{
		  signal(SIGINT, signal_handler);
	}	
	controler_run(&player_para);
	release_extern_lib(&player_para);
	/*reopen osd for mouse lost*/
	osd_blank("/sys/class/graphics/fb0/blank",0);
	osd_blank("/sys/class/graphics/fb1/blank",0);
	//amadec_thread_exit();
	//player_progress_exit();
	return 0;
}

