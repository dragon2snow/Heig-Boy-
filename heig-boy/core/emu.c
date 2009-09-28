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
		// D�termine la taille du fichier
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		// Au moins un tableau couvrant la map 0000-7FFF...
		mem_rom = malloc(max(size, 0x8000));
		// La cartouche pr�par�e, pr�pare le CPU
		cpu_init();
		lcd_init();
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

// Pr�charge un programme pourri non interpr�t�
#include "ports.h"
void cheat() {
	unsigned i;
	for (i = 0; i < 5 * 16; i++)
		mem_writeb(0x8000 + i, mem_readb(0x196 + i));
	mem_writeb(0xff00 + R_SCX, -50);
	mem_writeb(0xff00 + R_SCY, -50);
	mem_writeb(0x9800, 1);
	mem_writeb(0x9801, 2);
	mem_writeb(0x9802, 3);
	mem_writeb(0x9803, 3);
	mem_writeb(0x9804, 4);
}

void emu_do_frame() {
	int elapsed;

	cheat();
	while (1) {
		elapsed = cpu_exec_instruction() * 4;
//		elapsed = 4;
//		lcd_tick(elapsed);
	}
}

