package com.dxyh.fqrtmpplayer.util;

import android.os.Environment;

public class AndroidDevices {
    public final static String TAG = "AndroidDevices";
    public final static String EXTERNAL_PUBLIC_DIRECTORY = Environment.getExternalStorageDirectory().getPath();
    
    public static boolean hasExternalStorage() {
        return Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
    }
}