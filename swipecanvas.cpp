#include "swipecanvas.h"
#include "SwipeModel.h"
#include "control_socket.h"
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QMutexLocker>
#include <QOpenGLFunctions>

// Shader wierzchołków - bez zmian
static const char* VERTEX_SHADER_CODE = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 texcoord;
    uniform mat4 qt_Matrix;
    out vec2 vTexCoord;
    void main() {
        vTexCoord = texcoord;
        vec4 pos = vec4(position.xy, 0.0, 1.0);
        gl_Position = qt_Matrix * pos;
    }
)";

// Shader fragmentów - zoptymalizowany pod YUV420P
static const char* FRAGMENT_SHADER_CODE = R"(
    #version 330 core
    in vec2 vTexCoord;
    out vec4 fragColor;
    uniform sampler2D textureY;
    uniform sampler2D textureU;
    uniform sampler2D textureV;
    void main() {
        float y = texture(textureY, vTexCoord).r;
        float u = texture(textureU, vTexCoord).r - 0.5;
        float v = texture(textureV, vTexCoord).r - 0.5;
        float r = y + 1.402 * v;
        float g = y - 0.34414 * u - 0.71414 * v;
        float b = y + 1.772 * u;
        fragColor = vec4(r, g, b, 1.0);
    }
)";

static void logGlError(const char *where) {
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) qDebug() << "[GL ERROR]" << where << "(" << e << ")";
}

SwipeCanvas::SwipeCanvas(SwipeModel *model, ControlSocket *controlSocket, QWidget *parent)
    : QOpenGLWidget(parent), 
      m_model(model), 
      m_controlSocket(controlSocket),
      m_program(nullptr),
      m_texY(nullptr), m_texU(nullptr), m_texV(nullptr),
      m_textureInited(false),
      m_videoW(0), m_videoH(0),
      m_deviceWidth(720), m_deviceHeight(1280),
      m_scaleFactor(1.0), m_offsetX(0), m_offsetY(0),
      m_dragging(false)
{
    setMouseTracking(true);
}

SwipeCanvas::~SwipeCanvas() {
    makeCurrent();
    if (m_texY) delete m_texY;
    if (m_texU) delete m_texU;
    if (m_texV) delete m_texV;
    if (m_program) delete m_program;
    m_vbo.destroy();
    m_vao.destroy();
    doneCurrent();
}

void SwipeCanvas::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (m_program) delete m_program;
    m_program = new QOpenGLShaderProgram();
    
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER_CODE);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER_CODE);
    m_program->bindAttributeLocation("position", 0);
    m_program->bindAttributeLocation("texcoord", 1);
    m_program->link();

    float vertices[] = {
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 0.0f
    };
    m_vao.create();
    m_vao.bind();
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(float));
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    m_vao.release();
    m_vbo.release();}

void SwipeCanvas::initTextures(int width, int height) {
    makeCurrent();
    if (m_texY) delete m_texY;
    if (m_texU) delete m_texU;
    if (m_texV) delete m_texV;

    auto createTex = [&](int w, int h) -> QOpenGLTexture* {
        QOpenGLTexture* tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
        tex->setFormat(QOpenGLTexture::R8_UNorm);
        tex->setSize(w, h);
        tex->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
        tex->setMinificationFilter(QOpenGLTexture::Linear);
        tex->setMagnificationFilter(QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::ClampToEdge);
        return tex;};
    m_texY = createTex(width, height);
    m_texU = createTex(width / 2, height / 2);
    m_texV = createTex(width / 2, height / 2);
    m_videoW = width;
    m_videoH = height;
    m_textureInited = true;
    calculateScale();
    doneCurrent();}

void SwipeCanvas::onFrameReady(AVFramePtr frame) {
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = frame;}
    update();}

void SwipeCanvas::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    AVFramePtr frame;
    {
        QMutexLocker locker(&m_frameMutex);
        if (!m_currentFrame) return;
        frame = m_currentFrame;
    }

    if (!m_textureInited) {
        initTextures(frame->width, frame->height);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    m_texY->bind(0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]);
    m_texY->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[0]);
    m_texU->bind(1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[1]);
    m_texU->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[1]);
    m_texV->bind(2);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[2]);
    m_texV->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[2]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Reset
    m_program->bind();
    m_vao.bind();
    m_program->setUniformValue("textureY", 0);
    m_program->setUniformValue("textureU", 1);
    m_program->setUniformValue("textureV", 2);
    
    QMatrix4x4 matrix;
    m_program->setUniformValue("qt_Matrix", matrix);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_vao.release();
    m_program->release();
    QPainter p(this);
    if (m_dragging) {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
        p.drawLine(m_start, m_end);
        p.setBrush(Qt::green);
        p.drawEllipse(m_start, 6, 6);
        p.setBrush(Qt::red);
        p.drawEllipse(m_end, 6, 6);
    }
}

void SwipeCanvas::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    calculateScale();
}

void SwipeCanvas::calculateScale() {
    if (m_videoW <= 0 || m_videoH <= 0) return;
    float widgetRatio = (float)width() / height();
    float videoRatio = (float)m_videoW / m_videoH;
    
    if (widgetRatio > videoRatio) {
        m_scaleFactor = (double)height() / m_videoH;
        m_offsetX = (width() - (m_videoW * m_scaleFactor)) / 2;
        m_offsetY = 0;
    } else {
        m_scaleFactor = (double)width() / m_videoW;
        m_offsetY = (height() - (m_videoH * m_scaleFactor)) / 2;
        m_offsetX = 0;
    }
}

QPoint SwipeCanvas::mapToDevice(QPoint p) {
    if (m_videoW <= 0 || m_videoH <= 0) return p;
    double videoX = (p.x() - m_offsetX) / m_scaleFactor;
    double videoY = (p.y() - m_offsetY) / m_scaleFactor;
    
    int finalX = (int)(videoX * m_deviceWidth / m_videoW);
    int finalY = (int)(videoY * m_deviceHeight / m_videoH);
    
    return QPoint(qBound(0, finalX, m_deviceWidth - 1), 
                  qBound(0, finalY, m_deviceHeight - 1));
}

void SwipeCanvas::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_start = e->pos();
        m_end = e->pos();
        m_dragging = true;
        QPoint devPos = mapToDevice(m_start);
        if (m_controlSocket) m_controlSocket->sendTouchDown(devPos.x(), devPos.y());
        update();
    }
}

void SwipeCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging) {
        m_end = e->pos();
        QPoint devPos = mapToDevice(m_end);
        if (m_controlSocket) m_controlSocket->sendTouchMove(devPos.x(), devPos.y());
        update();
    }
}

void SwipeCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        QPoint devStart = mapToDevice(m_start);
        QPoint devEnd = mapToDevice(e->pos());
        if (m_controlSocket) m_controlSocket->sendTouchUp(devEnd.x(), devEnd.y());
        
        if ((m_start - e->pos()).manhattanLength() < 10) {
            m_model->addTap(devStart.x(), devStart.y());
        } else {
            m_model->addSwipe(devStart.x(), devStart.y(), devEnd.x(), devEnd.y(), 300);
        }
        update();
    }
}

void SwipeCanvas::setDeviceResolution(int width, int height) {
    m_deviceWidth = width;
    m_deviceHeight = height;
    calculateScale();
}

void SwipeCanvas::setStatus(const QString &msg, bool isError) {
    if (isError) {
        qCritical() << "Canvas Error:" << msg;
    } else {
        qInfo() << "Canvas Status:" << msg;
    }
    // Opcjonalnie: narysuj napis na ekranie lub wyemituj dalej
    update();
}
