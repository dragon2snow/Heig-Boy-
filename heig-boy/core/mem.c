#include "mem.h"
#include "io.h"
#include "mbc.h"
#include <string.h>

u8 *mem_rom;					// ROM: taille dépend de la cartouche
u8 mem_io[0x100];				// IO + HIRAM = 128 + 128 octets
u8 mem_vram[0x2000];			// VRAM = 8k
u8 mem_oam[0x100];				// OAM = plage de 256 octets, 160 utilisés
static u8 mem_ram[0x2000];		// RAM = 8k

u8 mem_readb(u16 address) {
	// TODO peut être mieux avec des if?
	// Sélection de la zone mémoire (digit X du haut: X000-XFFF)
	switch (address >> 12) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:		// ROM
			return mbc_read(address);
		case 0x8:
		case 0x9:		// Video RAM (VRAM)
			return mem_vram[address - 0x8000];
		case 0xa:
		case 0xb:		// External RAM (SRAM)
			return mbc_sram_read(address);
		case 0xc:
		case 0xd:		// Work RAM (WRAM)
			return mem_ram[address - 0xc000];
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				return mem_ram[address - 0xe000];
			else if (address < 0xff00)	// OAM (object attribute memory)
				return mem_oam[address - 0xfe00];
			else						// Ports + HIRAM en fait
				return io_read(address - 0xff00);
	}
}

void mem_writeb(u16 address, u8 value) {
	// Sélection de la zone mémoire, voir mem_readb
	switch (address >> 12) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:		// Ecriture en ROM
			mbc_write(address, value);
			break;
		case 0x8:
		case 0x9:		// Video RAM (VRAM)
			mem_vram[address - 0x8000] = value;
			break;
		case 0xa:
		case 0xb:		// External RAM (SRAM)
			mbc_sram_write(address, value);
			break;
		case 0xc:
		case 0xd:		// Work RAM (WRAM)
			mem_ram[address - 0xc000] = value;
			break;
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				mem_ram[address - 0xe000] = value;
			else if (address < 0xff00)	// OAM (object attribute memory)
				mem_oam[address - 0xfe00] = value;
			else						// Ports + HIRAM en fait
				io_write(address - 0xff00, value);
			break;
	}
}

u16 mem_readw(u16 address) {
	return mem_readb(address) | mem_readb(address + 1) << 8;
}

void mem_writew(u16 address, u16 value) {
	mem_writeb(address, value & 0xff);
	mem_writeb(address + 1, value >> 8 & 0xff);
}

void mem_init() {
	// En fait la RAM de la Game Boy contient initialement des données
	// aléatoires, mais on la mettra à zéro
	memset(mem_ram, 0, sizeof(mem_ram));
	memset(mem_vram, 0, sizeof(mem_vram));
	memset(mem_io, 0, sizeof(mem_io));
	memset(mem_oam, 0, sizeof(mem_oam));
	// Initialise les autres composants
	io_init();
}
