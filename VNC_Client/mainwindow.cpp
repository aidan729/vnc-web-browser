#include "mainwindow.h"
#include "vncclient.h"
#include "vncviewerwidget.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>

// this code just sets the ui for the main window
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client(nullptr)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *formLayout = new QHBoxLayout();
    addressEdit = new QLineEdit("10.0.0.113", this);
    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(5901);
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Username (optional)");
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Password (optional)");
    passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addWidget(addressEdit);
    formLayout->addWidget(portSpin);
    formLayout->addWidget(usernameEdit);
    formLayout->addWidget(passwordEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    connectButton = new QPushButton("Connect", this);
    disconnectButton = new QPushButton("Disconnect", this);
    disconnectButton->setEnabled(false);
    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(disconnectButton);

    viewer = new VncViewerWidget(this);
    viewer->setMinimumSize(320, 240);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(viewer);
    setCentralWidget(centralWidget);

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
}

MainWindow::~MainWindow() {
    if (client) {
        client->disconnectFromServer();
        client->wait();
        delete client;
        client = nullptr;
    }
}

//event click methods.

void MainWindow::onConnectClicked() {
    if (client)
        return;
    QString host = addressEdit->text().trimmed();
    int port = portSpin->value();
    QString user = usernameEdit->text().trimmed();
    QString pwd = passwordEdit->text().trimmed();

    client = new VncClient(host, port, user, pwd);

    connect(client, &VncClient::frameUpdated,
            viewer, &VncViewerWidget::onFrameUpdated);

    connect(client, &VncClient::errorOccured,
            this, &MainWindow::onClientError,
            Qt::QueuedConnection);

    client->start();

    connectButton->setEnabled(false);
    disconnectButton->setEnabled(true);
}

void MainWindow::onDisconnectClicked() {
    if (client) {
        client->disconnectFromServer();
        client->wait();
        delete client;
        client = nullptr;
    }
    qDebug() << "Disconnected from server.";
    connectButton->setEnabled(true);
    disconnectButton->setEnabled(false);

}

void MainWindow::onClientError(const QString &msg) {
    // this slot runs on the GUI thread (assuming a queued connection)
    // so we can safely show a message box
    QMessageBox::warning(this, "VNC Client Error", msg);
}
