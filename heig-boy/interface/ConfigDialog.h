/**
 Classe permettant de configurer les touches.
 */

#pragma once

#include "wx/wx.h"

class ConfigDialog : public wxDialog
{
	/**
	 Constructeur
	 */
	ConfigDialog(MainWindow* parent, wxKeyCode* keys);


	DECLARE_CLASS(ConfigDialog)
    DECLARE_EVENT_TABLE()