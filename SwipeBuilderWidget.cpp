#include "SwipeBuilderWidget.h"
#include "SwipeModel.h"
#include "SwipeCanvas.h"
#include "KeyboardWidget.h"
#include "argsparser.h"
#include "commandexecutor.h"
#include "ActionEditDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QJsonDocument>
#include <QDebug>
#include <QLabel>
#include <QTimer>
#include <QListWidget>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QListWidgetItem>
#include <algorithm>
#include <QImage> 

SwipeBuilderWidget::SwipeBuilderWidget(CommandExecutor *executor, QWidget *parent)
    : QWidget(parent), m_executor(executor) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    m_model = new SwipeModel(this);
    m_canvas = new SwipeCanvas(m_model, this);
    m_canvas->setSizePolicy(QSizePolicy::Expanding,
                            QSizePolicy::Expanding);
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
    QWidget *controlsWidget = new QWidget();
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *listHeaderLayout = new QHBoxLayout();
    QLabel *recordedActionsLabel = new QLabel(tr("Recorded Actions")); 
    listHeaderLayout->addWidget(recordedActionsLabel);
    m_useRawCheckbox = new QCheckBox(tr("use RAW"), this); 
    m_useRawCheckbox->setChecked(m_useRaw); 
    connect(m_useRawCheckbox, &QCheckBox::checkStateChanged, 
            this, &SwipeBuilderWidget::onRawToggleChanged); 
    listHeaderLayout->addWidget(m_useRawCheckbox);
    listHeaderLayout->addStretch(1); 
    controlsLayout->addLayout(listHeaderLayout);
    m_list = new QListWidget();
    setAcceptDrops(true);
    controlsLayout->addWidget(m_list);
    QPushButton *b_toggle_keyboard = new QPushButton("Toggle Keyboard");
    b_toggle_keyboard->setFlat(true);
    b_toggle_keyboard->setStyleSheet("color: #FFAA00;");
    controlsLayout->addWidget(b_toggle_keyboard);
    QHBoxLayout *runLayout = new QHBoxLayout();
    m_runButton = new QPushButton("▶ Action");
    m_runButton->setStyleSheet(
        "background-color: #4CAF50; color: white;");
    m_runSequenceButton = new QPushButton("▶ Sequence");
    m_runSequenceButton->setStyleSheet(
        "background-color: #00BCD4; color: white;");
    runLayout->addWidget(m_runButton);
    runLayout->addWidget(m_runSequenceButton);
    controlsLayout->addLayout(runLayout);
    QHBoxLayout *editAddLayout = new QHBoxLayout();
    QPushButton *b_edit = new QPushButton("Edit");
    QPushButton *b_add_cmd = new QPushButton("Add");
    editAddLayout->addWidget(b_edit);
    editAddLayout->addWidget(b_add_cmd);
    controlsLayout->addLayout(editAddLayout);
    QHBoxLayout *deleteLayout = new QHBoxLayout();
    QPushButton *b_del = new QPushButton("Delete");
    QPushButton *b_clear = new QPushButton("Clear All");
    deleteLayout->addWidget(b_del);
    deleteLayout->addWidget(b_clear);
    controlsLayout->addLayout(deleteLayout);
    QHBoxLayout *fileLayout = new QHBoxLayout();
    QPushButton *b_export = new QPushButton("Save");
    QPushButton *b_import = new QPushButton("Load");
    fileLayout->addWidget(b_export);
    fileLayout->addWidget(b_import);
    controlsLayout->addLayout(fileLayout);
    rightSplitter->addWidget(controlsWidget);
    m_keyboardWidget = new KeyboardWidget(this);
    m_keyboardWidget->hide();
    rightSplitter->addWidget(m_keyboardWidget);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 0);
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(m_canvas);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(mainSplitter);
    /* ADB processes */
    m_adbProcess = new QProcess(this);
    connect(m_adbProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                m_lastError = error;
                if (error == QProcess::FailedToStart) {
                    emit adbStatus(
                        "ADB command not found! Check if path is set correctly.",
                        true);
                } else {
                    emit adbStatus(
                        QString("ADB process error: %1").arg(error), true);}});
    connect(m_adbProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SwipeBuilderWidget::onScreenshotReady);
    m_resolutionProcess = new QProcess(this);
    connect(m_resolutionProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SwipeBuilderWidget::onResolutionReady);
    m_refreshTimer.setInterval(33);
    connect(&m_refreshTimer, &QTimer::timeout, this,
            &SwipeBuilderWidget::captureScreenshot);
    connect(m_model, &SwipeModel::modelChanged, this,
            &SwipeBuilderWidget::updateList);
    connect(b_export, &QPushButton::clicked, this,
            &SwipeBuilderWidget::saveJson);
    connect(b_import, &QPushButton::clicked, this,
            &SwipeBuilderWidget::loadJson);
    connect(b_clear, &QPushButton::clicked, this,
            &SwipeBuilderWidget::clearActions);
    connect(b_del, &QPushButton::clicked, this,
            &SwipeBuilderWidget::deleteSelected);
    connect(b_edit, &QPushButton::clicked, this,
            &SwipeBuilderWidget::editSelected);
    connect(b_add_cmd, &QPushButton::clicked, this,
            &SwipeBuilderWidget::addActionFromDialog);
    connect(m_runButton, &QPushButton::clicked, this,
            &SwipeBuilderWidget::runSelectedAction);
    connect(m_runSequenceButton, &QPushButton::clicked, this,
            &SwipeBuilderWidget::runFullSequence);
    connect(b_toggle_keyboard, &QPushButton::clicked, this,
            &SwipeBuilderWidget::onKeyboardToggleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) {
                QMenu menu;
                menu.addAction("Edit", this,
                               &SwipeBuilderWidget::editSelected);
                menu.addAction("Run", this,
                               &SwipeBuilderWidget::runSelectedAction);
                menu.addSeparator();
                menu.addAction("Move Up", this,
                               &SwipeBuilderWidget::moveSelectedUp);
                menu.addAction("Move Down", this,
                               &SwipeBuilderWidget::moveSelectedDown);
                menu.addSeparator();
                menu.addAction("Delete", this,
                               &SwipeBuilderWidget::deleteSelected);
                menu.exec(m_list->mapToGlobal(pos));
            });
    connect(m_keyboardWidget, &KeyboardWidget::adbCommandGenerated, this,
            &SwipeBuilderWidget::onKeyboardCommandGenerated);
    connect(m_executor,
            QOverload<int, QProcess::ExitStatus>::of(&CommandExecutor::finished),
            this, &SwipeBuilderWidget::onAdbCommandFinished);
    QTimer::singleShot(500, this,
                       &SwipeBuilderWidget::fetchDeviceResolution);}

