/** \file lcd.c
	\brief Implémentation de l'affichage du LCD (chip de dessin).
*/
#include "lcd.h"
#include "mem.h"
#include "ports.h"
#include "../color-it/user.h"
#include <stdlib.h>
#include <string.h>

/*
	Registres
*/
/** Contenu du registre LCDC. */
typedef struct {
	u8 bg_en: 1, obj_en: 1;
	u8 obj_size: 1;
	u8 bg_map_addr: 1, tile_addr: 1;
	u8 win_en: 1, win_map_addr: 1, enable: 1;
} LCDC_t;
#define lcd_ctrl	(*((LCDC_t*)&REG(LCDC)))
/** Ligne courante du balayage LCD */
#define cur_line	REG(LY)
/** Défilement */
#define scroll_x	REG(SCX)
#define scroll_y	REG(SCY)
/** Structure représentant les attributs d'un sprite. */
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
static u8 bg_palette[4], obj_palette[2][4];
/** Ligne courante de dessin de la fenêtre */
static u8 win_line;
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
static void backdrop_render(u32 *pixel);
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
	\param write_transparent indique qu'il faut écrire les pixels de couleur
		zéro (transparents) s'il n'y a encore rien en dessous
*/
static void draw_tile(u32 *pixel, const u32 *color_table, const u8 *palette,
					  const u8 *tile_ptr, bool write_transparent);
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
		obj_render(dest, 0);		// Dessin des objets de basse priorité
		bg_render(dest);			// Dessin du BG et de la fenêtre
		win_render(dest);			// aplatie par dessus
		obj_render(dest, 1);		// Et finalement les objets de haute prio.
	}
}

void lcd_draw_init() {
	if (!lcd_buffer)
		lcd_buffer = malloc(256 * 144 * sizeof(u32));
}

void lcd_begin() {
	win_line = 0;
}

void backdrop_render(u32 *pixel) {
	unsigned i;
	for (i = 8; i < 168; i++)		// 8 pixels de libre à gauche et à droite
		pixel[i] = ColorIt_palette[bg_palette[0]];
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
	u8 *tile_data = mem_vram + mod8(offset_y) * 2,
		*custom_data = ColorIt_tileData + mod8(offset_y) * 2;
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
		u8 tile_val = bg_map[mod32(tile_offset_x++)], *data = tile_data;
		// Si le mode est 1, le n° de motif est signé, sinon non signé
		u16 tile_no = lcd_ctrl.tile_addr ? (u8)tile_val : (256 + (s8)tile_val);
		// Calcul de la palette en tenant compte de ColorIt
		u32 *palette = ColorIt_palette + 4 * ColorIt_tilePalette[tile_no];
		// Tile perso ColorIt => utilise un autre jeu de données
		if (ColorIt_tileCustom[tile_no] != 0xffff) {
			data = custom_data;
			tile_no = ColorIt_tileCustom[tile_no];
		}
		// Dessin du motif (ajout du n° de tile * 16 octets par motif)
		draw_tile(pixel, palette, bg_palette, data + tile_no * 16, true);
		pixel += 8;
	}
}

