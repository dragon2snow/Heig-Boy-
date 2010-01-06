/**
 Classe repr�sentant une fen�tre de capture de touche clavier.
 Insipr�e de la classe du m�me nom de l'application Nipples.
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
	//Sauvegarde du code de la touche press�e
	long keyCode;

	/**
	 Appel�e lors de l'appui sur la croix de fermeture
	 \param event L'�v�nement g�n�r�
	 */
	void onWindowClose(wxCloseEvent &event);

public:
	/**
	Constructeur
	\param parent Le parent
	*/
	CaptureDialog(wxWindow* parent);

	/**
	Permet d'obtenir la touche pr�s�e par l'utilisateur.
	*/
	long getKeyPressed();

};