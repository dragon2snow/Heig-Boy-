/**
Repr�sente les boutons.
*/

#pragma once

#include "wx/wx.h"

#include <istream>
#include <ostream>



class KeysMap
{
	friend class ConfigDialog;
	friend std::ostream& operator<<(std::ostream &, const KeysMap &);
	friend std::istream& operator>>(std::istream &, KeysMap &);

public:
	//Ordonnancement des boutons
	enum Buttons {keyUp, 
			      keyDown, 
				  keyLeft, 
				  keyRight, 
				  keyA, 
				  keyB, 
				  keyStart, 
				  keySelect, 
				  keyPause, 
				  keySaveState, 
				  keyLoadState, 
				  keyTurbo};

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
	
	/**
	Redef de l'affectation
	\param keys La KeysMap � copier
	*/
	void copyFromMap(const KeysMap& keys);

	//Les codes des boutons bind�s
	long keyMap[12];
};

/**
Permet d'�crire un objet KeysMap dans un flux ostream
\param os Le flux de sortie
\parma keys La KeysMap � �crire
*/
std::ostream& operator<<(std::ostream &os, const KeysMap &keys);

/**
Permet d'initialiser une KeysMap depuis un flux d'entr�e

\param is Le flux
\param keys La KeysMap � initialiser
*/
std::istream& operator>>(std::istream &is, KeysMap &keys);

