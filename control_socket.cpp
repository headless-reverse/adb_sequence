#include "control_socket.h"
#include "control_protocol.h"
#include <QDebug>
#include <QHostAddress>
#include <QtEndian>

ControlSocket::ControlSocket(QObject *parent) 
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    
    connect(m_socket, &QTcpSocket::connected, this, &ControlSocket::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ControlSocket::onDisconnected);
    connect(m_socket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &ControlSocket::onErrorOccurred);
}

void ControlSocket::connectToLocalhost(quint16 port) {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_socket->connectToHost(QHostAddress::LocalHost, port);
}

void ControlSocket::disconnectFromAgent() {
    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
}

void ControlSocket::onConnected() {
    emit connected();
    qDebug() << "Control Socket: Connection established with Android Daemon.";
}

void ControlSocket::onDisconnected() {
    emit disconnected();
    qDebug() << "Control Socket: Connection closed.";
}

void ControlSocket::onErrorOccurred(QTcpSocket::SocketError socketError) {
    if (socketError != QTcpSocket::RemoteHostClosedError) {
        emit errorOccurred(QString("Control Socket Error: %1").arg(m_socket->errorString()));
    }
}

void ControlSocket::sendPacket(ControlEventType type, uint16_t x, uint16_t y, uint16_t data) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    ControlPacket pkt;
    pkt.magic = qToBigEndian<uint32_t>(CONTROL_MAGIC);
    pkt.type  = (uint8_t)type;
    pkt.x     = qToBigEndian<uint16_t>(x);
    pkt.y     = qToBigEndian<uint16_t>(y);
    pkt.data  = qToBigEndian<uint16_t>(data);

    m_socket->write(reinterpret_cast<const char*>(&pkt), sizeof(ControlPacket));
    
    if (type != EVENT_TYPE_TOUCH_MOVE) {
        m_socket->flush();
    }
}

void ControlSocket::sendTouchDown(uint16_t x, uint16_t y) {
    sendPacket(EVENT_TYPE_TOUCH_DOWN, x, y);
}

void ControlSocket::sendTouchMove(uint16_t x, uint16_t y) {
    sendPacket(EVENT_TYPE_TOUCH_MOVE, x, y);
}

void ControlSocket::sendTouchUp(uint16_t x, uint16_t y) {
    sendPacket(EVENT_TYPE_TOUCH_UP, x, y);
}

void ControlSocket::sendKey(uint16_t androidKeyCode) {
    sendPacket(EVENT_TYPE_KEY, 0, 0, androidKeyCode);
}
