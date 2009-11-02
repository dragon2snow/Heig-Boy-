#ifndef IO_H
#define IO_H

#include "common.h"

/** Initialise le syst�me d'entr�e-sortie */
void io_init();

/** Lit depuis un port (IO)
	\param port une des constantes REG_* d�clar�es dans ports.h, ou une adresse
		� laquelle on soustrait 0xff00 (adresse du premier port).
	\return la valeur du port
	\note peut d�clencher un �v�nement (g�n�r� par l'acc�s sur le bus). Pour
		acc�der directement � un port de fa�on interne, utiliser le tableau
		mem_io (par exemple mem_io[R_LCDC] = 0) ou la macro #REG.
*/
u8 io_read(u16 port);

/** Ecrit sur un port (IO)
	\param port une des constantes REG_* d�clar�es dans ports.h, ou une adresse
		� laquelle on soustrait 0xff00 (adresse du premier port).
	\param value la valeur � �crire
	\note voir #io_readb.
*/
void io_write(u16 port, u8 value);

/** Liste des touches de la Game Boy (voir #io_key_press) */
typedef enum {
	GBK_RIGHT = 0,
	GBK_LEFT,
	GBK_UP,
	GBK_DOWN,
	GBK_A,
	GBK_B,
	GBK_SELECT,
	GBK_START,
} key_t;

/** Simule le changement d'�tat d'une touche de la Game Boy virtuelle.
	\param k touche en question (voir #key_t)
	\param state nouveau statut de la touche (1 = press�, 0 = rel�ch�)
	\note cette fonction peut �tre appel�e par exemple lorsqu'un utilisateur
	a appuy� sur une touche de son clavier.
*/
void io_key_press(key_t k, bool state);

#endif

