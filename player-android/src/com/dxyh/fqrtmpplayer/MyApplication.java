package com.dxyh.fqrtmpplayer;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.util.Log;

public class MyApplication extends Application {
    public final static String TAG = "MyApplication";
    private static MyApplication instance;
    
    private ThreadPoolExecutor mThreadPool = new ThreadPoolExecutor(0, 2, 2, TimeUnit.SECONDS,
            new LinkedBlockingQueue<Runnable>());
    
    @Override
    public void onCreate() {
        super.onCreate();
        
        instance = this;
    }
    
    @Override
    public void onLowMemory() {
        super.onLowMemory();
        Log.w(TAG, "System is running low on memory");
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