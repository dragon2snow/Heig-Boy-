/** \file lcd.c
	\brief Implémentation de l'affichage du LCD (chip de dessin).
*/
#include "lcd.h"
#include "mem.h"
#include "ports.h"
#include <stdlib.h>
#include <string.h>

/*
	Registres
*/
// Contenu du registre LCDC
typedef struct {
	u8 bg_en: 1, obj_en: 1;
	u8 obj_size: 1;
	u8 bg_map_addr: 1, tile_addr: 1;
	u8 win_en: 1, win_map_addr: 1, enable: 1;
} LCDC_t;
#define lcd_ctrl	(*((LCDC_t*)&REG(LCDC)))
// Ligne courante du balayage LCD
#define cur_line	REG(LY)
#define scroll_x	REG(SCX)
#define scroll_y	REG(SCY)
// Structure des attributs d'un sprite
typedef struct {
	u8 y, x, tile;
	struct obj_attr3_t {
		u8 cgb_pal_num: 3;		// GB Color seulement
		u8 vram_bank: 1;		// GB Color seulement
		u8 pal_num: 1;			// N° de palette (OBP0 / OBP1)
		u8 flip_x: 1, flip_y: 1;	// Retournement (miroir)
		u8 prio: 1;				// Priorité par rapport au BG (0=devant, 1=derrière)
	} attr;
} obj_t;

/*
	Outils
*/
// Modulos communs
#define mod8(x)		((x) & 7)
#define mod32(x)	((x) & 31)
#define mod256(x)	((x) & 255)
// Divisions communes
#define div8(x)		((x) >> 3)

/*
	Buffer écran
*/
// Temporaire: table de conversion des couleurs (GB <-> 32 bits)
//static const u32 color_table[4] = {0xffd3de34, 0xffa5b52b, 0xff70831b, 0xff284440};
//static const u32 color_table[4] = {0xffff00ff, 0xffc000c0, 0xff800080, 0xff400040};
static const u32 color_table[4] = {0xff00e7ff, 0xff009ac7, 0xff004d8f, 0xff000057};
//static const u32 color_table[4] = {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000};
static u8 bg_palette[4], obj_palette[2][4];
/** Bitmap 32 bits de 256x144 pixels */
static u32 *lcd_buffer = NULL;
// Taille d'une ligne de buffer
#define BUFFER_LINE_PITCH 256
// Donne la ligne n° n du buffer
#define lcd_buffer_line(n)	(lcd_buffer + n * BUFFER_LINE_PITCH)

/** Réalise le rendu du fond d'écran: fond uni utilisant la couleur 0 de la
	BGP (palette du BG).
	\param pixel tampon de destination, contenant au moins 176 pixels 32 bits.
	\note Pour toutes les fonctions suivantes prenant un tampon de 176 pixels,
		seuls les 160 pixels du milieu sont visibles, les autres sont utilisés
		pour les débordements éventuels des routines graphiques.
*/
void backdrop_render(u32 *pixel);
/** Réalise le rendu de la ligne courante du BG (background).
	Il s'agit d'une tile map.
	\param pixel tampon de destination, contenant au moins 176 pixels 32 bits.
*/
static void bg_render(u32 *pixel);
/** Réalise le rendu de la fenêtre pour la ligne courante.
	La fenêtre vient remplacer le BG et commence à la position (WX, WY).
	\param pixel tampon de destination, contenant au moins 176 pixels 32 bits.
*/
static void win_render(u32 *pixel);
/** Réalise le rendu des objets pour la ligne courante. Le rendu doit être fait
	en deux passes afin d'intercaler le BG entre les objets (en fonction de
	leur priorité). On appellera donc la fonction deux fois en ayant mis le
	paramètre à la valeur adéquate.
	\param pixel tampon de destination, contenant au moins 176 pixels 32 bits.
	\param skipped_prio 0 pour dessiner les objets au-dessous du BG, 1 pour
		ceux au-dessus
*/
static void obj_render(u32 *pixel, u8 skipped_prio);
/** Dessine un motif de 8 pixels au format GB 2 bits entrelacé sur l'écran.
	\param pixel buffer destination de l'écran (32 bits)
	\param color_table table de traduction de couleur 2 bits <-> 32 bits
	\param palette table of 4 colors to use for mapping the gray shades
	\param tile_ptr pointeur sur le motif
*/
static void draw_tile(u32 *pixel, const u32 *color_table, const u8 *palette, const u8 *tile_ptr);
/** Retourne un motif de 8x8 au format GB 2 bits horizontalement.
	\param in pointeur sur le motif (2 octets)
	\param out destination (2 octets)
*/
static void flip_tile(u8 *out, const u8 *in);
/** Décompose un registre palette en une table de valeurs à utiliser pour
	mapper les niveaux de gris.
	\param out table de destination de 4 éléments
	\param reg un registre palette (BGP, OBP...)
*/
static void translate_palette(u8 *out, u8 reg);

