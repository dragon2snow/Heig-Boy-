#ifndef MBC_H
#define MBC_H

#include "common.h"

typedef struct {
	u8 rom_bank, bank_mode, ram_bank, ram_enable;
} mbc_params_t;

/** Initialise un contr�leur multibanque.
	La ROM doit avoir �t� charg�e d'abord! La fonction va notamment acc�der
	� l'adresse $147 pour obtenir le type de cartouche.
	\param loaded_size taille de la cartouche charg�e
*/
void mbc_init(u32 loaded_size);

/** R�alise une lecture dans la zone I/O de la cartouche ($4000-$7FFF).
	\param address adresse � lire
	\return valeur de retour par le bus de donn�es
	\note les adresses entre $0000 et $3FFF peuvent �tre retourn�es directement
		via mem_rom car les premiers 16k sont toujours mapp�s � la page 0
*/
extern u8 (*mbc_read)(u16 address);

/** R�alise une �criture dans la zone I/O de la cartouche ($0000-$7FFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse � �crire
	\param value valeur � �crire (via le bus de donn�es)
*/
extern void (*mbc_write)(u16 address, u8 value);

/** R�alise une lecture dans la zone SRAM de la cartouche ($A000-$BFFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse � �crire
	\param value valeur � �crire (via le bus de donn�es)
*/
u8 mbc_sram_read(u16 address);

/** R�alise une �criture dans la zone SRAM de la cartouche ($A000-$BFFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse � �crire
	\param value valeur � �crire (via le bus de donn�es)
*/
void mbc_sram_write(u16 address, u8 value);

/** Retourne les param�tres du MBC. Utile pour r�aliser une sauvegarde de
	l'�tat de la machine.
	\return une structure #mbc_params_t avec les param�tres courants.
*/
const mbc_params_t *mbc_get_params();

/** Lit les donn�es de la SRAM. Utile pour la sauvegarde d'�tat.
	\param buffer tampon de destination des donn�es
	\param max_size taille maximale du tampon
	\return nombre d'octets lus (taille de la SRAM)
*/
u32 mbc_get_sram_data(u8 *buffer, u32 max_size);

/** Ecrit les donn�es dans la SRAM. Utile pour la restauration d'�tat.
	\param buffer donn�es � �crire
	\param size nombre d'octets � �crire
*/
void mbc_set_sram_data(const u8 *buffer, u32 size);

#endif
