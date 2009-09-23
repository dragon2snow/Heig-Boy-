#ifndef PORTS_H
#define PORTS_H

#include "common.h"
#include "mem.h"

/** Petite entorse à la règle de nommage vu l'importante présence des ports
	Note: une étoile * signifie que le composant n'est pas émulé.
	[W] ou [R] signifie que le port n'est accessible qu'en lecture (R) ou
	écriture (W), alors que [!] signifie que cela dépend des bits.
*/
enum {
	// Général
	REG_JOYP = 0x00,		// Joypad
	REG_SB = 0x01,			// Serial transfer data*
	REG_SC = 0x02,			// Serial transfer control*
	REG_DIV = 0x04,			// Timer divider
	REG_TIMA = 0x05,		// Timer counter
	REG_TMA = 0x06,			// Timer modulo. Repris quand TIMA dépasse (overflow).
	REG_TAC = 0x07,			// Timer control
	REG_IF = 0x0f,			// Interrupt flag

	// Audio
	REG_NR10 = 0x10,		// Channel 1 Sweep
	REG_NR11,				// Channel 1 Sound length/Wave pattern duty
	REG_NR12,				// Channel 1 Volume Envelope
	REG_NR13,				// [W] Channel 1 Frequency lo
	REG_NR14,				// [!] Channel 1 Frequency hi
	REG_NR21 = 0x16,		// Channel 2 Sound length/Wave pattern duty
	REG_NR22,				// Channel 2 Volume Envelope
	REG_NR23,				// [W] Channel 2 Frequency lo
	REG_NR24,				// [!] Channel 2 Frequency hi
	REG_NR30,				// Channel 3 enable
	REG_NR31,				// Channel 3 sound length
	REG_NR32,				// Channel 3 output level
	REG_NR33,				// [W] Channel 3 Frequency lo
	REG_NR34,				// [!] Channel 3 Frequency hi
	REG_NR41 = 0x20,		// Channel 4 sound length
	REG_NR42,				// Channel 4 volume envelope
	REG_NR43,				// Channel 4 Polynomial Counter
	REG_NR44,				// Channel 4 Counter select
	REG_NR50,				// Channel control (master volume and external channel)
	REG_NR51,				// Selection of Sound output terminal
	REG_NR52,				// [!] Sound enable
	// FF27 - FF2F inutilisé, FF30 - FF3F pattern RAM
	REG_LCDC = 0x40,		// LCD control
	REG_STAT,				// LCD status
	REG_SCY,				// Scroll Y
	REG_SCX,				// Scroll X
	REG_LY,					// Ligne du balayage LCD
	REG_LYC,				// LY compare (déclenche une IRQ)
	REG_DMA,				// [W] DMA Transfer and Start Address
	REG_BGP,				// BG Palette Data (DMG only)
	REG_OBP0,				// Object Palette 0 Data (DMG only)
	REG_OBP1,				// Object Palette 0 Data (DMG only)
	REG_WY,					// Fenêtre Y
	REG_WX,					// Fenêtre X
};

#endif
