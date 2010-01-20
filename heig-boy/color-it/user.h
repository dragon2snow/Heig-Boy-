/** \file user.h

	User-specific file (in this case VisualBoyAdvance) grouping shared
	variables and functions within VisualBoyAdvance and ColorIt scripts.
*/
#ifndef USER_H
#define USER_H

#ifdef __cplusplus
extern "C" {
#endif

extern int ColorIt_init(const char *fullRomPath, const unsigned char *romData);
extern void ColorIt_execCrc(unsigned crc32);

#define NB_PALETTES 256
#define numberof(o)		(sizeof(o) / sizeof(*(o)))

// ColorIt enabled?
extern int ColorIt_enabled;
// Palettes of tiles
extern unsigned char ColorIt_tilePalette[384];
// Custom tile indexes
extern unsigned short ColorIt_tileCustom[384];
// Tiles included to CRC (bit-per-bit)
extern unsigned char ColorIt_tileIncluded[384 / 8];
// Actual palettes
extern unsigned long ColorIt_palette[NB_PALETTES * 4];
// Custom tile data
extern unsigned char ColorIt_tileData[384 * 16];
// [optional] When fileCheckThread detects a change
extern int ColorIt_reload;

#include "system.h"

#ifdef __cplusplus
}
#endif

#endif
