#include "mainwindow.h"
#include "argsparser.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("adb_sequence"); 
    a.setApplicationVersion("6.0");
    a.setOrganizationName("AdbShell");
    ArgsParser::parse(a.arguments());
    QString adbPath = ArgsParser::get("adb-path");
    QString targetSerial = ArgsParser::get("device-serial");
    MainWindow w(nullptr, adbPath, targetSerial);
    w.show();
    return a.exec();}
