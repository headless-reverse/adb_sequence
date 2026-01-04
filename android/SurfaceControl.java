package dev.headless.sequence;

import android.graphics.Rect;
import android.os.Build;
import android.os.IBinder;
import android.view.Surface;
import java.lang.reflect.Method;

public final class SurfaceControl {
    private static final Class<?> CLASS;
    static {
        try {
            CLASS = Class.forName("android.view.SurfaceControl");
        } catch (ClassNotFoundException e) {
            throw new AssertionError(e);}}

    public static void openTransaction() {
        try {
            CLASS.getMethod("openTransaction").invoke(null);
        } catch (Exception e) { throw new AssertionError(e); }}

    public static void closeTransaction() {
        try {
            CLASS.getMethod("closeTransaction").invoke(null);
        } catch (Exception e) { throw new AssertionError(e); }}

    public static void setDisplayProjection(IBinder displayToken, int orientation, Rect layerStackRect, Rect displayRect) {
        try {
            CLASS.getMethod("setDisplayProjection", IBinder.class, int.class, Rect.class, Rect.class)
                    .invoke(null, displayToken, orientation, layerStackRect, displayRect);
        } catch (Exception e) { throw new AssertionError(e); }}

    public static void setDisplayLayerStack(IBinder displayToken, int layerStack) {
        try {
            CLASS.getMethod("setDisplayLayerStack", IBinder.class, int.class).invoke(null, displayToken, layerStack);
        } catch (Exception e) { throw new AssertionError(e); }}

    public static void setDisplaySurface(IBinder displayToken, Surface surface) {
        try {
            CLASS.getMethod("setDisplaySurface", IBinder.class, Surface.class).invoke(null, displayToken, surface);
        } catch (Exception e) { throw new AssertionError(e); }}

    public static IBinder createDisplay(String name, boolean secure) throws Exception {
        return (IBinder) CLASS.getMethod("createDisplay", String.class, boolean.class).invoke(null, name, secure);}

    public static IBinder getBuiltInDisplay() {
        try {
            if (Build.VERSION.SDK_INT < 29) {
                Method method = CLASS.getMethod("getBuiltInDisplay", int.class);
                return (IBinder) method.invoke(null, 0);
            } else {
                try {
                    Method method = CLASS.getMethod("getInternalDisplayToken");
                    return (IBinder) method.invoke(null);
                } catch (NoSuchMethodException e) {
                    long[] ids = (long[]) CLASS.getMethod("getPhysicalDisplayIds").invoke(null);
                    if (ids != null && ids.length > 0) {
                        return (IBinder) CLASS.getMethod("getPhysicalDisplayToken", long.class).invoke(null, ids[0]);}
                    throw e;}}
        } catch (Exception e) {
            e.printStackTrace();
            return null;}}}
