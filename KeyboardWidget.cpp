#include "KeyboardWidget.h"
#include <QDebug>
#include <QWidget>
#include <QMargins>

KeyboardWidget::KeyboardWidget(QWidget *parent)
    : QWidget(parent) {
    setStyleSheet("QPushButton { font-size: 9pt; font-weight: bold; min-width: 22px; min-height: 22px; margin: 0px; padding: 0px; }");
    setupUI();
    setupKeyLayouts();
    updateKeyboardUI();}

KeyboardWidget::~KeyboardWidget(){}

void KeyboardWidget::setupUI(){
    mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(1);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    for (int i = 0; i < 14; ++i) {
        mainLayout->setColumnStretch(i, 1);}}

QPushButton* KeyboardWidget::createKey(const QString &text, int androidKeyCode, const QString &textShift, int androidKeyCodeShift){
    QPushButton *button = new QPushButton(text);
    button->setProperty("androidKeyCode", QVariant(androidKeyCode));
    button->setProperty("textNormal", QVariant(text));
    button->setProperty("textShift", QVariant(textShift.isEmpty() ? text.toUpper() : textShift));
    if (androidKeyCodeShift != 0) {
        button->setProperty("androidKeyCodeShift", QVariant(androidKeyCodeShift));
    } else {
        button->setProperty("androidKeyCodeShift", QVariant(androidKeyCode));}
    connect(button, &QPushButton::clicked, this, &KeyboardWidget::onKeyClicked);
    return button;}

QPushButton* KeyboardWidget::createSpecialKey(const QString &text, int androidKeyCode, int colSpan, int rowSpan){
    QPushButton *button = new QPushButton(text);
    button->setProperty("androidKeyCode", QVariant(androidKeyCode));
    button->setProperty("colSpan", colSpan);
    button->setProperty("rowSpan", rowSpan);
    connect(button, &QPushButton::clicked, this, &KeyboardWidget::onSpecialKeyClicked);
    return button;}

QPushButton* KeyboardWidget::createPolishKey(const QString &text, const QString &fullChar){
    QPushButton *button = new QPushButton(text);
    button->setProperty("fullChar", QVariant(fullChar));
    connect(button, &QPushButton::clicked, this, &KeyboardWidget::onPolishKeyClicked);
    return button;}

