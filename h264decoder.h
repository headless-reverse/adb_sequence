#ifndef H264DECODER_H
#define H264DECODER_H

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

using AVFramePtr = std::shared_ptr<AVFrame>;

class H264Decoder : public QObject {
    Q_OBJECT
public:
    explicit H264Decoder(QObject *parent = nullptr);
    ~H264Decoder();

    bool init();
    bool initSize(int width, int height);
    void decode(const QByteArray &packet);

signals:
    void frameReady(AVFramePtr frame);
    void decoderError(const QString &msg);

private:
    void parseAndDecode(const QByteArray &packet);
    void decodePacket(AVPacket *pkt);
    void cleanup();

    static int read_socket_callback(void *opaque, uint8_t *buf, int buf_size);

    // Konteksty FFmpeg
    AVCodecContext *m_codecCtx = nullptr;
    const AVCodec *m_codec = nullptr;
    AVCodecParserContext *m_parser = nullptr;
    AVFrame *m_frame = nullptr;
    AVPacket *m_pkt = nullptr;
    
    AVFormatContext *m_formatCtx = nullptr; 
    
    QMutex m_mutex;
    int m_width = 0;
    int m_height = 0;
};

#endif