SwipeBuilderWidget::~SwipeBuilderWidget() {
    stopMonitoring();
    if (m_adbProcess) {
        m_adbProcess->kill();
        m_adbProcess->waitForFinished(500);}}

void SwipeBuilderWidget::setCanvasStatus(const QString &message, bool isError) {m_canvas->setStatus(message, isError);}

void SwipeBuilderWidget::setAdbPath(const QString &path) {if (!path.isEmpty()) m_adbPath = path;}

void SwipeBuilderWidget::startMonitoring() {
    if (!m_refreshTimer.isActive() &&
        m_adbProcess->state() == QProcess::NotRunning) {
        m_refreshTimer.start();}}

void SwipeBuilderWidget::stopMonitoring() {m_refreshTimer.stop();}

void SwipeBuilderWidget::setRunSequenceButtonEnabled(bool enabled) {
    if (m_runSequenceButton) m_runSequenceButton->setEnabled(enabled);}

void SwipeBuilderWidget::captureScreenshot() {
    if (m_adbProcess->state() != QProcess::NotRunning) return;
    if (m_deviceWidth == 0 || m_deviceHeight == 0) return;
    QStringList args;
    QString targetSerial = m_executor->targetDevice();
    if (!targetSerial.isEmpty()) args << "-s" << targetSerial;
    args << "exec-out" << "screencap";
    if (!m_useRaw) {
        args << "-p";}    
    m_lastError = QProcess::UnknownError;
    m_adbProcess->setProgram(m_adbPath);
    m_adbProcess->setArguments(args);
    m_adbProcess->start();}

