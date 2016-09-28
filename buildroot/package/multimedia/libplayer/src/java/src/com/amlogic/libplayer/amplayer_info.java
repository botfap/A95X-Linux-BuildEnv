package com.amlogic.libplayer;

public class amplayer_info {
    //typedef enum { ... } player_status;
    public static final int PLAYER_UNKNOWN  = 0;
    
	/******************************
	* 0x1000x: 
	* player do parse file
	* decoder not running
	******************************/
    public static final int PLAYER_INITING  	= 0x10001;
	public static final int PLAYER_TYPE_REDY  	= 0x10002;
    public static final int PLAYER_INITOK   	= 0x10003;	

	/******************************
	* 0x2000x: 
	* playback status
	* decoder is running
	******************************/
    public static final int PLAYER_RUNNING  	= 0x20001;
    public static final int PLAYER_BUFFERING 	= 0x20002;
    public static final int PLAYER_PAUSE    	= 0x20003;
    public static final int PLAYER_SEARCHING	= 0x20004;
	
    public static final int PLAYER_SEARCHOK 	= 0x20005;
    public static final int PLAYER_START    	= 0x20006;	
    public static final int PLAYER_FF_END   	= 0x20007;
    public static final int PLAYER_FB_END   	= 0x20008;

	/******************************
	* 0x3000x: 
	* player will exit	
	******************************/
    public static final int PLAYER_ERROR		= 0x30001;
    public static final int PLAYER_PLAYEND  	= 0x30002;	
    public static final int PLAYER_STOPED   	= 0x30003; 
	
	public static final int PLAYER_EXIT   		= 0x30004; 


    public static final int DIVX_AUTHOR_ERR     = 0x40001;
    public static final int DIVX_EXPIRED        = 0x40002;

	public static final int PLAYER_EVENTS_PLAYER_INFO=1;			///<ext1=player_info*,ext2=0,same as update_statue_callback 
	public static final int PLAYER_EVENTS_STATE_CHANGED=2;			///<ext1=new_state,ext2=0,
	public static final int PLAYER_EVENTS_ERROR=3;					///<ext1=error_code,ext2=message char *
	public static final int PLAYER_EVENTS_BUFFERING=4;				///<ext1=buffered=d,d={0-100},ext2=0,
	public static final int PLAYER_EVENTS_FILE_TYPE=5;				///<ext1=player_file_type_t*,ext2=0
}
