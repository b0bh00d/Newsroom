#include "mainwindow.h"
#include <QApplication>

#include "runguard.h"

int main(int argc, char *argv[])
{
    RunGuard guard("{993B2710-0F9C-43B0-9687-6AFC29EA53DB}");
    if(!guard.tryToRun())
        return 0;

    QApplication a(argc, argv);
    MainWindow w;
    w.hide();

    return a.exec();
}
