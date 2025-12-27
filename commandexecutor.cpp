#include "commandexecutor.h"
#include <QByteArray>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include "adb_client.h" 

CommandExecutor::CommandExecutor(QObject *parent) : QObject(parent) {
    m_adbPath = "adb";
    m_targetSerial = QString();
    m_adbClient = new AdbClient(this);
    connect(m_adbClient, &AdbClient::adbError,
            this, &CommandExecutor::onAdbClientError);
    connect(m_adbClient, &AdbClient::rawDataReady,
            this, &CommandExecutor::onAdbClientRawDataReady);
    connect(m_adbClient, &AdbClient::commandResponseReady,
            this, &CommandExecutor::onAdbClientCommandResponseReady);
    m_shellProcess = new QProcess(this);
    connect(m_shellProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, &CommandExecutor::onShellProcessFinished);
    connect(m_shellProcess, &QProcess::readyReadStandardOutput, this, &CommandExecutor::readStdOut);
    connect(m_shellProcess, &QProcess::readyReadStandardError, this, &CommandExecutor::readStdErr);
}

CommandExecutor::~CommandExecutor() {stop();}

void CommandExecutor::setAdbPath(const QString &path) {if (!path.isEmpty()) m_adbPath = path;}

void CommandExecutor::setTargetDevice(const QString &serial) {
    m_targetSerial = serial;
    if (m_adbClient) {
        m_adbClient->setTargetDevice(serial);}
    if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
        m_shellProcess->kill();
        m_shellProcess->waitForFinished(500);}}

void CommandExecutor::stop() {
    if (m_process) {
        if (m_process->state() == QProcess::Running) {
            m_process->kill();
            m_process->waitForFinished(500);}
        m_process->disconnect();
        delete m_process;
        m_process = nullptr;}
    if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
        m_shellProcess->kill();    
        m_shellProcess->waitForFinished(500);}}

void CommandExecutor::cancelCurrentCommand() {stop();}

bool CommandExecutor::isRunning() const {return m_process && m_process->state() == QProcess::Running; }

void CommandExecutor::runAdbCommand(const QStringList &args) {
    stop();
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &CommandExecutor::readStdOut);
    connect(m_process, &QProcess::readyReadStandardError, this, &CommandExecutor::readStdErr);
    connect(m_process, &QProcess::started, this, &CommandExecutor::onStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
    this, &CommandExecutor::onFinished);
    QStringList finalArgs;
    if (!m_targetSerial.isEmpty()) {
        finalArgs << "-s" << m_targetSerial;}
    finalArgs.append(args);
    m_process->start(m_adbPath, finalArgs);}

void CommandExecutor::executeAdbCommand(const QString &command) {
    QStringList args = command.split(' ', Qt::SkipEmptyParts);
    runAdbCommand(args);
}

void CommandExecutor::executeShellCommand(const QString &command) {
    ensureShellRunning();
    if (m_shellProcess->state() != QProcess::Running) {
        qCritical() << "Cannot execute shell command, persistent shell is not running.";
        return;}
    QByteArray cmdData = (command + "\n").toUtf8();
    m_shellProcess->write(cmdData);}

void CommandExecutor::executeRootShellCommand(const QString &command) {
    runAdbCommand(QStringList() << "shell" << "su" << "-c" << command);}

void CommandExecutor::ensureShellRunning() {
    if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
        return;}
    qDebug() << "Starting persistent ADB shell process...";
    QStringList args;
    if (!m_targetSerial.isEmpty()) {
        args << "-s" << m_targetSerial;}
    args << "shell";
    m_shellProcess->start(m_adbPath, args);
    if (!m_shellProcess->waitForStarted(5000)) {    
        qCritical() << "Failed to start persistent ADB shell!";
    } else {
        qDebug() << "Persistent ADB shell started.";}}

void CommandExecutor::onShellProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "Persistent Shell process finished unexpectedly. Exit code:" << exitCode << "Status:" << exitStatus;
}

void CommandExecutor::onAdbClientError(const QString &message) {
    emit errorReceived(QString("[ADB SOCKET ERROR] %1").arg(message));
    emit finished(1, QProcess::NormalExit); }

void CommandExecutor::onAdbClientRawDataReady(const QByteArray &data) {
    emit rawDataReady(data);}

void CommandExecutor::onAdbClientCommandResponseReady(const QByteArray &response) {
    emit outputReceived(QString::fromUtf8(response));
    emit finished(0, QProcess::NormalExit);}

void CommandExecutor::readStdOut() {
    if (m_process && sender() == m_process) {    
        const QByteArray data = m_process->readAllStandardOutput();
        if (!data.isEmpty()) emit outputReceived(QString::fromUtf8(data));}    
    else if (m_shellProcess && sender() == m_shellProcess) {    
        const QByteArray data = m_shellProcess->readAllStandardOutput();
        if (!data.isEmpty()) {
            emit outputReceived("[SHELL] " + QString::fromUtf8(data));}}}

void CommandExecutor::readStdErr() {
    if (m_process && sender() == m_process) {
        const QByteArray data = m_process->readAllStandardError();
        if (!data.isEmpty()) emit errorReceived(QString::fromUtf8(data));}
    else if (m_shellProcess && sender() == m_shellProcess) {
        const QByteArray data = m_shellProcess->readAllStandardError();
        if (!data.isEmpty()) {
             emit errorReceived("[SHELL ERROR] " + QString::fromUtf8(data));}}}

void CommandExecutor::onStarted() {emit started();}

void CommandExecutor::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    emit finished(exitCode, exitStatus);
    if (m_process) {
        m_process->disconnect();    
        m_process->deleteLater();    
        m_process = nullptr;}}

void CommandExecutor::executeSequenceCommand(const QString &command, const QString &runMode) {
    QString mode = runMode.toLower();    
    if (mode == "shell" && command.startsWith("input ")) {
        if (m_adbClient && !m_targetSerial.isEmpty() && m_adbClient->targetDevice() == m_targetSerial) {
            qDebug() << "Executing command via AdbClient (Socket):" << command;
            m_adbClient->sendDeviceCommand("shell:" + command);
            emit finished(0, QProcess::NormalExit); 
            return;}
        qWarning() << "AdbClient nie jest gotowy lub brak urządzenia docelowego. Powrót do Persistent Shell dla:" << command;
    }
    if (mode == "shell" && command.startsWith("input ")) {
        ensureShellRunning();
        if (m_shellProcess->state() != QProcess::Running) {
            qCritical() << "Persistent shell is not running, cannot execute fast command:" << command;
            emit finished(1, QProcess::NormalExit);
            return;}
        QByteArray cmdData = (command + "\n").toUtf8();
        m_shellProcess->write(cmdData);
        emit finished(0, QProcess::NormalExit); 
        return;}
    else if (mode == "root") {
        executeRootShellCommand(command);
        return;
    }
    
    else if (mode == "shell") {
        runAdbCommand(QStringList() << "shell" << command);
        return;}
    else {
        executeAdbCommand(command);
        return;}}
