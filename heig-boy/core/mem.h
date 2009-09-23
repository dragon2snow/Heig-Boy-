#ifndef MEM_H
#define MEM_H

#include "common.h"
#include "ports.h"

/*	Note: ces �l�ments sont mis publics car il a sens d'y acc�der sans passer
	par les fonctions wrapper (mem_read / write).
	Typiquement mem_io[REG_JOYP] = 0 est une fa�on l�gale d'initialiser le
	registre du joypad car l'�criture via le bus (mem_writeb) verrait la
	moiti� des bits ignor�s (car ils sont en lecture seule).
	(On aurait pu cr�er des fonctions suppl�mentaires, on sait, mais le code
	est d�j� bien assez complexe comme �a, donc on fera entorse).
*/
/** Image de la cartouche, allou�e dynamiquement */
extern u8 *mem_rom;
/** Ports (zone I/O � FF00 utilis�e pour configurer les p�riph�riques) */
extern u8 mem_io[0x80];

/** Initialise la m�moire � l'�tat connu sur la vraie Game Boy apr�s la
	power-up sequence (affichage du logo Nintendo + boot du jeu s'il
	correspond).
*/
void mem_init();

/** Lecture depuis une adresse m�moire via le bus
	\param address adresse de lecture
	\return valeur � cette adresse
	\note cette fonction n'acc�de pas directement � la m�moire mais passe par
		le bus. Cela implique que l'espace d'adressage est tel que vu par le
		programme Game Boy lui-m�me (tient compte des banques, etc.). De plus,
		des  �v�nements pourront �tre d�clench�s suite � la lecture ou �
		l'�criture en m�moire (p. ex. �criture sur le timer).
*/
u8 mem_readb(u16 address);

/** Ecriture � une adresse m�moire via le bus. Voir #mem_readb.
	\param address adresse
	\param value valeur � �crire
*/
void mem_writeb(u16 address, u8 value);

/** Lecture d'un mot en m�moire via le bus
	\param address adresse de lecture
	\return le mot lu en little endian
	\note la lecture est r�alis�e via deux lectures 8 bits sur le bus
*/
u16 mem_readw(u16 address);

/** Ecriture d'un mot en m�moire via le bus
	\param adresse d'�criture
	\param value mot (16 bits) � �crire
*/
void mem_writew(u16 address, u16 value);


#endif
