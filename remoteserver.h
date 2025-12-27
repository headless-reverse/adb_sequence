#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QJsonObject>
#include <QHostAddress>
#include <QTimer>
#include <QProcess>

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
    void onRunnerLog(const QString &text, const QString &color);
    void onRunnerFinished(bool success);
    
    void captureScreen();
    void onScreenCaptured(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QWebSocketServer *m_wsServer = nullptr;
    QList<QWebSocket *> m_clients;
    CommandExecutor *m_executor = nullptr;
    SequenceRunner *m_runner = nullptr;
    
    QTimer *m_screenTimer = nullptr;
    QProcess *m_screenProcess = nullptr;

    void sendMessageToAll(const QString &message);
    void handleCommand(QWebSocket *sender, const QJsonObject &json);
    QJsonObject createLogMessage(const QString &text, const QString &type = QStringLiteral("info")) const;
    QJsonObject createStatusMessage(const QString &status, const QString &message) const;
};
