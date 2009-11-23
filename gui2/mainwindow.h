#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui>

// Pour le core de la GameBoy
extern "C"
{
	 #include "../heig-boy/core/emu.h"
	 #include "../heig-boy/core/lcd.h"
	 #include "../heig-boy/core/io.h"
}

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void debug(QString texte);

    // Permet de loader un jeu
    int loadGame(const char* fileName);

    // Permet d'afficher une frame du LCD
    void afficheLCD();

    // Permet d'emuler les touches
    void lectureTouche();

    // Permet de recuperer la variable quitter
    bool getQuitter();

    // Permet de recuperer la variable jeuCharge
    int getjeuCharge();

private slots:
    // Pour ouvrir une rom
    void ouvrirFichier();

    // Pour quitter
    void aQuitter();

private:
    // Variable
    // ========
    Ui::MainWindow *ui;

    // Permet de savoir si un jeu est charger
    int jeuCharge;

    // Permet de mettre en pause
    bool pause;

    // Permet de faire du pas a pas
    bool pas_a_pas;

    // Pour stopper le jeu
    bool stop;

    // Pour stopper la boucle principale dans le main
    bool quitter;

    // Methodes
    // ========
    // Permet de recuperer les touches pressee
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);


};

#endif // MAINWINDOW_H
