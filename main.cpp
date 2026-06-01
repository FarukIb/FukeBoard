#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/logo.png"));

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
