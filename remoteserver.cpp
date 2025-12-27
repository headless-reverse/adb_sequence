#include "remoteserver.h"
#include "commandexecutor.h"
#include "sequencerunner.h"
#include <QImage>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QHostAddress>
#include <QProcess>
#include <QTimer>

RemoteServer::RemoteServer(const QString &adbPath, const QString &targetSerial, quint16 port, QObject *parent) 
    : QObject(parent) {
    
    m_executor = new CommandExecutor(this);
    m_executor->setAdbPath(adbPath);
    if (!targetSerial.isEmpty()) {
        m_executor->setTargetDevice(targetSerial);}
    m_runner = new SequenceRunner(m_executor, this);
    connect(m_runner, &SequenceRunner::logMessage, this, &RemoteServer::onRunnerLog);
    connect(m_runner, &SequenceRunner::sequenceFinished, this, &RemoteServer::onRunnerFinished);
    m_screenProcess = new QProcess(this);
    connect(m_screenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RemoteServer::onScreenCaptured);
    m_screenTimer = new QTimer(this);
//Interval screenTimer refresh
    m_screenTimer->setInterval(33); 
    m_screenTimer->setSingleShot(false);
    connect(m_screenTimer, &QTimer::timeout, this, &RemoteServer::captureScreen);
    m_wsServer = new QWebSocketServer(QStringLiteral("AdbSequenceServer"), 
                                      QWebSocketServer::NonSecureMode, this);
    if (m_wsServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Server started on ws://0.0.0.0:" << port;
        qDebug() << "Target ADB Path:" << m_executor->adbPath();
        qDebug() << "Target Device:" << (targetSerial.isEmpty() ? "AUTO/Any" : targetSerial);
        connect(m_wsServer, &QWebSocketServer::newConnection, this, &RemoteServer::onNewConnection);
    } else {
        qCritical() << "Error starting server:" << m_wsServer->errorString();}}


RemoteServer::~RemoteServer() {
    m_wsServer->close();
    if (m_screenTimer && m_screenTimer->isActive()) {
        m_screenTimer->stop();}
    qDeleteAll(m_clients.begin(), m_clients.end());}


void RemoteServer::onNewConnection() {
    QWebSocket *pSocket = m_wsServer->nextPendingConnection();
    qDebug() << "Client connected from" << pSocket->peerAddress().toString();
    connect(pSocket, &QWebSocket::textMessageReceived, this, &RemoteServer::onTextMessageReceived);
    connect(pSocket, &QWebSocket::disconnected, this, &RemoteServer::onSocketDisconnected);
    m_clients << pSocket;
    pSocket->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("connected"), QStringLiteral("Polaczono z serwerem AdbSequence."))).toJson(QJsonDocument::Compact));
    if (m_clients.size() == 1) {
        m_screenTimer->start();
        qDebug() << "Screen streaming started (10 FPS).";}}


void RemoteServer::onSocketDisconnected() {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        qDebug() << "Client disconnected from" << pClient->peerAddress().toString();
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        if (m_clients.isEmpty()) {
            m_screenTimer->stop();
            qDebug() << "Screen streaming stopped.";}}}


void RemoteServer::onTextMessageReceived(const QString &message) {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (!pClient) return;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        pClient->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Invalid JSON format."))).toJson(QJsonDocument::Compact));
        return;}
    handleCommand(pClient, doc.object());}


void RemoteServer::handleCommand(QWebSocket *sender, const QJsonObject &json) {
    if (!json.contains(QStringLiteral("command"))) {
        sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Missing 'command' field."))).toJson(QJsonDocument::Compact));
        return;}
    QString command = json[QStringLiteral("command")].toString();
    QJsonObject payload = json[QStringLiteral("payload")].toObject();
    if (command == QStringLiteral("loadSequence")) {
        if (!payload.contains(QStringLiteral("path"))) {
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Missing 'path' for loadSequence."))).toJson(QJsonDocument::Compact));
            return;}
        QString path = payload[QStringLiteral("path")].toString();
        if (m_runner->isRunning()) {
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Cannot load sequence while a sequence is running."))).toJson(QJsonDocument::Compact));
            return;}
        if (m_runner->appendSequence(path)) { 
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("ok"), QStringLiteral("Sekwencja zaladowana. Gotowa do startu."))).toJson(QJsonDocument::Compact));
        } else {
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Blad ladowania sekwencji. Sprawdz logi."))).toJson(QJsonDocument::Compact));}
    } else if (command == QStringLiteral("startSequence")) {
        if (m_runner->startSequence()) { 
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("ok"), QStringLiteral("Sekwencja uruchomiona."))).toJson(QJsonDocument::Compact));
        } else {
            sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Nie udalo sie uruchomic sekwencji. Upewnij sie ze zostala zaladowana."))).toJson(QJsonDocument::Compact));}
    } else if (command == QStringLiteral("stopSequence")) {
        m_runner->stopSequence();
        sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("ok"), QStringLiteral("Sekwencja zatrzymana."))).toJson(QJsonDocument::Compact));
    } else if (command == QStringLiteral("status")) {
        QString statusMsg = m_runner->isRunning() ? QStringLiteral("running") : QStringLiteral("idle");
        sender->sendTextMessage(QJsonDocument(createStatusMessage(statusMsg, QStringLiteral("Aktualny status SequenceRunnera."))).toJson(QJsonDocument::Compact));
    } else {
        sender->sendTextMessage(QJsonDocument(createStatusMessage(QStringLiteral("error"), QStringLiteral("Nieznana komenda."))).toJson(QJsonDocument::Compact));}}


