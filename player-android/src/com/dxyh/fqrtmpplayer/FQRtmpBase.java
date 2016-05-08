package com.dxyh.fqrtmpplayer;

import com.dxyh.libfqrtmp.LibFQRtmp;

import android.app.Activity;

public abstract class FQRtmpBase {
	protected Activity mActivity;
	protected LibFQRtmp mLibFQRtmp;
	
	public FQRtmpBase(Activity activity, int layoutResID) {
		mActivity = activity;
		mActivity.setContentView(layoutResID);
		
		mLibFQRtmp = new LibFQRtmp();
	}
	
	public abstract void process(final String url);
	public abstract void onResume();
	public abstract void onPause();
	public void close() {
		mLibFQRtmp.stop();
		mLibFQRtmp = null;
		
		mActivity.setContentView(R.layout.info);
		mActivity = null;
	}
}