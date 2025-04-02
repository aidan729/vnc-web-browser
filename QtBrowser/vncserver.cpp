#include "vncserver.h"
#include <QDataStream>
#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QPainter>
#include <QtOpenGLWidgets/QtOpenGLWidgets>

static const QByteArray PROTOCOL_VERSION = "RFB 003.008\n"; // must be exactly 12 bytes

VncServer::VncServer(QWidget* view, QObject* parent)
    : QTcpServer(parent), m_view(view)
{
}

void VncServer::incomingConnection(qintptr socketDescriptor) {
    qDebug() << "New Connection, socket descriptor:" << socketDescriptor;
    VncSession* session = new VncSession(socketDescriptor, m_view, this);
    session->start();
}

VncSession::VncSession(qintptr socketDescriptor, QWidget* view, QObject* parent)
    : QObject(parent),
    m_socket(new QTcpSocket(this)),
    m_view(view),
    m_handshakeDone(false)
{
    if(!m_socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Failed to set socket descriptor:" << m_socket->errorString();
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &VncSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &VncSession::onDisconnected);

    m_updateTimer.setInterval(1000);
    connect(&m_updateTimer, &QTimer::timeout, this, &VncSession::sendFramebufferUpdate);
}

void VncSession::start() {
    qDebug() << "Starting handshake, sending protocol version:" << PROTOCOL_VERSION;
    m_socket->write(PROTOCOL_VERSION);
    m_socket->flush();
}

void VncSession::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    doHandshake();
    if (m_handshakeDone) {
        processClientMessage();
    }
}

void VncSession::onDisconnected() {
    qDebug() << "Client disconnected";
    m_socket->deleteLater();
    deleteLater();
}

void VncSession::doHandshake() {
    while (!m_handshakeDone) {
        switch (m_handshakeState) {
        case HandshakeState::ReadingProtocolVersion:
            if (m_buffer.size() < 12) return;
            m_buffer.remove(0, 12);
            m_handshakeState = HandshakeState::SendingSecurityTypes;
            break;
        case HandshakeState::SendingSecurityTypes:
            m_socket->write("\x01\x01", 2);
            m_socket->flush();
            m_handshakeState = HandshakeState::ReadingChosenSecurityType;
            break;
        case HandshakeState::ReadingChosenSecurityType:
            if (m_buffer.size() < 1) return;
            m_buffer.remove(0, 1);
            m_handshakeState = HandshakeState::SendingSecurityResult;
            break;
        case HandshakeState::SendingSecurityResult: {
            quint32 secResult = 0;
            secResult = qToBigEndian(secResult);
            m_socket->write(reinterpret_cast<const char*>(&secResult), 4);
            m_socket->flush();
            m_handshakeState = HandshakeState::ReadingClientInit;
            break;
        }
        case HandshakeState::ReadingClientInit:
            if (m_buffer.size() < 1) return;
            m_buffer.remove(0, 1);
            m_handshakeState = HandshakeState::SendingServerInit;
            break;
        case HandshakeState::SendingServerInit:
            sendServerInit();
            m_handshakeDone = true;
            m_handshakeState = HandshakeState::Done;
            break;
        default:
            return;
        }
    }
}

void VncSession::sendServerInit() {
    QByteArray initBytes;
    QDataStream out(&initBytes, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);

    int screenWidth = m_view ? m_view->width() : 640;
    int screenHeight = m_view ? m_view->height() : 480;

    out << (quint16)screenWidth;
    out << (quint16)screenHeight;

    out << (quint8)32 << (quint8)24 << (quint8)0 << (quint8)1;
    out << (quint16)255 << (quint16)255 << (quint16)255;
    out << (quint8)16 << (quint8)8 << (quint8)0;
    out.writeRawData("\0\0\0", 3);

    QString desktopName = "Qt VNC Browser";
    QByteArray nameBytes = desktopName.toUtf8();
    out << (quint32)nameBytes.size();
    out.writeRawData(nameBytes.constData(), nameBytes.size());

    m_socket->write(initBytes);
    m_socket->flush();
    forceInitialUpdate();
    m_updateTimer.start();
}

void VncSession::sendFramebufferUpdate() {
    if (!m_view) return;

    m_view->update();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    QPixmap capture = m_view->grab();

    if (QOpenGLWidget* gl = m_view->findChild<QOpenGLWidget*>()) {
        QImage glImage = gl->grabFramebuffer();
        QPainter p(&capture);
        p.drawImage(gl->geometry().topLeft(), glImage);
        p.end();
    }

    QImage image = capture.toImage().convertToFormat(QImage::Format_RGBA8888);
    quint16 w = image.width();
    quint16 h = image.height();

    QByteArray update;
    QDataStream stream(&update, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_0);
    stream.setByteOrder(QDataStream::BigEndian);

    // --- FramebufferUpdate Header ---
    stream << (quint8)0      // message type: 0 = FramebufferUpdate
           << (quint8)0      // padding (0)
           << (quint16)1;    // number of rectangles (1)

    // --- Rectangle header ---
    stream << (quint16)0         // x position
           << (quint16)0         // y position
           << w                  // width
           << h                  // height
           << (qint32)0;         // encoding type: 0 = raw

    // --- Pixel data ---
    int rowBytes = w * 4;
    for (int y = 0; y < h; ++y) {
        const uchar* row = image.constScanLine(y);
        update.append(reinterpret_cast<const char*>(row), rowBytes);
    }

    qDebug() << "Sending framebuffer update of size:" << update.size();
    qDebug() << "Sent header bytes (first 20):" << update.left(20).toHex();

    m_socket->write(update);
    m_socket->flush();

    image.save("framebuffer_output.png");
}

void VncSession::processClientMessage() {
    qDebug() << "[Server] processClientMessage() - buffered:" << m_buffer.size();
    while (m_buffer.size() > 0) {
        quint8 msgType = static_cast<quint8>(m_buffer.at(0));
        if (msgType == 3 && m_buffer.size() >= 10) {
            m_buffer.remove(0, 10);
            sendFramebufferUpdate();
            qDebug() << "[Server] Handling FramebufferUpdateRequest.";
        } else {
            m_buffer.remove(0, 1);
        }
    }
}

void VncSession::forceInitialUpdate() {
    QTimer::singleShot(500, this, [this]() {
        sendFramebufferUpdate();
    });
}
