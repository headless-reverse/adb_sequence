#include "adb_client.h"
#include <QDebug>
#include <QHostAddress>
#include <QElapsedTimer>

AdbClient::AdbClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    // Połączenie sygnałów gniazda
    connect(m_socket, &QTcpSocket::connected, this, &AdbClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &AdbClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &AdbClient::onReadyRead);
    connect(m_socket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &AdbClient::onErrorOccurred);}

void AdbClient::setTargetDevice(const QString &serial) {
    m_targetSerial = serial;
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();}}

void AdbClient::connectToAdbServer(const QString &host, quint16 port) {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (!m_socket->waitForDisconnected(100)) {
            qWarning() << "AdbClient: Poprzednie połączenie nie zostało rozłączone.";
        }}
    m_state = Idle;
    m_readBuffer.clear();
    m_socket->connectToHost(QHostAddress(host), port);}

void AdbClient::onConnected() {
    emit adbConnected();}

void AdbClient::onDisconnected() {
    emit adbDisconnected();}

void AdbClient::onErrorOccurred(QTcpSocket::SocketError socketError) {
    if (socketError != QTcpSocket::RemoteHostClosedError) {
        emit adbError(QString("Błąd połączenia z ADB: %1").arg(m_socket->errorString()));}}

void AdbClient::writeAdbHeader(const QString &command) {
    // Nagłówek ADB  4-bajtowa długość w HEX ASCII
    QByteArray header = QByteArray::number(command.length(), 16).rightJustified(4, '0').toUpper();
    m_socket->write(header);
    m_socket->write(command.toUtf8());
    m_state = WaitingForAdbStatus;}

void AdbClient::sendAdbCommand(const QString &command) {
    connectToAdbServer(); 
    if (m_socket->waitForConnected(1000)) {
        writeAdbHeader(command);
    } else {
        emit adbError("Nie można połączyć się z serwerem ADB (5037).");}}

void AdbClient::sendDeviceCommand(const QString &command) {
    if (m_targetSerial.isEmpty()) {
        emit adbError("Nie ustawiono urządzenia docelowego (serial).");
        return;}
    connectToAdbServer();
    if (!m_socket->waitForConnected(1000)) {
        emit adbError("Nie można połączyć się z serwerem ADB (5037).");
        return;}
    QElapsedTimer timer;
    timer.start();
    QString transportCommand = QString("host:transport:%1").arg(m_targetSerial);
    writeAdbHeader(transportCommand);
    if (!m_socket->waitForReadyRead(500)) {
        emit adbError("Timeout: Brak odpowiedzi po żądaniu transportu.");
        return;}
    if (m_socket->bytesAvailable() >= 4) {
        QByteArray status = m_socket->read(4);
        if (status != "OKAY") {
            if (status == "FAIL") {
                if (m_socket->waitForReadyRead(500) && m_socket->bytesAvailable() >= 4) {
                    QByteArray lengthHex = m_socket->read(4);
                    bool ok;
                    int length = lengthHex.toInt(&ok, 16);
                    if (ok && m_socket->waitForReadyRead(500) && m_socket->bytesAvailable() >= length) {
                        QByteArray errorMsg = m_socket->read(length);
                        emit adbError(QString("Błąd transportu do %1: %2").arg(m_targetSerial, QString::fromUtf8(errorMsg)));
                        return;}}}
            emit adbError(QString("Błąd transportu: nieznany status %1").arg(QString::fromUtf8(status)));
            return;}
    } else {
        emit adbError("Błąd transportu: Niekompletny status OKAY/FAIL.");
        return;}
    writeAdbHeader(command);
    m_state = WaitingForDeviceData;
    if (command.startsWith("shell:input")) {
        m_socket->disconnectFromHost();}
    qDebug() << "Komenda wykonana w (ms):" << timer.elapsed();}

void AdbClient::onReadyRead() {
    m_readBuffer.append(m_socket->readAll());
    if (m_state == WaitingForAdbStatus) {
        processAdbStatus();
    } else if (m_state == WaitingForDeviceData) {
        handleDeviceData();}}

void AdbClient::processAdbStatus() {
    if (m_readBuffer.size() >= 4) {
        QByteArray status = m_readBuffer.left(4);
        m_readBuffer.remove(0, 4);
        if (status == "OKAY") {
            m_state = WaitingForDeviceData;
            if (m_socket->bytesAvailable() == 0 && !m_readBuffer.isEmpty()) {
                emit commandResponseReady(m_readBuffer);
                m_readBuffer.clear();
                m_socket->disconnectFromHost();}
        } else if (status == "FAIL") {
            if (m_readBuffer.size() >= 4) {
                QByteArray lengthHex = m_readBuffer.left(4);
                m_readBuffer.remove(0, 4);
                bool ok;
                int length = lengthHex.toInt(&ok, 16);

                if (ok && m_readBuffer.size() >= length) {
                    QByteArray errorMsg = m_readBuffer.left(length);
                    m_readBuffer.remove(0, length);
                    emit adbError(QString("Błąd ADB: %1").arg(QString::fromUtf8(errorMsg)));
                } else {
                    emit adbError("Błąd ADB: Niekompletna wiadomość o błędzie.");}}
            m_state = Idle;
            m_socket->disconnectFromHost();
        } else {
            emit adbError(QString("Błąd protokołu ADB: Nieznany status '%1'").arg(QString::fromUtf8(status)));
            m_state = Idle;
            m_socket->disconnectFromHost();}}}

void AdbClient::handleDeviceData() {
    if (!m_readBuffer.isEmpty()) {
        emit rawDataReady(m_readBuffer);
        m_readBuffer.clear();}
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_state = Idle;}}
