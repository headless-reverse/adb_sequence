#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QSettings>
#include <QProcess>
#include "remoteserver.h" 
#include "sequencerunner.h" 
#include "commandexecutor.h" 
#include "argsparser.h" 


#define CONFIG_PATH "adb_sequence.conf"
#define DEFAULT_PORT 12345

struct AppConfig {
    QString adbPath = "adb";
    QString targetSerial;
    quint16 serverPort = DEFAULT_PORT;
    QString sequencePath;
    bool isServerMode = false;
    bool isHeadlessRun = false;};

AppConfig loadConfiguration(const QCommandLineParser &parser) {
    AppConfig config;
    QSettings settings(CONFIG_PATH, QSettings::IniFormat);
    if (QFile::exists(CONFIG_PATH)) {
        settings.beginGroup("Settings");
        config.adbPath = settings.value("adbPath", config.adbPath).toString();
        config.targetSerial = settings.value("targetSerial", config.targetSerial).toString();
        config.serverPort = settings.value("serverPort", DEFAULT_PORT).toUInt();
        settings.endGroup();
        qDebug() << "Wczytano konfiguracje z pliku:" << CONFIG_PATH;
    } else {
        qDebug() << "Nie znaleziono pliku konfiguracyjnego, uzycie wartosci domyslnych.";}
    if (parser.isSet("adb-path")) {
        config.adbPath = parser.value("adb-path");}
    if (parser.isSet("device-serial")) {
        config.targetSerial = parser.value("device-serial");}
    if (parser.isSet("port")) {
        config.serverPort = parser.value("port").toUShort();}
    config.isServerMode = parser.isSet("server");
    if (parser.isSet("sequence")) {
        config.isHeadlessRun = true;
        config.sequencePath = parser.value("sequence");}
    return config;}

int runHeadless(int argc, char *argv[], const AppConfig &config) {
    QCoreApplication a(argc, argv);
    CommandExecutor executor(nullptr);
    executor.setAdbPath(config.adbPath);
    executor.setTargetDevice(config.targetSerial);
    SequenceRunner runner(&executor, nullptr);
    QObject::connect(&runner, &SequenceRunner::sequenceFinished, &a, &QCoreApplication::quit);
    QObject::connect(&runner, &SequenceRunner::logMessage, [](const QString &text, const QString &color) {
        Q_UNUSED(color);
        qDebug() << "RUNNER LOG:" << text;
    });
    if (!runner.appendSequence(config.sequencePath)) {
        qCritical() << "BLAD: Nie udalo sie zaladowac sekwencji z:" << config.sequencePath;
        return 1;}
    qDebug() << "Sekwencja zaladowana. Rozpoczynanie wykonania...";
    runner.startSequence();
    return a.exec(); }

int runServer(int argc, char *argv[], const AppConfig &config) {
    QCoreApplication a(argc, argv); 
    qDebug() << "Uruchamianie serwera WebSocket...";
    RemoteServer server(config.adbPath, config.targetSerial, config.serverPort);
    return a.exec(); }

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName("adb_sequence_d"); 
    QCoreApplication::setApplicationVersion("4.0");
    QCoreApplication::setOrganizationName("AdbShell");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption sequenceFileOption(QStringList() << "s" << "sequence",
        "Ścieżka do pliku JSON sekwencji do uruchomienia w trybie CLI.", "path");
    parser.addOption(sequenceFileOption);
    QCommandLineOption serverOption(QStringList() << "server",
        "Uruchamia aplikację jako serwer WebSocket dla zdalnego sterowania.");
    parser.addOption(serverOption);
    QCommandLineOption adbPathOption(QStringList() << "a" << "adb-path",
        "Ścieżka do binarki ADB (nadpisuje conf).", "path");
    parser.addOption(adbPathOption);
    QCommandLineOption serialOption(QStringList() << "d" << "device-serial",
        "Numer seryjny urządzenia docelowego ADB (nadpisuje conf).", "serial");
    parser.addOption(serialOption);
    QCommandLineOption portOption(QStringList() << "p" << "port",
        QString("Port dla serwera WebSocket (domyślnie %1, nadpisuje conf).").arg(DEFAULT_PORT), "port", QString::number(DEFAULT_PORT));
    parser.addOption(portOption);
    QCoreApplication tempApp(argc, argv);
    parser.process(tempApp);
    AppConfig config = loadConfiguration(parser);
    if (config.isServerMode) {
        return runServer(argc, argv, config);
    } else if (config.isHeadlessRun) {
        return runHeadless(argc, argv, config);
    } else {
        qDebug() << "Brak trybu (serwer lub sekwencja) - nic nie robie. Użyj --help.";
        return 0;}}
