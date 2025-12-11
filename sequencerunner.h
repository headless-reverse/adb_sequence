#pragma once

#include <QProcess>
#include <QObject>
#include <QList>
#include <QTimer>
#include <QJsonObject>

class CommandExecutor;

struct SequenceCmd {
    QString command;
    int delayAfterMs = 0;
    QString runMode = "adb"; // "adb", "shell", "root"
    bool stopOnError = true;};

class SequenceRunner : public QObject {
    Q_OBJECT
public:
    explicit SequenceRunner(CommandExecutor *executor, QObject *parent = nullptr);
    bool appendSequence(const QString &filePath);
    void clearSequence();
    void startSequence();
    void stopSequence();
    void setIntervalToggle(bool toggle);
    void setIntervalValue(int seconds);
    bool isRunning() const { return m_isRunning; }
    QStringList getCommandsAsText() const;
    int commandCount() const { return m_commands.count(); }

signals:
    void sequenceStarted();
    void sequenceFinished(bool success);
    void scheduleRestart(int intervalSeconds);
    void commandExecuting(const QString &cmd, int index, int total);
    void logMessage(const QString &text, const QString &color);

private slots:
    void onCommandFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDelayTimeout();

private:
    CommandExecutor *m_executor;
    QList<SequenceCmd> m_commands;
    QTimer m_delayTimer;
    int m_currentIndex = 0;
    bool m_isRunning = false;
    bool m_isInterval = false;
    int m_intervalValueS = 60;
    void finishSequence(bool success);
    void executeNextCommand();
    SequenceCmd parseCommandFromJson(const QJsonObject &obj);
};
