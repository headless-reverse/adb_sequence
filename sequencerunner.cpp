#include "sequencerunner.h"
#include "commandexecutor.h"
#include "argsparser.h"
#include "adb_client.h"
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
    connect(m_executor, &CommandExecutor::finished, this, &SequenceRunner::onCommandFinished);
}

SequenceRunner::~SequenceRunner() {}

SequenceCmd SequenceRunner::parseCommandFromJson(const QJsonObject &obj) {
    SequenceCmd cmd;
    cmd.command = obj.value("command").toString();
    cmd.delayAfterMs = obj.value("delayAfterMs").toInt(100);
    cmd.runMode = obj.value("runMode").toString("adb").toLower(); 
    cmd.stopOnError = obj.value("stopOnError").toBool(true);
    cmd.successCommand = obj.value("successCommand").toString();
    cmd.failureCommand = obj.value("failureCommand").toString();
    return cmd;}

void SequenceRunner::clearSequence() {
    m_commands.clear();
    emit logMessage("Sequence queue cleared.", "#BDBDBD");}

bool SequenceRunner::appendSequence(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit logMessage(QString("Cannot open file: %1").arg(file.errorString()), "#F44336");
        return false;}
    QByteArray jsonData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) {
        emit logMessage("File does not contain a valid JSON array.", "#F44336");
        return false;}
    return loadSequenceFromJsonArray(doc.array());}

bool SequenceRunner::loadSequenceFromJsonArray(const QJsonArray &array) {
    if (m_isRunning) {
        emit logMessage("Cannot load sequence while one is running.", "#F44336");
        return false;}
    m_commands.clear();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            m_commands.append(parseCommandFromJson(value.toObject()));
        } else {
            emit logMessage("Invalid command format in JSON array.", "#F44336");
            return false;}}
    emit logMessage(QString("Loaded %1 commands.").arg(m_commands.count()), "#4CAF50");
    return true;}

QStringList SequenceRunner::getCommandsAsText() const {
    QStringList list;
    for (const auto &cmd : m_commands) {
        QString text = cmd.command;
        if (!cmd.successCommand.isEmpty()) {
            text += QString(" (Sukces: '%1')").arg(cmd.successCommand);}
        if (!cmd.failureCommand.isEmpty()) {
            text += QString(" (Błąd: '%1')").arg(cmd.failureCommand);}
        list.append(text);}
    return list;}

bool SequenceRunner::startSequence() {
    if (m_commands.isEmpty()) {
        emit logMessage("Sequence is empty. Nothing to start.", "#FFC107");
        return false;}
    if (m_isRunning) {
        emit logMessage("Sequence is already running.", "#FFC107");
        return true;}
    m_currentIndex = 0;
    m_isRunning = true;
    emit sequenceStarted();
    emit logMessage("--- SEQUENCE STARTED ---", "#009688");
    executeNextCommand();
    return true;}

void SequenceRunner::stopSequence() {
    if (!m_isRunning) return;    
    m_isRunning = false;
    m_delayTimer.stop();
    m_executor->cancelCurrentCommand(); 
    finishSequence(false);}

void SequenceRunner::executeNextCommand() {
    if (!m_isRunning || m_currentIndex >= m_commands.count()) {
        finishSequence(true);
        return;}
    const SequenceCmd &cmd = m_commands.at(m_currentIndex);
    if (cmd.isConditionalExecution) {
        emit logMessage(QString("Wykonywanie komendy warunkowej: %1").arg(cmd.command), "#FF9800");
    } else {
        emit commandExecuting(cmd.command, m_currentIndex + 1, m_commands.count());}
    m_executor->executeSequenceCommand(cmd.command, cmd.runMode);}

void SequenceRunner::onDelayTimeout() {
    executeNextCommand();}

void SequenceRunner::executeConditionalCommand(const QString& cmd, const QString& runMode, bool isSuccess) {
    if (cmd.isEmpty()) return;
    SequenceCmd conditionalCmd;
    conditionalCmd.command = cmd;
    conditionalCmd.runMode = runMode;
    conditionalCmd.delayAfterMs = 0;
    conditionalCmd.stopOnError = true;
    conditionalCmd.isConditionalExecution = true;
    m_commands.insert(m_currentIndex + 1, conditionalCmd);
    emit logMessage(QString("Wstrzyknięto komendę warunkową (ExitCode: %1): '%2'").arg(isSuccess ? "0 (Sukces)" : "!=0 (Błąd)", cmd), "#2196F3");}

void SequenceRunner::onCommandFinished(int exitCode, QProcess::ExitStatus) {
    if (!m_isRunning) return;
    const SequenceCmd &currentCmd = m_commands.at(m_currentIndex);
    if (!currentCmd.isConditionalExecution) {
        if (exitCode == 0) {
            executeConditionalCommand(currentCmd.successCommand, currentCmd.runMode, true);
        } else {
            executeConditionalCommand(currentCmd.failureCommand, currentCmd.runMode, false);}}
    if (exitCode != 0) {
        if (currentCmd.stopOnError) {
            emit logMessage(QString("Sekwencja zatrzymana: Komenda nie powiodła się (kod %1).").arg(exitCode), "#F44336");
            finishSequence(false);
            return;}}
    m_currentIndex++;
    if (currentCmd.isConditionalExecution) {
        m_commands.removeAt(m_currentIndex - 1);
        m_currentIndex--;
    }
    if (m_currentIndex < m_commands.count()) {
        if (currentCmd.delayAfterMs > 0) {
            emit logMessage(QString("Oczekiwanie %1 ms...").arg(currentCmd.delayAfterMs), "#FFC107");
            m_delayTimer.setInterval(currentCmd.delayAfterMs);
            m_delayTimer.start();
        } else {
            executeNextCommand();}
    } else {
        finishSequence(true);}}

void SequenceRunner::finishSequence(bool success) {
    if (!m_isRunning) return;
    m_isRunning = false;
    m_delayTimer.stop();
    m_executor->cancelCurrentCommand(); 
    emit sequenceFinished(success);
    if (m_isInterval) {
        if (success) {
            emit logMessage(QString("Sekwencja zakończona sukcesem. Restart za %1 sekund.").arg(m_intervalValueS), "#00BCD4");
        } else {
            emit logMessage(QString("Sekwencja zakończona błędem. Restart za %1 sekund.").arg(m_intervalValueS), "#00BCD4");}
        emit scheduleRestart(m_intervalValueS);
    } else {
        if (success) {
            emit logMessage("--- SEKWENCJA ZAKOŃCZONA SUKCESEM ---", "#4CAF50");
        } else {
            emit logMessage("--- SEKWENCJA ZAKOŃCZONA BŁĘDEM ---", "#F44336");}}}

void SequenceRunner::setIntervalToggle(bool toggle) {
    m_isInterval = toggle;
    emit logMessage(QString("Interwał sekwencji: %1").arg(toggle ? "WŁĄCZONY" : "WYŁĄCZONY"), "#BDBDBD");}

void SequenceRunner::setIntervalValue(int seconds) {
    m_intervalValueS = seconds;
    emit logMessage(QString("Wartość interwału ustawiona na %1 s.").arg(seconds), "#BDBDBD");}
