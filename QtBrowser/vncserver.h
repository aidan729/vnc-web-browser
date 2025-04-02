#ifndef VNCSERVER_H
#define VNCSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QWebEngineView>
#include <QWidget>
#include <QObject>
#include <QTimer>

class VncSession; // this is a forward declaration for the session class

class VncServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit VncServer(QWidget* view, QObject* parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QWidget* m_view;
};

// VncSession handles a single VNC client connection and implements the RFB 3.8 handshake
// sends full ServerInit, processes FramebufferUpdateRequest (type 3), KeyEvent (type 4),
// and PointerEvent (type 5) I dont think I need any others for this but Ill come back
// to this
class VncSession : public QObject
{
    Q_OBJECT

public:
    explicit VncSession(qintptr socketDescriptor, QWidget* view, QObject* parent = nullptr);
    void start();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket;
    QWidget* m_view;
    bool m_handshakeDone;
    QByteArray m_buffer;

    // removed fixed screen dimensions. We now use the views current size at update time
    QTimer m_updateTimer; // periodically send fullframe updates

    // handshake and message methods
    void doHandshake();
    void sendServerInit();
    void processClientMessage();
    void sendFramebufferUpdate();
    void forceInitialUpdate();

    enum class HandshakeState {
        ReadingProtocolVersion,
        SendingSecurityTypes,
        ReadingChosenSecurityType,
        SendingSecurityResult,
        ReadingClientInit,
        SendingServerInit,
        Done
    };

    HandshakeState m_handshakeState = HandshakeState::ReadingProtocolVersion;
};

#endif // VNCSERVER_H
