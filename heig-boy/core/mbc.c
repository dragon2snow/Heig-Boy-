#include "mbc.h"
#include "mem.h"
#include "emu.h"
#include "debug.h"
#include <stdlib.h>		// min
#include <string.h>		// NULL

// Paramètres courants (banque, RAM active, etc.)
static mbc_params_t params;
// Fonctions utilisées pour la lecture / écriture, en fonction du MBC
u8 (*mbc_read)(u16 address) = NULL;
void (*mbc_write)(u16 address, u8 value) = NULL;
// Taille courante de la ROM et de la SRAM, pour ne pas dépasser
static u32 rom_size, ram_size;
// SRAM, utilisée pour la sauvegarde dans la cartouche, max. 32k
// On utilise le terme de RAM ici, à ne pas confondre avec la RAM de travail
// connectée au CPU. Celle-ci est dans la cartouche (externe).
static u8 ram_data[0x8000];

/** Définit la banque ROM courante en faisant attention à ne pas dépasser
	de l'image ROM chargée.
	\param bank_no n° de banque (1 banque = 16 ko)
*/
static void set_rom_bank(u8 bank_no) {
	// FIXME modulo et division lentes
	params.rom_bank = bank_no % (rom_size / 0x4000);
}

/** Définit la banque RAM courante.
	\param bank_no n° de banque (1 banque = 8 ko)
*/
static void set_ram_bank(u8 bank_no) {
	// FIXME modulo et division lentes
	if (ram_size >= 0x2000)		// Eviter le modulo zéro
		params.ram_bank = bank_no % (ram_size / 0x2000);
}

// Lecture directe, pas de MBC
static u8 direct_read(u16 address) {
	return mem_rom[address];
}

// ROM simple (écriture désactivée)
static void null_write(u16 address, u8 value) {}

// Lecture en SRAM
u8 mbc_sram_read(u16 address) {
	return ram_data[params.ram_bank * 8192 + (address - 0xA000)];
}

// Ecriture en SRAM
void mbc_sram_write(u16 address, u8 value) {
	if (params.ram_enable)
		ram_data[params.ram_bank * 8192 + (address - 0xA000)] = value;
}

// MBC1
static u8 mbc1_read(u16 address) {
	if (address < 0x4000)
		return mem_rom[address];
	else		// Tient compte de la banque
		return mem_rom[params.rom_bank * 0x4000 + (address & 0x3fff)];
}

static void mbc1_write(u16 address, u8 value) {
	switch (address >> 13) {
		case 0x0:		// 0000-1FFF: RAM enable
			params.ram_enable = (value == 0x0a);
			break;
		case 0x1:		// 2000-3FFF: ROM bank number - lower 5 bits
			if (value == 0)			// cf pandocs
				value++;
			params.rom_bank &= ~31;		// Remplace les bits du bas
			set_rom_bank(params.rom_bank | (value & 31));
			break;
		case 0x2:		// 4000-5FFF: ROM bank number - upper 2 bits
			if (params.bank_mode == 0) {		// Sélection banque ROM
				params.rom_bank &= 31;			// Garde les bits du bas
				set_rom_bank(params.rom_bank | (value & 3) << 5);
			}
			else						// Sélection banque RAM
				set_ram_bank(value & 3);
			break;
		case 0x3:		// 6000-7FFF: ROM/RAM bank select switch
			params.bank_mode = value & 1;
			break;
	}
}

// MBC2
static void mbc2_write(u16 address, u8 value) {
	switch (address >> 13) {
		case 0x0:		// 0000-1FFF: RAM enable
			if ((address & 0x100) == 0)		// LSB of upper addr byte zero
				params.ram_enable = (value == 0x0a);	// FIXME comme MBC1?
			break;
		case 0x1:		// 2000-3FFF: ROM bank number - lower 5 bits
			if ((address & 0x0100) == 1)
				set_rom_bank(value & 0xf);
			break;
	}
}

// MBC3
static void mbc3_write(u16 address, u8 value) {
	switch (address >> 13) {
		case 0x0:		// 0000-1FFF: RAM enable
			params.ram_enable = (value == 0x0a);
			break;
		case 0x1:		// 2000-3FFF: ROM bank number - 7 bits
			if (value == 0)
				value = 1;
			set_rom_bank(value & 127);
			break;
		case 0x02:		// 4000-5FFF: RAM bank number
			set_ram_bank(value & 3);
			break;
	}
}

