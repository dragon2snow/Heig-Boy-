#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sound.h"
#include "timer.h"
#include "mbc.h"
#include "save.h"
#include "../color-it/user.h"
#include <string.h>		// strcpy
#include <stdio.h>		// file
#include <stdlib.h>		// min, max

char emu_file_name[256];

/** Détermine la taille du fichier et met le curseur de lecture au début
	\param f flux (fichier) ouvert
	\return taille du fichier en octets
*/
static unsigned get_file_size(FILE *f) {
	unsigned size;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	return size;
}

bool emu_load_cart(const char *file_name) {
	FILE *f = fopen(file_name, "rb");
	if (f) {
		// Détermine la taille du fichier
		unsigned size = get_file_size(f);
		// Garde pour les sauvegardes d'état
		strcpy(emu_file_name, file_name);
		// Au moins un tableau couvrant la map 0000-7FFF...
		size = max(size, 0x8000);
		// Et multiple d'une page (arrondissement à la page supérieure)
		size = ((size + 0x3FFF) / 0x4000) * 0x4000;
		mem_rom = malloc(size);
		// La cartouche préparée, prépare le CPU
		cpu_init();
		lcd_init();
		sound_init();
		mem_init();
		// Charge le contenu
		fread(mem_rom, size, 1, f);
		fclose(f);
		// Démarrage
		mbc_init(size);
		load_sram();
		// Color-It
		ColorIt_init(file_name, mem_rom);
		return true;
	}
	return false;
}

void emu_do_frame(bool draw) {
	// Exécute une frame
	lcd_frame_end_flag = false;
	while (!lcd_frame_end_flag) {
		unsigned elapsed = cpu_exec_instruction() * 4;
		lcd_tick(elapsed, draw);
		timer_tick(elapsed);
	}

	// ColorIt: une demande de rechargement est effectuée
	if (ColorIt_reload) {
		ColorIt_systemInit();
		ColorIt_exitingLcdc(mem_vram);
		ColorIt_reload = 0;
	}
}
