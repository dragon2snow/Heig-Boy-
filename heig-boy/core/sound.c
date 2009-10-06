/**	\file sound.c
	\brief impl�mentation de l'APU (Audio Processing Unit) de la Game Boy
*/
#include "sound.h"
#include "ports.h"
#include <string.h>		// memcpy
#include <stdlib.h>		// rand
#include <stdio.h>		// temp

#define SAMPLE_RATE 44100		// samples per second
//#define BASE_AMPL   1000		// base amplitude
#define BASE_AMPL   600			// base amplitude

// Configuration des canaux quadrangulaires (1 et 2)
typedef struct {
	struct {				// sweep (balayage fr�quence)
		u8 sweep_shift: 3;	// n
		u8 sweep_op: 1;		// 0: add, 1: sub
		u8 sweep_time: 3;
	};
	struct {				// taille du son
		u8 sound_length: 6;
		u8 wave_duty: 2;
	};
	struct {				// enveloppe volume
		u8 vol_shift: 3;	// n, 0: stop
		u8 vol_op: 1;		// 0: sub, 1: add
		u8 vol_initial: 4;	// initial volume, 0: no sound
	};
	u8 freq_lo;				// fr�quence
	struct {
		u8 freq_hi: 3;
		u8 dummy: 3;
		u8 consecutive: 1;	// son de taille max sound_length
		u8 restart: 1;		// red�marrer le son
	};
} tone_channel_t;

// Configuration de l'onde volontaire (voluntary wave)
typedef struct {
	struct {		// 1=disable, 0=playback
		u8 dummy1: 7, disable: 1;
	};
	u8 length;
	struct {		// 0=off, 1=100% (0dB), 2=50% (-3dB), 3=25% (-6dB)
		u8 dummy2: 5, volume: 2, dummy3: 1;
	};
	u8 freq_lo;
	struct {
		u8 freq_hi: 3, dummy4: 3;
		u8 consecutive: 1;
		u8 restart: 1;
	};
} wave_channel_t;

// Configuration du g�n�rateur de bruit blanc
typedef struct {
	struct {		// NR41
		u8 sound_length: 6, dummy1: 2;
	};
	struct {
		u8 vol_shift: 3;		// n, 0: stop
		u8 vol_op: 1;			// 0: sub, 1: add
		u8 vol_initial: 4;		// initial volume, 0: no sound
	};
	struct {					// compteur polynomial
		u8 div_freq: 3;			// diviseur de fr�quence r
		u8 counter_width: 1;	// 0: 15 bits, 1: 7 bits
		u8 shift_freq: 4;		// diviseur de fr�quence s
	};
	struct {
		u8 dummy2: 6;
		u8 consecutive: 1;
		u8 restart: 1;
	};
} noise_channel_t;

typedef struct {
	struct {		// NR50 (volume & vin pas �mul�)
		u8 so1_vol: 3, so1_vin: 1;
		u8 so2_vol: 3, so2_vin: 1;
	};
	struct {		// NR51 (activation des canaux � gauche/droite)
		u8 so1_en1: 1, so1_en2: 1, so1_en3: 1, so1_en4: 1;
		u8 so2_en1: 1, so2_en2: 1, so2_en3: 1, so2_en4: 1;
	};
	struct {
		// Etat de la lecture des canaux, lecture seule
		u8 ch1_playing: 1, ch2_playing: 1, ch3_playing: 1, ch4_playing: 1;
		u8 dummy: 3, sound_enable: 1;		// Master enable
	};
} control_channel_t;

// Canaux dans la zone I/O
#define ch1			(*((tone_channel_t*)&REG(NR10)))
#define ch2			(*((tone_channel_t*)&REG(NR20)))
#define ch3			(*((wave_channel_t*)&REG(NR30)))
#define ch4			(*((noise_channel_t*)&REG(NR41)))
#define ch5			(*((control_channel_t*)&REG(NR50)))
// M�moire o� sont stock�es les donn�es PCM
#define wave_data	(mem_io + 0x30)

// Variables utilis�es pour le rendu d'un canal quadrangulaire
typedef struct {
	tone_channel_t *channel;	// Canal � traiter
	u32 cur_sample;				// Pour la modulation de fr�quence
	u8 cur_volume;				// Volume de l'enveloppe
	u16 len_time, vol_time,
		sweep_time;				// Temps pour atteindre un �v�nement donn�
	u16 len_ctr, vol_ctr,
		sweep_ctr;				// Compteurs de temps (en samples)
} tone_channel_vars_t;

