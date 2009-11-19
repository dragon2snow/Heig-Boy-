#ifndef EMU_H
#define EMU_H

#include "common.h"

/** Charge une image de cartouche de jeu
	\param file_name nom du fichier image (ex: tetris.gb)
	\return 0 si le fichier a �t� charg�,<br>
		-1 si le fichier n'a pas �t� trouv�
*/
int emu_load_cart(const char *file_name);

/** Ex�cute la "power-up sequence" virtuelle de la Game Boy qui initialise le
	mat�riel et les registres � des valeurs connues.
*/
void emu_init();

/** Ex�cute l'�mulateur durant le temps d'une frame (1/60 de seconde) et
	retourne ensuite, le laissant dans le m�me �tat.
*/
void emu_do_frame();

/** Nom du fichier image en cours d'�mulation. */
extern char emu_file_name[256];

#endif