void win_render(u32 *pixel) {
	// Position (transformée dans le repère écran) du premier pixel à traiter
	int offset_x = REG(WX) - 7;
	unsigned tile_offset_x = 0;
	// Calcule l'adresse des objets en mémoire, ici offset_y = win_line
	u8 *tile_data = mem_vram + mod8(win_line) * 2,
		*custom_data = ColorIt_tileData + mod8(win_line) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.win_map_addr ? 0x1C00 : 0x1800) +
		div8(win_line) * 32;
	// Fenêtre désactivée ou ligne courante en dehors
	if (!lcd_ctrl.win_en || cur_line < REG(WY) || offset_x >= 160)
		return;
	// Laisse les 8 pixels de libre à gauche pour le dépassement
	pixel += 8;
	// La fenêtre est un peu spéciale: son n° de ligne n'est incrémenté que si
	// elle est visible...
	win_line++;
	// Efface tout ce qui a été dessiné par le BG à partir du commencement
	// de la fenêtre.
	memset(pixel + offset_x, 0, 4 * (160 - offset_x));
	// Dessine jusqu'à la fin de la fenêtre (toujours tout à droite de l'écran)
	while (offset_x < 160) {
		// Voir bg_render pour plus d'infos
		u8 tile_val = bg_map[mod32(tile_offset_x++)], *data = tile_data;
		u16 tile_no = lcd_ctrl.tile_addr ? (u8)tile_val : (256 + (s8)tile_val);
		// Calcul de la palette en tenant compte de ColorIt
		u32 *palette = ColorIt_palette + 4 * ColorIt_tilePalette[tile_no];
		// Tile perso ColorIt => utilise un autre jeu de données
		if (ColorIt_tileCustom[tile_no] != 0xffff) {
			data = custom_data;
			tile_no = ColorIt_tileCustom[tile_no];
		}
		// Dessin + avancement de 8 pixels
		draw_tile(pixel + offset_x, palette, bg_palette, data + 16 * tile_no, true);
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
		u8 x = oam[i].x, tile = oam[i].tile & tile_mask;
		u8 pattern[2], *tile_ptr = mem_vram;
		u32 *palette;
		struct obj_attr3_t attr = oam[i].attr;
		if (attr.prio == skipped_prio)
			continue;			// Pas la priorité voulue
		if (y >= obj_height)	// En fait le test devrait avoir || y < 0 mais
			continue;			// comme y est non signé il revient à 255
		if (x == 0 || x >= 168)	// Invisible...
			continue;
		// Retournement vertical
		if (attr.flip_y)
			y = obj_height - 1 - y;
		// Calcule la palette en tenant compte de ColorIt
		palette = ColorIt_palette + 4 * ColorIt_tilePalette[tile + y / 8];
		// Tile perso?
		if (ColorIt_tileCustom[tile + y / 8] != 0xffff) {
			tile = ColorIt_tileCustom[tile + y / 8] & tile_mask;
			tile_ptr = ColorIt_tileData;
		}
		// Pointe sur la bonne tile, avec un offset de y lignes
		tile_ptr += 16 * tile + y * 2;
		// Retourne éventuellement le motif
		if (attr.flip_x)
			flip_tile(pattern, tile_ptr);
		else
			pattern[0] = tile_ptr[0], pattern[1] = tile_ptr[1];
		// Finalement, dessine le motif
		draw_tile(pixel + x, palette, obj_palette[attr.pal_num], pattern, false);
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

void draw_tile(u32 *pixel, const u32 *color_table, const u8 *palette,
			   const u8 *tile_ptr, bool write_transparent) {
	unsigned j;
	// Les pixels sont sur 2 plans: le premier octet indique le plan
	// "foncé", le deuxième le plan "clair". Aucun = blanc, les 2 = noir.
	u8 pat1 = tile_ptr[0], pat2 = tile_ptr[1];
	for (j = 0; j < 8; j++) {
		u8 color = (pat2 & 0x80) >> 6 | (pat1 & 0x80) >> 7;
		// Pixel opaque (couleur non 0) -> dessine avec l'alpha à 100%
		if (color != 0)
			*pixel = color_table[palette[color]] | 0xff000000;
		// L'alpha (bits 31..24) indique que le pixel a été dessiné avec
		// un pixel opaque, ce qui n'est pas le cas d'un pixel à couleur 0.
		// => n'écrase pas les pixels déjà dessinés avec une couleur opaque.
		else if (write_transparent && *pixel >> 24 == 0)
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
		for (j = 0; j < 160; j++)		// Rend tous les pixels opaques
			*dest++ = *pixel++ | 0xff000000;
		dest += destWidth - 160;
	}
}
