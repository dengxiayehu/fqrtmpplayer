package com.dxyh.fqrtmpplayer.aidl;

import com.dxyh.fqrtmpplayer.aidl.IPlayer;

interface IClientCallback {
    void onPrepared(IPlayer player);
    void onCompletion(IPlayer player);
    void onBufferingUpdateListener(IPlayer player, int percent);
    boolean onError(IPlayer player, int what, int extra);
}
