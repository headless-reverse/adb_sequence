#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include "adb_client.h" 

class HardwareGrabbed;

class CommandExecutor : public QObject {
    Q_OBJECT
public:
    explicit CommandExecutor(QObject *parent = nullptr);
    ~CommandExecutor();

	void setHardwareInterface(HardwareGrabbed *hw) { m_hwGrab = hw; }
    void setAdbPath(const QString &path);
    void setTargetDevice(const QString &serial);
    void runAdbCommand(const QStringList &args);
    void executeShellCommand(const QString &command);
    void executeRootShellCommand(const QString &command);
    void executeAdbCommand(const QString &command);
    void executeSequenceCommand(const QString &command, const QString &runMode); 
    
    void stop();
    void cancelCurrentCommand();    
    QString adbPath() const { return m_adbPath; }
    QString targetDevice() const { return m_targetSerial; }
    bool isRunning() const;

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void outputReceived(const QString &output);
    void errorReceived(const QString &error);
    void adbStatusChanged(const QString &message, bool isError); 
    void rawDataReady(const QByteArray &data);

private slots:
    void readStdOut();
    void readStdErr();
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onShellProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void onAdbClientError(const QString &message);
    void onAdbClientRawDataReady(const QByteArray &data);
    void onAdbClientCommandResponseReady(const QByteArray &response);

private:
    void ensureShellRunning();
    QString m_adbPath;
    QString m_targetSerial;
    QProcess *m_process = nullptr;
    QProcess *m_shellProcess = nullptr;
    
    AdbClient *m_adbClient = nullptr;
    HardwareGrabbed *m_hwGrab = nullptr;
};

#endif // COMMANDEXECUTOR_H
