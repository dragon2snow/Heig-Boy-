#include "common.h"
#include "cpu.h"
#include "emu.h"
#include "io.h"
#include "mbc.h"
#include "mem.h"
#include "save.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char *header = "HBSSv1.0";

/** Crée un nouveau nom avec une extension différente.
	\param dest chaîne avec le nom de fichier résultant
	\param source chaîne avec le nom de fichier d'origine
	\param new_ext nouvelle extension, avec le point
*/
static void change_ext(char *dest, const char *source, const char *new_ext) {
	int i;
	strcpy(dest, source);
	for (i = strlen(dest) - 1;			// Trouve le dernier point
		i > 0 && dest[i] != '.';		// (pour l'extension)
		i--);
	if (i > 0)
		strcpy(dest + i, new_ext);
}

void load_sram() {
	FILE *f;
	char sram_name[256];
	// Détermine le nom du fichier SRAM
	change_ext(sram_name, emu_file_name, ".sav");
	f = fopen(sram_name, "rb");
	if (f) {
		// Lit la SRAM dans un tampon temporaire
		u8 tmp_buf[32768];
		unsigned size = fread(tmp_buf, 1, sizeof(tmp_buf), f);
		mbc_set_sram_data(tmp_buf, size);
		fclose(f);
	}
}

void save_sram() {
	FILE *f;
	char sram_name[256];
	// Détermine le nom du fichier SRAM
	change_ext(sram_name, emu_file_name, ".sav");
	f = fopen(sram_name, "wb");
	if (f) {
		// Récupère la SRAM dans un tampon temporaire
		u8 tmp_buf[32768];
		unsigned size = mbc_get_sram_data(tmp_buf, sizeof(tmp_buf));
		fwrite(tmp_buf, size, 1, f);
		fclose(f);
	}
}

void save_state(int slot) {
	// Ecrit un fichier .st0
	FILE *f;
	char st_name[256];
	change_ext(st_name, emu_file_name, ".st0");
	f = fopen(st_name, "wb");
	if (f) {
		// Ecriture des différents champs
		u8 tmp_buf[32768];
		unsigned size;
		// 0) En-tête
		fwrite(header, strlen(header), 1, f);
		// 1) Registres CPU
		size = cpu_get_state(tmp_buf);
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 2) RAM
		size = mem_get_data(tmp_buf, MEM_RAM, sizeof(tmp_buf));
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 3) VRAM
		size = mem_get_data(tmp_buf, MEM_VRAM, sizeof(tmp_buf));
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 4) Ports
		size = mem_get_data(tmp_buf, MEM_IO, sizeof(tmp_buf));
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 5) OAM
		size = mem_get_data(tmp_buf, MEM_OAM, sizeof(tmp_buf));
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 6) SRAM (sauvegarde)
		size = mbc_get_sram_data(tmp_buf, sizeof(tmp_buf));
		fwrite(&size, sizeof(size), 1, f);
		fwrite(tmp_buf, size, 1, f);
		// 7) Etat du MBC
		size = sizeof(mbc_params_t);
		fwrite(&size, sizeof(size), 1, f);
		fwrite(mbc_get_params(), size, 1, f);
		fclose(f);
	}
}

void load_state(int slot) {
	// Lit un fichier .st0
	FILE *f;
	char st_name[256];
	change_ext(st_name, emu_file_name, ".st0");
	f = fopen(st_name, "rb");
	if (f) {
		// Lecture des différents champs
		// Même schéma que #save_state
		u8 tmp_buf[32768];
		unsigned size, i;
		// 0) En-tête
		fread(tmp_buf, strlen(header), 1, f);
		// L'en-tête correspond-t-il?
		if (!memcmp(tmp_buf, header, strlen(header))) {
			// 1) Registres CPU
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			cpu_set_state(tmp_buf);
			// 2) RAM
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			mem_set_data(tmp_buf, MEM_RAM, size);
			// 3) VRAM
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			mem_set_data(tmp_buf, MEM_VRAM, size);
			// 4) Ports
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			mem_set_data(tmp_buf, MEM_IO, size);
			// Signale aux composants qu'ils ont changé
			for (i = 0; i <= 0x4B; i++)
				if (i != R_DMA)		// Sauf le DMA (lancerait une copie!)
					io_write(i, tmp_buf[i]);
			// 5) OAM
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			mem_set_data(tmp_buf, MEM_OAM, size);
			// 6) SRAM (sauvegarde)
			fread(&size, sizeof(size), 1, f);
			fread(tmp_buf, size, 1, f);
			mbc_set_sram_data(tmp_buf, size);
			// 7) Etat du MBC
			fread(&size, sizeof(size), 1, f);
			size = min(size, sizeof(mbc_params_t));
			fread(mbc_get_params(), 1, size, f);
		}
		fclose(f);
	}
}
