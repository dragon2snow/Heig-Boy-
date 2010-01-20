// for compilers that support precompilation, includes "wx/wx.h"
#include "wx/wx.h"
//Pour les About Box
#include "wx/aboutdlg.h"
// Classes implémentées
#include "MainFrame.h"
#include "RefreshManager.h"
#include "KeysMap.h"
#include "ConfigDialog.h"

extern "C"  {
	#include "../core/io.h"
	#include "../core/save.h"
}


#define S(x) wxString::FromAscii(x)

IMPLEMENT_APP(MyApp)

// La tables des évênements permettant de faire le lien
// avec la fonction de gestion.
BEGIN_EVENT_TABLE(MainWindow, wxFrame)
EVT_PAINT(MainWindow::onPaint)
EVT_KEY_DOWN(MainWindow::onKeyDown)
EVT_KEY_UP(MainWindow::onKeyUp)
EVT_CLOSE(MainWindow::onClose)
EVT_SIZE(MainWindow::onSizing)
EVT_MENU(MainWindow::idMenuQuit,  MainWindow::mnuClose)
EVT_MENU(MainWindow::idMenuOuvrir, MainWindow::mnuOuvrir)
EVT_MENU(MainWindow::idMenuConfig, MainWindow::mnuConfiguration)
EVT_MENU(MainWindow::idMenuAbout, MainWindow::mnuAbout)
END_EVENT_TABLE()

/**
Point d'entrée
*/
bool MyApp::OnInit()
{
	try 
	{
		// Vérifie les arguments
		/*if (this->argc != 2)
		throw "Please open with a ROM file.";*/
		// Création de la fenêtre principale
		MainWindow *frame = new MainWindow();
		frame->Show(true);
		return true;
	}
	catch (const char *error) 
	{
		wxMessageBox(S(error), S("Error"));
		return false;
	}
}

/**
Constructeur

\param fileName Le fichier contenant la ROM
*/
MainWindow::MainWindow()
: wxFrame(NULL, wxID_ANY, S("HEIG-Boy"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE)
{
	//Ecran noir au démarage
	for (int i=0; i<144 * 160; ++i)
		lcdPixels[i]= 0;

	currentSlot = 0;

	pauseGame = false;

	//Création des menus
	menuFile = new wxMenu();
	menuFile->Append(idMenuOuvrir, _T("&Open ROM...\tCtrl+O"), _T("Load a ROM"));
	menuFile->Append(idMenuConfig, _T("&Config...\tCtrl+P"), _T("Change the keys configuration"));
	menuFile->AppendSeparator();
	menuFile->Append(idMenuQuit, _T("&Quit\tCtrl+Q"), _T("Quit heig-boy"));

	menuAbout = new wxMenu();
	menuAbout->Append(idMenuAbout, _T("&About\tF1"), _T("Show about dialog"));

	//Création de la barre de menu
	menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _T("&File"));
	menuBar->Append(menuAbout, _T("?"));

	//Ajoute la barre de menu à la fenêtre
	SetMenuBar(menuBar);

	//Création de la barre de status
	myStatusBar = CreateStatusBar();
	
	//TODO
	//Vérifier si le fichier de config existe, si c'est le cas,
	//récupérer la configuration des touches
	keys = new KeysMap();

	keys->load("config.cfg");

	refreshManager = new RefreshManager(this);
	SetClientSize(160 * 2, 144 * 2);

}

void MainWindow::onPaint(wxPaintEvent& event)
{
#ifdef WIN32
	// Code rapide pour Windows
	wxPaintDC dcPaint(this);
	wxBitmap bitmap((char*)lcdPixels, 160, 144, 32);
	// Dessine l'écran sur toute la surface
	wxRect rect(GetClientRect());
	dcPaint.SetUserScale(rect.width / 160.0, rect.height / 144.0);
	dcPaint.DrawBitmap(bitmap, 0, 0);
#else
	// Code lent pour Linux, ne supportant pas les bitmaps 32 bits
	wxPaintDC dcPaint(this);
	wxRect rect(GetClientRect());
	wxImage img(160, 144);
	unsigned char *data = img.GetData();
	u32 *lcd = lcdPixels;
	for (int j = 0; j < 144; j++)
		for (int i = 0; i < 160; i++) {
			*data++ = *lcd >> 16 & 0xff;
			*data++ = *lcd >> 8 & 0xff;
			*data++ = *lcd++ & 0xff;
		}
		// Dessine l'écran sur toute la surface
		dcPaint.SetUserScale(rect.width / 160.0, rect.height / 144.0);
		dcPaint.DrawBitmap(wxBitmap(img), 0, 0);
#endif
}

void MainWindow::onSizing(wxSizeEvent& event) {
	wxRect clientRect(GetClientRect());
	int scaleX = (clientRect.width + 80) / 160,
		scaleY = (clientRect.height + 72) / 144;
	SetClientSize(160 * scaleX, 144 * scaleY);
}

/**
Gestion générale des touches

\param code Code de la touche présée
\param state Permet de savoir si la touche 
**/
void MainWindow::keyEvent(long code, bool state) 
{
	if (keys->isButton(KeysMap::keyUp, code))
	{
		io_key_press(GBK_UP, state);
	}
	if (keys->isButton(KeysMap::keyDown, code))
	{
		io_key_press(GBK_DOWN, state);
	}
	if (keys->isButton(KeysMap::keyLeft, code) )
	{
		io_key_press(GBK_LEFT, state);
	}
	if (keys->isButton(KeysMap::keyRight, code) )
	{
		io_key_press(GBK_RIGHT, state);
	}
	if (keys->isButton(KeysMap::keyA, code) )
	{
		io_key_press(GBK_A, state);
	}
	if (keys->isButton(KeysMap::keyB, code) )
	{
		io_key_press(GBK_B, state);
	}
	if (keys->isButton(KeysMap::keyStart, code) )
	{
		io_key_press(GBK_START, state);
	}
	if (keys->isButton(KeysMap::keySelect, code) )
	{
		io_key_press(GBK_SELECT, state);
	}

	
}

