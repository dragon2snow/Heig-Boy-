/** \file lcd-status.c
	\brief Gestion des transitions d'état du LCD et de la génération des IRQ.
*/
#include "lcd.h"
#include "cpu.h"
#include "ports.h"

// Contenu du registre STAT
struct STAT_t {
	u8 mode: 2;			// read only
	u8 vcount_match: 1;	// read only
	u8 hblank_irq: 1, vblank_irq: 1, oam_irq: 1, vcount_irq: 1;
};
#define lcd_status	(*((struct STAT_t*)&REG(STAT)))
// Ligne courante du balayage LCD
#define lcd_line	REG(LY)

// Nombre de cycles jusqu'au prochain événement
static int next_event;

void lcd_init() {
	next_event = 0;
	lcd_line = 0;
	lcd_draw_init();
}

void lcd_tick(int elapsed) {
	next_event -= elapsed;
	// Evénement?
	while (next_event < 0) {
		/* Le LCD cycle entre les modes 0, 2, 3 en temps normal, puis à la
		   vblank (balayage au fond) il passe en mode 1 pour 10 lignes.
		   Pour plus d'informations, consultez les pandocs à la section
		   LCD Status Register. */
		switch (lcd_status.mode) {
			case 0:		// hblank
				// Prochaine ligne
				if (++lcd_line == 144) {
					// Balayage au fond du LCD => commencement de la vblank
					cpu_trigger_irq(INT_VBLANK);
					lcd_status.mode = 1;
					next_event += 456;
				}
				else {
					lcd_status.mode = 2;
					next_event += 80;
				}
				// La ligne demandée par l'utilisateur correspond
				if (lcd_line == REG(LYC))
					cpu_trigger_irq(INT_STAT);
				break;
			case 1:		// vblank
				// Fin de la vblank? (10 lignes)
				if (++lcd_line == 154) {
					// On recommence en haut
					lcd_status.mode = 2;
					lcd_line = 0;
					next_event += 80;
				}
				else
					next_event += 456;
				break;
			case 2:		// lecture de l'OAM
				lcd_status.mode = 3;
				next_event += 172;
				lcd_draw_line();
				break;
			case 3:		// dessin en cours
				lcd_status.mode = 0;
				next_event += 204;
				break;
		}
	}
}

