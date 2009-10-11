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
//static const u32 color_table[4] = {0xff00e7ff, 0xff009ac7, 0xff004d8f, 0xff000057};
static const u32 color_table[4] = {0xffd3de34, 0xffa5b52b, 0xff70831b, 0xff284440};
//static const u32 color_table[4] = {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000};
static u8 bg_palette[4], obj_palette[2][4];
/** Bitmap 32 bits de 256x144 pixels */
static u32 *lcd_buffer = NULL;
// Taille d'une ligne de buffer
#define BUFFER_LINE_PITCH 256
// Donne la ligne n° n du buffer
#define lcd_buffer_line(n)	(lcd_buffer + n * BUFFER_LINE_PITCH + 8)

/** Réalise le rendu du fond d'écran: fond uni utilisant la couleur 0 de la
	BGP (palette du BG). */
void backdrop_render();
/** Réalise le rendu de la ligne courante du BG (background).
	Il s'agit d'une tile map. */
static void bg_render();
/** Réalise le rendu de la fenêtre pour la ligne courante.
	La fenêtre vient remplacer le BG et commence à la position (WX, WY).
*/
static void win_render();
/** Réalise le rendu des objets pour la ligne courante. Le rendu doit être fait
	en deux passes afin d'intercaler le BG entre les objets (en fonction de
	leur priorité). On appellera donc la fonction deux fois en ayant mis le
	paramètre à la valeur adéquate.
	\param skipped_prio 0 pour dessiner les objets au-dessous du BG, 1 pour
		ceux au-dessus
*/
static void obj_render(u8 skipped_prio);
/** Dessine un motif de 8 pixels au format GB 2 bits entrelacé sur l'écran.
	\param pixel buffer destination de l'écran (32 bits)
	\param palette table of 4 colors to use for mapping the gray shades
	\param tile_ptr pointeur sur le motif
*/
static void draw_tile(u32 *pixel, const u8 *palette, const u8 *tile_ptr);
/** Retourne un motif de 8x8 au format GB 2 bits horizontalement.
	\param in pointeur sur le motif (2 octets)
	\param out destination (2 octets)
*/
static void flip_tile(u8 *out, const u8 *in);
/** Décompose un registre palette en une table de valeurs à utiliser pour
	mapper les niveaux de gris.
	\param out table de destination de 4 éléments
	\param reg un registre palette (BGP, OBP...)
	Par exe
*/
static void translate_palette(u8 *out, u8 reg);
/** Routine super temporaire qui balance le rendu actuel sur l'écran via un
	méga hack win32-only. (SetPixel sur GetForegroundWindow...) */
static void temp_render_to_screen();

