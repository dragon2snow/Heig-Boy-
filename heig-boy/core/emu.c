#include "emu.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sound.h"
#include "timer.h"
#include "mbc.h"
#include "io.h"			// temp
#include <string.h>		// strcpy
#include <stdio.h>
#include <stdlib.h>

static char rom_file_name[256];

/** D�termine la taille du fichier et met le curseur de lecture au d�but
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

/** Cr�e un nouveau nom avec une extension diff�rente.
	\param dest cha�ne avec le nom de fichier r�sultant
	\param source cha�ne avec le nom de fichier d'origine
	\param new_ext nouvelle extension, avec le point
*/
static void change_ext(char *dest, const char *source, const char *new_ext) {
	int i;
	strcpy(dest, source);
	for (i = strlen(dest) - 1;			// Trouve le dernier point
		i > 0 && dest[i] != '.';		// (pour l'extension)
		i--);
	if (i > 0)
		strcpy(dest + i, ".sav");
}

/** Essaie de charger la SRAM associ�e au fichier image charg� */
static void load_sram() {
	FILE *f;
	char sram_name[256];
	// D�termine le nom du fichier SRAM
	change_ext(sram_name, rom_file_name, ".sav");
	f = fopen(sram_name, "rb");
	if (f) {
		// Lit la SRAM dans un tampon temporaire
		u8 tmp_buf[32768];
		// V�rifie de ne pas d�passer...
		int size = min(get_file_size(f), 32768);
		fread(tmp_buf, 1, size, f);
		mbc_set_sram_data(tmp_buf, size);
		fclose(f);
	}
}

/** Sauvegarde la SRAM associ�e au logiciel en cours d'ex�cution */
static void save_sram() {
	FILE *f;
	char sram_name[256];
	// D�termine le nom du fichier SRAM
	change_ext(sram_name, rom_file_name, ".sav");
	f = fopen(sram_name, "wb");
	if (f) {
		// R�cup�re la SRAM dans un tampon temporaire
		u8 tmp_buf[32768];
		int size = mbc_get_sram_data(tmp_buf, sizeof(tmp_buf));
		fwrite(tmp_buf, size, 1, f);
		fclose(f);
	}
}

int emu_load_cart(const char *file_name) {
	FILE *f = fopen(file_name, "rb");
	if (f) {
		// D�termine la taille du fichier
		int size = get_file_size(f);
		// Garde pour les sauvegardes d'�tat
		strcpy(rom_file_name, file_name);
		// Au moins un tableau couvrant la map 0000-7FFF...
		size = max(size, 0x8000);
		mem_rom = malloc(size);
		// La cartouche pr�par�e, pr�pare le CPU
		cpu_init();
		lcd_init();
		sound_init();
		mem_init();
		// Charge le contenu
		fread(mem_rom, size, 1, f);
		fclose(f);
		// D�marrage
		mbc_init(size);
		return 0;
	}
	else
		return -1;
}

#if 1
	void emu_do_frame() {
		int elapsed;
		while (1) {
			elapsed = cpu_exec_instruction() * 4;
			lcd_tick(elapsed);
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
		}
	}
#endif
