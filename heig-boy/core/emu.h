#ifndef EMU_H
#define EMU_H

#include "common.h"

/** Charge une image de cartouche de jeu
	\param file_name nom du fichier image (ex: tetris.gb)
	\return 0 si le fichier a été chargé,<br>
		-1 si le fichier n'a pas été trouvé
*/
int emu_load_cart(const char *file_name);

/** Exécute l'émulateur durant le temps d'une frame (1/60 de seconde) et
	retourne ensuite, le laissant dans le même état.
*/
void emu_do_frame();

#endif
