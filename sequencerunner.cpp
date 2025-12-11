#include "sequencerunner.h"
#include "commandexecutor.h"
#include "argsparser.h"
#include "nlohmann/json.hpp"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

SequenceRunner::SequenceRunner(CommandExecutor *executor, QObject *parent)
    : QObject(parent), m_executor(executor) {
    m_delayTimer.setSingleShot(true);
    connect(&m_delayTimer, &QTimer::timeout, this, &SequenceRunner::onDelayTimeout);
    connect(m_executor, &CommandExecutor::finished, this, &SequenceRunner::onCommandFinished);}

SequenceCmd SequenceRunner::parseCommandFromJson(const QJsonObject &obj) {
    SequenceCmd cmd;
    cmd.command = obj.value("command").toString();
    cmd.delayAfterMs = obj.value("delayAfterMs").toInt(1000);
    cmd.runMode = obj.value("runMode").toString("adb").toLower(); 
    cmd.stopOnError = obj.value("stopOnError").toBool(true);
    return cmd;}

void SequenceRunner::clearSequence() {
    m_commands.clear();
    emit logMessage("Sequence queue cleared.", "#BDBDBD");}

bool SequenceRunner::appendSequence(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit logMessage(QString("Cannot open file: %1").arg(filePath), "#F44336");
        return false;}
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    file.close();
    if (!doc.isArray()) {
        emit logMessage(QString("Invalid JSON in %1: Root must be Array.").arg(filePath), "#F44336");
        return false;}
    QJsonArray array = doc.array();
    int addedCount = 0;
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            m_commands.append(parseCommandFromJson(value.toObject()));
            addedCount++;}}
    emit logMessage(QString("Appended %1 commands from %2. Total in queue: %3").arg(addedCount).arg(filePath).arg(m_commands.count()), "#2196F3");
    return true;}

QStringList SequenceRunner::getCommandsAsText() const {
    QStringList result;
    for (const SequenceCmd &cmd : m_commands) {
        QString line = QString("[%1] %2").arg(cmd.runMode, cmd.command);
        if (cmd.delayAfterMs > 0) line += QString(" (Delay: %1ms)").arg(cmd.delayAfterMs);
        result.append(line);}
    return result;}

void SequenceRunner::startSequence() {
    if (m_isRunning) {
        emit logMessage("Sequence is already running.", "#FFAA66");
        return;}
    if (m_commands.isEmpty()) {
        emit logMessage("Sequence queue is empty. Load a file first.", "#F44336");
        return;}
    m_currentIndex = 0;
    m_isRunning = true;
    emit sequenceStarted();
    executeNextCommand();}

void SequenceRunner::stopSequence() {
    if (!m_isRunning) {
        emit logMessage("Sequence is not running.", "#BDBDBD");
        return;}
    m_executor->stop();
    finishSequence(false);
    emit logMessage("Sequence stopped by user.", "#FFAA66");}

void SequenceRunner::executeNextCommand() {
    if (!m_isRunning || m_currentIndex >= m_commands.count()) {
        finishSequence(true);
        return;}
    const SequenceCmd &currentCmd = m_commands.at(m_currentIndex);
    emit commandExecuting(currentCmd.command, m_currentIndex, m_commands.count());
    QStringList args;
    QString logPrefix;    
    QStringList parsedCmd = ArgsParser::parse(currentCmd.command);
    if (currentCmd.runMode == "root") {
        args << "shell" << "su" << "-c" << currentCmd.command;
        logPrefix = ">>> adb root (SEQ): ";
    } else if (currentCmd.runMode == "shell") {
        args.append("shell");
        args.append(parsedCmd);
        logPrefix = ">>> adb shell (SEQ): ";
    } else {
        args = parsedCmd;
        logPrefix = ">>> adb (SEQ): ";}
    emit logMessage(logPrefix + currentCmd.command, "#00BCD4");
    m_executor->runAdbCommand(args);}

void SequenceRunner::finishSequence(bool success) {
    if (!m_isRunning) return;
    m_isRunning = false;
    m_delayTimer.stop();
    emit sequenceFinished(success);
    if (success) {
        if (m_isInterval) {
            emit scheduleRestart(m_intervalValueS);
            emit logMessage(QString("Sequence finished. Restart in %1 seconds.").arg(m_intervalValueS), "#00BCD4");
        } else {
            emit logMessage("--- SEQUENCE FINISHED SUCCESSFULLY ---", "#4CAF50");}
    } else {
        emit logMessage("--- SEQUENCE TERMINATED WITH ERROR ---", "#F44336");}}

void SequenceRunner::onCommandFinished(int exitCode, QProcess::ExitStatus) {
    if (!m_isRunning) return;
    const SequenceCmd &currentCmd = m_commands.at(m_currentIndex);
    if (exitCode != 0 && currentCmd.stopOnError) {
        emit logMessage(QString("Sequence stopped: Command failed (code %1).").arg(exitCode), "#F44336");
        finishSequence(false);
        return;}  
    m_currentIndex++;
    if (m_currentIndex < m_commands.count()) {
        if (currentCmd.delayAfterMs > 0) {
            emit logMessage(QString("Waiting %1 ms...").arg(currentCmd.delayAfterMs), "#FFC107");
            m_delayTimer.setInterval(currentCmd.delayAfterMs);
            m_delayTimer.start();
        } else {
            executeNextCommand();}
    } else {
        finishSequence(true);}}

void SequenceRunner::setIntervalToggle(bool toggle) {
    m_isInterval = toggle;
    emit logMessage(QString("Sequence interval: %1").arg(toggle ? "ON" : "OFF"), "#BDBDBD");}

void SequenceRunner::setIntervalValue(int seconds) {
    m_intervalValueS = seconds;
    emit logMessage(QString("Interval duration: %1s").arg(seconds), "#BDBDBD");}

void SequenceRunner::onDelayTimeout() {
    executeNextCommand();}
