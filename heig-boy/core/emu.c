#include "emu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

int emu_load_cart(const char *file_name) {
	FILE *f = fopen(file_name, "rb");
	int size;
	if (f) {
		// Détermine la taille du fichier
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		// Charge le contenu
		mem_rom = malloc(size);
		fseek(f, 0, SEEK_SET);
		fread(mem_rom, size, 1, f);
		fclose(f);
		return 0;
	}
	else
		return -1;
}

void emu_do_frame() {
}

