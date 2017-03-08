#include "mainwindow.h"
#include <QApplication>

#ifndef QT_DEBUG
#include "runguard.h"
#endif

int main(int argc, char *argv[])
{
#ifndef QT_DEBUG
    RunGuard guard("{993B2710-0F9C-43B0-9687-6AFC29EA53DB}");
    if(!guard.tryToRun())
        return 0;
#endif

    QApplication a(argc, argv);
    MainWindow w;
    w.hide();

    return a.exec();
}
