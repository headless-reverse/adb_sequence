#include "SwipeCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

SwipeCanvas::SwipeCanvas(SwipeModel *model, QWidget *parent)
    : QWidget(parent), m_model(model) {
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);}

void SwipeCanvas::setStatus(const QString &msg, bool isError) {
    m_statusMessage = msg;
    m_isErrorStatus = isError;
    update();}

void SwipeCanvas::setDeviceResolution(int width, int height) {
    m_deviceWidth = width;
    m_deviceHeight = height;
    if (width > 0 && height > 0 && m_img.isNull()) {
        setStatus("Device resolution known. Ready to capture in RAW mode.", false);}}

void SwipeCanvas::setCaptureMode(bool useRaw) {
    m_useRawMode = useRaw;
    if (m_useRawMode) {
        setStatus("Device resolution known. Ready to capture in RAW mode.", false);
    } else {
        m_img = QImage(); 
        setStatus("Switched to PNG mode. Waiting for image data.", false);}
    update();}

void SwipeCanvas::loadFromData(const QByteArray &data) {
    // Obsługa PNG, gdy m_useRawMode == false
    if (!m_useRawMode) {
        if (m_img.loadFromData(data, "PNG")) {
            m_statusMessage.clear();
            update();
            return;}
        setStatus("PNG Error: Failed to load PNG data. Check ADB connection or screen capture output.", true);
        return;}
    if (m_deviceWidth == 0 || m_deviceHeight == 0) {
        setStatus("Resolution unknown. Cannot process RAW data.", true);
        return;}
    int bytesPerPixel = 4;
    int expectedSize = m_deviceWidth * m_deviceHeight * bytesPerPixel;
    if (data.size() < expectedSize) {
        setStatus(QString("RAW error: Expected %1 bytes (Resolution: %2x%3), got %4. Check if the device is disconnected or the screen format is different (e.g., RGB565).")
                     .arg(expectedSize).arg(m_deviceWidth).arg(m_deviceHeight).arg(data.size()), true);
        return;}
    QImage rawImage((const uchar*)data.constData(),
                     m_deviceWidth,
                     m_deviceHeight,
                     QImage::Format_RGBA8888); 
    if (!rawImage.isNull()) {
        m_img = rawImage.convertToFormat(QImage::Format_ARGB32); 
        m_statusMessage.clear();
        update();
    } else {
        setStatus("Failed to create QImage from RAW data (Check internal format constant).", true);}}

void SwipeCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    if (m_img.isNull() || !m_statusMessage.isEmpty()) {
        p.setPen(m_isErrorStatus ? Qt::red : Qt::white);
        QString msg = m_statusMessage.isEmpty()
                      ? "Waiting for screen... (Check ADB connection)" 
                      : m_statusMessage;
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, msg);
        return;}
    double wRatio = (double)width() / m_img.width();
    double hRatio = (double)height() / m_img.height();
    m_scaleFactor = std::min(wRatio, hRatio);
    int drawnW = m_img.width() * m_scaleFactor;
    int drawnH = m_img.height() * m_scaleFactor;
    m_offsetX = (width() - drawnW) / 2;
    m_offsetY = (height() - drawnH) / 2;
    QRect targetRect(m_offsetX, m_offsetY, drawnW, drawnH);
    p.drawImage(targetRect, m_img);
    if (m_dragging) {
        p.setPen(QPen(Qt::green, 3)); 
        p.drawLine(m_start, m_end);
        p.setBrush(Qt::green);
        p.drawEllipse(m_start, 5, 5);
        p.setBrush(Qt::red);
        p.drawEllipse(m_end, 5, 5);}}

QPoint SwipeCanvas::mapToDevice(QPoint p) {
    if (m_img.isNull() || m_scaleFactor == 0) return QPoint(0,0);
    int devX = (p.x() - m_offsetX) / m_scaleFactor;
    int devY = (p.y() - m_offsetY) / m_scaleFactor;
    devX = std::max(0, std::min(devX, m_img.width()));
    devY = std::max(0, std::min(devY, m_img.height()));
    return QPoint(devX, devY);}

void SwipeCanvas::mousePressEvent(QMouseEvent *e) {
    if (m_img.isNull()) return;
    m_start = e->pos();
    m_end = m_start;
    m_dragging = true;
    update();}

void SwipeCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging) {
        m_end = e->pos();
        update();}}

void SwipeCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (!m_dragging) return;
    m_end = e->pos();
    QPoint devStart = mapToDevice(m_start);
    QPoint devEnd = mapToDevice(m_end);
    if ((m_start - m_end).manhattanLength() < 5) {
        m_model->addTap(devStart.x(), devStart.y());
    } else {
        m_model->addSwipe(devStart.x(), devStart.y(), devEnd.x(), devEnd.y(), 500);}
    m_dragging = false;
    update();}
