package com.dxyh.fqrtmpplayer;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
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
    
    private void handlePlay() {
        
    }
    
    private void handleLive() {
    }
    
    private void handleAbout() {
        
    }
}
