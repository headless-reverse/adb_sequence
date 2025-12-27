#pragma once
#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QMap>
#include <QVector>

class KeyboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget();

signals:
    void adbCommandGenerated(const QString &command);
    void adbKeyCommandGenerated(const QString &keyCode);

private slots:
    void onKeyClicked();
    void onSpecialKeyClicked();
    void onPolishKeyClicked();
    void onShiftToggled(bool checked);
    void onCapsToggled(bool checked);
    void onLayoutSwitchToggled(bool checked);

private:
    void setupUI();
    QPushButton* createKey(const QString &text, int androidKeyCode, const QString &textShift = "", int androidKeyCodeShift = 0);
    QPushButton* createSpecialKey(const QString &text, int androidKeyCode, int colSpan = 1, int rowSpan = 1);
    QPushButton* createPolishKey(const QString &text, const QString &fullChar);
    void setupKeyLayouts();
    void setupMultimediaKeys();
    void updateKeyboardUI();
    QGridLayout *mainLayout;
    QMap<QString, QPushButton*> keyMap;
    
    QPushButton *shiftLeftButton;
    QPushButton *shiftRightButton;
    QPushButton *capsLockButton;
    QPushButton *layoutSwitchButton;

    bool isShiftActive = false;
    bool isCapsActive = false;
    bool isPolishLayout = false;
};
