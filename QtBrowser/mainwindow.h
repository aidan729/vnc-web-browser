#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QLineEdit>
#include "vncserver.h"

class WebView; // forward declaration for our custom webengineview

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;
    void onNewTab();
    // helper to get the current tabs QWebEngineView
    QWebEngineView* currentWebView() const;
    void onToggleVnc();

private slots:
    void onAddressEntered();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);


private:

    // ui elements
    QTabWidget   *tabWidget;
    QLineEdit    *addressBar;
    QToolBar     *navBar;
    QAction      *backAction;
    QAction      *forwardAction;
    QAction      *reloadAction;

    bool vncEnabled = false;
    VncServer *vncServer = nullptr;
};

/// Custom QWebEngineView to handle new window/tab requests
class WebView : public QWebEngineView {
    Q_OBJECT
public:
    explicit WebView(QWidget *parent = nullptr) : QWebEngineView(parent) {}

protected:
    // override to handle requests to open new windows
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;
};

#endif // MAINWINDOW_H
