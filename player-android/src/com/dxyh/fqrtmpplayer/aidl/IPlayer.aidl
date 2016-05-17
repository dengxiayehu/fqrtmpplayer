package com.dxyh.fqrtmpplayer.aidl;

import android.view.Surface;

import com.dxyh.fqrtmpplayer.aidl.IClientCallback;

interface IPlayer {
    void create();
    void setDataSource(String path);
    void prepare();
    void setDisplay(in Surface surf);
    void start();
    void stop();
    void pause();
    void release();
    int getVideoWidth();
    int getVideoHeight();
    boolean isPlaying();
    boolean isSeekable();
    int getCurrentPosition();
    int getDuration();
    long getTime();
    void setTime(long time);
    int getVolume();
    int setVolume(int volume);
    void setPosition(float pos);
    int getAudioTracksCount();
    int getAudioTrack();
    boolean setAudioTrack(int index);
    void seekTo(int delta);
    void selectTrack(int index);
    void registerClientCallback(IClientCallback cb);
}
