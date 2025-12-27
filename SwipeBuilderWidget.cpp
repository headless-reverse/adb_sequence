#include "SwipeBuilderWidget.h"
#include "argsparser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QJsonDocument>
#include <QGroupBox>
#include <QDebug>
#include <QLabel> 
#include <QTimer>
#include <QListWidgetItem>
#include <QSplitter> 

SwipeBuilderWidget::SwipeBuilderWidget(CommandExecutor *executor, QWidget *parent)
    : QWidget(parent), m_executor(executor) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    m_model = new SwipeModel(this);
    m_canvas = new SwipeCanvas(m_model, this);
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
    QWidget *controlsWidget = new QWidget();
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    m_list = new QListWidget();
    controlsLayout->addWidget(new QLabel("Recorded Actions:"));
    controlsLayout->addWidget(m_list);
    QPushButton *b_toggle_keyboard = new QPushButton("Toggle Virtual Keyboard");
    b_toggle_keyboard->setFlat(true);
    b_toggle_keyboard->setStyleSheet("color: #FFAA00;");
    controlsLayout->addWidget(b_toggle_keyboard);
    m_runButton = new QPushButton("▶ Run Selected Action");
    m_runButton->setStyleSheet("background-color: #4CAF50; color: white;");
    controlsLayout->addWidget(m_runButton);
    QPushButton *b_del = new QPushButton("Delete Selected");
    QPushButton *b_clear = new QPushButton("Clear All");
    QPushButton *b_export = new QPushButton("Save JSON");
    controlsLayout->addWidget(b_del);
    controlsLayout->addWidget(b_clear);
    controlsLayout->addWidget(b_export);
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
    // --- LOGIKA ---
    m_adbProcess = new QProcess(this);
    connect(m_adbProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        m_lastError = error;
        if (error == QProcess::FailedToStart) {
            emit adbStatus("ADB command not found! Check if path is set correctly.", true);
        } else {
            emit adbStatus(QString("ADB process error: %1").arg(error), true);}});
    connect(m_adbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SwipeBuilderWidget::onScreenshotReady);
    connect(&m_refreshTimer, &QTimer::timeout, this, &SwipeBuilderWidget::captureScreenshot);
    connect(m_model, &SwipeModel::modelChanged, this, &SwipeBuilderWidget::updateList);
    connect(b_export, &QPushButton::clicked, this, &SwipeBuilderWidget::saveJson);
    connect(b_clear, &QPushButton::clicked, this, &SwipeBuilderWidget::clearActions);
    connect(b_del, &QPushButton::clicked, this, &SwipeBuilderWidget::deleteSelected);
    connect(m_keyboardWidget, &KeyboardWidget::adbCommandGenerated, 
            this, &SwipeBuilderWidget::onKeyboardCommandGenerated);
    connect(m_runButton, &QPushButton::clicked, this, &SwipeBuilderWidget::runSelectedAction);
    connect(b_toggle_keyboard, &QPushButton::clicked, this, &SwipeBuilderWidget::onKeyboardToggleClicked);
    connect(m_executor, QOverload<int, QProcess::ExitStatus>::of(&CommandExecutor::finished), 
            this, &SwipeBuilderWidget::onAdbCommandFinished);}

SwipeBuilderWidget::~SwipeBuilderWidget() {
    stopMonitoring();
    if (m_adbProcess) {
        m_adbProcess->kill();
        m_adbProcess->waitForFinished(500);}}

void SwipeBuilderWidget::setCanvasStatus(const QString &message, bool isError) {
    m_canvas->setStatus(message, isError);}

void SwipeBuilderWidget::setAdbPath(const QString &path) {
    if (!path.isEmpty()) m_adbPath = path;}

void SwipeBuilderWidget::startMonitoring() {
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start(300);}}

void SwipeBuilderWidget::stopMonitoring() {
    m_refreshTimer.stop();}

void SwipeBuilderWidget::captureScreenshot() {
    if (m_adbProcess->state() != QProcess::NotRunning) return;
    m_lastError = QProcess::UnknownError;
    m_adbProcess->start(m_adbPath, QStringList() << "exec-out" << "screencap" << "-p");}

void SwipeBuilderWidget::onScreenshotReady(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_lastError != QProcess::UnknownError && m_lastError != QProcess::Crashed) {
        return;}
    if (exitCode == 0) {
        QByteArray data = m_adbProcess->readAllStandardOutput();
        if (data.startsWith("\r\n")) data.remove(0, 2);
        else if (data.startsWith('\r')) data.remove(0, 1);
        if (!data.isEmpty()) {
            m_canvas->loadFromData(data);
            if (m_lastMonitoringStatus != 0) {
                emit adbStatus("Monitoring: OK", false);}
            m_lastMonitoringStatus = 0;
        } else {
             if (m_lastMonitoringStatus != 1) {
                 emit adbStatus("ADB returned success but sent no image data.", true);
                 m_lastMonitoringStatus = 1; }}
    } else {
        QByteArray errData = m_adbProcess->readAllStandardError();
        QString errorMsg = QString("ADB failed (Code: %1). Error: %2")
                           .arg(exitCode)
                           .arg(QString::fromUtf8(errData).trimmed());
        if (m_lastMonitoringStatus != exitCode) {
            emit adbStatus(errorMsg, true);
            m_lastMonitoringStatus = exitCode;}}}

