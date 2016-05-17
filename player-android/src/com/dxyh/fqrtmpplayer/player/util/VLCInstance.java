package com.dxyh.fqrtmpplayer.player.util;

import org.videolan.libvlc.LibVLC;
import org.videolan.libvlc.util.VLCUtil;

import android.content.Context;
import android.util.Log;;

public class VLCInstance {
    public final static String TAG = "VLCInstance";

    private static LibVLC sLibVLC = null;

    /** A set of utility functions for the VLC application */
    public synchronized static LibVLC get(final Context context) throws IllegalStateException {
        if (sLibVLC == null) {
            if (!VLCUtil.hasCompatibleCPU(context)) {
                Log.e(TAG, VLCUtil.getErrorMsg());
                throw new IllegalStateException("LibVLC initialisation failed: " + VLCUtil.getErrorMsg());
            }

            sLibVLC = new LibVLC(VLCOptions.getLibOptions());
            LibVLC.setOnNativeCrashListener(new LibVLC.OnNativeCrashListener() {
                @Override
                public void onNativeCrash() {
                    Log.e(TAG, "FATAL: LHPlayer crashed");
                }
            });
        }
        return sLibVLC;
    }

    public static synchronized void restart() throws IllegalStateException {
        if (sLibVLC != null) {
            sLibVLC.release();
            sLibVLC = new LibVLC(VLCOptions.getLibOptions());
        }
    }
    
    public static synchronized boolean testCompatibleCPU(Context context) {
        if (sLibVLC == null && !VLCUtil.hasCompatibleCPU(context))
            return false;
        else
            return true;
    }
}
