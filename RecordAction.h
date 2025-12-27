#ifndef RECORDACTION_H
#define RECORDACTION_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class RecordAction : public QDialog
{
    Q_OBJECT

public:
    explicit RecordAction(QWidget *parent = nullptr);
    ~RecordAction();

signals:
    void actionTriggered(const QString &text);

private slots:
    void onActionClicked();
    void updateHighlight();

private:
    QLineEdit *m_edit;
    QPushButton *m_btn;
    QLabel *m_highlight;
};

#endif // RECORDACTION_H