void SwipeBuilderWidget::onKeyboardCommandGenerated(const QString &command) {
    m_model->addCommand(command, 100); 
    QString logCmd = command.simplified();
    if (logCmd.length() > 50) logCmd = logCmd.left(50) + "...";
    emit adbStatus(QString("KEYBOARD: Added action: adb shell %1").arg(logCmd), false);}

void SwipeBuilderWidget::updateList() {
    m_list->clear();
    int idx = 1;
    for (const auto &a : m_model->actions()) {
        QString text;
        if (a.type == SwipeAction::Tap)
            text = QString("%1. Tap (%2, %3)").arg(idx).arg(a.x1).arg(a.y1);
        else if (a.type == SwipeAction::Swipe)
            text = QString("%1. Swipe %2ms").arg(idx).arg(a.duration);
        else if (a.type == SwipeAction::Command) {
            QString cmdText = a.command.simplified();
            if (cmdText.length() > 40) cmdText = cmdText.left(40) + "...";
            text = QString("%1. CMD: %2 (Delay: %3ms)").arg(idx).arg(cmdText).arg(a.duration);}
        m_list->addItem(text);
        idx++;}
    m_list->scrollToBottom();}

void SwipeBuilderWidget::runSelectedAction() {
    int row = m_list->currentRow();
    if (row < 0 || row >= m_model->actions().count()) {
        emit adbStatus("No action selected to run.", false);
        return;}
    const SwipeAction &a = m_model->actions().at(row);
    QStringList args;
    QString runMode;
    if (a.type == SwipeAction::Tap) {
        args << "shell" << "input" << "tap" << QString::number(a.x1) << QString::number(a.y1);
        runMode = "shell";
    } else if (a.type == SwipeAction::Swipe) {
        args << "shell" << "input" << "swipe" << QString::number(a.x1) << QString::number(a.y1)
             << QString::number(a.x2) << QString::number(a.y2) << QString::number(a.duration);
        runMode = "shell";
    } else if (a.type == SwipeAction::Command) {
        QString fullCommand = a.command.trimmed();
        QStringList shellArgs = ArgsParser::parse(fullCommand);
        if (shellArgs.isEmpty()) {
             emit adbStatus("CMD: Command parsing failed or empty.", true);
             return;}
        args << "shell" << shellArgs;
        runMode = "shell";
    } else {
        emit adbStatus("Unknown action type.", true);
        return;}
    if (m_executor->isRunning()) {
        emit adbStatus("Another command is currently running. Please wait.", true);
        return;}
    QString logCmd = args.join(' ').simplified();
    if (logCmd.length() > 80) logCmd = logCmd.left(80) + "...";
    emit adbStatus(QString("EXECUTING (Live): adb %1").arg(logCmd), false);
    m_executor->runAdbCommand(args);
    QListWidgetItem *item = m_list->item(row);
    if (item) item->setBackground(QBrush(QColor("#FFC107")));}

void SwipeBuilderWidget::onAdbCommandFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    int finishedRow = -1;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->background().color() == QColor("#FFC107")) {
            finishedRow = i;
            item->setBackground(Qt::NoBrush); 
            break;}}
    if (exitCode == 0) {
        if (finishedRow != -1) {
            QListWidgetItem *item = m_list->item(finishedRow);
            if (item) item->setBackground(QBrush(QColor("#4CAF50"))); 
            QTimer::singleShot(200, this, [item](){ if (item) item->setBackground(Qt::NoBrush); });}
        emit adbStatus("Execution finished successfully.", false);
    } else {
         if (finishedRow != -1) {
            QListWidgetItem *item = m_list->item(finishedRow);
            if (item) item->setBackground(QBrush(QColor("#F44336"))); 
            QTimer::singleShot(500, this, [item](){ if (item) item->setBackground(Qt::NoBrush); });}
        emit adbStatus(QString("Live execution failed with code: %1.").arg(exitCode), true);}
    startMonitoring();}

void SwipeBuilderWidget::saveJson() {
    QString path = QFileDialog::getSaveFileName(this, "Save Sequence", "", "JSON Files (*.json)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_model->toJsonSequence());
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
        emit sequenceGenerated(path);}}

void SwipeBuilderWidget::clearActions() { m_model->clear(); }

void SwipeBuilderWidget::deleteSelected() {
    int row = m_list->currentRow();
    if (row >= 0) m_model->removeActionAt(row);}

void SwipeBuilderWidget::onKeyboardToggleClicked() {
    m_keyboardWidget->setVisible(!m_keyboardWidget->isVisible());
    QPushButton *senderBtn = qobject_cast<QPushButton*>(sender());
    if (senderBtn) {
        senderBtn->setText(m_keyboardWidget->isVisible() ? "Hide Virtual Keyboard" : "Toggle Virtual Keyboard");}}
