package dev.headless.sequence;

public class Protocol {
    public static final int MAGIC = 0x41444253; // "ADBS"
    
    public static final byte TYPE_VIDEO = 0x01;
    public static final byte TYPE_META  = 0x02;
    
    public static final byte EVENT_TYPE_KEY = 1;
    public static final byte EVENT_TYPE_TOUCH_DOWN = 2;
    public static final byte EVENT_TYPE_TOUCH_UP = 3;
    public static final byte EVENT_TYPE_TOUCH_MOVE = 4;
}
