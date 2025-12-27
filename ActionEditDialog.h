#ifndef ACTIONEDITDIALOG_H
#define ACTIONEDITDIALOG_H

#include "SwipeModel.h"
#include <QDialog>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QFormLayout;

class ActionEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ActionEditDialog(SwipeAction &action, QWidget *parent = nullptr);

public slots:
    void accept() override;

private:
    SwipeAction &m_action;
    QLineEdit *m_commandEdit = nullptr;
    QComboBox *m_runModeCombo = nullptr;
    QSpinBox *m_delaySpinBox = nullptr;    
    QSpinBox *m_x1Spin = nullptr;
    QSpinBox *m_y1Spin = nullptr;
    QSpinBox *m_x2Spin = nullptr;
    QSpinBox *m_y2Spin = nullptr;
    QSpinBox *m_durationSpin = nullptr;
    void setupUi();
};

#endif // ACTIONEDITDIALOG_H
