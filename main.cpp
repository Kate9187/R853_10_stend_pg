#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle(QString("Стенд Р-853-В2М кп 10 Сборка от %1 %2").arg(__DATE__).arg(__TIME__));
    w.show();

    return a.exec();
}
