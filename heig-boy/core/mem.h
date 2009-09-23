#ifndef MEM_H
#define MEM_H

#include "common.h"
#include "ports.h"

/** Image de la cartouche, allou�e dynamiquement */
// FIMXE: peut �tre cacher (mem_load_rom)
extern u8 *mem_rom;
/** Ports (zone I/O � FF00 utilis�e pour configurer les p�riph�riques) */
extern u8 mem_io[0x80];

/** Lecture depuis une adresse m�moire
	\param address ...
	\return valeur � cette adresse
*/
u8 mem_readb(u16 address);

/** Ecriture � une adresse m�moire */
void mem_writeb(u16 address, u8 value);

/** Lecture d'un mot en m�moire via le bus */
u16 mem_readw(u16 address);
void mem_writew(u16 address, u16 value);


#endif
