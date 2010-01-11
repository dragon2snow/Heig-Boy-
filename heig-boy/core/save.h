#ifndef SAVE_H
#define SAVE_H

/** Sauvegarde la SRAM associée au logiciel en cours d'exécution (espace de
	sauvegarde réservé à cet effet dans la cartouche). */
void save_sram();

/** Essaie de charger la SRAM associée au fichier image chargé. Ne fonctionne
	que si une sauvegarde a préalablement été faite avec #save_sram. */
void load_sram();

/** Sauvegarde l'état de la machine.
	\param slot numéro du slot de sauvegarde (0-9)
*/
void save_state(int slot);

/** Restore l'état précédemment sauvegardé de la machine.
	\param slot numéro du slot de sauvegarde (0-9)
	\return vrai si le slot a été chargé
*/
int load_state(int slot);

#endif
