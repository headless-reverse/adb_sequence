#include "RecordAction.h"
#include <QVBoxLayout>
#include <QShortcut>

RecordAction::RecordAction(QWidget *parent)
    : QDialog(parent){
    setWindowTitle("Record Action");
    setMinimumWidth(320);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Action name:"));
    m_edit = new QLineEdit(this);
    layout->addWidget(m_edit);
    m_highlight = new QLabel("Current action: (none)", this);
    m_highlight->setStyleSheet("color: #888;");
    layout->addWidget(m_highlight);

    connect(m_edit, &QLineEdit::textChanged,
            this, &RecordAction::updateHighlight);
    m_btn = new QPushButton("Action", this);
    m_btn->setAutoDefault(false);
    m_btn->setDefault(false);
    layout->addWidget(m_btn);
    connect(m_btn, &QPushButton::clicked,
            this, &RecordAction::onActionClicked);
    auto *sc = new QShortcut(QKeySequence("Ctrl+A"), this);
    connect(sc, &QShortcut::activated,
            this, &RecordAction::onActionClicked);
    m_edit->setFocus();}

RecordAction::~RecordAction() {}

void RecordAction::onActionClicked(){emit actionTriggered(m_edit->text());accept();}

void RecordAction::updateHighlight(){
    QString t = m_edit->text();
    if (t.isEmpty()) {
        m_highlight->setText("Current action: (none)");
        m_highlight->setStyleSheet("color: #888;");
    } else {
        m_highlight->setText("Current action: " + t);
        m_highlight->setStyleSheet("color: #00aaff; font-weight: bold;");}}
