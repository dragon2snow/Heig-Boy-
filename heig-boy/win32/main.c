#include <windows.h>
#include <stdio.h>
#include "emu.h"

int main(int argc, char *argv[]) {
	emu_load_cart("tetris.gb");
	emu_do_frame();
}
