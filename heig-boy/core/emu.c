#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sound.h"
#include "timer.h"
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
		lcd_init();
		sound_init();
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

#if 0
	void emu_do_frame() {
		int elapsed;
		while (1) {
			elapsed = cpu_exec_instruction() * 4;
			lcd_tick(elapsed);
		}
	}
#else
	#include <windows.h>
	void emu_do_frame() {
//		__int64 freq, val1, val2;
//		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//		QueryPerformanceCounter((LARGE_INTEGER*)&val1);
		while (1) {
			int elapsed = cpu_exec_instruction() * 4;
			lcd_tick(elapsed);
			timer_tick(elapsed);
		}
//		QueryPerformanceCounter((LARGE_INTEGER*)&val2);
//		printf("Time: %f\n", (double)(val2 - val1) / freq);
	}
#endif
