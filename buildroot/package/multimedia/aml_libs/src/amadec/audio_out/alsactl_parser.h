typedef struct alsactl_setting{
char ctlname[1024]; //for master playback volume control name 
int  minvalue;  //mute control name
int  maxvalue;
int is_parsed;
}alsactl_setting_t;

extern alsactl_setting_t playback_ctl;
extern alsactl_setting_t mute_ctl;
int alsactl_parser();
