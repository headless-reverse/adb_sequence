package dev.headless.sequence;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.view.Surface;
import java.io.DataOutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.WritableByteChannel;

public class ScreenEncoder {
    private MediaCodec encoder;
    private final int width;
    private final int height;
    private final int bitrate;
    
    private static final byte PACKET_TYPE_VIDEO = 1;

    public ScreenEncoder(int width, int height, int bitrate) {
        this.width = width;
        this.height = height;
        this.bitrate = bitrate;
    }

    public void stream(DataOutputStream out) throws Exception {
        MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, 60);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
        format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
        format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AVCLevel1);
        try {
            format.setInteger("prepend-sps-pps-to-idr-frames", 1);
        } catch (Exception ignored) {}
        format.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, 1000000 / 30); 
        encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
        encoder.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        Surface inputSurface = encoder.createInputSurface();
        Server.createDisplayMirror(inputSurface, width, height);
        encoder.start();
        WritableByteChannel channel = Channels.newChannel(out);
        try {
            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            while (!Thread.interrupted()) {
                int outputBufferIndex = encoder.dequeueOutputBuffer(bufferInfo, 10000);
                
                if (outputBufferIndex >= 0) {
                    ByteBuffer buf = encoder.getOutputBuffer(outputBufferIndex);
                    if (buf != null && bufferInfo.size > 0) {
                        out.writeByte(PACKET_TYPE_VIDEO);
                        out.writeInt(bufferInfo.size);
                        buf.position(bufferInfo.offset);
                        buf.limit(bufferInfo.offset + bufferInfo.size);
                        channel.write(buf);
                        out.flush();}
                    encoder.releaseOutputBuffer(outputBufferIndex, false);
                } else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    System.out.println("Encoder format changed: " + encoder.getOutputFormat());}}
        } finally {
            cleanup(inputSurface);}}

    private void cleanup(Surface inputSurface) {
        try {
            if (encoder != null) {
                encoder.stop();
                encoder.release();}
            if (inputSurface != null) {
                inputSurface.release();}
        } catch (Exception ignored) {}
    }
}
