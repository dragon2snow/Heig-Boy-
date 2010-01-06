
#include "wx/wx.h"

#include "ConfigDialog.h"
#include "CaptureDialog.h"

BEGIN_EVENT_TABLE(ConfigDialog, wxDialog)
EVT_BUTTON(KeysMap::keyUp, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyDown,ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyLeft, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyRight, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyA, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyB, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyStart, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keySelect, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyPause, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keySaveState, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyLoadState, ConfigDialog::onConfigurableButton)
	EVT_BUTTON(KeysMap::keyTurbo, ConfigDialog::onConfigurableButton)

	EVT_BUTTON(idBtnSave, ConfigDialog::onSaveButton)
	EVT_BUTTON(idBtnDefault, ConfigDialog::onDefaultButton)
	EVT_BUTTON(idBtnCancel, ConfigDialog::onCancelButton)
END_EVENT_TABLE()


ConfigDialog::ConfigDialog(wxWindow *parent, KeysMap &keys)
: mapParent(keys)
{
	//Création
	Create(parent, wxID_ANY, _("Input Settings"),wxDefaultPosition, wxDefaultSize ,
		wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX);

	//Mise en place de l'interface
	init();

	//Redimmensionement et centrage
	GetSizer()->SetSizeHints(this);
	Centre();
}

int ConfigDialog::ShowModal()
{
	//Copie la config courente
	local.copyFromMap(mapParent);

	showButtonsValues();

	return wxDialog::ShowModal();
}


