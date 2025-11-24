#pragma once

#include <QDialog>

class QLineEdit;
class QCheckBox;
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    void setAdbPath(const QString &p);
    QString adbPath() const;
    void setSafeMode(bool v);
    bool safeMode() const;
private:
    QLineEdit *m_adbEdit = nullptr;
    QCheckBox *m_safeCheck = nullptr;};
