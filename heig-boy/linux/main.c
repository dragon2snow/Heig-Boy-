#include <stdio.h>
#include "emu.h"

#define SCREEN_WIDTH  160*2
#define SCREEN_HEIGHT 144*2

char *pixels;

int main(int argc, char *argv[]) {
	emu_load_cart("testprog.gb");
	emu_do_frame();
	return 0;
}
