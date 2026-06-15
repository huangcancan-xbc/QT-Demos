#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setFont(QFont("Microsoft YaHei", 12));
    Widget w;
    w.show();
    return a.exec();
}
