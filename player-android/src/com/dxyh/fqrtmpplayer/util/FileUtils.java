package com.dxyh.fqrtmpplayer.util;

import java.io.File;

import android.annotation.TargetApi;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;

public class FileUtils {

    public interface Callback {
        void onResult(boolean success);
    }

    public static String getFileNameFromPath(String path) {
        if (path == null)
            return "";
        int index = path.lastIndexOf('/');
        if (index > -1)
            return path.substring(index + 1);
        else
            return path;
    }

    public static String getParent(String path) {
        if (TextUtils.equals("/", path))
            return path;
        String parentPath = path;
        if (parentPath.endsWith("/"))
            parentPath = parentPath.substring(0, parentPath.length() - 1);
        int index = parentPath.lastIndexOf('/');
        if (index > 0) {
            parentPath = parentPath.substring(0, index);
        } else if (index == 0)
            parentPath = "/";
        return parentPath;
    }

    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static boolean deleteFile(String path) {
        boolean deleted = false;
        path = Uri.decode(Strings.removeFileProtocole(path));
        File file = new File(path);
        if (file.exists())
            deleted |= file.delete();
        return deleted;
    }

    public static void recursiveDelete(String path, Callback callback) {
        recursiveDelete(new File(path), callback);
    }

    public static void recursiveDelete(String path) {
        recursiveDelete(path, null);
    }

    private static void recursiveDelete(final File fileOrDirectory, final Callback callback) {
        if (!fileOrDirectory.exists() || !fileOrDirectory.canWrite())
            return;
        boolean success = true;
        if (fileOrDirectory.isDirectory()) {
            for (File child : fileOrDirectory.listFiles())
                recursiveDelete(child, null);
            success = fileOrDirectory.delete();
        } else {
            success = deleteFile(fileOrDirectory.getPath());
        }
        if (callback != null)
            callback.onResult(success);
    }

    public static boolean canWrite(String path) {
        if (path == null)
            return false;
        if (path.startsWith("file://"))
            path = path.substring(7);
        if (!path.startsWith("/"))
            return false;
        if (path.startsWith(AndroidDevices.EXTERNAL_PUBLIC_DIRECTORY))
            return true;
        File file = new File(path);
        return (file.exists() && file.canWrite());
    }
}
