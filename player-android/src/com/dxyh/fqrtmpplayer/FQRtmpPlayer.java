package com.dxyh.fqrtmpplayer;

import com.dxyh.fqrtmpplayer.aidl.IClientCallback;
import com.dxyh.fqrtmpplayer.aidl.IPlayer;
import com.dxyh.fqrtmpplayer.gui.UiTools;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceView;

public class FQRtmpPlayer implements IFQRtmp {
    private static final String TAG = "FQRtmpPlayer";
    
    private static final String LHPLAYERSERVICE_ACTION = "com.dxyh.fqrtmpplayer.action.playerservice";
    private static final String LHPLAYERSERVICE_PKGNAME = "com.dxyh.fqrtmpplayer";
    
    private Activity mActivity;
    
    private SurfaceView mSurfaceView;
    
    private IPlayer mPlayer;
    
    private boolean mPlaybackStarted = false;
    
    private String mUrl;
    
	public FQRtmpPlayer(Activity activity, int layoutResID) {
	    mActivity = activity;
        mActivity.setContentView(layoutResID);
        
        mSurfaceView = (SurfaceView) mActivity.findViewById(R.id.player_surface);
	}
	
	private static final int ON_PREPARED = 1;
    private static final int ON_COMPLETION = 2;
    private static final int ON_BUFFERING = 3;
    private static final int ON_ERROR = 4;
	
	@SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler () {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case ON_PREPARED:
                Log.d(TAG, "Handling OnPrepared");
                try {
                    mPlayer.start();
                    Log.d(TAG, "Is Player playing? " + mPlayer.isPlaying());
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                break;
            case ON_COMPLETION:
                Log.i(TAG, "Handling OnCompletion");
                break;
            case ON_BUFFERING:
                Log.d(TAG, "Handling OnBuffering: " + msg.arg1);
                break;
            case ON_ERROR:
                Log.e(TAG, "Handling OnError: " + msg.arg1 + ", " + msg.arg2);
                break;
            default:
                super.handleMessage(msg);
            }
        }
    };
	
	private IClientCallback mClientCallback = new IClientCallback.Stub() {
        @Override
        public void onPrepared(IPlayer player) throws RemoteException {
            Log.i(TAG, "onPrepared");
            mHandler.sendEmptyMessage(ON_PREPARED);
        }

        @Override
        public void onCompletion(IPlayer player) throws RemoteException {
            Log.i(TAG, "onCompletion");
            mHandler.sendEmptyMessage(ON_COMPLETION);
        }

        @Override
        public void onBufferingUpdateListener(IPlayer player, int percent) throws RemoteException {
            Log.i(TAG, "onBufferingUpdateListener");
            Message msg = mHandler.obtainMessage(ON_BUFFERING, percent, 0);
            mHandler.sendMessage(msg);
        }

        @Override
        public boolean onError(IPlayer player, int what, int extra) throws RemoteException {
            Log.i(TAG, "onError");
            Message msg = mHandler.obtainMessage(ON_ERROR, what, extra);
            mHandler.sendMessage(msg);
            return true;
        }
    };
	
	@Override
	public void process(final String url) {
	    if (TextUtils.isEmpty(url)) {
	        UiTools.toast(mActivity, "Invalid play url", UiTools.SHORT_TOAST);
	        mActivity.setContentView(R.layout.info);
	        return;
	    }
	    mUrl = url;
	    bindToService();
	}
	
	private void bindToService() {
	    Log.d(TAG, "bindToService()");
        Intent intent = new Intent();
        intent.setAction(LHPLAYERSERVICE_ACTION);
        intent.setPackage(LHPLAYERSERVICE_PKGNAME);
        mActivity.bindService(intent, mServiceConn, Context.BIND_AUTO_CREATE);
    }

	@Override
	public void onResume() {
	}
	
	private ServiceConnection mServiceConn = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.i(TAG, "onServiceConnected");
            mPlayer = IPlayer.Stub.asInterface(service);
            
            try {
                mPlayer.create();
                mPlayer.registerClientCallback(mClientCallback);
                mPlayer.setDataSource(mUrl);
                mPlayer.setDisplay(mSurfaceView.getHolder().getSurface());
                mPlayer.prepare();
                mPlayer.start();
            } catch (RemoteException e) {
                e.printStackTrace();
            }

            mPlaybackStarted = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.i(TAG, "onServiceDisconnected");
            bindToService();
        }
    };

	@Override
	public void onPause() {
	    close();
	}
	
	@Override
	public void close() {
	    if (!mPlaybackStarted)
            return;
        
        if (mPlayer != null) {
            try {
                mPlayer.stop();
                mPlayer.release();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
            
            mActivity.unbindService(mServiceConn);
        }
        
        mActivity.setContentView(R.layout.info);

        mPlaybackStarted = false;
	}
}