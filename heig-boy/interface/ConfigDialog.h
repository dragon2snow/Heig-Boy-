/**
 Classe permettant de configurer les touches.
 Inspirée de la classe ayant le même fonctionnement
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

	//Identifiants des objets de la fenêtre
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
	//Bouton permettant de remettre les touches par défaut
	wxButton* btnDefault;
	//Bouton permettant d'annuler les modifications
	wxButton* btnCancel;

	/**
	 Prépare la fenêtre.
	 */
	void init();

	/**
	 Appelé lorsque l'utilisateur appuie sur le
	 bouton 'Save'
	 \param event Evénement correspondant
	 */
	void onSaveButton(wxCommandEvent &event);

	/**
	 Appelé lorsque l'utilisateur presse sur le
	 bouton 'Reset'
	 \param event Evénement correspondant
	 */
	void onDefaultButton(wxCommandEvent &event);

	/**
	 Appelé lorsque l'utilisateur presse sur le
	 bouton 'Cancel'
	 \param event Evénement correspondant
	 */
	void onCancelButton(wxCommandEvent &event);

	/**
	 Applé lorsqu'un des boutons configurable est
	 pressé.
	 \param event Evénement correspondant
	 */
	void onConfigurableButton(wxCommandEvent &event);

	/**
	 Affiche la valeur actuelle des touches sur les boutons.
	 */
	void showValues();

public:
	/**
	 Constructeur.
	 Construit une fenêtre permettant de modifier
	 la configuration des touches.
	 \param parent La fenêtre mère
	 \param keys Permet au parent de de récupérer 
				les nouvelles touches.
	 */
	ConfigDialog(wxWindow* parent, KeysMap& keys);

	/**
	 Permet d'afficher la fenêtre (redef)
	 */
	int ShowModal();

	/**
	 Affiche la valeur actuelle des touches sur les boutons.
	 */
	void showButtonsValues();
};