void KeyboardWidget::setupKeyLayouts(){
    // Row 0
    mainLayout->addWidget(createKey("1", 8, "!"), 0, 0); keyMap["1"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 0)->widget());
    mainLayout->addWidget(createKey("2", 9, "@"), 0, 1); keyMap["2"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 1)->widget());
    mainLayout->addWidget(createKey("3", 10, "#"), 0, 2); keyMap["3"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 2)->widget());
    mainLayout->addWidget(createKey("4", 11, "$"), 0, 3); keyMap["4"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 3)->widget());
    mainLayout->addWidget(createKey("5", 12, "%"), 0, 4); keyMap["5"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 4)->widget());
    mainLayout->addWidget(createKey("6", 13, "^"), 0, 5); keyMap["6"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 5)->widget());
    mainLayout->addWidget(createKey("7", 14, "&"), 0, 6); keyMap["7"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 6)->widget());
    mainLayout->addWidget(createKey("8", 15, "*"), 0, 7); keyMap["8"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 7)->widget());
    mainLayout->addWidget(createKey("9", 16, "("), 0, 8); keyMap["9"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 8)->widget());
    mainLayout->addWidget(createKey("0", 7, ")"), 0, 9); keyMap["0"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 9)->widget());
    mainLayout->addWidget(createKey("-", 69, "_"), 0, 10); keyMap["-"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 10)->widget());
    mainLayout->addWidget(createKey("=", 70, "+"), 0, 11); keyMap["="] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(0, 11)->widget());
    mainLayout->addWidget(createSpecialKey("DEL", 67), 0, 12, 1, 2);
    
    // Row 1
    mainLayout->addWidget(createKey("q", 45), 1, 0); keyMap["q"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 0)->widget());
    mainLayout->addWidget(createKey("w", 51), 1, 1); keyMap["w"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 1)->widget());
    mainLayout->addWidget(createKey("e", 33), 1, 2); keyMap["e"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 2)->widget());
    mainLayout->addWidget(createKey("r", 46), 1, 3); keyMap["r"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 3)->widget());
    mainLayout->addWidget(createKey("t", 48), 1, 4); keyMap["t"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 4)->widget());
    mainLayout->addWidget(createKey("y", 53), 1, 5); keyMap["y"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 5)->widget());
    mainLayout->addWidget(createKey("u", 49), 1, 6); keyMap["u"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 6)->widget());
    mainLayout->addWidget(createKey("i", 37), 1, 7); keyMap["i"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 7)->widget());
    mainLayout->addWidget(createKey("o", 43), 1, 8); keyMap["o"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 8)->widget());
    mainLayout->addWidget(createKey("p", 44), 1, 9); keyMap["p"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 9)->widget());
    mainLayout->addWidget(createKey("[", 71, "{"), 1, 10); keyMap["["] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 10)->widget());
    mainLayout->addWidget(createKey("]", 72, "}"), 1, 11); keyMap["]"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 11)->widget());
    mainLayout->addWidget(createKey("\\", 73, "|"), 1, 12); keyMap["\\"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(1, 12)->widget());
    
    // Row 2
    mainLayout->addWidget(createSpecialKey("TAB", 61), 2, 0, 1, 1);
    mainLayout->addWidget(createKey("a", 29), 2, 1); keyMap["a"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 1)->widget());
    mainLayout->addWidget(createKey("s", 47), 2, 2); keyMap["s"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 2)->widget());
    mainLayout->addWidget(createKey("d", 32), 2, 3); keyMap["d"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 3)->widget());
    mainLayout->addWidget(createKey("f", 34), 2, 4); keyMap["f"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 4)->widget());
    mainLayout->addWidget(createKey("g", 35), 2, 5); keyMap["g"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 5)->widget());
    mainLayout->addWidget(createKey("h", 36), 2, 6); keyMap["h"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 6)->widget());
    mainLayout->addWidget(createKey("j", 38), 2, 7); keyMap["j"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 7)->widget());
    mainLayout->addWidget(createKey("k", 39), 2, 8); keyMap["k"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 8)->widget());
    mainLayout->addWidget(createKey("l", 40), 2, 9); keyMap["l"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 9)->widget());
    mainLayout->addWidget(createKey(";", 74, ":"), 2, 10); keyMap[";"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 10)->widget());
    mainLayout->addWidget(createKey("'", 75, "\""), 2, 11); keyMap["'"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(2, 11)->widget());
    mainLayout->addWidget(createSpecialKey("ENTER", 66), 2, 12, 1, 2);
    
    // Row 3
    shiftLeftButton = new QPushButton("SHIFT");
    shiftLeftButton->setCheckable(true);
    connect(shiftLeftButton, &QPushButton::toggled, this, &KeyboardWidget::onShiftToggled);
    mainLayout->addWidget(shiftLeftButton, 3, 0, 1, 2);
    mainLayout->addWidget(createKey("z", 54), 3, 2); keyMap["z"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 2)->widget());
    mainLayout->addWidget(createKey("x", 52), 3, 3); keyMap["x"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 3)->widget());
    mainLayout->addWidget(createKey("c", 31), 3, 4); keyMap["c"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 4)->widget());
    mainLayout->addWidget(createKey("v", 50), 3, 5); keyMap["v"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 5)->widget());
    mainLayout->addWidget(createKey("b", 30), 3, 6); keyMap["b"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 6)->widget());
    mainLayout->addWidget(createKey("n", 42), 3, 7); keyMap["n"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 7)->widget());
    mainLayout->addWidget(createKey("m", 41), 3, 8); keyMap["m"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 8)->widget());
    mainLayout->addWidget(createKey(",", 55, "<"), 3, 9); keyMap[","] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 9)->widget());
    mainLayout->addWidget(createKey(".", 56, ">"), 3, 10); keyMap["."] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 10)->widget());
    mainLayout->addWidget(createKey("/", 76, "?"), 3, 11); keyMap["/"] = qobject_cast<QPushButton*>(mainLayout->itemAtPosition(3, 11)->widget());
    shiftRightButton = new QPushButton("SHIFT");
    shiftRightButton->setCheckable(true);
    connect(shiftRightButton, &QPushButton::toggled, this, &KeyboardWidget::onShiftToggled);
    mainLayout->addWidget(shiftRightButton, 3, 12, 1, 2);
    
    // Row 4
    mainLayout->addWidget(createSpecialKey("CTRL", 113), 4, 0, 1, 2);
    mainLayout->addWidget(createSpecialKey("ALT", 57), 4, 2, 1, 2);
    capsLockButton = new QPushButton("CAPS");
    capsLockButton->setCheckable(true);
    connect(capsLockButton, &QPushButton::toggled, this, &KeyboardWidget::onCapsToggled);
    mainLayout->addWidget(capsLockButton, 4, 4, 1, 2);
    mainLayout->addWidget(createSpecialKey("SPACE", 62), 4, 6, 1, 6);
    mainLayout->addWidget(createSpecialKey("ALT", 57), 4, 12, 1, 1);
    mainLayout->addWidget(createSpecialKey("CTRL", 113), 4, 13, 1, 1);
    
    // Row 5 (Navigation and media keys)
    mainLayout->addWidget(createSpecialKey("HM", 3), 5, 0);
    mainLayout->addWidget(createSpecialKey("BK", 4), 5, 1);
    mainLayout->addWidget(createSpecialKey("PW", 26), 5, 2);
    mainLayout->addWidget(createSpecialKey("MN", 82), 5, 3);
    mainLayout->addWidget(createSpecialKey("V+", 24), 5, 4);
    mainLayout->addWidget(createSpecialKey("V-", 25), 5, 5);
    mainLayout->addWidget(createSpecialKey("⬆", 19), 5, 7);
    mainLayout->addWidget(createSpecialKey("⬅", 21), 6, 6);
    mainLayout->addWidget(createSpecialKey("➡", 22), 6, 8);    
    mainLayout->addWidget(createSpecialKey("⬇", 20), 7, 7);
    
    // Layout Switch button
    layoutSwitchButton = new QPushButton("PL");
    layoutSwitchButton->setCheckable(true);
    connect(layoutSwitchButton, &QPushButton::toggled, this, &KeyboardWidget::onLayoutSwitchToggled);
    mainLayout->addWidget(layoutSwitchButton, 5, 10);
    keyMap["a_pl"] = keyMap["a"]; keyMap["c_pl"] = keyMap["c"];
    keyMap["e_pl"] = keyMap["e"]; keyMap["l_pl"] = keyMap["l"];
    keyMap["n_pl"] = keyMap["n"]; keyMap["o_pl"] = keyMap["o"];
    keyMap["s_pl"] = keyMap["s"]; keyMap["z_pl"] = keyMap["z"];
    keyMap["x_pl"] = keyMap["x"];
}

