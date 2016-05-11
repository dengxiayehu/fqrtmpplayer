package com.dxyh.fqrtmpplayer;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.media.AudioFormat;
import android.media.CamcorderProfile;
import android.util.Log;

public class MyApplication extends Application {
	private final static String TAG = "MyApplication";
    private static MyApplication instance;
    
    private VideoConfig mVideoConfig = new VideoConfig();
    private AudioConfig mAudioConfig = new AudioConfig();
    
    public class VideoConfig {
    	private int mCamcorderProfileId = CamcorderProfile.QUALITY_HIGH;
    	
    	public int getCamcorderProfileId() {
    		return mCamcorderProfileId;
    	}
    }
    
    public class AudioConfig {
    	private int mSamplerateInHz = 11025;
    	private int mChannel = AudioFormat.CHANNEL_IN_MONO;
    	private int mEncoding = AudioFormat.ENCODING_PCM_16BIT;
    	
    	public int getSamplerate() {
    		return mSamplerateInHz;
    	}
    	public void setSamplerate(int samplerate) {
    		mSamplerateInHz = samplerate;
    	}
    	public int getChannel() {
    		return mChannel;
    	}
    	public int getChannelCount() {
    		switch (mChannel) {
    		case AudioFormat.CHANNEL_IN_MONO:
    			return 1;
    		case AudioFormat.CHANNEL_IN_STEREO:
    			return 2;
			default:
				return -1;
    		}
    	}
    	public int getEncoding() {
    		return mEncoding;
    	}
    	public int getBitsPerSample() {
    		switch (mEncoding) {
    		case AudioFormat.ENCODING_PCM_16BIT:
    			return 16;
    		case AudioFormat.ENCODING_PCM_8BIT:
    			return 8;
			default:
				return -1;
    		}
    	}
    }
    
    private ThreadPoolExecutor mThreadPool = new ThreadPoolExecutor(0, 3, 2, TimeUnit.SECONDS,
            new LinkedBlockingQueue<Runnable>());
    
    @Override
    public void onCreate() {
        super.onCreate();
        
        instance = this;
    }
    
	public VideoConfig getVideoConfig() {
	    return mVideoConfig;
	}
	public void setVideoConfig(VideoConfig videoConfig) {
		mVideoConfig = videoConfig;
	}
	
	public AudioConfig getAudioConfig() {
	    return mAudioConfig;
	}
	public void setAudioConfig(AudioConfig audioConfig) {
		mAudioConfig = audioConfig;
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