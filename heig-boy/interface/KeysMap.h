/**
 Repr�sente les boutons.
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
	 Permet de remettre les touches par d�faut
	 */
	void reset();

	/**
	 Permet de savoir si la touche est bind� sur un
	 bouton sp�cifique.
	 */
	bool isButton(enum Buttons button, long code) const;


private:
	//Les codes des boutons bind�s
	long keyMap[11];


};