void KeyboardWidget::onKeyClicked(){
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    QString command;
    if (isPolishLayout) {
        QString charToSend;
        if (isShiftActive || isCapsActive) {
             if (button->property("textNormal").toString() == "a") charToSend = "Ą";
             else if (button->property("textNormal").toString() == "c") charToSend = "Ć";
             else if (button->property("textNormal").toString() == "e") charToSend = "Ę";
             else if (button->property("textNormal").toString() == "l") charToSend = "Ł";
             else if (button->property("textNormal").toString() == "n") charToSend = "Ń";
             else if (button->property("textNormal").toString() == "o") charToSend = "Ó";
             else if (button->property("textNormal").toString() == "s") charToSend = "Ś";
             else if (button->property("textNormal").toString() == "z") charToSend = "Ź";
             else if (button->property("textNormal").toString() == "x") charToSend = "Ż";
             else charToSend = button->property("textShift").toString();
        } else {
             if (button->property("textNormal").toString() == "a") charToSend = "ą";
             else if (button->property("textNormal").toString() == "c") charToSend = "ć";
             else if (button->property("textNormal").toString() == "e") charToSend = "ę";
             else if (button->property("textNormal").toString() == "l") charToSend = "ł";
             else if (button->property("textNormal").toString() == "n") charToSend = "ń";
             else if (button->property("textNormal").toString() == "o") charToSend = "ó";
             else if (button->property("textNormal").toString() == "s") charToSend = "ś";
             else if (button->property("textNormal").toString() == "z") charToSend = "ź";
             else if (button->property("textNormal").toString() == "x") charToSend = "ż";
             else charToSend = button->property("textNormal").toString();}
        command = QString("input text \"%1\"").arg(charToSend);
    } else {
        int keyCode = button->property("androidKeyCode").toInt();
        if (isShiftActive || isCapsActive) {
            int shiftKeyCode = button->property("androidKeyCodeShift").toInt();
            command = QString("input keyevent %1 --meta 2").arg(shiftKeyCode);
        } else {
            command = QString("input keyevent %1").arg(keyCode);}}
    if (isShiftActive && !isCapsActive) {
        isShiftActive = false;
        shiftLeftButton->setChecked(false);
        shiftRightButton->setChecked(false);}
    emit adbCommandGenerated(command);
    updateKeyboardUI();}

