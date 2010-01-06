/**
 Classe représentant une fenêtre de capture de touche clavier.
 Insiprée de la classe du même nom de l'application Nipples.
 */

#pragma once

class CaptureDialog :public wxDialog
{
	friend class CapturePanel;

	DECLARE_CLASS(CaptureDialog)
    DECLARE_EVENT_TABLE()
	
private:
	//Panel d'affichage
	wxPanel *panel;
	//Sauvegarde du code de la touche pressée
	long keyCode;

	/**
	 Appelée lors de l'appui sur la croix de fermeture
	 \param event L'événement généré
	 */
	void onWindowClose(wxCloseEvent &event);

public:
	/**
	Constructeur
	\param parent Le parent
	*/
	CaptureDialog(wxWindow* parent);

	/**
	Permet d'obtenir la touche présée par l'utilisateur.
	*/
	long getKeyPressed();

};