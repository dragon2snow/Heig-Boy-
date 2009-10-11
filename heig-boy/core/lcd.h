#ifndef LCD_H
#define LCD_H

#include "common.h"

/** Initialisation du LCD (au chargement du jeu) */
void lcd_init();
/** Initialisation de la partie dessin (faite automatiquement par lcd_init). */
void lcd_draw_init();
/** Dessine la ligne courante (LY) sur le buffer LCD */
void lcd_draw_line();
/** Mise � jour du composant LCD
	\param elapsed cycles elapsed since the last tick
*/
void lcd_tick(int elapsed);

#endif
