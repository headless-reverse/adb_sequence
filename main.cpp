#include "mainwindow.h"
#include "argsparser.h"
#include <QApplication>
#include <QDebug>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    QSurfaceFormat format;
    format.setVersion(3, 3); 
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4);
    
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    
    a.setApplicationName("adb_sequence"); 
    a.setApplicationVersion("4.0");
    a.setOrganizationName("AdbShell");
    
    ArgsParser::parse(a.arguments());
    
    QString adbPath = ArgsParser::get("adb-path");
    QString targetSerial = ArgsParser::get("device-serial");
    if (adbPath.isEmpty()) {
        qDebug() << "Warning: No adb-path provided, using default 'adb'";
    }
    
    MainWindow w(nullptr, adbPath, targetSerial);
    w.show();
    
    return a.exec();
}
