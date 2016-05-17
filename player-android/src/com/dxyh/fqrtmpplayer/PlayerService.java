package com.dxyh.fqrtmpplayer;

import java.io.IOException;

import com.dxyh.fqrtmpplayer.aidl.IClientCallback;
import com.dxyh.fqrtmpplayer.aidl.IPlayer;
import com.dxyh.fqrtmpplayer.player.VLCPlayer;
import com.dxyh.fqrtmpplayer.player.VLCPlayer.OnBufferingUpdateListener;
import com.dxyh.fqrtmpplayer.player.VLCPlayer.OnCompletionListener;
import com.dxyh.fqrtmpplayer.player.VLCPlayer.OnErrorListener;
import com.dxyh.fqrtmpplayer.player.VLCPlayer.OnPreparedListener;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;

public class PlayerService extends Service {
    private static final String TAG = "PlayerService";
    
    private IPlayer.Stub mBinder;

    private enum State {
        UNKNOWN, IDLE, INITIALIZED, PREPARED, STARTED, PAUSED, STOPPED, ERROR
    }

    private State mState = State.UNKNOWN;

    @Override
    public void onCreate() {
        super.onCreate();
        mBinder = new MyBinder();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    private VLCPlayer mPlayer;
    private IClientCallback mClientCb;

    private void infoOnPrepared() {
        if (mClientCb != null) {
            try {
                Log.d(TAG, "callback onPrepared()");
                mClientCb.onPrepared(mBinder);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    private void infoOnBufferingUpdate(int percent) {
        if (mClientCb != null) {
            try {
                Log.d(TAG, "callback onBufferingUpdateListener(" + percent + ")");
                mClientCb.onBufferingUpdateListener(mBinder, percent);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    private void infoOnCompletion() {
        if (mClientCb != null) {
            try {
                Log.d(TAG, "callback onCompletion()");
                mClientCb.onCompletion(mBinder);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    private boolean infoOnError(int what, int extra) {
        mState = State.ERROR;
        Log.d(TAG, "Player state changed to: ERROR");
        boolean retval = false;
        if (mClientCb != null) {
            try {
                retval = mClientCb.onError(mBinder, what, extra);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return retval;
    }
    
    private class MyBinder extends IPlayer.Stub {
        @Override
        public void setDataSource(final String path) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.setOnPreparedListener(new OnPreparedListener() {
                    @Override
                    public void onPrepared(VLCPlayer player) {
                        infoOnPrepared();
                    }
                });
                mPlayer.setOnBufferingUpdateListener(new OnBufferingUpdateListener() {
                    @Override
                    public void onBufferingUpdate(VLCPlayer player, int percent) {
                        infoOnBufferingUpdate(percent);
                    }
                });
                mPlayer.setOnCompletionListener(new OnCompletionListener() {
                    @Override
                    public void onCompletion(VLCPlayer player) {
                        infoOnCompletion();
                    }
                });
                mPlayer.setOnErrorListener(new OnErrorListener() {
                    @Override
                    public boolean onError(VLCPlayer player, int what, int extra) {
                        return infoOnError(what, extra);
                    }
                });

                boolean bail = false;
                try {
                    mPlayer.setDataSource(path);
                } catch (IllegalArgumentException e) {
                    e.printStackTrace();
                    bail = true;
                } catch (SecurityException e) {
                    e.printStackTrace();
                    bail = true;
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                    bail = true;
                } catch (IOException e) {
                    e.printStackTrace();
                    bail = true;
                } finally {
                    if (bail) {
                        infoOnError(-1001, 0);
                    } else {
                        mState = State.INITIALIZED;
                        Log.d(TAG, "Player state changed to: INITIALIZED");
                    }
                }
            } else {
                Log.e(TAG, "Player instance is null, call create first");
                infoOnError(-1001, 0);
            }
        }

        @Override
        public void prepare() throws RemoteException {
            if (mPlayer != null) {
                boolean bail = false;
                try {
                    mPlayer.prepare();
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                    bail = true;
                } catch (IOException e) {
                    e.printStackTrace();
                    bail = true;
                } finally {
                    if (bail) {
                        infoOnError(-1001, 0);
                    } else {
                        mState = State.PREPARED;
                        Log.d(TAG, "Player state changed to: PREPARED");
                    }
                }
            } else {
                Log.e(TAG, "Player instance is null, call create first");
                infoOnError(-1001, 0);
            }
        }

        @Override
        public void setDisplay(final Surface surf) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.setDisplay(surf);
                Log.d(TAG, "setDisplay(" + surf + ") done");
            } else {
                Log.e(TAG, "Player instance is null, call create first");
                infoOnError(-1001, 0);
            }
        }

        @Override
        public void start() throws RemoteException {
            if (mPlayer != null) {
                mPlayer.start();
                mState = State.STARTED;
                Log.d(TAG, "Player state changed to: STARTED");
            } else {
                Log.e(TAG, "Player instance is null, call create first");
                infoOnError(-1001, 0);
            }
        }

        @Override
        public void stop() throws RemoteException {
            if (mPlayer != null) {
                mPlayer.stop();

                mState = State.STOPPED;
                Log.d(TAG, "Player state changed to: STOPPED");
            }
        }

        @Override
        public void pause() throws RemoteException {
            if (mPlayer != null) {
                mPlayer.pause();

                mState = State.PAUSED;
                Log.d(TAG, "Player state changed to: PAUSED");
            }
        }

        @Override
        public void release() throws RemoteException {
            if (mPlayer != null) {
                mPlayer.relase();
                mClientCb = null;
                mPlayer = null;

                mState = State.UNKNOWN;
                Log.d(TAG, "Player state changed to: UNKNOWN");
            }
        }

        @Override
        public int getVideoWidth() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getVideoWidth();
            }
            return -1;
        }

        @Override
        public int getVideoHeight() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getVideoHeight();
            }
            return -1;
        }

        @Override
        public boolean isPlaying() throws RemoteException {
            boolean retval = false;
            if (mPlayer != null) {
                retval = mPlayer.isPlaying();
                if (mState == State.STARTED) {
                    Log.w(TAG, "Player state is STARTED, regard it as playing");
                    retval = true;
                }
            }
            return retval;
        }

        @Override
        public boolean isSeekable() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.isSeekable();
            }
            return false;
        }

        @Override
        public int getCurrentPosition() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getCurrentPosition();
            }
            return -1;
        }

        @Override
        public int getDuration() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getDuration();
            }
            return -1;
        }

        @Override
        public long getTime() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getTime();
            }
            return -1;
        }

        @Override
        public void setTime(final long time) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.setTime(time);
            }
        }

        @Override
        public int getVolume() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getVolume();
            }
            return -1;
        }

        @Override
        public int setVolume(final int volume) throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.setVolume(volume);
            }
            return -1;
        }

        @Override
        public void setPosition(final float pos) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.setPosition(pos);
            }
        }

        @Override
        public int getAudioTracksCount() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getAudioTracksCount();
            }
            return -1;
        }

        @Override
        public int getAudioTrack() throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.getAudioTrack();
            }
            return -1;
        }

        @Override
        public boolean setAudioTrack(final int index) throws RemoteException {
            if (mPlayer != null) {
                return mPlayer.setAudioTrack(index);
            }
            return false;
        }

        @Override
        public void seekTo(final int delta) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.seekTo(delta);
            }
        }

        @Override
        public void selectTrack(final int index) throws RemoteException {
            if (mPlayer != null) {
                mPlayer.selectTrack(index);
            }
        }

        @Override
        public void registerClientCallback(final IClientCallback cb) throws RemoteException {
            mClientCb = cb;
        }

        @Override
        public void create() throws RemoteException {
            if (mPlayer != null) {
                Log.e(TAG, "Previous player instance hasn't been released");
                infoOnError(-1001, 0);
                return;
            }
            
            mPlayer = new VLCPlayer(PlayerService.this);
            mState = State.IDLE;
            Log.d(TAG, "Player state changed to: IDLE");
        }
    }
}