void SwipeBuilderWidget::onRawToggleChanged(int state) {
    m_useRaw = (state == Qt::Checked);
    m_canvas->setCaptureMode(m_useRaw); 
    if (m_useRaw) {
        setCanvasStatus(tr("Screen capture mode switched to RAW (Fast, but without rotation fix)."), false);
    } else {
        setCanvasStatus(tr("Screen capture mode switched to PNG (Stable, with rotation fix, slower)."), false);}
    captureScreenshot();}

void SwipeBuilderWidget::onScreenshotReady(int exitCode, QProcess::ExitStatus) {
    if (m_lastError == QProcess::FailedToStart) {
        m_refreshTimer.start();
        return;}
    QByteArray data = m_adbProcess->readAllStandardOutput();

    if (exitCode != 0) {
        QByteArray err = m_adbProcess->readAllStandardError();
        if (m_lastMonitoringStatus != exitCode) {
            QString mode = m_useRaw ? "RAW" : "PNG";
            emit adbStatus(
                QString("%1 screenshot failed. Exit code %2. Error: %3")
                    .arg(mode).arg(exitCode).arg(QString::fromUtf8(err).trimmed()),
                true);
            m_lastMonitoringStatus = exitCode;}
        Q_UNUSED(err);
        return;}
    if (!m_useRaw) {
        m_consecutiveRawErrors = 0;
        m_canvas->loadFromData(data);
        if (m_verboseScreenshots) {
            emit adbStatus("Screenshot OK (PNG).", false);}
        m_lastMonitoringStatus = 0;
        return;}
    const int expected = m_deviceWidth * m_deviceHeight * 4;    // RGBA8888
    if (data.size() < expected) {
        ++m_consecutiveRawErrors;
        if (m_consecutiveRawErrors > 3 && m_useRaw) {
            m_useRaw = false; 
            if (m_useRawCheckbox) { 
                m_useRawCheckbox->setChecked(false);}
            m_consecutiveRawErrors = 0;
            setCanvasStatus("RAW capture repeatedly failed – switching to PNG mode.", true);
            m_canvas->setCaptureMode(false); 
            captureScreenshot(); 
            return; }
        if (m_lastMonitoringStatus != 1) {
            emit adbStatus("RAW frame too small – skipping.", true);
            m_lastMonitoringStatus = 1;}
        return;}
    int headerSize = data.size() - expected;
    if (headerSize < 0) headerSize = 0;

    if (m_rawBuffer.size() != expected) {
        m_rawBuffer.resize(expected);
        m_rawImage = QImage(reinterpret_cast<const uchar *>(m_rawBuffer.constData()),
                            m_deviceWidth, m_deviceHeight,
                            QImage::Format_RGBA8888);}
    memcpy(m_rawBuffer.data(), data.constData() + headerSize, expected);
    m_consecutiveRawErrors = 0;
    m_canvas->loadFromData(m_rawBuffer);
    if (m_verboseScreenshots) {
        emit adbStatus("Screenshot OK (RAW).", false);}
    m_lastMonitoringStatus = 0;}


void SwipeBuilderWidget::onKeyboardCommandGenerated(const QString &command) {
    if (command.startsWith("input keyevent")) {
        QString keyName = command.mid(15).trimmed();
        m_model->addKey(keyName, 100);
    } else {
        m_model->addCommand(command, 100, "shell");}
    QString logCmd = command.simplified();
    if (logCmd.length() > 50) logCmd = logCmd.left(50) + "...";
    emit adbStatus(QString("KEYBOARD: Added: %1").arg(logCmd), false);}


