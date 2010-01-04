#include "lcd.h"
#include "io.h"
#include "ports.h"
#include "sound.h"
#include "timer.h"

static u8 key_state;

void io_init() {
	// Initialise les registres ayant des valeurs connues différentes de zéro
	io_write(R_TIMA, 0x00);
	io_write(R_TMA, 0x00);
	io_write(R_TAC, 0x00);
	io_write(R_NR10, 0x80);
	io_write(R_NR11, 0xBF);
	io_write(R_NR12, 0x03);		// au lieu de F3 pour désactiver le son
	io_write(R_NR14, 0xBF);
	io_write(R_NR21, 0x3F);
	io_write(R_NR22, 0x00);
	io_write(R_NR24, 0xBF);
	io_write(R_NR30, 0x7F);
	io_write(R_NR31, 0xFF);
	io_write(R_NR32, 0x9F);
	io_write(R_NR33, 0xBF);
	io_write(R_NR41, 0xFF);
	io_write(R_NR42, 0x00);
	io_write(R_NR43, 0x00);
	io_write(R_NR44, 0xBF);
	io_write(R_NR50, 0x77);
	io_write(R_NR51, 0xF3);
	io_write(R_NR52, 0xF1);
	io_write(R_LCDC, 0x91);
	io_write(R_SCY, 0x00);
	io_write(R_SCX, 0x00);
	io_write(R_LYC, 0x00);
	io_write(R_BGP, 0xFC);
	io_write(R_OBP0, 0xFF);
	io_write(R_OBP1, 0xFF);
	io_write(R_WY, 0x00);
	io_write(R_WX, 0x00);
	io_write(R_IE, 0x00);
	// Tout à 1 = aucune touche pressée
	key_state = ~0;
}

u8 io_read(u16 port) {
	// Cas particuliers -> redirige aux composants pour qu'ils le traitent
	if (port >= 0x10 && port < 0x40)
		return sound_read(port);
	else if (port >= 0x04 && port < 0x08)
		return timer_read(port);
	// Lecture sur des composants généraux
	switch (port) {
		case R_JOYP:			// joypad
			// Que souhaite lire l'utilisateur?
			if (REG(JOYP) & BIT(5))		// touches de direction
				return REG(JOYP) & 0xf0 | key_state & 0xf;
			else						// boutons
				return REG(JOYP) & 0xf0 | key_state >> 4;
		
		case R_DMA:				// lecture du DMA -> interdit
			return 0xff;		// retourne de la merde

		default:
			// Les autres n'ont rien de spécial (agissent comme de la mémoire)
			return mem_io[port];
	}
}

void io_write(u16 port, u8 value) {
	// Cas particuliers -> redirige aux composants pour qu'ils le traitent
	if (port >= 0x10 && port < 0x40)
		sound_write(port, value);
	else if (port >= 0x04 && port < 0x08)
		timer_write(port, value);
	else {
		// Ecriture sur des composants généraux
		switch (port) {
			case R_JOYP:		// joypad
				// Bits du bas read-only
				REG(JOYP) = REG(JOYP) & 0x0f | (value & 0xf0);
				break;

			case R_STAT:		// statut LCD
				// Bits du bas read only
				mem_io[port] = mem_io[port] & 7 | (value & ~7);
				break;

			case R_LCDC:		// contrôle du LCD
				// Petit hack: le LCD ne peut être activé que pendant la VBLANK
				// On utilise donc cette indication pour se synchroniser et
				// commencer l'affichage à ce moment
				if (value & ~mem_io[port] & BIT(7)) {
					lcd_begin();
					REG(LY) = 0;
				}
				mem_io[port] = value;
				break;

			case R_LY:			// ligne courante LCD -> read only!
				break;

			case R_DMA: {		// effectue une copie DMA
				// L'adresse est la valeur écrite / 0x100
				u16 i, source = value << 8;
				mem_io[port] = value;
				for (i = 0; i < 0xa0; i++)
					mem_oam[i] = mem_readb(source++);
				break;
			}

			default:
				// Les autres ne font rien de spécial
				mem_io[port] = value;
				break;
		}
	}
}

void io_key_press(gb_key_t k, bool state) {
	// On stocke à l'envers comme une vraie GB (pressé = 0)
	if (state)
		key_state &= ~BIT(k);
	else
		key_state |= BIT(k);
}
