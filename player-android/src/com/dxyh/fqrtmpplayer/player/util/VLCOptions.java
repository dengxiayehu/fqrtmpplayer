package com.dxyh.fqrtmpplayer.player.util;

import java.util.ArrayList;

import org.videolan.libvlc.Media;
import org.videolan.libvlc.util.AndroidUtil;
import org.videolan.libvlc.util.HWDecoderUtil;
import org.videolan.libvlc.util.VLCUtil;

import com.dxyh.fqrtmpplayer.player.media.MediaWrapper;

import android.util.Log;

public class VLCOptions {
    private static final String TAG = "VLCOptions";

    public static final int AOUT_AUDIOTRACK = 0;
    public static final int AOUT_OPENSLES = 1;

    public static final int HW_ACCELERATION_AUTOMATIC = -1;
    public static final int HW_ACCELERATION_DISABLED = 0;
    public static final int HW_ACCELERATION_DECODING = 1;
    public static final int HW_ACCELERATION_FULL = 2;

    public static ArrayList<String> getLibOptions() {
        ArrayList<String> options = new ArrayList<String>(50);

        final boolean timeStreching = false;
        final String subtitlesEncoding = "";
        final boolean frameSkip = false;
        String chroma = "RV32";
        if (chroma != null)
            chroma = chroma.equals("YV12") && !AndroidUtil.isGingerbreadOrLater() ? "" : chroma;
        final boolean verboseMode = true;

        int deblocking = -1;
        try {
            deblocking = getDeblocking(-1);
        } catch (NumberFormatException ignored) {
        }

        /* CPU intensive plugin, setting for slow devices */
        options.add(timeStreching ? "--audio-time-stretch" : "--no-audio-time-stretch");
        options.add("--avcodec-skiploopfilter");
        options.add("" + deblocking);
        options.add("--avcodec-skip-frame");
        options.add(frameSkip ? "2" : "0");
        options.add("--avcodec-skip-idct");
        options.add(frameSkip ? "2" : "0");
        options.add("--subsdec-encoding");
        options.add(subtitlesEncoding);
        options.add("--stats");
        options.add("--androidwindow-chroma");
        options.add(chroma != null ? chroma : "RV32");
        options.add("--audio-resampler");
        options.add(getResampler());

        options.add(verboseMode ? "-vvv" : "-vv");
        return options;
    }

    public static String getAout() {
        int aout = AOUT_AUDIOTRACK;
        final HWDecoderUtil.AudioOutput hwaout = HWDecoderUtil.getAudioOutputFromDevice();
        if (hwaout == HWDecoderUtil.AudioOutput.AUDIOTRACK || hwaout == HWDecoderUtil.AudioOutput.OPENSLES)
            aout = hwaout == HWDecoderUtil.AudioOutput.OPENSLES ? AOUT_OPENSLES : AOUT_AUDIOTRACK;

        return aout == AOUT_OPENSLES ? "opensles_android" : "android_audiotrack";
    }

    private static int getDeblocking(int deblocking) {
        int ret = deblocking;
        if (deblocking < 0) {
            /**
             * Set some reasonable sDeblocking defaults:
             *
             * Skip all (4) for armv6 and MIPS by default Skip non-ref (1) for
             * all armv7 more than 1.2 Ghz and more than 2 cores Skip non-key
             * (3) for all devices that don't meet anything above
             */
            VLCUtil.MachineSpecs m = VLCUtil.getMachineSpecs();
            if (m == null)
                return ret;
            if ((m.hasArmV6 && !(m.hasArmV7)) || m.hasMips)
                ret = 4;
            else if (m.frequency >= 1200 && m.processors > 2)
                ret = 1;
            else if (m.bogoMIPS >= 1200 && m.processors > 2) {
                ret = 1;
                Log.d(TAG, "Used bogoMIPS due to lack of frequency info");
            } else
                ret = 3;
        } else if (deblocking > 4) { // sanity check
            ret = 3;
        }
        return ret;
    }

    private static String getResampler() {
        final VLCUtil.MachineSpecs m = VLCUtil.getMachineSpecs();
        return m.processors > 2 ? "soxr" : "ugly";
    }

    public static void setMediaOptions(Media media, int flags) {
        boolean noHardwareAcceleration = (flags & MediaWrapper.MEDIA_NO_HWACCEL) != 0;
        boolean noVideo = (flags & MediaWrapper.MEDIA_VIDEO) == 0;
        final boolean paused = (flags & MediaWrapper.MEDIA_PAUSED) != 0;
        int hardwareAcceleration = HW_ACCELERATION_DISABLED;

        if (!noHardwareAcceleration)
            hardwareAcceleration = HW_ACCELERATION_FULL;
        if (hardwareAcceleration == HW_ACCELERATION_DISABLED)
            media.setHWDecoderEnabled(false, false);
        else if (hardwareAcceleration == HW_ACCELERATION_FULL || hardwareAcceleration == HW_ACCELERATION_DECODING) {
            media.setHWDecoderEnabled(true, true);
            if (hardwareAcceleration == HW_ACCELERATION_DECODING) {
                media.addOption(":no-mediacodec-dr");
                media.addOption(":no-omxil-dr");
            }
        } /* else automatic: use default options */

        if (noVideo)
            media.addOption(":no-video");
        if (paused)
            media.addOption(":start-paused");
    }
}
