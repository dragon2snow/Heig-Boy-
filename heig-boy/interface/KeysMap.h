/**
 Représente les boutons.
 */

#pragma once

#include "wx/wx.h"

class KeysMap
{
public:
	//Ordonnancement des boutons
	enum Buttons {keyUp, keyDown, keyLeft, keyRight, keyA, keyB, keyStart, keySelect, 
		keyPause, keySaveState, keyLoadState};

	/**
	 Constructeur
	 */
	KeysMap();

	/**
	 Permet de remettre les touches par défaut
	 */
	void reset();

	/**
	 Permet de savoir si la touche est bindé sur un
	 bouton spécifique.
	 */
	bool isButton(enum Buttons button, long code) const;


private:
	//Les codes des boutons bindés
	long keyMap[11];


};