#ifndef VNCVIEWERWIDGET_H
#define VNCVIEWERWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMutex>
#include <QImage>

class VncViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit VncViewerWidget(QWidget* parent = nullptr);
    ~VncViewerWidget();

public slots:
    // slot to receive new frame images from the client
    void onFrameUpdated(const QImage& frame);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    GLuint m_texture;       // OpenGL texture for the framebuffer
    QImage m_latestFrame;   // latest frame image
    QMutex m_frameMutex;    // mutex to protect m_latestFrame access
    bool m_textureInitialized;
    int m_frameWidth;
    int m_frameHeight;
};

#endif // VNCVIEWERWIDGET_H
