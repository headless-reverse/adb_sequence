#include "SwipeModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <QVector>

SwipeModel::SwipeModel(QObject *parent) : QObject(parent) {}

void SwipeModel::addTap(int x, int y, int delayMs) {
    // duration=0 (dla Tap), delayAfterMs=delayMs
    m_actions.append(SwipeAction(SwipeAction::Tap, x, y, 0, 0, 0, delayMs, QString(), "root"));
    emit modelChanged();}

void SwipeModel::addSwipe(int x1, int y1, int x2, int y2, int duration, int delayMs) {
    m_actions.append(SwipeAction(SwipeAction::Swipe, x1, y1, x2, y2, duration, delayMs, QString(), "root"));
    emit modelChanged();}

void SwipeModel::addCommand(const QString &command, int delayMs, const QString &runMode) {
    m_actions.append(SwipeAction(SwipeAction::Command, 0, 0, 0, 0, 0, delayMs, command, runMode));
    emit modelChanged();}

void SwipeModel::addKey(const QString &keyName, int delayMs) {
    m_actions.append(SwipeAction(SwipeAction::Key, 0, 0, 0, 0, 0, delayMs, keyName, "root"));
    emit modelChanged();}

SwipeAction SwipeModel::actionAt(int index) const {
    if (index >= 0 && index < m_actions.size()) {
        return m_actions.at(index);}
    return SwipeAction(SwipeAction::Command, 0, 0, 0, 0, 0, 0, QString(), "shell");}

void SwipeModel::editActionAt(int index, const SwipeAction &action) {
    if (index >= 0 && index < m_actions.size()) {
        m_actions[index] = action;
        emit modelChanged();}}

void SwipeModel::moveAction(int from, int to) {
    if (from != to && from >= 0 && from < m_actions.size() && to >= 0 && to < m_actions.size()) {
        m_actions.move(from, to);
        emit modelChanged();}}

void SwipeModel::clear() {
    m_actions.clear();
    emit modelChanged();}

void SwipeModel::removeActionAt(int index) {
    if (index >= 0 && index < m_actions.size()) {
        m_actions.removeAt(index);
        emit modelChanged();}}

QJsonArray SwipeModel::toJsonSequence() const {
    QJsonArray array;
    for (const SwipeAction &action : m_actions) {
        array.append(actionToJson(action));}
    return array;}

QJsonObject SwipeModel::actionToJson(const SwipeAction &action) const {
    QJsonObject obj;
    QString finalCommand;
    obj["delayAfterMs"] = action.delayAfterMs;
    obj["runMode"] = action.runMode.isEmpty() ? "shell" : action.runMode.toLower();
    obj["stopOnError"] = true;

    switch (action.type) {
        case SwipeAction::Tap:
            finalCommand = QString("input tap %1 %2").arg(action.x1).arg(action.y1);
            break;
        case SwipeAction::Swipe:
            finalCommand = QString("input swipe %1 %2 %3 %4 %5")
                           .arg(action.x1).arg(action.y1)
                           .arg(action.x2).arg(action.y2)
                           .arg(action.duration);
            break;
        case SwipeAction::Key:
            finalCommand = QString("input keyevent %1").arg(action.command);
            break;
        case SwipeAction::Command:
            finalCommand = action.command;
            break;}
    obj["command"] = finalCommand;
    return obj;}