void KeyboardWidget::onSpecialKeyClicked(){
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    int keyCode = button->property("androidKeyCode").toInt();
    emit adbCommandGenerated(QString("input keyevent %1").arg(keyCode));}

void KeyboardWidget::onPolishKeyClicked(){}

void KeyboardWidget::onShiftToggled(bool checked){
    isShiftActive = checked;
    if (shiftLeftButton->isChecked() != checked) shiftLeftButton->setChecked(checked);
    if (shiftRightButton->isChecked() != checked) shiftRightButton->setChecked(checked);
    updateKeyboardUI();}

void KeyboardWidget::onCapsToggled(bool checked){
    isCapsActive = checked;
    updateKeyboardUI();}

void KeyboardWidget::onLayoutSwitchToggled(bool checked){
    isPolishLayout = checked;
    updateKeyboardUI();}

void KeyboardWidget::updateKeyboardUI(){
    bool isUpperCase = isShiftActive || isCapsActive;
    for (auto& pair : keyMap.toStdMap()) {
        QPushButton* button = pair.second;
        QString textNormal = button->property("textNormal").toString();
        QString textShift = button->property("textShift").toString();        
        if (isUpperCase && !textShift.isEmpty()) {
            button->setText(textShift);
        } else {
            button->setText(textNormal);}}
    if (isPolishLayout) {
        keyMap["a"]->setText(isUpperCase ? "Ą" : "ą");
        keyMap["c"]->setText(isUpperCase ? "Ć" : "ć");
        keyMap["e"]->setText(isUpperCase ? "Ę" : "ę");
        keyMap["l"]->setText(isUpperCase ? "Ł" : "ł");
        keyMap["n"]->setText(isUpperCase ? "Ń" : "ń");
        keyMap["o"]->setText(isUpperCase ? "Ó" : "ó");
        keyMap["s"]->setText(isUpperCase ? "Ś" : "ś");
        keyMap["z"]->setText(isUpperCase ? "Ź" : "ź");
        keyMap["x"]->setText(isUpperCase ? "Ż" : "ż");
    } else {
        keyMap["a"]->setText(isUpperCase ? "A" : "a");
        keyMap["c"]->setText(isUpperCase ? "C" : "c");
        keyMap["e"]->setText(isUpperCase ? "E" : "e");
        keyMap["l"]->setText(isUpperCase ? "L" : "l");
        keyMap["n"]->setText(isUpperCase ? "N" : "n");
        keyMap["o"]->setText(isUpperCase ? "O" : "o");
        keyMap["s"]->setText(isUpperCase ? "S" : "s");
        keyMap["z"]->setText(isUpperCase ? "Z" : "z");
        keyMap["x"]->setText(isUpperCase ? "X" : "x");
    }
    QString styleActive = "background-color: #a0c4ff;";
    QString styleInactive = "";
    shiftLeftButton->setStyleSheet(isShiftActive ? styleActive : styleInactive);
    shiftRightButton->setStyleSheet(isShiftActive ? styleActive : styleInactive);
    capsLockButton->setStyleSheet(isCapsActive ? styleActive : styleInactive);
    layoutSwitchButton->setStyleSheet(isPolishLayout ? styleActive : styleInactive);}
