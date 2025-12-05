#include "mainwindow.h"
#include "commandexecutor.h"
#include "settingsdialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QDockWidget>
#include <QListWidget>
#include <QMenuBar>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLineEdit>
#include <QTextEdit>
#include <QMessageBox>
#include <QFileInfo>
#include <QEvent>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDir>
#include <QHeaderView>
#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QStandardItem>

class LogDialog : public QDialog {
public:
    explicit LogDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("headless");
        resize(900, 400);
        setStyleSheet("background: #000; border: none;");
        auto layout = new QVBoxLayout(this);
        m_output = new QTextEdit();
        m_output->setReadOnly(true);
        m_output->setStyleSheet("background: #000; color: #f0f0f0; border: none; font-family: monospace;");
        layout->addWidget(m_output);
        m_fontSize = m_output->font().pointSize();
        installEventFilter(this);}
    void setDocument(QTextDocument *doc) {
        if (doc) m_output->setDocument(doc);}
    void appendText(const QString &text, const QColor &color = QColor("#f0f0f0")) {
        m_output->setTextColor(color);
        m_output->append(text);
        m_output->setTextColor(QColor("#f0f0f0"));}
    void clear() { m_output->clear(); }
    QString text() const { return m_output->toPlainText(); }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_C) {
                clear();
                return true;}}
        return QDialog::eventFilter(obj, event);}

    void wheelEvent(QWheelEvent *event) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
            int delta = event->angleDelta().y();
            if (delta > 0) m_fontSize = qMin(m_fontSize + 1, 32);
            else m_fontSize = qMax(m_fontSize - 1, 8);
            QFont f = m_output->font(); f.setPointSize(m_fontSize); m_output->setFont(f);
            event->accept();
        } else QDialog::wheelEvent(event);}

private:
    QTextEdit *m_output = nullptr;
    int m_fontSize = 9;};

QJsonObject AdbCmd::toJson() const {
    QJsonObject o;
    o.insert("command", command);
    o.insert("description", description);
    return o;}

