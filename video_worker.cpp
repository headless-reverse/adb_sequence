#include "video_worker.h"
#include "h264decoder.h"
#include <QHostAddress>
#include <QtEndian>
#include <QDebug>
#include <QThread>
#include <QTimer>

static const int RECONNECT_MS = 500;

VideoWorker::VideoWorker(H264Decoder *decoder, QObject *parent) 
    : QObject(parent), m_decoder(decoder), m_pendingPacketSize(0) {
    m_socket.setParent(this); 
    connect(&m_socket, &QTcpSocket::connected, this, &VideoWorker::onSocketConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &VideoWorker::onSocketDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &VideoWorker::onSocketReadyRead);
    connect(&m_socket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::errorOccurred), 
            this, &VideoWorker::onSocketError);
    if (m_decoder) {
        connect(m_decoder, &H264Decoder::frameReady, this, &VideoWorker::onFrameReady);}}

void VideoWorker::startStream(const QString &deviceSerial, int localPort, int devicePort, const QString &adbPath) {
    Q_UNUSED(adbPath);
    m_deviceSerial = deviceSerial;
    m_localPort = localPort;
    m_devicePort = devicePort;
    m_readBuffer.clear();
    m_pendingPacketSize = 0;
    if (!m_decoder) {
        qDebug() << "[Worker] Creating decoder in thread:" << QThread::currentThread();
        m_decoder = new H264Decoder(this);
        connect(m_decoder, &H264Decoder::frameReady, this, &VideoWorker::onFrameReady);}
    if (!m_decoder || m_decoder->thread() != QThread::currentThread()) {
        emit statusUpdate("Błąd krytyczny: Dekoder w złym wątku!", true);
        emit finished();
        return;}
    emit statusUpdate(QString("Łączenie z agentem wideo (port %1)...").arg(localPort));
    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        m_socket.connectToHost(QHostAddress::LocalHost, m_localPort);}}

void VideoWorker::onSocketReadyRead() {
    m_readBuffer.append(m_socket.readAll());
    while (true) {
        if (m_pendingPacketSize == 0) {
            if (m_readBuffer.size() < 5) break;
            m_pendingPacketType = static_cast<uint8_t>(m_readBuffer[0]);
            uint32_t rawLen = 0;
            memcpy(&rawLen, m_readBuffer.constData() + 1, 4);
            m_pendingPacketSize = qFromBigEndian(rawLen);
            m_readBuffer.remove(0, 5);}
        if (m_readBuffer.size() < (int)m_pendingPacketSize) break;
        QByteArray payload = m_readBuffer.left(m_pendingPacketSize);
        m_readBuffer.remove(0, m_pendingPacketSize);
        if (m_pendingPacketType == 0x01) {
            if (m_decoder) {
                m_decoder->decode(payload);}}
        m_pendingPacketSize = 0;}}

void VideoWorker::onFrameReady(AVFramePtr frame) {
    emit frameReady(frame);}

void VideoWorker::stopStream() {
    m_socket.close();
    emit finished();}

void VideoWorker::onSocketConnected() {
    emit statusUpdate("Strumień aktywny.");}

void VideoWorker::onSocketDisconnected() {
    emit statusUpdate("Rozłączono.", true);
    QTimer::singleShot(RECONNECT_MS, this, [this]() {
        if (m_socket.state() == QAbstractSocket::UnconnectedState)
            m_socket.connectToHost(QHostAddress::LocalHost, m_localPort);});}

void VideoWorker::onSocketError(QTcpSocket::SocketError) {
    m_socket.close();}
