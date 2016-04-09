package com.dxyh.libfqrtmp;

public class Event {
    public final int type;
    public final long arg1;
    public final String arg2;
    
    protected Event(int type, long arg1, String arg2) {
        this.type = type;
        this.arg1 = arg1;
        this.arg2 = arg2;
    }
    
    public interface Listener {
        void onEvent(Event event);
    }
}