// Variables utilis�es pour le playback de la voluntary wave
typedef struct {
	wave_channel_t *channel;	// Canal � traiter
	u32 cur_sample;				// Pour la modulation de fr�quence
	u16 len_time;				// Temps avant la fin du son
	u16 len_ctr;				// Compteur de temps (en samples)
} wave_channel_vars_t;

// Variables utilis�es pour le rendu du g�n�rateur de bruit blanc
typedef struct {
	noise_channel_t *channel;
	u32 cur_sample;				// Pour la modulation de fr�quence
	u8 cur_volume;				// Volume de l'enveloppe
	u16 len_time, vol_time;		// Temps avant le prochain �v�nement
	u16 len_ctr, vol_ctr;		// Compteur de temps (en samples)
	u8 rand_data[32768 / 8];	// Table de donn�es al�atoires
} noise_channel_vars_t;

tone_channel_vars_t tone_ch[2];	// Pour les 2 canaux
wave_channel_vars_t wave_ch;	// Pour l'onde (PCM)
noise_channel_vars_t noise_ch;	// Pour le bruit blanc

/** Motifs carr�s g�n�r�s en fonction de la s�lection du duty (nrx1) */
static u8 tone_wave_pat[4][8] = {
	{1, 0, 0, 0, 0, 0, 0, 0},		// 00: 12.5%
	{1, 1, 0, 0, 0, 0, 0, 0},		// 01: 25%
	{1, 1, 1, 1, 0, 0, 0, 0},		// 10: 50%
	{1, 1, 1, 1, 1, 1, 0, 0},		// 11: 75%
};

// Pour moduler la fr�quence on utilise une variable de plus grande taille
#define freq_div(x)		((x) >> 16)
#define freq_mul(x)		((x) << 16)
#define freq_clamp(x)	min(x, 65535)		// pour �viter les d�passements

/** Met � jour la fr�quence d'un des canaux quadrangulaires (ch1/2)
	\param ch canal � affecter
	\param freq nouvelle fr�quence � �crire
*/
static void tone_channel_write_freq(tone_channel_t *ch, u16 freq) {
	ch->freq_lo = freq & 0xff;		// 8 bits du bas dans freq_lo (nrx3)
	ch->freq_hi = (freq >> 8) & 7;	// 3 bits du haut dans freq_hi (nrx4)
}

/** Lit la fr�quence d'un des canaux quadrangulaires ou PCM (ch1 � 3)
	 \param ch canal duquel lire la fr�quence
	 \return valeur 11 bits repr�sentant la fr�quence lue dans les registres
	 \note cette fonction peut aussi traiter un canal #wave_channel_t. Utilisez un
		cast dans ce cas.
*/
static u16 tone_channel_read_freq(tone_channel_t *ch) {
	return ch->freq_lo + (ch->freq_hi << 8);
}

