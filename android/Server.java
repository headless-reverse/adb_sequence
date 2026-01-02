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
                port = Integer.parseInt(args[0]);
            }
            new Server().start(port);
        } catch (Exception e) {
            System.err.println("Fatal error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    public void start(int port) throws Exception {
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("Server started on port " + port + ". Waiting for connection...");
            
            while (true) {
                try (Socket client = serverSocket.accept()) {
                    client.setTcpNoDelay(true);
                    System.out.println("Qt Client connected: " + client.getRemoteSocketAddress());
                    DataOutputStream out = new DataOutputStream(client.getOutputStream());
                    int width = 720;
                    int height = 1280;
                    int bitrate = 4000000;
                    ScreenEncoder encoder = new ScreenEncoder(width, height, bitrate);
                    encoder.stream(out); 
                } catch (Exception e) {
                    System.err.println("Connection error: " + e.getMessage());
                }
                System.out.println("Ready for next connection...");
            }
        }
    }

    public static void createDisplayMirror(Surface surface, int w, int h) throws Exception {
    IBinder display = SurfaceControl.getBuiltInDisplay();

    if (display == null) {
        throw new RuntimeException("Could not get display token. Are you running as shell/root?");
    }

    IBinder virtualDisplay = SurfaceControl.createDisplay("adb_sequence", false);

    SurfaceControl.openTransaction();
    try {
        SurfaceControl.setDisplaySurface(virtualDisplay, surface);
        Rect rect = new Rect(0, 0, w, h);
        SurfaceControl.setDisplayProjection(virtualDisplay, 0, rect, rect);
        SurfaceControl.setDisplayLayerStack(virtualDisplay, 0);
    } finally {
        SurfaceControl.closeTransaction();
    }
    System.out.println("adb_sequence - Virtual Display Mirror Activated!");
}}
