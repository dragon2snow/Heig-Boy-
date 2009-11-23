#ifndef OS_SPECIFIC_H
#define OS_SPECIFIC_H

enum {COL_NORMAL = 0x7, COL_RED = 0xc, COL_YELLOW = 0xe, COL_GREEN = 0xa};

/** Définit la couleur du texte
	\param c bits[7..4]: couleur de fond, bits[3..0]: couleur du texte */
void set_text_color(int c);
/** Initialise et démarre l'exécution du son. */
void sound_driver_init();

#endif
