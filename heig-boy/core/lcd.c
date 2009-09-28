/** \file lcd.c
	\brief Impl�mentation de l'affichage du LCD (chip de dessin).
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

/*
	Outils
*/
// Modulos communs
#define mod8(x)		((x) & 7)
#define mod32(x)	((x) & 31)
#define mod256(x)	((x) & 255)
// Divisions communes
#define div8(x)		((x) >> 3)
// Structure des attributs d'un sprite
typedef struct {
	u8 y, x, tile;
	struct obj_attr3_t {
		u8 cgb_pal_num: 3;		// GB Color seulement
		u8 vram_bank: 1;		// GB Color seulement
		u8 pal_num: 1;			// N� de palette (OBP0 / OBP1)
		u8 flip_x: 1, flip_y: 1;	// Retournement (miroir)
		u8 prio: 1;				// Priorit� par rapport au BG (0=devant, 1=derri�re)
	} attr;
} obj_t;


/*
	Buffer �cran
*/
// Temporaire: table de conversion des couleurs (GB <-> 32 bits)
static const u32 color_table[4] = {0xff101010, 0xff606060, 0xffb0b0b0, 0xffffffff};
static u8 bg_palette[4] = {0, 1, 2, 3};
/** Bitmap 32 bits de 256x144 pixels */
static u32 *lcd_buffer = NULL;
// Taille d'une ligne de buffer
#define BUFFER_LINE_PITCH 256
// Donne la ligne n� n du buffer
#define lcd_buffer_line(n)	(lcd_buffer + n * BUFFER_LINE_PITCH + 8)

/** R�alise le rendu du fond d'�cran: fond uni utilisant la couleur 0 de la
	BGP (palette du BG). */
void backdrop_render();
/** R�alise le rendu de la ligne courante du BG (background).
	Il s'agit d'une tile map. */
static void bg_render();
/** R�alise le rendu de la fen�tre pour la ligne courante.
	La fen�tre vient remplacer le BG et commence � la position (WX, WY).
*/
static void win_render();
/** R�alise le rendu des objets pour la ligne courante. Le rendu doit �tre fait
	en deux passes afin d'intercaler le BG entre les objets (en fonction de
	leur priorit�). On appellera donc la fonction deux fois en ayant mis le
	param�tre � la valeur ad�quate.
	\param skipped_prio 0 pour dessiner les objets au-dessous du BG, 1 pour
		ceux au-dessus
*/
static void obj_render(u8 skipped_prio);
/** Dessine un motif de 8 pixels au format GB 2 bits entrelac� sur l'�cran.
	\param pixel buffer destination de l'�cran (32 bits)
	\param palette table of 4 colors to use for mapping the gray shades
	\param tile_ptr pointeur sur le motif
*/
static void draw_tile(u32 *pixel, const u8 *palette, const u8 *tile_ptr);
/** Retourne un motif de 8x8 au format GB 2 bits horizontalement.
	\param in pointeur sur le motif (2 octets)
	\param out destination (2 octets)
*/
static void flip_tile(u8 *out, const u8 *in);
/** Routine super temporaire qui balance le rendu actuel sur l'�cran via un
	m�ga hack win32-only. (SetPixel sur GetForegroundWindow...) */
static void temp_render_to_screen();

void lcd_draw_line() {
	if (!lcd_ctrl.enable)		// LCD d�sactiv�
		return;
	backdrop_render();
	obj_render(0);
	bg_render();
	obj_render(1);
//	temp_render_to_screen();
}

void lcd_begin() {
	memset(lcd_buffer, 0, 256 * 144 * sizeof(u32));
}

void lcd_draw_init() {
	if (!lcd_buffer)
		lcd_buffer = malloc(256 * 144 * sizeof(u32));
}

void backdrop_render() {
	int i;
	u32 *pixel = lcd_buffer_line(cur_line);
	for (i = 0; i < 160; i++)
		*pixel++ = color_table[bg_palette[0]];
}