/** R�alise le rendu des canaux 1 et 2
	\param vars variables de rendu du canal
	\param buf tampon de sortie
	\param len nombre d'�chantillons (voir #sound_render)
	\note ceci produit du son MONO 16 bits 44 kHz et non st�r�o
*/
static void tone_channel_render(tone_channel_vars_t *vars,
								s16 *buf, unsigned len) {
	tone_channel_t *ch = vars->channel;
	/* La fr�quence r�elle est calcul�e en fonction de la fr�quence �crite sur
	   le registre. Si x est la fr�quence �crite sur le registre, la fr�quence
	   r�elle est calcul�e comme suit:
	   freq = 131072/(2048-x) Hz
	   Pour un "tick" la Game Boy g�n�re en fait un motif de 8 samples (cf.
	   tone_wave_pat) donc la fr�quence d'�chantillonnage est 8x plus �lev�e.
	*/
	u16 read_freq = tone_channel_read_freq(ch);
	u16 freq = freq_clamp(8 * 131072 / (2048 - read_freq));
	// G�n�re des �chantillons 16 bits st�r�o tant que n�cessaire
	while (len--) {
		s16 data = 0;			// Valeur produite
		if (vars->len_ctr < vars->len_time) {		// Son pas expir�
			// G�n�re un son depuis le motif choisi (duty)
			data = BASE_AMPL * vars->cur_volume *
				tone_wave_pat[ch->wave_duty][freq_div(vars->cur_sample) % 8];
			// Avance le pointeur � l'int�rieur du tableau. Avec une fr�quence
			// de SAMPLE_RATE, cur_sample est additionn� d'un (en fixed point).
			vars->cur_sample += freq_mul(freq) / SAMPLE_RATE;
			// Ev�nement de fin du son?
			vars->len_ctr++;
			if (vars->len_ctr == vars->len_time) {
				if (!ch->consecutive)		// R�p�ter?
					vars->len_ctr = 0;		// C'est reparti pour un tour
			}
			// Pareil pour l'enveloppe (si active)
			if (vars->vol_time) {
				vars->vol_ctr++;
				if (vars->vol_ctr == vars->vol_time) {
					if (ch->vol_op) {		// incr�menter
						if (vars->cur_volume < 15)
							vars->cur_volume++;
					}
					else {					// d�cr�menter
						if (vars->cur_volume > 0)
							vars->cur_volume--;
					}
					vars->vol_ctr = 0;
				}
			}
			// Pour finir le sweep
			if (vars->sweep_time) {
				vars->sweep_ctr++;
				if (vars->sweep_ctr > vars->sweep_time) {
					// Formule � chaque shift: X(t) = X(t-1) +/- X(t-1)/2^n
					if (ch->sweep_op)		// soustraction
						read_freq -= read_freq / (1 << ch->sweep_shift);
					else					// addition
						read_freq += read_freq / (1 << ch->sweep_shift);
					// Met � jour la fr�quence et la recalcule
					tone_channel_write_freq(ch, read_freq);
					freq = 131072 / (2048 - read_freq);
					vars->sweep_ctr = 0;
				}
			}
		}
		*buf++ = data;			// Ecrit la valeur produite dans le tampon
	}
}

/** Initialise les variables internes pour le rendu d'un canal quadrangulaire.
	\param vars objet contenant les variables du g�n�rateur
	\param channel adresse mat�rielle du canal � g�rer
*/
static void tone_channel_init(tone_channel_vars_t *vars, tone_channel_t *channel) {
	memset(vars, 0, sizeof(*vars));
	vars->channel = channel;
}

/** R�alise le rendu du canal 3 (PCM)
	\param vars variables de rendu du canal
	\param buf tampon de sortie
	\param len nombre d'�chantillons (voir #sound_render)
	\note ceci produit du son MONO 16 bits 44 kHz et non st�r�o
*/
static void wave_channel_render(wave_channel_vars_t *vars,
								s16 *buf, unsigned len) {
	wave_channel_t *ch = vars->channel;
	/* Voyez #tone_channel_render pour plus d'informations. Ici la fr�quence
	   est calcul�e comme suit:
	   freq = 65536/(2048-x) Hz
	   Comme pour le square generator, 1 tick ("Hz") repr�sente le jeu de toute
	   la pattern RAM, soit 32 samples.
	*/
	u16 freq = freq_clamp(32 * 65536 /
		(2048 - tone_channel_read_freq((tone_channel_t*)ch)));
	// Cf. pandocs, registre NR32 (r�glage du volume)
	// Si ch->volume = 0, ch->volume - 1 vaudra -1, et en non sign� cela
	// donnera 0xff (d�calage suffisamment grand pour �teindre le son).
	u8 volume = (ch->volume - 1) & 0xff;
	// Phase de g�n�ration
	while (len--) {
		s16 data = 0;			// Valeur produite
		if (vars->len_ctr < vars->len_time) {
			// Lit une donn�e depuis la wave pattern RAM (4 bits)
			u8 patno = freq_div(vars->cur_sample) % 32;
			u8 pat = wave_data[patno / 2];
			if (patno % 2 == 0)		// bits du haut
				pat >>= 4;
			else					// bits du bas
				pat &= 0xf;
			data = BASE_AMPL * pat >> volume;
			// Le m�canisme ensuite est le m�me que tone_channel_render
			vars->cur_sample += freq_mul(freq) / SAMPLE_RATE;
			// Fin du son?
			if (++vars->len_ctr == vars->len_time && !ch->consecutive)
				vars->len_ctr = 0;
		}
		*buf++ = data;			// Ajoute les valeurs produites au tampon
	}
}

/** Initialise les variables internes pour le rendu du canal PCM.
	\param vars objet contenant les variables du g�n�rateur
	\param channel adresse mat�rielle du canal � g�rer
*/
static void wave_channel_init(wave_channel_vars_t *vars, wave_channel_t *channel) {
	memset(vars, 0, sizeof(*vars));
	vars->channel = channel;
}

