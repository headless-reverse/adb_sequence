package dev.headless.sequence;

import android.os.IBinder;
import android.view.Surface;
import android.graphics.Rect;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    public static void main(String[] args) {
        try {
            new Server().start(7373);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    public void start(int port) throws Exception {
        ServerSocket serverSocket = new ServerSocket(port);
        System.out.println("Server started. Waiting for Qt...");
        
        Socket client = serverSocket.accept();
        DataOutputStream out = new DataOutputStream(client.getOutputStream());

        ScreenEncoder encoder = new ScreenEncoder(720, 1280, 4000000);
        encoder.stream(null, out); 
    }

    public static void createDisplayMirror(Surface surface, int w, int h) throws Exception {
        IBinder display = SurfaceControl.getBuiltInDisplay();
        if (display == null) {
            throw new RuntimeException("Could not get display token");
        }

        IBinder virtualDisplay = SurfaceControl.createDisplay("adb_sequence", false);

        SurfaceControl.openTransaction();
        try {
            SurfaceControl.setDisplaySurface(virtualDisplay, surface);
            SurfaceControl.setDisplayProjection(virtualDisplay, 0, new Rect(0, 0, w, h), new Rect(0, 0, w, h));
            SurfaceControl.setDisplayLayerStack(virtualDisplay, 0);
        } finally {
            SurfaceControl.closeTransaction();
        }
        System.out.println("adb_sequence - activated!");
    }
}
