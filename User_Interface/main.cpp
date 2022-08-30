#include <QMainWindow>
#include <QtWidgets/QApplication>
#include "user_interface.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    User_Interface w;
    w.show();

    return a.exec();
}
