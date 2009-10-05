/*	\file cpu-log.c
	\brief pilote alternatif remplaçant le CPU en faisant une suite
		d'accès mémoire programmés.
*/
#include <stdio.h>
#include "cpu.h"
#include "mem.h"

typedef struct {
	unsigned short address;
	unsigned char value, dummy;
	unsigned long clock;
} record_t;

static FILE *log;
static unsigned long cycle_counter;
static record_t next_record;

void cpu_init() {
	log = fopen("C:\\log.bin", "rb");
	cycle_counter = 0;
	fread(&next_record, sizeof(record_t), 1, log);
}

int cpu_exec_instruction() {
	// Fait "avancer le temps"
	cycle_counter += 16;
	// L'événement arrive
	while (cycle_counter >= next_record.clock) {
//		printf("%x, next @%x", cycle_counter, next_record.clock);
		mem_writeb(next_record.address, next_record.value);
//		if (next_record.address == 0xff00 + R_BGP)
//			printf("Wrote @ %x: %x\n", next_record.address, next_record.value);
		if (fread(&next_record, sizeof(record_t), 1, log) != 1)
			printf("EOF");
	}
	return 16;
}
