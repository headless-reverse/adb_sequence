#pragma once
#include <QObject>
#include <QPoint>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>

struct SwipeAction {
    enum Type {
        Tap,
        Swipe,
        Command,
        Key,
        // Wait // Opóźnienie bez akcji
    };
    
    Type type;
    int x1, y1;
    int x2, y2;
    int duration;
    int delayAfterMs;
    QString command;
    QString runMode;

    SwipeAction(Type t, int x1, int y1, int x2 = 0, int y2 = 0, 
                int duration = 0, int delayAfterMs = 0,
                const QString &cmd = QString(), const QString &mode = "shell")
        : type(t), x1(x1), y1(y1), x2(x2), y2(y2), 
          duration(duration), delayAfterMs(delayAfterMs),
          command(cmd), runMode(mode) {}
};

class SwipeModel : public QObject {
    Q_OBJECT
public:
    explicit SwipeModel(QObject *parent = nullptr);
    void addTap(int x, int y, int delayMs = 0);
    void addSwipe(int x1, int y1, int x2, int y2, int duration, int delayMs = 100);
    void addCommand(const QString &command, int delayMs = 100, const QString &runMode = "shell");
    void addKey(const QString &keyCommand, int delayMs = 100);
    void editActionAt(int index, const SwipeAction &action);
    void moveAction(int from, int to);
    SwipeAction actionAt(int index) const;
    void clear();
    void removeActionAt(int index);
    QVector<SwipeAction> actions() const { return m_actions; }
    int actionCount() const { return m_actions.count(); }
    
    QJsonArray toJsonSequence() const;
    QJsonObject actionToJson(const SwipeAction &action) const;
    
signals:
    void modelChanged();

private:
    QVector<SwipeAction> m_actions;
};