AdbCmd AdbCmd::fromJson(const QJsonObject &obj) {
    AdbCmd c;
    c.command = obj.value("command").toString();
    c.description = obj.value("description").toString();
    return c;}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Android Debug Bridge ");
    resize(1100, 720);
    ensureJsonPathLocal();
    m_executor = new CommandExecutor(this);
    QString adbPath = m_settings.value("adbPath", "adb").toString();
    m_executor->setAdbPath(adbPath);
    connect(m_executor, &CommandExecutor::outputReceived, this, &MainWindow::onOutput);
    connect(m_executor, &CommandExecutor::errorReceived, this, &MainWindow::onError);
    connect(m_executor, &CommandExecutor::started, this, &MainWindow::onProcessStarted);
    connect(m_executor, &CommandExecutor::finished, this, &MainWindow::onProcessFinished);
    setupMenus();
    m_categoryList = new QListWidget();
    m_categoryList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_categoryList, &QListWidget::currentItemChanged, this, &MainWindow::onCategoryChanged);
    m_dockCategories = new QDockWidget(tr("Categories"), this);
    m_dockCategories->setObjectName("dockCategories");
    m_dockCategories->setWidget(m_categoryList);
    m_dockCategories->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_dockCategories);
    m_commandModel = new QStandardItemModel(this);
    m_commandModel->setHorizontalHeaderLabels(QStringList{"Command", "Description"});
    m_commandProxy = new QSortFilterProxyModel(this);
    m_commandProxy->setSourceModel(m_commandModel);
    m_commandProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_commandProxy->setFilterKeyColumn(-1);
    m_commandView = new QTreeView();
    m_commandView->setModel(m_commandProxy);
    m_commandView->setRootIsDecorated(false);
    m_commandView->setAlternatingRowColors(true);
    m_commandView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commandView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_commandView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_commandView, &QTreeView::doubleClicked, this, &MainWindow::onCommandDoubleClicked);
    connect(m_commandView, &QTreeView::clicked, [this](const QModelIndex &idx){
        QModelIndex s = m_commandProxy->mapToSource(idx);
        if (s.isValid()) {
            QString cmd = m_commandModel->item(s.row(), 0)->text();
            m_commandEdit->setText(cmd);
            if (m_dockControls) { m_dockControls->setVisible(true); m_dockControls->raise(); if (m_commandEdit) { m_commandEdit->setFocus(); m_commandEdit->selectAll(); } }
        }
    });
    m_commandView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_commandView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pt){
        QModelIndex idx = m_commandView->indexAt(pt);
        if (!idx.isValid()) return;
        QModelIndex s = m_commandProxy->mapToSource(idx);
        if (!s.isValid()) return;
        QString cmd = m_commandModel->item(s.row(), 0)->text();
        QMenu menu(this);
        menu.addAction("Execute", [this, cmd](){ m_commandEdit->setText(cmd); runCommand(); });
        menu.addAction("Edit", [this, s](){ m_commandView->selectionModel()->clear(); m_commandView->setCurrentIndex(m_commandProxy->mapFromSource(s)); editCommand(); });
        menu.addAction("Remove", [this, s](){ m_commandView->selectionModel()->clear(); m_commandView->setCurrentIndex(m_commandProxy->mapFromSource(s)); removeCommand(); });
        menu.exec(m_commandView->viewport()->mapToGlobal(pt));
    });
    m_dockCommands = new QDockWidget(tr("Commands"), this);
    m_dockCommands->setObjectName("dockCommands");
    m_dockCommands->setWidget(m_commandView);
    m_dockCommands->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_dockCommands);
    splitDockWidget(m_dockCategories, m_dockCommands, Qt::Horizontal);
    m_log = new QTextEdit();
    m_log->setReadOnly(true);
    m_log->setStyleSheet("background: #000; color: #f0f0f0; font-family: monospace;");
    m_dockLog = new QDockWidget(tr("Log Output"), this);
    m_dockLog->setObjectName("dockLog");
    m_dockLog->setWidget(m_log);
    m_dockLog->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockLog);
    QWidget *controls = createControlsWidget();
    m_dockControls = new QDockWidget(tr("Controls"), this);
    m_dockControls->setObjectName("dockControls");
    m_dockControls->setWidget(controls);
    m_dockControls->setAllowedAreas(Qt::TopDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    addDockWidget(Qt::TopDockWidgetArea, m_dockControls);
    m_viewCategoriesAct = m_dockCategories->toggleViewAction();
    m_viewCommandsAct = m_dockCommands->toggleViewAction();
    m_viewLogAct = m_dockLog->toggleViewAction();
    m_viewControlsAct = m_dockControls->toggleViewAction();
    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_viewCategoriesAct);
    viewMenu->addAction(m_viewCommandsAct);
    viewMenu->addAction(m_viewControlsAct);
    viewMenu->addAction(m_viewLogAct);
    LogDialog *dlg = new LogDialog(this);
    dlg->setDocument(m_log->document());
    dlg->move(this->x() + this->width() + 20, this->y());
    dlg->show();
    m_detachedLogDialog = dlg;
    loadCommands();
    populateCategoryList();
    if (m_categoryList->count() > 0) m_categoryList->setCurrentRow(0);
    restoreWindowStateFromSettings();
    m_commandView->installEventFilter(this);
    m_categoryList->installEventFilter(this);
    if (m_commandEdit) m_commandEdit->installEventFilter(this);}

MainWindow::~MainWindow() {
    saveCommands();
    saveWindowStateToSettings();}

void MainWindow::setupMenus() {
    QMenu *file = menuBar()->addMenu("&File");
    QAction *loadAct = file->addAction("Load commands…");
    QAction *saveAct = file->addAction("Save commands as…");
    QAction *quitAct = file->addAction("Quit");
    connect(loadAct, &QAction::triggered, this, [this]{
        QDir startDir = QDir(QCoreApplication::applicationDirPath());
        QString fn = QFileDialog::getOpenFileName(
                         this,
                         tr("Load JSON"),
                         startDir.path(),
                         tr("JSON files (*.json);;All files (*)"));
        if (!fn.isEmpty()) {
            m_jsonFile = fn;
            loadCommands();
            populateCategoryList();}});
    connect(saveAct, &QAction::triggered, this, [this]{
        QDir startDir = QDir(QCoreApplication::applicationDirPath());
        QString fn = QFileDialog::getSaveFileName(
                         this,
                         tr("Save JSON"),
                         startDir.filePath("adb_command.json"),
                         tr("JSON files (*.json);;All files (*)"));
        if (!fn.isEmpty()) {
            m_jsonFile = fn;
            saveCommands();}});
    connect(quitAct, &QAction::triggered, this, &QMainWindow::close);
    QMenu *proc = menuBar()->addMenu("&Process");
    QAction *stopAct = proc->addAction("Stop current command");
    connect(stopAct, &QAction::triggered, this, &MainWindow::stopCommand);
    QMenu *settings = menuBar()->addMenu("&Settings");
    QAction *restoreAct = settings->addAction("Restore default layout");
    connect(restoreAct, &QAction::triggered, this, &MainWindow::restoreDefaultLayout);
    QAction *appSettingsAct = settings->addAction("Application settings...");
    connect(appSettingsAct, &QAction::triggered, this, &MainWindow::showSettingsDialog);}

