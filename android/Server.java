package dev.headless.sequence;

import android.os.IBinder;
import android.view.Surface;
import android.graphics.Rect;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.lang.reflect.Method;

public class Server {

    public static void main(String[] args) {
        try {
            int port = 7373;
            if (args.length > 0) {
                port = Integer.parseInt(args[0]);}
            new Server().start(port);
        } catch (Exception e) {
            System.err.println("Fatal error: " + e.toString());
            e.printStackTrace();
            System.exit(1);}}

    public void start(int port) throws Exception {
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("ADB_SEQUENCE_SERVER: Listening on port " + port);
            while (true) {
                Socket client = serverSocket.accept();
                // Każde połączenie dostaje osobny wątek
                new Thread(() -> handleClient(client)).start();}}}

    private void handleClient(Socket client) {
        System.out.println("Client Connected: " + client.getRemoteSocketAddress());
        try {
            client.setTcpNoDelay(true);
            DataInputStream in = new DataInputStream(client.getInputStream());
            DataOutputStream out = new DataOutputStream(client.getOutputStream());
            int targetWidth = 720;
            int targetHeight = 1280;
            int bitrate = 4000000;
            try {
                client.setSoTimeout(2000); 
                targetWidth = in.readInt();
                targetHeight = in.readInt();
                bitrate = in.readInt();
                if (targetWidth <= 0) targetWidth = 720;
                if (targetHeight <= 0) targetHeight = 1280;
                System.out.println(String.format("CONFIG: %dx%d @ %d bps", targetWidth, targetHeight, bitrate));
            } catch (IOException e) {
                System.out.println("Handshake failed or timed out. Using default 720x1280.");
            }
            client.setSoTimeout(0);
            ScreenEncoder encoder = new ScreenEncoder(targetWidth, targetHeight, bitrate);
            encoder.stream(out);
        } catch (Exception e) {
            System.err.println("SERVER ERROR: " + e.toString());
            e.printStackTrace();
        } finally {
            try {
                client.close();
                System.out.println("Client Connection Closed.");
            } catch (IOException e) {
                // ignore
            }
        }
    }

    public static void createDisplayMirror(Surface surface, int w, int h) throws Exception {
        IBinder display = getBuiltInDisplay();
        if (display == null) throw new RuntimeException("No display token!");
        Method createDisplayMethod = Class.forName("android.view.SurfaceControl")
                .getMethod("createDisplay", String.class, boolean.class);
        IBinder virtualDisplay = (IBinder) createDisplayMethod.invoke(null, "adb_sequence", false);
        openTransaction();
        try {
            setDisplaySurface(virtualDisplay, surface);
            Rect rect = new Rect(0, 0, w, h);
            setDisplayProjection(virtualDisplay, 0, rect, rect);
            setDisplayLayerStack(virtualDisplay, 0);
        } finally {
            closeTransaction();}}

    private static IBinder getBuiltInDisplay() throws Exception {
        try {
            return (IBinder) Class.forName("android.view.SurfaceControl")
                    .getMethod("getInternalDisplayToken").invoke(null);
        } catch (Exception e) {
            return (IBinder) Class.forName("android.view.SurfaceControl")
                    .getMethod("getBuiltInDisplay", int.class).invoke(null, 0);}}

    private static void openTransaction() throws Exception {
        Class.forName("android.view.SurfaceControl").getMethod("openTransaction").invoke(null);}

    private static void closeTransaction() throws Exception {
        Class.forName("android.view.SurfaceControl").getMethod("closeTransaction").invoke(null);}

    private static void setDisplaySurface(IBinder display, Surface surface) throws Exception {
        Class.forName("android.view.SurfaceControl")
                .getMethod("setDisplaySurface", IBinder.class, Surface.class)
                .invoke(null, display, surface);}

    private static void setDisplayProjection(IBinder display, int orientation, Rect layerStack, Rect displayRect) throws Exception {
        Class.forName("android.view.SurfaceControl")
                .getMethod("setDisplayProjection", IBinder.class, int.class, Rect.class, Rect.class)
                .invoke(null, display, orientation, layerStack, displayRect);}

    private static void setDisplayLayerStack(IBinder display, int layerStack) throws Exception {
        Class.forName("android.view.SurfaceControl")
                .getMethod("setDisplayLayerStack", IBinder.class, int.class)
                .invoke(null, display, layerStack);}}
