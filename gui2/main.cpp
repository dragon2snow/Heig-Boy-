#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.show();


	 w.loadGame("sml_bck.gb");

    while(!w.getQuitter())
    {
        w.afficheLCD();
        a.processEvents();
    }

    w.debug("QUITTER");

    return a.exec();
}
