#include "vncclient.h"
#include <QDebug>
#include <QThread>
#include <QtEndian>
#include <qpainter.h>

// protocol constants
static const QByteArray PROTOCOL_VERSION = "RFB 003.008\n";

VncClient::VncClient(const QString &host, int port,
                     const QString &username,
                     const QString &password,
                     QObject *parent)
    : QThread(parent),
    m_host(host),
    m_port(port),
    m_username(username),
    m_password(password),
    m_socket(nullptr),
    m_running(false),
    m_isUpdating(false)
{
    m_socket = new QTcpSocket();
}

VncClient::~VncClient() {
    disconnectFromServer();
}

void VncClient::disconnectFromServer() {
    m_running = false;
    if(m_socket) {
        // schedule the disconnect to occur in the client's thread
        QMetaObject::invokeMethod(this, "doDisconnect", Qt::QueuedConnection);
    }
    if(isRunning()) {
        wait(1000);
    }
}

bool VncClient::readBytes(char *buffer, int length, int timeout) {
    // add logging at the beginning
    qDebug() << "[Client] Starting to read" << length << "bytes";

    int totalRead = 0;
    while (totalRead < length && m_running) {
        if (!m_socket->waitForReadyRead(timeout)) {
            qWarning() << "Timeout while waiting for" << length - totalRead << "bytes";
            return false;
        }
        qint64 bytesRead = m_socket->read(buffer + totalRead, length - totalRead);
        if (bytesRead <= 0)
            return false;
        totalRead += bytesRead;

        // add per read logging
        qDebug() << "[Client] Read chunk of" << bytesRead << "bytes, total so far:" << totalRead;
    }

    QByteArray debugBytes(buffer, totalRead);
    qDebug() << "[Client] readBytes(" << length << ") =" << debugBytes.toHex();

    return (totalRead == length);
}


bool VncClient::writeData(const QByteArray &data) {
    qint64 written = m_socket->write(data);
    if (written != data.size()) {
        qWarning() << "Failed to write all data. Written:" << written;
        return false;
    }
    if (written == -1) {
        qWarning() << "[Client] Socket write failed:" << m_socket->errorString();
    }
    return m_socket->flush();
}

bool VncClient::performHandshake() {
    // read servers protocol version (12 bytes).
    char serverVersion[13] = {0};
    if (!readBytes(serverVersion, 12)) {
        emit errorOccured("Failed to read server protocol version");
        return false;
    }
    qDebug() << "Server protocol version:" << serverVersion;

    // send our protocol version
    if (!writeData(PROTOCOL_VERSION)) {
        emit errorOccured("Failed to send client protocol version");
        return false;
    }
    qDebug() << "Sent client protocol version:" << PROTOCOL_VERSION;

    // read security types
    // first byte: number of security types
    char secCount;
    if (!readBytes(&secCount, 1)) {
        emit errorOccured("Failed to read security types count");
        return false;
    }
    int numSec = static_cast<int>(secCount);
    if (numSec < 1) {
        emit errorOccured("No security types offered by server");
        return false;
    }
    QByteArray secTypes = m_socket->read(numSec);
    if (secTypes.size() != numSec) {
        emit errorOccured("Failed to read security types");
        return false;
    }
    qDebug() << "Security types offered:" << secTypes.toHex();

    // Ffr simplicity we choose type 1 (no authentication) if available
    if (!secTypes.contains(1)) {
        emit errorOccured("Server does not support no authentication (type 1)");
        return false;
    }
    // send our chosen security type (one byte: 1)
    char chosen = 1;
    if (!writeData(QByteArray(&chosen, 1))) {
        emit errorOccured("Failed to send chosen security type");
        return false;
    }
    qDebug() << "Chose security type 1 (no authentication)";

    // read security result (4 bytes, 0 means OK)
    quint32 secResult = 0;
    if (!readBytes(reinterpret_cast<char*>(&secResult), 4)) {
        emit errorOccured("Failed to read security result");
        return false;
    }
    secResult = qFromBigEndian(secResult);
    if (secResult != 0) {
        emit errorOccured(QString("Security handshake failed, result: %1").arg(secResult));
        return false;
    }
    qDebug() << "Security result OK";

    // send ClientInit (1 byte) (Well indicate shared session (value = 1))
    char clientInit = 1;
    if (!writeData(QByteArray(&clientInit, 1))) {
        emit errorOccured("Failed to send ClientInit");
        return false;
    }
    qDebug() << "Sent ClientInit (shared=1)";

    // process ServerInit message
    if (!processServerInit())
        return false;

    return true;
}

