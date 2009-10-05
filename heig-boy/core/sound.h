#ifndef SOUND_H
#define SOUND_H

#include "common.h"

/** Initialisation du syst�me sonore. */
void sound_init();
/** Produit du son � 44.1 kHz en fonction de l'�tat actuel des registres de
	l'APU.
	\param buf tampon o� placer les donn�es sonores
	\param len nombre d'�chantillons st�r�o � produire (44100 = 1 seconde)
	\note le son produit est � 44100 Hz, 16 bits par �chantillon, st�r�o.
*/
void sound_render(s16 *buf, unsigned len);

/** R�alise une lecture dans la zone I/O r�serv�e � l'APU ($FF10-$FF3F).
	On a besoin d'isoler la lecture et �criture sur cette zone car les valeurs
	pouvant �tre lues �voluent en fonction du son produit.
	\param port n� du port � lire (adresse - $FF00)
	\return valeur de retour par le bus de donn�es
*/
u8 sound_readb(u16 port);

/** R�alise une �criture dans la zone I/O r�serv�e � l'APU ($FF10-$FF3F).
	Voir #sound_readb pour plus d'informations.
	\param port n� du port � lire (adresse - $FF00)
	\return value valeur � �crire (via le bus de donn�es)
*/
void sound_writeb(u16 port, u8 value);

#endif
