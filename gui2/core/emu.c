#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sound.h"
#include "timer.h"
#include "mbc.h"
#include "save.h"
#include "io.h"			// temp
#include <string.h>		// strcpy
#include <stdio.h>		// file
#include <stdlib.h>		// min, max

char emu_file_name[256];

/** Détermine la taille du fichier et met le curseur de lecture au début
	\param f flux (fichier) ouvert
	\return taille du fichier en octets
*/
static int get_file_size(FILE *f) {
	int size;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	return size;
}

int emu_load_cart(const char *file_name) {
   FILE *f;

   if(f != NULL)
       close(f);
   f = fopen(file_name, "rb");

	if (f) {
		// Détermine la taille du fichier
		int size = get_file_size(f);
		strcpy(emu_file_name, file_name);
      // Garde pour les sauvegardes d'état
      // Au moins un tableau couvrant la map 0000-7FFF...
		size = max(size, 0x8000);
		// Et multiple d'une page (arrondissement à la page supérieure)
		if (size % 0x4000) {
			size &= 0x3FFF;
			size += 0x4000;
		}
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
		return 0;
	}
	else
		return -1;
}

#if linux
	void emu_do_frame() {
      lcd_frame_end_flag = false;
      while (!lcd_frame_end_flag) {
         unsigned elapsed = cpu_exec_instruction() * 4;
         lcd_tick(elapsed);
         timer_tick(elapsed);
		}
	}
#else
	#include <windows.h>
	#include <conio.h>
	extern HWND hwnd;
	void emu_do_frame() {
		lcd_frame_end_flag = false;
		while (!lcd_frame_end_flag) {
			unsigned elapsed = cpu_exec_instruction() * 4;
			lcd_tick(elapsed);
			timer_tick(elapsed);
		}

		// DEBUG: gestion des touches
		if (GetForegroundWindow() == hwnd) {
			io_key_press(GBK_RIGHT, GetAsyncKeyState('F')? 1:0);
			io_key_press(GBK_LEFT, GetAsyncKeyState('A')? 1:0);
			io_key_press(GBK_UP, GetAsyncKeyState('E')? 1:0);
			io_key_press(GBK_DOWN, GetAsyncKeyState('C')? 1:0);
			io_key_press(GBK_A, GetAsyncKeyState('K')? 1:0);
			io_key_press(GBK_B, GetAsyncKeyState('J')? 1:0);
			io_key_press(GBK_SELECT, GetAsyncKeyState(' ')? 1:0);
			io_key_press(GBK_START, GetAsyncKeyState(VK_RETURN)? 1:0);
			if (GetAsyncKeyState(VK_F5))
				save_state(0);
			if (GetAsyncKeyState(VK_F8))
				load_state(0);
			if (GetAsyncKeyState(VK_F9))
				save_sram();
			if (GetAsyncKeyState(VK_F12)) {
				FILE *f = fopen("C:\\dump.vram", "wb");
				fwrite(mem_vram, 8192, 1, f);
				fclose(f);
			}
		}
	}
#endif
