#include "mainwindow.h"
#include "ui_mainwindow.h"

// Pour la synchro
#include <sys/time.h>
#include <time.h>


// Pour le core de la GameBoy
extern "C"
{
	 #include "../heig-boy/core/emu.h"
	 #include "../heig-boy/core/lcd.h"
	 #include "../heig-boy/core/io.h"
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialise certaines variables
    jeuCharge = 0;      // Indique qu'aucun jeu n'est charge
    pause = false;      // indique que l'on est pas en pause
    pas_a_pas = false;  // Pas de pas a pas au lancement
    quitter = false;    // On ne veut pas quitter

    // Pour ouvrir un fichier
    connect(ui->actionOuvrir,SIGNAL(triggered()),this,SLOT(ouvrirFichier()));

    // Pour quitter
    connect(ui->actionQuitter,SIGNAL(triggered()),this,SLOT(aQuitter()));
}

// Pour quitter
void MainWindow::aQuitter()
{
    quitter = true;
}

// Ouvre un fichier
void MainWindow::ouvrirFichier()
{
    // Recupere le nom du fichier a ouvrir
    QString rom = QFileDialog::getOpenFileName(this, tr("Ouvrir un fichier"),"/media/my_files",tr("Rom (*.gb)"));

    // Ouvre le fichier
    loadGame(rom.toStdString().c_str());
    quitter = false;
}

// Permet de recuperer les touches pressee
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Right)
        io_key_press(GBK_RIGHT,1);

    if(event->key() == Qt::Key_Left)
        io_key_press(GBK_LEFT,1);

    if(event->key() == Qt::Key_Up)
        io_key_press(GBK_UP,1);

    if(event->key() == Qt::Key_Down)
        io_key_press(GBK_DOWN,1);

    if(event->key() == Qt::Key_A)
        io_key_press(GBK_A,1);

    if(event->key() == Qt::Key_B)
        io_key_press(GBK_B,1);
    
    if(event->key() == Qt::Key_S)
        io_key_press(GBK_SELECT,1);

    if(event->key() == Qt::Key_Return)
        io_key_press(GBK_START,1);

    if(event->key() == Qt::Key_P)
    {
        pause = !pause;
        pas_a_pas = !pas_a_pas;
    }

    if(event->key() == Qt::Key_N)
        pause = false;

}

// Permet de recuperer les touches pressee
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{

    if(event->key() == Qt::Key_Right)
        io_key_press(GBK_RIGHT,0);

    if(event->key() == Qt::Key_Left)
        io_key_press(GBK_LEFT,0);

    if(event->key() == Qt::Key_Up)
        io_key_press(GBK_UP,0);

    if(event->key() == Qt::Key_Down)
        io_key_press(GBK_DOWN,0);

    if(event->key() == Qt::Key_A)
        io_key_press(GBK_A,0);

    if(event->key() == Qt::Key_B)
        io_key_press(GBK_B,0);

    if(event->key() == Qt::Key_S)
        io_key_press(GBK_SELECT,0);

    if(event->key() == Qt::Key_Return)
        io_key_press(GBK_START,0);
}


// Permet de loader un jeu
int MainWindow::loadGame(const char* fileName)
{

    // Load le jeu
    jeuCharge = emu_load_cart(fileName);

    // Si on a pas peu ouvrir le jeu
    if(jeuCharge)
    {
        // Cree une message box
        QMessageBox err;

        err.setText("Le jeu n'a pas pu etre ouvert!");
        err.setIcon(QMessageBox::Warning);

        // Affiche la boite de message
        err.exec();

        return -1;
    }

    // On a reussi
    return 0;
}

void MainWindow::debug(QString texte)
{
    // Cree une message box
    QMessageBox err;

    err.setText("DEBUG: " + texte);
    err.setIcon(QMessageBox::Warning);
    err.exec();
}

// Permet d'afficher une frame du LCD
void MainWindow::afficheLCD()
{
    if(!pause)
    {
        // Cree une QPixmap pour l'affichage
        QPixmap imgLCD;

        // Pour recuperer les pixels de l'image courante
        unsigned buf[144][160];

		  // Pour le temps de la synchro
		  struct timeval temps;

		  // Debute le chrono
		  gettimeofday(&temps,NULL);
		  double t1=temps.tv_sec+(temps.tv_usec/1000000.0);

        // Genere une frame
		 emu_do_frame();

        // Pour chaque ligne de l'image

        for (int i = 0; i < 144; i++)
        {
            // Recupere la ligne courante
				u32 *pixel = lcd_line(i);

            // Insere chaque pixel dans le buffer
            for (int j = 0; j < 160; j++)
                buf[i][j] = *pixel++;
		  }

        // Cree une QImage avec le contenu du buffer
        QImage img((uchar*)buf, 160, 144, QImage::Format_RGB32);

        // Cree un QPixmap
        imgLCD = imgLCD.fromImage(img);

        // Rempli le label
		  ui->label->setPixmap(imgLCD);

		  // Fin du chrono
		  gettimeofday(&temps,NULL);
		  double t2=temps.tv_sec+(temps.tv_usec/1000000.0);

		  // Synchro
			usleep(16667.0-((t2-t1)*1000000.0));
    }

    if(pas_a_pas)
        pause = true;
}

// Permet de recuperer la variable quitter
bool MainWindow::getQuitter()
{
    return quitter;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	quitter = true;
	event->accept();
}

// Permet de recuperer la variable jeuCharge
int MainWindow::getjeuCharge()
{
    return jeuCharge;
}

MainWindow::~MainWindow()
{
    delete ui;
}
