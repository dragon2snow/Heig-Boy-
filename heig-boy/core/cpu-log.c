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

static FILE *log_file;
static unsigned long cycle_counter;
static record_t next_record;

void cpu_init() {
	log_file = fopen("C:\\log.bin", "rb");
	cycle_counter = 0;
	fread(&next_record, sizeof(record_t), 1, log_file);
}

void cpu_trigger_irq(cpu_interrupt_t irq) {}

unsigned cpu_exec_instruction() {
	// Fait "avancer le temps"
	cycle_counter += 4;
	// L'événement arrive
	while (cycle_counter >= next_record.clock) {
		mem_writeb(next_record.address, next_record.value);
//		if (next_record.address >= 0xff20 && next_record.address <= 0xff23)
//			printf("Wrote @ %x: %x\n", next_record.address, next_record.value);
		if (fread(&next_record, sizeof(record_t), 1, log_file) != 1)
			printf("EOF");
	}
	return 1;
}
