#include "user.h"
#include "crc32.h"

static unsigned long lastChecksum;
static unsigned char lastPalette;

// Calcule le CRC de la VRAM sur les tiles sélectionnées
static unsigned long vram_checksum(const unsigned char *vram) {
	int i;
	unsigned long crc = 0;
	for (i = 0; i < 384; i++)
		if (ColorIt_tileIncluded[i >> 3] & (1 << (i & 7)))
			crc = crc32(crc, vram + i * 16, 16);
	return crc;
}

void ColorIt_systemInit() {
	lastChecksum = 0;
	lastPalette = 0;
}

void ColorIt_exitingLcdc(const unsigned char *vram) {
	if (ColorIt_enabled) {
		unsigned long crc = vram_checksum(vram);
		// Crc changed?
		if (crc != lastChecksum)
			ColorIt_execCrc(crc);
		lastChecksum = crc;
	}
}

void ColorIt_endFrame() {
}
