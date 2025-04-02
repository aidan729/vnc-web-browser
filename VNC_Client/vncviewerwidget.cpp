#include "vncviewerwidget.h"
#include <QOpenGLTexture>
#include <QPainter>
#include <QDebug>

// this class is similar to my fist submission
// it is just a custom viewer widget to properly display the framebuffer

VncViewerWidget::VncViewerWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    m_texture(0),
    m_textureInitialized(false),
    m_frameWidth(0),
    m_frameHeight(0)
{
    setMinimumSize(320, 240);
}

VncViewerWidget::~VncViewerWidget() {
    makeCurrent();
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    doneCurrent();
}

void VncViewerWidget::initializeGL() {
    initializeOpenGLFunctions();
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VncViewerWidget::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);
}

void VncViewerWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_texture || m_frameWidth == 0 || m_frameHeight == 0)
        return;

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!m_textureInitialized) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_frameWidth, m_frameHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, m_latestFrame.bits());
        m_textureInitialized = true;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_frameWidth, m_frameHeight,
                        GL_RGBA, GL_UNSIGNED_BYTE, m_latestFrame.bits());
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width(), 0, height(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float widgetW = width();
    float widgetH = height();
    float scale = qMin(widgetW / float(m_frameWidth), widgetH / float(m_frameHeight));
    float dispW = m_frameWidth * scale;
    float dispH = m_frameHeight * scale;
    float xOffset = (widgetW - dispW) / 2.0f;
    float yOffset = (widgetH - dispH) / 2.0f;

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(xOffset, yOffset);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(xOffset + dispW, yOffset);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(xOffset + dispW, yOffset + dispH);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(xOffset, yOffset + dispH);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        qDebug() << "OpenGL error:" << err;
}

void VncViewerWidget::onFrameUpdated(const QImage &frame) {
    QMutexLocker locker(&m_frameMutex);
    m_latestFrame = frame.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
    m_frameWidth = m_latestFrame.width();
    m_frameHeight = m_latestFrame.height();
    m_textureInitialized = false;
    update();
}
