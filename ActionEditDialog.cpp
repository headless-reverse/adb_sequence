#include "ActionEditDialog.h"
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QDialogButtonBox>

ActionEditDialog::ActionEditDialog(SwipeAction &action, QWidget *parent)
    : QDialog(parent), m_action(action) {
    setupUi();}

void ActionEditDialog::setupUi() {
    QString title;
    switch(m_action.type) {
        case SwipeAction::Tap: title = "Edit Tap Action"; break;
        case SwipeAction::Swipe: title = "Edit Swipe Action"; break;
        case SwipeAction::Command: title = "Edit Shell Command"; break;
        case SwipeAction::Key: title = "Edit Key Event"; break;
    }
    setWindowTitle(title);
    QFormLayout *formLayout = new QFormLayout(this);

    if (m_action.type == SwipeAction::Tap || m_action.type == SwipeAction::Swipe) {
        m_x1Spin = new QSpinBox(); m_x1Spin->setRange(0, 9999); m_x1Spin->setValue(m_action.x1);
        m_y1Spin = new QSpinBox(); m_y1Spin->setRange(0, 9999); m_y1Spin->setValue(m_action.y1);
        formLayout->addRow("X:", m_x1Spin);
        formLayout->addRow("Y:", m_y1Spin);

        if (m_action.type == SwipeAction::Swipe) {
            m_x2Spin = new QSpinBox(); m_x2Spin->setRange(0, 9999); m_x2Spin->setValue(m_action.x2);
            m_y2Spin = new QSpinBox(); m_y2Spin->setRange(0, 9999); m_y2Spin->setValue(m_action.y2);
            m_durationSpin = new QSpinBox(); m_durationSpin->setRange(0, 10000); m_durationSpin->setValue(m_action.duration);
            m_durationSpin->setSuffix(" ms");
            formLayout->addRow("End X:", m_x2Spin);
            formLayout->addRow("End Y:", m_y2Spin);
            formLayout->addRow("Duration:", m_durationSpin);
        }
    }
    if (m_action.type == SwipeAction::Command || m_action.type == SwipeAction::Key) {
        m_commandEdit = new QLineEdit(m_action.command);
        if (m_action.type == SwipeAction::Key) {
            m_commandEdit->setReadOnly(true);
            formLayout->addRow("Key Code:", m_commandEdit);
        } else {
            formLayout->addRow("Command:", m_commandEdit);
        }
}
    m_runModeCombo = new QComboBox();
    m_runModeCombo->addItem("adb", "adb");
    m_runModeCombo->addItem("shell", "shell");
    m_runModeCombo->addItem("root", "root");
    m_runModeCombo->addItem("hw", "hw");
    int idx = m_runModeCombo->findData(m_action.runMode.toLower());
    if (idx != -1) m_runModeCombo->setCurrentIndex(idx);
    else m_runModeCombo->setCurrentIndex(1);
    if (m_action.type == SwipeAction::Key) {
        formLayout->addRow("Run Mode:", m_runModeCombo);
        m_runModeCombo->setEnabled(true); 
    } else if (m_action.type == SwipeAction::Command) {
        formLayout->addRow("Run Mode:", m_runModeCombo);
    }
    m_delaySpinBox = new QSpinBox();
    m_delaySpinBox->setRange(0, 60000);
    m_delaySpinBox->setSingleStep(100);
    m_delaySpinBox->setSuffix(" ms");
    m_delaySpinBox->setValue(m_action.delayAfterMs);
    formLayout->addRow("Delay After:", m_delaySpinBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ActionEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ActionEditDialog::reject);
    formLayout->addWidget(buttonBox);}

void ActionEditDialog::accept() {
    m_action.delayAfterMs = m_delaySpinBox->value();
    if (m_action.type == SwipeAction::Command && m_commandEdit) {
        m_action.command = m_commandEdit->text().trimmed();
        m_action.runMode = m_runModeCombo->currentData().toString();}
    if (m_action.type == SwipeAction::Tap) {
        if (m_x1Spin) m_action.x1 = m_x1Spin->value();
        if (m_y1Spin) m_action.y1 = m_y1Spin->value();}
    if (m_action.type == SwipeAction::Swipe) {
        if (m_x1Spin) m_action.x1 = m_x1Spin->value();
        if (m_y1Spin) m_action.y1 = m_y1Spin->value();
        if (m_x2Spin) m_action.x2 = m_x2Spin->value();
        if (m_y2Spin) m_action.y2 = m_y2Spin->value();
        if (m_durationSpin) m_action.duration = m_durationSpin->value();}
    QDialog::accept();}
