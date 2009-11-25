#ifndef EMU_H
#define EMU_H

#include "common.h"

/** Charge une image de cartouche de jeu
	\param file_name nom du fichier image (ex: tetris.gb)
	\return true si le fichier a été chargé,<br>
		false si le fichier n'a pas été trouvé
*/
bool emu_load_cart(const char *file_name);

/** Exécute la "power-up sequence" virtuelle de la Game Boy qui initialise le
	matériel et les registres à des valeurs connues.
*/
void emu_init();

/** Exécute l'émulateur durant le temps d'une frame (1/60 de seconde) et
	retourne ensuite, le laissant dans le même état.
	\param draw indique s'il faut dessiner l'affichage (plus lent)
*/
void emu_do_frame(bool draw);

/** Nom du fichier image en cours d'émulation. */
extern char emu_file_name[256];

#endif
