/********************************************************************************
** Form generated from reading ui file 'mainwindow.ui'
**
** Created: Wed Nov 18 18:12:22 2009
**      by: Qt User Interface Compiler version 4.5.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionQuitter;
    QAction *actionOuvrir;
    QAction *actionStop;
    QAction *actionPause;
    QWidget *centralWidget;
    QLabel *label;
    QMenuBar *menuGui;
    QMenu *menuFile;
    QMenu *menuJeu;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(300, 196);
        MainWindow->setMinimumSize(QSize(300, 196));
        MainWindow->setMaximumSize(QSize(300, 196));
        actionQuitter = new QAction(MainWindow);
        actionQuitter->setObjectName(QString::fromUtf8("actionQuitter"));
        actionOuvrir = new QAction(MainWindow);
        actionOuvrir->setObjectName(QString::fromUtf8("actionOuvrir"));
        actionStop = new QAction(MainWindow);
        actionStop->setObjectName(QString::fromUtf8("actionStop"));
        actionPause = new QAction(MainWindow);
        actionPause->setObjectName(QString::fromUtf8("actionPause"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 20, 160, 144));
        MainWindow->setCentralWidget(centralWidget);
        menuGui = new QMenuBar(MainWindow);
        menuGui->setObjectName(QString::fromUtf8("menuGui"));
        menuGui->setGeometry(QRect(0, 0, 300, 20));
        menuFile = new QMenu(menuGui);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuJeu = new QMenu(menuGui);
        menuJeu->setObjectName(QString::fromUtf8("menuJeu"));
        MainWindow->setMenuBar(menuGui);

        menuGui->addAction(menuFile->menuAction());
        menuGui->addAction(menuJeu->menuAction());
        menuFile->addAction(actionOuvrir);
        menuFile->addSeparator();
        menuFile->addAction(actionQuitter);
        menuJeu->addAction(actionStop);
        menuJeu->addAction(actionPause);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionQuitter->setText(QApplication::translate("MainWindow", "Quitter", 0, QApplication::UnicodeUTF8));
        actionOuvrir->setText(QApplication::translate("MainWindow", "Ouvrir...", 0, QApplication::UnicodeUTF8));
        actionStop->setText(QApplication::translate("MainWindow", "Stop", 0, QApplication::UnicodeUTF8));
        actionPause->setText(QApplication::translate("MainWindow", "Pause", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "TextLabel", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
        menuJeu->setTitle(QApplication::translate("MainWindow", "Jeu", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
