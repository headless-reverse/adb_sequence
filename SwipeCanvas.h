#pragma once
#include <QWidget>
#include <QImage>
#include <QPoint>
#include "SwipeModel.h"

class SwipeCanvas : public QWidget {
    Q_OBJECT
public:
    explicit SwipeCanvas(SwipeModel *model, QWidget *parent = nullptr);
    void loadFromData(const QByteArray &data);
    
    void setStatus(const QString &msg, bool isError);
    void setDeviceResolution(int width, int height);
    void setCaptureMode(bool useRaw);

signals:
    void tapAdded(int x, int y);
    void swipeAdded(int x1, int y1, int x2, int y2, int duration);
    
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    SwipeModel *m_model;
    QImage m_img;
    bool m_dragging = false;
    QPoint m_start;
    QPoint m_end;
    QPoint mapToDevice(QPoint p);
    double m_scaleFactor = 1.0;
    int m_offsetX = 0;
    int m_offsetY = 0;
    QString m_statusMessage; 
    bool m_isErrorStatus = false;
    int m_deviceWidth = 0;
    int m_deviceHeight = 0;
    bool m_useRawMode = true; // domyślnie RAW
};
