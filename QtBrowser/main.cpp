#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {

    // disable GPU acceleration in Qt WebEngine
    // this was for testin purposes. to speed the webbrowser back up upon building a release build
    // ideally this should be removed.
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");

    QApplication app(argc, argv);
    QApplication::setStyle("Fusion"); // I just like the look of Fusion

    MainWindow browser;
    browser.show();

    return app.exec();
}
