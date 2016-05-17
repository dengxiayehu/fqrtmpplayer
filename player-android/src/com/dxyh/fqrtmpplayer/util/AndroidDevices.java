package com.dxyh.fqrtmpplayer.util;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.StringTokenizer;

import android.os.Environment;

public class AndroidDevices {
    public final static String TAG = "AndroidDevices";
    public final static String EXTERNAL_PUBLIC_DIRECTORY = Environment.getExternalStorageDirectory().getPath();
    
    public static boolean hasExternalStorage() {
        return Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
    }
    
    public static ArrayList<String> getStorageDirectories() {
        BufferedReader bufReader = null;
        ArrayList<String> list = new ArrayList<String>();
        list.add(EXTERNAL_PUBLIC_DIRECTORY);

        List<String> typeWL = Arrays.asList("vfat", "exfat", "sdcardfs", "fuse", "ntfs", "fat32", "ext3", "ext4",
                "esdfs");
        List<String> typeBL = Arrays.asList("tmpfs");
        String[] mountWL = { "/mnt", "/Removable", "/storage" };
        String[] mountBL = { "/mnt/secure", "/mnt/shell", "/mnt/asec", "/mnt/obb", "/mnt/media_rw/extSdCard",
                "/mnt/media_rw/sdcard", "/storage/emulated" };
        String[] deviceWL = { "/dev/block/vold", "/dev/fuse", "/mnt/media_rw" };

        try {
            bufReader = new BufferedReader(new FileReader("/proc/mounts"));
            String line;
            while ((line = bufReader.readLine()) != null) {

                StringTokenizer tokens = new StringTokenizer(line, " ");
                String device = tokens.nextToken();
                String mountpoint = tokens.nextToken();
                String type = tokens.nextToken();

                // skip if already in list or if type/mountpoint is blacklisted
                if (list.contains(mountpoint) || typeBL.contains(type) || Strings.startsWith(mountBL, mountpoint))
                    continue;

                // check that device is in whitelist, and either type or
                // mountpoint is in a whitelist
                if (Strings.startsWith(deviceWL, device)
                        && (typeWL.contains(type) || Strings.startsWith(mountWL, mountpoint))) {
                    int position = Strings.containsName(list, FileUtils.getFileNameFromPath(mountpoint));
                    if (position > -1)
                        list.remove(position);
                    list.add(mountpoint);
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            Util.close(bufReader);
        }
        return list;
    }
}