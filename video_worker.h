#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include "h264decoder.h"

class VideoWorker : public QObject {
    Q_OBJECT
public:
    explicit VideoWorker(H264Decoder *decoder = nullptr, QObject *parent = nullptr);
    
public slots:
    void startStream(const QString &deviceSerial, int localPort, int devicePort, const QString &adbPath);
    void stopStream();

signals:
    void frameReady(AVFramePtr frame);
    void statusUpdate(const QString &msg, bool isError = false);
    void finished();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QTcpSocket::SocketError socketError);
    void onFrameReady(AVFramePtr frame);

private:
    QTcpSocket m_socket;
    H264Decoder *m_decoder = nullptr;
    QByteArray m_readBuffer;
    uint8_t m_pendingPacketType;
    uint32_t m_pendingPacketSize;
    
    QString m_deviceSerial;
    int m_localPort;
    int m_devicePort;
    bool m_isConnected = false;

    enum { TYPE_VIDEO = 0x01, TYPE_META = 0x02 };
};

#endif
