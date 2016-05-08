package com.dxyh.fqrtmpplayer;

import java.util.List;

import com.dxyh.fqrtmpplayer.camera.CameraHardwareException;
import com.dxyh.fqrtmpplayer.camera.CameraHolder;
import com.dxyh.fqrtmpplayer.gui.PreviewFrameLayout;
import com.dxyh.fqrtmpplayer.gui.UiTools;
import com.dxyh.fqrtmpplayer.util.Util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.CamcorderProfile;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

@SuppressWarnings("deprecation")
public class FQRtmpPusher extends FQRtmpBase implements SurfaceHolder.Callback, SensorEventListener {
    private static final String TAG = "FQRtmpPusher";
    
    private static final int FIRST_TIME_INIT = 1;
    private static final int CLEAR_SCREEN_DELAY = 2;
    private static final int SET_CAMERA_PARAMETERS_WHEN_IDLE = 3;
    
    private static final int UPDATE_PARAM_INITIALIZE = 1;
    private static final int UPDATE_PARAM_PREFERENCE = 2;
    private static final int UPDATE_PARAM_ALL = -1;
    
    private static final float ACCEL_THRESHOLD = .5f;
    
    private int mUpdateSet;
    
    private static final float DEFAULT_CAMERA_BRIGHTNESS = 0.7f;
    
    private static final int SCREEN_DELAY = 2 * 60 * 1000;
    
    private Parameters mParameters;
    
    private MyOrientationEventListener mOrientationListener;
    private int mOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    private int mOrientationCompensation = 0;

    private Camera mCameraDevice;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder = null;
    
    private CamcorderProfile mProfile;
    
    private int mCameraId;
    
    private boolean mStartPreviewFail = false;
    
    private boolean mPreviewing;
    private boolean mPausing;
    private boolean mFirstTimeInitialized;
    
    private final ErrorCallback mErrorCallback = new ErrorCallback();
    
    private static final int FOCUS_NOT_STARTED = 0;
    private static final int FOCUSING = 1;
    private static final int FOCUS_SUCCESS = 2;
    private static final int FOCUS_FAIL = 3;
    private int mFocusState = FOCUS_NOT_STARTED;
    
    private final AutoFocusCallback mAutoFocusCallback =
            new AutoFocusCallback();
    
    private String mFocusMode;
    
    private SensorManager mSensorManager;
    private final Sensor mAccel;
    
    private SensorEvent mSensorEvent;
    
    private final Handler mHandler = new MainHandler();
    
