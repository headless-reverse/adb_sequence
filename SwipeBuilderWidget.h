#pragma once
#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QListWidget>
#include "SwipeModel.h"
#include "SwipeCanvas.h"
#include "KeyboardWidget.h"
#include "commandexecutor.h"

class SwipeBuilderWidget : public QWidget {
    Q_OBJECT
public:
    explicit SwipeBuilderWidget(CommandExecutor *executor, QWidget *parent = nullptr);
    ~SwipeBuilderWidget();

    void setAdbPath(const QString &path);
    void startMonitoring();
    void stopMonitoring();
    void captureScreenshot();
    void setCanvasStatus(const QString &message, bool isError);

signals:
    void adbStatus(const QString &message, bool isError);
    void sequenceGenerated(const QString &filePath);

private slots:
    void onScreenshotReady(int exitCode, QProcess::ExitStatus exitStatus);
    void onKeyboardCommandGenerated(const QString &command);
    void onKeyboardToggleClicked();
    void updateList();
    void saveJson();
    void clearActions();
    void deleteSelected();
    void runSelectedAction();
    void onAdbCommandFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    SwipeModel *m_model = nullptr;
    SwipeCanvas *m_canvas = nullptr;
    KeyboardWidget *m_keyboardWidget = nullptr;
    QListWidget *m_list = nullptr;
    int m_lastMonitoringStatus = -1;
    QProcess *m_adbProcess = nullptr; 
    QString m_adbPath = QStringLiteral("adb");
    QTimer m_refreshTimer;
    QProcess::ProcessError m_lastError;
    CommandExecutor *m_executor = nullptr;
    QPushButton *m_runButton = nullptr; 
    QString getAdbCommandForAction(const SwipeAction &a, QStringList &args, QString &runMode) const;
};
