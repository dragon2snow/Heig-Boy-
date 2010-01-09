/** \file lang.c

	This file implements the core language and base ColorIt functions. You
	don't need to modify this file.
*/
#include <string.h>
#include "parser.h"
#include "lang.h"

// Retourne 0 < tileNo < 383, check isError() before using returned value!
static unsigned readTileNo() {			// Notation +123 ou 123
	Token *tok;
	unsigned tileNo;
	tileNo = 0;
	if (tok = readIf(PLUS))
		tileNo = 128;
	if (tok = read(NUMBER))
		tileNo += tok->data;
	if (tileNo >= 384)
		errorTok(tok, "invalid tile number (0 < t < 384)");
	return tileNo;
}

// Retourne 0 < ctNo < 512, please check isError()!
static unsigned readCustomTileNo() {
	Token *tok;
	if (tok = read(NUMBER)) {
		if (tok->data < 512)
			return tok->data;
		else
			errorTok(tok, "invalid custom tile number (0 < ct < 512)");
	}
	return 0;
}

// Retourne 0 < palNo < 256, please check isError()!
static unsigned readPaletteNo() {
	Token *tok;
	if (tok = read(NUMBER)) {
		if (tok->data < 256)
			return tok->data;
		else
			errorTok(tok, "invalid palette number (0 < p < 256)");
	}
	return 0;
}

// Retourne une couleur, please check isError()!
static unsigned readColor() {
	Token *tok;
	unsigned color = 0xff000000;		// Couleur opaque par défaut
	if (tok = readIf(IDENTIFIER)) {		// Syntaxe RGB(r, g, b)
		if (strcmp(tok->text, "rgb")) {
			errorTok(tok, "only RGB can be used here");
			return color;
		}
		read(LEFT_PAR);
		if (tok = read(NUMBER))
			color |= tok->data & 0xff;
		read(COMMA);
		if (tok = read(NUMBER))
			color |= (tok->data & 0xff) << 8;
		read(COMMA);
		if (tok = read(NUMBER))
			color |= (tok->data & 0xff) << 16;
		read(RIGHT_PAR);
	}
	else if (tok = read(NUMBER))		// Syntaxe 0x123456
		color ^= tok->data;
	return color;
}

// Retourne une valeur vrai ou faux, please check isError()!
static int readBoolean() {
	Token *tok;
	if (tok = readIf(NUMBER))
		return tok->data;
	if (tok = read(IDENTIFIER)) {
		if (!strcmp(tok->text, "true"))
			return 1;
		else if (!strcmp(tok->text, "false"))
			return 0;
		else
			errorTok(tok, "only true or false can be used here");
	}
	return 0;
}

// Fait en sorte que x < y
static void ensureXlessThanY(unsigned *x, unsigned *y) {
	if (*x > *y) {
		int temp = *x;
		*x = *y;
		*y = temp;
	}
}

// ColorIt.addTileCrc tileStart[, tileEnd]
static void func_addTileCrc() {
	int tileStart = readTileNo(), tileEnd;
	if (readIf(COMMA))
		tileEnd = readTileNo();
	else
		tileEnd = tileStart;
	if (!isError()) {
		ensureXlessThanY(&tileStart, &tileEnd);
		ColorIt_addTileCrc(tileStart, tileEnd);
	}
}

// ColorIt.setPalette palNo, color1, color2, color3, color4
static void func_setPalette() {
	unsigned palNo = readPaletteNo(), i;
	unsigned colors[4];
	// Les 4 couleurs de palette
	for (i = 0; i < 4; i++) {
		read(COMMA);
		colors[i] = readColor();
	}
	if (!isError())
		ColorIt_setPalette(palNo, colors);
}

// ColorIt.addTileRule tileStart[, tileEnd], palNo
static void func_addTileRule() {
	unsigned tileStart = readTileNo(), tileEnd, palNo;
	read(COMMA);
	tileEnd = readTileNo();
	if (readIf(COMMA))		// 3 arguments
		palNo = readPaletteNo();
	else {					// 2 arguments
		palNo = tileEnd;
		tileEnd = tileStart;
	}
	if (!isError()) {
		ensureXlessThanY(&tileStart, &tileEnd);
		ColorIt_addTileRule(tileStart, tileEnd, palNo);
	}
}

