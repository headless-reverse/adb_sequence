#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

class CommandExecutor : public QObject {
    Q_OBJECT
public:
    explicit CommandExecutor(QObject *parent = nullptr);
    ~CommandExecutor();

    void runAdbCommand(const QStringList &args);
    void stop();

    void setAdbPath(const QString &path);

signals:
    void outputReceived(const QString &text);
    void errorReceived(const QString &text);
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void readStdOut();
    void readStdErr();
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *m_process = nullptr;
    QString m_adbPath = QStringLiteral("adb");
};
