#ifndef MBC_H
#define MBC_H

#include "common.h"

typedef struct {
	u8 rom_bank, bank_mode, ram_bank, ram_enable;
} mbc_params_t;

/** Initialise un contrôleur multibanque.
	La ROM doit avoir été chargée d'abord! La fonction va notamment accéder
	à l'adresse $147 pour obtenir le type de cartouche.
	\param loaded_size taille de la cartouche chargée
*/
void mbc_init(u32 loaded_size);

/** Réalise une lecture dans la zone I/O de la cartouche ($4000-$7FFF).
	\param address adresse à lire
	\return valeur de retour par le bus de données
	\note les adresses entre $0000 et $3FFF peuvent être retournées directement
		via mem_rom car les premiers 16k sont toujours mappés à la page 0
*/
extern u8 (*mbc_read)(u16 address);

/** Réalise une écriture dans la zone I/O de la cartouche ($0000-$7FFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse à écrire
	\param value valeur à écrire (via le bus de données)
*/
extern void (*mbc_write)(u16 address, u8 value);

/** Réalise une lecture dans la zone SRAM de la cartouche ($A000-$BFFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse à écrire
	\param value valeur à écrire (via le bus de données)
*/
u8 mbc_sram_read(u16 address);

/** Réalise une écriture dans la zone SRAM de la cartouche ($A000-$BFFF).
	Voir #mbc_read pour plus d'informations.
	\param address adresse à écrire
	\param value valeur à écrire (via le bus de données)
*/
void mbc_sram_write(u16 address, u8 value);

/** Retourne les paramètres du MBC. Utile pour réaliser une sauvegarde de
	l'état de la machine.
	\return une structure #mbc_params_t avec les paramètres courants.
*/
const mbc_params_t *mbc_get_params();

/** Lit les données de la SRAM. Utile pour la sauvegarde d'état.
	\param buffer tampon de destination des données
	\param max_size taille maximale du tampon
	\return nombre d'octets lus (taille de la SRAM)
*/
u32 mbc_get_sram_data(u8 *buffer, u32 max_size);

/** Ecrit les données dans la SRAM. Utile pour la restauration d'état.
	\param buffer données à écrire
	\param size nombre d'octets à écrire
*/
void mbc_set_sram_data(const u8 *buffer, u32 size);

#endif
