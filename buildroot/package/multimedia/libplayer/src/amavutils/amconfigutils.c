/*
libplayer's configs.
changed to HASH and list for fast get,set...

*/
#include "include/amconfigutils.h"
#include <stdio.h>
#include <pthread.h>
static char *amconfigs[MAX_CONFIG] = {0};
static int    amconfig_inited = 0;
#define lock_t          pthread_mutex_t
#define lp_lock_init(x,v)   pthread_mutex_init(x,v)
#define lp_lock(x)      pthread_mutex_lock(x)
#define lp_unlock(x)    pthread_mutex_unlock(x)
#define lp_trylock(x)   pthread_mutex_trylock(x)
#ifdef ANDROID
#include <cutils/properties.h>

#include <sys/system_properties.h>
 #endif
//#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#define DBGPRINT printf
#else
#define DBGPRINT(...)
#endif
static lock_t config_lock;
static char *malloc_config_item()
{
    return malloc(CONFIG_PATH_MAX + CONFIG_VALUE_MAX + 8);
}
static void free_config_item(char *item)
{
    free(item);
}

static int get_matched_index(const char * path)
{
    int len = strlen(path);
    char *ppath;
    int i;

    if (len >= CONFIG_PATH_MAX) {
        return -40;
    }
    for (i = 0; i < MAX_CONFIG; i++) {
        ppath = amconfigs[i];
        if (ppath)
            ;//DBGPRINT("check match [%d]=%s ?= %s \n",i,path,amconfigs[i]);
        if (ppath != NULL && strncmp(path, ppath, len) == 0) {
            return i;
        }
    }
    return -10;
}
static int get_unused_index(const char * path)
{
    int i;
    for (i = 0; i < MAX_CONFIG; i++) {
        if (amconfigs[i] == NULL) {
            return i;
        }
    }
    return -20;
}
int am_config_init(void)
{
    lp_lock_init(&config_lock, NULL);
    lp_lock(&config_lock);
    //can do more init here.
    memset(amconfigs, 0, sizeof(amconfigs));
    amconfig_inited = 1;
    lp_unlock(&config_lock);
    return 0;
}
int am_getconfig(const char * path, char *val, const char * def)
{
    int i,ret;
    if (!amconfig_inited) {
        am_config_init();
    }
    val[0]='\0';	
    lp_lock(&config_lock);
    i = get_matched_index(path);
    if (i >= 0) {
        strcpy(val, amconfigs[i] + CONFIG_VALUE_OFF);
    } else if (def != NULL) {
        strcpy(val, def);
    }
    lp_unlock(&config_lock);
#ifdef ANDROID
	if(i<0){
		/*get failed,get from android prop settings*/
	 	ret=property_get(path, val, def);	
		if(ret>0)
			i=1;
	}
#else
	//if(i<0){
		/*get failed,get from android prop settings*/
	// 	val=getenv(path);	
	//	if(val!=NULL)
		//	i=1;
	//}
#endif
    return strlen(val) ;
}


int am_setconfig(const char * path, const char *val)
{
    int i;
    char **pppath, *pconfig;
    char value[CONFIG_VALUE_MAX];
    char *setval = NULL;
    int ret = -1;
    if (!amconfig_inited) {
        am_config_init();
    }
    if (strlen(path) > CONFIG_PATH_MAX) {
        return -1;    /*too long*/
    }
    if (val != NULL) {
        setval = strdup(val);
        if(strlen(setval) >= CONFIG_VALUE_MAX)
            setval[CONFIG_VALUE_MAX] = '\0'; /*maybe val is too long,cut it*/
    }
    lp_lock(&config_lock);
    i = get_matched_index(path);
    if (i >= 0) {
        pppath = &amconfigs[i];
        if (!setval || strlen(setval) == 0) { //del value
            free_config_item(*pppath);
            amconfigs[i] = NULL;
            ret = 1; /*just not setting*/
            goto end_out;
        }
    } else {
        i = get_unused_index(path);
        if (i < 0) {
            ret = i;
            goto end_out;
        }
        if (!setval || strlen(setval) == 0) { //value is nothing.exit now;
            ret = 1; /*just not setting*/
            goto end_out;
        }
        DBGPRINT("used config index=%d,path=%s,val=%s\n", i, path, setval);
        pppath = &amconfigs[i];
        *pppath = malloc_config_item();
        if (!*pppath) {
            ret = -4; /*no MEM ?*/
            goto end_out;
        }
    }
    pconfig = *pppath;
    strcpy(pconfig, path);
    strcpy(pconfig + CONFIG_VALUE_OFF, setval);
    ret = 0;
end_out:
    if(setval!=NULL)
    	free(setval);
    lp_unlock(&config_lock);
    return ret;
}

int am_dumpallconfigs(void)
{
    int i;
    char *config;
    lp_lock(&config_lock);
    for (i = 0; i < MAX_CONFIG; i++) {
        config = amconfigs[i];
        if (config != NULL) {
            fprintf(stderr, "[%d] %s=%s\n", i, config, config + CONFIG_VALUE_OFF);
        }
    }
    lp_unlock(&config_lock);
    return 0;
}
int am_setconfig_float(const char * path, float value)
{
    char buf[CONFIG_VALUE_MAX];
    int len;
    len = snprintf(buf, CONFIG_VALUE_MAX - 1, "%f", value);
    buf[len] = '\0';
    return am_setconfig(path, buf);
}
int am_getconfig_float(const char * path, float *value)
{
    char buf[CONFIG_VALUE_MAX];
    int ret = -1;
      
    *value = -1.0;
    ret = am_getconfig(path, buf,NULL);
    if (ret > 0) {
        ret = sscanf(buf, "%f", value);
    }
    return ret > 0 ? 0 : -2;
}

float am_getconfig_float_def(const char * path,float defvalue)
{
    char buf[CONFIG_VALUE_MAX];
    int ret = -1;
    float value;
    ret = am_getconfig(path, buf,NULL);
    if (ret > 0) {
        ret = sscanf(buf, "%f", &value);
    }
    if(ret<=0)
        value=defvalue;
    return value;
}

int am_getconfig_bool(const char * path)
{
    char buf[CONFIG_VALUE_MAX];
    int ret = -1;

    ret = am_getconfig(path, buf,NULL);
    if (ret > 0) {
        if(strcasecmp(buf,"true")==0 || strcmp(buf,"1")==0)
            return 1;
    }
    return 0;
}

int am_getconfig_bool_def(const char * path,int def)
{
    char buf[CONFIG_VALUE_MAX];
    int ret = -1;

    ret = am_getconfig(path, buf,NULL);
    if (ret > 0) {
        if(strcasecmp(buf,"true")==0 || strcmp(buf,"1")==0)
            return 1;
        else
            return 0;
    }
    return def;
}


#ifndef ADROID
int property_get(const char *key, char *value, const char *default_value)
{
    printf("player system property_get [%s] %s\n", __FILE__, __FUNCTION__);
    return am_getconfig(key, value, default_value);
}
#endif  

