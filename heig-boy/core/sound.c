/**	\file sound.c
	\brief impl�mentation de l'APU (Audio Processing Unit) de la Game Boy
*/
#include "sound.h"
#include "ports.h"
#include <string.h>
#include <stdio.h>		// temp

#define SAMPLE_RATE 44100

// Configuration des canaux quadrangulaires (1 et 2)
typedef struct {
	struct {
		u8 sweep_shift: 3;	// n
		u8 sweep_op: 1;		// 0: add, 1: sub
		u8 sweep_time: 3;
	};
	struct {
		u8 sound_length: 6;
		u8 wave_duty: 2;
	};
	struct {
		u8 vol_shift: 3;	// n, 0: stop
		u8 vol_op: 1;		// 0: sub, 1: add
		u8 vol_initial: 4;	// initial volume, 0: no sound
	};
	u8 freq_lo;
	struct {
		u8 freq_hi: 3;
		u8 dummy: 3;
		u8 consecutive: 1;
		u8 restart: 1;
	};
} tone_channel_t;

// Configuration de l'onde volontaire (voluntary wave)
typedef struct {
	struct {		// 1=disable, 0=playback
		u8 dummy1: 7, disable: 1;
	};
	u8 length;
	struct {		// 0=off, 1=100% (0dB), 2=50% (-3dB), 3=25% (-6dB)
		u8 dummy2: 6, volume: 2;
	};

} voluntary_wave_channel_t;

#define ch1		(*((tone_channel_t*)&REG(NR10)))
#define ch2		(*((tone_channel_t*)&REG(NR20)))

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

/** Variables utilis�es pour le playback de la voluntary wave et du g�n�rateur
	de bruit blanc */
typedef struct {
} other_channel_vars_t;

tone_channel_vars_t tone_ch[2];	// Pour les 2 canaux




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
		case R_NR30:		// Voluntary wave enable
			break;
	}
}

/** Met � jour la fr�quence d'un des canaux quadrangulaires (ch1/2)
	\param ch canal � affecter
	\param freq nouvelle fr�quence � �crire
*/
static void tone_channel_write_freq(tone_channel_t *ch, u16 freq) {
	ch->freq_lo = freq & 0xff;		// 8 bits du bas dans freq_lo (nrx3)
	ch->freq_hi = (freq >> 8) & 7;	// 3 bits du haut dans freq_hi (nrx4)
}

/** Lit la frq�uence d'un des canaux quadrangulaires (ch1/2)
	 \param ch canal duquel lire la fr�quence
	 \return valeur 11 bits repr�sentant la fr�quence lue dans les registres
*/
static u16 tone_channel_read_freq(tone_channel_t *ch) {
	return ch->freq_lo + (ch->freq_hi << 8);
}

/** R�alise le rendu des canaux 1 et 2
	\param vars variables de rendu du canal
	\param buf tampon de sortie
	\param len nombre d'�chantillons (voir #sound_render)
	\note les valeurs produites sont additionn�es aux valeurs actuellement dans
	le tampon. On peut ainsi cascader les appels pour mixer plusieurs canaux
	ensemble.
*/
static void tone_channel_render(tone_channel_vars_t *vars,
								s16 *buf, unsigned len) {
	tone_channel_t *ch = vars->channel;
	/* La fr�quence r�elle est calcul�e en fonction de la fr�quence �crite sur
	   le registre. Si x est la fr�quence �crite sur le registre, la fr�quence
	   r�elle est calcul�e comme suit:
	   freq = 131072/(2048-x) Hz
	*/
	u16 read_freq = tone_channel_read_freq(ch);
	u16 freq = 131072 / (2048 - read_freq);
	// G�n�re des �chantillons 16 bits st�r�o tant que n�cessaire
	while (len--) {
		s16 data = 0;			// Valeur produite
		if (vars->len_ctr < vars->len_time) {		// Son pas expir�
			// G�n�re un son depuis le motif choisi (duty)
			data = 1000 * tone_wave_pat[ch->wave_duty][freq_div(vars->cur_sample) % 8] * vars->cur_volume;
			// Avance le pointeur � l'int�rieur du tableau. Avec une fr�quence
			// de SAMPLE_RATE, cur_sample est additionn� d'un (en fixed point).
			vars->cur_sample += 8 * freq_mul(freq) / SAMPLE_RATE;
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
		*buf++ += data;			// Ajoute les valeurs produites dans le tampon
		*buf++ += data;
	}
}

static void tone_channel_init(tone_channel_vars_t *vars, tone_channel_t *channel) {
	memset(vars, 0, sizeof(*vars));
	vars->channel = channel;
}

void sound_render(s16 *buf, unsigned len) {
	memset(buf, 0, len * sizeof(u16) * 2);
	tone_channel_render(&tone_ch[0], buf, len);
	tone_channel_render(&tone_ch[1], buf, len);
}

void sound_init() {
	tone_channel_init(&tone_ch[0], &ch1);
	tone_channel_init(&tone_ch[1], &ch2);
}