/** Permet d'�crire un bit sur un tableau de bits
	\param table tableau d'au moins bitno �l�ments 1 bit
	\param bitno num�ro du bit � affecter
	\param value nouvelle valeur � attribuer au bit (1 ou 0)
*/
static void noise_set_bit(u8 *table, u16 bitno, u8 value) {
	if (value)
		table[bitno / 8] |= 1 << (bitno % 8);		// set
	else
		table[bitno / 8] &= ~(1 << (bitno % 8));	// clear
}

/** Permet de lire un bit sur un tableau de bits
	\param table tableau d'au moins bitno �l�ments 1 bit
	\param bitno num�ro du bit � r�cup�rer
*/
static u8 noise_get_bit(u8 *table, u16 bitno) {
	return table[bitno / 8] >> (bitno % 8) & 1;
}

/** R�alise le rendu du canal 4 (bruit blanc)
	\param vars variables de rendu du canal
	\param buf tampon de sortie
	\param len nombre d'�chantillons (voir #sound_render)
	\note ceci produit du son MONO 16 bits 44 kHz et non st�r�o
*/
static void noise_channel_render(noise_channel_vars_t *vars,
								 s16 *buf, unsigned len) {
	noise_channel_t *ch = vars->channel;
	u16 width = ch->counter_width ? 127 : 32767;		// NR43.bit3
	// Frequency = 524288 Hz / r / 2^(s+1) ; For r=0 assume r=0.5 instead
	u32 freq;
	if (ch->div_freq != 0)
		freq = 524288 / ch->div_freq / (2 << ch->shift_freq);
	else
		freq = 524288 * 2 / (2 << ch->shift_freq);
	freq = freq_clamp(freq);
	// Le fonctionnement est le m�me partout, voir tone_channel_render
	while (len--) {
		s16 data = 0;
		if (vars->len_ctr < vars->len_time) {
			// G�n�re une donn�e al�atoire
			data = BASE_AMPL * vars->cur_volume *
				noise_get_bit(vars->rand_data, freq_div(vars->cur_sample) & width);
			vars->cur_sample += freq_mul(freq) / SAMPLE_RATE;
			// Gestion de l'enveloppe (si active)
			if (vars->vol_time) {
				vars->vol_ctr++;
				if (vars->vol_ctr == vars->vol_time) {
					if (ch->vol_op) {		// incr�menter
						if (vars->cur_volume < 15)
							vars->cur_volume++;
					}
					else {					// d�cr�menter
						if (vars->cur_volume > 0)
							vars->cur_volume--;
					}
					vars->vol_ctr = 0;
				}
			}
			// Fin du son
			if (++vars->len_ctr == vars->len_time && !ch->consecutive)
				vars->len_ctr = 0;
		}
		*buf++ = data;			// Ajoute les valeurs produites au tampon
	}
}

/** Initialise les variables internes pour le rendu du canal de bruit.
	\param vars objet contenant les variables du g�n�rateur
	\param channel adresse mat�rielle du canal � g�rer
*/
static void noise_channel_init(noise_channel_vars_t *vars, noise_channel_t *channel) {
	unsigned i;
	u8 data = 0;
	memset(vars, 0, sizeof(*vars));
	vars->channel = channel;
	// FIXME
	vars->len_time = SAMPLE_RATE / 256;
	// G�n�re des donn�es al�atoires pour le son
	srand(100);			// donne bien selon les tests
	for (i = 0; i < 32768; i++) {
		// Le bruit est obtenu en invers�ment al�atoirement l'�tat de la sortie
		if (rand() % 2)
			data ^= 1;
		noise_set_bit(vars->rand_data, i, data);
	}
}

