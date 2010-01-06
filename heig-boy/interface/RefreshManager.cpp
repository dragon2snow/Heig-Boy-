#include "wx/wx.h"
//Pour les About Box
#include "wx/aboutdlg.h"
//Pour les dialog de parcours
#include "wx/artprov.h"

#include "MainFrame.h"
#include "RefreshManager.h"

extern "C" {
#include "../core/emu.h"
#include "../core/lcd.h"
#include "../win32/os_specific.h"
}

RefreshManager::RefreshManager(MainWindow *frame)
: wxThread(wxTHREAD_JOINABLE), frame(frame) {

	fastMode = false;
	romLoaded = false;
	//permet d'attendre le chargement d'une ROM au lancement
	mutexRomLoaded.Lock();
	pauseActive = false;
	fin = false;

	Create();
	Run();
}

void *RefreshManager::Entry() {
	bool retard = false;
	int skippedFrames = 0;
	// Initialisation
	sound_driver_init();
	synchroInit();
	// Boucle principale
	while (!fin) {
		//Attendre qu'une ROM soit chargée
		if (!romLoaded)
		{
			mutexRomLoaded.Lock();
			mutexRomLoaded.Unlock();
			if (fin)
				break;
		}
		
		//Se mettre en pause si nécessaire
		if (pauseActive)
		{
			mutexPause.Lock();
			mutexPause.Unlock();
			if (fin)
				break;
		}

		// Fait avancer le CPU d'une image
		mutexInFrame.Lock();
		emu_do_frame(!retard);
		mutexInFrame.Unlock();
		// Récupère et met à jour l'affichage de la fenêtre
		if (!retard) {
			lcd_copy_to_buffer(frame->getPixels(), 160);
			frame->Refresh(false);
		}
		// Synchro à 60 images seconde
		Yield();
		if (fastMode)
			retard = false;
		else
			retard = synchroDo();
		// S'assure qu'on ne saute pas trop de frames non plus
		// (famine sinon)
		if (!retard)
			skippedFrames = 0;
		else if (++skippedFrames > 4) {
			retard = false;			// Réinitialisation du retard
			skippedFrames = 0;
			lastTime = getTime();
		}
		
	}
	return NULL;
}

bool RefreshManager::synchroDo() {
	unsigned long long curTime;
	// Attend que 16 millisecondes se soient écoulées
	// Attente active nécessaire, car le noyau Windows est
	// cadencé à 20 millisec
	do
	{
		curTime = getTime();
	}
	while (curTime - lastTime < 16667);
	lastTime += 16667;
	// En retard?
	return curTime - lastTime >= 16667;
}

/**
 Permet de charger un jeu
 */
void RefreshManager::loadRom()
{
	romLoaded = false;

	//On ne doit pas changer de rom pendant la création d'une frame
	mutexInFrame.Lock();

	wxFileDialog dialog(frame, _T("Please choose the game to load"),
		wxEmptyString, wxEmptyString, _T("*.gb"), wxFD_OPEN);
	if (dialog.ShowModal() == wxID_OK)
	{
		//Récupèration du chemin
		wxString filename(dialog.GetPath());

		// Chargement de l'image ROM
		if (!emu_load_cart(filename.ToAscii()))
			throw "Could not find ROM file.";

		//Le jeux peux commencer
		romLoaded = true;
		mutexRomLoaded.Unlock();
	}

	mutexInFrame.Unlock();
}

/**
Mettre la partie en pause
\param active Pour activer ou désactiver la pause
*/
void RefreshManager::pause(bool active)
{
	if (active)
	{
		pauseActive = true;
		mutexPause.Lock();
	}
	else
	{
		pauseActive = false;
		mutexPause.Unlock();
	}
}

void RefreshManager::turbo(bool active)
{
	fastMode = active;
}

bool RefreshManager::isPlaying()
{
	return romLoaded;
}

void RefreshManager::endGame()
{
	fin = true;
}

#ifdef WIN32
#include <windows.h>
void RefreshManager::synchroInit() {
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
	lastTime = getTime();
}
unsigned long long RefreshManager::getTime() {
	__int64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
	return 1000000 * curTime / frequency;
}
#else
#include <sys/time.h>
void RefreshManager::synchroInit() {
	lastTime = getTime();
}
unsigned long long RefreshManager::getTime() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return (unsigned long long)t.tv_sec * 1000000 + t.tv_usec;
}
#endif