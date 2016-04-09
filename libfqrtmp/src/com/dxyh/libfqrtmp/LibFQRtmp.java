package com.dxyh.libfqrtmp;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class LibFQRtmp {
    private static final String TAG = "LibFQRtmp";
    
    public static final int OPENING = 0;
    public static final int CONNECTED = 1;
    public static final int PLAYING = 2;
    public static final int PUSHING = 3;
    public static final int ENCOUNTERED_ERROR = 4;
    
    private Event.Listener mEventListener = null;
    private Handler mHandler = null;
    private synchronized void dispatchEventFromNative(int eventType, long arg1, String arg2) {
        final Event event = new Event(eventType, arg1, arg2);
        
        class EventRunnable implements Runnable {
            private final Event.Listener listener;
            private final Event event;
            
            private EventRunnable(Event.Listener listener, Event event) {
                this.listener = listener;
                this.event = event;
            }
            @Override
            public void run() {
                listener.onEvent(event);
            }
        }
        
        if (mEventListener != null && mHandler != null)
            mHandler.post(new EventRunnable(mEventListener, event));
    }
    
    public synchronized void setEventListener(Event.Listener listener) {
        if (mHandler != null)
            mHandler.removeCallbacksAndMessages(null);
        mEventListener = listener;
        if (mEventListener != null && mHandler == null)
            mHandler = new Handler(Looper.getMainLooper());
    }
    
    public LibFQRtmp() {
    }
    
    public void start(String cmdline) {
        nativeNew(cmdline);
    }
    public void stop() {
        nativeRelease();
    }
    
    public native String version();
    private native void nativeNew(String cmdline);
    private native void nativeRelease();
    
    private static OnNativeCrashListener sOnNativeCrashListener;
    
    public static interface OnNativeCrashListener {
        public void onNativeCrash();
    }
    
    public static void setOnNativeCrashListener(OnNativeCrashListener l) {
        sOnNativeCrashListener = l;
    }

    private static void onNativeCrash() {
        if (sOnNativeCrashListener != null)
            sOnNativeCrashListener.onNativeCrash();
    }
    
    static {
        try {
            System.loadLibrary("rtmp");
            System.loadLibrary("fqrtmpjni");
        } catch (UnsatisfiedLinkError ule) {
            Log.e(TAG, "Can't load libraries: " + ule);
            System.exit(1);
        } catch (SecurityException se) {
            Log.e(TAG, "Encountered as security issue when loading libraries: " + se);
            System.exit(1);
        }
    }
}
