#include "mainwindow.h"

#include <QApplication>
#include "zoombackground.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);



    ZoomBackground w("H:/Qt project/Maka_IT/typo.jpg", 5000);
    w.setWindowTitle("Maka_IT Application");
    w.resize(900, 500);
    w.show();

    return a.exec();
}
