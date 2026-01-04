package dev.headless.sequence;

import android.hardware.input.InputManager;
import android.view.InputEvent;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.os.SystemClock;
import java.lang.reflect.Method;

public class InputHandler {
    private Method injectInputEventMethod;
    private InputManager inputManager;

    public InputHandler() {
        try {
            Method getInstanceMethod = InputManager.class.getDeclaredMethod("getInstance");
            inputManager = (InputManager) getInstanceMethod.invoke(null);
            injectInputEventMethod = InputManager.class.getMethod("injectInputEvent", InputEvent.class, int.class);
            blockHardwareForReal();
        } catch (Exception e) {
            e.printStackTrace();}}

    private void blockHardwareForReal() {
        try {
            Runtime.getRuntime().exec("settings put global policy_control immersive.navigation=*");
            Runtime.getRuntime().exec("settings put global power_button_very_long_press 0");
            Runtime.getRuntime().exec("settings put global power_button_long_press 0");
            Runtime.getRuntime().exec("settings put global key_chord_power_volume_up 0");
            Runtime.getRuntime().exec("wm lock-task-mode pull-up dev.headless.sequence");
        } catch (Exception e) {}
    }

    public void injectTap(int x, int y) {
        long now = SystemClock.uptimeMillis();
        injectMotionEvent(InputDevice.SOURCE_TOUCHSCREEN, MotionEvent.ACTION_DOWN, now, now, x, y, 1.0f);
        injectMotionEvent(InputDevice.SOURCE_TOUCHSCREEN, MotionEvent.ACTION_UP, now, now + 50, x, y, 0.0f);}

    private void injectMotionEvent(int source, int action, long downTime, long eventTime, float x, float y, float pressure) {
        try {
            MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, x, y, pressure, 1.0f, 0, 1.0f, 1.0f, 0, 0);
            event.setSource(source);
            injectInputEventMethod.invoke(inputManager, event, 2);
        } catch (Exception e) {
            e.printStackTrace();}}}