void MainWindow::ensureJsonPathLocal() {
    QDir exeDir = QDir(QCoreApplication::applicationDirPath());
    m_jsonFile = exeDir.filePath("adb_command.json");}

QWidget* MainWindow::createControlsWidget() {
    QWidget *w = new QWidget();
    auto mainLayout = new QVBoxLayout(w);
    m_commandEdit = new QLineEdit();
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &MainWindow::runCommand);
    mainLayout->addWidget(m_commandEdit);
    auto btnLayout = new QHBoxLayout();
    m_runBtn = new QPushButton("Execute");
    connect(m_runBtn, &QPushButton::clicked, this, &MainWindow::runCommand);
    btnLayout->addWidget(m_runBtn);
    m_stopBtn = new QPushButton("Stop");
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::stopCommand);
    btnLayout->addWidget(m_stopBtn);
    m_clearBtn = new QPushButton("Clear Log");
    connect(m_clearBtn, &QPushButton::clicked, [this](){ m_log->clear(); });
    btnLayout->addWidget(m_clearBtn);
    m_saveBtn = new QPushButton("Save Log .txt");
    connect(m_saveBtn, &QPushButton::clicked, [this](){
        QString def = QString("adb_log_%1.txt")
                          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        QString fn = QFileDialog::getSaveFileName(
            this, "Save log", QDir::current().filePath(def),
            "Text Files (*.txt);;All Files (*)");
        if (!fn.isEmpty()) {
            QFile f(fn);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(m_log->toPlainText().toUtf8());
                f.close();
                QMessageBox::information(this, "Saved",
                                         QString("Saved: %1").arg(fn));
            } else {
                QMessageBox::warning(this, "Error", "Cannot write file");}}});
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
    auto editLayout = new QHBoxLayout();
    auto addBtn = new QPushButton("Add");
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addCommand);
    editLayout->addWidget(addBtn);
    auto editBtn = new QPushButton("Edit");
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::editCommand);
    editLayout->addWidget(editBtn);
    auto rmBtn = new QPushButton("Remove");
    connect(rmBtn, &QPushButton::clicked, this, &MainWindow::removeCommand);
    editLayout->addWidget(rmBtn);
    mainLayout->addLayout(editLayout);
    return w;}

void MainWindow::loadCommands() {
    QFile f(m_jsonFile);
    if (!f.exists()) {
        m_commands.clear();
        QStringList cats = {
            "headless"
        };
        for (const QString &c: cats) m_commands.insert(c, {});
        saveCommands();
        return;}
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Error", QString("Cannot open JSON file: %1").arg(m_jsonFile)); return; }
    QByteArray data = f.readAll(); f.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    QJsonObject root = doc.object();
    m_commands.clear();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QString category = it.key();
        QJsonArray arr = it.value().toArray();
        QVector<AdbCmd> vec;
        for (auto v: arr) if (v.isObject()) vec.append(AdbCmd::fromJson(v.toObject()));
        m_commands.insert(category, vec);}}

void MainWindow::saveCommands() {
    QJsonObject root;
    for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
        QJsonArray arr;
        for (const AdbCmd &c: it.value()) arr.append(c.toJson());
        root.insert(it.key(), arr);}
    QJsonDocument doc(root);
    QFile f(m_jsonFile);
    if (!f.open(QIODevice::WriteOnly)) { QMessageBox::warning(this, "Error", QString("Cannot write JSON: %1").arg(m_jsonFile)); return; }
    f.write(doc.toJson(QJsonDocument::Indented)); f.close();}

void MainWindow::populateCategoryList() {
    m_categoryList->clear();
    for (auto it = m_commands.constBegin(); it != m_commands.constEnd(); ++it) m_categoryList->addItem(it.key());}

void MainWindow::populateCommandList(const QString &category) {
    m_commandModel->removeRows(0, m_commandModel->rowCount());
    auto vec = m_commands.value(category);
    for (const AdbCmd &c: vec) {
        QList<QStandardItem*> row;
        row << new QStandardItem(c.command) << new QStandardItem(c.description);
        m_commandModel->appendRow(row);}
    m_commandView->resizeColumnToContents(1);}

