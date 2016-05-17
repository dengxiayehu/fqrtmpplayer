package com.dxyh.libfqrtmp;

import android.graphics.ImageFormat;
import android.media.AudioFormat;
import android.media.CamcorderProfile;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class LibFQRtmp {
    private static final String TAG = "LibFQRtmp";
    
    private VideoConfig mVideoConfig = new VideoConfig();
    private AudioConfig mAudioConfig = new AudioConfig();
    
    public class Rational {
    	public int num;
    	public int den;
    	
    	public Rational(int num, int den) {
    		this.num = num;
    		this.den = den;
    	}
    };
    
    public class VideoConfig {
    	private int mCamcorderProfileId = CamcorderProfile.QUALITY_LOW;
    	private final String mPreset = "ultrafast";
    	private final String mTune = "zerolatency";
    	private final String mProfile = "baseline";
    	private int mLevelIDC = -1;
    	private int mInputCSP = ImageFormat.NV21;
    	private int mBitrate = 200 * 1000; // Temporary fixed
    	private int mWidth = -1;
    	private int mHeight = -1;
    	private Rational mFPS = new Rational(12, 1); // Temporary fixed
    	private int mIFrameInterval = 3;
    	private final boolean mRepeatHeaders = true;
    	private final int mBFrames = 0;
    	private final boolean mDeblockingFilter = false;
    	
    	public int getCamcorderProfileId() {
    		return mCamcorderProfileId;
    	}
    	
    	public String getPreset() {
    		return mPreset;
    	}
    	
    	public String getTune() {
    		return mTune;
    	}
    	
    	public String getProfile() {
    		return mProfile;
    	}
    	
    	public int getLevelIDC() {
    		return mLevelIDC;
    	}
    	
    	public void setInputCSP(int csp) {
    		mInputCSP = csp;
    	}
    	public int getInputCSP() {
    		return mInputCSP;
    	}
    	
    	public void setWidth(int width) {
    		mWidth = width;
    	}
    	public int getWidth() {
    		return mWidth;
    	}
    	
    	public void setFPS(Rational r) {
    		mFPS = r;
    	}
    	public Rational getFPS() {
    		return mFPS;
    	}
    	public int getFPSInt() {
    		return mFPS.num / mFPS.den;
    	}
    	
    	public void setIFrameInterval(int interval) {
    		mIFrameInterval = interval;
    	}
    	public int getIFrameInterval() {
    		return mIFrameInterval;
    	}
    	
    	public void setHeight(int height) {
    		mHeight = height;
    	}
    	public int getHeight() {
    		return mHeight;
    	}
    	
    	public boolean getRepeatHeaders() {
    		return mRepeatHeaders;
    	}
    	
    	public int getBFrames() {
    		return mBFrames;
    	}
    	
    	public boolean getDeblockingFilter() {
    		return mDeblockingFilter;
    	}
    	
    	public void setBitrate(int bitrate) {
    		mBitrate = bitrate;
    	}
    	public int getBitrate() {
    		return mBitrate;
    	}
    }
    
    public class AudioConfig {
    	private int mSamplerateInHz = 11025;
    	private int mChannel = AudioFormat.CHANNEL_IN_STEREO;
    	private int mEncoding = AudioFormat.ENCODING_PCM_16BIT;
    	private int mBitrate = -1; // Auto
    	private int mAudioObjectType = 2; // AOT_AAC_LC
    	private int mFrameLength = -1;
    	private int mEncoderDelay = -1;
        private float mVolume = 0.8f;
    	
    	public int getAudioObjectType() {
    		return mAudioObjectType;
    	}
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
    	public int getBitrate() {
    		return mBitrate;
    	}
    	
    	public int getFrameLength() {
    		return mFrameLength;
    	}
    	
    	public int getEncoderDelay() {
    		return mEncoderDelay;
    	}

        public float getVolume() {
            return mVolume;
        }
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
    
    public native int sendRawAudio(byte[] data, int length);
    public native int sendRawVideo(byte[] data, int length);
    
    public native int openAudioEncoder(AudioConfig audioConfig);
    public native int closeAudioEncoder();
    
    public native int openVideoEncoder(VideoConfig videoConfig);
    public native int closeVideoEncoder();
    
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
