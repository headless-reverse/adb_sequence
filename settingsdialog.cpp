#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent):QDialog(parent){
    setWindowTitle("Application settings");
    auto main = new QVBoxLayout(this);
    auto row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("ADB binary:"));
    m_adbEdit = new QLineEdit();
    row1->addWidget(m_adbEdit);
    auto browseAdb = new QPushButton("Browse");
    row1->addWidget(browseAdb);
    connect(browseAdb, &QPushButton::clicked, [this]() {
        QString fn = QFileDialog::getOpenFileName(this, "Select adb binary", m_adbEdit->text(), "All Files (*)");
        if (!fn.isEmpty()) m_adbEdit->setText(fn);});
    main->addLayout(row1);
    m_safeCheck = new QCheckBox("Safe mode (block destructive commands)");
    main->addWidget(m_safeCheck);
    auto btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto ok = new QPushButton("OK");
    auto cancel = new QPushButton("Cancel");
    btnRow->addWidget(ok);
    btnRow->addWidget(cancel);
    main->addLayout(btnRow);
    connect(ok, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &SettingsDialog::reject);}

SettingsDialog::~SettingsDialog() = default;
void SettingsDialog::setAdbPath(const QString &p) { m_adbEdit->setText(p); }
QString SettingsDialog::adbPath() const { return m_adbEdit->text().trimmed(); }
void SettingsDialog::setSafeMode(bool v) { m_safeCheck->setChecked(v); }
bool SettingsDialog::safeMode() const { return m_safeCheck->isChecked(); }