bool VncClient::processServerInit() {
    char header[4];
    if (!readBytes(header, 4)) {
        emit errorOccured("Failed to read ServerInit screen dimensions");
        return false;
    }

    // read screen dimensions
    quint16 screenWidth = qFromBigEndian(*reinterpret_cast<quint16*>(header));
    quint16 screenHeight = qFromBigEndian(*reinterpret_cast<quint16*>(header + 2));
    qDebug() << "Server screen size:" << screenWidth << "x" << screenHeight;

    // parse pixel format (16 bytes)
    char pixelFormat[16];
    if (!readBytes(pixelFormat, 16)) {
        emit errorOccured("Failed to read pixel format");
        return false;
    }

    // extract key pixel format details
    quint8 bitsPerPixel = pixelFormat[0];
    quint8 depth = pixelFormat[1];
    quint8 bigEndianFlag = pixelFormat[2];
    quint8 trueColorFlag = pixelFormat[3];
    quint16 redMax = qFromBigEndian(*reinterpret_cast<quint16*>(pixelFormat + 4));
    quint16 greenMax = qFromBigEndian(*reinterpret_cast<quint16*>(pixelFormat + 6));
    quint16 blueMax = qFromBigEndian(*reinterpret_cast<quint16*>(pixelFormat + 8));
    quint8 redShift = pixelFormat[10];
    quint8 greenShift = pixelFormat[11];
    quint8 blueShift = pixelFormat[12];

    // debug log to verify server's pixel format
    qDebug() << "Server pixel format:"
             << "Bits:" << bitsPerPixel
             << "Depth:" << depth
             << "RGB shifts:" << redShift << greenShift << blueShift;

    // read desktop name
    char nameLenBytes[4];
    if (!readBytes(nameLenBytes, 4)) {
        emit errorOccured("Failed to read desktop name length");
        return false;
    }
    quint32 nameLen = qFromBigEndian(*reinterpret_cast<quint32*>(nameLenBytes));
    QByteArray nameData = m_socket->read(nameLen);
    if (nameData.size() != static_cast<int>(nameLen)) {
        emit errorOccured("Failed to read desktop name");
        return false;
    }
    QString desktopName = QString::fromUtf8(nameData);
    qDebug() << "Desktop name:" << desktopName;

    // initialize framebuffer with server's format (assume 32-bit BGRA)
    m_framebufferImage = QImage(screenWidth, screenHeight, QImage::Format_RGB32);

    return true;
}

void VncClient::requestFramebufferUpdate() {
    QByteArray request;
    QDataStream out(&request, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);

    // RFB FramebufferUpdateRequest message structure
    out << (quint8)3;    // message type (3 = FramebufferUpdateRequest)
    out << (quint8)1;    // incremental flag (1 = only send changes)
    out << (quint16)0    // x position
        << (quint16)0    // y position
        << (quint16)m_framebufferImage.width()
        << (quint16)m_framebufferImage.height();

    if (!writeData(request)) {
        emit errorOccured("Failed to send framebuffer update request");
    } else {
        qDebug() << "[Client] Sent FramebufferUpdateRequest:" << request.toHex();
    }
}

void VncClient::run() {
    m_running = true;
    //m_socket = new QTcpSocket();
    m_socket->connectToHost(m_host, m_port);
    if (!m_socket->waitForConnected(5000)) {
        emit errorOccured("Failed to connect to server: " + m_socket->errorString());
        m_running = false;
        return;
    }

    qDebug() << "Connected to VNC server at" << m_host << ":" << m_port;

    // perform handshake
    if (!performHandshake()) {
        m_running = false;
        return;
    }

    // request full framebuffer update after handshake
    requestFramebufferUpdate();
    qDebug() << "[Client] Sent initial FramebufferUpdateRequest.";

    // now enter the main loop to process messages
    while (m_running) {
        if (!m_socket->waitForReadyRead(100)) {
            continue;
        }

        while (m_socket->bytesAvailable() > 0) {
            if (!processServerMessage()) {
                return;
            }
        }
    }

}

