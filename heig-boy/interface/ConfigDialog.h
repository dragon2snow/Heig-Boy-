/**
 Classe permettant de configurer les touches.
 Inspir�e de la classe ayant le m�me fonctionnement
 dans l'application Nipples.
 */

#pragma once

#include "wx/wx.h"
#include "KeysMap.h"

class ConfigDialog : public wxDialog
{
	DECLARE_CLASS(ConfigDialog)
    DECLARE_EVENT_TABLE()

private:

	//Identifiants des objets de la fen�tre
	enum {
		idBtnSave = 100,
		idBtnDefault,
		idBtnCancel
	};
	
	//Pour que les changements restes locaux
	KeysMap local;
	//Lien la config du parent
	KeysMap &mapParent;

	//Les boutons de jeux
	wxButton* buttons[12];
	//Bouton pour sauver les modif
	wxButton* btnSave;
	//Bouton permettant de remettre les touches par d�faut
	wxButton* btnDefault;
	//Bouton permettant d'annuler les modifications
	wxButton* btnCancel;

	/**
	 Pr�pare la fen�tre.
	 */
	void init();

	/**
	 Appel� lorsque l'utilisateur appuie sur le
	 bouton 'Save'
	 \param event Ev�nement correspondant
	 */
	void onSaveButton(wxCommandEvent &event);

	/**
	 Appel� lorsque l'utilisateur presse sur le
	 bouton 'Reset'
	 \param event Ev�nement correspondant
	 */
	void onDefaultButton(wxCommandEvent &event);

	/**
	 Appel� lorsque l'utilisateur presse sur le
	 bouton 'Cancel'
	 \param event Ev�nement correspondant
	 */
	void onCancelButton(wxCommandEvent &event);

	/**
	 Appl� lorsqu'un des boutons configurable est
	 press�.
	 \param event Ev�nement correspondant
	 */
	void onConfigurableButton(wxCommandEvent &event);

	/**
	 Affiche la valeur actuelle des touches sur les boutons.
	 */
	void showValues();

public:
	/**
	 Constructeur.
	 Construit une fen�tre permettant de modifier
	 la configuration des touches.
	 \param parent La fen�tre m�re
	 \param keys Permet au parent de de r�cup�rer 
				les nouvelles touches.
	 */
	ConfigDialog(wxWindow* parent, KeysMap& keys);

	/**
	 Permet d'afficher la fen�tre (redef)
	 */
	int ShowModal();

	/**
	 Affiche la valeur actuelle des touches sur les boutons.
	 */
	void showButtonsValues();
};