package com.dxyh.fqrtmpplayer;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.util.Log;

import com.dxyh.libfqrtmp.LibFQRtmp;

public class MyApplication extends Application implements LibFQRtmp.OnNativeCrashListener {
	private final static String TAG = "MyApplication";
    private static MyApplication instance;
    
    private ThreadPoolExecutor mThreadPool = new ThreadPoolExecutor(0, 3, 2, TimeUnit.SECONDS,
            new LinkedBlockingQueue<Runnable>());
    
    @Override
    public void onCreate() {
        super.onCreate();
        
        instance = this;
        
        LibFQRtmp.setOnNativeCrashListener(this);
    }
    
	@Override
	public void onNativeCrash() {
		Log.e(TAG, "FQRtmpPlayer crashed");
	}
    
    @Override
    public void onLowMemory() {
        super.onLowMemory();
        Log.w(TAG, "System is running low on memory");
    }
    
    public static MyApplication getInstance() {
    	return instance;
    }
    
    public static Context getAppContext() {
        return instance;
    }
    
    public static Resources getAppResources() {
        return instance.getResources();
    }
    
    public static void runBackground(Runnable runnable) {
        instance.mThreadPool.execute(runnable);
    }
}