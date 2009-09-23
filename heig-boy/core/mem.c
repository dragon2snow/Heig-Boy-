#include "mem.h"
#include "debug.h"

u8 *mem_rom;
u8 mem_io[0x80];
u8 mem_hiram[0x7f];
u8 mem_vram[0x2000];
u8 mem_ram[0x2000];
u8 mem_sram[0x2000];
u8 mem_oam[0xa0];

/** Donne l'accès à un port par son nom documenté. Il n'est pas nécessaire
	d'utiliser le préfixe REG_*.
	\note Un poil moche mais améliore beaucoup la lisibilité étant donnée la
	quantité d'accès aux IO réalisés dans l'application */
#define PORT(r)		mem_io[REG_##r]

/** Lit depuis un port (IO)
	\param port une des constantes REG_* déclarées dans ports.h, ou une adresse
		à laquelle on soustrait 0xff80 (adresse du premier port).
	\return la valeur du port
	@note déclenche éventuellement un événement (généré par la lecture sur le
		bus)
*/
static u8 mem_io_readb(u16 port) {
	if (port >= 0x80 && port <= 0xfe)		// Mappée dans la zone des ports
		return mem_hiram[port - 0xff80];
	switch (port) {
		case REG_JOYP:
			return PORT(JOYP);				// Pas supporté
		default:
			return mem_io[port];
	}
}

static void mem_io_writeb(u16 port, u8 value) {
	if (port >= 0x80 && port <= 0xfe)		// Mappée dans la zone des ports
		mem_hiram[port - 0xff80] = value;
	else {
		switch (port) {
			case REG_JOYP:
				// Bits du bas read-only
				PORT(JOYP) = value & 0xf0;
			default:
				mem_io[port] = value;
		}
	}
}

u8 mem_readb(u16 address) {
	// TODO peut être mieux avec des if?
	// Sélection de la zone mémoire
	switch (address & 0xf000) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:	// ROM bank 0	
			return mem_rom[address];
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:	// ROM bank 1-n
			// FIXME: temporaire (émuler le MBC please...)
			return mem_rom[address];
		case 0x8:
		case 0x9:	// Video RAM (VRAM)
			return mem_vram[address - 0x8000];
		case 0xa:
		case 0xb:	// External RAM (SRAM)
			return mem_sram[address - 0xa000];
		case 0xc:
		case 0xd:	// Work RAM (WRAM)
			return mem_ram[address - 0xc000];
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				return mem_ram[address - 0xe000];
			else if (address < 0xfea0)	// OAM (object attribute memory)
				return mem_oam[address - 0xfe00];
			else if (address < 0xff00) {
				dbg_info("read from unusable memory %04x", address);
				return 0xff;
			}
			else						// Ports + HIRAM en fait
				return mem_io_readb(address - 0xff00);
	}
}

void mem_writeb(u16 address, u8 value) {
	// Sélection de la zone mémoire
	switch (address & 0xf000) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:	// Ecriture en ROM, viendra au MBC...
			break;
		case 0x8:
		case 0x9:	// Video RAM (VRAM)
			mem_vram[address - 0x8000] = value;
		case 0xa:
		case 0xb:	// External RAM (SRAM)
			mem_sram[address - 0xa000] = value;
		case 0xc:
		case 0xd:	// Work RAM (WRAM)
			mem_ram[address - 0xc000] = value;
		default:
			if (address < 0xfe00)		// E000 - FDFF: miroir C000 - DDFF
				mem_ram[address - 0xe000] = value;
			else if (address < 0xfea0)	// OAM (object attribute memory)
				mem_oam[address - 0xfe00] = value;
			else if (address < 0xff00)
				dbg_info("write to unusable memory %04x", address);
			else						// Ports + HIRAM en fait
				mem_io_writeb(address - 0xff00, value);
	}
}

u16 mem_readw(u16 address) {
	// /!\ Little endian
	return mem_readb(address) | mem_readb(address + 1) << 8;
}

void mem_writew(u16 address, u16 value) {
	mem_writeb(address, value & 0xff);
	mem_writeb(address + 1, value >> 8 & 0xff);
}
