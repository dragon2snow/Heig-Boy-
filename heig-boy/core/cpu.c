#include "cpu.h"
#include "mem.h"
#include "debug.h"

/** Registres du processeur (BC, DE, HL, AF dans cet ordre) */
static u8 registers[8];
static u16 SP, PC;
// Simples & paires
enum {R_B = 0, R_C, R_D, R_E, R_H, R_L, R_A, R_F};
enum {R_BC = 0, R_DE, R_HL, R_AF};

/** Lit une paire de registres */
static u16 read_pair(u16 index) {
	return registers[index * 2] | registers[index * 2 + 1] << 8;
}

/** Ecrit une paire de registres */
static void write_pair(u16 index, u16 value) {
	registers[index * 2] = value & 0xff;
	registers[index * 2 + 1] = value >> 8 & 0xff;
}

/** Décode un registre 8 bits d'un opcode et le lit.
	\param index même que pour les opérandes de type r dans Z80.DOC.
*/
static u8 read_r(u16 index) {
	switch (index) {
		case 6:
			return mem_readb(read_pair(R_HL));
		case 7:
			return registers[R_A];
		default:
			return registers[index & 7];
	}
}

/** Décode un registre 8 bits d'un opcode et l'écrit.
	\param index même que pour les opérandes de type r dans Z80.DOC.
*/
static void write_r(u16 index, u8 value) {
	switch (index) {
		case 6:
			mem_writeb(read_pair(R_HL), value);
		default:
			registers[index] = value;
	}
}

/** Décode une paire de registres (dd) et la lit.
	\param index même que pour les opérandes de type dd dans Z80.DOC.
*/
static u16 read_dd(u16 index) {
	switch (index) {
		case 3:
			return SP;
		default:
			return read_pair(index);
	}
}

/** Décode une paire de registres (dd) et l'écrit.
	\param index même que pour les opérandes de type dd dans Z80.DOC.
*/
static void write_dd(u16 index, u16 value) {
	switch (index) {
		case 3:
			SP = value;
		default:
			write_pair(index, value);
	}
}

/** Lecture du prochain byte pointé par le PC */
static u8 pc_readb() { return mem_readb(PC++); }
/** Lecture du prochain mot (16 bits) pointé par le PC */
static u16 pc_readw() {
	u16 r = mem_readw(PC);
	PC += 2;
	return r;
}

void cpu_init() {
	write_pair(R_AF, 0x01B0);
	write_pair(R_BC, 0x0013);
	write_pair(R_DE, 0x00D8);
	write_pair(R_HL, 0x0013);
	SP = 0xFFFE;
	PC = 0x0100;
}

int cpu_exec_instruction() {
	u16 opcode = pc_readb();
	u8 mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;

	switch (opcode) {
		case 0x00:		// nop
			return 1;
		case 0xc3:		// jp nn
			PC = pc_readw();
			return 4;
	}

	// 01 rrr sss -> ld r, s
	if ((opcode & 0300) == 0100) {
		write_r(mid_digit, read_r(low_digit));
		return low_digit == 6 ? 2 : 1;		// (hl)
	}

	dbg_error("unkown instruction @ %04x", PC - 1);
	return 1;
}
