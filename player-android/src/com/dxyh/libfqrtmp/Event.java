package com.dxyh.libfqrtmp;

public class Event {
    public static final int OPENING = 0;
    public static final int CONNECTED = 1;
    public static final int ENCOUNTERED_ERROR = 2;
    
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