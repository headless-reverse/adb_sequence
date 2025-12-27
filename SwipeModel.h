#pragma once
#include <QObject>
#include <QPoint>
#include <QVector>
#include <QJsonArray>

struct SwipeAction {
    enum Type {
        Tap,
        Swipe,
        Command
    };
    
    Type type;
    int x1, y1;
    int x2, y2;
    int duration;
    QString command;

    SwipeAction(Type t, int x1, int y1, int x2 = 0, int y2 = 0, int duration = 0, const QString &cmd = QString())
        : type(t), x1(x1), y1(y1), x2(x2), y2(y2), duration(duration), command(cmd) {}
};

class SwipeModel : public QObject {
    Q_OBJECT
public:
    explicit SwipeModel(QObject *parent = nullptr);
    void addTap(int x, int y);
    void addSwipe(int x1, int y1, int x2, int y2, int duration);
    void addCommand(const QString &command, int delayMs = 100);
    void clear();
    void removeActionAt(int index);
    QVector<SwipeAction> actions() const { return m_actions; }
    QJsonArray toJsonSequence() const;

signals:
    void modelChanged();

private:
    QVector<SwipeAction> m_actions;
};
