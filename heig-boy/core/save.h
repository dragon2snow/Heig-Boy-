#ifndef SAVE_H
#define SAVE_H

/** Sauvegarde la SRAM associ�e au logiciel en cours d'ex�cution (espace de
	sauvegarde r�serv� � cet effet dans la cartouche). */
void save_sram();

/** Essaie de charger la SRAM associ�e au fichier image charg�. Ne fonctionne
	que si une sauvegarde a pr�alablement �t� faite avec #save_sram. */
void load_sram();

/** Sauvegarde l'�tat de la machine.
	\param slot num�ro du slot de sauvegarde (0-9)
*/
void save_state(int slot);

/** Restore l'�tat pr�c�demment sauvegard� de la machine.
	\param slot num�ro du slot de sauvegarde (0-9)
	\return vrai si le slot a �t� charg�
*/
int load_state(int slot);

#endif