void lcd_draw_line() {
	u32 *dest = lcd_buffer_line(cur_line);
	// Prépare les palettes
	translate_palette(bg_palette, REG(BGP));
	translate_palette(obj_palette[0], REG(OBP0));
	translate_palette(obj_palette[1], REG(OBP1));
	// Fait le rendu à proprement parler
	backdrop_render(dest);
	if (lcd_ctrl.enable) {			// LCD activé
		unsigned i;
		u32 bgwin[160 + 16] = {0};	// Tampon temporaire pour la fusion BG/WIN
		bg_render(bgwin);			// Dessin du BG et de la fenêtre
		win_render(bgwin);			// aplatie par dessus
		// Priorité: objets avec prio=0, BG, fenêtre, objets avec prio=1
		obj_render(dest, 0);
		// Dessine la fusion BG/fenêtre sur le tampon écran
		for (i = 8; i < 168; i++)
			if (bgwin[i] != 0)		// opaque?
				dest[i] = bgwin[i];
		obj_render(dest, 1);
	}
}

void lcd_draw_init() {
	if (!lcd_buffer)
		lcd_buffer = malloc(256 * 144 * sizeof(u32));
}

void backdrop_render(u32 *pixel) {
	unsigned i;
	for (i = 8; i < 168; i++)		// 8 pixels de libre à gauche et à droite
		pixel[i] = color_table[bg_palette[0]];
}

void bg_render(u32 *pixel) {
	// Position (transformée dans le repère écran) du premier pixel à traiter
	unsigned offset_y = mod256(scroll_y + cur_line);
	unsigned tile_offset_x = mod32(div8(scroll_x));
	unsigned i;
	// Calcule l'adresse de la map et du charset en mémoire en fonction des
	// registres de configuration. Ajoute également l'offset depuis le haut de
	// l'écran: 1 ligne pour 8 pixels dans la map, et une ligne (2 octets) par
	// pixel dans le motif, modulo la taille du motif (8 pixels). Voir rapport.
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.bg_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// BG désactivé -> rien à faire
	if (!lcd_ctrl.bg_en)
		return;
	// Tampon écran - laisse 8 pixels de libre à gauche pour dépassement
	pixel = pixel + 8 - mod8(scroll_x);
	// Dessin de la map à proprement parler
	for (i = 0; i < 21; i++) {
		// Lit la map pour obtenir le n° de motif à cet endroit
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		// Si le mode est 1, le n° de motif est signé, sinon non signé
		int tile_no = lcd_ctrl.tile_addr ? (u8)tile_val : (s8)tile_val;
		// Dessin du motif (ajout du n° de tile * 16 octets par motif)
		draw_tile(pixel, color_table, bg_palette, tile_data + tile_no * 16);
		pixel += 8;
	}
}

