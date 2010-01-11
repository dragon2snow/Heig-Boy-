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
				  keyTurbo,
				  keyIncSlot,
				  keyDecSlot
	};

	static const int nbButtons = 14;

	//Les codes des boutons bindés
	long keyMap[nbButtons];

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

	/**
	Permet de sauvegarder les touches dans un fichier CFG.
	\param fileName Le nom du fichier
	*/
	void save(const char* fileName);
	
	/**
	Permet de charger une config de touches depuis un fichier
	CFG.
	\param fileName Le nom du fichier
	*/
	void load(const char* fileName);
};
