#ifndef TIMER_H
#define TIMER_H

#include "common.h"

/** Initialisation du timer
	\note à appeler au début de l'exécution ou lorsque  du jeu */
void timer_init();

/** Mise à jour du composant timer
	\param elapsed cycles écoulés depuis le dernier tick
*/
void timer_tick(int elapsed);

/** Réalise une lecture dans la zone I/O du timer
	\param port n° du port à lire (adresse - $FF00)
	\return valeur de retour par le bus de données
*/
u8 timer_read(u16 port);

/** Réalise une écriture dans la zone I/O du timer
	\param port n° du port à lire (adresse - $FF00)
	\param value valeur à écrire (via le bus de données)
*/
void timer_write(u16 port, u8 value);

#endif
