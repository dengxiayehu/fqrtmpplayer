package com.dxyh.fqrtmpplayer.gui;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;

import com.dxyh.fqrtmpplayer.FQRtmpPlayer;
import com.dxyh.fqrtmpplayer.FQRtmpPusher;
import com.dxyh.fqrtmpplayer.IFQRtmp;
import com.dxyh.fqrtmpplayer.MyApplication;
import com.dxyh.fqrtmpplayer.R;

public class MainActivity extends Activity {
    public static final String TAG = "MainActivity";
    
    private static final int KEYBACK_EXPIRE = 3000;
    private boolean mWaitingExit;
    
    private IFQRtmp mFQRtmp;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.info);
    }
    
    @Override
    protected void onResume() {
    	super.onResume();
    	
    	if (mFQRtmp != null) {
    		mFQRtmp.onResume();
    	}
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
    
    void handlePlay() {
    	UiTools.inputDialog(this,
                MyApplication.getAppResources().getString(R.string.action_play),
                "Any media uri",
                new UiTools.OnInputDialogClickListener() {
            @Override
            public void onClick(DialogInterface dialog, final String input) {
				resetFQRtmp();
				mFQRtmp = new FQRtmpPlayer(MainActivity.this, R.layout.player);
				mFQRtmp.process(input);
            }
        }, null);
    }
    
    void handleLive() {
    	UiTools.inputDialog(this,
                MyApplication.getAppResources().getString(R.string.action_live),
                "rtmp://...",
                new UiTools.OnInputDialogClickListener() {
            @Override
            public void onClick(DialogInterface dialog, final String input) {
            	resetFQRtmp();
            	mFQRtmp = new FQRtmpPusher(MainActivity.this, R.layout.live);
            	mFQRtmp.process(input);
            }
        }, null);
    }
    
    void handleAbout() {
        UiTools.toast(this, "Thanks for your attention.", UiTools.SHORT_TOAST);
        resetFQRtmp();
    }
    
    @Override
    protected void onPause() {
    	super.onPause();
    	
    	if (mFQRtmp != null) {
    		mFQRtmp.onPause();
    	}
    }
    
    private void resetFQRtmp() {
    	if (mFQRtmp != null) {
    		mFQRtmp.close();
    		mFQRtmp = null;
    	}
    }
    
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	
    	resetFQRtmp();
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (!mWaitingExit) {
                UiTools.toast(this,
                        MyApplication.getAppResources().getString(R.string.press_again_exit),
                        UiTools.SHORT_TOAST);
                mWaitingExit = true;
                Timer timer = new Timer();
                timer.schedule(new TimerTask() {
                    @Override
                    public void run() {
                        mWaitingExit = false;
                    }
                  }, KEYBACK_EXPIRE);
            } else {
                finish();
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}