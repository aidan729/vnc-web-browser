#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class VncViewerWidget;
class VncClient;
class QLineEdit;
class QSpinBox;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onClientError(const QString &msg);

private:
    QLineEdit *addressEdit;
    QSpinBox  *portSpin;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    VncViewerWidget *viewer;
    VncClient *client;
};

#endif // VNCMAINWINDOW_H