/**
Appelé lorsqu'une touche est enfoncée.

\param L'évênement lié à la touche
*/
void MainWindow::onKeyDown(wxKeyEvent& event) 
{
	// Touches spéciales
	if (keys->isButton(KeysMap::keyPause, event.m_keyCode))
	{
		pauseGame = !pauseGame;
		refreshManager->pause(pauseGame);
	}
	else if (keys->isButton(KeysMap::keySaveState, event.m_keyCode)) 
	{
		// Un save state n'est pas cohérent en milieu de frame...
		refreshManager->mutexInFrame.Lock();
		save_state(currentSlot);
		save_sram();
		refreshManager->mutexInFrame.Unlock();
		myStatusBar->SetStatusText(_("Etat sauvegarde"));
	}
	else if(keys->isButton(KeysMap::keyLoadState,event.m_keyCode))
	{
		// Pareil pour le chargement
		refreshManager->mutexInFrame.Lock();
		if (load_state(currentSlot))
			myStatusBar->SetStatusText(_("Etat precedent charge"));
		else
			myStatusBar->SetStatusText(_("Slot vide"));
		refreshManager->mutexInFrame.Unlock();
		
	}
	else if(keys->isButton(KeysMap::keyTurbo, event.m_keyCode))
	{
		refreshManager->turbo(true);
	}
	else if(keys->isButton(keys->keyIncSlot, event.m_keyCode))
	{
		this->currentSlot = (currentSlot + 1) %10;
		myStatusBar->SetStatusText(wxString::Format("Slot courant : %d", currentSlot));
	}
	else if(keys->isButton(keys->keyDecSlot, event.m_keyCode))
	{
		if (--currentSlot < 0)
			currentSlot = 9;
		myStatusBar->SetStatusText(wxString::Format("Slot courant : %d", currentSlot));
	}
	else //Touche de la GB
	{
		keyEvent(event.m_keyCode, true);
	}
}

/**
Appelé lorsqu'une touche est relachée

\param event L'évênement lié à la touche
*/
void MainWindow::onKeyUp(wxKeyEvent& event) 
{
	if(keys->isButton(KeysMap::keyTurbo, event.m_keyCode))
	{
		refreshManager->turbo(false);
	}
	else
	{
		keyEvent(event.m_keyCode, false);
	}
}

bool MainWindow::testQuit()
{
	if (refreshManager->isPlaying())
	{
		return wxMessageBox(_("Really exit heig-boy ?"),
                                      wxT("Confirmation"),
									  wxYES_NO | wxICON_QUESTION) == wxYES;
	}

	//Pas de question si on ne joue pas
	return true;
}

/**
Appelé lorsque la fenêtre se ferme.
*/
void MainWindow::onClose(wxCloseEvent &event) 
{
	//Peut-on annuler l'évènement
	if (event.CanVeto())
	{
		// TODO sauver la sram si modifiée
		if (!testQuit())
		{
			event.Veto();

			return;
			
		}
	}
	
	//Fin du thread d'affichage
	refreshManager->endGame();
	refreshManager->mutexRomLoaded.Unlock();
	refreshManager->pause(false);
	//Attend la fin du thread
	refreshManager->Wait();
	//Ferme l'interface principale
	Destroy();

}

void MainWindow::mnuClose(wxCommandEvent& WXUNUSED(event))
{
	if (testQuit())
	{
		//Fin du thread d'affichage
		refreshManager->endGame();
		refreshManager->mutexRomLoaded.Unlock();
		refreshManager->pause(false);
		//Attend la fin du thread
		refreshManager->Wait();
		//Ferme l'interface principale
		Destroy();
	}
}

/**
Permet de configurer les touches
*/
void MainWindow::mnuConfiguration(wxCommandEvent& WXUNUSED(event))
{
	refreshManager->pause(true);
	//Afficher la fenêtre de configuration
	(new ConfigDialog(this, *keys))->ShowModal();
	refreshManager->pause(false);
}

/**
Affiche la fenêtre about
*/
void MainWindow::mnuAbout(wxCommandEvent& WXUNUSED(event))
{
	refreshManager->pause(true);
	//Afficher la fenêtre A propos
	wxAboutDialogInfo info;
	info.SetName(_("HEIG-BOY Emulateur Game-Boy"));
	info.SetDescription(_("Emulateur de la célèbre console Nintendo Game-Boy."));
	info.SetCopyright(_T("(C) 2010 Nicolas Blanchard, Florian Brönniman, David Puippe, Raphael Plomb, Julien Rinaldini"));

	info.AddDeveloper(_T("Nicolas Blanchard"));
	info.AddDeveloper(_T("Florianr Brunniman"));
	info.AddDeveloper(_T("Raphael Plomb"));
	info.AddDeveloper(_T("Julien Rinalidini"));

	//Afficher About
	wxAboutBox(info);

	refreshManager->pause(false);
}

/**
Permet d'ouvrir une ROM en la cherchant sur l'ordinateur
*/
void MainWindow::mnuOuvrir(wxCommandEvent& WXUNUSED(event))
{
	refreshManager->loadRom();	
}
