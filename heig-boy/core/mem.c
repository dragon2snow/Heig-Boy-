#include "mem.h"
#include "debug.h"
#include "sound.h"
#include <string.h>

u8 *mem_rom;
u8 mem_io[0x100];
u8 mem_vram[0x2000];
u8 mem_oam[0x100];
static u8 mem_ram[0x2000];
static u8 mem_sram[0x2000];

/** Lit depuis un port (IO)
	\param port une des constantes REG_* déclarées dans ports.h, ou une adresse
		à laquelle on soustrait 0xff80 (adresse du premier port).
	\return la valeur du port
	@note déclenche éventuellement un événement (généré par la lecture sur le
		bus)
*/
static u8 mem_io_readb(u16 port) {
	if (port >= 0x10 && port < 0x40)	// Son
		return sound_read(port);
	switch (port) {
		case R_JOYP:
			return 0xdf;
//			return REG(JOYP);				// Pas supporté
		default:
			return mem_io[port];
	}
}

static void mem_io_writeb(u16 port, u8 value) {
	if (port >= 0x10 && port < 0x40)	// Son
		sound_write(port, value);
	else {
		switch (port) {
			case R_JOYP:
				// Bits du bas read-only
				REG(JOYP) = REG(JOYP) & 0x0f | (value & 0xf0);
				break;
			case R_STAT:
				REG(STAT) = REG(STAT) & 0x7 | (value & ~7);
				break;
			case R_DMA: {
				// L'adresse est la valeur écrite / 0x100
				u16 i, source = value << 8;
				mem_io[port] = value;
				for (i = 0; i < 0xa0; i++)
					mem_oam[i] = mem_readb(source++);
				break;
			}
			default:
				mem_io[port] = value;
				break;
		}
	}
}

u8 mem_readb(u16 address) {
	// TODO peut être mieux avec des if?
	// Sélection de la zone mémoire
	switch (address & 0xf000) {
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:	// ROM bank 0	
			return mem_rom[address];
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:	// ROM bank 1-n
			// FIXME: temporaire (émuler le MBC please...)
			return mem_rom[address];
		case 0x8000:
		case 0x9000:	// Video RAM (VRAM)
			return mem_vram[address - 0x8000];
		case 0xa000:
		case 0xb000:	// External RAM (SRAM)
			return mem_sram[address - 0xa000];
		case 0xc000:
		case 0xd000:	// Work RAM (WRAM)
			return mem_ram[address - 0xc000];
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				return mem_ram[address - 0xe000];
			else if (address < 0xff00)	// OAM (object attribute memory)
				return mem_oam[address - 0xfe00];
			else						// Ports + HIRAM en fait
				return mem_io_readb(address - 0xff00);
	}
}

void mem_writeb(u16 address, u8 value) {
	// Sélection de la zone mémoire
	switch (address & 0xf000) {
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:	// Ecriture en ROM, viendra au MBC...
			break;
		case 0x8000:
		case 0x9000:	// Video RAM (VRAM)
			mem_vram[address - 0x8000] = value;
			break;
		case 0xa000:
		case 0xb000:	// External RAM (SRAM)
			mem_sram[address - 0xa000] = value;
			break;
		case 0xc000:
		case 0xd000:	// Work RAM (WRAM)
			mem_ram[address - 0xc000] = value;
			break;
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				mem_ram[address - 0xe000] = value;
			else if (address < 0xff00)	// OAM (object attribute memory)
				mem_oam[address - 0xfe00] = value;
			else						// Ports + HIRAM en fait
				mem_io_writeb(address - 0xff00, value);
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
	const u16 hi = 0xff00;
	// En fait la RAM de la Game Boy contient initialement des données
	// aléatoires, mais on la mettra à zéro
	memset(mem_ram, 0, sizeof(mem_ram));
	memset(mem_vram, 0, sizeof(mem_vram));
	memset(mem_io, 0, sizeof(mem_io));
	memset(mem_oam, 0, sizeof(mem_oam));
	memset(mem_sram, 0, sizeof(mem_sram));
	// Initialise les registres ayant des valeurs connues différentes de zéro
	mem_writeb(hi + R_TIMA, 0x00);
	mem_writeb(hi + R_TMA, 0x00);
	mem_writeb(hi + R_TAC, 0x00);
	mem_writeb(hi + R_NR10, 0x80);
	mem_writeb(hi + R_NR11, 0xBF);
	mem_writeb(hi + R_NR12, 0x03);		// au lieu de F3 pour désactiver le son
	mem_writeb(hi + R_NR14, 0xBF);
	mem_writeb(hi + R_NR21, 0x3F);
	mem_writeb(hi + R_NR22, 0x00);
	mem_writeb(hi + R_NR24, 0xBF);
	mem_writeb(hi + R_NR30, 0x7F);
	mem_writeb(hi + R_NR31, 0xFF);
	mem_writeb(hi + R_NR32, 0x9F);
	mem_writeb(hi + R_NR33, 0xBF);
	mem_writeb(hi + R_NR41, 0xFF);
	mem_writeb(hi + R_NR42, 0x00);
	mem_writeb(hi + R_NR43, 0x00);
	mem_writeb(hi + R_NR44, 0xBF);
	mem_writeb(hi + R_NR50, 0x77);
	mem_writeb(hi + R_NR51, 0xF3);
	mem_writeb(hi + R_NR52, 0xF1);
	mem_writeb(hi + R_LCDC, 0x91);
	mem_writeb(hi + R_SCY, 0x00);
	mem_writeb(hi + R_SCX, 0x00);
	mem_writeb(hi + R_LYC, 0x00);
	mem_writeb(hi + R_BGP, 0xFC);
	mem_writeb(hi + R_OBP0, 0xFF);
	mem_writeb(hi + R_OBP1, 0xFF);
	mem_writeb(hi + R_WY, 0x00);
	mem_writeb(hi + R_WX, 0x00);
	mem_writeb(hi + R_IE, 0x00);
}
