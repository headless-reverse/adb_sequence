#include "remoteserver.h"
#include "commandexecutor.h"
#include "sequencerunner.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QHostAddress>
#include <QDateTime>
#include <QTimer>

RemoteServer::RemoteServer(const QString &adbPath, const QString &targetSerial, quint16 port, QObject *parent)
    : QObject(parent)
{
    m_executor = new CommandExecutor(this);
    m_executor->setAdbPath(adbPath);
    if (!targetSerial.isEmpty()) {
        m_executor->setTargetDevice(targetSerial);}
    m_runner = new SequenceRunner(m_executor, this);
    connect(m_runner, &SequenceRunner::logMessage, this, &RemoteServer::onRunnerLog);
    connect(m_runner, &SequenceRunner::sequenceFinished, this, &RemoteServer::onRunnerFinished);
    m_wsServer = new QWebSocketServer(QStringLiteral("AdbSequenceServer"),
                                      QWebSocketServer::NonSecureMode, this);
    if (m_wsServer->listen(QHostAddress::Any, port)) {
        qDebug() << "RemoteServer listening on ws://0.0.0.0:" << port;
        connect(m_wsServer, &QWebSocketServer::newConnection, this, &RemoteServer::onNewConnection);
    } else {
        qCritical() << "RemoteServer failed to listen on port" << port << ":" << m_wsServer->errorString();}
    m_agentSocket = new QTcpSocket(this);
    connect(m_agentSocket, &QTcpSocket::connected, this, &RemoteServer::onAgentConnected);
    connect(m_agentSocket, &QTcpSocket::disconnected, this, &RemoteServer::onAgentDisconnected);
    connect(m_agentSocket, &QTcpSocket::readyRead, this, &RemoteServer::onAgentReadyRead);
    connect(m_agentSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &RemoteServer::onAgentError);}

RemoteServer::~RemoteServer() {
    if (m_wsServer) m_wsServer->close();
    stopAgentAndDisconnect();
}

void RemoteServer::onNewConnection() {
    QWebSocket *socket = m_wsServer->nextPendingConnection();
    if (!socket) return;
    qDebug() << "[RemoteServer] New WS client from" << socket->peerAddress().toString();
    connect(socket, &QWebSocket::textMessageReceived, this, &RemoteServer::onTextMessageReceived);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &RemoteServer::onBinaryMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &RemoteServer::onSocketDisconnected);
    m_clients << socket;
    socket->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("connected"), QStringLiteral("Connected to AdbSequence remote server."))).toJson(QJsonDocument::Compact));
    if (m_clients.size() == 1) {
        QTimer::singleShot(50, this, &RemoteServer::startAgentAndConnect);}}

void RemoteServer::onSocketDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;
    m_clients.removeAll(client);
    client->deleteLater();
    if (m_clients.isEmpty()) {
        stopAgentAndDisconnect();}}

void RemoteServer::onTextMessageReceived(const QString &message) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;
    handleCommand(client, doc.object());}

void RemoteServer::onBinaryMessageReceived(const QByteArray &message) {
    Q_UNUSED(message);
    // Tutaj możesz obsłużyć np. zdarzenia dotyku przesyłane binarnie z przeglądarki
}

void RemoteServer::onRawVideoPacketReceived(const QByteArray &packet) {
    Q_UNUSED(packet);}

void RemoteServer::onAgentReadyRead() {
    m_agentBuffer.append(m_agentSocket->readAll());
    while (true) {
        if (m_agentBuffer.size() < 5) break;
        quint8 type = static_cast<quint8>(m_agentBuffer.at(0));
        const uchar *ptr = reinterpret_cast<const uchar*>(m_agentBuffer.constData() + 1);
        quint32 payloadSize = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        if (m_agentBuffer.size() < static_cast<int>(5 + payloadSize)) break;
        QByteArray framed = m_agentBuffer.left(5 + payloadSize);
        m_agentBuffer.remove(0, 5 + payloadSize);
        for (QWebSocket *client : m_clients) {
            if (client && client->isValid()) {
                client->sendBinaryMessage(framed);}}}}

