#ifndef SOUND_H
#define SOUND_H

#include "common.h"

/** Initialisation du système sonore. */
void sound_init();
/** Produit du son à 44.1 kHz en fonction de l'état actuel des registres de
	l'APU.
	\param buf tampon où placer les données sonores
	\param len nombre d'échantillons stéréo à produire (44100 = 1 seconde)
	\note le son produit est à 44100 Hz, 16 bits par échantillon, stéréo.
*/
void sound_render(s16 *buf, unsigned len);

/** Réalise une lecture dans la zone I/O réservée à l'APU ($FF10-$FF3F).
	On a besoin d'isoler la lecture et écriture sur cette zone car les valeurs
	pouvant être lues évoluent en fonction du son produit.
	\param port n° du port à lire (adresse - $FF00)
	\return valeur de retour par le bus de données
*/
u8 sound_readb(u16 port);

/** Réalise une écriture dans la zone I/O réservée à l'APU ($FF10-$FF3F).
	Voir #sound_readb pour plus d'informations.
	\param port n° du port à lire (adresse - $FF00)
	\return value valeur à écrire (via le bus de données)
*/
void sound_writeb(u16 port, u8 value);

#endif
