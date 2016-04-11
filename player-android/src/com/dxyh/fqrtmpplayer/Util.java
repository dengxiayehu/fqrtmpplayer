package com.dxyh.fqrtmpplayer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.widget.EditText;

public class Util {
    public static final String TAG = "FQRtmpPlayer/Util";

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
        edit.setText("rtmp://192.168.200.146/live/va");
        
        AlertDialog.Builder builder =
                new AlertDialog.Builder(activity);
        builder.setTitle(title)
               .setIcon(android.R.drawable.ic_dialog_info)
               .setView(edit)
               .setCancelable(false)
               .setPositiveButton(R.string.dialog_positive_btn_label,new Dialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
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
}