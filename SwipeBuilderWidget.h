#pragma once
#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QListWidget>
#include "SwipeModel.h"
#include "swipecanvas.h"
#include "KeyboardWidget.h"
#include "commandexecutor.h"
#include <QJsonArray>
#include <QPushButton>
#include <QCheckBox>

class CommandExecutor;
class VideoClient;

class SwipeBuilderWidget : public QWidget {
    Q_OBJECT
public:
    explicit SwipeBuilderWidget(CommandExecutor *executor, VideoClient *videoClient, QWidget *parent = nullptr);
    ~SwipeBuilderWidget() override;

    SwipeCanvas* canvas() const { return m_canvas; }
    void setAdbPath(const QString &path);
    void startMonitoring();
    void stopMonitoring();
    void setCanvasStatus(const QString &message, bool isError);
    void setRunSequenceButtonEnabled(bool enabled);
    void setVerboseScreenshots(bool v) { m_verboseScreenshots = v; }
    bool verboseScreenshots() const { return m_verboseScreenshots; }
    SwipeModel *model() const { return m_model; }
    void loadSequence(const QString &filePath);
    void loadJson();
    void runFullSequence();
    
signals:
    void adbStatus(const QString &message, bool isError);
    void sequenceGenerated(const QString &filePath);
    void runFullSequenceRequested();

public slots:
	void onSequenceCommandExecuting(const QString &cmd, int index, int total);

private slots:
    void onKeyboardCommandGenerated(const QString &command);
    void onKeyboardToggleClicked();
    void updateList();
    void saveJson();
    void clearActions();
    void deleteSelected();
    void runSelectedAction();
    void editSelected();
    void moveSelectedUp();
    void moveSelectedDown();
    void onAdbCommandFinished(int exitCode, QProcess::ExitStatus);
    void addActionFromDialog();
    void onRawToggleChanged(int state);
    void onResolutionReady(int exitCode, QProcess::ExitStatus);
    void fetchDeviceResolution();
    void handleCanvasScreenshotReady(const QImage &image);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    SwipeModel      *m_model = nullptr;
    SwipeCanvas     *m_canvas = nullptr;
    KeyboardWidget  *m_keyboardWidget = nullptr;
    QListWidget     *m_list = nullptr;

    QProcess        *m_resolutionProcess = nullptr;
    QProcess::ProcessError m_lastError = QProcess::UnknownError;
    QProcess *m_resProcess = nullptr;

    CommandExecutor *m_executor = nullptr;
    VideoClient     *m_videoClient = nullptr;

    QPushButton     *m_runButton = nullptr;
    QPushButton     *m_runSequenceButton = nullptr;
    QCheckBox       *m_useRawCheckbox = nullptr;

    QByteArray       m_rawBuffer;
    QImage           m_rawImage;
    bool             m_useRaw = true;
    int              m_consecutiveRawErrors = 0;
    bool             m_verboseScreenshots = false;
    QString          m_adbPath = QStringLiteral("adb");
    int              m_deviceWidth = 0;
    int              m_deviceHeight = 0;
    int              m_lastMonitoringStatus = -1;

    bool loadSequenceFromJsonArray(const QJsonArray &array);
    QString getAdbCommandForAction(const SwipeAction &action,
                                   bool forceRoot = false) const;
};