void MainWindow::onCategoryChanged(QListWidgetItem *current, QListWidgetItem *) {
    if (!current) return;
    populateCommandList(current->text());
    m_commandEdit->clear();
    m_inputHistoryIndex = -1;}

void MainWindow::onCommandSelected(const QModelIndex &current, const QModelIndex &) {
    if (!current.isValid()) return;
    QModelIndex s = m_commandProxy->mapToSource(current);
    if (s.isValid()) {
        QString cmd = m_commandModel->item(s.row(), 0)->text();
        m_commandEdit->setText(cmd);
        if (m_dockControls) { m_dockControls->setVisible(true); m_dockControls->raise(); if (m_commandEdit) { m_commandEdit->setFocus(); m_commandEdit->selectAll(); } }}}

void MainWindow::onCommandDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QModelIndex s = m_commandProxy->mapToSource(index);
    if (s.isValid()) {
        QString cmd = m_commandModel->item(s.row(), 0)->text();
        m_commandEdit->setText(cmd);
        runCommand();}}

bool MainWindow::commandNeedsRoot(const QString &cmdText) {
    Q_UNUSED(cmdText);
    return true;}

void MainWindow::runCommand() {
    const QString cmd = m_commandEdit->text().trimmed();
    if (cmd.isEmpty()) return;
    bool safeMode = m_settings.value("safeMode", false).toBool();
    if (safeMode && isDestructiveCommand(cmd)) {
        QMessageBox::warning(this, "Safe mode", "Application is in Safe Mode. Destructive commands are blocked.");
        return;}
    if (isDestructiveCommand(cmd)) {
        auto reply = QMessageBox::question(this, "Confirm", QString("Command looks destructive:\n%1\nContinue?").arg(cmd), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;}
    if (m_inputHistory.isEmpty() || m_inputHistory.last() != cmd) m_inputHistory.append(cmd);
    m_inputHistoryIndex = -1;
    QStringList args;
    args << "shell" << "su" << "-c" << cmd;
    appendLog(QString(">>> adb %1").arg(args.join(' ')), "#FFE066");
    m_executor->runAdbCommand(args);}

void MainWindow::stopCommand() {
    if (m_executor) {
        m_executor->stop();
        appendLog("Process stopped by user.", "#FFAA66");}}

void MainWindow::onOutput(const QString &text) {
    const QStringList lines = text.split('\n');
    for (const QString &l : lines) if (!l.trimmed().isEmpty()) appendLog(l.trimmed(), "#A9FFAC");}

void MainWindow::onError(const QString &text) {
    const QStringList lines = text.split('\n');
    for (const QString &l : lines) if (!l.trimmed().isEmpty()) appendLog(QString("!!! %1").arg(l.trimmed()), "#FF6565");
    logErrorToFile(text);}

void MainWindow::onProcessStarted() {
    appendLog("Process adb started.", "#8ECAE6");}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    appendLog(QString("Process finished. Exit code: %1").arg(exitCode), "#BDBDBD");
    if (exitCode != 0) appendLog(QString("Command finished with error code: %1").arg(exitCode), "#FF6565");}