void RemoteServer::captureScreen() {
    if (m_screenProcess->state() != QProcess::NotRunning) return;
    m_screenProcess->readAllStandardOutput(); 
    QStringList args;
    QString targetSerial = m_executor->targetDevice(); 
    QString adbPath = m_executor->adbPath();
    if (!targetSerial.isEmpty()) {
        args << "-s" << targetSerial;}
    // wysyła surowy bufor klatek (Raw Frame Buffer)
    args << "exec-out" << "screencap"; 
    m_screenProcess->start(adbPath, args);}

// Raw Frame Buffer (13 x 4 bajty = 52 bajty)
struct FramebufferHeader {
    quint32 width;
    quint32 height;
    quint32 bpp;
    quint32 size;
    quint32 offset;
    quint32 red_offset;
    quint32 red_length;
    quint32 green_offset;
    quint32 green_length;
    quint32 blue_offset;
    quint32 blue_length;
    quint32 alpha_offset;
    quint32 alpha_length;
};

void RemoteServer::onScreenCaptured(int exitCode, QProcess::ExitStatus) {
    const int HEADER_SIZE = 52; 
    if (exitCode == 0 && !m_clients.isEmpty()) {
        QByteArray data = m_screenProcess->readAllStandardOutput();
        if (data.size() < HEADER_SIZE) {
            qWarning() << "Screen capture failed: Data too short for raw frame buffer.";
            return;}
        const FramebufferHeader *header = reinterpret_cast<const FramebufferHeader*>(data.constData());
        quint32 w = header->width;
        quint32 h = header->height;
        quint32 bpp = header->bpp;
        if (bpp != 32) {
            qWarning() << "Unsupported BPP:" << bpp << ". Expected 32-bit format (RGBA/BGRA). Skipping frame.";
            return;}        
        const char* pixelData = data.constData() + HEADER_SIZE;
        int pixelDataSize = data.size() - HEADER_SIZE;
        if (pixelDataSize < (int)w * (int)h * (bpp / 8)) {
            qWarning() << "Incomplete pixel data received.";
            return;}
        QImage screenImage(
            (const uchar*)pixelData, 
            (int)w, 
            (int)h, 
            QImage::Format_RGB32 
        );
        if (!screenImage.isNull()) {
            QByteArray imageBytes;
            QBuffer buffer(&imageBytes);
            buffer.open(QIODevice::WriteOnly);
            screenImage.save(&buffer, "JPG", 70); 
            QJsonObject json;
            json["type"] = "screen";
            json["data"] = QString(imageBytes.toBase64());
            sendMessageToAll(QJsonDocument(json).toJson(QJsonDocument::Compact));
        } else {
             qWarning() << "Failed to create QImage from raw data.";}
    } else if (exitCode != 0) {
        QString error = m_screenProcess->readAllStandardError();
        if (!error.isEmpty()) {
            qWarning() << "Screen capture failed:" << error;}}}

void RemoteServer::onRunnerLog(const QString &text, const QString &color) {
    Q_UNUSED(color);
    sendMessageToAll(QJsonDocument(createLogMessage(text)).toJson(QJsonDocument::Compact));}

void RemoteServer::onRunnerFinished(bool success) {
    sendMessageToAll(QJsonDocument(createLogMessage(success ? QStringLiteral("Sekwencja zakończona sukcesem.") : QStringLiteral("Sekwencja zakończona błędem."), 
                                   success ? QStringLiteral("success") : QStringLiteral("error"))).toJson(QJsonDocument::Compact));
    sendMessageToAll(QJsonDocument(createStatusMessage(QStringLiteral("finished"), success ? QStringLiteral("Success") : QStringLiteral("Failure"))).toJson(QJsonDocument::Compact));}

void RemoteServer::sendMessageToAll(const QString &message) {
    for (QWebSocket *client : m_clients) {
        if (client->isValid()) {
            client->sendTextMessage(message);}}}

QJsonObject RemoteServer::createLogMessage(const QString &text, const QString &type) const {
    QJsonObject json;
    json[QStringLiteral("type")] = QStringLiteral("log");
    json[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json[QStringLiteral("message")] = text;
    json[QStringLiteral("logType")] = type;
    return json;}

QJsonObject RemoteServer::createStatusMessage(const QString &status, const QString &message) const {
    QJsonObject json;
    json[QStringLiteral("type")] = QStringLiteral("status");
    json[QStringLiteral("status")] = status;
    json[QStringLiteral("message")] = message;
    return json;}
