package com.dxyh.fqrtmpplayer;

import java.io.IOException;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.dxyh.libfqrtmp.Event;
import com.dxyh.libfqrtmp.LibFQRtmp;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

public class MainActivity extends Activity {
    public static final String TAG = "FQRtmpPlayer/MainActivity";
    
    private LibFQRtmp mLibFQRtmp;
    
    private static final int KEYBACK_EXPIRE = 3000;
    private boolean mWaitingExit;
    
    private SurfaceView mSurfaceView;
    private Camera mCamera;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        init();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.action_play:
            handlePlay();
            return true;
        case R.id.action_live:
            handleLive();
            return true;
        case R.id.action_about:
            handleAbout();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }
    
    private Event.Listener mListener = new Event.Listener() {
        @Override
        public void onEvent(Event event) {
            switch (event.type) {
            case Event.OPENING:
                MyApplication.runBackground(new Runnable() {
                    @Override
                    public void run() {
                        openCamera();
                    }
                });
                break;
            case Event.CONNECTED:
                break;
            case Event.ENCOUNTERED_ERROR:
            default:
                break;
            }
        }
    };
    
    private void init() {
        mSurfaceView = (SurfaceView) findViewById(R.id.surface_view);
        
        mLibFQRtmp = new LibFQRtmp();
        mLibFQRtmp.setEventListener(mListener);
    }
    
    private void handlePlay() {
    }
    
    private void handleLive() {
        Util.inputDialog(this,
                MyApplication.getAppResources().getString(R.string.action_live),
                new Util.OnInputDialogClickListener() {
            @Override
            public void onClick(DialogInterface dialog, final String input) {
                MyApplication.runBackground(new Runnable() {
                    @Override
                    public void run() {
                        if (input != null && input.startsWith("rtmp"))
                            mLibFQRtmp.start("-L " + input);
                        else
                            MainActivity.this.runOnUiThread(new Runnable () {
                                @Override
                                public void run() {
                                    Toast.makeText(MyApplication.getAppContext(),
                                            MyApplication.getAppResources().getString(R.string.invalid_rtmp_url),
                                            Toast.LENGTH_SHORT).show();
                                }});
                    }
                });
            }
        }, null);
    }
    
    private void handleAbout() {
        
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (!mWaitingExit) {
                Toast.makeText(MyApplication.getAppContext(),
                        MyApplication.getAppResources().getString(R.string.press_again_exit),
                        Toast.LENGTH_SHORT).show();
                mWaitingExit = true;
                Timer timer = new Timer();
                timer.schedule(new TimerTask() {
                    @Override
                    public void run() {
                        mWaitingExit = false;
                    }
                  }, KEYBACK_EXPIRE);
            } else
                finish();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.release();
        }
        
        mLibFQRtmp.stop();
    }
    

    private void openCamera() {
        try {
            mCamera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
        } catch (RuntimeException e) {
            e.printStackTrace();
            return;
        }
        Camera.Parameters parameters = mCamera.getParameters();
        
        parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
        parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        parameters.setSceneMode(Camera.Parameters.SCENE_MODE_AUTO);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
        parameters.setPreviewFormat(ImageFormat.YV12);
        Camera.Size size = findBestPreviewSize(mCamera, mSurfaceView);
        if (size != null)
            parameters.setPreviewSize(size.width, size.height);
        mCamera.setParameters(parameters);
        
        byte buffer[] = new byte[getYUVBuffer(size.width, size.height)];
        mCamera.addCallbackBuffer(buffer);
        mCamera.setPreviewCallbackWithBuffer(onYUVFrame);
        try {
            mCamera.setPreviewDisplay(mSurfaceView.getHolder());
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }
        mCamera.setDisplayOrientation(90);
        mCamera.startPreview();
    }
    
    private Camera.Size findBestPreviewSize(Camera mCamera, View view) {
        List<Camera.Size> sizeList = mCamera.getParameters().getSupportedPreviewSizes();
        if (sizeList == null || sizeList.isEmpty()) {
            return null;
        }
        final int viewWid = view.getWidth();
        final int viewHei = view.getHeight();
        final int viewArea = viewWid * viewHei;
        final int length = sizeList.size();
        Log.d(TAG, "findBestPreviewSize viewWid=" + viewWid + " viewHei=" + viewHei);
        Camera.Size resultSize = null;
        int deltaArea = 0;
        for (int i = 0; i < length; i++) {
            Camera.Size size = sizeList.get(i);
            final int area = size.width * size.height;
            final int delta = Math.abs(area - viewArea);
            if (deltaArea == 0 || delta < deltaArea) {
                deltaArea = delta;
                resultSize = size;
            }
        }
        return resultSize;
    }
    
    private int getYUVBuffer(int width, int height) {
        int stride = (int) Math.ceil(width / 16.0) * 16;
        int y_size = stride * height;
        int c_stride = (int) Math.ceil(width / 32.0) * 16;
        int c_size = c_stride * height / 2;
        return y_size + c_size * 2;
    }
    
    final Camera.PreviewCallback onYUVFrame = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
        }
    };
}