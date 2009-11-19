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
extern u8 mem_vram[0x2000];
extern u8 mem_oam[0x100];
/** Ports (zone I/O � FF00 utilis�e pour configurer les p�riph�riques) */
extern u8 mem_io[0x100];

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

/** Types de m�moire, utilis� pour #mem_get_area et #mem_set_area. */
typedef enum {
	MEM_RAM = 0,
	MEM_VRAM,
	MEM_IO,
	MEM_OAM
} mem_area_t;

/** R�cup�ration de la m�moire (pour une sauvegarde d'�tat)
	\param buffer tampon de destination
	\param area zone de m�moire � r�cup�rer
	\param max_size taille maximale du tampon
*/
unsigned mem_get_data(u8 *buffer, mem_area_t area, u32 max_size);

/** Restauration de la m�moire (pour une sauvegarde d'�tat)
	\param buffer tampon source
	\param area zone de m�moire � restaurer
	\param max_size taille du tampon
*/
void mem_set_data(const u8 *buffer, mem_area_t area, unsigned size);


#endif
