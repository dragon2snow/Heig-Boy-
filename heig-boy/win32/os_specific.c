/* Win32-specific things */
#include "os_specific.h"
#include <windows.h>

void set_text_color(int c) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