void win_render(u32 *pixel) {
	// Position (transformée dans le repère écran) du premier pixel à traiter
	int offset_x = REG(WX) - 7, offset_y = cur_line - REG(WY);
	unsigned tile_offset_x = 0;
	// Calcule l'adresse des objets en mémoire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.win_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Fenêtre désactivée ou ligne courante en dehors
	if (!lcd_ctrl.win_en || offset_y < 0)
		return;
	// Laisse les 8 pixels de libre à gauche pour le dépassement
	pixel += 8;
	// Efface tout ce qui a été dessiné par le BG à partir du commencement
	// de la fenêtre.
	memset(pixel + offset_x, 0, 4 * (160 - offset_x));
	// Dessine jusqu'à la fin de la fenêtre (toujours tout à droite de l'écran)
	while (offset_x < 160) {
		// Voir bg_render pour plus d'infos
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (u8)tile_val : (s8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// Dessin + avancement de 8 pixels
		draw_tile(pixel + offset_x, color_table, bg_palette, tile_ptr);
		offset_x += 8;
	}
}

void obj_render(u32 *pixel, u8 skipped_prio) {
	// Liste d'attributs des sprites
	obj_t *oam = (obj_t*)mem_oam;
	// En mode 8x16 (obj_size=1), le dernier bit de la tile est ignoré
	u8 obj_height = 8 + lcd_ctrl.obj_size * 8;
	u8 tile_mask = ~lcd_ctrl.obj_size;
	int i;
	if (!lcd_ctrl.obj_en)			// Comme d'hab, fonction désactivée
		return;
	/*	Le système de priorités de la GB est très compliqué; il implique de
		trier les sprites par position X. A la place on va émuler le système
		comme sur la Game Boy Color (selon le n° de sprite, le premier ayant
		la plus grosse priorité). Ca ne devrait pas poser de problème car
		la Game Boy Color reste compatible avec la Game Boy malgré cela. */
	for (i = 39; i >= 0; i--) {
		// 4 octets par objet le décrivant (position, motif, etc.)
		// Note: la GB soustrait 16 à la position y et 8 à la position x
		u8 y = cur_line - oam[i].y + 16;	// ligne courante à dessiner
		// Pas besoin de -8, le buffer a déjà 8 pixels invisibles à gauche
		u8 x = oam[i].x;
		u8 *tile_ptr = mem_vram + 16 * (oam[i].tile & tile_mask);
		struct obj_attr3_t attr = oam[i].attr;
		u8 pattern[2];
		if (attr.prio == skipped_prio)	// pas la priorité voulue
			continue;
		if (y >= obj_height)	// en fait le test devrait avoir || y < 0 mais
			continue;			// comme y est non signé il revient à 255
		if (x == 0 || x >= 168)			// invisible
			continue;
		if (attr.flip_y)		// retournement vertical
			y = obj_height - 1 - y;	
		tile_ptr += y * 2;		// plus bas dans le motif
		if (attr.flip_x)		// Retourne éventuellement le motif
			flip_tile(pattern, tile_ptr);
		else
			pattern[0] = tile_ptr[0], pattern[1] = tile_ptr[1];
		// Finalement, dessine le motif
		draw_tile(pixel + x, color_table, obj_palette[attr.pal_num], pattern);
	}
}

void flip_tile(u8 *out, const u8 *in) {
	// Inverse les bits du motif 2x8 bits entrelacé
	out[0] =
		 in[0] >> 7         | (in[0] & 0x40) >> 5 |
		(in[0] & 0x20) >> 3 | (in[0] & 0x10) >> 1 |
		(in[0] & 0x08) << 1 | (in[0] & 0x04) << 3 |
		(in[0] & 0x02) << 5 |  in[0] << 7;
	out[1] =
		 in[1] >> 7         | (in[1] & 0x40) >> 5 |
		(in[1] & 0x20) >> 3 | (in[1] & 0x10) >> 1 |
		(in[1] & 0x08) << 1 | (in[1] & 0x04) << 3 |
		(in[1] & 0x02) << 5 |  in[1] << 7;
}

void draw_tile(u32 *pixel, const u32 *color_table, const u8 *palette, const u8 *tile_ptr) {
	unsigned j;
	// Les pixels sont sur 2 plans: le premier octet indique le plan
	// "foncé", le deuxième le plan "clair". Aucun = blanc, les 2 = noir.
	u8 pat1 = tile_ptr[0], pat2 = tile_ptr[1];
	for (j = 0; j < 8; j++) {
		u8 color = (pat2 & 0x80) >> 6 | (pat1 & 0x80) >> 7;
		if (color != 0)			// couleur 0 = transparente
			*pixel = color_table[palette[color]];
		pixel++, pat1 <<= 1, pat2 <<= 1;
	}
}

void translate_palette(u8 *out, u8 reg) {
	out[3] = reg >> 6 & 3;
	out[2] = reg >> 4 & 3;
	out[1] = reg >> 2 & 3;
	out[0] = reg      & 3;
}

void lcd_copy_to_buffer(u32 *dest, int destWidth) {
	int i, j;
	if (destWidth < 160)		// Argument invalide
		return;
	for (i = 0; i < 144; i++) {
		u32 *pixel = lcd_buffer_line(i) + 8;
		for (j = 0; j < 160; j++)
			*dest++ = *pixel++;
		dest += destWidth - 160;
	}
}
