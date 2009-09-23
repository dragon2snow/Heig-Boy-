#ifndef MEM_H
#define MEM_H

#include "common.h"
#include "ports.h"

/** Image de la cartouche, allouée dynamiquement */
// FIMXE: peut être cacher (mem_load_rom)
extern u8 *mem_rom;
/** Ports (zone I/O à FF00 utilisée pour configurer les périphériques) */
extern u8 mem_io[0x80];

/** Lecture depuis une adresse mémoire
	\param address ...
	\return valeur à cette adresse
*/
u8 mem_readb(u16 address);

/** Ecriture à une adresse mémoire */
void mem_writeb(u16 address, u8 value);

/** Lecture d'un mot en mémoire via le bus */
u16 mem_readw(u16 address);
void mem_writew(u16 address, u16 value);


#endif
