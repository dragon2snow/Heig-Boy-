#include "KeysMap.h"


KeysMap::KeysMap()
{
	KeysMap::reset();
}

void KeysMap::reset()
{
	//Touches par défaut
	keyMap[keyUp] = (wxKeyCode)'W';
	keyMap[keyDown] = (wxKeyCode)'S';
	keyMap[keyLeft] = (wxKeyCode)'A';
	keyMap[keyRight] = (wxKeyCode)'D';
	keyMap[keyA] = (wxKeyCode)'K';
	keyMap[keyB] = (wxKeyCode)'L';
	keyMap[keyStart] = WXK_RETURN;
	keyMap[keySelect] = WXK_BACK;
	keyMap[keyPause] = (wxKeyCode)'P';
	keyMap[keySaveState] = WXK_F5;
	keyMap[keyLoadState] = WXK_F6;
	keyMap[keyTurbo] = (wxKeyCode)'T';
}

bool KeysMap::isButton(KeysMap::Buttons button, long code) const
{
	return keyMap[button] == code;
}

void KeysMap::copyFromMap(const KeysMap& keys)
{
	for (int i=0; i<12; ++i)
	{
		this->keyMap[i] = keys.keyMap[i];
	}

}

std::ostream& operator << (std::ostream& os, KeysMap& keys)
{
	//Ecriture de la valeur de tous les boutons
	for (int i=0; i<12; ++i)
		os << keys.keyMap[i] << ' ';
	

	return os;

}

std::istream& operator >> (std::istream& is, KeysMap& keys)
{
	//Lecture de la valeur de tous les boutons
	for (int i=0; i<12; ++i)
		is >> keys.keyMap[i];

	return is;
}