void RemoteServer::handleCommand(QWebSocket *sender, const QJsonObject &json) {
    QString command = json[QStringLiteral("command")].toString();
    QJsonObject payload = json[QStringLiteral("payload")].toObject();
    if (command == QStringLiteral("loadSequence")) {
        QString path = payload[QStringLiteral("path")].toString();
        if (m_runner->appendSequence(path)) {
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("ok"), "Loaded")).toJson());}
    } else if (command == QStringLiteral("startSequence")) {
        m_runner->startSequence();
    } else if (command == QStringLiteral("stopSequence")) {
        m_runner->stopSequence();}}

bool RemoteServer::deployAgentJar() {
    QString jarPath = "/opt/build/adb_sequence/adb_sequence_pro/android/sequence.jar";
    QString adb = m_executor->adbPath();
    QProcess push;
    QStringList args;
    if (!m_executor->targetDevice().isEmpty()) args << "-s" << m_executor->targetDevice();
    args << "push" << jarPath << "/data/local/tmp/sequence.jar";
    push.start(adb, args);
    return push.waitForFinished() && push.exitCode() == 0;}

void RemoteServer::startAgentAndConnect() {
    if (!deployAgentJar()) return;
    QString adb = m_executor->adbPath();
    QStringList args;
    if (!m_executor->targetDevice().isEmpty()) args << "-s" << m_executor->targetDevice();
    QProcess::execute(adb, args << "forward" << QString("tcp:%1").arg(m_localPort) << QString("tcp:%1").arg(m_devicePort));
    m_agentProcess = new QProcess(this);
    QString shellCmd = "CLASSPATH=/data/local/tmp/sequence.jar app_process /data/local/tmp dev.headless.sequence.Server";
    m_agentProcess->start(adb, args << "shell" << shellCmd);
    QTimer::singleShot(2000, this, [this]() {
        m_agentSocket->connectToHost(QHostAddress::LocalHost, m_localPort);});}

void RemoteServer::stopAgentAndDisconnect() {
    if (m_agentSocket) m_agentSocket->abort();
    if (m_agentProcess) {
        m_agentProcess->kill();
        m_agentProcess->deleteLater();
        m_agentProcess = nullptr;}}

void RemoteServer::onAgentConnected() { qDebug() << "Agent Connected"; }
void RemoteServer::onAgentDisconnected() { qDebug() << "Agent Disconnected"; }
void RemoteServer::onAgentError(QAbstractSocket::SocketError err) { Q_UNUSED(err); }
void RemoteServer::onAgentProcessFinished(int exitCode, QProcess::ExitStatus es) { Q_UNUSED(es); qDebug() << "Exit code:" << exitCode; }
void RemoteServer::onAgentProcessError(QProcess::ProcessError err) { Q_UNUSED(err); }

void RemoteServer::onRunnerLog(const QString &text, const QString &color) {
    Q_UNUSED(color);
    sendMessageToAll(QJsonDocument(createLogMessage(text)).toJson(QJsonDocument::Compact));
}

void RemoteServer::onRunnerFinished(bool success) {
    sendMessageToAll(QJsonDocument(createStatusMessage(QStringLiteral("finished"), success ? "Success" : "Failed")).toJson());
}

void RemoteServer::sendMessageToAll(const QString &message) {
    for (QWebSocket *client : m_clients) {
        if (client && client->isValid()) client->sendTextMessage(message);}}

QJsonObject RemoteServer::createLogMessage(const QString &text, const QString &type) const {
    QJsonObject json;
    json["type"] = "log";
    json["message"] = text;
    json["logType"] = type;
    json["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return json;}

QJsonObject RemoteServer::createStatusMessage(const QString &status, const QString &message) const {
    QJsonObject json;
    json["type"] = "status";
    json["status"] = status;
    json["message"] = message;
    return json;}
