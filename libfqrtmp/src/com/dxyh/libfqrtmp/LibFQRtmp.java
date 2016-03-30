package com.dxyh.libfqrtmp;

import android.util.Log;

public class LibFQRtmp {
    private static final String TAG = LibFQRtmp.class.getSimpleName();
    
    static {
        try {
            System.loadLibrary("rtmp");
            System.loadLibrary("fqrtmpjni");
        } catch (UnsatisfiedLinkError ule) {
            Log.e(TAG, "Can't load libraries: " + ule);
            System.exit(1);
        } catch (SecurityException se) {
            Log.e(TAG, "Encountered as security issue when loading libraries: " + se);
            System.exit(1);
        }
    }
}
