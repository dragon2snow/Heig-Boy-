#include "cpu.h"
#include "timer.h"
#include "ports.h"

#define CPU_FREQ	(4 << 20)

static s32 div_cycles, tac_cycles;
/** Liste des fr�quences s�lectionnables via le registre TAC. */
static s32 freq_table[4] = {
	CPU_FREQ / 4096,
	CPU_FREQ / 262144,
	CPU_FREQ / 65536,
	CPU_FREQ / 16384
};

void timer_init() {
	div_cycles = 16384;
	tac_cycles = 0;
}

void timer_tick(int elapsed) {
	// Incr�mente le div tous les 16k cycles
	div_cycles -= elapsed;
	if (div_cycles <= 0) {
		REG(DIV)++;
		div_cycles += 16384;
	}
	if (REG(TAC) & 4) {		// timer activ�?
		tac_cycles -= elapsed;
		if (tac_cycles <= 0) {
			// Calcule quand il "sonnera" la prochaine fois
			tac_cycles += freq_table[REG(TAC) & 3];
			REG(TIMA)++;
			// Overflow?
			if (REG(TIMA) == 0) {
				REG(TIMA) = REG(TMA);
				cpu_trigger_irq(INT_TIMER);
			}
		}
	}
}

u8 timer_read(u16 port) {
	return mem_io[port];
}

void timer_write(u16 port, u8 value) {
	switch (port) {
		case R_DIV:
			REG(DIV) = 0;
			break;
		case R_TAC:		// met � jour la fr�quence
			tac_cycles = freq_table[value & 3];
			// Continue sur l'�criture
		default:
			mem_io[port] = value;
			break;
	}
}