bool VncClient::processServerMessage() {
    qDebug() << "[Client] processServerMessage - bytes available:" << m_socket->bytesAvailable();

    if (m_socket->bytesAvailable() < 1)
        return true;

    // Before reading message type, dump the first few bytes as hex
    QByteArray peekData = m_socket->peek(8); // peek at first 8 bytes
    qDebug() << "[Client] First few bytes in buffer:" << peekData.toHex();

    char msgType;
    if (!readBytes(&msgType, 1)) {
        qWarning() << "[Client] Failed to read message type.";
        return false;
    }

    qDebug() << "[Client] Received message type:" << static_cast<int>(msgType);

    switch (msgType) {
    case 0: // FramebufferUpdate
        return handleFramebufferUpdate();
    default:
        qWarning() << "Unhandled message type:" << static_cast<int>(msgType);
        return false;
    }
}


bool VncClient::handleFramebufferUpdate() {
    qDebug() << "Start of function";
    char header[4];
    if (!readBytes(header, 4)) {
        emit errorOccured("Failed to read FramebufferUpdate header");
        return false;
    }

    quint8 messageType = static_cast<quint8>(header[0]);
    quint8 padding     = static_cast<quint8>(header[1]);
    quint16 numRects   = (static_cast<quint8>(header[3]) << 8) | static_cast<quint8>(header[2]);

    qDebug() << "[Client] messageType=" << messageType
             << "padding=" << padding
             << "numRects=" << numRects;

    if (messageType != 0) {
        emit errorOccured(QString("Expected FramebufferUpdate (type=0), got %1").arg(messageType));
        return false;
    }

    if (numRects != 1) {
        emit errorOccured(QString("Expected 1 rectangle, got %1").arg(numRects));
        return false;
    }

    // next 12 bytes are for the rectangle header
    char rectHeader[12];
    if (!readBytes(rectHeader, 12)) {
        emit errorOccured("Failed to read rectangle header");
        return false;
    }

    quint16 x = qFromBigEndian(*reinterpret_cast<quint16*>(rectHeader));
    quint16 y = qFromBigEndian(*reinterpret_cast<quint16*>(rectHeader + 2));
    quint16 w = qFromBigEndian(*reinterpret_cast<quint16*>(rectHeader + 4));
    quint16 h = qFromBigEndian(*reinterpret_cast<quint16*>(rectHeader + 6));
    qint32 encoding = qFromBigEndian(*reinterpret_cast<qint32*>(rectHeader + 8)); //reads in big endian finally (the error beleive it or not was because the project wasnt being rebuilt..)

    if (encoding != 0) {
        emit errorOccured(QString("Unsupported encoding: %1").arg(encoding));
        return false;
    }

    const int bytesNeeded = w * h * 4;
    QByteArray pixelData;
    pixelData.reserve(bytesNeeded);

    while (pixelData.size() < bytesNeeded) {
        if (!m_socket->waitForReadyRead(1000)) {
            emit errorOccured("Timeout waiting for pixel data");
            return false;
        }
        pixelData.append(m_socket->read(bytesNeeded - pixelData.size()));
    }

    if (pixelData.size() != bytesNeeded) {
        emit errorOccured(QString("Incomplete pixel data. Expected %1, got %2")
                              .arg(bytesNeeded).arg(pixelData.size()));
        return false;
    }

    QImage frame(reinterpret_cast<const uchar*>(pixelData.constData()),
                 w, h, QImage::Format_RGBA8888);
    frame.save("client_received.png");

    QPainter painter(&m_framebufferImage);
    painter.drawImage(x, y, frame);
    painter.end();

    emit frameUpdated(m_framebufferImage.copy());
    return true;
}

void VncClient::doDisconnect() {
    if(m_socket) {
        m_socket->disconnectFromHost();
    }
}
