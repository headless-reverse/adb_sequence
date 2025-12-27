#include "SwipeModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

SwipeModel::SwipeModel(QObject *parent) : QObject(parent) {}

void SwipeModel::addTap(int x, int y) {
    m_actions.append(SwipeAction(SwipeAction::Tap, x, y));
    emit modelChanged();}

void SwipeModel::addSwipe(int x1, int y1, int x2, int y2, int duration) {
    m_actions.append(SwipeAction(SwipeAction::Swipe, x1, y1, x2, y2, duration));
    emit modelChanged();}

void SwipeModel::addCommand(const QString &command, int delayMs) {
    m_actions.append(SwipeAction(SwipeAction::Command, 0, 0, 0, 0, delayMs, command));
    emit modelChanged();}

void SwipeModel::clear() {
    m_actions.clear();
    emit modelChanged();}

void SwipeModel::removeActionAt(int index) {
    if (index >= 0 && index < m_actions.size()) {
        m_actions.removeAt(index);
        emit modelChanged();}}

QJsonArray SwipeModel::toJsonSequence() const {
    QJsonArray array;
    for (const auto &action : m_actions) {
        QJsonObject obj;
        obj["delay_ms"] = action.duration; 
        
        if (action.type == SwipeAction::Tap) {
            obj["type"] = "tap";
            obj["x"] = action.x1;
            obj["y"] = action.y1;
        } else if (action.type == SwipeAction::Swipe) {
            obj["type"] = "swipe";
            obj["x1"] = action.x1;
            obj["y1"] = action.y1;
            obj["x2"] = action.x2;
            obj["y2"] = action.y2;
        } else if (action.type == SwipeAction::Command) {
             obj["type"] = "command";
             obj["command"] = action.command;
             obj["runMode"] = "shell"; 
        }
        array.append(obj);}
    return array;}
