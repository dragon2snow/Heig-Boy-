#ifndef LCD_H
#define LCD_H

#include "common.h"

extern bool lcd_frame_end_flag;
extern u32 *lcd_buffer;
// Taille d'une ligne de buffer
#define BUFFER_LINE_PITCH 256
// Donne la ligne n° n du buffer
#define lcd_buffer_line(n)	(lcd_buffer + n * BUFFER_LINE_PITCH)

/** Initialisation du LCD (au chargement du jeu) */
void lcd_init();
/** Initialisation de la partie dessin (faite automatiquement par lcd_init). */
void lcd_draw_init();
/** Dessine la ligne courante (LY) sur le buffer LCD */
void lcd_draw_line();
/** Mise à jour du composant LCD
	\param elapsed cycles elapsed since the last tick
*/
void lcd_tick(int elapsed);

#endif
