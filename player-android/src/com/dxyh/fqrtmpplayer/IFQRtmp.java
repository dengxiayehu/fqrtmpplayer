package com.dxyh.fqrtmpplayer;

public interface IFQRtmp {
	public void process(final String url);
	public void onResume();
	public void onPause();
	public void close();
}