#include "mainwindow.h"
#include <QTabWidget>
#include <QToolBar>
#include <QLineEdit>
#include <QToolButton>
#include <QAction>
#include <QUrl>
#include <QUrlQuery>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineHistory>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QHostAddress>

QString getLocalIpAddress() {
    const QList<QHostAddress>& allAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : allAddresses) {
        // choose IPv4 addresses that are not the loopback address
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            return address.toString();
    }
    // fallback to localhost if nothing else is found
    return QHostAddress(QHostAddress::LocalHost).toString();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tabWidget(new QTabWidget(this)), addressBar(new QLineEdit(this))
{
    // setup main window properties
    setWindowTitle("Qt Web Browser");
    resize(1024, 768);

    // configure tab widget for browser pages
    tabWidget->setTabsClosable(true);
    tabWidget->setDocumentMode(true);            // modern tab style
    tabWidget->setMovable(true);                 // allow reordering tabs
    setCentralWidget(tabWidget);

    // connect tab widget signals for change and close
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    // navigation toolbar (Back, Forward, Reload, and address bar)
    navBar = addToolBar("Navigation");
    navBar->setMovable(false);
    navBar->setIconSize(QSize(16, 16));  // small icon size for a modern compact look

    // back button
    backAction = navBar->addAction(QIcon(":/icons//left-arrow.png"), "");
    backAction->setToolTip("Back");
    connect(backAction, &QAction::triggered, this, [this](){
        if (auto *webView = currentWebView()) webView->back();
    });
    // forward button
    forwardAction = navBar->addAction(QIcon(":/icons//right-arrow.png"), "");
    forwardAction->setToolTip("Forward");
    connect(forwardAction, &QAction::triggered, this, [this](){
        if (auto *webView = currentWebView()) webView->forward();
    });
    // Reload/Refresh button
    reloadAction = navBar->addAction(QIcon(":/icons//refresh-page-option.png"), "");
    reloadAction->setToolTip("Refresh");
    connect(reloadAction, &QAction::triggered, this, [this](){
        if (auto *webView = currentWebView()) webView->reload();
    });

    // address bar (QLineEdit for URLs and search terms)
    addressBar->setClearButtonEnabled(true);
    addressBar->setPlaceholderText("Search or enter address");
    addressBar->setMinimumWidth(400);
    navBar->addWidget(addressBar);
    connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::onAddressEntered);

    // add a New Tab button on the tab bar corner
    QToolButton *newTabButton = new QToolButton();
    newTabButton->setIcon(QIcon(":/icons//duplicated.png"));
    newTabButton->setToolTip("New Tab");
    tabWidget->setCornerWidget(newTabButton, Qt::TopRightCorner);
    connect(newTabButton, &QToolButton::clicked, this, &MainWindow::onNewTab);

    // --- VNC Toggle Drop Down Menu ---
    QToolButton *vncToolButton = new QToolButton(this);
    vncToolButton->setText("VNC");
    vncToolButton->setPopupMode(QToolButton::InstantPopup);

    QMenu *vncMenu = new QMenu(this);
    QAction *toggleVncAction = new QAction("Toggle VNC Server", this);
    vncMenu->addAction(toggleVncAction);
    toggleVncAction->setCheckable(true);
    vncToolButton->setMenu(vncMenu);

    // add the VNC tool button to the navigation bar (on the far right)
    navBar->addWidget(vncToolButton);

    connect(toggleVncAction, &QAction::triggered, this, &MainWindow::onToggleVnc);

    // open an initial blank tab
    onNewTab();
}

void MainWindow::onToggleVnc() {
    if (!vncEnabled) {
        qDebug() << "Starting VNC server...";
        // pass the entire MainWindow (this) to capture all UI elements
        vncServer = new VncServer(centralWidget(), this);
        if (vncServer->listen(QHostAddress::Any, 5901)) {
            QString ip = getLocalIpAddress();
            qDebug() << "VNC server listening on" << ip << ":" << vncServer->serverPort();
            vncEnabled = true;

            QString info = QString("VNC server is listening on:\nIP: %1\nPort: %2")
                               .arg(ip)
                               .arg(vncServer->serverPort());
            QMessageBox::information(this, "VNC Server Started", info);
        } else {
            qWarning() << "Failed to start VNC server:" << vncServer->errorString();
            delete vncServer;
            vncServer = nullptr;
        }

    } else {
        qDebug() << "Stopping VNC server...";
        if (vncServer) {
            vncServer->close();
            delete vncServer;
            vncServer = nullptr;
        }
        vncEnabled = false;
        qDebug() << "VNC server disabled.";
    }

    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        action->setChecked(vncEnabled);
}

