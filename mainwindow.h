#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>

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

struct AdbCmd {
    QString command;
    QString description;

    QJsonObject toJson() const;
    static AdbCmd fromJson(const QJsonObject &obj);
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onCategoryChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onCommandSelected(const QModelIndex &current, const QModelIndex &previous);
    void onCommandDoubleClicked(const QModelIndex &index);
    void runCommand();
    void stopCommand();
    void onOutput(const QString &text);
    void onError(const QString &text);
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void addCommand();
    void editCommand();
    void removeCommand();
    void saveCommands();
    void loadCommands();
    void restoreDefaultLayout();
    void showSettingsDialog();

private:
    QDockWidget *m_dockCategories = nullptr;
    QDockWidget *m_dockCommands = nullptr;
    QDockWidget *m_dockLog = nullptr;
    QDockWidget *m_dockControls = nullptr;
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
    QMap<QString, QVector<AdbCmd>> m_commands;
    CommandExecutor *m_executor = nullptr;
    QString m_jsonFile = QStringLiteral("adb_command.json");
    QStringList m_inputHistory;
    int m_inputHistoryIndex = -1;
    QAction *m_viewCategoriesAct = nullptr;
    QAction *m_viewCommandsAct = nullptr;
    QAction *m_viewLogAct = nullptr;
    QAction *m_viewControlsAct = nullptr;
    QSettings m_settings{"AdbShell", "adb_shell"};
    LogDialog *m_detachedLogDialog = nullptr;
    void populateCategoryList();
    void populateCommandList(const QString &category);
    void appendLog(const QString &text, const QString &color = QString());
    void logErrorToFile(const QString &text);
    bool isDestructiveCommand(const QString &cmd);
    bool commandNeedsRoot(const QString &cmdText);
    QWidget* createControlsWidget();
    void ensureJsonPathLocal();
    void setupMenus();
    void restoreWindowStateFromSettings();
    void saveWindowStateToSettings();
    QModelIndex currentCommandModelIndex() const;
};

#endif // MAINWINDOW_H
