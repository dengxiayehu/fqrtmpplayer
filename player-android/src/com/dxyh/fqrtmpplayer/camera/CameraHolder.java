package com.dxyh.fqrtmpplayer.camera;

import static com.dxyh.fqrtmpplayer.util.Util.Assert;

import java.io.IOException;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

@SuppressWarnings("deprecation")
public class CameraHolder {
    private static final String TAG = "CameraHolder";
    
    private android.hardware.Camera mCameraDevice;
    private long mKeepBeforeTime = 0;
    private final Handler mHandler;
    private int mUsers = 0;
    private int mNumberOfCameras;
    private int mCameraId = -1;
    private CameraInfo[] mInfo;
    
    private Parameters mParameters;
    
    private static CameraHolder sHolder;
    public static synchronized CameraHolder instance() {
        if (sHolder == null) {
            sHolder = new CameraHolder();
        }
        return sHolder;
    }
    
    private static final int RELEASE_CAMERA = 1;
    private class MyHandler extends Handler {
        MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case RELEASE_CAMERA:
                    synchronized (CameraHolder.this) {
                        if (CameraHolder.this.mUsers == 0)
                            releaseCamera();
                    }
                    break;
            }
        }
    }
    
    private CameraHolder() {
        HandlerThread ht = new HandlerThread("CameraHolder");
        ht.start();
        mHandler = new MyHandler(ht.getLooper());
        mNumberOfCameras = android.hardware.Camera.getNumberOfCameras();
        mInfo = new CameraInfo[mNumberOfCameras];
        for (int i = 0; i < mNumberOfCameras; ++i) {
            mInfo[i] = new CameraInfo();
            android.hardware.Camera.getCameraInfo(i, mInfo[i]);
        }
    }
    
    public int getNumberOfCameras() {
        return mNumberOfCameras;
    }

    public CameraInfo[] getCameraInfo() {
        return mInfo;
    }
    
    public synchronized android.hardware.Camera open(int cameraId)
            throws CameraHardwareException {
        Assert(mUsers == 0);
        if (mCameraDevice != null && mCameraId != cameraId) {
            mCameraDevice.release();
            mCameraDevice = null;
            mCameraId = -1;
        }
        if (mCameraDevice == null) {
            try {
                Log.d(TAG, "Open camera " + cameraId);
                mCameraDevice = android.hardware.Camera.open(cameraId);
                mCameraId = cameraId;
            } catch (RuntimeException e) {
                Log.e(TAG, "Fail to connect Camera", e);
                throw new CameraHardwareException(e);
            }
            mParameters = mCameraDevice.getParameters();
        } else {
            try {
                mCameraDevice.reconnect();
            } catch (IOException e) {
                Log.e(TAG, "Reconnect failed");
                throw new CameraHardwareException(e);
            }
            mCameraDevice.setParameters(mParameters);
        }
        ++mUsers;
        mHandler.removeMessages(RELEASE_CAMERA);
        mKeepBeforeTime = 0;
        return mCameraDevice;
    }
    
    public synchronized android.hardware.Camera tryOpen(int cameraId) {
        try {
            return mUsers == 0 ? open(cameraId) : null;
        } catch (CameraHardwareException e) {
            return null;
        }
    }
    
    public synchronized void release() {
        Assert(mUsers == 1);
        --mUsers;
        mCameraDevice.stopPreview();
        releaseCamera();
    }
    
    private synchronized void releaseCamera() {
        Assert(mUsers == 0);
        Assert(mCameraDevice != null);
        long now = System.currentTimeMillis();
        if (now < mKeepBeforeTime) {
            mHandler.sendEmptyMessageDelayed(RELEASE_CAMERA,
                    mKeepBeforeTime - now);
            return;
        }
        mCameraDevice.release();
        mCameraDevice = null;
        mCameraId = -1;
    }
    
    public synchronized void keep() {
        Assert(mUsers == 1 || mUsers == 0);
        mKeepBeforeTime = System.currentTimeMillis() + 3000;
    }
    
    public static int getBackFacingCameraId() {
        int numOfCameras = CameraHolder.instance().getNumberOfCameras();
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        for (int idx = 0; idx < numOfCameras; ++idx) {
            Camera.getCameraInfo(idx, cameraInfo);
            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK)
                return idx;
        }
        return -1;
    }
    
    public static int getFrontFacingCameraId() {
        int numOfCameras = CameraHolder.instance().getNumberOfCameras();
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        for (int idx = 0; idx < numOfCameras; ++idx) {
            Camera.getCameraInfo(idx, cameraInfo);
            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT)
                return idx;
        }
        return -1;
    }
}