void MainWindow::addCommand() {
    bool ok;
    QString cmd = QInputDialog::getText(this, "Add command", "Command:", QLineEdit::Normal, "", &ok);
    if (!ok || cmd.isEmpty()) return;
    QString desc = QInputDialog::getText(this, "Add command", "Description:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString category = m_categoryList->currentItem() ? m_categoryList->currentItem()->text() : QString();
    if (category.isEmpty()) { QMessageBox::warning(this, "No category", "Select a category first."); return; }
    m_commands[category].append({cmd, desc});
    populateCommandList(category);
    saveCommands();}

void MainWindow::editCommand() {
    QModelIndex idx = m_commandView->currentIndex();
    if (!idx.isValid()) return;
    QModelIndex s = m_commandProxy->mapToSource(idx);
    if (!s.isValid()) return;
    QString cmd = m_commandModel->item(s.row(), 0)->text();
    QString desc = m_commandModel->item(s.row(), 1)->text();
    bool ok;
    QString ncmd = QInputDialog::getText(this, "Edit command", "Command:", QLineEdit::Normal, cmd, &ok);
    if (!ok) return;
    QString ndesc = QInputDialog::getText(this, "Edit command", "Description:", QLineEdit::Normal, desc, &ok);
    if (!ok) return;
    QString category = m_categoryList->currentItem() ? m_categoryList->currentItem()->text() : QString();
    if (category.isEmpty()) return;
    auto &vec = m_commands[category];
    for (int i=0;i<vec.size();++i) {
        if (vec[i].command == cmd && vec[i].description == desc) { vec[i].command = ncmd; vec[i].description = ndesc; break; }}
    populateCommandList(category);
    saveCommands();}

void MainWindow::removeCommand() {
    QModelIndex idx = m_commandView->currentIndex();
    if (!idx.isValid()) return;
    QModelIndex s = m_commandProxy->mapToSource(idx);
    if (!s.isValid()) return;
    QString cmd = m_commandModel->item(s.row(), 0)->text();
    QString category = m_categoryList->currentItem() ? m_categoryList->currentItem()->text() : QString();
    if (category.isEmpty()) return;
    auto &vec = m_commands[category];
    for (int i=0;i<vec.size();++i) {
        if (vec[i].command == cmd) { vec.removeAt(i); break; }}
    populateCommandList(category);
    saveCommands();}

void MainWindow::appendLog(const QString &text, const QString &color) {
    QString line = text;
    if (!color.isEmpty()) m_log->setTextColor(QColor(color)); else m_log->setTextColor(QColor("#F0F0F0"));
    m_log->append(line);
    m_log->setTextColor(QColor("#F0F0F0"));}

void MainWindow::logErrorToFile(const QString &text) {
    QString logFile = QDir::current().filePath("adb_shell.log");
    QFile f(logFile);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " - " << text << "\n";
        f.close();}}

bool MainWindow::isDestructiveCommand(const QString &cmd) {
    QString c = cmd.toLower();
    return c.contains("rm ") || c.contains("wipe") || c.contains("format") || c.contains("dd ") || c.contains("flashall");}

void MainWindow::restoreDefaultLayout() {
    addDockWidget(Qt::LeftDockWidgetArea, m_dockCategories);
    addDockWidget(Qt::RightDockWidgetArea, m_dockCommands);
    splitDockWidget(m_dockCategories, m_dockCommands, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockLog);
    addDockWidget(Qt::TopDockWidgetArea, m_dockControls);
    m_settings.remove("geometry");
    m_settings.remove("windowState");}

void MainWindow::showSettingsDialog() {
    SettingsDialog dlg(this);
    dlg.setAdbPath(m_settings.value("adbPath", "adb").toString());
    dlg.setSafeMode(m_settings.value("safeMode", false).toBool());
    if (dlg.exec() == QDialog::Accepted) {
        m_settings.setValue("adbPath", dlg.adbPath());
        m_settings.setValue("safeMode", dlg.safeMode());
        m_executor->setAdbPath(dlg.adbPath());}}

void MainWindow::restoreWindowStateFromSettings() {
    if (m_settings.contains("geometry")) restoreGeometry(m_settings.value("geometry").toByteArray());
    if (m_settings.contains("windowState")) restoreState(m_settings.value("windowState").toByteArray());}

void MainWindow::saveWindowStateToSettings() {
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("windowState", saveState());}

QModelIndex MainWindow::currentCommandModelIndex() const {
    QModelIndex idx = m_commandView->currentIndex();
    if (!idx.isValid()) return QModelIndex();
    return m_commandProxy->mapToSource(idx);}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if ((obj == m_commandView || obj == m_categoryList || obj == m_commandEdit) && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            QModelIndex idx = m_commandView->currentIndex();
            if (idx.isValid()) {
                QModelIndex s = m_commandProxy->mapToSource(idx);
                if (s.isValid()) {
                    QString cmd = m_commandModel->item(s.row(), 0)->text();
                    m_commandEdit->setText(cmd);
                    runCommand();
                    return true;}}
            if (obj == m_categoryList && m_commandModel->rowCount() > 0) {
                QModelIndex first = m_commandProxy->mapFromSource(m_commandModel->index(0,0));
                if (first.isValid()) {
                    m_commandView->setCurrentIndex(first);
                    QModelIndex s = m_commandProxy->mapToSource(first);
                    if (s.isValid()) {
                        QString cmd = m_commandModel->item(s.row(), 0)->text();
                        m_commandEdit->setText(cmd);
                        runCommand();
                        return true;}}}}}
    return QMainWindow::eventFilter(obj, event);}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveWindowStateToSettings();
    QMainWindow::closeEvent(event);}

#include "mainwindow.moc"
