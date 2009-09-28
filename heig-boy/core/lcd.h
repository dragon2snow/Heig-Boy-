#ifndef LCD_H
#define LCD_H

#include "common.h"

/** Initialisation du LCD (au chargement du jeu) */
void lcd_init();
/** Initialisation de la partie dessin (faite automatiquement par lcd_init). */
void lcd_draw_init();
/** Réinitialisation du LCD pour une prochaine image. */
void lcd_begin();
/** Dessine la ligne courante (LY) sur le buffer LCD */
void lcd_draw_line();
/** Mise à jour du composant LCD
	\param elapsed cycles elapsed since the last tick
*/
void lcd_tick(int elapsed);
int lcd_idle_cycles();

#endif
