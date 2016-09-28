package com.amlogic.libplayer;

import java.io.IOException;
import android.util.Log;
import java.lang.ref.WeakReference;

public class amavjni {
	private String TAG = "amavjniJ";

	static {
		System.loadLibrary("amavjni");
	}

	@SuppressWarnings( { "unused", "unchecked" })
	private static void postEventFromNative(Object mediaplayer_ref, int what,
			int arg1, int arg2, Object obj) {
		amavjni mp = (amavjni) ((WeakReference<Object>) mediaplayer_ref).get();
		if (mp == null) {
			return;
		}
		mp.handle_events(what, arg1, arg2);
	}

	public interface onStateChangeLisenser {
		void StateChanged(int msg, int ext1, int ext2);
	}

	private static int player;

	private onStateChangeLisenser monStateChangeLisenser;

	public amavjni() {
		Log.i(TAG, "amavjni");
		globalinit();
		player = newplayer();
	} 

	public int fffb(int speed) throws IllegalStateException {
		Log.i(TAG, "native_fffb");
		return native_fffb(player, speed);
	}

	public int getcurrenttime() throws IllegalStateException {
		Log.i(TAG, "native_getcurrenttime");
		return native_getcurrenttime(player);
	}

	public int getduration() throws IllegalStateException {
		Log.i(TAG, "native_getduration");
		return native_getduration(player);
	}

	public int getstate() throws IllegalStateException {
		Log.i(TAG, "native_getstate");
		return native_getstate(player);
	}

	private void globalinit() throws IllegalStateException {
		Log.i(TAG, "native_globalinit");
		native_globalinit();
	}

	public void handle_events(int what, int arg1, int arg2)
			throws IllegalStateException {
		Log.i(TAG, "handle_events:" + what + ":" + arg1 + ":" + arg2);
		if (monStateChangeLisenser != null)
			monStateChangeLisenser.StateChanged(what, arg1, arg2);
		return;
	}

	private native final int native_fffb(int player, int speed)
			throws IllegalStateException;

	private native final int native_getcurrenttime(int player)
			throws IllegalStateException;

	private native final int native_getduration(int player)
			throws IllegalStateException;

	private native final int native_getstate(int player)
			throws IllegalStateException;

	private native final int native_globalinit() throws IllegalStateException;

	private native final int native_newplayer(Object amavjni_this)
			throws IllegalStateException;

	private native final int native_pause(int player)
			throws IllegalStateException;

	private native final int native_playercmd(int player, String paras)
			throws IllegalStateException;

	private native final int native_prepare(int player) throws IOException,
			IllegalStateException;

	private native final int native_prepare_async(int player)
			throws IllegalStateException;

	private native final int native_release(int player)
			throws IllegalStateException;

	private native final int native_seekto(int player, int to_ms)
			throws IllegalStateException;

	private native final int native_setdatasource(int player, String path,
			String headers) throws IllegalStateException;

	private native final int native_start(int player)
			throws IllegalStateException;

	private native final int native_stop(int player)
			throws IllegalStateException;

	private native final int native_sysset(int player, String paras)
			throws IllegalStateException;

	private int newplayer() throws IllegalStateException {
		Log.i(TAG, "native_newplayer");
		return native_newplayer(new WeakReference<amavjni>(this));
	}

	public int pause() throws IllegalStateException {
		Log.i(TAG, "native_pause");
		return native_pause(player);
	}

	public int playercmd(String para) throws IllegalStateException {
		Log.i(TAG, "native_playercmd");
		return native_playercmd(player, para);
	}

	public int prepare() throws IOException, IllegalStateException {
		Log.i(TAG, "native_prepare");
		return native_prepare(player);
	}

	public int prepare_async() throws IllegalStateException {
		Log.i(TAG, "native_prepare_async");
		return native_prepare_async(player);
	}

	public int release() throws IllegalStateException {
		Log.i(TAG, "native_release");
		return native_release(player);
	}

	public int seekto(int to_ms) throws IllegalStateException {
		Log.i(TAG, "native_seekto");
		return native_seekto(player, to_ms);
	}

	public int setdatasource(String path, String headers)
			throws IllegalStateException {
		Log.i(TAG, "native_setdatasource");
		return native_setdatasource(player, path, headers);
	}

	public void setonStateChangeLisenser(onStateChangeLisenser listener) {
		monStateChangeLisenser = listener;
	}

	public int start() throws IllegalStateException {
		Log.i(TAG, "native_start");
		return native_start(player);
	}

	public int stop() throws IllegalStateException {
		Log.i(TAG, "native_stop");

		return native_stop(player);
	}

	public int sysset(String para) throws IllegalStateException {
		Log.i(TAG, "native_sysset");
		return native_sysset(player, para);
	}

}



