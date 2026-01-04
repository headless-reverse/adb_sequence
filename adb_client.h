#pragma once
#include <QObject>
#include <QTcpSocket>

class AdbClient : public QObject {
    Q_OBJECT
public:
    explicit AdbClient(QObject *parent = nullptr);
    void setTargetDevice(const QString &serial);
    QString targetDevice() const { return m_targetSerial; }
    void connectToAdbServer(const QString &host = "127.0.0.1", quint16 port = 5037);
    void sendAdbCommand(const QString &command);
    void sendDeviceCommand(const QString &command);

signals:
    void adbConnected();
    void adbDisconnected();
    void commandResponseReady(const QByteArray &response); 
    void rawDataReady(const QByteArray &data);
    void adbError(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QTcpSocket::SocketError socketError);

private:
    enum State {
        Idle,
        WaitingForAdbStatus,
        WaitingForDeviceData
    };
    State m_state = Idle;
    
    QTcpSocket *m_socket;
    QByteArray m_readBuffer;
    QString m_targetSerial;

    void writeAdbHeader(const QString &command);
    void processAdbStatus();
    void handleDeviceData();
};