void SwipeBuilderWidget::updateList() {
    m_list->clear();
    int idx = 1;
    for (const auto &a : m_model->actions()) {
        QString txt;
        QString delayStr = QString("[D:%1ms]").arg(a.delayAfterMs);
        if (a.type == SwipeAction::Tap) {
            txt = QString("%1. Tap (%2,%3) %4")
                      .arg(idx).arg(a.x1).arg(a.y1).arg(delayStr);
        } else if (a.type == SwipeAction::Swipe) {
            QString durStr = QString("[T:%1ms]").arg(a.duration);
            txt = QString("%1. Swipe (%2,%3)->(%4,%5) %6 %7")
                      .arg(idx).arg(a.x1).arg(a.y1)
                      .arg(a.x2).arg(a.y2).arg(durStr).arg(delayStr);
        } else if (a.type == SwipeAction::Command) {
            QString cmd = a.command.simplified();
            if (cmd.length() > 40) cmd = cmd.left(40) + "...";
            txt = QString("%1. CMD [%2]: %3 %4")
                      .arg(idx).arg(a.runMode.toUpper())
                      .arg(cmd).arg(delayStr);
        } else if (a.type == SwipeAction::Key) {
            QString cmd = a.command.simplified();
            txt = QString("%1. KEY [%2]: %3 %4")
                      .arg(idx).arg(a.runMode.toUpper())
                      .arg(cmd).arg(delayStr);}
        m_list->addItem(txt);
        ++idx;}
    m_list->scrollToBottom();}


void SwipeBuilderWidget::runSelectedAction() {
    int row = m_list->currentRow();
    if (row < 0 || row >= m_model->actions().count()) {
        emit adbStatus("No action selected.", false);
        return;}
    const SwipeAction &a = m_model->actions().at(row);
    QString fullCommand = getAdbCommandForAction(a, true);
    QString runMode = (a.type == SwipeAction::Command) ? a.runMode : "root";
    if (fullCommand.isEmpty()) {
        emit adbStatus("Empty action or command.", true);
        return;
    }
    if (m_executor->isRunning()) {
        emit adbStatus("ADB is busy running another command...", true);
        return;
    }
    QStringList args;
    QStringList parsed = ArgsParser::parse(fullCommand);

    if (runMode.compare("adb", Qt::CaseInsensitive) == 0) {
        args = parsed;
    } else if (runMode.compare("root", Qt::CaseInsensitive) == 0) {
        args << "shell" << "su" << "-c" << fullCommand;
    } else { // shell
        args << "shell" << parsed;
    }

    QString logCmd = args.join(' ').simplified();
    if (logCmd.length() > 80) logCmd = logCmd.left(80) + "...";

    emit adbStatus(QString("EXEC (%1): %2")
                       .arg(runMode.toUpper())
                       .arg(logCmd),
                     false);
    m_executor->runAdbCommand(args);
    QListWidgetItem *item = m_list->item(row);
    if (item) item->setBackground(QBrush(QColor("#FFC107")));}


void SwipeBuilderWidget::onAdbCommandFinished(int exitCode, QProcess::ExitStatus) {
    int finishedRow = -1;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->background().color() == QColor("#FFC107")) {
            finishedRow = i;
            item->setBackground(Qt::NoBrush);
            break;
        }
    }

    if (exitCode == 0) {
        if (finishedRow != -1) {
            QListWidgetItem *item = m_list->item(finishedRow);
            if (item) item->setBackground(QBrush(QColor("#4CAF50")));
            QTimer::singleShot(200, this, [item]() {
                if (item) item->setBackground(Qt::NoBrush);
            });
        }
        emit adbStatus("OK", false);
    } else {
        if (finishedRow != -1) {
            QListWidgetItem *item = m_list->item(finishedRow);
            if (item) item->setBackground(QBrush(QColor("#F44336")));
            QTimer::singleShot(500, this, [item]() {
                if (item) item->setBackground(Qt::NoBrush);});}
        emit adbStatus(QString("Error: %1").arg(exitCode), true);}}


