#include <gb/gb.h>
//#include <asm/types.h>
#include "main.h"
#include "gfx/all.h"

u8 joyState, joyPressed;
u8 xCamera;
u8 posCailloux[16];

// Implémentées en assembleur
void perso_handle_cam();
void game_init();
void game_handle();
void game_draw();

/*void map_display(u8 *map) {
	u8 *dest = BG_MAP;
	u8 i, j;
	for (i = 0; i < 18; i++) {
		for (j = 0; j < 32; j++)
			*dest++ = *map++;
	}
}*/

void fillTile() {
	u8 *ptr = (u8*)0x8000;
	*ptr++ = 0xff;
	*ptr++ = 0xff;
}

void level_load() {
	// Pour être sûr d'avoir accès à l'affichage
	DISPLAY_OFF;
	disable_interrupts();
	// CHR code transfer
	set_bkg_data(0, sizeof(bglvl01_tiles) / 16, bglvl01_tiles);
	set_bkg_tiles(0, 0, 32, 18, bglvl01_map);
	fillTile();
	SHOW_BKG;
	SHOW_SPRITES;
}

int main(void)
{
	u8 joyLast = 0;
	level_load();
	game_init();

	enable_interrupts();
	DISPLAY_ON;

	while (1) {
		joyState = joypad();
		joyPressed = ~joyLast & joyState;
		joyLast = joyState;
		game_handle();
		wait_vbl_done();
		BGP_REG = 0;
		perso_handle_cam();
		game_draw();
		BGP_REG = 0xE4;
	}
	return 0;
}
