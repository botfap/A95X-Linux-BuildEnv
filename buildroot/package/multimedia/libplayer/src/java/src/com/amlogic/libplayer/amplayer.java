package com.amlogic.libplayer;

import java.io.IOException;

import com.amlogic.libplayer.amavjni.onStateChangeLisenser;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class amplayer {
	private String TAG = "amplayerJ";
	private static amavjni mavjni;
	private static boolean isPlayering;
	private onStateChangeLisenser mLisenser;
	private static int LOCAL_MSG=1;
	private static int LOWLEVEL_MSG=2;
	
	private amavjni.onStateChangeLisenser monStateChangeLisenser = new amavjni.onStateChangeLisenser() {
		public void StateChanged(int msg, int ext1, int ext2) {
			Log.i(TAG, "get Msg:" + msg + ":" + ext1 + ":" + ext2);
			Message newmsg = mHandler.obtainMessage(msg);
			newmsg.arg1=ext1;
			newmsg.arg2=ext2;
			mHandler.sendMessage(newmsg);
		}
	};
	private void Notify(int msg, int ext1, int ext2){
		if(mLisenser!=null)
			mLisenser.StateChanged(msg,ext1,ext2);
	}
	private Handler mHandler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				int pos;
				switch (msg.what) {
					case amplayer_info.PLAYER_EVENTS_PLAYER_INFO:
						Notify(msg.what,msg.arg1,msg.arg2);
						break;
					case  amplayer_info.PLAYER_EVENTS_STATE_CHANGED:
						Notify(msg.what,msg.arg1,msg.arg2);
						break;
					case  amplayer_info.PLAYER_EVENTS_ERROR:
						Notify(msg.what,msg.arg1,msg.arg2);
						break;	
					case  amplayer_info.PLAYER_EVENTS_BUFFERING:
						Notify(msg.what,msg.arg1,msg.arg2);
						break;
					case  amplayer_info.PLAYER_EVENTS_FILE_TYPE:
						Notify(msg.what,msg.arg1,msg.arg2);
						break;		
				}
			}
	};

	public interface onStateChangeLisenser {
		void StateChanged(int msg, int ext1, int ext2);
	}
	public void setonStateChangeLisenser(onStateChangeLisenser listener) {
		mLisenser = listener;
	}
	
	public amplayer() {
		Log.i(TAG, "start amavjni");
		isPlayering = false;
		mavjni = new amavjni();
		mavjni.setonStateChangeLisenser(monStateChangeLisenser);
	}

	public int Fffb(int speed) {
		mavjni.fffb(speed);
		return 0;
	}

	public int Getcurrenttime() {
		return mavjni.getcurrenttime();
	}

	public int Getduration() {
		return mavjni.getduration();
	}

	public boolean isPlaying() {
		return isPlayering;
	}

	public int Pause() {
		isPlayering = false;
		mavjni.pause();
		return 0;
	}

	public int Play(String url, String header) {
		if (url == null) {
			return -1;
		}
		Log.i(TAG, "Play" + url);
		mavjni.setdatasource(url, header);
		try {
			mavjni.prepare();
		} catch (IOException e) {
			Log.w(TAG, "av player prepare failed", e);
			return -1;
		}
		mavjni.playercmd("AMPLAYER:VIDEO_ON");
		mavjni.start();
		isPlayering=true;
		return 0;
	}

	public int SeekTo(int mS) {
		mavjni.seekto(mS);
		return 0;
	}

	public int Start() {
		mavjni.start();
		isPlayering = true;
		return 0;
	}

	public int Stop() {
		mavjni.playercmd("AMPLAYER:VIDEO_OFF");
		mavjni.stop();
		isPlayering = false;
		mavjni.release();
		return 0;
	}
}