void lcd_draw_line() {
	// Prépare les palettes
	translate_palette(bg_palette, REG(BGP));
	translate_palette(obj_palette[0], REG(OBP0));
	translate_palette(obj_palette[1], REG(OBP1));
	// Fait le rendu à proprement parler
	backdrop_render();
	if (lcd_ctrl.enable) {		// LCD activé
		obj_render(0);
		bg_render();
		win_render();
		obj_render(1);
	}
	temp_render_to_screen();
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
	// Position (transformée dans le repère écran) du premier pixel à traiter
	unsigned offset_y = mod256(scroll_y + cur_line);
	unsigned tile_offset_x = mod32(div8(scroll_x));
	// Calcule l'adresse des objets en mémoire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.bg_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Buffer écran
	u32 *pixel = lcd_buffer_line(cur_line) - mod8(scroll_x);
	unsigned i;
	// BG désactivé
	if (!lcd_ctrl.bg_en)
		return;
	// Dessin de la map à proprement parler
	for (i = 0; i < 21; i++) {
		// Si le mode est 1, le n° de tile est signé, sinon non signé
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (u8)tile_val : (s8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// Dessin du motif
		draw_tile(pixel, bg_palette, tile_ptr);
		pixel += 8;
	}
}

void win_render() {
	// Position (transformée dans le repère écran) du premier pixel à traiter
	int offset_x = REG(WX) - 7, offset_y = cur_line - REG(WY);
	unsigned tile_offset_x = 0;
	// Calcule l'adresse des objets en mémoire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.win_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Commence au début de la fenêtre
	u32 *pixel = lcd_buffer_line(cur_line) + offset_x;
	// Fenêtre désactivée ou ligne courante en dehors
	if (!lcd_ctrl.win_en || offset_y < 0)
		return;
	// Dessine jusqu'à la fin de la fenêtre (toujours tout à droite de l'écran)
	while (offset_x < 160) {
		// Si le mode est 1, le n° de tile est signé, sinon non signé
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (s8)tile_val : (u8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// Dessin + avancement de 8 pixels
		draw_tile(pixel + offset_x, bg_palette, tile_ptr);
		offset_x += 8;
	}
}

void obj_render(u8 skipped_prio) {
	// Liste d'attributs des sprites
	obj_t *oam = (obj_t*)mem_oam;
	// En mode 8x16 (obj_size=1), le dernier bit de la tile est ignoré
	u8 obj_height = 8 + lcd_ctrl.obj_size * 8;
	u8 tile_mask = ~lcd_ctrl.obj_size;
	u32 *pixel = lcd_buffer_line(cur_line) - 8;
	int i;
	if (!lcd_ctrl.obj_en)			// Comme d'hab, fonction désactivée
		return;
	/*	Le système de priorités de la GB est très compliqué (il implique de
		trier les sprites par position X. A la place on va émuler le système
		comme sur la Game Boy Color (selon le n° de sprite, le premier ayant
		la plus grosse priorité). Ca ne devrait pas poser trop de souci. */
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
		if (x == 0 || x >= 168)			// invisible
			continue;
		if (y >= obj_height)	// en fait le test devrait avoir || y < 0 mais
			continue;			// comme y est non signé donc on revient à 255
		if (attr.flip_y)		// retournement vertical
			y = obj_height - 1 - y;	
		tile_ptr += y * 2;		// plus bas dans le motif
		if (attr.flip_x)		// Retourne éventuellement le motif
			flip_tile(pattern, tile_ptr);
		else
			pattern[0] = tile_ptr[0], pattern[1] = tile_ptr[1];
		// Finalement, dessine le motif
		draw_tile(pixel + x, obj_palette[attr.pal_num], pattern);
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

void draw_tile(u32 *pixel, const u8 *palette, const u8 *tile_ptr) {
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

// KICKME: sooner the better
#ifdef WIN32
	#include <windows.h>
	HDC hdc = NULL;
	HBITMAP hbm = NULL;
	HWND hwnd = NULL;
	unsigned long *pix_data;
	__int64 freq, last_val;

	void create(int width, int height) {
		const int bitDepth = 32;
		BITMAPINFOHEADER bih;
		HDC hdcEcran = GetDC(NULL);
		// Aligned to 4 pixels for simplicity
		int alignedWidth = (width % 4 == 0 ? width : width + (4 - width % 4));

		memset(&bih, 0, sizeof(bih));
		bih.biSize = sizeof(bih);
		bih.biWidth = alignedWidth;
		bih.biHeight = height;
		bih.biPlanes = 1;
		bih.biBitCount = bitDepth;
		bih.biCompression = BI_RGB;

		hdc = CreateCompatibleDC(hdcEcran);
		hbm = CreateDIBSection(hdc, (BITMAPINFO*)&bih, DIB_PAL_COLORS, &pix_data, NULL, 0);
		// Select the new bitmap into the buffer DC.
		if (hbm && pix_data)
			SelectObject(hdc, hbm);
		ReleaseDC(NULL, hdcEcran);
	}
	void temp_render_to_screen() {
		static int frameNo = 0;
		// Première fois
		if (!hdc) {
			create(256, 144);
			hwnd = GetForegroundWindow();
			QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
			QueryPerformanceCounter((LARGE_INTEGER*)&last_val);
		}
		// Rendu
		if (cur_line == 0) {
			HDC hdcDest = GetDC(hwnd);
			unsigned i, j;
			__int64 val;

			for (i = 0; i < 144; i++) {
				u32 *pixel = lcd_buffer_line(i);
				for (j = 0; j < 160; j++)
					pix_data[(143 - i) * 256 + j] = *pixel++;
			}
			StretchBlt(hdcDest, 160, 0, 160*2, 144*2, hdc, 0, 0, 160, 144, SRCCOPY);
			ReleaseDC(hwnd, hdcDest);

			// Attend que 16 millisecondes se soient écoulées
			do	QueryPerformanceCounter((LARGE_INTEGER*)&val);
			while ((double)(val - last_val) / freq < 0.016666);
			last_val += (__int64)(0.016666 * freq);
		}

/*		int i;
		HDC hdc;
		HWND hwnd = GetForegroundWindow();
		u32 *pixel = lcd_buffer_line(cur_line);
		hdc = GetDC(hwnd);
		for (i = 0; i < 160; i++)
			SetPixel(hdc, i, cur_line, *pixel++ & 0xffffff);
		ReleaseDC(hwnd, hdc);*/
	}
#else
	void temp_render_to_screen() {}
#endif