// Pour la sauvegarde d'état
mbc_params_t *mbc_get_params() {
	return &params;
}

unsigned mbc_get_sram_data(u8 *buffer, u32 max_size) {
	int written = min(max_size, ram_size);
	// Copie la SRAM actuelle
	memcpy(buffer, ram_data, written);
	return written;
}

void mbc_set_sram_data(const u8 *buffer, unsigned size) {
	// Remplace la SRAM actuelle
	memcpy(ram_data, buffer, min(size, ram_size));
}

// Calcule la taille de la ROM en fonction de l'en-tête
static u32 calc_rom_size(u8 rom_byte) {
	// Note: X << 10 = X Kilooctets, X << 20 = X Megaoctets
	if (rom_byte < 8)			// Voir 0148 - ROM Size (pandocs)
		return (32 << 10) << rom_byte;
	// Codes spéciaux (voir pandocs)
	else if (rom_byte >= 0x52 && rom_byte <= 0x54)
		return (1 << 20) + ((128 << 10) << (rom_byte - 0x52));
	dbg_warning("Invalid ROM size byte");
	return 0;
}

// Calcule la taille de la SRAM en fonction de l'en-tête
static u32 calc_ram_size(u8 ram_byte) {
	// Table des tailles de RAM
	const u16 size_table[4] = {0, 2 << 10, 8 << 10, 32 << 10};
	if (ram_byte < 4)
		return size_table[ram_byte];
	dbg_warning("Invalid RAM size byte");
	return 0;
}

void mbc_init(u32 loaded_size) {
	u8 cart_type = mem_rom[0x147];
	// Si la taille n'est pas celle définie dans l'en-tête, il y a probablement une erreur
	rom_size = calc_rom_size(mem_rom[0x148]);
	if (rom_size != loaded_size)
		dbg_error("Invalid ROM size. Please check that it is not corrupt.");
	rom_size = min(rom_size, loaded_size);		// Pour ne pas dépasser...
	// Taille de la RAM
	ram_size = calc_ram_size(mem_rom[0x149]);
	memset(ram_data, 0, ram_size);
	// Initialisation; la bank 0 est toujours mappée à 0000 - 3FFF
	// rom_bank concerne uniquement les accès à 4000 - 7FFF
	params.rom_bank = 1;		// 4000-7FFF
	params.ram_bank = 0;
	params.bank_mode = 0;		// ROM bank select
	params.ram_enable = 0;		// RAM désactivée
	// MBC selon le types de carte: voir pandocs
	switch (cart_type) {
		case 0x00:		// ROM only
		case 0x08:		// ROM + RAM
		case 0x09:		// ROM + RAM + BATTERY
			mbc_read = direct_read;
			mbc_write = null_write;
			params.ram_enable = 1;		// RAM toujours activée si pas de MBC
			dbg_info("No mapper");
			break;
		case 0x01:		// MBC1
		case 0x02:		// MBC1 + RAM
		case 0x03:		// MBC1 + RAM + BATTERY
			mbc_read = mbc1_read;
			mbc_write = mbc1_write;
			dbg_info("MBC1 mapper");
			break;
		case 0x05:		// MBC2
		case 0x06:		// MBC2 + BATTERY
			mbc_read = mbc1_read;
			mbc_write = mbc2_write;
			dbg_info("MBC2 mapper");
			break;
		case 0x0f:		// MBC3 + TIMER + BATTERY
		case 0x10:		// MBC3 + TIMER + RAM + BATTERY
		case 0x11:		// MBC3
		case 0x12:		// MBC3 + RAM
		case 0x13:		// MBC3 + RAM + BATTERY
			mbc_read = mbc1_read;
			mbc_write = mbc3_write;
			dbg_info("MBC3 mapper");
			break;
		default:
			dbg_warning("Unknown cartridge type %02x. Will be emulated as MBC3", cart_type);
			mbc_read = mbc1_read;
			mbc_write = mbc3_write;
			break;
	}
}
