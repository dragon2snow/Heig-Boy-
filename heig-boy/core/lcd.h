#ifndef LCD_H
#define LCD_H

#include "common.h"

extern bool lcd_frame_end_flag;

/** Initialisation du LCD (au chargement du jeu) */
void lcd_init();
/** Initialisation de la partie dessin (faite automatiquement par lcd_init). */
void lcd_draw_init();
/** Démarrage du dessin d'une nouvelle image. */
void lcd_begin();
/** Dessine la ligne courante (LY) sur le buffer LCD */
void lcd_draw_line();
/** Mise à jour du composant LCD
	\param elapsed cycles écoulés depuis le dernier tick
	\param draw indique si le rendu (dessin) doit être effectué. Plus lent.
*/
void lcd_tick(int elapsed, bool draw);
/** Récupère l'image dessinée. A appeler juste après emu_do_frame pour obtenir
	l'image dessinée pour la frame en question.
	\param dest tampon de taille suffisante pour accueillir 144 * destWidth
		pixels (32 bits par pixel)
	\param destWidth largeur en pixels du tampon
	\note le format est BGRA (8 bits Bleu, Vert, Rouge, Alpha dans cet ordre,
		avec l'alpha comme bits de poids fort).
*/
void lcd_copy_to_buffer(u32 *dest, int destWidth);

#endif
