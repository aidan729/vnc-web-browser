#ifndef VNCCLIENT_H
#define VNCCLIENT_H

#include <QThread>
#include <QImage>
#include <QString>
#include <QTcpSocket>

class VncClient : public QThread {
    Q_OBJECT
public:
    // initialize with connection details
    VncClient(const QString &host, int port,
              const QString &username = QString(),
              const QString &password = QString(),
              QObject *parent = nullptr);
    ~VncClient();

    void disconnectFromServer();
    bool processServerMessage();
    bool handleFramebufferUpdate();

public slots:
    void doDisconnect();

signals:
    // emitted when a new full framebuffer (QImage) is available
    void frameUpdated(const QImage &frame);
    // 3mitted if an error occurs
    void errorOccured(const QString &message);

protected:
    void run() override;

private:
    QString m_host;
    int m_port;
    QString m_username;
    QString m_password;
    QTcpSocket *m_socket;
    QImage m_framebufferImage;
    bool m_running;
    bool m_isUpdating;

    // helper methods for protocol communication
    bool readBytes(char *buffer, int length, int timeout = 3000);
    bool writeData(const QByteArray &data);
    bool performHandshake();
    bool processServerInit();
    void requestFramebufferUpdate();
};

#endif // VNCCLIENT_H
