#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "systemcmd.h"
#include "SwipeBuilderWidget.h"
#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QDir>

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QTextEdit;
class QDockWidget;
class QTreeView;
class QStandardItemModel;
class QSortFilterProxyModel;
class QPushButton;
class CommandExecutor;
class QAction;
class LogDialog;
class SequenceRunner;
class QSpinBox;
class QCheckBox;
class QModelIndex;
class QHBoxLayout;
class QLabel;
class QDragEnterEvent;
class QDropEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onCategoryChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onCommandSelected(const QModelIndex &current, const QModelIndex &previous);
    void onCommandDoubleClicked(const QModelIndex &index);
    void runCommand();
    void stopCommand();
    void addCommand();
    void editCommand();
    void removeCommand();
    void saveCommands();
    void loadCommands();
    void onOutput(const QString &text);
    void onError(const QString &text);
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void restoreDefaultLayout();
    void showSettingsDialog();
    void navigateHistory(int direction);    
    void onScheduleButtonClicked();
    void executeScheduledCommand();
    void updateTimerDisplay();
    void loadSequenceFile();
    void startIntervalSequence();
    void onSequenceStarted();
    void onSequenceFinished(bool success);
    void onSequenceCommandExecuting(const QString &cmd, int index, int total);
    void handleSequenceLog(const QString &text, const QString &color);    
    void showSequencePreview();

private:
    QDockWidget *m_dockBuilder = nullptr;
    SwipeBuilderWidget *m_swipeBuilder = nullptr;
    QAction *m_viewBuilderAct = nullptr;
    QDockWidget *m_dockCategories = nullptr;
    QDockWidget *m_dockCommands = nullptr;
    QDockWidget *m_dockLog = nullptr;
    QDockWidget *m_dockControls = nullptr;
    QDockWidget *m_dockSequence = nullptr;
    QListWidget *m_categoryList = nullptr;
    QTreeView *m_commandView = nullptr;
    QStandardItemModel *m_commandModel = nullptr;
    QSortFilterProxyModel *m_commandProxy = nullptr;
    QLineEdit *m_commandEdit = nullptr;
    QTextEdit *m_log = nullptr;
    QPushButton *m_runBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_scheduleBtn = nullptr;
    QMap<QString, QVector<SystemCmd>> m_commands;
    CommandExecutor *m_executor = nullptr;
    QString m_adbPath = QStringLiteral("adb");
    QString m_jsonFile = QStringLiteral("adb_commands.json");
    QStringList m_inputHistory;
    int m_inputHistoryIndex = -1;    
    QCheckBox *m_shellToggle = nullptr;
    QCheckBox *m_rootToggle = nullptr;
    bool m_isRootShell = false;
    QTimer *m_commandTimer = nullptr;
    QLabel *m_commandTimerLabel = nullptr;
    QSpinBox *m_intervalSpinBox = nullptr;
    QCheckBox *m_intervalToggle = nullptr;
    QString m_scheduledCommand;
    SequenceRunner *m_sequenceRunner = nullptr;
    QCheckBox *m_sequenceIntervalToggle = nullptr;
    QSpinBox *m_sequenceIntervalSpinBox = nullptr;
    QTimer *m_sequenceIntervalTimer = nullptr;
    QLabel *m_sequenceIntervalLabel = nullptr;
    QVector<QString> m_sequenceQueue;
    QTimer *m_displayTimer = nullptr;
    QAction *m_addCommandAct = nullptr;
    QAction *m_editCommandAct = nullptr;
    QAction *m_removeCommandAct = nullptr;
    QAction *m_viewCategoriesAct = nullptr;
    QAction *m_viewCommandsAct = nullptr;
    QAction *m_viewLogAct = nullptr;
    QAction *m_viewControlsAct = nullptr;
    QAction *m_viewSequenceAct = nullptr;
    QSettings m_settings{"AdbShell", "adb_shell"};
    LogDialog *m_detachedLogDialog = nullptr;

    void populateCategoryList();
    void populateCommandList(const QString &category);
    void appendLog(const QString &text, const QString &color = QString());
    void logErrorToFile(const QString &text);
    bool isDestructiveCommand(const QString &cmd);
    QWidget* createControlsWidget();
    QWidget* createSequenceWidget();
    void setupSequenceDock();
    void ensureJsonPathLocal();
    void setupMenus();
    void restoreWindowStateFromSettings();
    void saveWindowStateToSettings();
    QModelIndex currentCommandModelIndex() const;
};

#endif // MAINWINDOW_H
