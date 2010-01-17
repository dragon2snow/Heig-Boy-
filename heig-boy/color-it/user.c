/** \file user.c

	User-specific file (in this case VisualBoyAdvance).
	Basically process callbacks from the parser.
*/
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "parser.h"
#include "lang.h"
#include "user.h"

// Global variables
int ColorIt_enabled;
unsigned char ColorIt_tilePalette[384], ColorIt_tileIncluded[384 / 8];
unsigned short ColorIt_tileCustom[384];
unsigned char ColorIt_tileData[384 * 16];
unsigned long ColorIt_palette[NB_PALETTES * 4];

// Extracts the path (without fill name) from the full path fileName to dest
static void extractPath(char *dest, const char *fileName) {
	int i;
	strcpy(dest, fileName);
	for (i = strlen(dest) - 1; i >= 0; i--) {
		if (dest[i] == '\\' || dest[i] == '/') {
			dest[i + 1] = '\0';
			return;
		}
	}
}

static void init() {
	// Réinitialisation des paramètres
	const unsigned long defaultCol[4] =
		{0xd3de34, 0xa5b52b, 0x70831b, 0x284440};
	//	{0xffff00ff, 0xffc000c0, 0xff800080, 0xff400040};
	//	{0xff00e7ff, 0xff009ac7, 0xff004d8f, 0xff000057};
	//	{0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000};
	memset(ColorIt_tilePalette, 0, sizeof(ColorIt_tilePalette));
	memset(ColorIt_tileCustom, -1, sizeof(ColorIt_tileCustom));
	memcpy(ColorIt_palette, defaultCol, 4 * 4);
}

int ColorIt_init(const char *fullRomPath, const unsigned char *romData) {
	char name[128], fullName[1024];
	unsigned i;

	ColorIt_systemInit();
	init();

	// rom_directory/cart_name.pal.ini
	memcpy(name, romData + 0x134, 15);
	// Only allowed chars in the cart name
	for (i = 0; fullName[i]; i++) {
        if (name[i] >= 'A' && name[i] <= 'Z' ||
            name[i] >= 'a' && name[i] <= 'z' ||
            name[i] == ' ' || name[i] == '-' || name[i] == '.' ||
            name[i] >= '0' && name[i] <= '9' ||
			name[i] == 0) {}
		else
			name[i] = '_';
	}
	name[15] = 0;
	strcat(name, ".pal.ini");
	extractPath(fullName, fullRomPath);
	strcat(fullName, name);

	// By default no tile is included to the Crc
	memset(ColorIt_tileIncluded, 0, sizeof(ColorIt_tileIncluded));
	return ColorIt_enabled = ColorIt_execScript(fullName, "init");
}

void ColorIt_execCrc(unsigned crc32) {
	char buf[12];
	sprintf(buf, "[%08x]", crc32);
	init();
	if (!ColorIt_execScript(NULL, buf))			// Si la section n'existe pas
		ColorIt_execScript(NULL, "[default]");	// -> [default]
}

void ColorIt_ShowError(int line, int col, const char *msg) {
	char buf[1024];
	sprintf(buf, "At line %i, col %i: %s\n", line, col, msg);
	MessageBox(NULL, buf, "Script error", 0);
}

void ColorIt_messageBox(const char *msg) {
	MessageBox(NULL, msg, "Script message", 0);
}

void ColorIt_autoShowVramCrc(int value) {
	// Do nothing of it
}

void ColorIt_addTileCrc(unsigned tileStart, unsigned tileEnd) {
	unsigned i;
	for (i = tileStart; i <= tileEnd; i++)
		ColorIt_tileIncluded[i >> 3] |= 1 << (i & 7);
}

void ColorIt_setPalette(unsigned palNo, unsigned long *colors) {
	unsigned long *pal = ColorIt_palette + palNo * 4;
	unsigned i;
	for (i = 0; i < 4; i++)
		// Invert RGB order
		pal[i] =
			(colors[i] & 0xff00) |
			(colors[i] >> 16 & 0xff) |
			(colors[i] & 0xff) << 16;
}

void ColorIt_addTileRule(unsigned tileStart, unsigned tileEnd, unsigned palNo) {
	unsigned i;
	for (i = tileStart; i <= tileEnd; i++)
		ColorIt_tilePalette[i] = palNo;
}

void ColorIt_setTilesetData(unsigned ctNo, unsigned char *data) {
	memcpy(ColorIt_tileData + ctNo * 16, data, 16);
}

void ColorIt_setTile(unsigned tileStart, unsigned tileEnd, unsigned ctNo) {
	unsigned i;
	for (i = tileStart; i <= tileEnd; i++)
		ColorIt_tileCustom[i] = ctNo++;
}