QWebEngineView* MainWindow::currentWebView() const {
    // get the current widget in the tab widget and cast to QWebEngineView
    return qobject_cast<QWebEngineView*>(tabWidget->currentWidget());
}

void MainWindow::onAddressEntered() {
    if (!currentWebView()) return;
    QString input = addressBar->text().trimmed();
    if (input.isEmpty()) return;

    // interpret input using QUrl::fromUserInput
    QUrl url = QUrl::fromUserInput(input);
    // if the input contains spaces or the host doesn't include a dot treat as a search query
    if (input.contains(' ') || !url.host().contains('.')) {
        QUrl searchUrl("https://www.google.com/search");
        QUrlQuery query;
        query.addQueryItem("q", input);
        searchUrl.setQuery(query);
        url = searchUrl;
    }
    currentWebView()->load(url);
}

void MainWindow::onTabChanged(int index) {
    // a different tab was activated
    Q_UNUSED(index);
    QWebEngineView *webView = currentWebView();
    if (!webView) return;
    // update address bar to current tabs URL
    addressBar->setText(webView->url().toString());
    // update window title to page title (or default if empty)
    QString title = webView->title().isEmpty() ? "Qt Web Browser" : webView->title();
    setWindowTitle(title);
    // update navigation buttons enabled state
    bool canGoBack = webView->history()->canGoBack();
    bool canGoForward = webView->history()->canGoForward();
    backAction->setEnabled(canGoBack);
    forwardAction->setEnabled(canGoForward);
}

void MainWindow::onTabCloseRequested(int index) {
    QWidget *page = tabWidget->widget(index);
    if (page) {
        tabWidget->removeTab(index);
        delete page;
    }
    if (tabWidget->count() == 0) {
        // if all tabs closed open a new blank tab (so the browser is never without a tab)
        // we could also think about just closing the application (that would be easy to implement)
        onNewTab();
    }
}

void MainWindow::onNewTab() {
    // create a new web view (our custom WebView that can handle new windows)
    WebView *webView = new WebView();
    // add the new tab with a placeholder title
    int newIndex = tabWidget->addTab(webView, "New Tab");
    tabWidget->setCurrentIndex(newIndex);

    // connect signals from the webView to update UI components
    connect(webView, &QWebEngineView::titleChanged, this, [this, webView](const QString &title){
        int idx = tabWidget->indexOf(webView);
        if (idx >= 0) {
            // update tab text to page title (or "New Tab" if title is empty)
            QString text = title.isEmpty() ? "New Tab" : title;
            tabWidget->setTabText(idx, text);
        }
        // if this tab is currently selected also update window title
        if (webView == currentWebView()) {
            setWindowTitle(title.isEmpty() ? "Qt Web Browser" : title);
        }
    });
    connect(webView, &QWebEngineView::urlChanged, this, [this, webView](const QUrl &url){
        if (webView == currentWebView()) {
            // update address bar to new URL of current tab
            addressBar->setText(url.toString());
            // update navigation buttons (back/forward) for current tab
            bool canGoBack = webView->history()->canGoBack();
            bool canGoForward = webView->history()->canGoForward();
            backAction->setEnabled(canGoBack);
            forwardAction->setEnabled(canGoForward);
        }
    });
    // enable/disable nav buttons initially for the new tab
    backAction->setEnabled(false);
    forwardAction->setEnabled(false);

    // load a default page (blank page)
    webView->setUrl(QUrl("about:blank"));
}

QWebEngineView* WebView::createWindow(QWebEnginePage::WebWindowType type) {
    // handle requests to open a new window (tab) by creating a new tab in the existing browser
    MainWindow *mainWin = qobject_cast<MainWindow*>(window());
    if (!mainWin) {
        return nullptr;
    }
    // always open new window requests as a foreground tab in this implementation
    mainWin->onNewTab();
    return mainWin->currentWebView();
}
