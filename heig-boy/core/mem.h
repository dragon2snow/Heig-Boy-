#ifndef MEM_H
#define MEM_H

#include "common.h"
#include "ports.h"

/*	Note: ces éléments sont mis publics car il a sens d'y accéder sans passer
	par les fonctions wrapper (mem_read / write).
	Typiquement mem_io[REG_JOYP] = 0 est une façon légale d'initialiser le
	registre du joypad car l'écriture via le bus (mem_writeb) verrait la
	moitié des bits ignorés (car ils sont en lecture seule).
	(On aurait pu créer des fonctions supplémentaires, on sait, mais le code
	est déjà bien assez complexe comme ça, donc on fera entorse).
*/
/** Image de la cartouche, allouée dynamiquement */
extern u8 *mem_rom;
/** Ports (zone I/O à FF00 utilisée pour configurer les périphériques) */
extern u8 mem_io[0x80];

/** Initialise la mémoire à l'état connu sur la vraie Game Boy après la
	power-up sequence (affichage du logo Nintendo + boot du jeu s'il
	correspond).
*/
void mem_init();

/** Lecture depuis une adresse mémoire via le bus
	\param address adresse de lecture
	\return valeur à cette adresse
	\note cette fonction n'accède pas directement à la mémoire mais passe par
		le bus. Cela implique que l'espace d'adressage est tel que vu par le
		programme Game Boy lui-même (tient compte des banques, etc.). De plus,
		des  événements pourront être déclenchés suite à la lecture ou à
		l'écriture en mémoire (p. ex. écriture sur le timer).
*/
u8 mem_readb(u16 address);

/** Ecriture à une adresse mémoire via le bus. Voir #mem_readb.
	\param address adresse
	\param value valeur à écrire
*/
void mem_writeb(u16 address, u8 value);

/** Lecture d'un mot en mémoire via le bus
	\param address adresse de lecture
	\return le mot lu en little endian
	\note la lecture est réalisée via deux lectures 8 bits sur le bus
*/
u16 mem_readw(u16 address);

/** Ecriture d'un mot en mémoire via le bus
	\param adresse d'écriture
	\param value mot (16 bits) à écrire
*/
void mem_writew(u16 address, u16 value);


#endif
