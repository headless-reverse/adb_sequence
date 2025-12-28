#pragma once

#include <QObject>
#include <QTcpSocket>
#include "control_protocol.h"

class ControlSocket : public QObject {
    Q_OBJECT
public:
    explicit ControlSocket(QObject *parent = nullptr);
    
    void connectToLocalhost(quint16 port);
    void disconnectFromAgent();

    void sendTouchDown(uint16_t x, uint16_t y);
    void sendTouchMove(uint16_t x, uint16_t y);
    void sendTouchUp(uint16_t x = 0, uint16_t y = 0);
    void sendKey(uint16_t androidKeyCode);
    void connectToAgent(const QString &addr, quint16 port) { connectToLocalhost(port); }
    void sendTouch(int x, int y, int action) {
    if (action == 0) sendTouchDown(x, y);
    else if (action == 1) sendTouchUp(x, y);
    else if (action == 2) sendTouchMove(x, y);
}

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    
private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QTcpSocket::SocketError socketError);
    
private:
    QTcpSocket *m_socket;

    void sendPacket(ControlEventType type, uint16_t x = 0, uint16_t y = 0, uint16_t data = 0);
};
