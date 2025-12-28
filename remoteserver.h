#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QJsonObject>
#include <QHostAddress>
#include <QTimer>
#include <QProcess>
#include <QTcpSocket>
#include "video_worker.h"

class CommandExecutor;
class SequenceRunner;

class RemoteServer : public QObject {
    Q_OBJECT
public:
    explicit RemoteServer(const QString &adbPath, const QString &targetSerial,
                          quint16 port = 12345, QObject *parent = nullptr);
    ~RemoteServer();

private slots:
    void onNewConnection();
    void onSocketDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onRawVideoPacketReceived(const QByteArray &data);
    void onRunnerLog(const QString &text, const QString &color);
    void onRunnerFinished(bool success);

    // agent/socket lifecycle
    void startAgentAndConnect();
    void stopAgentAndDisconnect();
    void onAgentConnected();
    void onAgentDisconnected();
    void onAgentReadyRead();
    void onAgentError(QAbstractSocket::SocketError err);
    void onAgentProcessError(QProcess::ProcessError err);
    void onAgentProcessFinished(int exitCode, QProcess::ExitStatus es);

private:
    QWebSocketServer *m_wsServer = nullptr;
    QList<QWebSocket *> m_clients;
    CommandExecutor *m_executor = nullptr;
    SequenceRunner *m_runner = nullptr;
    
    // Moduł wideo pracujący w tle
    VideoWorker *m_videoWorker = nullptr;

    // agent process + socket
    QProcess *m_agentProcess = nullptr;
    QTcpSocket *m_agentSocket = nullptr;
    QByteArray m_agentBuffer;
    quint16 m_localPort = 7373;
    quint16 m_devicePort = 7373;

    void sendMessageToAll(const QString &message);
    void handleCommand(QWebSocket *sender, const QJsonObject &json);
    QJsonObject createLogMessage(const QString &text, const QString &type = QStringLiteral("info")) const;
    QJsonObject createStatusMessage(const QString &status, const QString &message) const;
    bool deployAgentJar();
};
