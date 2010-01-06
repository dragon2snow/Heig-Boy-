/**
Représente les boutons.
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
	Permet de remettre les touches par défaut
	*/
	void reset();

	/**
	Permet de savoir si la touche est bindé sur un
	bouton spécifique.
	*/
	bool isButton(enum Buttons button, long code) const;
	
	/**
	Redef de l'affectation
	\param keys La KeysMap à copier
	*/
	void copyFromMap(const KeysMap& keys);

	//Les codes des boutons bindés
	long keyMap[12];
};

/**
Permet d'écrire un objet KeysMap dans un flux ostream
\param os Le flux de sortie
\parma keys La KeysMap à écrire
*/
std::ostream& operator<<(std::ostream &os, const KeysMap &keys);

/**
Permet d'initialiser une KeysMap depuis un flux d'entrée

\param is Le flux
\param keys La KeysMap à initialiser
*/
std::istream& operator>>(std::istream &is, KeysMap &keys);

