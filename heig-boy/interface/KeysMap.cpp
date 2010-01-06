#include "KeysMap.h"
#include <fstream>
#include <iostream>


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

void KeysMap::save(const char* fileName)
{
	std::ofstream file(fileName);

	if (!file.bad())
	{
		for (int i=0; i<12; ++i)
		{
			file << keyMap[i] << ' ';
		}

		file << '\n';
		file.close();
	}
}

void KeysMap::load(const char* fileName)
{
	std::ifstream file(fileName);

	if (!file.bad())
	{
		for (int i=0; i<12; ++i)
		{
			file >> keyMap[i];
		}

		file.close();
	}
}
