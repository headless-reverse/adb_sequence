#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMutex>
#include <QPoint>
#include "h264decoder.h"

class SwipeModel;
class ControlSocket;
class QMouseEvent;

class SwipeCanvas : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit SwipeCanvas(SwipeModel *model, ControlSocket *controlSocket, QWidget *parent = nullptr);
    ~SwipeCanvas();

    void setControlSocket(ControlSocket *socket) { m_controlSocket = socket; } 
    void setCaptureMode(bool raw);
    void setDeviceResolution(int width, int height); 

public slots:
    void onFrameReady(AVFramePtr frame);
    void setStatus(const QString &msg, bool isError);
    
signals:
    void tapAdded(int x, int y);
    void swipeAdded(int x1, int y1, int x2, int y2, int duration);
    void screenshotReady(const QImage &image);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    SwipeModel *m_model = nullptr;
    ControlSocket *m_controlSocket = nullptr;
    
    bool m_dragging = false;
    QPoint m_start;
    QPoint m_end;
    
    QMutex m_frameMutex;
    AVFramePtr m_currentFrame; 

    int m_videoW = 0;
    int m_videoH = 0;
    int m_deviceWidth = 0;
    int m_deviceHeight = 0;
    
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    
    QOpenGLTexture *m_texY = nullptr;
    QOpenGLTexture *m_texU = nullptr; 
    QOpenGLTexture *m_texV = nullptr;
    bool m_textureInited = false;
    
    double m_scaleFactor = 1.0;
    int m_offsetX = 0;
    int m_offsetY = 0;
    
    void initTextures(int width, int height);
    void calculateScale();
    QPoint mapToDevice(QPoint p); 
};
