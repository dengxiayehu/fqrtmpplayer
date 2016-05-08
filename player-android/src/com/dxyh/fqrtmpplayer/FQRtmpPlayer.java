package com.dxyh.fqrtmpplayer;

import android.app.Activity;

public class FQRtmpPlayer extends FQRtmpBase {
	public FQRtmpPlayer(Activity activity, int layoutResID) {
		super(activity, layoutResID);
	}
	
	@Override
	public void process(final String url) {
	}

	@Override
	public void onResume() {
	}

	@Override
	public void onPause() {
	}
	
	@Override
	public void close() {
		super.close();
	}
}