void sound_render(s16 *buf, unsigned len) {
	// 4 buffers temporaires pour le mix (allou�s sur la pile)
	s16 *mix_buf1 = alloca(sizeof(s16) * len),
		*mix_buf2 = alloca(sizeof(s16) * len),
		*mix_buf3 = alloca(sizeof(s16) * len),
		*mix_buf4 = alloca(sizeof(s16) * len);
	// Comme les ET logiques peuvent �tre faits rapidement, on va pr�calculer
	// des masques pour mettre � z�ro les sons inactifs
	s16 mask11 = ch5.so1_en1 ? 0xffff : 0, mask21 = ch5.so2_en1 ? 0xffff : 0,
		mask12 = ch5.so1_en2 ? 0xffff : 0, mask22 = ch5.so2_en2 ? 0xffff : 0,
		mask13 = ch5.so1_en3 ? 0xffff : 0, mask23 = ch5.so2_en3 ? 0xffff : 0,
		mask14 = ch5.so1_en4 ? 0xffff : 0, mask24 = ch5.so2_en4 ? 0xffff : 0;
	if (!ch5.sound_enable)		// Son d�sactiv�
		return;
	// Rend le son dans les buffers
	tone_channel_render(&tone_ch[0], mix_buf1, len);
	tone_channel_render(&tone_ch[1], mix_buf2, len);
	wave_channel_render(&wave_ch, mix_buf3, len);
	noise_channel_render(&noise_ch, mix_buf4, len);
	// Mixe les canaux sonores
	while (len--) {
		*buf++ =			// Canal gauche
			(*mix_buf1 & mask11) +
			(*mix_buf2 & mask12) +
			(*mix_buf3 & mask13) +
			(*mix_buf4 & mask14);
		*buf++ =			// Canal droit
			(*mix_buf1++ & mask21) +
			(*mix_buf2++ & mask22) +
			(*mix_buf3++ & mask23) +
			(*mix_buf4++ & mask24);
	}
}

void sound_init() {
	tone_channel_init(&tone_ch[0], &ch1);
	tone_channel_init(&tone_ch[1], &ch2);
	wave_channel_init(&wave_ch, &ch3);
	noise_channel_init(&noise_ch, &ch4);
}

u8 sound_readb(u16 port) {
	// TODO
	return 0xff;
}

void sound_writeb(u16 port, u8 value) {
	mem_io[port] = value;
	// G�n�ralisation pour le g�n�rateur de fr�quence (tone)
	if (port >= R_NR10 && port < R_NR30 && port != R_NR20) {
		// S�lection du canal en fonction du port vis� (5 ports par canal)
		tone_channel_vars_t *vars = &tone_ch[(port - R_NR10) / 5];
		tone_channel_t *channel = vars->channel;
		port = (port - R_NR10) % 5;
		switch (port) {
			case 0:				// nrX0: sweep (ch1 only)
				vars->sweep_time = SAMPLE_RATE * channel->sweep_time / 128;
				vars->sweep_ctr = 0;
				break;
			case 1:				// nrX1: length & duty
				// Sound Length = (64-t1)*(1/256) seconds
				vars->len_time = SAMPLE_RATE * (64 - channel->sound_length) / 256;
				vars->len_ctr = 0;
				break;
			case 2:				// nrX2: volume envelope
				// 1 step = n*(1/64) seconds
				vars->vol_time = channel->vol_shift * SAMPLE_RATE / 64;
				vars->vol_ctr = 0;
				vars->cur_volume = channel->vol_initial;
				break;
			case 3:
			case 4:				// nrX3/4: fr�quence
				// Reprend le son au d�but
				vars->len_ctr = 0;
				// Recommence le son
				if (channel->restart) {
					vars->cur_sample = 0;
					vars->cur_volume = channel->vol_initial;
					vars->vol_ctr = vars->sweep_ctr = 0;
					channel->restart = 0;
				}
				break;
		}
		return;
	}

	switch (port) {
		case R_NR31:		// PCM volume
			// Sound Length = (256-t1)*(1/256) seconds
			wave_ch.len_time = SAMPLE_RATE * (256 - ch3.length) / 256;
			wave_ch.len_ctr = 0;
			break;
		case R_NR33:
		case R_NR34:		// PCM fr�quence
			// Reprend le son au d�but
			wave_ch.len_ctr = 0;
			break;
		case R_NR41:		// longueur du bruit
			// Sound Length = (64-t1)*(1/256) seconds
			noise_ch.len_time = SAMPLE_RATE * (64 - ch4.sound_length) / 256;
			break;
		case R_NR42:		// volume envelope
			// 1 step = n*(1/64) seconds
			noise_ch.vol_time = ch4.vol_shift * SAMPLE_RATE / 64;
			noise_ch.vol_ctr = 0;
			noise_ch.cur_volume = ch4.vol_initial;
			break;
		case R_NR44:		// red�marrage du son
			noise_ch.len_ctr = 0;
			if (ch4.restart) {
				noise_ch.cur_sample = 0;
				noise_ch.cur_volume = ch4.vol_initial;
				noise_ch.vol_ctr = 0;
			}
			break;
	}
}
