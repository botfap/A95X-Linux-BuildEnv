#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "alsactl_parser.h"
#define DEFALUT_CONF_PATH "/etc/alsactl.conf"
char *key_find;
char *strtrimr(char *pstr)
{
    int i;
    i = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i >= 0))
        pstr[i--] = '\0';
    return pstr;
}

char *strtriml(char *pstr)
{
    int i = 0,j;
    j = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i <= j))
        i++;
    if (0<i)
        strcpy(pstr, &pstr[i]);
    return pstr;
}


char *strtrim(char *pstr)
{
    char *p;
    p = strtrimr(pstr);
    return strtriml(p);

}

int get_setting_from_line(char *line, struct alsactl_setting *setting, char * key)
{	
    char *p = strtrim(line); 	
    int len = strlen(p);
	
    if(len <= 0){
        return 1;
    }
    else if(p[0]=='#'){
        return 2;
    }else{
        char *p1 = strstr(p, key);
        char *p2 = strchr(p1, '"');
        char *p3 = strchr(p2+1, '"');
 
	//printf("p1value=%s,p2value=%s,p3=%s\n",p1,p2,p3);
       key_find = (char *)malloc(strlen(key) + 1);
       // printf("len23=%d\n",strlen(p2)-strlen(p3));
        strncpy(key_find,p1,strlen(key));
        *(key_find+strlen(key))='\0';
      //  printf("key_find=%s\n",key_find);
        *p3='\0';
        strncpy(setting->ctlname,p2+1,strlen(p2)-strlen(p3));
	//setting->ctlname[strlen(p2)-strlen(p3)-1] ='\0'; 	
       // printf("ctlname=%s,val=%c:%d\n",setting->ctlname,*(p3+2),(*(p3+2)));
        setting->minvalue = atoi(p3+2);
        setting->maxvalue = atoi(p3+4);
	//printf("minvalue=%d,maxvalue=%d,name=%s\n",setting->minvalue,setting->maxvalue,setting->ctlname);
        return 3;	
    }

    return 0;//

}

void get_setting_control(FILE *file, struct alsactl_setting *setting, char * key){
    char line[1024];
  
    while (fgets(line, 1023, file)){
        get_setting_from_line(line, setting, key);
        if(key_find&&!strcmp(key_find,key)){
	    free(key_find);     
            key_find=NULL;
            break;
        }		
    }
}

int alsactl_parser(){
    FILE *file;
    struct alsactl_setting setting;
    mute_ctl.is_parsed=playback_ctl.is_parsed =0;
    file = fopen(DEFALUT_CONF_PATH, "r");
    if (!file) {
        printf("Failed to open %s", DEFALUT_CONF_PATH);
        fclose(file);
        return -1;
    }
    
    get_setting_control(file,&setting,"MASTERVOL");
    playback_ctl = setting;	
    get_setting_control(file,&setting,"MUTENAME");
    mute_ctl = setting;
 //   printf("vloname=%s,mutename=%s\n",playback_ctl.ctlname, mute_ctl.ctlname);
    fclose(file); 
   mute_ctl.is_parsed=playback_ctl.is_parsed =1;
    return 0;//
}


	

