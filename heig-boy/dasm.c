/** \file dasm.c
	\brief Impl�mentation s�par�e du d�sassembleur d�clar� dans le module CPU.c
*/
#include "mem.h"
#include <string.h>
#include <stdio.h>

void cpu_disassemble(u16 address, char *name, int *length, int *cycles) {
	const char *reg_names[8] = {"b", "c", "d", "e", "h", "l", "(hl)", "a"};
	const char *pair_names[8] = {"bc", "de", "hl", "sp"};
	const char *stack_reg_names[8] = {"bc", "de", "hl", "af"};
	const char *op_table[8] = {"add a,", "adc a,", "sub a,", "sbc a,", "and", "xor", "or", "cp"};
	const char *conditions[4] = {"nz", "z", "nc", "c"};
	u16 opcode = mem_readb(address);
	// Les opcodes sont vus en octal sous la forme xx yyy zzz o� zzz peut par
	// exemple �tre un des registres ci-dessus
	u8 upper_digit = (opcode >> 6) & 3, mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;

	// Si on ne trouve rien, c'est un bug -> le signaler
	strcpy(name, "<unk-inst>");
	*length = 0, *cycles = 0;

	// D�codage des opcodes communs
	switch (opcode) {
		// GMB 8-bit load commands
		case 0x0a:
			strcpy(name, "ld a, (bc)");
			*cycles = 2, *length = 1;
			return;
		case 0x1a:
			strcpy(name, "ld a, (de)");
			*cycles = 2, *length = 1;
			return;
		case 0xfa:
			sprintf(name, "ld a, ($%04x)", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0x02:
			strcpy(name, "ld (bc), a");
			*cycles = 2, *length = 1;
			return;
		case 0x12:
			strcpy(name, "ld (de), a");
			*cycles = 2, *length = 1;
			return;
		case 0xea:
			sprintf(name, "ld ($%04x), a", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0xf0:
			sprintf(name, "ld a, ($ff%02x)", mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0x08:			// ld (nn), sp PAS DOCUMENT�, peut �tre faux!!!
			sprintf(name, "ld ($%04x), sp", mem_readw(address + 1));
			*cycles = 5, *length = 3;
			return;
		case 0xe0:
			// ldh = load from io (ld avec $ff00 + adresse)
			sprintf(name, "ld ($ff%02x), a", mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0xf2:
			strcpy(name, "ld a, ($ff00 + c)");
			*cycles = 2, *length = 1;
			return;
		case 0xe2:
			// S'�crit ld [c], a avec RGBDS
			strcpy(name, "ld ($ff00 + c), a");
			*cycles = 2, *length = 1;
			return;
		case 0x22:
			// Formellement ldi (hl), a
			strcpy(name, "ld (hl+), a");
			*cycles = 2, *length = 1;
			return;
		case 0x2a:
			strcpy(name, "ld a, (hl+)");
			*cycles = 2, *length = 1;
			return;
		case 0x32:
			// Formellement ldd (hl), a
			strcpy(name, "ld (hl-), a");
			*cycles = 2, *length = 1;
			return;
		case 0x3a:
			strcpy(name, "ld a, (hl-)");
			*cycles = 2, *length = 1;
			return;
		// GMB 16-bit load commands
		case 0xf9:
			strcpy(name, "ld sp, hl");
			*cycles = 2, *length = 1;
			return;
		// GMB 8-bit arithmetic / logical commands
		case 0x27:
			strcpy(name, "daa");
			*cycles = 1, *length = 1;
			return;
		case 0x2f:
			strcpy(name, "cpl");
			*cycles = 1, *length = 1;
			return;
		// GMB 16-bit arithmetic / logical commands
		case 0xe8:
			sprintf(name, "add sp, %i", (s8)mem_readb(address + 1));
			*cycles = 4, *length = 2;
			return;
		case 0xf8:
			sprintf(name, "ld hl, sp + %i", (s8)mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		// GMB rotate and shift commands
		case 0x07:
			strcpy(name, "rlca");		// a <<= 1
			*cycles = 1, *length = 1;
			return;
		case 0x17:
			strcpy(name, "rla");		// a <<= 1 with carry
			*cycles = 1, *length = 1;
			return;
		case 0x0f:
			strcpy(name, "rrca");		// a >>= 1
			*cycles = 1, *length = 1;
			return;
		case 0x1f:
			strcpy(name, "rra");
			*cycles = 1, *length = 1;
			return;
		// GMB control commands
		case 0x3f:
			strcpy(name, "ccf");
			*cycles = 1, *length = 1;
			return;
		case 0x37:
			strcpy(name, "scf");
			*cycles = 1, *length = 1;
			return;
		case 0x00:
			strcpy(name, "nop");
			*cycles = 1, *length = 1;
			return;
		case 0x76:
			strcpy(name, "halt");
			*cycles = 1, *length = 1;
			return;
		case 0x10:
			strcpy(name, "stop");
			*cycles = 0, *length = 1;
			return;
		case 0xf3:
			strcpy(name, "di");
			*cycles = 1, *length = 1;
			return;
		case 0xfb:
			strcpy(name, "ei");
			*cycles = 1, *length = 1;
			return;
		case 0xc3:
			sprintf(name, "jp $%04x", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0xe9:
			strcpy(name, "jp hl");
			*cycles = 1, *length = 1;
			return;
		case 0x18:
			sprintf(name, "jr $%04x", address + 2 + (s8)mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0xcd:
			sprintf(name, "call $%04x", mem_readw(address + 1));
			*cycles = 6, *length = 3;
			return;
		case 0xc9:
			strcpy(name, "ret");
			*cycles = 4, *length = 1;
			return;
		case 0xd9:
			strcpy(name, "reti");
			*cycles = 4, *length = 1;
			return;
	}

	// 01 rrr sss -> ld r, s
	if ((opcode & 0300) == 0100) {
		sprintf(name, "ld %s, %s", reg_names[mid_digit], reg_names[low_digit]);
		*length = 1;
		*cycles = low_digit == 6 ? 2 : 1;		// (hl)
	}
	// 00 rrr 110 [nn] -> ld r, n
	else if ((opcode & 0307) == 0006) {
		sprintf(name, "ld %s, $%02x", reg_names[mid_digit], mem_readb(address + 1));
		*length = 2;
		*cycles = low_digit == 6 ? 3 : 2;		// (hl)
	}
	// 00 dd0 001 [nn nn] -> ld dd, nnnn (dd est une paire de registres)
	else if ((opcode & 0317) == 0001) {
		sprintf(name, "ld %s, $%04x", pair_names[mid_digit >> 1], mem_readw(address + 1));
		*length = 3, *cycles = 3;
	}
	// 11 qq0 101 -> push qq (qq est une paire de registres)
	else if ((opcode & 0317) == 0305) {
		sprintf(name, "push %s", stack_reg_names[mid_digit >> 1]);
		*length = 1, *cycles = 4;
	}
	// 11 qq0 001 -> pop qq (qq est une paire de registres)
	else if ((opcode & 0317) == 0301) {
		sprintf(name, "pop %s", stack_reg_names[mid_digit >> 1]);
		*length = 1, *cycles = 3;
	}
	// 10 ooo rrr -> ooo a, r
	else if ((opcode & 0300) == 0200) {
		sprintf(name, "%s %s", op_table[mid_digit], reg_names[low_digit]);
		*length = 1, *cycles = 1;
	}
	// 11 ooo 110 [nn] -> ooo nn
	else if ((opcode & 0307) == 0306) {
		sprintf(name, "%s $%02x", op_table[mid_digit], mem_readb(address + 1));
		*length = 2, *cycles = 2;
	}
	// 00 rrr 10o -> inc r si o=0, dec r sinon
	else if ((opcode & 0306) == 0004) {
		const char *op[2] = {"inc", "dec"};
		sprintf(name, "%s %s", op[low_digit & 1], reg_names[mid_digit]);
		*length = 1, *cycles = 1;
	}
	// 00 dd1 001 -> add hl, dd
	else if ((opcode & 0317) == 0011) {
		sprintf(name, "add hl, %s", pair_names[mid_digit >> 1]);
		*length = 1, *cycles = 2;
	}
	// 00 ddo 011 -> inc dd si o=0, dec dd sinon
	else if ((opcode & 0307) == 0003) {
		const char *op[2] = {"inc", "dec"};
		sprintf(name, "%s %s", op[mid_digit & 1], pair_names[mid_digit >> 1]);
		*length = 1, *cycles = 2;
	}
	// PAS SUR!! 11 0cc 010 [nn nn] -> jp cc, nn | le bit 2 de c est ignor�! Comment sur GB?
	else if ((opcode & 0347) == 0302) {
		sprintf(name, "jp %s, $%04x", conditions[mid_digit & 3], mem_readw(address + 1));
		// 4 cycles or 3 if false
		*length = 3, *cycles = 4;
	}
	// 00 1cc 000 [nn] -> jr cc, nn
	else if ((opcode & 0347) == 0040) {
		sprintf(name, "jr %s, $%04x", conditions[mid_digit & 3], address + 2 + (s8)mem_readb(address + 1));
		// 3 cycles or 2 if false
		*length = 2, *cycles = 3;
	}
	// 11 ccc 100 [nn] -> call cc, nn
	else if ((opcode & 0307) == 0304) {
		sprintf(name, "call %s, $%04x", conditions[mid_digit & 3], mem_readw(address + 1));
		// 6 cycles or 3 if false
		*length = 3, *cycles = 6;
	}
	// 11 0cc 000 -> ret cc
	else if ((opcode & 0347) == 0300) {
		sprintf(name, "ret %s", conditions[mid_digit & 3]);
		// 5 cycles or 2 if false
		*length = 1, *cycles = 5;
	}
	// 11 ttt 111 -> rst t
	else if ((opcode & 0307) == 0307) {
		sprintf(name, "rst $%02x", mid_digit * 8);
		*length = 1, *cycles = 4;
	}

	// Extended opcode -> c'est reparti pour un tour
	if (opcode == 0xcb) {
		const char *rs_command[8] = {"rlc", "rrc", "rl", "rr", "sla", "sra", "swap", "srl"};
		opcode = mem_readb(address + 1);
		upper_digit = (opcode >> 6) & 3, mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;
		// 00 ooo rrr -> rotate / shift commands avec registre
		if ((opcode & 0300) == 0) {
			sprintf(name, "%s %s", rs_command[mid_digit], reg_names[low_digit]);
			*length = 2;
			*cycles = low_digit == 6 ? 4 : 2;		// (HL) -> plus lent
		}
		// oo bbb rrr -> bit/set/res b, r (oo != 0)
		else {
			const char *bit_command[3] = {"bit", "res", "set"};
			sprintf(name, "%s %i, %s", bit_command[upper_digit - 1], mid_digit, reg_names[low_digit]);
			*length = 2;
			if (low_digit == 6)		// (HL)
				*cycles = upper_digit == 1 ? 3 : 4;
			else
				*cycles = 2;
		}
	}
}
