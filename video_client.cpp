#include "video_client.h"
#include "h264decoder.h"
#include "video_worker.h"
#include "control_socket.h"
#include "swipecanvas.h"

#include <iostream>
#include <QDebug>
#include <QtEndian>
#include <QProcess>
#include <QTimer>
#include <QCoreApplication>
#include <QFile>
#include <QMetaObject>
#include <QThread>

VideoClient::VideoClient(QObject *parent)
    : QObject(parent),
      m_isStreaming(false),
      m_localPort(7373),
      m_devicePort(7373),
      m_worker(new VideoWorker(nullptr, nullptr)),
      m_controlSocket(new ControlSocket(this)) {
    
    m_adbPath = "adb";
    m_worker->moveToThread(&m_workerThread);

    // Połączenie sygnału nowej ramki (Zero-Copy)
    connect(m_worker, &VideoWorker::frameReady, this, &VideoClient::onFrameReady);
    
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &VideoClient::startWorker, m_worker, &VideoWorker::startStream);
    connect(this, &VideoClient::stopWorker, m_worker, &VideoWorker::stopStream);
    connect(m_worker, &VideoWorker::statusUpdate, this, &VideoClient::statusUpdate);
    connect(m_worker, &VideoWorker::finished, this, &VideoClient::onWorkerFinished);
    
    m_workerThread.start();
}

VideoClient::~VideoClient() {
    stopStream();
    m_workerThread.quit();
    m_workerThread.wait();
}

void VideoClient::startStream(const QString &deviceSerial, int localPort, int devicePort) {
    if (m_isStreaming) return;
    m_deviceSerial = deviceSerial;
    m_localPort = localPort;
    m_devicePort = devicePort;
    m_isStreaming = true;
    deployAndStartAgent();
}

void VideoClient::stopStream() {
    if (!m_isStreaming) return;
    emit stopWorker();
    
    if (m_controlSocket) {
        m_controlSocket->disconnectFromAgent();
    }

    if (m_agentProcess) {
        m_agentProcess->kill();
        m_agentProcess->waitForFinished(500);
        m_agentProcess->deleteLater();
        m_agentProcess = nullptr;
    }
    
    if (!m_deviceSerial.isEmpty()) {
        executeAdbCommand(QStringList() << "-s" << m_deviceSerial << "forward" << "--remove" << QString("tcp:%1").arg(m_localPort), false);
    }
    
    m_isStreaming = false;
    emit statusUpdate("Strumień zatrzymany.", false);
}

void VideoClient::deployAndStartAgent() {
    // Ścieżka do Twojego pliku .jar serwera
    QString jarPath = "/opt/build/adb_sequence/adb_sequence_pro/android/sequence.jar";
    QString deviceJarPath = "/data/local/tmp/sequence.jar";

    if (!QFile::exists(jarPath)) {
        qDebug() << "BŁĄD: Plik serwera nie istnieje pod ścieżką:" << jarPath;
        emit statusUpdate("Błąd: brak pliku sequence.jar na hostingu!", true);
        return;
    }

    if (!executeAdbCommand(QStringList() << "-s" << m_deviceSerial << "push" << jarPath << deviceJarPath)) return;
    executeAdbCommand(QStringList() << "-s" << m_deviceSerial << "forward" << QString("tcp:%1").arg(m_localPort) << QString("tcp:%1").arg(m_devicePort));

    if (m_agentProcess) {
        m_agentProcess->disconnect();
        m_agentProcess->kill();
        m_agentProcess->waitForFinished(1000);
        delete m_agentProcess;
        m_agentProcess = nullptr;
    }

    m_agentProcess = new QProcess(this);
    QString shellCmd = QString("CLASSPATH=%1 app_process / dev.headless.sequence.Server").arg(deviceJarPath);
    QStringList args;
    args << "-s" << m_deviceSerial << "shell" << shellCmd;

    connect(m_agentProcess, &QProcess::readyReadStandardError, [this](){
        std::cerr << "SERVER ERROR: " << m_agentProcess->readAllStandardError().toStdString() << std::endl;
    });

    QString program = m_adbPath.isEmpty() ? "adb" : m_adbPath;
    m_agentProcess->start(program, args);
    
    emit statusUpdate("Serwer Android uruchomiony. Łączenie...", false);

    QTimer::singleShot(1200, this, [this](){
        emit startWorker(m_deviceSerial, m_localPort, m_devicePort, m_adbPath);
        if (m_controlSocket) {
            m_controlSocket->connectToLocalhost(m_localPort);
        }
    });
}

/**
 * Zaktualizowany slot obsługujący ramkę AVFramePtr
 */
void VideoClient::onFrameReady(AVFramePtr frame) {
    if (m_swipeCanvas) {
        // Przekazanie wskaźnika bezpośrednio do widgetu OpenGL
        m_swipeCanvas->onFrameReady(frame);
    } else {
        static int nullCounter = 0;
        if (nullCounter++ % 100 == 0) {
            qDebug() << "[VideoClient] Ostrzeżenie: m_swipeCanvas jest NULL, klatki są ignorowane.";
        }
    }
    emit frameUpdated(frame);
}

void VideoClient::onWorkerFinished() {
    m_isStreaming = false;
    emit finished();
}

bool VideoClient::executeAdbCommand(const QStringList &args, bool wait) {
    QProcess adb;
    QString program = m_adbPath.isEmpty() ? "adb" : m_adbPath;
    adb.start(program, args);
    if (!wait) return adb.waitForStarted();
    if (!adb.waitForFinished(5000)) return false;
    return (adb.exitStatus() != QProcess::CrashExit && adb.exitCode() == 0);
}

void VideoClient::setAdbPath(const QString &path) { m_adbPath = path; }
void VideoClient::setDeviceSerial(const QString &serial) { m_deviceSerial = serial; }
void VideoClient::setSwipeCanvas(SwipeCanvas *canvas) { m_swipeCanvas = canvas; }