void SwipeBuilderWidget::saveJson() {
    QString path = QFileDialog::getSaveFileName(this,
                                               tr("Save Sequence"),
                                               QString(),
                                               tr("JSON Files (*.json)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_model->toJsonSequence());
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
        emit sequenceGenerated(path);
    } else {
        emit adbStatus("Failed to save file.", true);
    }
}


bool SwipeBuilderWidget::loadSequenceFromJsonArray(const QJsonArray &array) {
    m_model->clear();
    bool ok = true;

    QRegularExpression tapRx("^input\\s+tap\\s+(\\d+)\\s+(\\d+)(?:\\s+\\d+)?$");
    QRegularExpression swipeRx("^input\\s+swipe\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)(?:\\s+(\\d+))?$");

    for (const QJsonValue &val : array) {
        if (!val.isObject()) {
            ok = false;
            break;
        }
        QJsonObject obj = val.toObject();
        QString cmd = obj.value("command").toString().trimmed();
        int delayMs = obj.value("delayAfterMs").toInt(100);
        QString runMode = obj.value("runMode").toString("shell").toLower();

        QRegularExpressionMatch tapMatch = tapRx.match(cmd);
        QRegularExpressionMatch swipeMatch = swipeRx.match(cmd);

        if (tapMatch.hasMatch()) {
            int x = tapMatch.captured(1).toInt();
            int y = tapMatch.captured(2).toInt();
            m_model->addTap(x, y, delayMs);
            if (m_model->actions().last().runMode.toLower() != runMode) {
                 SwipeAction a = m_model->actions().last();
                 a.runMode = runMode;
                 m_model->editActionAt(m_model->actions().count() - 1, a);
            }
        } else if (swipeMatch.hasMatch()) {
            int x1 = swipeMatch.captured(1).toInt();
            int y1 = swipeMatch.captured(2).toInt();
            int x2 = swipeMatch.captured(3).toInt();
            int y2 = swipeMatch.captured(4).toInt();
            int dur = swipeMatch.captured(5).toInt();
            m_model->addSwipe(x1, y1, x2, y2, dur, delayMs);
            if (m_model->actions().last().runMode.toLower() != runMode) {
                 SwipeAction a = m_model->actions().last();
                 a.runMode = runMode;
                 m_model->editActionAt(m_model->actions().count() - 1, a);
            }
        } else if (cmd.startsWith("input keyevent")) {
            QString key = cmd.mid(15).trimmed();
            if (!key.isEmpty())
                m_model->addKey(key, delayMs);
            else
                m_model->addCommand(cmd, delayMs, runMode);
        } else {
            m_model->addCommand(cmd, delayMs, runMode);
        }
    }
    return ok;
}


QString SwipeBuilderWidget::getAdbCommandForAction(const SwipeAction &action,bool) const {
    switch (action.type) {
    case SwipeAction::Tap:
        return QString("input tap %1 %2")
            .arg(action.x1)
            .arg(action.y1);
    case SwipeAction::Swipe:
        return QString("input swipe %1 %2 %3 %4 %5")
            .arg(action.x1)
            .arg(action.y1)
            .arg(action.x2)
            .arg(action.y2)
            .arg(action.duration);
    case SwipeAction::Command:
        return action.command;
    case SwipeAction::Key:
        return QString("input keyevent %1").arg(action.command);
    default:
        return QString();
    }
}

void SwipeBuilderWidget::clearActions() {m_model->clear();}

void SwipeBuilderWidget::deleteSelected() {
    int row = m_list->currentRow();
    if (row >= 0) m_model->removeActionAt(row);}

void SwipeBuilderWidget::onKeyboardToggleClicked() {
    m_keyboardWidget->setVisible(!m_keyboardWidget->isVisible());
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        btn->setText(m_keyboardWidget->isVisible()
                             ? "Hide Virtual Keyboard"
                             : "Toggle Virtual Keyboard");
    }
}

void SwipeBuilderWidget::editSelected() {
    int row = m_list->currentRow();
    if (row < 0) return;
    SwipeAction cur = m_model->actionAt(row);
    
    ActionEditDialog dlg(cur, this); 
    
    if (dlg.exec() == QDialog::Accepted) {
        m_model->editActionAt(row, cur);}}

void SwipeBuilderWidget::moveSelectedUp() {
    int row = m_list->currentRow();
    if (row > 0) {
        m_model->moveAction(row, row - 1);
        m_list->setCurrentRow(row - 1);}}

void SwipeBuilderWidget::moveSelectedDown() {
    int row = m_list->currentRow();
    if (row >= 0 && row < m_model->actions().count() - 1) {
        m_model->moveAction(row, row + 1);
        m_list->setCurrentRow(row + 1);}}

void SwipeBuilderWidget::addActionFromDialog() {
    SwipeAction tmp(SwipeAction::Command, 0, 0, 0, 0, 0, 100, "", "shell");
    ActionEditDialog dlg(tmp, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_model->addCommand(tmp.command, tmp.delayAfterMs, tmp.runMode);
        emit adbStatus(QString("Added Command: %1")
                              .arg(tmp.command.simplified()),
                      false);}}

void SwipeBuilderWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile() &&
                QFileInfo(url.toLocalFile())
                    .suffix()
                    .compare("json", Qt::CaseInsensitive) == 0) {
                event->acceptProposedAction();
                return;}}}
    event->ignore();}

void SwipeBuilderWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();}

void SwipeBuilderWidget::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile() &&
            QFileInfo(url.toLocalFile())
                .suffix()
                .compare("json", Qt::CaseInsensitive) == 0) {
            loadSequence(url.toLocalFile());
            break;}}
    event->acceptProposedAction();}

void SwipeBuilderWidget::fetchDeviceResolution() {
    if (m_resolutionProcess->state() != QProcess::NotRunning) return;
    emit adbStatus(tr("Fetching device resolution (adb shell wm size)..."),
                     false);
    QStringList args;
    QString targetSerial = m_executor->targetDevice();
    if (!targetSerial.isEmpty())
        args << "-s" << targetSerial;
    args << "shell" << "wm" << "size";

    m_resolutionProcess->setProgram(m_adbPath);
    m_resolutionProcess->setArguments(args);
    m_resolutionProcess->start();}

void SwipeBuilderWidget::onResolutionReady(int exitCode, QProcess::ExitStatus) {
    if (exitCode == 0) {
        QByteArray out = m_resolutionProcess->readAllStandardOutput();
        QRegularExpression rx("Physical size:\\s*(\\d+)x(\\d+)");
        QRegularExpressionMatch match = rx.match(QString::fromUtf8(out));

        if (match.hasMatch()) {
            m_deviceWidth  = match.captured(1).toInt();
            m_deviceHeight = match.captured(2).toInt();

            m_canvas->setDeviceResolution(m_deviceWidth,
                                          m_deviceHeight);
            emit adbStatus(
                QString("Resolution found: %1x%2. Starting RAW capture.")
                    .arg(m_deviceWidth)
                    .arg(m_deviceHeight),
                false);
            startMonitoring();
        } else {
            emit adbStatus(
                tr("Failed to parse device resolution from 'wm size'. "
                   "Is device connected?"),
                true);
            QTimer::singleShot(2000,
                               this,
                               &SwipeBuilderWidget::fetchDeviceResolution);}
    } else {
        emit adbStatus(
            QString("Error fetching resolution. Exit code %1")
                .arg(exitCode),
            true);}}

void SwipeBuilderWidget::loadJson() {
    QString path = QFileDialog::getOpenFileName(this,
                                               tr("Load Sequence"),
                                               QString(),
                                               tr("JSON Files (*.json)"));
    if (!path.isEmpty())
        loadSequence(path);}

void SwipeBuilderWidget::runFullSequence() {
    if (m_model->actions().isEmpty()) {
        emit adbStatus(tr("Sequence is empty. Add actions first."), true);
        return;}
    emit adbStatus(tr("Sending request to run full sequence…"), false);
    emit runFullSequenceRequested();}

void SwipeBuilderWidget::loadSequence(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit adbStatus(
            tr("Failed to open file: %1")
                .arg(QFileInfo(filePath).fileName()),
            true);
        return;}
    QByteArray data = f.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        emit adbStatus(tr("Invalid JSON format (expected array)."), true);
        return;}
    if (loadSequenceFromJsonArray(doc.array())) {
        emit adbStatus(
            tr("Sequence loaded from: %1")
                .arg(QFileInfo(filePath).fileName()),
            false);
    } else {
        emit adbStatus(
            tr("Failed to parse sequence from: %1")
                .arg(QFileInfo(filePath).fileName()),
            true);}}

void SwipeBuilderWidget::onSequenceCommandExecuting(const QString &cmd, int index, int total) {
    Q_UNUSED(cmd);
    Q_UNUSED(total);
    int listIndex = index - 1;
    
    if (listIndex >= 0 && listIndex < m_list->count()) {
        QListWidgetItem *item = m_list->item(listIndex);
        m_list->setCurrentItem(item); 
        m_list->scrollToItem(item);
    }
}

#include "SwipeBuilderWidget.moc"
