/**
 Classe de gestion de l'�mulation.
 G�re l'ouverture d'une ROM puis de l'affichage
 de la partie.
 */

#pragma once

#include "wx/wx.h"


class RefreshManager : public wxThread
{
    MainWindow *frame;
	#ifdef WIN32
	unsigned long long frequency;
	#else
	#endif
	unsigned long long lastTime;
	bool romLoaded;
	wxMutex mutexRomLoaded;
	bool pauseActive;
	wxMutex mutexPause;

public:
	wxMutex mutexInFrame;
	bool fastMode;
	
	/**
	 Mettre la partie en pause
	 \param active Pour activer ou d�sactiver la pause
	 */
	void pause(bool active);

	/**
	 Mode turbo on/off
	 \param active Pour activer ou d�sactiver le turbo
	 */
	void turbo(bool active);
	
	/**
	 Constructeur
	 \param frame La fen�tre m�re
	 */
	RefreshManager(MainWindow *frame);

	/**
	 Initialisation de la synchro, platform specifique.
	 */
	void synchroInit();

	/**
	 Obtient la valeur du temps courant.
	 */
	unsigned long long getTime();

	/**
	 Synchronize @ 60 fps. Retourne vrai si on est en retard.
	 \return vrai si on est en retard.
	 */
	bool synchroDo();
	/**
	 Permet de charger un jeu
	 */
	void loadRom();

	/**
	 Point d'entr�e du thread
	 */
	virtual void *Entry();
};

