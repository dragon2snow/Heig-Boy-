#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>

int emu_load_cart(const char *file_name) {
	FILE *f = fopen(file_name, "rb");
	int size;
	if (f) {
		// Détermine la taille du fichier
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		// Au moins un tableau couvrant la map 0000-7FFF...
		mem_rom = malloc(max(size, 0x8000));
		// La cartouche préparée, prépare le CPU
		cpu_init();
		mem_init();
		// Charge le contenu
		fseek(f, 0, SEEK_SET);
		fread(mem_rom, size, 1, f);
		fclose(f);
		return 0;
	}
	else
		return -1;
}

void emu_do_frame() {
	int elapsed;
/*	char buf[256];
	int len, time;
	cpu_disassemble(0x100, buf, &len, &time);
	printf("%s\n", buf);*/

	while (1) {
		elapsed = cpu_exec_instruction();
		lcd_tick(elapsed);
	}
}

