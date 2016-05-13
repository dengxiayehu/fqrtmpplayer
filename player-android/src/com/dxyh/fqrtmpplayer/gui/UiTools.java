package com.dxyh.fqrtmpplayer.gui;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.provider.Settings;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.Toast;

import com.dxyh.fqrtmpplayer.MyApplication;
import com.dxyh.fqrtmpplayer.R;

public class UiTools {
    @SuppressWarnings("unused")
    private static final String TAG = "UiTools";
    
    public interface OnInputDialogClickListener {
        public void onClick(DialogInterface dialog, final String input);
    }
    public static void inputDialog(Activity activity, final String title,
            final OnInputDialogClickListener positive_listener,
            final OnInputDialogClickListener negative_listener) {
        final EditText edit = new EditText(MyApplication.getAppContext());
        edit.setFocusable(true);
        edit.setHint(R.string.edit_hint);
        edit.setSingleLine(true);
        edit.setText("rtmp://192.168.6.127:1935/live/va");
        edit.setSelection(edit.getText().toString().length());
        
        AlertDialog.Builder builder =
                new AlertDialog.Builder(activity);
        builder.setTitle(title)
               .setIcon(android.R.drawable.ic_dialog_info)
               .setView(edit)
               .setCancelable(false)
               .setPositiveButton(R.string.dialog_positive_btn_label,new Dialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (positive_listener != null)
                            positive_listener.onClick(dialog, edit.getText().toString());
                    }})
               .setNegativeButton(R.string.dialog_negative_btn_label, new Dialog.OnClickListener() {
                   @Override
                   public void onClick(DialogInterface dialog, int which) {
                       if (negative_listener != null)
                           negative_listener.onClick(dialog, edit.getText().toString());
                   }});
        builder.show();
    }
    
    public static final int SHORT_TOAST = Toast.LENGTH_SHORT;
    public static final int LONG_TOAST = Toast.LENGTH_LONG;
    
    public static void toast(final Activity activity, final String text, final int duration) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(activity, text, duration).show();
            }
        });
    }
    
    public static void adjust_brightness(final Activity activity, final float brightness) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Window win = activity.getWindow();
                
                int mode = Settings.System.getInt(
                        activity.getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS_MODE,
                        Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
                if (mode == Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC) {
                    WindowManager.LayoutParams winParams = win.getAttributes();
                    winParams.screenBrightness = brightness;
                    win.setAttributes(winParams);
                }
            }
        });
    }
}
