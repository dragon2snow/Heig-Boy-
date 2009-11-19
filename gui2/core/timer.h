#ifndef TIMER_H
#define TIMER_H

#include "common.h"

/** Initialisation du timer
	\note � appeler au d�but de l'ex�cution ou lorsque  du jeu */
void timer_init();

/** Mise � jour du composant timer
	\param elapsed cycles �coul�s depuis le dernier tick
*/
void timer_tick(int elapsed);

/** R�alise une lecture dans la zone I/O du timer
	\param port n� du port � lire (adresse - $FF00)
	\return valeur de retour par le bus de donn�es
*/
u8 timer_read(u16 port);

/** R�alise une �criture dans la zone I/O du timer
	\param port n� du port � lire (adresse - $FF00)
	\param value valeur � �crire (via le bus de donn�es)
*/
void timer_write(u16 port, u8 value);

#endif