void ConfigDialog::init()
{
	/*Adaptation de l'application Nipples*/

	//Dimmensionneur principal
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
	

	//------ Group box pour les boutons GB ------
    wxStaticBoxSizer *keyboardBoxSizer = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _("Game-Boy Keys")), wxVERTICAL);

	sizer->Add(keyboardBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	//Grid Sizer pour les boutons GB
	wxFlexGridSizer *keyboardGridSizer = new wxFlexGridSizer(8,7, 0, 0);
	keyboardBoxSizer->Add(keyboardGridSizer,0, wxALIGN_CENTER_HORIZONTAL | wxALL,0);

	//Ajout d'espace (vide)
	keyboardGridSizer->Add(5, 5, 0,wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Création et ajout du bouton 'Up'
	buttons[KeysMap::keyUp] = new wxButton(this, KeysMap::keyUp);
	keyboardGridSizer->Add(buttons[KeysMap::keyUp], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//6 cases vides avant le prochain objet
	for (int i=0; i<6; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Label 'Up'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Up")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//5 cases vides avant le prochain objet
	for (int i=0; i<5; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}
	
	//Création et ajout du bouton 'Left'
	buttons[KeysMap::keyLeft] = new wxButton(this, KeysMap::keyLeft);
	keyboardGridSizer->Add(buttons[KeysMap::keyLeft], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);
	
	//Un espace
	keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Création et ajout du bouton 'Right'
	buttons[KeysMap::keyRight] = new wxButton(this, KeysMap::keyRight);
	keyboardGridSizer->Add(buttons[KeysMap::keyRight], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//2 espaces jusqu'au prochain objet
	for (int i=0; i<2; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Création et ajout du bouton 'A'
	buttons[KeysMap::keyA] = new wxButton(this, KeysMap::keyA);
	keyboardGridSizer->Add(buttons[KeysMap::keyA], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//Création et ajout du bouton 'B'
	buttons[KeysMap::keyB] = new wxButton(this, KeysMap::keyB);
	keyboardGridSizer->Add(buttons[KeysMap::keyB], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//Ajout du label 'Left'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Left")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Un espace
	keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Label 'Right'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Right")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//2 espaces
	for (int i=0; i<2; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Label 'A'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("A")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Label 'B'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("B")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Espace
	keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Création et ajout du bouton 'Down'
	buttons[KeysMap::keyDown] = new wxButton(this, KeysMap::keyDown);
	keyboardGridSizer->Add(buttons[KeysMap::keyDown], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//6 espaces
	for (int i=0; i<6; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Label 'Down'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Down")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//8 espaces
	for (int i=0; i<8; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Création et ajout du bouton 'Select'
	buttons[KeysMap::keySelect] = new wxButton(this, KeysMap::keySelect);
	keyboardGridSizer->Add(buttons[KeysMap::keySelect], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//Création et ajout du bouton 'Start'
	buttons[KeysMap::keyStart] = new wxButton(this, KeysMap::keyStart);
	keyboardGridSizer->Add(buttons[KeysMap::keyStart], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL);

	//5 espaces
	for (int i=0; i<5; ++i)
	{
		keyboardGridSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);
	}

	//Label 'Select'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Select")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Label 'Start'
	keyboardGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Start")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);


	//------ Group Box pour les boutons spéciaux ------
    wxStaticBoxSizer *specialBoxSizer = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _("Specials Keys")), wxVERTICAL);

	sizer->Add(specialBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	//Grid Sizer pour les boutons GB
	wxFlexGridSizer *specialGridSizer = new wxFlexGridSizer(4, 2);
	specialBoxSizer->Add(specialGridSizer,0, wxALIGN_CENTER_HORIZONTAL | wxALL,0);

	//Création et ajout du bouton 'Turbo'
	buttons[KeysMap::keyTurbo] = new wxButton(this, KeysMap::keyTurbo);
	specialGridSizer->Add(buttons[KeysMap::keyTurbo], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Bouton 'Pause'
	buttons[KeysMap::keyPause] = new wxButton(this, KeysMap::keyPause);
	specialGridSizer->Add(buttons[KeysMap::keyPause], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Bouton 'Save State'
	buttons[KeysMap::keySaveState] = new wxButton(this, KeysMap::keySaveState);
	specialGridSizer->Add(buttons[KeysMap::keySaveState], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Bouton 'Load State'
	buttons[KeysMap::keyLoadState] = new wxButton(this, KeysMap::keyLoadState);
	specialGridSizer->Add(buttons[KeysMap::keyLoadState], 0, wxALIGN_CENTER_HORIZONTAL | 
                           wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//Label 'Turbo'
	specialGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Turbo")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Label 'Pause'
	specialGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Pause")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Label 'Save State'
	specialGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Save State")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);

	//Label 'Load State'
	specialGridSizer->Add(new wxStaticText(this, wxID_STATIC, _("Load State")),0,wxALIGN_CENTER_HORIZONTAL |
                           wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE);


	//-------Les boutons d'actions--------
	wxBoxSizer *actionsBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(actionsBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	//Bouton 'Default'
	btnDefault = new wxButton(this, idBtnDefault, _("De&fault"));
	actionsBoxSizer->Add(btnDefault, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 30);

	//Bouton 'Save'
	btnSave = new wxButton(this, idBtnSave, _("&Save"));
	actionsBoxSizer->Add(btnSave, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);

	//Bouton 'Cancel'
	btnCancel = new wxButton(this, idBtnCancel, _("&Cancel"));
	actionsBoxSizer->Add(btnCancel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 2);

}


void ConfigDialog::onSaveButton(wxCommandEvent &event)
{
	//Modif des touches du parent
	mapParent.copyFromMap(local);

	Close();
}

void ConfigDialog::onDefaultButton(wxCommandEvent &event)
{
	//Remise à zéro des touches
	local.reset();

	showButtonsValues();
}

void ConfigDialog::onCancelButton(wxCommandEvent &event)
{
	Close();
}

void ConfigDialog::onConfigurableButton(wxCommandEvent &event)
{
	CaptureDialog *capture;
	capture = new CaptureDialog(this);
	
	if (capture->ShowModal() == wxID_OK)
	{
		local.keyMap[event.GetId()] = capture->getKeyPressed();
		buttons[event.GetId()]->SetLabel(wxString::Format(_("%ld"), local.keyMap[event.GetId()]));
	}

	delete capture;

}

void ConfigDialog::showButtonsValues()
{
	for (int i=0; i<12; ++i)
	{
		buttons[i]->SetLabel(wxString::Format(_("%ld"), local.keyMap[i]));
	}

}

IMPLEMENT_CLASS(ConfigDialog, wxDialog)
