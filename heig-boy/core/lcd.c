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
struct LCDC_t {
	u8 bg_en: 1, obj_en: 1;
	u8 obj_size: 1;
	u8 bg_map_addr: 1, tile_addr: 1;
	u8 win_en: 1, win_map_addr: 1, enable: 1;
};
#define lcd_ctrl	(*((struct LCDC_t*)&REG(LCDC)))
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

/*
	Buffer écran
*/
// Temporaire: table de conversion des couleurs (GB <-> 32 bits)
static const u32 color_table[4] = {0xff101010, 0xff606060, 0xffb0b0b0, 0xffffffff};
static u8 bg_palette[4] = {0, 1, 2, 3};
/** Bitmap 32 bits de 256x144 pixels */
static u32 *lcd_buffer = NULL;
// Donne la ligne n° n du buffer
#define lcd_buffer_line(n)	(lcd_buffer + n * BUFFER_LINE_PITCH + 8)
// Taille d'une ligne de buffer
#define BUFFER_LINE_PITCH 256

void lcd_begin() {
	memset(lcd_buffer, 0xff, 256 * 144 * sizeof(u32));
}

void lcd_draw_init() {
	if (!lcd_buffer)
		lcd_buffer = malloc(256 * 144 * sizeof(u32));
}

static void bg_render() {
	// Position (transformée dans le repère écran) du premier pixel à traiter
	unsigned offset_y = mod256(scroll_y + cur_line);
	unsigned tile_offset_x = mod32(div8(scroll_x));
	// Calcule l'adresse des objets en mémoire
	u8 *tile_data = mem_vram + (lcd_ctrl.tile_addr ? 0 : 0x1000) +
		mod8(offset_y) * 2;
	u8 *bg_map = mem_vram + (lcd_ctrl.bg_map_addr ? 0x1C00 : 0x1800) +
		div8(offset_y) * 32;
	// Buffer écran
	u32 *pixel = lcd_buffer_line(cur_line) - mod8(8 - scroll_x);
	unsigned i, j;
	
	if (!lcd_ctrl.bg_en)		// BG désactivé
		return;
	// Dessin de la map à proprement parler
	for (i = 0; i < 21; i++) {
		// Si le mode est 1, le n° de tile est signé, sinon non signé
		u8 tile_val = bg_map[mod32(tile_offset_x++)];
		int tile_no = lcd_ctrl.tile_addr ? (s8)tile_val : (u8)tile_val;
		u8 *tile_ptr = tile_data + tile_no * 16;
		// Les pixels sont sur 2 plans: le premier octet indique le plan
		// "foncé", le deuxième le plan "clair". Aucun = blanc, les 2 = noir.
		u8 pat1 = tile_ptr[0], pat2 = tile_ptr[1];
		// Déroulage de boucle pour accélérer (partie critique)
		for (j = 0; j < 2; j++) {
			*pixel++ = color_table[bg_palette[(pat1 & 0x80) >> 6 | (pat2 & 0x80) >> 7]];
			*pixel++ = color_table[bg_palette[(pat1 & 0x40) >> 5 | (pat2 & 0x40) >> 6]];
			*pixel++ = color_table[bg_palette[(pat1 & 0x20) >> 4 | (pat2 & 0x20) >> 5]];
			*pixel++ = color_table[bg_palette[(pat1 & 0x10) >> 3 | (pat2 & 0x10) >> 4]];
			pat1 <<= 4, pat2 <<= 4;
		}
	}
}

#ifdef WIN32
	#include <windows.h>
	static void temp_render_to_screen() {
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
	static void temp_render_to_screen() {}
#endif

void lcd_draw_line() {
	if (!lcd_ctrl.enable)		// LCD désactivé
		return;
	bg_render();
	temp_render_to_screen();
}
