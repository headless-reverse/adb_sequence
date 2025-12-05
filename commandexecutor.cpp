#include "commandexecutor.h"
#include <QByteArray>

CommandExecutor::CommandExecutor(QObject *parent) : QObject(parent) {}

CommandExecutor::~CommandExecutor() {stop();}

void CommandExecutor::setAdbPath(const QString &path) {
    if (!path.isEmpty()) m_adbPath = path;}

void CommandExecutor::runAdbCommand(const QStringList &args) {
    stop();
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &CommandExecutor::readStdOut);
    connect(m_process, &QProcess::readyReadStandardError, this, &CommandExecutor::readStdErr);
    connect(m_process, &QProcess::started, this, &CommandExecutor::onStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CommandExecutor::onFinished);
    m_process->start(m_adbPath, args);}

void CommandExecutor::stop() {
    if (!m_process) return;
    if (m_process->state() == QProcess::Running) {
        m_process->kill();
        m_process->waitForFinished(1000);}
    m_process->deleteLater();
    m_process = nullptr;}

void CommandExecutor::readStdOut() {
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardOutput();
    if (!data.isEmpty()) emit outputReceived(QString::fromUtf8(data));}

void CommandExecutor::readStdErr() {
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty()) emit errorReceived(QString::fromUtf8(data));}

void CommandExecutor::onStarted() {
    emit started();}

void CommandExecutor::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    emit finished(exitCode, exitStatus);}