// ColorIt.setTilesetData ctNo, [L]"32 digits of hex-data"
static void func_setTilesetData() {
	int i, j;
	unsigned ctNo = readCustomTileNo();
	unsigned char tileData[16] = {0}, mode = 'n';
	Token *tok;
	if (!read(COMMA))
		return;
	// Préfixe pour le mode étendu
	if (tok = readIf(IDENTIFIER)) {
		if (tok->text[0] == 'l' && tok->text[1] == '\0')
			mode = 'l';
		else {
			errorTok(tok, "only L can be used here");
			return;
		}
	}
	if (tok = read(STRING)) {
		const char *text = tok->text;
		// Mode normal: données RAW au format GB
		if (mode == 'n') {
			// 16 octets par tile
			for (i = 0; i < 16; i++) {
				// Récupération d'un nombre (doublet) hexadécimal
				unsigned char vals[2];
				for (j = 0; j < 2; j++) {
					char c = *text++;
					if (c == '\0') {
						errorTok(tok, "incomplete hex tile data");
						return;
					}
					while (c == ' ' || c == '\t')
						c = *text++;
					if (c >= '0' && c <= '9')
						vals[j] = c - '0';
					else if (c >= 'a' && c <= 'f')
						vals[j] = c - 'a' + 10;
					else {
						errorTok(tok, "invalid in tile data");
						return;
					}
				}
				// Valeur finale
				tileData[i] = vals[1] + (vals[0] << 4);
			}
		}
		else if (mode == 'l') {		// Deprecated (and buggy, value == 0 should be 1)
			unsigned char *tilePtr = tileData;
			// 8*8 pixels par tile
			for (j = 0; j < 8; j++) {
				for (i = 7; i >= 0; i--) {
					//Récupération d'un nombre
					unsigned char c = *text++;
					if (c == '\0') {
						errorTok(tok, "incomplete hex tile data");
						return;
					}
					while (c == ' ' || c == '\t')
						c = *text++;
					if (c < '0' || c > '3') {
						errorTok(tok, "invalid in tile data");
						return;
					}
					//Affectation des bits selon la valeur (1 à 4)
					if (c == '0' || c == '3')
						tilePtr[0] |= 1 << i;
					if (c == '2' || c == '3')
						tilePtr[1] |= 1 << i;
				}
				// 2 octets par ligne
				tilePtr += 2;
			}
		}
		ColorIt_setTilesetData(ctNo, tileData);
	}
}

// ColorIt.setTile tileStart[, tileEnd], ctNo
static void func_setTile() {
	unsigned tileStart = readTileNo(), tileEnd, ctNo;
	read(COMMA);
	tileEnd = readTileNo();
	if (readIf(COMMA))		// 3 arguments
		ctNo = readCustomTileNo();
	else {					// 2 arguments
		ctNo = tileEnd;
		tileEnd = tileStart;
	}
	if (!isError()) {
		ensureXlessThanY(&tileStart, &tileEnd);
		ColorIt_setTile(tileStart, tileEnd, ctNo);
	}
}

// ColorIt.autoShowVramCrc = true | false
static void func_autoShowVramCrc() {
	int enabled;
	read(EQUAL);
	enabled = readBoolean();
	if (!isError())
		ColorIt_autoShowVramCrc(enabled);
}

// MsgBox "text"
static void func_msgBox() {
	Token *tok;
	if (tok = read(STRING))
		ColorIt_messageBox(tok->text);
}

void user_call(Token *tok) {
	// Fonctions externes
	int i;
	struct {
		const char *name;
		void (*func)();
	} externFuncs[] = {
		{"addtilecrc", func_addTileCrc},
		{"setpalette", func_setPalette},
		{"addtilerule", func_addTileRule},
		{"settilesetdata", func_setTilesetData},
		{"settile", func_setTile},
		{"autoshowvramcrc", func_autoShowVramCrc},
		{"msgbox", func_msgBox},
	};
	// Préfixe 'ColorIt.'
	if (!strcmp(tok->text, "colorit")) {
		read(DOT);
		if (!(tok = read(IDENTIFIER)))
			return;
	}
	for (i = 0; i < numberof(externFuncs); i++)
		if (!strcmp(tok->text, externFuncs[i].name)) {
			externFuncs[i].func();
			break;
		}
	// Pas trouvé
	if (i == numberof(externFuncs))
		errorTok(tok, "unkown instruction");
}
