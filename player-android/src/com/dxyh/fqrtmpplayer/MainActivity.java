package com.dxyh.fqrtmpplayer;

import java.util.Timer;
import java.util.TimerTask;

import com.dxyh.libfqrtmp.Event;
import com.dxyh.libfqrtmp.LibFQRtmp;

import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

public class MainActivity extends Activity {
    public static final String TAG = "FQRtmpPlayer/MainActivity";
    
    private LibFQRtmp mLibFQRtmp;
    
    private static final int KEYBACK_EXPIRE = 3000;
    private boolean mWaitingExit;
    
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
            Toast.makeText(getApplicationContext(), "event.type=" + event.type, Toast.LENGTH_SHORT).show();
        }
    };
    
    private void init() {
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
        mLibFQRtmp.stop();
    }
}