void bg_render() {
	// Position (transform�e dans le rep�re �cran) du premier pixel � traiter
	unsigned offset_y = mod256(scroll_y + cur_line);
	unsigned tile_offset_x = mod32(div8(scroll_x));
	// Calcule l'adresse des objets en m�moire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.bg_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Buffer �cran
	u32 *pixel = lcd_buffer_line(cur_line) - mod8(8 - scroll_x);
	unsigned i;
	// BG d�sactiv�
	if (!lcd_ctrl.bg_en)
		return;
	// Dessin de la map � proprement parler
	for (i = 0; i < 21; i++) {
		// Si le mode est 1, le n� de tile est sign�, sinon non sign�
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (s8)tile_val : (u8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// Dessin du motif
		draw_tile(pixel, bg_palette, tile_ptr);
		pixel += 8;
	}
}

void win_render() {
	// Position (transform�e dans le rep�re �cran) du premier pixel � traiter
	unsigned offset_x = REG(WX) - 7, offset_y = cur_line - REG(WY);
	unsigned tile_offset_x = 0;
	// Calcule l'adresse des objets en m�moire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.win_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Commence au d�but de la fen�tre
	u32 *pixel = lcd_buffer_line(cur_line) + offset_x;
	// Fen�tre d�sactiv�e ou ligne courante en dehors
	if (!lcd_ctrl.win_en || offset_y < 0)
		return;
	// Dessine jusqu'� la fin de la fen�tre (toujours tout � droite de l'�cran)
	while (offset_x < 160) {
		// Si le mode est 1, le n� de tile est sign�, sinon non sign�
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (s8)tile_val : (u8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// On avance de 8 pixels
		draw_tile(pixel + offset_x, bg_palette, tile_ptr);
		offset_x += 8;
	}
}

void obj_render(u8 skipped_prio) {
	obj_t *oam = (obj_t*)mem_oam;	// Liste d'attributs des sprites
	u8 obj_size = lcd_ctrl.obj_size * 8;
	u32 *pixel = lcd_buffer_line(cur_line);
	int i;
	if (!lcd_ctrl.obj_en)			// Comme d'hab, fonction d�sactiv�e
		return;
	/*	Le syst�me de priorit�s de la GB est tr�s compliqu� (il implique de
		trier les sprites par position X. A la place on va �muler le syst�me
		comme sur la Game Boy Color (selon le n� de sprite, le premier ayant
		la plus grosse priorit�). Ca ne devrait pas poser trop de souci. */
	for (i = 39; i >= 0; i--) {
		// 4 octets par objet, d�crivant les attributs
		u8 y = oam->y - 16 - cur_line;	// ligne courante � dessiner
		// Pas besoin de -8, le buffer a d�j� 8 pixels invisibles � gauche
		u8 x = oam->x;
		u8 *tile_ptr = mem_vram + 16 * oam->tile;
		struct obj_attr3_t attr = oam->attr;
		u8 pattern[2];
		// Avance le pointeur pour le prochain sprite de la liste
		oam++;
		if (attr.prio == skipped_prio)	// pas la priorit� voulue
			continue;
		if (x == 0 || x >= 168)			// invisible
			continue;
		if (y >= obj_size)		// en fait le test devrait avoir || y < 0 mais
			continue;			// comme y est non sign� donc on revient � 255
		if (attr.flip_y)		// retournement vertical
			y = 7 - y;			// compte les lignes depuis le bas
		if (attr.flip_x)		// Retourne �ventuellement le motif
			flip_tile(pattern, tile_ptr);
		else
			pattern[0] = tile_ptr[0], pattern[1] = tile_ptr[1];
		// FIXME: tenir compte des OBP
		draw_tile(pixel + x, bg_palette, pattern);
	}
}

void flip_tile(u8 *out, const u8 *in) {
	// Vitesse critique, d'o� d�roulage...
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

void draw_tile(u32 *pixel, const u8 *palette, const u8 *tile_ptr) {
	unsigned j;
	// Les pixels sont sur 2 plans: le premier octet indique le plan
	// "fonc�", le deuxi�me le plan "clair". Aucun = blanc, les 2 = noir.
	u8 pat1 = tile_ptr[0], pat2 = tile_ptr[1];
	for (j = 0; j < 8; j++) {
		u8 color = (pat1 & 0x80) >> 6 | (pat2 & 0x80) >> 7;
		if (color != 0)
			*pixel = color_table[palette[color]];
		pixel++, pat1 <<= 1, pat2 <<= 1;
	}
}

// KICKME: sooner the better
#ifdef WIN32
	#include <windows.h>
	void temp_render_to_screen() {
		int i;
		HDC hdc;
		HWND hwnd = GetForegroundWindow();
		u32 *pixel = lcd_buffer_line(cur_line);
		hdc = GetDC(hwnd);
		for (i = 0; i < 160; i++)
			SetPixel(hdc, i, cur_line, *pixel++ & 0xffffff);
		ReleaseDC(hwnd, hdc);
	}
#else
	void temp_render_to_screen() {}
#endif