    @SuppressLint("HandlerLeak")
    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CLEAR_SCREEN_DELAY: {
                    mActivity.getWindow().clearFlags(
                            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    break;
                }

                case FIRST_TIME_INIT: {
                    initializeFirstTime();
                    break;
                }
                
                case SET_CAMERA_PARAMETERS_WHEN_IDLE: {
                    setCameraParametersWhenIdle(0);
                    break;
                }
            }
        }
    }
    
	public FQRtmpPusher(Activity activity, int layoutResID) {
        super(activity, layoutResID);
        
        mSurfaceView = (SurfaceView) mActivity.findViewById(R.id.camera_preview);
        
        mCameraId = CameraHolder.getBackFacingCameraId();
        
        mSensorManager = (SensorManager) mActivity.getSystemService(Activity.SENSOR_SERVICE);
        mAccel = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
	}
	
	@Override
	public void process(final String url) {
	    Log.d(TAG, "process");
	    
	    Thread startPreviewThread = new Thread(new Runnable() {
            public void run() {
                try {
                    mStartPreviewFail = false;
                    startPreview();
                } catch (CameraHardwareException e) {
                    mStartPreviewFail = true;
                }
            }
        });
        startPreviewThread.start();
        
        SurfaceHolder holder = mSurfaceView.getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        
        try {
            startPreviewThread.join();
            if (mStartPreviewFail) {
                Log.e(TAG, "startPreview failed in thread");
                return;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        
        onResume();
	}
	
	@Override
	public void onResume() {
	    Log.d(TAG, "onResume");
	    
	    mPausing = false;
	    
	    if (!mPreviewing && !mStartPreviewFail) {
            if (!restartPreview()) return;
        }
	    
	    if (mSurfaceHolder != null) {
            if (!mFirstTimeInitialized) {
                mHandler.sendEmptyMessage(FIRST_TIME_INIT);
            } else {
                initializeSecondTime();
            }
        }
        keepScreenOnAwhile();
        
        mSensorManager.registerListener(this, mAccel, SensorManager.SENSOR_DELAY_UI);
	}
	
	private void resetScreenOn() {
        mHandler.removeMessages(CLEAR_SCREEN_DELAY);
        mActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
	
	private void keepScreenOnAwhile() {
        mHandler.removeMessages(CLEAR_SCREEN_DELAY);
        mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mHandler.sendEmptyMessageDelayed(CLEAR_SCREEN_DELAY, SCREEN_DELAY);
    }

	@Override
	public void onPause() {
	    Log.d(TAG, "onPause");

	    mPausing = true;
        stopPreview();
        closeCamera();
        
        resetScreenOn();
        
        if (mFirstTimeInitialized) {
            mOrientationListener.disable();
        }
        
        mSensorManager.unregisterListener(this);
        
        mHandler.removeMessages(FIRST_TIME_INIT);
	}
	
	@Override
	public void close() {
	    Log.d(TAG, "close");
	    
	    if (!mPausing) {
	        onPause();
	    }
	    
	    closeCamera();
	    
	    super.close();
	}
	
	private void ensureCameraDevice() throws CameraHardwareException {
        if (mCameraDevice == null) {
            mCameraDevice = CameraHolder.instance().open(mCameraId);
        }
    }
	
	private void startPreview() throws CameraHardwareException {
        if (mPausing || mActivity.isFinishing()) return;

        ensureCameraDevice();

        if (mPreviewing) stopPreview();

        setPreviewDisplay(mSurfaceHolder);
        Util.setCameraDisplayOrientation(mActivity, mCameraId, mCameraDevice);
        setCameraParameters(UPDATE_PARAM_ALL);

        mCameraDevice.setErrorCallback(mErrorCallback);

        try {
            Log.d(TAG, "startPreview");
            mCameraDevice.startPreview();
        } catch (Throwable ex) {
            closeCamera();
            throw new RuntimeException("startPreview failed", ex);
        }

        autoFocus();

        mPreviewing = true;
    }
	
	private void stopPreview() {
        if (mCameraDevice != null && mPreviewing) {
            Log.d(TAG, "stopPreview");
            cancelAutoFocus();
            mCameraDevice.stopPreview();
        }
        mPreviewing = false;
    }
	
	private boolean restartPreview() {
        try {
            startPreview();
        } catch (CameraHardwareException e) {
            Log.d(TAG, "startPreview failed");
            return false;
        }
        return true;
    }
	
	private void setPreviewDisplay(SurfaceHolder holder) {
        try {
            mCameraDevice.setPreviewDisplay(holder);
        } catch (Throwable ex) {
            closeCamera();
            throw new RuntimeException("setPreviewDisplay failed", ex);
        }
    }
	
	private void closeCamera() {
        if (mCameraDevice != null) {
            CameraHolder.instance().release();
            mCameraDevice.setZoomChangeListener(null);
            mCameraDevice = null;
            mPreviewing = false;
        }
    }
	
	private void initializeFirstTime() {
	    if (mFirstTimeInitialized) return;
	    
	    mOrientationListener = new MyOrientationEventListener(mActivity);
        mOrientationListener.enable();
        
        initializeScreenBrightness();
        mFirstTimeInitialized = true;
	}
	
	private void initializeScreenBrightness() {
	    UiTools.adjust_brightness(mActivity, DEFAULT_CAMERA_BRIGHTNESS);
    }
	
	private void initializeSecondTime() {
	    mOrientationListener.enable();
	}
	
	private void setCameraParameters(int updateSet) {
        mParameters = mCameraDevice.getParameters();

        if ((updateSet & UPDATE_PARAM_INITIALIZE) != 0) {
            updateCameraParametersInitialize();
        }

        if ((updateSet & UPDATE_PARAM_PREFERENCE) != 0) {
            updateCameraParametersPreference();
        }

        mCameraDevice.setParameters(mParameters);
    }
	
	private int chooseQuality() {
	    return CamcorderProfile.QUALITY_HIGH;
	}
	
	private void updateCameraParametersInitialize() {
	    mProfile = CamcorderProfile.get(mCameraId, chooseQuality());
	    
        List<Integer> frameRates = mParameters.getSupportedPreviewFrameRates();
        if (frameRates != null && frameRates.contains(Integer.valueOf(mProfile.videoFrameRate))) {
            Log.d(TAG, "set fps " + mProfile.videoFrameRate);
            mParameters.setPreviewFrameRate(mProfile.videoFrameRate);
        }
    }
	
	private static boolean isSupported(String value, List<String> supported) {
        return supported == null ? false : supported.indexOf(value) >= 0;
    }
	
	private void updateCameraParametersPreference() {
	    PreviewFrameLayout frameLayout =
                (PreviewFrameLayout) mActivity.findViewById(R.id.frame_layout);
        frameLayout.setAspectRatio((double) mProfile.videoFrameWidth / mProfile.videoFrameHeight);
        
        mParameters = mCameraDevice.getParameters();

        mParameters.setPreviewSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
        mParameters.setPreviewFrameRate(mProfile.videoFrameRate);
        
        String sceneMode = Camera.Parameters.SCENE_MODE_AUTO;
        if (isSupported(sceneMode, mParameters.getSupportedSceneModes())) {
            mParameters.setSceneMode(sceneMode);
        }
        
        String focusMode = Camera.Parameters.FOCUS_MODE_AUTO;
        if (isSupported(focusMode, mParameters.getSupportedFocusModes())) {
            mFocusMode = focusMode;
            mParameters.setFocusMode(focusMode);
        } else {
            mFocusMode = mParameters.getFocusMode();
        }

        String flashMode = Camera.Parameters.FLASH_MODE_OFF;
        if (isSupported(flashMode, mParameters.getSupportedFlashModes())) {
            mParameters.setFlashMode(flashMode);
        }

        String whiteBalance = Camera.Parameters.WHITE_BALANCE_AUTO;
        if (isSupported(whiteBalance, mParameters.getSupportedWhiteBalance())) {
            mParameters.setWhiteBalance(whiteBalance);
        }

        String colorEffect = Camera.Parameters.EFFECT_NONE;
        if (isSupported(colorEffect, mParameters.getSupportedColorEffects())) {
            mParameters.setColorEffect(colorEffect);
        }

        mCameraDevice.setParameters(mParameters);
        mParameters = mCameraDevice.getParameters();
	}
	
	private boolean isCameraIdle() {
        return mCameraDevice != null && mFocusState != FOCUSING;
    }
	
	private void setCameraParametersWhenIdle(int additionalUpdateSet) {
        mUpdateSet |= additionalUpdateSet;
        if (mCameraDevice == null) {
            mUpdateSet = 0;
            return;
        } else if (isCameraIdle()) {
            setCameraParameters(mUpdateSet);
            mUpdateSet = 0;
        } else {
            if (!mHandler.hasMessages(SET_CAMERA_PARAMETERS_WHEN_IDLE)) {
                mHandler.sendEmptyMessageDelayed(
                        SET_CAMERA_PARAMETERS_WHEN_IDLE, 1000);
            }
        }
    }
	
    private static final class ErrorCallback implements Camera.ErrorCallback {
        public void onError(int error, Camera camera) {
            if (error == Camera.CAMERA_ERROR_SERVER_DIED) {
                Log.e(TAG, "media server died");
            }
        }
    }
    
    public static int roundOrientation(int orientation) {
        return ((orientation + 45) / 90 * 90) % 360;
    }

    private class MyOrientationEventListener
            extends OrientationEventListener {
        public MyOrientationEventListener(Context context) {
            super(context);
        }

        @Override
        public void onOrientationChanged(int orientation) {
            if (orientation == ORIENTATION_UNKNOWN) return;
            mOrientation = roundOrientation(orientation);
            int orientationCompensation = mOrientation
                    + Util.getDisplayRotation(mActivity);
            if (mOrientationCompensation != orientationCompensation) {
                mOrientationCompensation = orientationCompensation;
                setOrientationIndicator(mOrientationCompensation);
            }
        }
    }

    private void setOrientationIndicator(int degree) {
    }
    
    private void autoFocus() {
        if (!(mFocusMode.equals(Parameters.FOCUS_MODE_INFINITY) ||
              mFocusMode.equals(Parameters.FOCUS_MODE_FIXED) ||
              mFocusMode.equals(Parameters.FOCUS_MODE_EDOF))) {
            Log.d(TAG, "autoFocus");
            
            mFocusState = FOCUSING;
            mCameraDevice.autoFocus(mAutoFocusCallback);
        }
    }
    
    private void cancelAutoFocus() {
        if (mFocusState == FOCUSING || mFocusState == FOCUS_SUCCESS || mFocusState == FOCUS_FAIL) {
            if (!(mFocusMode.equals(Parameters.FOCUS_MODE_INFINITY) ||
                  mFocusMode.equals(Parameters.FOCUS_MODE_FIXED) ||
                  mFocusMode.equals(Parameters.FOCUS_MODE_EDOF))) {
                Log.d(TAG, "cancelAutoFocus");
                
                mCameraDevice.cancelAutoFocus();
            }
        }
        mFocusState = FOCUS_NOT_STARTED;
    }
    
    private final class AutoFocusCallback implements android.hardware.Camera.AutoFocusCallback {
        public void onAutoFocus(boolean focused, android.hardware.Camera camera) {
            Log.v(TAG, "onAutoFocus: " + focused);
            mFocusState = focused ? FOCUS_SUCCESS : FOCUS_FAIL;
        }
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.d(TAG, "surfaceChanged(format=" + format + ", w=" + w + ", h=" + h + ")");
        
        if (holder.getSurface() == null) {
            Log.d(TAG, "holder.getSurface() == null");
            return;
        }

        mSurfaceHolder = holder;

        if (mCameraDevice == null) return;

        if (mPausing || mActivity.isFinishing()) return;

        if (mPreviewing && holder.isCreating()) {
            setPreviewDisplay(holder);
        } else {
            restartPreview();
        }

        if (!mFirstTimeInitialized) {
            mHandler.sendEmptyMessage(FIRST_TIME_INIT);
        } else {
            initializeSecondTime();
        }
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();
        mSurfaceHolder = null;
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.d(TAG, "onAccuracyChanged accuracy=" + accuracy);
    }
    
    @Override
    public void onSensorChanged(SensorEvent event) {
        if (mSensorEvent == null) {
            mSensorEvent = event;
        } else if (Math.abs(event.values[0] - mSensorEvent.values[0]) > ACCEL_THRESHOLD ||
                   Math.abs(event.values[1] - mSensorEvent.values[1]) > ACCEL_THRESHOLD ||
                   Math.abs(event.values[2] - mSensorEvent.values[2]) > ACCEL_THRESHOLD) {
            if (mFocusState != FOCUSING) {
                autoFocus();
            }
            mSensorEvent = event;
        }
    }
}