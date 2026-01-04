package dev.headless.sequence;

import android.os.SystemClock;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.InputDevice;
import java.lang.reflect.Method;

public class DeviceControl {
    private Object inputManager;
    private Method injectMethod;
    private long lastMouseDownTime = 0;

    public DeviceControl() {
        try {
            Method getServiceMethod = Class.forName("android.os.ServiceManager")
                    .getDeclaredMethod("getService", String.class);
            Object imBinder = getServiceMethod.invoke(null, "input");
            Method asInterfaceMethod = Class.forName("android.hardware.input.IInputManager$Stub")
                    .getDeclaredMethod("asInterface", android.os.IBinder.class);
            inputManager = asInterfaceMethod.invoke(null, imBinder);
            injectMethod = inputManager.getClass().getMethod("injectInputEvent", InputEvent.class, int.class);
        } catch (Exception e) {
            System.err.println("Failed to initialize DeviceControl: " + e.getMessage());}}

    public void injectEvent(byte type, int x, int y, int data) {
        try {
            InputEvent event = null;
            long now = SystemClock.uptimeMillis();
            switch (type) {
                case Protocol.EVENT_TYPE_TOUCH_DOWN:
                    lastMouseDownTime = now;
                    event = createMotionEvent(MotionEvent.ACTION_DOWN, now, x, y);
                    break;
                case Protocol.EVENT_TYPE_TOUCH_UP:
                    event = createMotionEvent(MotionEvent.ACTION_UP, lastMouseDownTime, x, y);
                    break;
                case Protocol.EVENT_TYPE_TOUCH_MOVE:
                    event = createMotionEvent(MotionEvent.ACTION_MOVE, lastMouseDownTime, x, y);
                    break;
                case Protocol.EVENT_TYPE_KEY:
                    injectKeyEvent(KeyEvent.ACTION_DOWN, data);
                    injectKeyEvent(KeyEvent.ACTION_UP, data);
                    return;}
            if (event != null) {
                injectMethod.invoke(inputManager, event, 0);}
        } catch (Exception e) {
            e.printStackTrace();}}

    private MotionEvent createMotionEvent(int action, long downTime, int x, int y) {
        MotionEvent.PointerProperties props = new MotionEvent.PointerProperties();
        props.id = 0;
        props.toolType = MotionEvent.TOOL_TYPE_FINGER;
        MotionEvent.PointerCoords coords = new MotionEvent.PointerCoords();
        coords.x = x;
        coords.y = y;
        coords.pressure = 1.0f;
        coords.size = 1.0f;
        return MotionEvent.obtain(
            downTime, SystemClock.uptimeMillis(), action, 1,
            new MotionEvent.PointerProperties[]{props},
            new MotionEvent.PointerCoords[]{coords},
            0, 0, 1.0f, 1.0f, 0, 0, 
            InputDevice.SOURCE_TOUCHSCREEN, 0);}
    
    private void injectKeyEvent(int action, int keyCode) throws Exception {
        long now = SystemClock.uptimeMillis();
        KeyEvent event = new KeyEvent(now, now, action, keyCode, 0);
        injectMethod.invoke(inputManager, event, 0);}}
