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
	keyMap[keyA] = WXK_RETURN;
	keyMap[keyB] = WXK_SHIFT;
	keyMap[keyStart] = WXK_SPACE;
	keyMap[keySelect] = WXK_BACK;
	keyMap[keyPause] = (wxKeyCode)'P';
	keyMap[keySaveState] = WXK_F5;
	keyMap[keyLoadState] = WXK_F6;
}

bool KeysMap::isButton(KeysMap::Buttons button, long code) const
{
	return keyMap[button] == code;
}
