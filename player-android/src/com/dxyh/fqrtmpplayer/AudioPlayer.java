package com.dxyh.fqrtmpplayer;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.dxyh.fqrtmpplayer.util.AndroidDevices;
import com.dxyh.fqrtmpplayer.util.Util;
import com.dxyh.libfqrtmp.LibFQRtmp.AudioConfig;

import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;

@SuppressWarnings("deprecation")
public class AudioPlayer {
    private static final String TAG = "AudioPlayer";
    
    private static final int INIT = 0;
    private static final int CLOSE = 1;
    private static final int PLAY = 2;

    private AudioTrack mAudioTrack;
    
    private HandlerThread mHandlerThread = null;
    private Handler mHandler = null;
    
    private String mWavePath = AndroidDevices.EXTERNAL_PUBLIC_DIRECTORY + "/fqrtmp.wav";
    private RandomAccessFile mWaveFile;
    private int mPayloadSize = 0;
    
    public void prepare(AudioConfig audioConfig) {
        mHandlerThread = new HandlerThread(TAG);
        mHandlerThread.start();
        
        mHandler = new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                case INIT: {
                    AudioConfig audioConfig = (AudioConfig) msg.obj;
                    
                    if (initWave(audioConfig) < 0) {
                        Log.e(TAG, "Init wave failed");
                        break;
                    }
                    
                    if (initAudioTrack(audioConfig) < 0) {
                        Log.e(TAG, "Init audio-track failed");
                        break;
                    }
                } break;
                
                case CLOSE: {
                    mAudioTrack.stop();
                    mAudioTrack.release();
                    mAudioTrack = null;
                    
                    mHandlerThread.quit();
                    mHandlerThread = null;
                    
                    if (mWaveFile != null) {
                        try {
                            mWaveFile.seek(4);
                            mWaveFile.writeInt(Integer.reverseBytes(36 + mPayloadSize));
                            mWaveFile.seek(40);
                            mWaveFile.writeInt(Integer.reverseBytes(mPayloadSize));
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        Util.close(mWaveFile);
                        mWaveFile = null;
                    }
                } break;
                
                case PLAY: {
                    byte[] buffer = (byte []) msg.obj;
                    mAudioTrack.write(buffer, 0, buffer.length);
                    if (mWaveFile != null) {
                        try {
                            mWaveFile.write(buffer);
                            mPayloadSize += buffer.length;
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    buffer = null;
                } break;
                
                default:
                    super.handleMessage(msg);
                }
            }
        };
        
        mHandler.sendMessage(mHandler.obtainMessage(INIT, audioConfig));
    }
    
    private int initWave(AudioConfig audioConfig) {
        if (mWavePath != null && !TextUtils.isEmpty(mWavePath)) {
            try {
                mWaveFile = new RandomAccessFile(mWavePath, "rw");
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                return -1;
            }
        
            try {
                mWaveFile.setLength(0);
                mWaveFile.writeBytes("RIFF");
                mWaveFile.writeInt(0);
                mWaveFile.writeBytes("WAVE");
                mWaveFile.writeBytes("fmt ");
                mWaveFile.writeInt(Integer.reverseBytes(16));
                mWaveFile.writeShort(Short.reverseBytes((short) 1));
                mWaveFile.writeShort(Short.reverseBytes((short) audioConfig.getAudioObjectType()));
                mWaveFile.writeInt(Integer.reverseBytes(audioConfig.getSamplerate()));
                mWaveFile.writeInt(Integer.reverseBytes(audioConfig.getSamplerate() * audioConfig.getChannelCount() * audioConfig.getBitsPerSample() / 8));
                mWaveFile.writeShort(Short.reverseBytes((short) (audioConfig.getChannelCount() * audioConfig.getBitsPerSample() / 8)));
                mWaveFile.writeShort(Short.reverseBytes((short) audioConfig.getBitsPerSample()));
                mWaveFile.writeBytes("data");
                mWaveFile.writeInt(0);
            } catch (IOException e) {
                e.printStackTrace();
                Util.close(mWaveFile);
                mWaveFile = null;
                return -1;
            }
        }
        
        return 0;
    }
    
    private int initAudioTrack(AudioConfig audioConfig) {
        int minBufferSize = AudioTrack.getMinBufferSize(audioConfig.getSamplerate(),
                audioConfig.getChannel(), audioConfig.getEncoding());
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, audioConfig.getSamplerate(),
                audioConfig.getChannel(), audioConfig.getEncoding(),
                minBufferSize, AudioTrack.MODE_STREAM);
        mAudioTrack.setStereoVolume(audioConfig.getVolume(), audioConfig.getVolume());
        mAudioTrack.play();
        return 0;
    }
    
    public boolean isPlaying() {
        return mAudioTrack != null &&
               mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING;
    }
    
    public void play(byte[] buffer, int length) {
        byte[] copy = new byte[length];
        System.arraycopy(buffer, 0, copy, 0, length);
        mHandler.sendMessage(mHandler.obtainMessage(PLAY, copy));
    }
    
    public void close() {
        mHandler.sendEmptyMessage(CLOSE);
    }
}