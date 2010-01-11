#pragma once

#include "wx/wx.h"
#include "KeysMap.h"


extern "C" {
#include "../core/common.h"
}

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
	virtual bool OnInit();
};

class MainWindow : public wxFrame
{
	//Tableau rerpésentant le lcd
	u32 lcdPixels[144 * 160];

	//Le slot de sauvegarde courant
	int currentSlot;

	class RefreshManager *refreshManager;

	//Les touches du gameboy
	KeysMap *keys;
	
	//Identifiant des menus
	enum {
		idMenuQuit = 1,
		idMenuOuvrir,
		idMenuConfig,
		idMenuAbout
	};
	
	wxMenu* menuFile;
	wxMenu* menuAbout;
    wxMenuBar* menuBar;
	wxStatusBar* myStatusBar;

	bool pauseGame;
	
	/**
	 Demande une confirmation avant la fermeture lorsque
	 l'utilisateur joue.
	 \return La réponse de l'utilisateur. True s'il ne joue pas
	 */
	bool testQuit();

public:
	// Constructeur
	MainWindow();

	// Gestion des évênements
	void onPaint(wxPaintEvent& event);
	void onKeyDown(wxKeyEvent& event);
	void onKeyUp(wxKeyEvent& event);
	void onClose(wxCloseEvent& event);
	void mnuClose(wxCommandEvent& WXUNUSED(event));
	void mnuConfiguration(wxCommandEvent& WXUNUSED(event));
	void mnuAbout(wxCommandEvent& WXUNUSED(event));
	void mnuOuvrir(wxCommandEvent& WXUNUSED(event));

	void keyEvent(long keyCode, bool state);
	u32 *getPixels() { return lcdPixels; }

	// any class wishing to process wxWidgets events must use this macro
	DECLARE_EVENT_TABLE()
};
