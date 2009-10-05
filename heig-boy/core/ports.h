#ifndef PORTS_H
#define PORTS_H

#include "common.h"
#include "mem.h"

/** Petite entorse � la r�gle de nommage vu l'importante pr�sence des ports
	Note: une �toile * signifie que le composant n'est pas �mul�.
	[W] ou [R] signifie que le port n'est accessible qu'en lecture (R) ou
	�criture (W), alors que [!] signifie que cela d�pend des bits.
*/
enum {
	// G�n�ral
	R_JOYP = 0x00,		// Joypad
	R_SB = 0x01,		// Serial transfer data*
	R_SC = 0x02,		// Serial transfer control*
	R_DIV = 0x04,		// Timer divider
	R_TIMA = 0x05,		// Timer counter
	R_TMA = 0x06,		// Timer modulo. Repris quand TIMA d�passe (overflow).
	R_TAC = 0x07,		// Timer control
	R_IF = 0x0f,		// Interrupt flag

	// Audio
	R_NR10 = 0x10,		// Channel 1 Sweep
	R_NR11,				// Channel 1 Sound length/Wave pattern duty
	R_NR12,				// Channel 1 Volume Envelope
	R_NR13,				// [W] Channel 1 Frequency lo
	R_NR14,				// [!] Channel 1 Frequency hi
	R_NR20,				// [!] N'existe pas, pour g�n�raliser channels 1 et 2
	R_NR21,				// Channel 2 Sound length/Wave pattern duty
	R_NR22,				// Channel 2 Volume Envelope
	R_NR23,				// [W] Channel 2 Frequency lo
	R_NR24,				// [!] Channel 2 Frequency hi
	R_NR30,				// Channel 3 enable
	R_NR31,				// Channel 3 sound length
	R_NR32,				// Channel 3 output level
	R_NR33,				// [W] Channel 3 Frequency lo
	R_NR34,				// [!] Channel 3 Frequency hi
	R_NR41 = 0x20,		// Channel 4 sound length
	R_NR42,				// Channel 4 volume envelope
	R_NR43,				// Channel 4 Polynomial Counter
	R_NR44,				// Channel 4 Counter select
	R_NR50,				// Channel control (master volume and external channel)
	R_NR51,				// Selection of Sound output terminal
	R_NR52,				// [!] Sound enable
	// FF27 - FF2F inutilis�, FF30 - FF3F pattern RAM
	R_LCDC = 0x40,		// LCD control
	R_STAT,				// LCD status
	R_SCY,				// Scroll Y
	R_SCX,				// Scroll X
	R_LY,				// Ligne du balayage LCD
	R_LYC,				// LY compare (d�clenche une IRQ)
	R_DMA,				// [W] DMA Transfer and Start Address
	R_BGP,				// BG Palette Data (DMG only)
	R_OBP0,				// Object Palette 0 Data (DMG only)
	R_OBP1,				// Object Palette 0 Data (DMG only)
	R_WY,				// Fen�tre Y
	R_WX,				// Fen�tre X
};

/** Donne l'acc�s � un port par son nom document�. Il n'est pas n�cessaire
	d'utiliser le pr�fixe R_*.
	\note Pas tr�s propre mais am�liore beaucoup la lisibilit� car les acc�s
		aux IO sont malheureusement l�gion...
*/
#define REG(r)		mem_io[R_##r]

#endif
